use std::{
    fs::File,
    io::{self, BufReader},
    path::PathBuf,
};

use crate::{AudioClip, AudioError, OggAudioClip, ReadAndSeek, WaveAudioClip};

/// Creates an audio clip from the specified file.
pub fn open_audio_file(path: impl Into<PathBuf>) -> Result<Box<dyn AudioClip>, AudioError> {
    let path: PathBuf = path.into();
    open_audio_file_with_reader(
        path.clone(),
        Box::new(move || Ok(BufReader::new(File::open(path.clone())?))),
    )
}

/// Creates an audio clip from the specified file, using the reader returned by the specified
/// factory function.
pub fn open_audio_file_with_reader<TReader>(
    path: impl Into<PathBuf>,
    create_reader: Box<dyn Fn() -> Result<TReader, io::Error>>,
) -> Result<Box<dyn AudioClip>, AudioError>
where
    TReader: 'static + ReadAndSeek,
{
    let path: PathBuf = path.into();
    let lower_case_extension = path
        .extension()
        .map(|e| e.to_os_string().into_string().unwrap_or_default());
    match lower_case_extension.as_deref() {
        Some("wav") => Ok(Box::new(WaveAudioClip::new(create_reader)?)),
        Some("ogg") => Ok(Box::new(OggAudioClip::new(create_reader)?)),
        _ => Err(AudioError::UnsupportedFileType),
    }
}
