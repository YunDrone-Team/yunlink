"""Synchronous generic Configuration Service client over a YunLink Runtime."""

from __future__ import annotations

import dataclasses
import queue
import time
from typing import Any, TypeVar

from .configuration import *
from .configuration_codec import decode, encode
from .v2 import Event, Family, Peer, Qos, Runtime, Target, TypeRef

T = TypeVar("T")
_PROFILE_ID = "yunlink.core"
_PROFILE_MAJOR = 2


@dataclasses.dataclass(frozen=True)
class ConfigurationEndpoint:
    peer: Peer
    session_id: int
    endpoint_uid: str


class ConfigurationClient:
    """A single-runtime consumer for request/response Configuration operations.

    The current Python runtime exposes one owned event queue. Use this client as the
    runtime's Configuration event consumer; unrelated events remain retrievable with
    :meth:`take_deferred_events` instead of being silently discarded.
    """

    def __init__(self, runtime: Runtime, timeout_s: float = 5.0) -> None:
        self._runtime = runtime
        self._timeout_s = timeout_s
        self._deferred: list[Event] = []

    def with_timeout(self, timeout_s: float) -> "ConfigurationClient":
        return ConfigurationClient(self._runtime, timeout_s)

    def take_deferred_events(self) -> tuple[Event, ...]:
        result = tuple(self._deferred)
        self._deferred.clear()
        return result

    def _request(
        self,
        endpoint: ConfigurationEndpoint,
        request_operation: int,
        response_operation: int,
        request_type: str,
        response_type: str,
        request: Any,
        response_type_class: type[T],
    ) -> T:
        handle = self._runtime.publish(
            endpoint.peer,
            endpoint.session_id,
            Family.CONFIGURATION,
            request_operation,
            Target.endpoint(endpoint.endpoint_uid),
            TypeRef(_PROFILE_ID, _PROFILE_MAJOR, request_type),
            encode(request),
            ttl_ms=min(int(self._timeout_s * 1000), 2**32 - 1),
            qos=Qos.RELIABLE_ORDERED,
        )
        deadline = time.monotonic() + self._timeout_s
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("YunLink Configuration response timed out")
            try:
                event = self._runtime.events.get(timeout=remaining)
            except queue.Empty as error:
                raise TimeoutError("YunLink Configuration response timed out") from error
            if self._matches(event, endpoint, handle.message_id, response_operation, response_type):
                return decode(response_type_class, event.payload)
            self._deferred.append(event)

    @staticmethod
    def _matches(
        event: Event,
        endpoint: ConfigurationEndpoint,
        correlation_id: int,
        operation: int,
        type_name: str,
    ) -> bool:
        return (
            event.peer_id == endpoint.peer.peer_id
            and event.session_id == endpoint.session_id
            and event.family == Family.CONFIGURATION
            and event.correlation_id == correlation_id
            and event.operation == operation
            and event.type_ref.profile_id == _PROFILE_ID
            and event.type_ref.major == _PROFILE_MAJOR
            and event.type_ref.type_name == type_name
        )

    def list(self, endpoint: ConfigurationEndpoint) -> ConfigResourceListResponse:
        return self._request(endpoint, 1, 2, "configuration.resource_list.request", "configuration.resource_list.response", ConfigResourceListRequest(), ConfigResourceListResponse)

    def describe(self, endpoint: ConfigurationEndpoint, resource_id: str) -> ConfigResourceDescribeResponse:
        return self._request(endpoint, 3, 4, "configuration.resource_describe.request", "configuration.resource_describe.response", ConfigResourceDescribeRequest(resource_id), ConfigResourceDescribeResponse)

    def get(self, endpoint: ConfigurationEndpoint, resource_id: str) -> ConfigResourceGetResponse:
        return self.get_variant(endpoint, resource_id, "")

    def get_variant(self, endpoint: ConfigurationEndpoint, resource_id: str, variant_id: str) -> ConfigResourceGetResponse:
        return self._request(endpoint, 5, 6, "configuration.resource_get.request", "configuration.resource_get.response", ConfigResourceGetRequest(resource_id, variant_id), ConfigResourceGetResponse)

    def patch(self, endpoint: ConfigurationEndpoint, request: ConfigResourcePatchRequest) -> ConfigResourcePatchResponse:
        return self._request(endpoint, 7, 8, "configuration.resource_patch.request", "configuration.resource_patch.response", request, ConfigResourcePatchResponse)

    def apply(self, endpoint: ConfigurationEndpoint, request: ConfigResourceApplyRequest) -> ConfigResourceApplyResponse:
        return self._request(endpoint, 9, 10, "configuration.resource_apply.request", "configuration.resource_apply.response", request, ConfigResourceApplyResponse)

    def list_variants(self, endpoint: ConfigurationEndpoint, resource_id: str) -> ConfigResourceVariantListResponse:
        return self._request(endpoint, 11, 12, "configuration.variant_list.request", "configuration.variant_list.response", ConfigResourceVariantListRequest(resource_id), ConfigResourceVariantListResponse)

    def create_variant(self, endpoint: ConfigurationEndpoint, request: ConfigResourceVariantCreateRequest) -> ConfigResourceVariantCreateResponse:
        return self._request(endpoint, 13, 14, "configuration.variant_create.request", "configuration.variant_create.response", request, ConfigResourceVariantCreateResponse)

    def save_current_as_variant(self, endpoint: ConfigurationEndpoint, request: ConfigResourceVariantSaveCurrentRequest) -> ConfigResourceVariantSaveCurrentResponse:
        return self._request(endpoint, 15, 16, "configuration.variant_save_current.request", "configuration.variant_save_current.response", request, ConfigResourceVariantSaveCurrentResponse)

    def activate_variant(self, endpoint: ConfigurationEndpoint, request: ConfigResourceVariantActivateRequest) -> ConfigResourceVariantActivateResponse:
        return self._request(endpoint, 17, 18, "configuration.variant_activate.request", "configuration.variant_activate.response", request, ConfigResourceVariantActivateResponse)

    def delete_variant(self, endpoint: ConfigurationEndpoint, request: ConfigResourceVariantDeleteRequest) -> ConfigResourceVariantDeleteResponse:
        return self._request(endpoint, 19, 20, "configuration.variant_delete.request", "configuration.variant_delete.response", request, ConfigResourceVariantDeleteResponse)


def configuration(runtime: Runtime, timeout_s: float = 5.0) -> ConfigurationClient:
    """Create an owned Configuration client without extending the C ABI."""
    return ConfigurationClient(runtime, timeout_s)
