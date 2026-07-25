pub mod org {
    pub mod yunlink {
        pub mod mobility {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.mobility.v1.rs"));
            }
        }
    }
}

pub mod com {
    pub mod yundrone {
        pub mod sunray {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/com.yundrone.sunray.v1.rs"));
            }
        }
    }
}

pub use com::yundrone::sunray::v1 as sunray;
pub use org::yunlink::mobility::v1 as mobility;

pub const MOBILITY_PROFILE_ID: &str = "org.yunlink.mobility";
pub const SUNRAY_PROFILE_ID: &str = "com.yundrone.sunray";

#[cfg(test)]
mod tests {
    use prost::Message;

    use super::{mobility, sunray};

    const GOTO_GOLDEN: &[u8] = &[
        0x0a, 0x03, 0x6d, 0x61, 0x70, 0x12, 0x1b, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf0, 0x3f, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x19, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0,
        0x3f,
    ];

    #[test]
    fn profile_payloads_match_cross_language_golden_vectors() {
        let goal = mobility::GotoGoal {
            frame_id: "map".into(),
            position: Some(mobility::Vector3 {
                x: 1.0,
                y: -2.0,
                z: 0.5,
            }),
            yaw_rad: 0.25,
        };
        assert_eq!(goal.encode_to_vec(), GOTO_GOLDEN);

        let request = sunray::FeatureStartRequest {
            name: "mapping".into(),
        };
        assert_eq!(request.encode_to_vec(), b"\x0a\x07mapping");
        assert!(sunray::FeatureStartRequest::decode(b"\x0a\x08mapping".as_slice()).is_err());
    }
}
