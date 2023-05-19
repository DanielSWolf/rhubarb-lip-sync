use num::{Float, FromPrimitive};
use rstest::*;
use rstest_reuse::{self, *};
use speculoos::prelude::*;
use speculoos::{AssertionFailure, Spec};
use std::fmt::Debug;
use std::iter::Sum;
use std::{
    cell::RefCell,
    fs::File,
    io::{self, ErrorKind, Read, Seek},
    path::{Path, PathBuf},
    rc::Rc,
    time::Duration,
};

use rhubarb_audio::{open_audio_file, open_audio_file_with_reader, AudioError, Sample};

/// A sine wave
fn sine(t: f64, f: f64) -> Sample {
    f64::sin(t * f * 2.0 * std::f64::consts::PI) as f32
}

/// A triangle wave
fn triangle(t: f64, f: f64) -> Sample {
    // See https://en.wikipedia.org/wiki/Triangle_wave#Definition
    let t2 = t + 0.25 / f;
    (2.0 * f64::abs(2.0 * (t2 * f - f64::floor(t2 * f + 0.5))) - 1.0) as f32
}

/// 50:50 mix of 1-kHz sine and triangle wave
fn sine_triangle_1_khz(t: f64) -> Sample {
    let f = 1000.0;
    (sine(t, f) + triangle(t, f)) / 2.0
}

fn get_resource_file_path(file_name: &str) -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("tests/res")
        .join(file_name)
}

mod open_audio_file {
    use super::*;

    #[rustfmt::skip]
    #[template]
    #[rstest]
    #[case::wav_u8_audition     ("sine-triangle-u8-audition.wav",       48000,  sine_triangle_1_khz, 2.0f32.powi(-7))]
    #[case::wav_u8_ffmpeg       ("sine-triangle-u8-ffmpeg.wav",         48000,  sine_triangle_1_khz, 2.0f32.powi(-7))]
    #[case::wav_u8_soundforge   ("sine-triangle-u8-soundforge.wav",     48000,  sine_triangle_1_khz, 2.0f32.powi(-7))]
    #[case::wav_i16_audacity    ("sine-triangle-i16-audacity.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-15))]
    #[case::wav_i16_audition    ("sine-triangle-i16-audition.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-15))]
    #[case::wav_i16_ffmpeg      ("sine-triangle-i16-ffmpeg.wav",        48000,  sine_triangle_1_khz, 2.0f32.powi(-15))]
    #[case::wav_i16_soundforge  ("sine-triangle-i16-soundforge.wav",    48000,  sine_triangle_1_khz, 2.0f32.powi(-15))]
    #[case::wav_i24_audacity    ("sine-triangle-i24-audacity.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_i24_audition    ("sine-triangle-i24-audition.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_i24_ffmpeg      ("sine-triangle-i24-ffmpeg.wav",        48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_i24_soundforge  ("sine-triangle-i24-soundforge.wav",    48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_i32_ffmpeg      ("sine-triangle-i32-ffmpeg.wav",        48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_i32_soundforge  ("sine-triangle-i32-soundforge.wav",    48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_f32_audacity    ("sine-triangle-f32-audacity.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_f32_audition    ("sine-triangle-f32-audition.wav",      48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_f32_ffmpeg      ("sine-triangle-f32-ffmpeg.wav",        48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_f32_soundforge  ("sine-triangle-f32-soundforge.wav",    48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::wav_f64_ffmpeg      ("sine-triangle-f64-ffmpeg.wav",        48000,  sine_triangle_1_khz, 2.0f32.powi(-21))]
    #[case::ogg                 ("sine-triangle.ogg",                   48000,  sine_triangle_1_khz, 2.0f32.powi(-3))] // lossy
    fn supported_audio_files(
        #[case] file_name: &str,
        #[case] sampling_rate: u32,
        #[case] signal_fn: fn(f64) -> Sample,
        #[case] tolerance: f32,
    ) {}

