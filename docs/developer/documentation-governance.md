# 文档维护与排布规则

这篇文档用于维护文档本身。它回答一个问题：当 YunLink 继续增长时，新内容应该写在哪里，旧内容应该怎样保持可读。

## 先确定文档的读者

写文档前，先判断读者是谁。

如果读者要理解协议行为，内容应该进入 `docs/protocol/`。
如果读者要使用 Rust 或 Python SDK，内容应该进入 `docs/bindings/`。
如果读者要开发 YunLink core、C ABI 或语言包装，内容应该进入 `docs/developer/`。
如果读者要运行工具或脚本，内容应该进入 `tools/` 或对应工具目录。

这个判断比文件名更重要。文档应该围绕读者任务组织，而不是围绕作者刚改过的文件组织。

## 当前文档分区

| 分区 | 主要读者 | 应该放什么 | 不应该放什么 |
| --- | --- | --- | --- |
| `docs/protocol/` | 协议实现者和集成者 | 协议语义、线包约束、场景行为 | Rust/Python 绑定维护流程 |
| `docs/bindings/` | SDK 使用者和集成测试维护者 | SDK 使用指南、bindings 总览、测试矩阵 | C++ core 详细开发流程 |
| `docs/developer/` | YunLink 和语言绑定开发者 | 开发路线、跨层流程、验证清单、维护规则 | 协议字段的重复定义 |
| `bindings/*/README.md` | 进入某个绑定目录的维护者 | 本目录分层说明和跳转入口 | 长篇教程和完整背景 |
| `tools/*/README.md` | 工具使用者 | 工具构建、运行和 smoke test | SDK API 参考 |

## 新增能力时的文档同步规则

新增能力时，按改动影响范围同步文档。

| 改动影响 | 必须检查的文档 |
| --- | --- |
| 改协议字段或消息语义 | `docs/protocol/` |
| 改 C++ runtime 行为 | `docs/developer/core-development.md` 和相关测试说明 |
| 新增 C ABI 函数或 struct | `docs/developer/c-abi-development.md`、`docs/bindings/overview.md` |
| 新增 Rust safe API | `docs/developer/rust-bindings-development.md`、`docs/bindings/rust-sdk.md` |
| 新增 Python high-level API | `docs/developer/python-bindings-development.md`、`docs/bindings/python-sdk.md` |
| 新增跨层能力 | `docs/developer/feature-to-binding-playbook.md` 和 `docs/developer/verification-release-checklist.md` |
| 新增工具页面或 monitor 页面 | 对应工具 README 和相关 developer 文档 |

如果文档已经覆盖了规则，不一定每次都改文字。但是提交说明里应该能说清楚“检查过，现有文档仍然准确”。

## 写作顺序

开发者文档优先采用这个顺序：

1. 先描述开发者会看到的现象。
2. 再给这个现象命名。
3. 然后解释机制。
4. 最后落到代码路径和验证命令。

例如，不要一开始就说“更新 tagged union parser”。先说“事件从 C ABI 出来时只有一个 type 字段和一块共享数据区”，再命名 tagged union，然后说明 Rust 和 Python 应该怎样解析。

## 链接和入口规则

新增文档后，至少要保证它从一个入口可达。

常见入口是：

- `docs/README.md`
- `docs/developer/README.md`
- `docs/bindings/overview.md`
- 对应目录的 `README.md`

如果一篇文档只能靠搜索才能找到，它还不是稳定文档体系的一部分。

## 路径和命令规则

文档中的路径应该是真实路径。写路径后，最好本地确认它存在。

文档中的命令应该从仓库根目录运行，除非文档明确说明当前工作目录。

推荐写法：

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
```

不推荐只写：

```bash
cargo test
```

因为仓库根目录没有顶层 Rust workspace，短命令会让新人遇到 rust-analyzer 或 Cargo 找不到项目的问题。

## 避免重复定义

协议字段以 `docs/protocol/` 为准。
C ABI 函数签名以 `include/yunlink/c/abi/functions.h` 为准。
C ABI 类型布局以 `include/yunlink/c/abi/types.h` 为准。
Rust raw 绑定以 `bindings/rust/yunlink-sys/` 为准。
Rust safe API 以 `bindings/rust/yunlink/` 为准。
Python high-level API 以 `bindings/python/src/yunlink/` 为准。

其他文档可以解释这些内容，但不应该复制一份完整定义后让它变成第二个事实来源。

## 提交前文档检查

文档改动提交前至少检查：

- 新文档是否能从入口到达。
- 相对链接是否存在。
- 代码路径是否存在。
- 命令是否说明了运行目录或使用了明确 manifest。
- 新术语是否在第一次出现时解释。
- 是否把协议、C ABI、SDK 和工具文档放在了正确分区。

如果改动很小，可以手动检查。
如果改动新增了多篇文档，建议运行一次 Markdown 相对链接检查。
