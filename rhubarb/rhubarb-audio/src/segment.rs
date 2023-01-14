use std::sync::Arc;

use crate::{
    sample_reader_assertions::SampleReaderAssertions, AudioClip, AudioError, SampleReader,
};

/// An audio clip representing a segment of another audio clip.
#[derive(Debug, Clone)]
pub struct Segment {
    inner_clip: Arc<dyn AudioClip>,
    start: u64,
    end: u64,
}

impl Segment {
    /// Creates a new audio clip from a segment of an existing audio clip.
    pub fn new(inner_clip: Box<dyn AudioClip>, start: u64, end: u64) -> Self {
        assert!(
            start <= end,
            "Start ({start}) must not be greater than end ({end})."
        );
        let inner_clip_len = inner_clip.len();
        assert!(
            end <= inner_clip_len,
            "Segment {start}..{end} exceeds {inner_clip_len}-frame audio clip.",
        );

        Self {
            inner_clip: Arc::from(inner_clip),
            start,
            end,
        }
    }
}

impl AudioClip for Segment {
    fn len(&self) -> u64 {
        self.end - self.start
    }

    fn sampling_rate(&self) -> u32 {
        self.inner_clip.sampling_rate()
    }

    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
        Ok(Box::new(SegmentSampleReader::new(self)?))
    }
}

#[derive(Debug)]
struct SegmentSampleReader {
    inner_sample_reader: Box<dyn SampleReader>,
    start: u64,
    end: u64,
}

impl SegmentSampleReader {
    fn new(segment: &Segment) -> Result<Self, AudioError> {
        let mut inner_sample_reader = segment.inner_clip.create_sample_reader()?;
        inner_sample_reader.set_position(segment.start);

        Ok(Self {
            inner_sample_reader,
            start: segment.start,
            end: segment.end,
        })
    }
}

impl SampleReader for SegmentSampleReader {
    fn len(&self) -> u64 {
        self.end - self.start
    }

    fn position(&self) -> u64 {
        self.inner_sample_reader.position() - self.start
    }

    fn set_position(&mut self, position: u64) {
        self.assert_valid_seek_position(position);
        self.inner_sample_reader.set_position(position + self.start)
    }

    fn read(&mut self, buffer: &mut [crate::Sample]) -> Result<(), crate::AudioError> {
        self.assert_valid_read_size(buffer);
        self.inner_sample_reader.read(buffer)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use rstest::*;
    use speculoos::prelude::*;

    use crate::{MemoryAudioClip, SampleReader};

    #[fixture]
    fn segment() -> Segment {
        let inner_clip = MemoryAudioClip::new(
            &[0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
            16000,
        );
        Segment::new(Box::new(inner_clip), 1, 9)
    }

    mod can_be_created_via_fluent_syntax {
        use super::*;
        use crate::AudioFilters;

        #[rstest]
        fn from_value() {
            let clip = MemoryAudioClip::new(&[0.0, 0.1, 0.2, 0.3, 0.4], 16000);
            clip.segment(1, 2);
        }

        #[rstest]
        fn from_box() {
            let clip = Box::new(MemoryAudioClip::new(&[0.0, 0.1, 0.2, 0.3, 0.4], 16000));
            clip.segment(1, 2);
        }
    }

    #[rstest]
    fn supports_debug(segment: Segment) {
        assert_that!(format!("{segment:?}"))
            .is_equal_to("Segment { inner_clip: MemoryAudioClip { buffer: 11 samples, sampling_rate: 16000 }, start: 1, end: 9 }".to_owned());
    }

    #[rstest]
    fn provides_length(segment: Segment) {
        assert_that!(segment.len()).is_equal_to(8);
    }

    #[rstest]
    fn provides_sampling_rate(segment: Segment) {
        assert_that!(segment.sampling_rate()).is_equal_to(16000);
    }

    #[rstest]
    fn supports_zero_samples() {
        let inner_clip = MemoryAudioClip::new(&[], 16000);
        let segment = Segment::new(Box::new(inner_clip), 0, 0);
        assert_that!(segment.len()).is_equal_to(0);
        assert_that!(segment.sampling_rate()).is_equal_to(16000);

        let mut sample_reader = segment.create_sample_reader().unwrap();
        let mut buffer = [0.0f32; 0];
        sample_reader.read(&mut buffer).unwrap();

        sample_reader.set_position(0);
    }

    mod sample_reader {
        use super::*;

        #[fixture]
        fn reader(segment: Segment) -> Box<dyn SampleReader> {
            segment.create_sample_reader().unwrap()
        }

        #[rstest]
        fn supports_debug(reader: Box<dyn SampleReader>) {
            assert_that!(format!("{reader:?}"))
                .is_equal_to("SegmentSampleReader { inner_sample_reader: MemorySampleReader { buffer: 11 samples, position: 1 }, start: 1, end: 9 }".to_owned());
        }

        #[rstest]
        fn provides_length(reader: Box<dyn SampleReader>) {
            assert_that!(reader.len()).is_equal_to(8);
        }

        #[rstest]
        fn position_is_initially_0(reader: Box<dyn SampleReader>) {
            assert_that!(reader.position()).is_equal_to(0);
        }

        #[rstest]
        fn reads_samples_up_to_the_end(mut reader: Box<dyn SampleReader>) {
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.1, 0.2, 0.3]);

            let mut five_samples = [0f32; 5];
            reader.read(&mut five_samples).unwrap();
            assert_that!(five_samples).is_equal_to([0.4, 0.5, 0.6, 0.7, 0.8]);
        }

        #[rstest]
        fn seeks(mut reader: Box<dyn SampleReader>) {
            reader.set_position(2);
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.3, 0.4, 0.5]);

            reader.set_position(1);
            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.2, 0.3, 0.4]);

            reader.read(&mut three_samples).unwrap();
            assert_that!(three_samples).is_equal_to([0.5, 0.6, 0.7]);
        }

        #[rstest]
        fn seeks_up_to_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(8);
            let mut zero_samples = [0f32; 0];
            reader.read(&mut zero_samples).unwrap();
        }

        #[rstest]
        #[should_panic(expected = "Attempting to read up to position 9 of 8-frame audio clip.")]
        fn reading_beyond_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(6);
            let mut three_samples = [0f32; 3];
            reader.read(&mut three_samples).unwrap();
        }

        #[rstest]
        #[should_panic(expected = "Attempting to seek to position 9 of 8-frame audio clip.")]
        fn seeking_beyond_the_end(mut reader: Box<dyn SampleReader>) {
            reader.set_position(9);
        }
    }
}
