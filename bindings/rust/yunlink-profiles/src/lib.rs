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
