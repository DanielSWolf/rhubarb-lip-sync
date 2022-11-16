use crate::{Sample, SampleReader};

pub trait SampleReaderAssertions {
    fn assert_valid_seek_position(&self, position: u64);
    fn assert_valid_read_size(&self, buffer: &[Sample]);
}

impl<TSampleReader: SampleReader> SampleReaderAssertions for TSampleReader {
    fn assert_valid_seek_position(&self, position: u64) {
        assert!(
            position <= self.len(),
            "Attempting to seek to position {} of {}-frame audio clip.",
            position,
            self.len()
        );
    }

    fn assert_valid_read_size(&self, buffer: &[Sample]) {
        let end = self.position() + buffer.len() as u64;
        assert!(
            end <= self.len(),
            "Attempting to read up to position {} of {}-frame audio clip.",
            end,
            self.len()
        );
    }
}
