"""YunLink Wire v2 Python SDK."""

from .v2 import (
    Error,
    Event,
    Family,
    MessageHandle,
    Peer,
    Profile,
    Qos,
    Runtime,
    RuntimeConfig,
    Target,
    TargetScope,
    TypeRef,
)
from .configuration import *
from .configuration_client import ConfigurationClient, ConfigurationEndpoint, configuration
from .configuration_codec import ConfigurationCodecError, decode as decode_configuration, encode as encode_configuration
from .discovery import Advertisement, DiscoveredEntity, discover
from .core import *
from .core_codec import CoreCodecError, encode as encode_core

__all__ = [
    "Error",
    "Event",
    "Family",
    "MessageHandle",
    "Peer",
    "Profile",
    "Qos",
    "Runtime",
    "RuntimeConfig",
    "Target",
    "TargetScope",
    "TypeRef",
    "ConfigurationClient",
    "ConfigurationEndpoint",
    "ConfigurationCodecError",
    "configuration",
    "decode_configuration",
    "encode_configuration",
    "Advertisement",
    "DiscoveredEntity",
    "discover",
    "CoreCodecError",
    "encode_core",
]

__all__ += [name for name in dir() if name in {
    "Availability", "ActionPhase", "EntityDescriptor", "EntityDirectory",
    "AttachmentRequest", "AttachmentResponse", "AuthorityRequest", "AuthorityStatus",
    "StreamDescriptor", "StreamCatalog", "StreamSubscription",
    "StreamSubscriptionStatus", "StreamSample", "ActionUpdate",
}]

__all__ += [name for name in dir() if name.startswith("Config")]
