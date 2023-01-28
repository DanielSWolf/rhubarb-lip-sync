// Raw FFI for libsamplerate

use std::os::raw::{c_int, c_long, c_void};

use crate::Sample;

#[link(name = "libsamplerate")]
extern "C" {
    pub fn src_callback_new(
        read_func: unsafe extern "C" fn(
            callback_data: *mut c_void,
            buffer: *mut *const Sample,
        ) -> c_long,
        converter_type: ConverterType,
        channel_count: c_int,
        error: *mut c_int,
        callback_data: *mut c_void,
    ) -> *mut State;

    pub fn src_callback_read(
        state: *mut State,
        ratio: f64,
        frame_count: c_long,
        buffer: *mut Sample,
    ) -> c_long;

    pub fn src_error(state: *mut State) -> c_int;

    pub fn src_reset(state: *mut State) -> c_int;

    // Always returns null
    pub fn src_delete(state: *mut State) -> *const c_void;
}

#[repr(C)]
#[allow(dead_code)]
pub enum ConverterType {
    /// Band-limited sinc interpolation, best quality, 144 dB SNR, 96% BW.
    SincBestQuality = 0,
    /// Band-limited sinc interpolation, medium quality, 121 dB SNR, 90% BW.
    SincMediumQuality = 1,
    /// Band-limited sinc interpolation, low quality, 97 dB SNR, 80% BW.
    SincFastest = 2,
    /// Zero order hold interpolator, very fast, poor quality.
    ZeroOrderHold = 3,
    /// Linear interpolator, blindingly fast, poor quality.
    Linear = 4,
}

#[repr(C)]
pub struct State {
    private: [u8; 0],
}
