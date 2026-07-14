from __future__ import annotations

from .configuration import ConfigFieldValue, ConfigurationHandle
from .errors import call_native
from .types import PeerConnection, Session, TargetSelector


class ConfigurationRuntimeMixin:
    @staticmethod
    def _configuration_handle(data: dict[str, int]) -> ConfigurationHandle:
        return ConfigurationHandle(**data)

    def configuration_resource_list(
        self, peer: PeerConnection, session: Session, target: TargetSelector
    ) -> ConfigurationHandle:
        return self._configuration_handle(
            call_native(
                self._core.configuration_list,
                peer.id,
                session.session_id,
                target.to_native(),
            )
        )

    def configuration_resource_describe(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        resource_id: str,
    ) -> ConfigurationHandle:
        return self._configuration_handle(
            call_native(
                self._core.configuration_describe,
                peer.id,
                session.session_id,
                target.to_native(),
                resource_id,
            )
        )

    def configuration_resource_get(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        resource_id: str,
    ) -> ConfigurationHandle:
        return self._configuration_handle(
            call_native(
                self._core.configuration_get,
                peer.id,
                session.session_id,
                target.to_native(),
                resource_id,
            )
        )

    def configuration_resource_patch(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        resource_id: str,
        expected_revision: str,
        updates: list[ConfigFieldValue],
        validate_only: bool = False,
    ) -> ConfigurationHandle:
        return self._configuration_handle(
            call_native(
                self._core.configuration_patch,
                peer.id,
                session.session_id,
                target.to_native(),
                resource_id,
                expected_revision,
                [item.to_native() for item in updates],
                validate_only,
            )
        )

    def configuration_resource_apply(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        resource_id: str,
        expected_revision: str,
    ) -> ConfigurationHandle:
        return self._configuration_handle(
            call_native(
                self._core.configuration_apply,
                peer.id,
                session.session_id,
                target.to_native(),
                resource_id,
                expected_revision,
            )
        )
