use std::os::raw::{c_int, c_long, c_void};

// Raw FFI to the vorbisfile API

#[link(name = "vorbis")]
extern "C" {
    // vorbisfile functions

    pub fn ov_open_callbacks(
        data_source: *mut c_void,
        vf: *mut OggVorbisFile,
        initial: *const u8,
        ibytes: c_long,
        callbacks: Callbacks,
    ) -> c_int;
    pub fn ov_pcm_total(vf: *mut OggVorbisFile, i: c_int) -> i64;
    pub fn ov_pcm_seek(vf: *mut OggVorbisFile, pos: i64) -> c_int;
    pub fn ov_info(vf: *mut OggVorbisFile, link: c_int) -> *const VorbisInfo;
    pub fn ov_read_float(
        vf: *mut OggVorbisFile,
        pcm_channels: *mut *const *const f32,
        samples: c_int,
        bitstream: *mut c_int,
    ) -> c_long;

    // Functions and constants defined by us in vorbis-utils.c

    pub fn vu_create_oggvorbisfile() -> *mut OggVorbisFile;
    pub fn vu_free_oggvorbisfile(vf: *mut OggVorbisFile);
    pub static vu_seek_origin_start: c_int;
    pub static vu_seek_origin_current: c_int;
    pub static vu_seek_origin_end: c_int;
}

#[repr(C)]
pub struct OggVorbisFile {
    private: [u8; 0],
}

#[repr(C)]
pub struct Callbacks {
    pub read_func: unsafe extern "C" fn(
        buffer: *mut c_void,
        element_size: usize,
        element_count: usize,
        data_source: *mut c_void,
    ) -> usize,
    pub seek_func: Option<
        unsafe extern "C" fn(data_source: *mut c_void, offset: i64, seek_origin: c_int) -> c_int,
    >,
    pub close_func: Option<unsafe extern "C" fn(data_source: *mut c_void) -> c_int>,
    pub tell_func: Option<unsafe extern "C" fn(data_source: *mut c_void) -> c_long>,
}

#[repr(C)]
pub struct VorbisInfo {
    pub encoder_version: c_int,
    pub channel_count: c_int,
    pub sampling_rate: c_int,
    pub bitrate_upper: c_long,
    pub bitrate_nominal: c_long,
    pub bitrate_lower: c_long,
    pub bitrate_window: c_long,
    codec_setup: *mut c_void,
}
