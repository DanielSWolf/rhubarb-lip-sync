use byteorder::{ReadBytesExt, LE};
use derivative::Derivative;

use crate::{
    audio_clip::{AudioClip, SampleReader},
    audio_error::AudioError,
    sample_reader_assertions::SampleReaderAssertions,
    ReadAndSeek,
};
use std::{
    io::{self, SeekFrom},
    sync::Arc,
};

use super::wave_file_info::{get_wave_file_info, SampleFormat, WaveFileInfo};

/// An audio clip read on the fly from a WAVE file.
#[derive(Derivative)]
#[derivative(Debug)]
pub struct WaveAudioClip<TReader> {
    wave_file_info: WaveFileInfo,
    #[derivative(Debug = "ignore")]
    create_reader: Arc<dyn Fn() -> Result<TReader, io::Error>>,
}

impl<TReader> WaveAudioClip<TReader>
where
    TReader: ReadAndSeek,
{
    /// Creates a new `WaveAudioClip` for the reader returned by the given callback function.
    pub fn new(
        create_reader: Box<dyn Fn() -> Result<TReader, io::Error>>,
    ) -> Result<Self, AudioError> {
        Ok(Self {
            wave_file_info: get_wave_file_info(&mut create_reader()?)?,
            create_reader: Arc::from(create_reader),
        })
    }
}

impl<TReader> Clone for WaveAudioClip<TReader> {
    fn clone(&self) -> Self {
        Self {
            wave_file_info: self.wave_file_info,
            create_reader: self.create_reader.clone(),
        }
    }
}

impl<TReader> AudioClip for WaveAudioClip<TReader>
where
    TReader: ReadAndSeek + 'static,
{
    fn len(&self) -> u64 {
        self.wave_file_info.frame_count
    }

    fn sampling_rate(&self) -> u32 {
        self.wave_file_info.sampling_rate
    }

    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
        match self.wave_file_info.sample_format {
            SampleFormat::U8 => {
                const FACTOR: f32 = 1.0f32 / 0x80 as f32;
                let sample_reader = WaveFileSampleReader::new(self, |reader| {
                    Ok((i32::from(reader.read_u8()?) - 0x80) as f32 * FACTOR)
                })?;
                Ok(Box::new(sample_reader))
            }
            SampleFormat::I16 => {
                const FACTOR: f32 = 1.0f32 / 0x8000 as f32;
                let sample_reader = WaveFileSampleReader::new(self, |reader| {
                    Ok(f32::from(reader.read_i16::<LE>()?) * FACTOR)
                })?;
                Ok(Box::new(sample_reader))
            }
            SampleFormat::I24 => {
                const FACTOR: f32 = 1.0f32 / 0x800000 as f32;
                let sample_reader = WaveFileSampleReader::new(self, |reader| {
                    let mut buffer = [0; 3];
                    reader.read_exact(&mut buffer)?;
                    // Make sure the most significant byte is set correctly for two's complement
                    let i24 = ((u32::from(buffer[0]) << 8
                        | u32::from(buffer[1]) << 16
                        | u32::from(buffer[2]) << 24) as i32)
                        >> 8;
                    Ok(i24 as f32 * FACTOR)
                })?;
                Ok(Box::new(sample_reader))
            }
            SampleFormat::I32 => {
                const FACTOR: f32 = 1.0f32 / 0x80000000u32 as f32;
                let sample_reader = WaveFileSampleReader::new(self, |reader| {
                    Ok(reader.read_i32::<LE>()? as f32 * FACTOR)
                })?;
                Ok(Box::new(sample_reader))
            }
            SampleFormat::F32 => {
                let sample_reader =
                    WaveFileSampleReader::new(self, |reader| Ok(reader.read_f32::<LE>()?))?;
                Ok(Box::new(sample_reader))
            }
            SampleFormat::F64 => {
                let sample_reader =
                    WaveFileSampleReader::new(self, |reader| Ok(reader.read_f64::<LE>()? as f32))?;
                Ok(Box::new(sample_reader))
            }
        }
    }
}

#[derive(Derivative)]
#[derivative(Debug)]
struct WaveFileSampleReader<TReader, TReadSample>
where
    TReadSample: Fn(&mut TReader) -> Result<f32, AudioError>,
{
    wave_file_info: WaveFileInfo,
    #[derivative(Debug = "ignore")]
    reader: TReader,
    // the position we're claiming to be at
    logical_position: u64,
    // the position we're actually at
    physical_position: Option<u64>,
    #[derivative(Debug = "ignore")]
    read_sample: TReadSample,
}

impl<TReader, TReadSample> WaveFileSampleReader<TReader, TReadSample>
where
    TReader: ReadAndSeek,
    TReadSample: Fn(&mut TReader) -> Result<f32, AudioError>,
{
    fn new(
        audio_clip: &WaveAudioClip<TReader>,
        read_sample: TReadSample,
    ) -> Result<Self, AudioError> {
        let sample_reader = Self {
            wave_file_info: audio_clip.wave_file_info,
            reader: (audio_clip.create_reader)()?,
            logical_position: 0,
            physical_position: None,
            read_sample,
        };
        Ok(sample_reader)
    }

    fn seek_physically(&mut self) -> Result<(), AudioError> {
        if self.physical_position == Some(self.logical_position) {
            return Ok(());
        }

        self.reader.seek(SeekFrom::Start(
            self.wave_file_info.data_offset
                + self.logical_position * u64::from(self.wave_file_info.bytes_per_frame),
        ))?;
        self.physical_position = Some(self.logical_position);
        Ok(())
    }
}

impl<TReader, TReadSample> SampleReader for WaveFileSampleReader<TReader, TReadSample>
where
    TReader: ReadAndSeek,
    TReadSample: Fn(&mut TReader) -> Result<f32, AudioError>,
{
    fn len(&self) -> u64 {
        self.wave_file_info.frame_count
    }

    fn position(&self) -> u64 {
        self.logical_position
    }

    fn set_position(&mut self, position: u64) {
        self.assert_valid_seek_position(position);
        self.logical_position = position;
    }

    fn read(&mut self, buffer: &mut [crate::audio_clip::Sample]) -> Result<(), AudioError> {
        self.assert_valid_read_size(buffer);
        self.seek_physically()?;

        let channel_count = self.wave_file_info.channel_count;
        let factor = 1.0 / channel_count as f32;
        for sample in buffer {
            let mut sum: f32 = 0.0;
            for _ in 0..channel_count {
                sum += (self.read_sample)(&mut self.reader)?;
            }
            *sample = sum * factor;
        }
        Ok(())
    }
}
