use std::{env, path::PathBuf};

fn main() {
    let root =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("manifest dir")).join("../../..");
    let profiles = root.join("profiles");
    let mobility = profiles.join("org.yunlink.mobility/v1/mobility.proto");
    let telemetry = profiles.join("org.yunlink.telemetry/v1/telemetry.proto");
    let media = profiles.join("org.yunlink.media/v1/media.proto");
    let sunray = profiles.join("com.yundrone.sunray/v2/sunray.proto");
    let system = profiles.join("org.yunlink.system/v1/system.proto");
    let shell = profiles.join("org.yunlink.shell/v1/shell.proto");
    let protoc = protoc_bin_vendored::protoc_bin_path().expect("vendored protoc");
    env::set_var("PROTOC", protoc);
    prost_build::Config::new()
        .compile_protos(
            &[
                mobility.clone(),
                telemetry.clone(),
                media.clone(),
                sunray.clone(),
                system.clone(),
                shell.clone(),
            ],
            &[profiles],
        )
        .expect("compile YunLink profiles");
    println!("cargo:rerun-if-changed={}", mobility.display());
    println!("cargo:rerun-if-changed={}", telemetry.display());
    println!("cargo:rerun-if-changed={}", media.display());
    println!("cargo:rerun-if-changed={}", sunray.display());
    println!("cargo:rerun-if-changed={}", system.display());
    println!("cargo:rerun-if-changed={}", shell.display());
}
