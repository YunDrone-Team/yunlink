use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

fn run(cmd: &mut Command) {
    let status = cmd.status().expect("failed to spawn command");
    if !status.success() {
        panic!("command failed with status {status}");
    }
}

fn watch_source_tree(root: &std::path::Path) {
    let entries = fs::read_dir(root)
        .unwrap_or_else(|error| panic!("failed to read {}: {error}", root.display()));
    for entry in entries {
        let path = entry.expect("failed to read source entry").path();
        if path.is_dir() {
            watch_source_tree(&path);
        } else if matches!(
            path.extension().and_then(|extension| extension.to_str()),
            Some("c" | "cc" | "cpp" | "h" | "hpp")
        ) {
            println!("cargo:rerun-if-changed={}", path.display());
        }
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
    let build_type = match env::var("PROFILE").as_deref() {
        Ok("release") => "Release",
        _ => "Debug",
    };

    watch_source_tree(&repo_root.join("include/yunlink"));
    watch_source_tree(&repo_root.join("src"));
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
        .arg(format!("-DCMAKE_BUILD_TYPE={build_type}"))
        .arg("-DYUNLINK_BUILD_TESTS=OFF")
        .arg("-DYUNLINK_BUILD_PROFILES=OFF"));

    run(Command::new("cmake")
        .arg("--build")
        .arg(&build_dir)
        .arg("--target")
        .arg("yunlink_ffi"));

    println!("cargo:rustc-link-search=native={}", build_dir.display());
    println!("cargo:rustc-link-lib=dylib=yunlink_ffi");
    println!("cargo:build_dir={}", build_dir.display());

    if cfg!(any(target_os = "macos", target_os = "linux")) {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", build_dir.display());
    }
}
