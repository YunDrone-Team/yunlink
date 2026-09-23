use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

/// 目标平台 —— 注意是**目标**，不是宿主。
///
/// 这里**不能**用 `cfg!(target_os = ...)`：build script 自身是编译给宿主机运行的
/// 可执行文件，它里面的 `cfg!` 反映的是**宿主**平台。原生构建时两者一致，所以这个
/// 错误看不出来；一旦交叉编译就会静默走错分支 —— 不报错，只是产出错的链接参数、
/// 找错运行库文件名。目标平台只能读 `CARGO_CFG_TARGET_OS`（cargo 注入的
/// `CARGO_CFG_<cfg>` 系列变量之一）。
fn target_os() -> String {
    env::var("CARGO_CFG_TARGET_OS").unwrap_or_default()
}

/// 判断某个文件名是不是本平台的 YunLink 运行库。
///
/// 按前后缀匹配而不是精确相等：CMake 在 macOS / Linux 上会额外生成带
/// VERSION/SOVERSION 的文件（`libyunlink_ffi.2.dylib`、`libyunlink_ffi.so.2`）
/// 以及指向它们的无版本符号链接。写死精确文件名，哪天 CMake 的版本号一变就又错了。
fn is_runtime_library(target_os: &str, file_name: &str) -> bool {
    match target_os {
        "macos" => file_name.starts_with("libyunlink_ffi") && file_name.ends_with(".dylib"),
        "windows" => file_name == "yunlink_ffi.dll",
        _ => file_name.starts_with("libyunlink_ffi.so"),
    }
}

/// 守住「本 crate 目前只支持原生构建」这条边界。
///
/// 为什么必须显式拒绝：下面调 CMake 时**没有指定工具链文件**，所以 CMake 用的是
/// 宿主编译器，产出的运行库是本平台的。而 cargo 可能正在为另一个目标链接它 ——
/// 那种产物不但没用，还会一路通过构建、直到装到用户机器上才以
/// "cannot open shared object file" / "找不到 DLL" 的形式炸掉。
///
/// 既然编出来必然是坏的，就在这里直接失败，并说清楚出路。
fn assert_native_build() {
    let host = env::var("HOST").unwrap_or_default();
    let target = env::var("TARGET").unwrap_or_default();

    // 拿不到变量时不拦（保守：宁可漏报也不误伤构造异常的构建环境）。
    if host.is_empty() || target.is_empty() || host == target {
        return;
    }

    panic!(
        "\n\
         YunLink C++ 运行库无法交叉编译。\n\
         \n\
         本 crate 调 CMake 时没有传工具链文件，CMake 会用**宿主**编译器生成运行库，\n\
         而 cargo 正为另一个目标链接它：\n\
         \x20 HOST   = {host}\n\
         \x20 TARGET = {target}\n\
         产物必然不可用 —— 要么链接失败，要么打出一个带错误动态库、到用户机器上\n\
         才报错的包。这里直接失败，不产出这种“看着构建成功”的东西。\n\
         \n\
         可行做法：在那三个平台上各自原生构建（每平台一台机器，或 CI 的原生 runner 矩阵）。\n"
    );
}

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

/// 找到运行库实际落在哪个目录。
///
/// - 单配置生成器（Ninja / Unix Makefiles）：产物直接放在 `build_dir` 根。
/// - 多配置生成器（Visual Studio / Xcode）：放在 `build_dir/<Config>/`。
///
/// 两种都探一遍再决定，这样 `CMAKE_GENERATOR="Visual Studio 17 2022"` 不需要改代码
/// 就能用 —— 而这一条对 Windows 很实际：CMake 用 Ninja 生成器时**不会**自动定位
/// MSVC，必须从「VS 开发者命令提示符」里启动（环境里有 cl.exe）才能配置成功；
/// 换成 VS 生成器就能在任意 shell 里构建。
fn locate_library_dir(build_dir: &Path, build_type: &str) -> PathBuf {
    let candidates = [build_dir.to_path_buf(), build_dir.join(build_type)];
    let platform = target_os();

    for candidate in &candidates {
        let Ok(entries) = fs::read_dir(candidate) else {
            continue;
        };
        for entry in entries.flatten() {
            let name = entry.file_name();
            let Some(name) = name.to_str() else { continue };
            if is_runtime_library(&platform, name) {
                return candidate.clone();
            }
        }
    }

    panic!(
        "CMake 构建完成，但在预期位置找不到 YunLink 运行库（目标平台 {platform}）。\n\
         已查找：\n{}\n\
         若换了生成器或输出目录，请在这里补上候选路径。",
        candidates
            .iter()
            .map(|candidate| format!("  {}", candidate.display()))
            .collect::<Vec<_>>()
            .join("\n")
    );
}

fn main() {
    assert_native_build();

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

    // `--config` 对单配置生成器是空操作、对多配置生成器是必需项，所以无条件带上。
    // 少了它，VS/Xcode 生成器即使 cargo 在 release 档也只会编出 Debug 产物。
    run(Command::new("cmake")
        .arg("--build")
        .arg(&build_dir)
        .arg("--config")
        .arg(build_type)
        .arg("--target")
        .arg("yunlink_ffi"));

    let library_dir = locate_library_dir(&build_dir, build_type);

    println!("cargo:rustc-link-search=native={}", library_dir.display());
    println!("cargo:rustc-link-lib=dylib=yunlink_ffi");
    // 供依赖方（`yunlink` / `sunray-gateway`）定位运行库用。这里给的是**运行库所在目录**，
    // 不是 CMake 的构建根目录 —— 多配置生成器下两者不同，给错了依赖方就找不到文件。
    println!("cargo:build_dir={}", library_dir.display());

    if matches!(target_os().as_str(), "macos" | "linux") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", library_dir.display());
    }
}