    #[rstest]
    #[case::wav(
        "sine-triangle-i16-audacity.wav",
        "WaveAudioClip { wave_file_info: WaveFileInfo { sample_format: I16, channel_count: 2, sampling_rate: 48000, frame_count: 480000, bytes_per_frame: 4, data_offset: 44 } }"
    )]
    #[case::ogg(
        "sine-triangle.ogg",
        "OggAudioClip { metadata: Metadata { frame_count: 480000, sampling_rate: 48000, channel_count: 2 } }"
    )]
    fn supports_debug(#[case] file_name: &str, #[case] expected: &str) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        assert_that!(format!("{audio_clip:?}")).is_equal_to(expected.to_owned());
    }

    #[rstest]
    #[case::wav(
        "sine-triangle-i16-audacity.wav",
        "WaveFileSampleReader { wave_file_info: WaveFileInfo { sample_format: I16, channel_count: 2, sampling_rate: 48000, frame_count: 480000, bytes_per_frame: 4, data_offset: 44 }, logical_position: 0, physical_position: None }"
    )]
    #[case::ogg(
        "sine-triangle.ogg",
        "OggFileSampleReader { metadata: Metadata { frame_count: 480000, sampling_rate: 48000, channel_count: 2 }, logical_position: 0, physical_position: 0 }"
    )]
    fn sample_reader_supports_debug(#[case] file_name: &str, #[case] expected: &str) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let sample_reader = audio_clip.create_sample_reader().unwrap();
        assert_that!(format!("{sample_reader:?}")).is_equal_to(expected.to_owned());
    }

    #[apply(supported_audio_files)]
    fn provides_metadata(
        #[case] file_name: &str,
        #[case] sampling_rate: u32,
        #[case] _signal_fn: fn(f64) -> Sample,
        #[case] _tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();

        assert_that!(audio_clip.len()).is_equal_to(10 * sampling_rate as u64);
        assert_that!(audio_clip.sampling_rate()).is_equal_to(sampling_rate);
        assert_that!(audio_clip.duration()).is_equal_to(Duration::from_secs(10));
    }

    #[apply(supported_audio_files)]
    fn reads_samples(
        #[case] file_name: &str,
        #[case] sampling_rate: u32,
        #[case] signal_fn: fn(f64) -> Sample,
        #[case] tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = [0.0f32; 48 * 2];
        sample_reader.read(&mut buffer).unwrap();

        assert_that!(buffer.to_vec()).is_close_to(&[], Tolerance::MaxError(tolerance));

        // for (i, sample) in buffer.iter().enumerate() {
        //     let expected = signal_fn(i as f64 / sampling_rate as f64);
        //     assert_that!(*sample)
        //         .named(&i.to_string())
        //         .is_close_to(expected, tolerance);
        // }
    }

    #[apply(supported_audio_files)]
    fn reads_samples_in_one_large_chunk(
        #[case] file_name: &str,
        #[case] sampling_rate: u32,
        #[case] signal_fn: fn(f64) -> Sample,
        #[case] tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = vec![0.0f32; sample_reader.len() as usize];
        sample_reader.read(&mut buffer).unwrap();

        for (i, sample) in buffer.iter().enumerate() {
            let expected = signal_fn(i as f64 / sampling_rate as f64);
            assert_that!(*sample)
                .named(&i.to_string())
                .is_close_to(expected, tolerance);
        }
    }

    #[apply(supported_audio_files)]
    fn seeks_up_to_the_end(
        #[case] file_name: &str,
        #[case] sampling_rate: u32,
        #[case] signal_fn: fn(f64) -> Sample,
        #[case] tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = [0.0f32; 48 * 2];

        for offset in [
            9 * sampling_rate as u64 - 5,
            2 * sampling_rate as u64 + 3,
            audio_clip.len() - buffer.len() as u64,
        ] {
            sample_reader.set_position(offset);
            sample_reader.read(&mut buffer).unwrap();

            for (i, sample) in buffer.iter().enumerate() {
                let expected = signal_fn((i as u64 + offset) as f64 / sampling_rate as f64);
                assert_that!(*sample)
                    .named(&i.to_string())
                    .is_close_to(expected, tolerance);
            }
        }

        sample_reader.set_position(audio_clip.len());
    }

    #[should_panic(expected = "Attempting to seek to position 480001 of 480000-frame audio clip.")]
    #[apply(supported_audio_files)]
    fn seeking_beyond_the_end(
        #[case] file_name: &str,
        #[case] _sampling_rate: u32,
        #[case] _signal_fn: fn(f64) -> Sample,
        #[case] _tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        sample_reader.set_position(audio_clip.len() + 1);
    }

    #[should_panic(
        expected = "Attempting to read up to position 480001 of 480000-frame audio clip."
    )]
    #[apply(supported_audio_files)]
    fn reading_beyond_the_end(
        #[case] file_name: &str,
        #[case] _sampling_rate: u32,
        #[case] _signal_fn: fn(f64) -> Sample,
        #[case] _tolerance: f32,
    ) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = [0.0f32; 48 * 2];
        sample_reader.set_position(audio_clip.len() - buffer.len() as u64 + 1);
        sample_reader.read(&mut buffer).unwrap();
    }

    #[rstest]
    #[case::mp3("sine-triangle.mp3")]
    #[case::flac("sine-triangle.flac")]
    fn fails_when_opening_file_of_unsupported_type(#[case] file_name: &str) {
        let path = get_resource_file_path(file_name);
        let result = open_audio_file(path);
        assert_that!(result).is_err_containing(AudioError::UnsupportedFileType);
    }

    #[rstest]
    #[case::wav_codec_flac(
        "sine-triangle-flac-ffmpeg.wav",
        "Unsupported audio codec: Free Lossless Audio Codec FLAC."
    )]
    #[case::wav_codec_vorbis("sine-triangle-vorbis-ffmpeg.wav", "Unsupported audio codec: 0x566F.")]
    fn fails_when_opening_file_using_unsupported_feature(
        #[case] file_name: &str,
        #[case] expected_message: &str,
    ) {
        let path = get_resource_file_path(file_name);
        let result = open_audio_file(path);
        assert_that!(result).is_err_containing(AudioError::UnsupportedFileFeature(
            expected_message.to_owned(),
        ));
    }

    #[rstest]
    #[case::wav("no-such-file.wav")]
    #[case::ogg("no-such-file.ogg")]
    fn fails_if_file_does_not_exist(#[case] file_name: &str) {
        let path = get_resource_file_path(file_name);
        let result = open_audio_file(path);
        assert_that!(result)
            .is_err_containing(AudioError::IoError(io::Error::from(ErrorKind::NotFound)));
    }

    #[rstest]
    #[case::wav_file_type_txt(
        "corrupt_file_type_txt.wav",
        "Expected master chunk ID \"RIFF\", got \"Lore\"."
    )]
    #[case::wav_file_type_pal(
        "corrupt_file_type_pal.wav",
        "Expected WAVE chunk ID \"WAVE\", got \"PAL \"."
    )]
    #[case::wav_truncated_header("corrupt_truncated_header.wav", "Unexpected end of file.")]
    #[case::wav_truncated_data("corrupt_truncated_data.wav", "Unexpected end of file.")]
    #[case::ogg_file_type_txt(
        "corrupt_file_type_txt.ogg",
        "The given file was not recognized as Ogg Vorbis data."
    )]
    #[case::ogg_truncated_header(
        "corrupt_truncated_header.ogg",
        "The given file was not recognized as Ogg Vorbis data."
    )]
    fn fails_if_file_is_corrupt(#[case] file_name: &str, #[case] expected_message: &str) {
        let path = get_resource_file_path(file_name);
        let result = open_audio_file(path)
            .and_then(|audio_clip| audio_clip.create_sample_reader())
            .and_then(|mut sample_reader| {
                let mut buffer = vec![0.0f32; sample_reader.len() as usize];
                sample_reader.read(&mut buffer)
            });

        assert_that!(result)
            .is_err_containing(AudioError::CorruptFile(expected_message.to_owned()));
    }

    #[rstest]
    #[case::wav_ascii("filename-ascii !#$%&'()+,-.;=@[]^_`{}~.wav")]
    #[case::wav_ansi("filename-ansi-€…‡‰‘’“”•™©±²½æ.wav")]
    #[case::wav_unicode_bmp("filename-unicode-bmp-①∀⇨.wav")]
    #[case::wav_unicode_wide("filename-unicode-wide-😀🤣🙈🍨.wav")]
    #[case::ogg_ascii("filename-ascii !#$%&'()+,-.;=@[]^_`{}~.ogg")]
    #[case::ogg_ansi("filename-ansi-€…‡‰‘’“”•™©±²½æ.ogg")]
    #[case::ogg_unicode_bmp("filename-unicode-bmp-①∀⇨.ogg")]
    #[case::ogg_unicode_wide("filename-unicode-wide-😀🤣🙈🍨.ogg")]
    fn supports_special_characters_in_file_names(#[case] file_name: &str) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = [0.0f32; 48 * 2];
        sample_reader.read(&mut buffer).unwrap();
    }

    #[rstest]
    #[case::wav("zero-samples.wav")]
    #[case::wav("zero-samples.ogg")]
    fn supports_zero_sample_files(#[case] file_name: &str) {
        let path = get_resource_file_path(file_name);
        let audio_clip = open_audio_file(path).unwrap();
        let mut sample_reader = audio_clip.create_sample_reader().unwrap();

        let mut buffer = [0.0f32; 0];
        sample_reader.read(&mut buffer).unwrap();

        sample_reader.set_position(0);
    }
}

