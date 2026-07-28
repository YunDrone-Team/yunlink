#include <cassert>

#include "yunlink/core/core_messages_v2.hpp"

int main() {
    using namespace yunlink::v2;
    EntityDirectory directory;
    directory.endpoint_uid = "endpoint.bridge";
    directory.revision = "42";
    EntityDescriptor entity;
    entity.entity_uid = "e-endpoint.bridge-device-1";
    entity.kind = "mobile_robot";
    entity.display_name = "Device 1";
    entity.hardware_id = "serial-1";
    entity.attributes = {{"provider.name", "alpha"}, {"provider.id", "1"}};
    entity.capabilities = {"stream", "action"};
    entity.availability = Availability::kOnline;
    directory.entities.push_back(entity);
    EntityDirectory decoded;
    assert(decode(encode(directory), &decoded));
    assert(decoded.endpoint_uid == directory.endpoint_uid);
    assert(decoded.entities.size() == 1);
    assert(decoded.entities.front().attributes == entity.attributes);

    ActionUpdate update{ActionPhase::kRunning, 0, 75, "executing"};
    ActionUpdate decoded_update;
    assert(decode(encode(update), &decoded_update));
    assert(decoded_update.phase == ActionPhase::kRunning);
    assert(decoded_update.progress_percent == 75);

    StreamCatalog catalog;
    catalog.revision = "1";
    catalog.streams.push_back({"entity.stream.pose",
                               {"org.yunlink.mobility", 1, 0, "Odometry"},
                               "protobuf",
                               {{"frame", "map"}}});
    StreamCatalog decoded_catalog;
    assert(decode(encode(catalog), &decoded_catalog));
    assert(decoded_catalog.streams.front().type.type_name == "Odometry");

    StreamSample sample;
    sample.stream_uid = "entity.stream.raw";
    sample.encoding = "application/json";
    sample.metadata = {{"schema.name", "example/PointCloud"}, {"schema.digest", "abcd"}};
    sample.source_timestamp_ns = 123456789;
    sample.sequence = 42;
    sample.data = {0, 1, 2, 255};
    StreamSample decoded_sample;
    const Bytes encoded_sample = encode(sample);
    assert(decode(encoded_sample, &decoded_sample));
    assert(decoded_sample.stream_uid == sample.stream_uid);
    assert(decoded_sample.metadata == sample.metadata);
    assert(decoded_sample.data == sample.data);
    Bytes truncated_sample = encoded_sample;
    truncated_sample.pop_back();
    assert(!decode(truncated_sample, &decoded_sample));

    yunlink::ConfigResourceGetResponse configuration;
    configuration.status = yunlink::ConfigServiceStatus::kOk;
    configuration.message = "ok";
    configuration.snapshot.resource_id = "bridge.runtime";
    configuration.snapshot.revision = "7";
    configuration.snapshot.values = {
        {"endpoint_uid", yunlink::ConfigValue::from_string("endpoint.bridge")},
        {"tcp_listen_port", yunlink::ConfigValue::from_int64(9696)},
    };
    yunlink::ConfigResourceGetResponse decoded_configuration;
    assert(decode(encode(configuration), &decoded_configuration));
    assert(decoded_configuration.snapshot.values.size() == 2);
    assert(decoded_configuration.snapshot.values.front().value.string_value == "endpoint.bridge");

    // Configuration resources are provider-neutral. Exercise the complete shape that
    // adapters use for parameter managers: groups, update policy, variants and a
    // revision-protected multi-field dry run.
    yunlink::ConfigResourceDescribeResponse describe;
    describe.status = yunlink::ConfigServiceStatus::kOk;
    describe.message = "ok";
    describe.resource = {"sunray.params.flight", "Flight", "", true, true, true, true};
    yunlink::ConfigFieldSchema field;
    field.path = "control.max_speed";
    field.group_path = "control";
    field.title = "Maximum speed";
    field.type = yunlink::ConfigValueType::kDouble;
    field.unit = "m/s";
    field.update_policy = yunlink::ConfigFieldUpdatePolicy::kHotReload;
    field.has_minimum = true;
    field.minimum = 0.0;
    field.has_maximum = true;
    field.maximum = 10.0;
    field.choices = {{yunlink::ConfigValue::from_double(3.0), "Indoor"}};
    describe.fields.push_back(field);
    yunlink::ConfigResourceDescribeResponse decoded_describe;
    const Bytes encoded_describe = encode(describe);
    assert(decode(encoded_describe, &decoded_describe));
    assert(decoded_describe.resource.variants_supported);
    assert(decoded_describe.fields.front().group_path == "control");
    assert(decoded_describe.fields.front().update_policy ==
           yunlink::ConfigFieldUpdatePolicy::kHotReload);
    assert(decoded_describe.fields.front().unit == "m/s");
    Bytes truncated_describe = encoded_describe;
    truncated_describe.pop_back();
    assert(!decode(truncated_describe, &decoded_describe));
    Bytes trailing_describe = encoded_describe;
    trailing_describe.push_back(0);
    assert(!decode(trailing_describe, &decoded_describe));

    yunlink::ConfigResourcePatchRequest patch;
    patch.resource_id = "sunray.params.flight";
    patch.variant_id = "indoor";
    patch.expected_revision = "rev-7";
    patch.validate_only = true;
    patch.updates = {
        {"control.max_speed", yunlink::ConfigValue::from_double(3.5)},
        {"control.enabled", yunlink::ConfigValue::from_bool(true)},
    };
    yunlink::ConfigResourcePatchRequest decoded_patch;
    assert(decode(encode(patch), &decoded_patch));
    assert(decoded_patch.validate_only);
    assert(decoded_patch.updates.size() == 2);
    assert(decoded_patch.variant_id == "indoor");
    const Bytes patch_golden = {
        0x14, 0x00, 's', 'u', 'n', 'r', 'a', 'y', '.', 'p', 'a', 'r', 'a', 'm', 's', '.',
        'f', 'l', 'i', 'g', 'h', 't', 0x06, 0x00, 'i', 'n', 'd', 'o', 'o', 'r', 0x05, 0x00,
        'r', 'e', 'v', '-', '7', 0x02, 0x00, 0x11, 0x00, 'c', 'o', 'n', 't', 'r', 'o', 'l',
        '.', 'm', 'a', 'x', '_', 's', 'p', 'e', 'e', 'd', 0x03, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x0c, 0x40, 0x0f, 0x00, 'c', 'o', 'n', 't', 'r', 'o', 'l', '.', 'e',
        'n', 'a', 'b', 'l', 'e', 'd', 0x01, 0x01, 0x01};
    assert(encode(patch) == patch_golden);

    // A dry-run exposes both the persisted snapshot and the provider-normalized
    // candidate. This lets clients present the actual proposed values before save.
    yunlink::ConfigResourcePatchResponse patch_preview;
    patch_preview.status = yunlink::ConfigServiceStatus::kOk;
    patch_preview.message = "validated";
    patch_preview.snapshot.resource_id = patch.resource_id;
    patch_preview.snapshot.revision = "r1";
    patch_preview.snapshot.applied_revision = "r1";
    patch_preview.snapshot.variant_id = patch.variant_id;
    patch_preview.snapshot.active_variant_id = patch.variant_id;
    patch_preview.snapshot.values = {
        {"control.max_speed", yunlink::ConfigValue::from_double(3.0)},
    };
    patch_preview.has_candidate_snapshot = true;
    patch_preview.candidate_snapshot = patch_preview.snapshot;
    patch_preview.candidate_snapshot.revision = "candidate-2";
    patch_preview.candidate_snapshot.values.front().value = yunlink::ConfigValue::from_double(3.5);
    yunlink::ConfigResourcePatchResponse decoded_patch_preview;
    const Bytes patch_preview_golden = {
        0x00, 0x09, 0x00, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x61, 0x74, 0x65, 0x64,
        0x14, 0x00, 0x73, 0x75, 0x6e, 0x72, 0x61, 0x79, 0x2e, 0x70, 0x61, 0x72,
        0x61, 0x6d, 0x73, 0x2e, 0x66, 0x6c, 0x69, 0x67, 0x68, 0x74, 0x02, 0x00,
        0x72, 0x31, 0x02, 0x00, 0x72, 0x31, 0x06, 0x00, 0x69, 0x6e, 0x64, 0x6f,
        0x6f, 0x72, 0x06, 0x00, 0x69, 0x6e, 0x64, 0x6f, 0x6f, 0x72, 0x01, 0x00,
        0x11, 0x00, 0x63, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x2e, 0x6d, 0x61,
        0x78, 0x5f, 0x73, 0x70, 0x65, 0x65, 0x64, 0x03, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x08, 0x40, 0x01, 0x14, 0x00, 0x73, 0x75, 0x6e, 0x72, 0x61,
        0x79, 0x2e, 0x70, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x2e, 0x66, 0x6c, 0x69,
        0x67, 0x68, 0x74, 0x0b, 0x00, 0x63, 0x61, 0x6e, 0x64, 0x69, 0x64, 0x61,
        0x74, 0x65, 0x2d, 0x32, 0x02, 0x00, 0x72, 0x31, 0x06, 0x00, 0x69, 0x6e,
        0x64, 0x6f, 0x6f, 0x72, 0x06, 0x00, 0x69, 0x6e, 0x64, 0x6f, 0x6f, 0x72,
        0x01, 0x00, 0x11, 0x00, 0x63, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x2e,
        0x6d, 0x61, 0x78, 0x5f, 0x73, 0x70, 0x65, 0x65, 0x64, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert(encode(patch_preview) == patch_preview_golden);
    assert(decode(patch_preview_golden, &decoded_patch_preview));
    assert(decoded_patch_preview.has_candidate_snapshot);
    assert(decoded_patch_preview.candidate_snapshot.revision == "candidate-2");
    assert(decoded_patch_preview.candidate_snapshot.values.front().value.double_value == 3.5);

    yunlink::ConfigResourceVariantListResponse variants;
    variants.status = yunlink::ConfigServiceStatus::kOk;
    variants.message = "ok";
    variants.active_variant_id = "indoor";
    variants.variants = {{"default", "Default", "r0", 100, false, false},
                         {"indoor", "Indoor", "r1", 200, true, true}};
    yunlink::ConfigResourceVariantListResponse decoded_variants;
    assert(decode(encode(variants), &decoded_variants));
    assert(decoded_variants.variants.size() == 2);
    assert(decoded_variants.variants.back().active);

    yunlink::ConfigResourceVariantCreateRequest create_variant;
    create_variant.resource_id = patch.resource_id;
    create_variant.variant_id = "outdoor";
    create_variant.source = yunlink::ConfigVariantSource::kActive;
    create_variant.expected_active_revision = "r1";
    yunlink::ConfigResourceVariantCreateRequest decoded_create_variant;
    assert(decode(encode(create_variant), &decoded_create_variant));
    assert(decoded_create_variant.source == yunlink::ConfigVariantSource::kActive);

    yunlink::ConfigResourceVariantSaveCurrentRequest save_variant;
    save_variant.resource_id = patch.resource_id;
    save_variant.variant_id = "backup";
    save_variant.expected_variant_revision = "r2";
    save_variant.expected_active_revision = "r1";
    yunlink::ConfigResourceVariantSaveCurrentRequest decoded_save_variant;
    assert(decode(encode(save_variant), &decoded_save_variant));
    assert(decoded_save_variant.expected_variant_revision == "r2");

    yunlink::ConfigResourceVariantActivateResponse activate_variant;
    activate_variant.status = yunlink::ConfigServiceStatus::kOk;
    activate_variant.message = "applied";
    activate_variant.applied_revision = "r3";
    activate_variant.outcome = yunlink::ConfigApplyOutcome::kApplied;
    activate_variant.effects.requirement = yunlink::ConfigApplyRequirement::kNone;
    yunlink::ConfigResourceVariantActivateResponse decoded_activate_variant;
    assert(decode(encode(activate_variant), &decoded_activate_variant));
    assert(decoded_activate_variant.outcome == yunlink::ConfigApplyOutcome::kApplied);

    LogListResponse logs;
    logs.success = true;
    logs.message = "ok";
    logs.logs.push_back({"bridge.runtime",
                         "endpoint.bridge",
                         "Bridge Runtime",
                         "running",
                         1,
                         0,
                         false,
                         0,
                         {{"component", "bridge"}},
                         "active"});
    LogListResponse decoded_logs;
    assert(decode(encode(logs), &decoded_logs));
    assert(decoded_logs.logs.size() == 1);
    assert(decoded_logs.logs.front().owner_uid == "endpoint.bridge");

    LogReadResponse log_read{true, "ok", "bridge.runtime", "line\n", 5, false, true};
    LogReadResponse decoded_log_read;
    assert(decode(encode(log_read), &decoded_log_read));
    assert(decoded_log_read.chunk == "line\n");
    return 0;
}
