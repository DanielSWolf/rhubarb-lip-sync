use crate::{
    resampled_audio_clip::libsamplerate_raw::src_error,
    sample_reader_assertions::SampleReaderAssertions, AudioClip, AudioError, Sample, SampleReader,
};
use std::{
    cmp::min,
    mem::MaybeUninit,
    os::raw::{c_long, c_void},
    pin::Pin,
    sync::Arc,
};

use super::libsamplerate_raw::{
    src_callback_new, src_callback_read, src_delete, src_reset, ConverterType, State,
};

/// A resampled representation of another audio clip.
#[derive(Debug, Clone)]
pub struct ResampledAudioClip {
    inner_clip: Arc<dyn AudioClip>,
    sampling_rate: u32,
}

impl ResampledAudioClip {
    pub fn new(inner_clip: Box<dyn AudioClip>, sampling_rate: u32) -> Self {
        Self {
            inner_clip: Arc::from(inner_clip),
            sampling_rate,
        }
    }

    fn factor(&self) -> f64 {
        f64::from(self.sampling_rate) / f64::from(self.inner_clip.sampling_rate())
    }
}

impl AudioClip for ResampledAudioClip {
    fn len(&self) -> u64 {
        (self.inner_clip.len() as f64 * self.factor()).round() as u64
    }

    fn sampling_rate(&self) -> u32 {
        self.sampling_rate
    }

    fn create_sample_reader(&self) -> Result<Box<dyn SampleReader>, AudioError> {
        Ok(Box::new(ResampledSampleReader::new(self)?))
    }
}

#[derive(Debug)]
struct ResampledSampleReader {
    callback_data: Pin<Box<CallbackData>>,
    factor: f64,
    len: u64,
    position: u64,
    position_changed: bool,
    libsamplerate_state: *mut State,
}

const INPUT_BUFFER_SIZE: usize = 500;

#[derive(Debug)]
struct CallbackData {
    inner_sample_reader: Box<dyn SampleReader>,
    input_buffer: [Sample; INPUT_BUFFER_SIZE],
    last_error: Option<AudioError>,
}

impl ResampledSampleReader {
    fn new(clip: &ResampledAudioClip) -> Result<Self, AudioError> {
        let callback_data = Box::pin(CallbackData {
            inner_sample_reader: clip.inner_clip.create_sample_reader()?,
            input_buffer: [0.0; INPUT_BUFFER_SIZE],
            last_error: None,
        });
        let channel_count = 1;
        let mut error = MaybeUninit::uninit();
        let libsamplerate_state = unsafe {
            src_callback_new(
                read,
                ConverterType::SincMediumQuality,
                channel_count,
                error.as_mut_ptr(),
                &*callback_data as *const CallbackData as *mut c_void,
            )
        };
        assert_eq!(unsafe { error.assume_init() }, 0);
        Ok(Self {
            callback_data,
            factor: clip.factor(),
            len: clip.len(),
            position: 0,
            position_changed: false,
            libsamplerate_state,
        })
    }
}

impl Drop for ResampledSampleReader {
    fn drop(&mut self) {
        unsafe {
            src_delete(self.libsamplerate_state);
        }
    }
}

impl SampleReader for ResampledSampleReader {
    fn len(&self) -> u64 {
        self.len
    }

    fn position(&self) -> u64 {
        self.position
    }

    fn set_position(&mut self, position: u64) {
        if position == self.position {
            return;
        }

        self.assert_valid_seek_position(position);
        self.position = position;
        self.position_changed = true;
    }

    fn read(&mut self, buffer: &mut [Sample]) -> Result<(), AudioError> {
        self.assert_valid_read_size(buffer);
        if self.position_changed {
            self.callback_data
                .inner_sample_reader
                .set_position((self.position as f64 / self.factor) as u64);
            let error = unsafe { src_reset(self.libsamplerate_state) };
            assert_eq!(error, 0);
            self.position_changed = false;
        }

        let samples_read = unsafe {
            src_callback_read(
                self.libsamplerate_state,
                self.factor,
                buffer.len() as i32,
                buffer.as_mut_ptr(),
            ) as usize
        };
        if let Some(error) = self.callback_data.last_error.take() {
            return Err(error);
        }
        if samples_read < buffer.len() {
            assert_eq!(unsafe { src_error(self.libsamplerate_state) }, 0);
            panic!("Expected {} samples, got {}.", buffer.len(), samples_read);
        }
        self.position += buffer.len() as u64;

        Ok(())
    }
}

