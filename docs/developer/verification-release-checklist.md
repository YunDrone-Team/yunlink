# 验证、发布与回归检查清单

这篇文档用于提交前自查。它不是替代 CI，而是帮助开发者在本地先发现明显问题。

## 先按改动范围选择验证

| 改动范围 | 必跑验证 |
| --- | --- |
| 只改文档 | markdown 链接检查或手动链接检查 |
| 只改 C++ core | CMake build + 相关 C++ tests |
| 改 C ABI | CMake build + C ABI contract / loader / roundtrip tests |
| 改 Rust SDK | Rust tests |
| 改 Python SDK | Python unittest + wheel smoke |
| 改 monitor UI | 对应 app build + smoke run |
| 跨层能力 | core + C ABI + Rust + Python + 文档 |

## C++ 和 C ABI

```bash
cmake -S . -B build/ninja-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

如果只想先跑 C ABI：

```bash
ctest --test-dir build/ninja-debug -R "test_c_ffi_(v1|contract|loader)" --output-on-failure
```

检查重点：

- 新函数是否导出。
- 新 struct 布局是否符合预期。
- loader 是否能找到动态库符号。
- 错误码是否稳定。

## Rust

```bash
cargo fmt --manifest-path bindings/rust/Cargo.toml --all
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
```

如果改了 Rust Advanced Monitor：

```bash
cargo fmt --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml
cargo test --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml
cargo build --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml
```

检查重点：

- `yunlink-sys` 没有和 C header 脱节。
- safe `yunlink` 没有把 raw pointer 暴露给 UI。
- `unsafe` 范围足够小。
- 事件和错误映射能覆盖新能力。

## Python

```bash
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -e bindings/python
python -m unittest discover -s bindings/python/tests
```

wheel smoke：

```bash
tools/bindings/build_python_wheel.sh
```

检查重点：

- native extension 能正确加载。
- Python exception 映射合理。
- sync 和 async API 行为一致。
- polling 线程能正常关闭。

## 文档

文档改动至少检查：

- 新增文件是否从 `docs/README.md` 或相关索引可达。
- 相对链接是否存在。
- 文档是否说明代码路径和验证命令。
- 如果新增协议字段，是否更新 `docs/protocol/`。
- 如果新增语言 API，是否更新 `docs/bindings/` 或 `docs/developer/`。

## 发布前的 ABI 自查

C ABI 一旦被外部语言 SDK 依赖，就要更谨慎。

提交前问：

- 是否修改了已有函数签名？如果是，通常不允许。
- 是否修改了已有 enum 数值？如果是，通常不允许。
- 是否改变了已有 struct 字段顺序？如果是，通常不允许。
- 新 struct 是否需要 `struct_size`？
- 新字符串字段是否有固定 buffer 大小？
- 新错误码是否被 Rust / Python 映射？
- 新事件 type 是否被旧 SDK 安全忽略？

## CI 失败时的排查顺序

### Rust 找不到项目

确认 VS Code 或命令使用了正确 manifest：

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
```

仓库根目录没有顶层 Rust `Cargo.toml`。

### Rust 链接失败

先看 `bindings/rust/yunlink-sys/build.rs` 是否成功构建 `yunlink_ffi`。再看平台 rpath / PATH / DYLD_LIBRARY_PATH。

### Python extension 加载失败

先确认 native extension 是否构建成功，再确认 Python 解释器版本和 wheel 平台标签。

### C ABI loader 失败

先跑：

```bash
ctest --test-dir build/ninja-debug -R test_c_ffi_loader --output-on-failure
```

它能帮助判断是动态库不存在、符号不存在，还是 ABI version 不一致。

## 合入前完成标准

一次开发完成时，至少应该能提供：

- 改动范围说明。
- 受影响层级列表。
- 本地验证命令和结果。
- 文档更新位置。
- 已知未完成项或刻意不做的范围。

如果改动涉及跨语言能力，还应该能说明：

- C++ core 行为测试在哪里。
- C ABI contract 测试在哪里。
- Rust safe API 测试在哪里。
- Python API 测试在哪里。
