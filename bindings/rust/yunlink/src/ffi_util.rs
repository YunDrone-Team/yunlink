pub(crate) fn write_c_buffer<const N: usize>(dst: &mut [std::ffi::c_char; N], value: &str) {
    for slot in dst.iter_mut() {
        *slot = 0;
    }
    for (index, byte) in value
        .as_bytes()
        .iter()
        .copied()
        .enumerate()
        .take(N.saturating_sub(1))
    {
        dst[index] = byte as std::ffi::c_char;
    }
}

pub(crate) fn string_from_c_buf<const N: usize>(buf: &[std::ffi::c_char; N]) -> String {
    let len = buf.iter().position(|value| *value == 0).unwrap_or(N);
    let bytes: Vec<u8> = buf[..len].iter().map(|value| *value as u8).collect();
    String::from_utf8_lossy(&bytes).into_owned()
}
