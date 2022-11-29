use std::{
    fmt::Debug,
    io::{self, Read, Seek, SeekFrom},
    mem::MaybeUninit,
    pin::Pin,
    ptr::{null, null_mut},
    slice,
};

use std::os::raw::{c_int, c_long, c_void};

use crate::{AudioError, ReadAndSeek, Sample};

use super::vorbis_file_raw::{
    self as raw, ov_info, ov_open_callbacks, ov_pcm_seek, ov_pcm_total, ov_read_float,
    vu_create_oggvorbisfile, vu_free_oggvorbisfile, vu_seek_origin_current, vu_seek_origin_end,
    vu_seek_origin_start,
};

/// A safe wrapper around the vorbisfile API.
pub struct VorbisFile {
    vf: *mut raw::OggVorbisFile,
    reader: Box<dyn ReadAndSeek>,
    last_error: Option<io::Error>,
    cached_metadata: Option<Metadata>,
}

impl VorbisFile {
    pub fn new(reader: impl ReadAndSeek + 'static) -> Result<Pin<Box<Self>>, AudioError> {
        let vf = unsafe { vu_create_oggvorbisfile() };
        assert!(!vf.is_null(), "Error creating raw OggVorbisFile.");

        // Pin the struct so that the pointer we pass for the callbacks stays valid
        let mut vorbis_file = Pin::new(Box::new(Self {
            vf,
            reader: Box::new(reader),
            last_error: None,
            cached_metadata: None,
        }));

        let callbacks = raw::Callbacks {
            read_func,
            seek_func: Some(seek_func),
            close_func: None, // The reader will be closed by Rust
            tell_func: Some(tell_func),
        };
        unsafe {
            let result_code = ov_open_callbacks(
                &*vorbis_file as *const Self as *mut c_void,
                vf,
                null(),
                0,
                callbacks,
            );
            vorbis_file.handle_error(result_code)?;
        }
        Ok(vorbis_file)
    }

    pub fn metadata(&mut self) -> Result<Metadata, AudioError> {
        if self.cached_metadata.is_some() {
            return Ok(self.cached_metadata.unwrap());
        }

        unsafe {
            let vorbis_info = ov_info(self.vf, -1);
            assert!(!vorbis_info.is_null(), "Error retrieving Vorbis info.");

            let metadata = Metadata {
                frame_count: self.handle_error(ov_pcm_total(self.vf, -1))? as u64,
                sampling_rate: (*vorbis_info).sampling_rate as u32,
                channel_count: (*vorbis_info).channel_count as u32,
            };
            self.cached_metadata = Some(metadata);
            Ok(metadata)
        }
    }

    pub fn seek(&mut self, position: u64) -> Result<(), AudioError> {
        unsafe {
            self.handle_error(ov_pcm_seek(self.vf, position as i64))
                .map(|_| ())
        }
    }

    pub fn read(
        &mut self,
        target_buffer: &mut [Sample],
        callback: impl Fn(&Vec<&[Sample]>, u64, &mut [Sample]),
    ) -> Result<u64, AudioError> {
        let channel_count = self.metadata()?.channel_count;

        unsafe {
            // Read to multi-channel buffer
            let mut buffer = MaybeUninit::uninit();
            let read_frame_count = self.handle_error(ov_read_float(
                self.vf,
                buffer.as_mut_ptr(),
                target_buffer.len().clamp(0, i32::MAX as usize) as i32,
                null_mut(),
            ))? as u64;
            let multi_channel_buffer = buffer.assume_init();

            // Transform to vector of slices
            let mut channels = Vec::<&[Sample]>::new();
            for channel_index in 0..channel_count as usize {
                let channel_buffer = *multi_channel_buffer.add(channel_index);
                channels.push(slice::from_raw_parts(
                    channel_buffer,
                    read_frame_count as usize,
                ));
            }

            callback(&channels, read_frame_count, target_buffer);
            Ok(read_frame_count)
        }
    }