unsafe extern "C" fn read(callback_data: *mut c_void, buffer: *mut *const Sample) -> c_long {
    let callback_data = callback_data.cast::<CallbackData>();
    let inner_sample_reader = &mut (*callback_data).inner_sample_reader;
    let sample_count = min(inner_sample_reader.remainder(), INPUT_BUFFER_SIZE as u64) as usize;

    // Allow reading beyond the end of the audio clip by filling with zeros
    let (sample_buffer, zero_buffer) = (*callback_data).input_buffer.split_at_mut(sample_count);
    let read_result = inner_sample_reader.read(sample_buffer);
    if let Err(error) = read_result {
        (*callback_data).last_error = Some(error);
        return 0;
    }
    zero_buffer.fill(0.0);

    *buffer = sample_buffer.as_ptr();
    sample_buffer.len() as c_long
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::{MemoryAudioClip, SampleReader};
//     use rstest::*;
//     use speculoos::prelude::*;

//     /// A sine wave
//     fn sine_440_hz(t: f64) -> Sample {
//         let f = 440.0;
//         f64::sin(t * f * 2.0 * std::f64::consts::PI) as f32
//     }

//     // A half-second clip of a 440 Hz sine wave sampled at 48 kHz
//     #[fixture]
//     fn original_clip() -> MemoryAudioClip {
//         let samples: Vec<Sample> = (0..48000 / 2)
//             .map(|i| sine_440_hz(i as f64 / 48000.0))
//             .collect();
//         MemoryAudioClip::new(&samples, 48000)
//     }

//     // The original clip resampled to 44.1 kHz
//     #[fixture]
//     fn resampled_clip(original_clip: MemoryAudioClip) -> ResampledAudioClip {
//         ResampledAudioClip::new(Box::new(original_clip), 44100)
//     }

//     mod can_be_created_via_fluent_syntax {
//         use super::*;
//         use crate::AudioFilters;

//         #[rstest]
//         fn from_value(original_clip: MemoryAudioClip) {
//             original_clip.resampled(44100);
//         }

//         #[rstest]
//         fn from_box(original_clip: MemoryAudioClip) {
//             let boxed_clip = Box::new(original_clip);
//             boxed_clip.resampled(44100);
//         }
//     }

//     #[rstest]
//     fn supports_debug(resampled_clip: ResampledAudioClip) {
//         assert_that!(format!("{resampled_clip:?}"))
//             .is_equal_to("ResampledAudioClip { inner_clip: MemoryAudioClip { buffer: 11 samples, sampling_rate: 16000 }, start: 1, end: 9 }".to_owned());
//     }

//     #[rstest]
//     fn provides_length(resampled_clip: ResampledAudioClip) {
//         assert_that!(resampled_clip.len()).is_equal_to(22050);
//     }

//     #[rstest]
//     fn provides_sampling_rate(resampled_clip: ResampledAudioClip) {
//         assert_that!(resampled_clip.sampling_rate()).is_equal_to(44100);
//     }

//     #[rstest]
//     fn supports_zero_samples() {
//         let inner_clip = MemoryAudioClip::new(&[], 22025);
//         let resampled_clip = ResampledAudioClip::new(Box::new(inner_clip), 16000);
//         assert_that!(resampled_clip.len()).is_equal_to(0);
//         assert_that!(resampled_clip.sampling_rate()).is_equal_to(16000);

//         let mut sample_reader = resampled_clip.create_sample_reader().unwrap();
//         let mut buffer = [0.0f32; 0];
//         sample_reader.read(&mut buffer).unwrap();

//         sample_reader.set_position(0);
//     }

//     mod sample_reader {
//         use super::*;

//         #[fixture]
//         fn reader(resampled_clip: ResampledAudioClip) -> Box<dyn SampleReader> {
//             resampled_clip.create_sample_reader().unwrap()
//         }

//         #[rstest]
//         fn supports_debug(reader: Box<dyn SampleReader>) {
//             assert_that!(format!("{reader:?}"))
//                 .is_equal_to("SegmentSampleReader { inner_sample_reader: MemorySampleReader { buffer: 11 samples, position: 1 }, start: 1, end: 9 }".to_owned());
//         }

//         #[rstest]
//         fn provides_length(reader: Box<dyn SampleReader>) {
//             assert_that!(reader.len()).is_equal_to(22050);
//         }

//         #[rstest]
//         fn position_is_initially_0(reader: Box<dyn SampleReader>) {
//             assert_that!(reader.position()).is_equal_to(0);
//         }

//         #[rstest]
//         fn reads_samples_up_to_the_end(mut reader: Box<dyn SampleReader>) {
//             let mut three_samples = [0f32; 3];
//             reader.read(&mut three_samples).unwrap();
//             assert_that!(three_samples).is_equal_to([0.1, 0.2, 0.3]);

//             let mut five_samples = [0f32; 5];
//             reader.read(&mut five_samples).unwrap();
//             assert_that!(five_samples).is_equal_to([0.4, 0.5, 0.6, 0.7, 0.8]);
//         }

//         #[rstest]
//         fn seeks(mut reader: Box<dyn SampleReader>) {
//             reader.set_position(2);
//             let mut three_samples = [0f32; 3];
//             reader.read(&mut three_samples).unwrap();
//             assert_that!(three_samples).is_equal_to([0.3, 0.4, 0.5]);

//             reader.set_position(1);
//             reader.read(&mut three_samples).unwrap();
//             assert_that!(three_samples).is_equal_to([0.2, 0.3, 0.4]);

//             reader.read(&mut three_samples).unwrap();
//             assert_that!(three_samples).is_equal_to([0.5, 0.6, 0.7]);
//         }

//         #[rstest]
//         fn seeks_up_to_the_end(mut reader: Box<dyn SampleReader>) {
//             reader.set_position(8);
//             let mut zero_samples = [0f32; 0];
//             reader.read(&mut zero_samples).unwrap();
//         }

//         #[rstest]
//         #[should_panic(expected = "Attempting to read up to position 9 of 8-frame audio clip.")]
//         fn reading_beyond_the_end(mut reader: Box<dyn SampleReader>) {
//             reader.set_position(6);
//             let mut three_samples = [0f32; 3];
//             reader.read(&mut three_samples).unwrap();
//         }

//         #[rstest]
//         #[should_panic(expected = "Attempting to seek to position 9 of 8-frame audio clip.")]
//         fn seeking_beyond_the_end(mut reader: Box<dyn SampleReader>) {
//             reader.set_position(9);
//         }
//     }
// }
