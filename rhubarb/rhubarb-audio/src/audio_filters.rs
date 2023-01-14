use crate::{AudioClip, Segment};

/// Blanket implementations of audio filters for audio clips.
pub trait AudioFilters {
    /// Returns a new audio clip containing a segment of the specified clip.
    fn segment(self, start: u64, end: u64) -> Box<dyn AudioClip>;
}

impl AudioFilters for Box<dyn AudioClip> {
    fn segment(self, start: u64, end: u64) -> Box<dyn AudioClip> {
        Box::new(Segment::new(self, start, end))
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
}
