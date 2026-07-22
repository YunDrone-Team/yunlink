/**
 * @file src/runtime/state/subscriptions.cpp
 * @brief Runtime subscription facade implementation.
 */

#include "../core/internal.hpp"

namespace yunlink {

StateSubscriber::StateSubscriber(Runtime* runtime) : runtime_(runtime) {}
void StateSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t StateSubscriber::subscribe_vehicle_core(VehicleCoreHandler cb) {
    return runtime_ ? runtime_->subscribe_vehicle_core_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_px4_state(Px4StateHandler cb) {
    return runtime_ ? runtime_->subscribe_px4_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_odom_status(OdomStatusHandler cb) {
    return runtime_ ? runtime_->subscribe_odom_status_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_control_fsm_state(UavControlFsmStateHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_control_fsm_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_controller_state(UavControllerStateHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_controller_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_gimbal_params(GimbalParamsHandler cb) {
    return runtime_ ? runtime_->subscribe_gimbal_params_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_local_odom(LocalOdomHandler cb) {
    return runtime_ ? runtime_->subscribe_local_odom_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_control_cmd(UavControlCmdHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_control_cmd_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_control_state(UavControlStateHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_control_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_command_execution_status(CommandExecutionStatusHandler cb) {
    return runtime_ ? runtime_->subscribe_command_execution_status_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_odom_state(OdomStateHandler cb) {
    return runtime_ ? runtime_->subscribe_odom_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_sunray_runtime_diagnostic(SunrayRuntimeDiagnosticHandler cb) {
    return runtime_ ? runtime_->subscribe_sunray_runtime_diagnostic_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_host_system(HostSystemHandler cb) {
    return runtime_ ? runtime_->subscribe_host_system_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_topic_samples(TopicSampleHandler cb) {
    return runtime_ ? runtime_->subscribe_topic_sample_internal(std::move(cb)) : 0;
}

void StateSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

EventSubscriber::EventSubscriber(Runtime* runtime) : runtime_(runtime) {}
void EventSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t EventSubscriber::subscribe_vehicle_event(VehicleEventHandler cb) {
    return runtime_ ? runtime_->subscribe_vehicle_event_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_command_results(CommandResultHandler cb) {
    return runtime_ ? runtime_->subscribe_command_result_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_authority_status(AuthorityStatusHandler cb) {
    return runtime_ ? runtime_->subscribe_authority_status_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_bulk_channel_descriptors(BulkChannelDescriptorHandler cb) {
    return runtime_ ? runtime_->subscribe_bulk_channel_descriptor_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_packet_trace(PacketTraceHandler cb) {
    return runtime_ ? runtime_->subscribe_packet_trace_internal(std::move(cb)) : 0;
}

void EventSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

SystemServiceSubscriber::SystemServiceSubscriber(Runtime* runtime) : runtime_(runtime) {}
void SystemServiceSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t SystemServiceSubscriber::subscribe_feature_list_requests(FeatureListRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_list_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_list_responses(FeatureListResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_list_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_get_requests(FeatureGetRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_get_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_get_responses(FeatureGetResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_get_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_start_requests(FeatureStartRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_start_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_start_responses(FeatureStartResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_start_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_stop_requests(FeatureStopRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_stop_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_feature_stop_responses(FeatureStopResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_feature_stop_response_internal(std::move(cb)) : 0;
}

size_t
SystemServiceSubscriber::subscribe_runtime_log_list_requests(RuntimeLogListRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_runtime_log_list_request_internal(std::move(cb)) : 0;
}

size_t
SystemServiceSubscriber::subscribe_runtime_log_list_responses(RuntimeLogListResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_runtime_log_list_response_internal(std::move(cb)) : 0;
}

size_t
SystemServiceSubscriber::subscribe_runtime_log_read_requests(RuntimeLogReadRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_runtime_log_read_request_internal(std::move(cb)) : 0;
}

size_t
SystemServiceSubscriber::subscribe_runtime_log_read_responses(RuntimeLogReadResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_runtime_log_read_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_topic_list_requests(TopicListRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_topic_list_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_topic_list_responses(TopicListResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_topic_list_response_internal(std::move(cb)) : 0;
}

size_t
SystemServiceSubscriber::subscribe_topic_subscription_requests(TopicSubscriptionRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_topic_subscription_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_topic_subscription_responses(
    TopicSubscriptionResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_topic_subscription_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_managed_entity_list_requests(
    ManagedEntityListRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_managed_entity_list_request_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_managed_entity_list_responses(
    ManagedEntityListResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_managed_entity_list_response_internal(std::move(cb)) : 0;
}

size_t SystemServiceSubscriber::subscribe_managed_entity_directory_changed(
    ManagedEntityDirectoryChangedHandler cb) {
    return runtime_ ? runtime_->subscribe_managed_entity_directory_changed_internal(std::move(cb))
                    : 0;
}

size_t SystemServiceSubscriber::subscribe_managed_entity_attachment_requests(
    ManagedEntityAttachmentRequestHandler cb) {
    return runtime_ ? runtime_->subscribe_managed_entity_attachment_request_internal(std::move(cb))
                    : 0;
}

size_t SystemServiceSubscriber::subscribe_managed_entity_attachment_responses(
    ManagedEntityAttachmentResponseHandler cb) {
    return runtime_ ? runtime_->subscribe_managed_entity_attachment_response_internal(std::move(cb))
                    : 0;
}

void SystemServiceSubscriber::unsubscribe(size_t token) {
    if (runtime_) {
        runtime_->unsubscribe_semantic(token);
    }
}

CommandSubscriber::CommandSubscriber(Runtime* runtime) : runtime_(runtime) {}
void CommandSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t CommandSubscriber::subscribe_takeoff(TakeoffHandler cb) {
    return runtime_ ? runtime_->subscribe_takeoff_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_land(LandHandler cb) {
    return runtime_ ? runtime_->subscribe_land_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_return(ReturnHandler cb) {
    return runtime_ ? runtime_->subscribe_return_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_goto(GotoHandler cb) {
    return runtime_ ? runtime_->subscribe_goto_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_velocity_setpoint(VelocitySetpointHandler cb) {
    return runtime_ ? runtime_->subscribe_velocity_setpoint_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_trajectory_chunk(TrajectoryChunkHandler cb) {
    return runtime_ ? runtime_->subscribe_trajectory_chunk_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_formation_task(FormationTaskHandler cb) {
    return runtime_ ? runtime_->subscribe_formation_task_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_uav_control(UavControlHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_control_internal(std::move(cb)) : 0;
}

void CommandSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

}  // namespace yunlink
