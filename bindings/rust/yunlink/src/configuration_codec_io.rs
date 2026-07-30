use crate::configuration::*;

pub(crate) const MAX_CONFIG_ITEMS: usize = 256;
const MAX_STRING_BYTES: usize = 1024;

pub(crate) struct Writer {
    data: Vec<u8>,
    valid: bool,
}

impl Writer {
    pub(crate) fn new() -> Self {
        Self {
            data: Vec::new(),
            valid: true,
        }
    }
    pub(crate) fn finish(self) -> Result<Vec<u8>, ()> {
        self.valid.then_some(self.data).ok_or(())
    }
    pub(crate) fn u8(&mut self, value: u8) {
        self.data.push(value);
    }
    pub(crate) fn boolean(&mut self, value: bool) {
        self.u8(u8::from(value));
    }
    pub(crate) fn u16(&mut self, value: u16) {
        self.data.extend(value.to_le_bytes());
    }
    pub(crate) fn u64(&mut self, value: u64) {
        self.data.extend(value.to_le_bytes());
    }
    pub(crate) fn f64(&mut self, value: f64) {
        if !value.is_finite() {
            self.valid = false;
            return;
        }
        self.u64(value.to_bits());
    }
    pub(crate) fn text(&mut self, value: &str) -> Result<(), ()> {
        if value.len() > MAX_STRING_BYTES {
            return Err(());
        }
        self.u16(value.len() as u16);
        self.data.extend(value.as_bytes());
        Ok(())
    }
    pub(crate) fn list<T>(
        &mut self,
        values: &[T],
        write: impl Fn(&mut Self, &T) -> Result<(), ()>,
    ) -> Result<(), ()> {
        if values.len() > MAX_CONFIG_ITEMS {
            return Err(());
        }
        self.u16(values.len() as u16);
        values.iter().try_for_each(|value| write(self, value))
    }
}

pub(crate) struct Reader<'a> {
    data: &'a [u8],
    cursor: usize,
}

impl<'a> Reader<'a> {
    pub(crate) fn new(data: &'a [u8]) -> Self {
        Self { data, cursor: 0 }
    }
    pub(crate) fn done(&self) -> bool {
        self.cursor == self.data.len()
    }
    pub(crate) fn u8(&mut self) -> Result<u8, ()> {
        let value = *self.data.get(self.cursor).ok_or(())?;
        self.cursor += 1;
        Ok(value)
    }
    pub(crate) fn boolean(&mut self) -> Result<bool, ()> {
        match self.u8()? {
            0 => Ok(false),
            1 => Ok(true),
            _ => Err(()),
        }
    }
    pub(crate) fn u16(&mut self) -> Result<u16, ()> {
        let value = self.data.get(self.cursor..self.cursor + 2).ok_or(())?;
        self.cursor += 2;
        Ok(u16::from_le_bytes([value[0], value[1]]))
    }
    pub(crate) fn u64(&mut self) -> Result<u64, ()> {
        let value = self.data.get(self.cursor..self.cursor + 8).ok_or(())?;
        self.cursor += 8;
        Ok(u64::from_le_bytes(value.try_into().map_err(|_| ())?))
    }
    pub(crate) fn f64(&mut self) -> Result<f64, ()> {
        let value = f64::from_bits(self.u64()?);
        value.is_finite().then_some(value).ok_or(())
    }
    pub(crate) fn text(&mut self) -> Result<String, ()> {
        let length = self.u16()? as usize;
        let value = self.data.get(self.cursor..self.cursor + length).ok_or(())?;
        self.cursor += length;
        String::from_utf8(value.to_vec()).map_err(|_| ())
    }
    pub(crate) fn list<T>(
        &mut self,
        read: impl Fn(&mut Self) -> Result<T, ()>,
    ) -> Result<Vec<T>, ()> {
        let count = self.u16()? as usize;
        if count > MAX_CONFIG_ITEMS {
            return Err(());
        }
        (0..count).map(|_| read(self)).collect()
    }
}

include!("configuration_codec_io/value.rs");
include!("configuration_codec_io/descriptor.rs");
include!("configuration_codec_io/snapshot.rs");
include!("configuration_codec_io/variant.rs");
include!("configuration_codec_io/enums.rs");
