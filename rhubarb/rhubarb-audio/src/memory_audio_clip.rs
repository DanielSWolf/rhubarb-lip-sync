use crate::{sample_reader_assertions::SampleReaderAssertions, AudioClip, Sample, SampleReader};
use derivative::Derivative;
use std::fmt::{self, Formatter};
use std::sync::Arc;

/// An audio clip representing audio samples in memory.
///
/// *Note:* Cloned instances share the same audio buffer, so cloning is cheap.
#[derive(Derivative)]
#[derivative(Clone, Debug)]
pub struct MemoryAudioClip {
    #[derivative(Debug(format_with = "format_buffer"))]
    buffer: Arc<Vec<Sample>>,
    sampling_rate: u32,
}

impl MemoryAudioClip {
    /// Creates a new memory audio clip.
    pub fn new(samples: &[Sample], sampling_rate: u32) -> Self {
        Self {
            buffer: Arc::new(samples.to_vec()),
            sampling_rate,
        }
    }
}

impl AudioClip for MemoryAudioClip {
    fn len(&self) -> u64 {
        self.buffer.len() as u64
    }

    fn sampling_rate(&self) -> u32 {
        self.sampling_rate
    }

    fn create_sample_reader(&self) -> Result<Box<dyn crate::SampleReader>, crate::AudioError> {
        Ok(Box::new(MemorySampleReader {
            buffer: self.buffer.clone(),
            position: 0,
        }))
    }
}

#[derive(Derivative)]
#[derivative(Debug)]
struct MemorySampleReader {
    #[derivative(Debug(format_with = "format_buffer"))]
    buffer: Arc<Vec<Sample>>,
    position: u64,
}

impl SampleReader for MemorySampleReader {
    fn len(&self) -> u64 {
        self.buffer.len() as u64
    }

    fn position(&self) -> u64 {
        self.position
    }

    fn set_position(&mut self, position: u64) {
        self.assert_valid_seek_position(position);
        self.position = position;
    }

    fn read(&mut self, buffer: &mut [Sample]) -> Result<(), crate::AudioError> {
        self.assert_valid_read_size(buffer);

        let start = self.position as usize;
        buffer.copy_from_slice(&self.buffer[start..start + buffer.len()]);
        self.position += buffer.len() as u64;
        Ok(())
    }
}

fn format_buffer(buffer: &Arc<Vec<Sample>>, f: &mut Formatter<'_>) -> fmt::Result {
    write!(f, "{:?} samples", buffer.len())
}

#[cfg(test)]
mod tests {
    use super::*;
    use rstest::*;
    use speculoos::prelude::*;

    use crate::{AudioClip, SampleReader};

    #[fixture]
    fn clip() -> MemoryAudioClip {
        MemoryAudioClip::new(&[0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9], 16000)
    }

    #[rstest]
    fn supports_debug(clip: MemoryAudioClip) {
        assert_that!(format!("{clip:?}"))
            .is_equal_to("MemoryAudioClip { buffer: 10 samples, sampling_rate: 16000 }".to_owned());
    }

    #[rstest]
    fn provides_length(clip: MemoryAudioClip) {
        assert_that!(clip.len()).is_equal_to(10);
    }

    #[rstest]
    fn provides_sampling_rate(clip: MemoryAudioClip) {
        assert_that!(clip.sampling_rate()).is_equal_to(16000);
    }

    #[rstest]
    fn clone_reuses_buffer(clip: MemoryAudioClip) {
        let clone = clip.clone();
        assert_that!(clone.buffer.as_ptr()).is_equal_to(clip.buffer.as_ptr());
    }

    #[rstest]
    fn supports_zero_samples() {
        let clip = MemoryAudioClip::new(&[], 16000);
        assert_that!(clip.len()).is_equal_to(0);
        assert_that!(clip.sampling_rate()).is_equal_to(16000);

        let mut sample_reader = clip.create_sample_reader().unwrap();
        let mut buffer = [0.0f32; 0];
        sample_reader.read(&mut buffer).unwrap();

        sample_reader.set_position(0);
    }

    mod sample_reader {
        use super::*;

        #[fixture]
        fn reader(clip: MemoryAudioClip) -> Box<dyn SampleReader> {
            clip.create_sample_reader().unwrap()
        }

        #[rstest]
        fn supports_debug(reader: Box<dyn SampleReader>) {
            assert_that!(format!("{reader:?}"))
                .is_equal_to("MemorySampleReader { buffer: 10 samples, position: 0 }".to_owned());
        }

        #[rstest]
        fn provides_length(reader: Box<dyn SampleReader>) {
            assert_that!(reader.len()).is_equal_to(10);
        }

        #[rstest]
        fn position_is_initially_0(reader: Box<dyn SampleReader>) {
            assert_that!(reader.position()).is_equal_to(0);
        }

        #[rstest]
        fn reads_samples_up_to_the_end(mut reader: Box<dyn SampleReader>) {
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.0, 0.1, 0.2]);

            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.3, 0.4, 0.5]);

            let mut four_samples = vec![0f32; 4];
            reader.read(&mut four_samples).unwrap();
            assert_that!(four_samples).is_equal_to(vec![0.6, 0.7, 0.8, 0.9]);
        }

        #[rstest]
        fn seeks(mut reader: Box<dyn SampleReader>) {
            reader.set_position(2);
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.2, 0.3, 0.4]);

            reader.set_position(1);
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.1, 0.2, 0.3]);

            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.4, 0.5, 0.6]);
        }

        #[rstest]
        fn seeks_up_to_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(10);
            let mut zero_samples = [0f32; 0];
            reader.read(&mut zero_samples).unwrap();
        }

        #[rstest]
        #[should_panic(expected = "Attempting to read up to position 11 of 10-frame audio clip.")]
        fn reading_beyond_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(8);
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
        }

        #[rstest]
        #[should_panic(expected = "Attempting to seek to position 11 of 10-frame audio clip.")]
        fn seeking_beyond_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(11);
        }
    }
}
