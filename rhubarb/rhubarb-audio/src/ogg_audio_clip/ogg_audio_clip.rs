use std::{io, pin::Pin, sync::Arc};

use derivative::Derivative;

use crate::{
    sample_reader_assertions::SampleReaderAssertions, AudioClip, AudioError, ReadAndSeek,
    SampleReader,
};

use super::vorbis_file::{Metadata, VorbisFile};

/// An audio clip read on the fly from an Ogg Vorbis file.
#[derive(Derivative)]
#[derivative(Debug)]
pub struct OggAudioClip<TReader> {
    metadata: Metadata,
    #[derivative(Debug = "ignore")]
    create_reader: Arc<dyn Fn() -> Result<TReader, io::Error>>,
}

impl<TReader> OggAudioClip<TReader>
where
    TReader: ReadAndSeek + 'static,
{
    /// Creates a new `OggAudioClip` for the reader returned by the given callback function.
    pub fn new(
        create_reader: Box<dyn Fn() -> Result<TReader, io::Error>>,
    ) -> Result<Self, AudioError> {
        let mut vorbis_file = VorbisFile::new(create_reader()?)?;
        let metadata = vorbis_file.metadata()?;
        Ok(Self {
            metadata,
            create_reader: Arc::from(create_reader),
        })
    }
}

impl<TReader> Clone for OggAudioClip<TReader> {
    fn clone(&self) -> Self {
        Self {
            metadata: self.metadata,
            create_reader: self.create_reader.clone(),
        }
    }
}

impl<TReader> AudioClip for OggAudioClip<TReader>
where
    TReader: ReadAndSeek + 'static,
{
    fn len(&self) -> u64 {
        self.metadata.frame_count
    }

    fn sampling_rate(&self) -> u32 {
        self.metadata.sampling_rate
    }

    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
        Ok(Box::new(OggFileSampleReader::new(self)?))
    }
}

#[derive(Derivative)]
#[derivative(Debug)]
struct OggFileSampleReader {
    #[derivative(Debug = "ignore")]
    vorbis_file: Pin<Box<VorbisFile>>,
    metadata: Metadata,
    // the position we're claiming to be at
    logical_position: u64,
    // the position we're actually at
    physical_position: u64,
}

impl OggFileSampleReader {
    fn new<TReader>(audio_clip: &OggAudioClip<TReader>) -> Result<Self, AudioError>
    where
        TReader: ReadAndSeek + 'static,
    {
        let vorbis_file = VorbisFile::new((audio_clip.create_reader)()?)?;
        Ok(Self {
            vorbis_file,
            metadata: audio_clip.metadata,
            logical_position: 0,
            physical_position: 0,
        })
    }
}

impl SampleReader for OggFileSampleReader {
    fn len(&self) -> u64 {
        self.metadata.frame_count
    }

    fn position(&self) -> u64 {
        self.logical_position
    }

    fn set_position(&mut self, position: u64) {
        self.assert_valid_seek_position(position);
        self.logical_position = position
    }

    fn read(&mut self, buffer: &mut [crate::Sample]) -> Result<(), AudioError> {
        self.assert_valid_read_size(buffer);

        if self.physical_position != self.logical_position {
            self.vorbis_file.seek(self.logical_position)?;
            self.physical_position = self.logical_position;
        }

        let mut remaining_buffer = buffer;
        let factor = 1.0f32 / self.metadata.channel_count as f32;
        while !remaining_buffer.is_empty() {
            let read_frame_count = self.vorbis_file.read(
                remaining_buffer,
                |channels, frame_count, target_buffer| {
                    // Downmix channels to output buffer
                    for frame_index in 0..frame_count as usize {
                        let mut sum = 0f32;
                        for channel in channels {
                            sum += channel[frame_index];
                        }
                        target_buffer[frame_index] = sum * factor;
                    }
                },
            )?;
            remaining_buffer = remaining_buffer.split_at_mut(read_frame_count as usize).1;
        }
        Ok(())
    }
}