trait FloatVecAssertions<T: Float> {
    fn is_close_to(&mut self, expected: &[T], tolerance: Tolerance<T>);
}

enum Tolerance<T: Float> {
    MaxError(T),
    MaxRmsError(T),
}

impl<'a, T: Float + Sum + FromPrimitive + Debug> FloatVecAssertions<T> for Spec<'a, Vec<T>> {
    fn is_close_to(&mut self, expected: &[T], tolerance: Tolerance<T>) {
        self.has_length(expected.len());

        let pairs = expected.iter().zip(self.subject.iter());
        match tolerance {
            Tolerance::MaxError(max_error) => {
                for (i, (expected_item, actual_item)) in pairs.enumerate() {
                    let error = (*expected_item - *actual_item).abs();
                    if error > max_error {
                        AssertionFailure::from_spec(self)
                            .with_expected(format!(
                                "vec[{i}] = <{expected_item:?}> ± {max_error:?}"
                            ))
                            .with_actual(format!("<{actual_item:?}>"))
                            .fail();
                    }
                }
            }
            Tolerance::MaxRmsError(max_rmse) => {
                let len = self.subject.len();
                let mse: T =
                    pairs.map(|(a, b)| (*a - *b).powi(2)).sum::<T>() / T::from_usize(len).unwrap();
                let rmse = mse.sqrt();

                if rmse > max_rmse {
                    AssertionFailure::from_spec(self)
                        .with_expected(format!("maximum RMS error <{max_rmse:?}>"))
                        .with_actual(format!("<{rmse:?}>"))
                        .fail();
                }
            }
        }
    }
}

