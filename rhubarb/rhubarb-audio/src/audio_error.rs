use std::{
    error::Error,
    fmt::{self, Display, Formatter},
    io::{self, ErrorKind},
};

/// The error type for working with audio.
#[derive(Debug)]
pub enum AudioError {
    /// This library doesn't know how to read audio files with the current file's extension.
    UnsupportedFileType,

    /// The current file uses an unsupported feature, such as an audio codec or a sample format.
    ///
    /// Expects a string containing a more detailed error message.
    UnsupportedFileFeature(String),

    /// The current file doesn't adhere to the expected file format. This could be because it got
    /// corrupted during transfer or because it is a valid file that received the wrong file
    /// extension.
    ///
    /// Expects a string containing a more detailed error message.
    CorruptFile(String),

    /// An I/O error occurred trying to read from or write to the current file.
    IoError(io::Error),
}

impl Error for AudioError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        // Don't expose source. See https://stackoverflow.com/a/63620301/52041
        None
    }
}

impl Display for AudioError {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        match self {
            Self::UnsupportedFileType => write!(f, "Unsupported audio file type."),
            Self::UnsupportedFileFeature(message) => {
                write!(f, "Unsupported audio file feature: {message}")
            }
            Self::CorruptFile(message) => {
                write!(f, "Audio file appears to be corrupt: {message}")
            }
            Self::IoError(io_error) => write!(f, "Error reading audio file: {io_error}"),
        }
    }
}

impl From<io::Error> for AudioError {
    fn from(io_error: io::Error) -> Self {
        if io_error.kind() == ErrorKind::UnexpectedEof {
            Self::CorruptFile("Unexpected end of file.".to_owned())
        } else {
            Self::IoError(io_error)
        }
    }
}

impl PartialEq for AudioError {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::UnsupportedFileType, Self::UnsupportedFileType) => true,
            (
                Self::UnsupportedFileFeature(left_message),
                Self::UnsupportedFileFeature(right_message),
            ) => left_message == right_message,
            (Self::CorruptFile(left_message), Self::CorruptFile(right_message)) => {
                left_message == right_message
            }
            (Self::IoError(left_error), Self::IoError(right_error)) => {
                left_error.kind() == right_error.kind()
            }
            _ => false,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use assert_matches::assert_matches;
    use rstest::*;
    use speculoos::prelude::*;

    #[rstest]
    fn does_not_expose_source() {
        let error = AudioError::UnsupportedFileType;
        assert_that!(error.source()).is_none();

        let error = AudioError::IoError(io::Error::from(ErrorKind::NotFound));
        assert_that!(error.source()).is_none();
    }

    #[rstest]
    fn supports_display() {
        let error = AudioError::UnsupportedFileType;
        assert_that!(format!("{error}")).is_equal_to("Unsupported audio file type.".to_owned());

        let error = AudioError::UnsupportedFileFeature("Spacial audio not supported.".to_owned());
        assert_that!(format!("{error}"))
            .is_equal_to("Unsupported audio file feature: Spacial audio not supported.".to_owned());

        let error = AudioError::CorruptFile("Checksum doesn't match.".to_owned());
        assert_that!(format!("{error}"))
            .is_equal_to("Audio file appears to be corrupt: Checksum doesn't match.".to_owned());

        let error = AudioError::IoError(io::Error::from(ErrorKind::NotFound));
        assert_that!(format!("{error}"))
            .is_equal_to("Error reading audio file: entity not found".to_owned());
    }

    mod from_io_error {
        use super::*;

        #[rstest]
        fn converts_from_io_error() {
            let error = AudioError::from(io::Error::from(ErrorKind::NotFound));
            assert_matches!(
                error,
                AudioError::IoError(io_error) if io_error.kind() == ErrorKind::NotFound
            );
        }

        #[rstest]
        fn treats_unexpected_eof_as_corrupt() {
            let error = AudioError::from(io::Error::from(ErrorKind::UnexpectedEof));
            assert_matches!(
                error,
                AudioError::CorruptFile(message) if message == "Unexpected end of file."
            );
        }
    }

    #[rstest]
    fn supports_partial_equality() {
        assert_that!(AudioError::UnsupportedFileType).is_equal_to(AudioError::UnsupportedFileType);
        assert_that!(AudioError::UnsupportedFileType)
            .is_not_equal_to(AudioError::CorruptFile("".to_owned()));

        assert_that!(AudioError::CorruptFile("Foo".to_owned()))
            .is_equal_to(AudioError::CorruptFile("Foo".to_owned()));
        assert_that!(AudioError::CorruptFile("Foo".to_owned()))
            .is_not_equal_to(AudioError::CorruptFile("Bar".to_owned()));

        assert_that!(AudioError::from(io::Error::from(ErrorKind::UnexpectedEof)))
            .is_equal_to(AudioError::from(io::Error::from(ErrorKind::UnexpectedEof)));
        assert_that!(AudioError::from(io::Error::new(
            ErrorKind::UnexpectedEof,
            "Foo"
        )))
        .is_equal_to(AudioError::from(io::Error::new(
            ErrorKind::UnexpectedEof,
            "Bar",
        )));
        assert_that!(AudioError::from(io::Error::from(ErrorKind::UnexpectedEof))).is_not_equal_to(
            AudioError::from(io::Error::from(ErrorKind::ConnectionReset)),
        );
    }
}
