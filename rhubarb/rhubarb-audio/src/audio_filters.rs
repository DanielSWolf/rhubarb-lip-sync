use crate::{resampled_audio_clip::ResampledAudioClip, AudioClip, Segment};

/// Blanket implementations of audio filters for audio clips.
pub trait AudioFilters {
    /// Returns a new audio clip containing a segment of the specified clip.
    fn segment(self, start: u64, end: u64) -> Box<dyn AudioClip>;

    /// Returns a new audio clip resampled to the specified sampling rate.
    fn resampled(self, sampling_rate: u32) -> Box<dyn AudioClip>;
}

impl AudioFilters for Box<dyn AudioClip> {
    fn segment(self, start: u64, end: u64) -> Box<dyn AudioClip> {
        Box::new(Segment::new(self, start, end))
    }

    fn resampled(self, sampling_rate: u32) -> Box<dyn AudioClip> {
        Box::new(ResampledAudioClip::new(self, sampling_rate))
    }
}

impl<TAudioClip> AudioFilters for TAudioClip
where
    TAudioClip: AudioClip + 'static,
{
    fn segment(self, start: u64, end: u64) -> Box<dyn AudioClip> {
        let boxed_audio_clip: Box<dyn AudioClip> = Box::new(self);
        boxed_audio_clip.segment(start, end)
    }

    fn resampled(self, sampling_rate: u32) -> Box<dyn AudioClip> {
        let boxed_audio_clip: Box<dyn AudioClip> = Box::new(self);
        boxed_audio_clip.resampled(sampling_rate)
    }
}
