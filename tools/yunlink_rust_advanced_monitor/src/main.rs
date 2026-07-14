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

fn configure_chinese_fonts(ctx: &egui::Context) {
    const FONT_CANDIDATES: &[&str] = &[
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
    ];
    let Some(font_data) = FONT_CANDIDATES
        .iter()
        .find_map(|path| std::fs::read(path).ok())
    else {
        return;
    };

    let mut fonts = egui::FontDefinitions::default();
    fonts.font_data.insert(
        "cjk".to_string(),
        std::sync::Arc::new(egui::FontData::from_owned(font_data)),
    );
    if let Some(family) = fonts.families.get_mut(&egui::FontFamily::Proportional) {
        family.insert(0, "cjk".to_string());
    }
    if let Some(family) = fonts.families.get_mut(&egui::FontFamily::Monospace) {
        family.push("cjk".to_string());
    }
    ctx.set_fonts(fonts);
}

fn main() -> eframe::Result<()> {
    // Keep CLI parsing local and dependency-free for the prototype. The C++ Qt
    // monitor accepts the same option names, so switching between tools is easy
    // during demos.
    let config = MonitorConfig::from_args(std::env::args().skip(1));
    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("YunLink Rust 高级监视器")
            .with_inner_size([1180.0, 760.0]),
        ..Default::default()
    };

    eframe::run_native(
        "YunLink Rust 高级监视器",
        native_options,
        Box::new(move |cc| {
            configure_chinese_fonts(&cc.egui_ctx);
            Ok(Box::new(MonitorApp::new(config)))
        }),
    )
}
