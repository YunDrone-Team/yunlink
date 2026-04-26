use std::env;
use std::path::PathBuf;
use std::process::Command;

fn run(cmd: &mut Command) {
    let status = cmd.status().expect("failed to spawn command");
    if !status.success() {
        panic!("command failed with status {status}");
    }
}

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let mut repo_root = manifest_dir.clone();
    repo_root.pop();
    repo_root.pop();
    repo_root.pop();
    let build_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR")).join("cmake-build");
    let generator = env::var("CMAKE_GENERATOR").unwrap_or_else(|_| "Ninja".to_string());

    println!(
        "cargo:rerun-if-changed={}",
        repo_root.join("include/yunlink/c/yunlink_c.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        repo_root.join("src/c/yunlink_c.cpp").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        repo_root.join("CMakeLists.txt").display()
    );

    run(Command::new("cmake")
        .arg("-S")
        .arg(&repo_root)
        .arg("-B")
        .arg(&build_dir)
        .arg("-G")
        .arg(&generator)
        .arg("-DCMAKE_BUILD_TYPE=Debug")
        .arg("-DYUNLINK_BUILD_EXAMPLES=OFF")
        .arg("-DYUNLINK_BUILD_TESTS=OFF"));

    run(Command::new("cmake")
        .arg("--build")
        .arg(&build_dir)
        .arg("--target")
        .arg("yunlink_ffi"));

    println!("cargo:rustc-link-search=native={}", build_dir.display());
    println!("cargo:rustc-link-lib=dylib=yunlink_ffi");
    println!("cargo:build_dir={}", build_dir.display());

    if cfg!(target_os = "macos") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", build_dir.display());
    } else if cfg!(target_os = "linux") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", build_dir.display());
    }
}
