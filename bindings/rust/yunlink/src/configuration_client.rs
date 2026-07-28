//! Async request helper for the generic YunLink Configuration Service.

use std::time::Duration;

use tokio::time::timeout;

use crate::{
    ConfigResourceApplyRequest, ConfigResourceApplyResponse, ConfigResourceDescribeRequest,
    ConfigResourceDescribeResponse, ConfigResourceGetRequest, ConfigResourceGetResponse,
    ConfigResourceListRequest, ConfigResourceListResponse, ConfigResourcePatchRequest,
    ConfigResourcePatchResponse, ConfigResourceVariantActivateRequest,
    ConfigResourceVariantActivateResponse, ConfigResourceVariantCreateRequest,
    ConfigResourceVariantCreateResponse, ConfigResourceVariantDeleteRequest,
    ConfigResourceVariantDeleteResponse, ConfigResourceVariantListRequest,
    ConfigResourceVariantListResponse, ConfigResourceVariantSaveCurrentRequest,
    ConfigResourceVariantSaveCurrentResponse, ConfigurationPayload, Error, Event, Family, Peer,
    Qos, Result, Runtime, Target, TypeRef,
};

const TYPE_PROFILE: &str = "yunlink.core";
const TYPE_MAJOR: u16 = 2;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigurationEndpoint {
    pub peer: Peer,
    pub session_id: u64,
    pub endpoint_uid: String,
}

impl ConfigurationEndpoint {
    pub fn new(peer: Peer, session_id: u64, endpoint_uid: impl Into<String>) -> Self {
        Self {
            peer,
            session_id,
            endpoint_uid: endpoint_uid.into(),
        }
    }
}

pub struct ConfigurationClient<'a> {
    runtime: &'a Runtime,
    request_timeout: Duration,
}

impl Runtime {
    pub fn configuration(&self) -> ConfigurationClient<'_> {
        ConfigurationClient {
            runtime: self,
            request_timeout: Duration::from_secs(5),
        }
    }
}

impl<'a> ConfigurationClient<'a> {
    pub fn with_timeout(mut self, value: Duration) -> Self {
        self.request_timeout = value;
        self
    }

    async fn request<Req: ConfigurationPayload, Resp: ConfigurationPayload>(
        &self,
        endpoint: &ConfigurationEndpoint,
        request_operation: u8,
        response_operation: u8,
        request_type: &'static str,
        response_type: &'static str,
        request: &Req,
    ) -> Result<Resp> {
        let mut events = self.runtime.subscribe();
        let handle = self.runtime.publish(
            &endpoint.peer,
            endpoint.session_id,
            Family::Configuration,
            request_operation,
            &Target::Endpoint(vec![endpoint.endpoint_uid.clone()]),
            &TypeRef::new(TYPE_PROFILE, TYPE_MAJOR, request_type),
            &request.encode()?,
            0,
            self.request_timeout
                .as_millis()
                .try_into()
                .unwrap_or(u32::MAX),
            Qos::ReliableOrdered,
            "",
        )?;
        timeout(self.request_timeout, async {
            loop {
                match events.recv().await.map_err(|_| Error { code: 8 })? {
                    Event::Configuration(message)
                        if message.peer_id == endpoint.peer.id
                            && message.session_id == endpoint.session_id
                            && message.correlation_id == handle.message_id
                            && message.operation == response_operation
                            && message.type_ref.profile_id == TYPE_PROFILE
                            && message.type_ref.major == TYPE_MAJOR
                            && message.type_ref.type_name == response_type =>
                    {
                        return Resp::decode(&message.payload);
                    }
                    _ => {}
                }
            }
        })
        .await
        .map_err(|_| Error { code: 8 })?
    }

    pub async fn list(
        &self,
        endpoint: &ConfigurationEndpoint,
    ) -> Result<ConfigResourceListResponse> {
        self.request(
            endpoint,
            1,
            2,
            "configuration.resource_list.request",
            "configuration.resource_list.response",
            &ConfigResourceListRequest,
        )
        .await
    }
    pub async fn describe(
        &self,
        endpoint: &ConfigurationEndpoint,
        resource_id: impl Into<String>,
    ) -> Result<ConfigResourceDescribeResponse> {
        self.request(
            endpoint,
            3,
            4,
            "configuration.resource_describe.request",
            "configuration.resource_describe.response",
            &ConfigResourceDescribeRequest {
                resource_id: resource_id.into(),
            },
        )
        .await
    }
    pub async fn get(
        &self,
        endpoint: &ConfigurationEndpoint,
        resource_id: impl Into<String>,
    ) -> Result<ConfigResourceGetResponse> {
        self.get_variant(endpoint, resource_id, "").await
    }
    pub async fn get_variant(
        &self,
        endpoint: &ConfigurationEndpoint,
        resource_id: impl Into<String>,
        variant_id: impl Into<String>,
    ) -> Result<ConfigResourceGetResponse> {
        self.request(
            endpoint,
            5,
            6,
            "configuration.resource_get.request",
            "configuration.resource_get.response",
            &ConfigResourceGetRequest {
                resource_id: resource_id.into(),
                variant_id: variant_id.into(),
            },
        )
        .await
    }
    pub async fn patch(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourcePatchRequest,
    ) -> Result<ConfigResourcePatchResponse> {
        self.request(
            endpoint,
            7,
            8,
            "configuration.resource_patch.request",
            "configuration.resource_patch.response",
            &request,
        )
        .await
    }
    pub async fn apply(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourceApplyRequest,
    ) -> Result<ConfigResourceApplyResponse> {
        self.request(
            endpoint,
            9,
            10,
            "configuration.resource_apply.request",
            "configuration.resource_apply.response",
            &request,
        )
        .await
    }
    pub async fn list_variants(
        &self,
        endpoint: &ConfigurationEndpoint,
        resource_id: impl Into<String>,
    ) -> Result<ConfigResourceVariantListResponse> {
        self.request(
            endpoint,
            11,
            12,
            "configuration.variant_list.request",
            "configuration.variant_list.response",
            &ConfigResourceVariantListRequest {
                resource_id: resource_id.into(),
            },
        )
        .await
    }
    pub async fn create_variant(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourceVariantCreateRequest,
    ) -> Result<ConfigResourceVariantCreateResponse> {
        self.request(
            endpoint,
            13,
            14,
            "configuration.variant_create.request",
            "configuration.variant_create.response",
            &request,
        )
        .await
    }
    pub async fn save_current_as_variant(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourceVariantSaveCurrentRequest,
    ) -> Result<ConfigResourceVariantSaveCurrentResponse> {
        self.request(
            endpoint,
            15,
            16,
            "configuration.variant_save_current.request",
            "configuration.variant_save_current.response",
            &request,
        )
        .await
    }
    pub async fn activate_variant(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourceVariantActivateRequest,
    ) -> Result<ConfigResourceVariantActivateResponse> {
        self.request(
            endpoint,
            17,
            18,
            "configuration.variant_activate.request",
            "configuration.variant_activate.response",
            &request,
        )
        .await
    }
    pub async fn delete_variant(
        &self,
        endpoint: &ConfigurationEndpoint,
        request: ConfigResourceVariantDeleteRequest,
    ) -> Result<ConfigResourceVariantDeleteResponse> {
        self.request(
            endpoint,
            19,
            20,
            "configuration.variant_delete.request",
            "configuration.variant_delete.response",
            &request,
        )
        .await
    }
}