    fn handle_error<T>(&mut self, result_code: T) -> Result<T, AudioError>
    where
        T: Copy + TryInto<i32> + PartialOrd<T> + Default,
        <T as TryInto<i32>>::Error: Debug,
    {
        // Constants from vorbis's codec.h file
        const OV_HOLE: c_int = -3;
        const OV_EREAD: c_int = -128;
        const OV_EFAULT: c_int = -129;
        const OV_EIMPL: c_int = -130;
        const OV_EINVAL: c_int = -131;
        const OV_ENOTVORBIS: c_int = -132;
        const OV_EBADHEADER: c_int = -133;
        const OV_EVERSION: c_int = -134;
        const OV_ENOTAUDIO: c_int = -135;
        const OV_EBADPACKET: c_int = -136;
        const OV_EBADLINK: c_int = -137;
        const OV_ENOSEEK: c_int = -138;

        if result_code >= Default::default() {
            // A non-negative value is always valid
            return Ok(result_code);
        }

        let error_code: i32 = result_code.try_into().expect("Error code out of range.");
        if error_code == OV_HOLE {
            // OV_HOLE, though technically an error code, is only informational
            return Ok(result_code);
        }

        // If we captured a Rust error object, it is probably more precise than an error code
        if let Some(last_error) = self.last_error.take() {
            return Err(AudioError::IoError(last_error));
        }

        // The call failed. Handle the error.
        match error_code {
            OV_EREAD => Err(AudioError::IoError(io::Error::new(
                io::ErrorKind::Other,
                "Read error while fetching compressed data for decoding.".to_owned(),
            ))),
            OV_EFAULT => panic!("Internal logic fault; indicates a bug or heap/stack corruption."),
            OV_EIMPL => panic!("Feature not implemented."),
            OV_EINVAL => panic!(
                "Either an invalid argument, or incompletely initialized argument passed to a call."
            ),
            OV_ENOTVORBIS => Err(AudioError::CorruptFile(
                "The given file was not recognized as Ogg Vorbis data.".to_owned(),
            )),
            OV_EBADHEADER => Err(AudioError::CorruptFile(
                "Ogg Vorbis stream contains a corrupted or undecipherable header.".to_owned(),
            )),

            OV_EVERSION => Err(AudioError::UnsupportedFileFeature(
                "Unsupported bit stream format revision.".to_owned(),
            )),
            OV_ENOTAUDIO => Err(AudioError::UnsupportedFileFeature(
                "Packet is not an audio packet.".to_owned(),
            )),
            OV_EBADPACKET => Err(AudioError::CorruptFile("Error in packet.".to_owned())),
            OV_EBADLINK => Err(AudioError::CorruptFile(
                "Link in Vorbis data stream is not decipherable due to garbage or corruption."
                    .to_owned(),
            )),
            OV_ENOSEEK => {
                // This would indicate a bug, since we're implementing the stream ourselves
                panic!("The given stream is not seekable.");
            }
            _ => panic!("An unexpected Vorbis error with code {error_code} occurred."),
        }
    }
}

impl Drop for VorbisFile {
    fn drop(&mut self) {
        unsafe {
            vu_free_oggvorbisfile(self.vf);
        }
    }
}

#[derive(Copy, Clone, Debug)]
pub struct Metadata {
    pub frame_count: u64,
    pub sampling_rate: u32,
    pub channel_count: u32,
}

unsafe extern "C" fn read_func(
    buffer: *mut c_void,
    element_size: usize,
    element_count: usize,
    data_source: *mut c_void,
) -> usize {
    let vorbis_file = data_source.cast::<VorbisFile>();
    (*vorbis_file).last_error = None;

    let requested_byte_count = element_count * element_size;
    let mut remaining_buffer = slice::from_raw_parts_mut(buffer.cast::<u8>(), requested_byte_count);
    while !remaining_buffer.is_empty() {
        let read_result = (*vorbis_file).reader.read(remaining_buffer);
        match read_result {
            Ok(read_bytes) => {
                if read_bytes == 0 {
                    break;
                }
                remaining_buffer = remaining_buffer.split_at_mut(read_bytes).1;
            }
            Err(error) => {
                (*vorbis_file).last_error = Some(error);
                break;
            }
        }
    }
    requested_byte_count - remaining_buffer.len()
}

unsafe extern "C" fn seek_func(data_source: *mut c_void, offset: i64, seek_origin: c_int) -> c_int {
    let vorbis_file = data_source.cast::<VorbisFile>();
    (*vorbis_file).last_error = None;

    let seek_from = if seek_origin == vu_seek_origin_start {
        SeekFrom::Start(offset as u64)
    } else if seek_origin == vu_seek_origin_current {
        SeekFrom::Current(offset)
    } else if seek_origin == vu_seek_origin_end {
        SeekFrom::End(offset)
    } else {
        panic!("Invalid seek origin {seek_origin}.");
    };

    let seek_result = (*vorbis_file).reader.seek(seek_from);
    match seek_result {
        Ok(_) => 0,
        Err(error) => {
            (*vorbis_file).last_error = Some(error);
            -1
        }
    }
}

unsafe extern "C" fn tell_func(data_source: *mut c_void) -> c_long {
    let vorbis_file = data_source.cast::<VorbisFile>();
    (*vorbis_file).last_error = None;

    let position_result = (*vorbis_file).reader.stream_position();
    match position_result {
        Ok(position) => position as c_long,
        Err(error) => {
            (*vorbis_file).last_error = Some(error);
            -1
        }
    }
}
