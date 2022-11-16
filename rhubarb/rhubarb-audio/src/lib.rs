#![warn(
    rust_2018_idioms,
    missing_debug_implementations,
    missing_docs,
    clippy::cast_lossless,
    clippy::checked_conversions,
    clippy::ptr_as_ptr,
    clippy::unnecessary_self_imports,
    clippy::use_self
)]
#![allow(clippy::module_inception)]

//! Audio library for use in Rhubarb Lip Sync.

mod audio_clip;
mod audio_error;

pub use audio_clip::{AudioClip, Sample, SampleReader};
pub use audio_error::AudioError;
