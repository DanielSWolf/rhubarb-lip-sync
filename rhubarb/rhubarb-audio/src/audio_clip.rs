use crate::audio_error::AudioError;
use dyn_clone::DynClone;
use std::{fmt::Debug, time::Duration};

const NANOS_PER_SEC: u32 = 1_000_000_000;

/// An audio clip containing monaural sampled audio.
///
/// Structs implementing this trait may read the audio data from disk, keep it in memory, or
/// generate it on the fly.
pub trait AudioClip: DynClone + Debug {
    /// The number of audio frames in the audio clip.
    fn len(&self) -> u64;

    /// The sampling rate in frames per second.
    fn sampling_rate(&self) -> u32;

    /// Creates a new sample reader for reading from this audio clip.
    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError>;

    /// Returns the duration of this audio clip.
    fn duration(&self) -> Duration {
        Duration::from_nanos(
            (u128::from(self.len()) * u128::from(NANOS_PER_SEC) / u128::from(self.sampling_rate()))
                as u64,
        )
    }

    /// Indicates whether this audio clip is empty, that is, contains zero samples.
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

/// Allows seeking within an [AudioClip] and reading its samples.
pub trait SampleReader: Debug {
    /// The number of audio frames in the associated audio clip.
    fn len(&self) -> u64;

    /// The current read position in frames.
    fn position(&self) -> u64;

    /// Seeks to the specified position in frames.
    ///
    /// *Performance note:* Implementers of source sample readers should make sure that this method
    /// doesn't perform any time-consuming work such as reading from the disk. This allows
    /// downstream sample readers to unconditionally call `seek` upstream without worrying about
    /// performance.
    fn set_position(&mut self, position: u64);

    /// Attempts to read samples until the given buffer is full.
    /// Errors if there are not enough samples left to fill the buffer.
    fn read(&mut self, buffer: &mut [Sample]) -> Result<(), AudioError>;

    /// Indicates whether the associated audio clip is empty, that is, contains zero samples.
    fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// The number of remaining samples before the end of the audio clip is reached.
    fn remainder(&self) -> u64 {
        self.len() - self.position()
    }
}

/// An audio sample in the range [-1, 1].
pub type Sample = f32;

#[cfg(test)]
mod tests {
    use super::*;
    use rstest::*;
    use speculoos::prelude::*;

    mod audio_clip {
        use super::*;

        #[derive(Clone, Debug)]
        struct MockAudioClip {
            len: u64,
            sampling_rate: u32,
        }

        impl AudioClip for MockAudioClip {
            fn len(&self) -> u64 {
                self.len
            }

            fn sampling_rate(&self) -> u32 {
                self.sampling_rate
            }

            fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
                todo!()
            }
        }

        #[rstest]
        fn provides_duration() {
            let mut clip = MockAudioClip {
                len: 0,
                sampling_rate: 44100,
            };
            assert_that!(clip.duration()).is_equal_to(Duration::ZERO);

            clip.len = 4410;
            assert_that!(clip.duration()).is_equal_to(Duration::from_millis(100));

            clip.len = 1;
            clip.sampling_rate = 1_000_000;
            assert_that!(clip.duration()).is_equal_to(Duration::from_micros(1));
        }

        #[rstest]
        fn provides_is_empty() {
            let mut clip = MockAudioClip {
                len: 0,
                sampling_rate: 44100,
            };
            assert_that!(clip.is_empty()).is_true();

            clip.len = 1;
            assert_that!(clip.is_empty()).is_false();

            clip.sampling_rate = u32::MAX;
            assert_that!(clip.is_empty()).is_false();
        }
    }

    mod sample_reader {
        use super::*;

        #[derive(Debug)]
        struct MockSampleReader {
            pos: u64,
            len: u64,
        }

        impl SampleReader for MockSampleReader {
            fn len(&self) -> u64 {
                self.len
            }

            fn position(&self) -> u64 {
                self.pos
            }

            fn set_position(&mut self, _position: u64) {
                todo!()
            }

            fn read(&mut self, _buffer: &mut [Sample]) -> Result<(), AudioError> {
                todo!()
            }
        }

        #[rstest]
        fn provides_is_empty() {
            let mut sample_reader = MockSampleReader { pos: 0, len: 0 };
            assert_that!(sample_reader.is_empty()).is_true();

            sample_reader.len = 1;
            assert_that!(sample_reader.is_empty()).is_false();
        }

        #[rstest]
        fn provides_remainder() {
            let mut sample_reader = MockSampleReader { pos: 0, len: 0 };
            assert_that!(sample_reader.remainder()).is_equal_to(0);

            sample_reader.len = 10;
            assert_that!(sample_reader.remainder()).is_equal_to(10);

            sample_reader.pos = 8;
            assert_that!(sample_reader.remainder()).is_equal_to(2);
        }
    }
}
