# 弱网 Profile 说明

`tools/testing/netem/profiles/` 中的 profile 用于后续双机弱网验证。

## 当前 profile

- `delay.env`
  纯延迟。
- `delay_jitter.env`
  延迟加抖动。
- `loss.env`
  延迟加丢包。
- `loss_reorder.env`
  延迟、丢包、乱序组合。
- `duplication.env`
  延迟加重复包。

## 使用方式

dry-run：

```bash
tools/testing/netem/apply_profile.sh --iface en0 --profile tools/testing/netem/profiles/delay.env --dry-run
tools/testing/netem/clear_profile.sh --iface en0 --dry-run
```

实际执行：

```bash
tools/testing/netem/apply_profile.sh --iface en0 --profile tools/testing/netem/profiles/delay.env
tools/testing/netem/clear_profile.sh --iface en0
```

## 原则

- 先有线基线，再加 profile。
- 每次只切一个 profile，避免归因混乱。
- 每次切 profile 都要记录在测试 summary 中。
