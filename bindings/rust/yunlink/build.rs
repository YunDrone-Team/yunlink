use std::env;

fn main() {
    let build_dir = env::var("DEP_YUNLINK_FFI_BUILD_DIR").expect("DEP_YUNLINK_FFI_BUILD_DIR");

    println!("cargo:rerun-if-env-changed=DEP_YUNLINK_FFI_BUILD_DIR");

    if cfg!(target_os = "macos") || cfg!(target_os = "linux") {
        // The final example/test executables need their own runtime search path.
        println!("cargo:rustc-link-arg-examples=-Wl,-rpath,{}", build_dir);
        println!("cargo:rustc-link-arg-tests=-Wl,-rpath,{}", build_dir);
    }
}
