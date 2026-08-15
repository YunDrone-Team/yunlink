use crate::system::{ClockSyncError, ClockSyncRequest, ClockSyncResponse};

pub fn validate_clock_sync_request(request: &ClockSyncRequest) -> Result<(), &'static str> {
    if !(crate::MINIMUM_TRUSTED_UNIX_TIME_MS..=crate::MAXIMUM_TRUSTED_UNIX_TIME_MS)
        .contains(&request.unix_time_ms)
    {
        return Err("clock sync time is outside the product range");
    }
    if request.source.is_empty()
        || request.source.len() > 64
        || !request
            .source
            .bytes()
            .all(|value| value.is_ascii_alphanumeric() || b"-_.".contains(&value))
    {
        return Err("clock sync source is invalid");
    }
    Ok(())
}

pub fn validate_clock_sync_response(response: &ClockSyncResponse) -> Result<(), &'static str> {
    if !matches!(
        ClockSyncError::try_from(response.error),
        Ok(ClockSyncError::ClockSyncOk)
            | Ok(ClockSyncError::ClockSyncInvalidRequest)
            | Ok(ClockSyncError::ClockSyncArmedBlocked)
            | Ok(ClockSyncError::ClockSyncOutOfRange)
            | Ok(ClockSyncError::ClockSyncHelperUnavailable)
            | Ok(ClockSyncError::ClockSyncInternalError)
    ) || response.message.len() > 256
    {
        return Err("clock sync response error or message is invalid");
    }
    if response.error == ClockSyncError::ClockSyncOk as i32 {
        let valid = |value| {
            (crate::MINIMUM_TRUSTED_UNIX_TIME_MS..=crate::MAXIMUM_TRUSTED_UNIX_TIME_MS)
                .contains(&value)
        };
        if !valid(response.previous_unix_time_ms) || !valid(response.applied_unix_time_ms) {
            return Err("clock sync response timestamps are invalid");
        }
        if response.delta_ms
            != response.applied_unix_time_ms as i64 - response.previous_unix_time_ms as i64
        {
            return Err("clock sync response delta is invalid");
        }
    } else if response.previous_unix_time_ms != 0
        || response.applied_unix_time_ms != 0
        || response.delta_ms != 0
    {
        return Err("failed clock sync response contains timestamps");
    }
    Ok(())
}
