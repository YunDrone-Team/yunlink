//! Small helpers for copying values across fixed-size C ABI buffers.

/// Copy a Rust string into a null-terminated fixed C buffer.
///
/// The C ABI avoids ownership transfer for strings, so callers provide fixed
/// buffers. This helper clears the entire buffer first and then truncates the
/// UTF-8 bytes to leave room for a trailing null byte.
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

/// Convert a null-terminated fixed C buffer into an owned Rust string.
///
/// Invalid UTF-8 is replaced lossily because runtime-facing display strings
/// should not make event parsing fail.
pub(crate) fn string_from_c_buf<const N: usize>(buf: &[std::ffi::c_char; N]) -> String {
    let len = buf.iter().position(|value| *value == 0).unwrap_or(N);
    let bytes: Vec<u8> = buf[..len].iter().map(|value| *value as u8).collect();
    String::from_utf8_lossy(&bytes).into_owned()
}
