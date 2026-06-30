/**
 * @file src/runtime/state/registry.cpp
 * @brief Runtime semantic subscription registry.
 */

#include "../core/internal.hpp"

namespace yunlink {

size_t Runtime::subscribe_takeoff_internal(CommandSubscriber::TakeoffHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->takeoff_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_land_internal(CommandSubscriber::LandHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->land_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_return_internal(CommandSubscriber::ReturnHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->return_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_goto_internal(CommandSubscriber::GotoHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->goto_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_velocity_setpoint_internal(CommandSubscriber::VelocitySetpointHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->velocity_setpoint_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_trajectory_chunk_internal(CommandSubscriber::TrajectoryChunkHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->trajectory_chunk_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_formation_task_internal(CommandSubscriber::FormationTaskHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->formation_task_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_vehicle_core_internal(StateSubscriber::VehicleCoreHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->vehicle_core_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_px4_state_internal(StateSubscriber::Px4StateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->px4_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_odom_status_internal(StateSubscriber::OdomStatusHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->odom_status_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_uav_control_fsm_state_internal(StateSubscriber::UavControlFsmStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_control_fsm_state_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_uav_controller_state_internal(StateSubscriber::UavControllerStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_controller_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_gimbal_params_internal(StateSubscriber::GimbalParamsHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->gimbal_params_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_local_odom_internal(StateSubscriber::LocalOdomHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->local_odom_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_uav_control_cmd_internal(StateSubscriber::UavControlCmdHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_control_cmd_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_uav_control_state_internal(StateSubscriber::UavControlStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_control_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_command_execution_status_internal(
    StateSubscriber::CommandExecutionStatusHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->command_execution_status_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_odom_state_internal(StateSubscriber::OdomStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->odom_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_sunray_runtime_diagnostic_internal(
    StateSubscriber::SunrayRuntimeDiagnosticHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->sunray_runtime_diagnostic_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_vehicle_event_internal(EventSubscriber::VehicleEventHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->vehicle_event_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_command_result_internal(EventSubscriber::CommandResultHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->command_result_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_authority_status_internal(EventSubscriber::AuthorityStatusHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->authority_status_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_packet_trace_internal(EventSubscriber::PacketTraceHandler cb) {
    const size_t bus_token = bus_.subscribe_packet_trace(std::move(cb));
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->packet_trace_bus_tokens[token] = bus_token;
    return token;
}

size_t Runtime::subscribe_feature_list_request_internal(
    SystemServiceSubscriber::FeatureListRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_list_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_list_response_internal(
    SystemServiceSubscriber::FeatureListResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_list_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_get_request_internal(
    SystemServiceSubscriber::FeatureGetRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_get_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_get_response_internal(
    SystemServiceSubscriber::FeatureGetResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_get_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_start_request_internal(
    SystemServiceSubscriber::FeatureStartRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_start_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_start_response_internal(
    SystemServiceSubscriber::FeatureStartResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_start_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_stop_request_internal(
    SystemServiceSubscriber::FeatureStopRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_stop_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_stop_response_internal(
    SystemServiceSubscriber::FeatureStopResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_stop_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_bulk_channel_descriptor_internal(
    EventSubscriber::BulkChannelDescriptorHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->bulk_channel_descriptor_handlers[token] = std::move(cb);
    return token;
}

void Runtime::unsubscribe_semantic(size_t token) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->takeoff_handlers.erase(token);
    impl_->land_handlers.erase(token);
    impl_->return_handlers.erase(token);
    impl_->goto_handlers.erase(token);
    impl_->velocity_setpoint_handlers.erase(token);
    impl_->trajectory_chunk_handlers.erase(token);
    impl_->formation_task_handlers.erase(token);
    impl_->vehicle_core_handlers.erase(token);
    impl_->px4_state_handlers.erase(token);
    impl_->odom_status_handlers.erase(token);
    impl_->uav_control_fsm_state_handlers.erase(token);
    impl_->uav_controller_state_handlers.erase(token);
    impl_->gimbal_params_handlers.erase(token);
    impl_->local_odom_handlers.erase(token);
    impl_->uav_control_cmd_handlers.erase(token);
    impl_->uav_control_state_handlers.erase(token);
    impl_->command_execution_status_handlers.erase(token);
    impl_->odom_state_handlers.erase(token);
    impl_->sunray_runtime_diagnostic_handlers.erase(token);
    impl_->vehicle_event_handlers.erase(token);
    impl_->command_result_handlers.erase(token);
    impl_->authority_status_handlers.erase(token);
    const auto packet_it = impl_->packet_trace_bus_tokens.find(token);
    if (packet_it != impl_->packet_trace_bus_tokens.end()) {
        bus_.unsubscribe(packet_it->second);
        impl_->packet_trace_bus_tokens.erase(packet_it);
    }
    impl_->feature_list_request_handlers.erase(token);
    impl_->feature_list_response_handlers.erase(token);
    impl_->feature_get_request_handlers.erase(token);
    impl_->feature_get_response_handlers.erase(token);
    impl_->feature_start_request_handlers.erase(token);
    impl_->feature_start_response_handlers.erase(token);
    impl_->feature_stop_request_handlers.erase(token);
    impl_->feature_stop_response_handlers.erase(token);
    impl_->bulk_channel_descriptor_handlers.erase(token);
}

}  // namespace yunlink
