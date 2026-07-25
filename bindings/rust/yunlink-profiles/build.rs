use std::{env, path::PathBuf};

fn main() {
    let root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("manifest dir"))
        .join("../../..");
    let profiles = root.join("profiles");
    let mobility = profiles.join("org.yunlink.mobility/v1/mobility.proto");
    let sunray = profiles.join("com.yundrone.sunray/v1/sunray.proto");
    let protoc = protoc_bin_vendored::protoc_bin_path().expect("vendored protoc");
    env::set_var("PROTOC", protoc);
    prost_build::Config::new()
        .compile_protos(&[mobility.clone(), sunray.clone()], &[profiles])
        .expect("compile YunLink profiles");
    println!("cargo:rerun-if-changed={}", mobility.display());
    println!("cargo:rerun-if-changed={}", sunray.display());
}
