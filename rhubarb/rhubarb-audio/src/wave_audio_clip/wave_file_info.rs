use super::{codecs::codec_to_string, four_cc::ReadFourCcExt};
use crate::{audio_error::AudioError, wave_audio_clip::four_cc::four_cc_to_string, ReadAndSeek};
use byteorder::{ReadBytesExt, LE};
use std::io::SeekFrom;

#[derive(Debug, Copy, Clone)]
pub enum SampleFormat {
    U8,
    I16,
    I24,
    I32,
    F32,
    F64,
}

#[derive(Debug, Copy, Clone)]
pub struct WaveFileInfo {
    pub sample_format: SampleFormat,
    pub channel_count: u32,
    pub sampling_rate: u32,
    pub frame_count: u64,
    pub bytes_per_frame: u32,
    // The offset, in bytes, of the raw audio data within the file
    pub data_offset: u64,
}

struct FormatInfo {
    sample_format: SampleFormat,
    channel_count: u32,
    frame_rate: u32,
    bytes_per_frame: u32,
}

struct DataInfo {
    data_offset: u64,
    data_byte_count: u64,
}

mod codecs {
    pub const PCM: u16 = 0x0001;
    pub const FLOAT: u16 = 0x0003;
    pub const EXTENSIBLE: u16 = 0xFFFE;
}

pub fn get_wave_file_info(reader: &mut impl ReadAndSeek) -> Result<WaveFileInfo, AudioError> {
    let master_chunk_id = reader.read_four_cc()?;
    if &master_chunk_id != b"RIFF" {
        return Err(AudioError::CorruptFile(format!(
            "Expected master chunk ID \"RIFF\", got {:?}.",
            four_cc_to_string(&master_chunk_id)
        )));
    }

    reader.read_u32::<LE>()?; // Skip chunk size

    let wave_chunk_id = reader.read_four_cc()?;
    if &wave_chunk_id != b"WAVE" {
        return Err(AudioError::CorruptFile(format!(
            "Expected WAVE chunk ID \"WAVE\", got {:?}.",
            four_cc_to_string(&wave_chunk_id),
        )));
    }

    let mut format_info: Option<FormatInfo> = None;
    let mut data_info: Option<DataInfo> = None;

    while format_info.is_none() || data_info.is_none() {
        let chunk_id = reader.read_four_cc()?;
        let chunk_size = reader.read_u32::<LE>()?;
        let chunk_end = round_up_to_even(reader.stream_position()? + u64::from(chunk_size));

        match &chunk_id {
            b"fmt " => {
                // Format chunk
                let mut codec = reader.read_u16::<LE>()?;
                let channel_count = reader.read_u16::<LE>()?;
                let frame_rate = reader.read_u32::<LE>()?;
                reader.read_u32::<LE>()?; // Skip bytes per second
                let bytes_per_frame = reader.read_u16::<LE>()?;
                let mut bits_per_sample = reader.read_u16::<LE>()?;
                if chunk_size > 16 {
                    let extension_size = reader.read_u16::<LE>()?;
                    if extension_size >= 22 {
                        // Read extension fields
                        bits_per_sample = reader.read_u16::<LE>()?;
                        reader.read_u32::<LE>()?; // Skip channel mask
                        let codec_override = reader.read_u16::<LE>()?;
                        if codec == codecs::EXTENSIBLE {
                            codec = codec_override
                        }
                    }
                }
                let (sample_format, bytes_per_sample) = match codec {
                    codecs::PCM => match bits_per_sample {
                        8 => (SampleFormat::U8, 1),
                        16 => (SampleFormat::I16, 2),
                        24 => (SampleFormat::I24, 3),
                        32 => (SampleFormat::I32, 4),
                        _ => {
                            return Err(AudioError::UnsupportedFileFeature(format!(
                                "Unsupported PCM sample size: {bits_per_sample} bits."
                            )));
                        }
                    },
                    codecs::FLOAT => match bits_per_sample {
                        32 => (SampleFormat::F32, 4),
                        64 => (SampleFormat::F64, 8),
                        _ => {
                            return Err(AudioError::UnsupportedFileFeature(format!(
                                "Unsupported floating-point sample size: {bits_per_sample} bits."
                            )));
                        }
                    },
                    _ => {
                        return Err(AudioError::UnsupportedFileFeature(format!(
                            "Unsupported audio codec: {}.",
                            codec_to_string(codec)
                        )));
                    }
                };
                let calculated_bytes_per_frame = bytes_per_sample * channel_count;
                if bytes_per_frame != calculated_bytes_per_frame {
                    return Err(AudioError::CorruptFile(format!(
                        "Expected {calculated_bytes_per_frame} bytes per frame, got {bytes_per_frame}."
                    )));
                }
                format_info = Some(FormatInfo {
                    sample_format,
                    channel_count: u32::from(channel_count),
                    frame_rate,
                    bytes_per_frame: u32::from(bytes_per_frame),
                });
            }
            b"data" => {
                // Data chunk
                let data_offset = reader.stream_position()?;
                let data_byte_count = chunk_size;
                data_info = Some(DataInfo {
                    data_offset,
                    data_byte_count: u64::from(data_byte_count),
                });
            }
            _ => {}
        }
        reader.seek(SeekFrom::Start(chunk_end))?;
    }

    let data_info = data_info.unwrap();
    let format_info = format_info.unwrap();
    let frame_count = data_info.data_byte_count / u64::from(format_info.bytes_per_frame);
    Ok(WaveFileInfo {
        sample_format: format_info.sample_format,
        channel_count: format_info.channel_count,
        sampling_rate: format_info.frame_rate,
        frame_count,
        bytes_per_frame: format_info.bytes_per_frame,
        data_offset: data_info.data_offset,
    })
}

fn round_up_to_even(i: u64) -> u64 {
    let is_even = i % 2 == 0;
    if is_even { i } else { i + 1 }
}
