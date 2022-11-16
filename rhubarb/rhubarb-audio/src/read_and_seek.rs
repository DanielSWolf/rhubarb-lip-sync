use std::io::{Read, Seek};

/// A seekable reader.
pub trait ReadAndSeek: Read + Seek {}

impl<T: Seek + Read> ReadAndSeek for T {}
