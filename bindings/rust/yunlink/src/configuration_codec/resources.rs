impl ConfigurationPayload for ConfigResourceListRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.u8(0);
            Ok(())
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let _ = reader.u8()?;
            Ok(Self)
        })
    }
}

impl ConfigurationPayload for ConfigResourceListResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            writer.list(&self.resources, write_descriptor)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                resources: reader.list(read_descriptor)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceDescribeRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| writer.text(&self.resource_id))
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceDescribeResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_descriptor(writer, &self.resource)?;
            writer.list(&self.fields, write_schema)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                resource: read_descriptor(reader)?,
                fields: reader.list(read_schema)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceGetRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceGetResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_snapshot(writer, &self.snapshot)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                snapshot: read_snapshot(reader)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourcePatchRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)?;
            writer.text(&self.expected_revision)?;
            writer.list(&self.updates, write_field_value)?;
            writer.boolean(self.validate_only);
            Ok(())
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
                expected_revision: reader.text()?,
                updates: reader.list(read_field_value)?,
                validate_only: reader.boolean()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourcePatchResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_snapshot(writer, &self.snapshot)?;
            writer.boolean(self.candidate_snapshot.is_some());
            if let Some(candidate_snapshot) = &self.candidate_snapshot {
                write_snapshot(writer, candidate_snapshot)?;
            }
            writer.list(&self.errors, write_error)?;
            write_effects(writer, &self.effects)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                snapshot: read_snapshot(reader)?,
                candidate_snapshot: if reader.boolean()? {
                    Some(read_snapshot(reader)?)
                } else {
                    None
                },
                errors: reader.list(read_error)?,
                effects: read_effects(reader)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceApplyRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.expected_revision)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                expected_revision: reader.text()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceApplyResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            writer.text(&self.applied_revision)?;
            writer.u8(self.outcome as u8);
            write_effects(writer, &self.effects)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                applied_revision: reader.text()?,
                outcome: outcome(reader.u8()?)?,
                effects: read_effects(reader)?,
            })
        })
    }
}
