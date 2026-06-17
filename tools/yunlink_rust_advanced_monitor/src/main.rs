//! Entry point for the Rust Advanced Monitor teaching prototype.
//!
//! The executable intentionally starts from a normal Rust desktop application
//! shape. The interesting FFI boundary is hidden behind the safe `yunlink`
//! crate and is explained in the in-app ABI page.

mod app;
mod ffi_explain;
mod model;
mod runtime_client;

use app::MonitorApp;
use model::MonitorConfig;

fn main() -> eframe::Result<()> {
    // Keep CLI parsing local and dependency-free for the prototype. The C++ Qt
    // monitor accepts the same option names, so switching between tools is easy
    // during demos.
    let config = MonitorConfig::from_args(std::env::args().skip(1));
    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("YunLink Rust Advanced Monitor")
            .with_inner_size([1180.0, 760.0]),
        ..Default::default()
    };

    eframe::run_native(
        "YunLink Rust Advanced Monitor",
        native_options,
        Box::new(move |_cc| Ok(Box::new(MonitorApp::new(config)))),
    )
}