struct MockFile {
    pub file: File,
    pub next_error_kind: Rc<RefCell<Option<io::ErrorKind>>>,
}

impl MockFile {
    fn from_resource(
        file_name: &str,
        next_error_kind: Rc<RefCell<Option<io::ErrorKind>>>,
    ) -> MockFile {
        let path = get_resource_file_path(file_name);
        MockFile {
            file: File::open(&path).unwrap(),
            next_error_kind,
        }
    }
}

impl Read for MockFile {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        match &*(*self.next_error_kind).borrow() {
            None => self.file.read(buf),
            Some(error_kind) => Err(io::Error::from(*error_kind)),
        }
    }
}

impl Seek for MockFile {
    fn seek(&mut self, pos: io::SeekFrom) -> io::Result<u64> {
        match &*(*self.next_error_kind).borrow() {
            None => self.file.seek(pos),
            Some(error_kind) => Err(io::Error::from(*error_kind)),
        }
    }
}

mod open_audio_file_with_reader {
    use super::*;

    #[rstest]
    #[case::wav_not_found("sine-triangle-i16-audacity.wav", io::ErrorKind::NotFound)]
    #[case::ogg_not_found("sine-triangle.ogg", io::ErrorKind::NotFound)]
    #[case::wav_permission_denied(
        "sine-triangle-i16-audacity.wav",
        io::ErrorKind::PermissionDenied
    )]
    #[case::ogg_permission_denied("sine-triangle.ogg", io::ErrorKind::PermissionDenied)]
    fn fails_on_io_errors(#[case] file_name: &'static str, #[case] error_kind: io::ErrorKind) {
        let next_error_kind = Rc::new(RefCell::new(None));
        let audio_clip = {
            let next_error_kind = next_error_kind.clone();
            open_audio_file_with_reader(
                file_name,
                Box::new(move || Ok(MockFile::from_resource(file_name, next_error_kind.clone()))),
            )
            .unwrap()
        };

        next_error_kind.replace(Some(error_kind));
        let mut buffer = [0.0f32; 48 * 2];
        let result = audio_clip
            .create_sample_reader()
            .and_then(|mut sample_reader| sample_reader.read(&mut buffer));
        assert_that!(result).is_err_containing(AudioError::IoError(io::Error::from(error_kind)));
    }
}
