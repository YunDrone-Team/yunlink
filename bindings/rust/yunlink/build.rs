use std::env;

/// 目标平台，不是宿主。
///
/// `cfg!(target_os = ...)` 在这里是错的：build script 自身编译给宿主机运行，
/// 里面的 `cfg!` 反映**宿主**平台。原生构建时两者一致所以看不出问题，交叉编译时
/// 会静默走错分支。目标平台读 `CARGO_CFG_TARGET_OS`。
fn target_os() -> String {
    env::var("CARGO_CFG_TARGET_OS").unwrap_or_default()
}

fn main() {
    // 由 `yunlink-sys` 经 `cargo:build_dir=...` 传出（links = "yunlink_ffi" 会自动
    // 加 `DEP_YUNLINK_FFI_` 前缀）。注意它给的是**运行库所在目录**，多配置生成器
    // （Visual Studio / Xcode）下与 CMake 构建根目录不是一回事。
    let build_dir = env::var("DEP_YUNLINK_FFI_BUILD_DIR").expect("DEP_YUNLINK_FFI_BUILD_DIR");

    println!("cargo:rerun-if-env-changed=DEP_YUNLINK_FFI_BUILD_DIR");

    if matches!(target_os().as_str(), "macos" | "linux") {
        // The final example/test executables need their own runtime search path.
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", build_dir);
    }
}
