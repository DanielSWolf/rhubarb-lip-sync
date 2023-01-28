use crate::{
    resampled_audio_clip::libsamplerate_raw::src_error,
    sample_reader_assertions::SampleReaderAssertions, AudioClip, AudioError, Sample, SampleReader,
};
use std::{
    cmp::min,
    mem::MaybeUninit,
    os::raw::{c_long, c_void},
    pin::Pin,
    sync::Arc,
};

use super::libsamplerate_raw::{
    src_callback_new, src_callback_read, src_delete, src_reset, ConverterType, State,
};

/// A resampled representation of another audio clip.
#[derive(Debug, Clone)]
pub struct ResampledAudioClip {
    inner_clip: Arc<dyn AudioClip>,
    sampling_rate: u32,
}

impl ResampledAudioClip {
    pub fn new(inner_clip: Box<dyn AudioClip>, sampling_rate: u32) -> Self {
        Self {
            inner_clip: Arc::from(inner_clip),
            sampling_rate,
        }
    }

    fn factor(&self) -> f64 {
        f64::from(self.sampling_rate) / f64::from(self.inner_clip.sampling_rate())
    }
}

impl AudioClip for ResampledAudioClip {
    fn len(&self) -> u64 {
        (self.inner_clip.len() as f64 * self.factor()).round() as u64
    }

    fn sampling_rate(&self) -> u32 {
        self.sampling_rate
    }

    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
        Ok(Box::new(ResampledSampleReader::new(self)?))
    }
}

#[derive(Debug)]
struct ResampledSampleReader {
    callback_data: Pin<Box<CallbackData>>,
    factor: f64,
    len: u64,
    position: u64,
    position_changed: bool,
    libsamplerate_state: *mut State,
}

const INPUT_BUFFER_SIZE: usize = 500;

#[derive(Debug)]
struct CallbackData {
    inner_sample_reader: Box<dyn SampleReader>,
    input_buffer: [Sample; INPUT_BUFFER_SIZE],
    last_error: Option<AudioError>,
}

impl ResampledSampleReader {
    fn new(clip: &ResampledAudioClip) -> Result<Self, AudioError> {
        let callback_data = Box::pin(CallbackData {
            inner_sample_reader: clip.inner_clip.create_sample_reader()?,
            input_buffer: [0.0; INPUT_BUFFER_SIZE],
            last_error: None,
        });
        let channel_count = 1;
        let mut error = MaybeUninit::uninit();
        let libsamplerate_state = unsafe {
            src_callback_new(
                read,
                ConverterType::SincMediumQuality,
                channel_count,
                error.as_mut_ptr(),
                &*callback_data as *const CallbackData as *mut c_void,
            )
        };
        assert_eq!(unsafe { error.assume_init() }, 0);
        Ok(Self {
            callback_data,
            factor: clip.factor(),
            len: clip.len(),
            position: 0,
            position_changed: false,
            libsamplerate_state,
        })
    }
}

impl Drop for ResampledSampleReader {
    fn drop(&mut self) {
        unsafe {
            src_delete(self.libsamplerate_state);
        }
    }
}

impl SampleReader for ResampledSampleReader {
    fn len(&self) -> u64 {
        self.len
    }

    fn position(&self) -> u64 {
        self.position
    }

    fn set_position(&mut self, position: u64) {
        if position == self.position {
            return;
        }

        self.assert_valid_seek_position(position);
        self.position = position;
        self.position_changed = true;
    }

    fn read(&mut self, buffer: &mut [Sample]) -> Result<(), AudioError> {
        self.assert_valid_read_size(buffer);
        if self.position_changed {
            self.callback_data
                .inner_sample_reader
                .set_position((self.position as f64 / self.factor) as u64);
            let error = unsafe { src_reset(self.libsamplerate_state) };
            assert_eq!(error, 0);
            self.position_changed = false;
        }

        let samples_read = unsafe {
            src_callback_read(
                self.libsamplerate_state,
                self.factor,
                buffer.len() as i32,
                buffer.as_mut_ptr(),
            ) as usize
        };
        if let Some(error) = self.callback_data.last_error.take() {
            return Err(error);
        }
        if samples_read < buffer.len() {
            assert_eq!(unsafe { src_error(self.libsamplerate_state) }, 0);
            panic!("Expected {} samples, got {}.", buffer.len(), samples_read);
        }
        self.position += buffer.len() as u64;

        Ok(())
    }
}

unsafe extern "C" fn read(callback_data: *mut c_void, buffer: *mut *const Sample) -> c_long {
    let callback_data = callback_data.cast::<CallbackData>();
    let inner_sample_reader = &mut (*callback_data).inner_sample_reader;
    let sample_count = min(inner_sample_reader.remainder(), INPUT_BUFFER_SIZE as u64) as usize;

    // Allow reading beyond the end of the audio clip by filling with zeros
    let (sample_buffer, zero_buffer) = (*callback_data).input_buffer.split_at_mut(sample_count);
    let read_result = inner_sample_reader.read(sample_buffer);
    if let Err(error) = read_result {
        (*callback_data).last_error = Some(error);
        return 0;
    }
    zero_buffer.fill(0.0);

    *buffer = sample_buffer.as_ptr();
    sample_buffer.len() as c_long
}
