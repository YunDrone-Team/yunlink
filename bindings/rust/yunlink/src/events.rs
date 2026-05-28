use yunlink_sys as sys;

use crate::ffi_util::string_from_c_buf;

#[derive(Debug, Clone, PartialEq)]
pub struct CommandResultEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub progress_percent: u8,
}

#[derive(Debug, Clone, PartialEq)]
pub struct VehicleCoreStateEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub armed: bool,
    pub battery_percent: f32,
}

#[derive(Debug, Clone, PartialEq)]
pub struct LinkEvent {
    pub peer_id: String,
    pub is_up: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ErrorEvent {
    pub code: u16,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Event {
    Link(LinkEvent),
    Error(ErrorEvent),
    CommandResult(CommandResultEvent),
    VehicleCoreState(VehicleCoreStateEvent),
}

pub const EVENT_CHANNEL_CAPACITY: usize = 64;

pub(crate) fn parse_event(event: sys::yunlink_runtime_event_t) -> Option<Event> {
    match event.type_ {
        sys::YUNLINK_RUNTIME_EVENT_LINK => {
            let data = unsafe { event.data.link };
            Some(Event::Link(LinkEvent {
                peer_id: string_from_c_buf(&data.peer_id),
                is_up: data.is_up != 0,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_ERROR => {
            let data = unsafe { event.data.error };
            Some(Event::Error(ErrorEvent {
                code: data.code,
                message: string_from_c_buf(&data.message),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_COMMAND_RESULT => {
            let data = unsafe { event.data.command_result };
            Some(Event::CommandResult(CommandResultEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                progress_percent: data.progress_percent,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE => {
            let data = unsafe { event.data.vehicle_core_state };
            Some(Event::VehicleCoreState(VehicleCoreStateEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                armed: data.armed != 0,
                battery_percent: data.battery_percent,
            }))
        }
        _ => None,
    }
}
