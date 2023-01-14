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
mod audio_filters;
mod memory_audio_clip;
mod ogg_audio_clip;
mod open_audio_file;
mod read_and_seek;
mod sample_reader_assertions;
mod segment;
mod wave_audio_clip;

pub use audio_clip::{AudioClip, Sample, SampleReader};
pub use audio_error::AudioError;
pub use audio_filters::AudioFilters;
pub use memory_audio_clip::MemoryAudioClip;
pub use ogg_audio_clip::ogg_audio_clip::OggAudioClip;
pub use open_audio_file::{open_audio_file, open_audio_file_with_reader};
pub use read_and_seek::ReadAndSeek;
pub use segment::Segment;
pub use wave_audio_clip::wave_audio_clip::WaveAudioClip;
