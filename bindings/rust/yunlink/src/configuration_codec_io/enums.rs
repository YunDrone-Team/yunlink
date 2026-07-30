pub(crate) fn status(value: u8) -> Result<ConfigServiceStatus, ()> {
    match value {
        0 => Ok(ConfigServiceStatus::Ok),
        1 => Ok(ConfigServiceStatus::NotFound),
        2 => Ok(ConfigServiceStatus::Unsupported),
        3 => Ok(ConfigServiceStatus::Unauthenticated),
        4 => Ok(ConfigServiceStatus::Unauthorized),
        5 => Ok(ConfigServiceStatus::Conflict),
        6 => Ok(ConfigServiceStatus::Invalid),
        7 => Ok(ConfigServiceStatus::UnsafeState),
        8 => Ok(ConfigServiceStatus::InternalError),
        _ => Err(()),
    }
}
pub(crate) fn requirement(value: u8) -> Result<ConfigApplyRequirement, ()> {
    match value {
        0 => Ok(ConfigApplyRequirement::None),
        1 => Ok(ConfigApplyRequirement::ComponentRestart),
        2 => Ok(ConfigApplyRequirement::EndpointRestart),
        3 => Ok(ConfigApplyRequirement::DeviceReboot),
        4 => Ok(ConfigApplyRequirement::Manual),
        _ => Err(()),
    }
}
pub(crate) fn outcome(value: u8) -> Result<ConfigApplyOutcome, ()> {
    match value {
        1 => Ok(ConfigApplyOutcome::Applied),
        2 => Ok(ConfigApplyOutcome::RestartScheduled),
        3 => Ok(ConfigApplyOutcome::ManualActionRequired),
        4 => Ok(ConfigApplyOutcome::Failed),
        _ => Err(()),
    }
}
pub(crate) fn value_type(value: u8) -> Result<ConfigValueType, ()> {
    match value {
        1 => Ok(ConfigValueType::Bool),
        2 => Ok(ConfigValueType::Int64),
        3 => Ok(ConfigValueType::Double),
        4 => Ok(ConfigValueType::String),
        5 => Ok(ConfigValueType::StringList),
        6 => Ok(ConfigValueType::DoubleList),
        _ => Err(()),
    }
}
pub(crate) fn update_policy(value: u8) -> Result<ConfigFieldUpdatePolicy, ()> {
    match value {
        0 => Ok(ConfigFieldUpdatePolicy::HotReload),
        1 => Ok(ConfigFieldUpdatePolicy::ComponentRestart),
        2 => Ok(ConfigFieldUpdatePolicy::EndpointRestart),
        3 => Ok(ConfigFieldUpdatePolicy::DeviceReboot),
        4 => Ok(ConfigFieldUpdatePolicy::Manual),
        _ => Err(()),
    }
}
pub(crate) fn variant_source(value: u8) -> Result<ConfigVariantSource, ()> {
    match value {
        1 => Ok(ConfigVariantSource::Default),
        2 => Ok(ConfigVariantSource::Active),
        _ => Err(()),
    }
}
pub(crate) fn write_status(
    writer: &mut Writer,
    status: ConfigServiceStatus,
    message: &str,
) -> Result<(), ()> {
    writer.u8(status as u8);
    writer.text(message)
}
pub(crate) fn read_status(reader: &mut Reader<'_>) -> Result<(ConfigServiceStatus, String), ()> {
    Ok((status(reader.u8()?)?, reader.text()?))
}
