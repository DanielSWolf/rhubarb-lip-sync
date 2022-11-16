use std::io::{self, Read};

/// A four-character code (see https://en.wikipedia.org/wiki/FourCC).
pub type FourCc = [u8; 4];

pub fn four_cc_to_string(four_cc: &FourCc) -> String {
    four_cc
        .iter()
        .map(|char_code| char::from_u32(u32::from(*char_code)).unwrap())
        .collect::<String>()
}

pub trait ReadFourCcExt {
    fn read_four_cc(&mut self) -> Result<FourCc, io::Error>;
}

impl<TReader: Read> ReadFourCcExt for TReader {
    fn read_four_cc(&mut self) -> Result<FourCc, io::Error> {
        let mut result = FourCc::default();
        self.read_exact(&mut result)?;
        Ok(result)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use speculoos::prelude::*;

    mod four_cc_to_string {
        use super::*;

        #[test]
        fn returns_a_string_representation() {
            assert_that!(four_cc_to_string(b"WAVE")).is_equal_to("WAVE".to_owned());
            assert_that!(four_cc_to_string(b"fmt ")).is_equal_to("fmt ".to_owned());
        }

        #[test]
        fn supports_arbitrary_bytes() {
            assert_that!(four_cc_to_string(&[0xa9, 0xff, 0x00, 0x0a]))
                .is_equal_to("©ÿ\0\n".to_owned());
        }
    }

    mod read_four_cc {
        use std::io::{Cursor, ErrorKind};

        use super::*;

        #[test]
        fn reads_from_a_reader() {
            let mut reader = Cursor::new(b"ABCDEFGH");
            assert_that!(reader.read_four_cc()).is_ok_containing(b"ABCD");
            assert_that!(reader.read_four_cc()).is_ok_containing(b"EFGH");
        }

        #[test]
        fn fails_when_reading_past_the_end() {
            let mut reader = Cursor::new(b"AB");
            assert_that!(reader.read_four_cc())
                .is_err()
                .matches(|error| error.kind() == ErrorKind::UnexpectedEof);
        }
    }
}
