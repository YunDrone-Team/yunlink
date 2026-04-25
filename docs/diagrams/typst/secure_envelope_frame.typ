#import "@preview/bytefield:0.0.7": *

#set page(
  paper: "a4",
  flipped: true,
  margin: (x: 10mm, y: 9mm),
)

#set par(justify: false, leading: 0.58em)
#set text(font: "Noto Sans CJK SC", size: 8.2pt, fill: rgb("#243039"))
#set heading(numbering: none)

#let mono(body) = text(font: "Noto Sans Mono CJK SC", body)

#let c-sync = rgb("#DCEBFA")
#let c-struct = rgb("#EEF2F4")
#let c-control = rgb("#F6E6BE")
#let c-state = rgb("#DCECDD")
#let c-risk = rgb("#F6D8D5")
#let c-wire = rgb("#F7F5F1")

#let byte-col-header = bitheader(
  numbers: none,
  angle: 0deg,
  text-size: 7pt,
  0, [#text(weight: "semibold")[0]],
  8, [#text(weight: "semibold")[1]],
  16, [#text(weight: "semibold")[2]],
  24, [#text(weight: "semibold")[3]],
  32, [#text(weight: "semibold")[4]],
  40, [#text(weight: "semibold")[5]],
  48, [#text(weight: "semibold")[6]],
  56, [#text(weight: "semibold")[7]],
  64, [#text(weight: "semibold")[8]],
  72, [#text(weight: "semibold")[9]],
  80, [#text(weight: "semibold")[10]],
  88, [#text(weight: "semibold")[11]],
)

#let field-caption(fill, label, note) = [
  #box(
    inset: (x: 5pt, y: 3pt),
    stroke: (paint: rgb("#5E6A71"), thickness: 0.6pt),
    fill: fill,
    radius: 4pt,
  )[
    #text(weight: "semibold")[#label]
  ]
  #linebreak()
  #text(size: 8pt, fill: rgb("#51606B"))[#note]
]

#align(center)[
  #text(size: 15pt, weight: "bold")[SecureEnvelope v1 Protocol Frame]
  #linebreak()
  #text(size: 8pt, fill: rgb("#51606B"))[
    Fixed header is rendered to-scale. Variable tail is rendered as structural schema, not literal width.
  ]
]

#v(5pt)

#box(
  stroke: (paint: rgb("#C9D1D6"), thickness: 0.7pt),
  fill: c-wire,
  radius: 6pt,
  inset: 7pt,
)[
  #text(size: 7.6pt, fill: rgb("#51606B"))[
    Byte columns shown per row: 0..11. Fields continue contiguously across rows until byte 75.
  ]
  #v(3pt)
  #bytefield(
    bpr: 96,
    byte-col-header,
    bytes(4, fill: c-sync)[magic / "SURY"],
    byte(fill: c-struct)[protocol_major],
    byte(fill: c-struct)[header_version],
    bytes(2, fill: c-struct)[flags],
    bytes(2, fill: c-struct)[header_len],
    bytes(4, fill: c-struct)[payload_len],
    byte(fill: c-control)[qos_class],
    byte(fill: c-control)[message_family],
    bytes(2, fill: c-control)[message_type],
    bytes(2, fill: c-control)[schema_version],
    bytes(8, fill: c-control)[session_id],
    bytes(8, fill: c-control)[message_id],
    bytes(8, fill: c-control)[correlation_id],
    bytes(8, fill: c-control)[created_at_ms],
    bytes(4, fill: c-control)[ttl_ms],
    byte(fill: c-state)[src.agent_type],
    bytes(4, fill: c-state)[src.agent_id],
    byte(fill: c-state)[src.role],
    byte(fill: c-state)[tgt.scope],
    byte(fill: c-state)[tgt.type],
    bytes(4, fill: c-state)[tgt.group_id],
    bytes(2, fill: c-struct)[target_count],
    bytes(4, fill: c-struct)[key_epoch],
    bytes(2, fill: c-struct)[auth_tag_len],
  )
]

#v(6pt)

#box(
  stroke: (paint: rgb("#C9D1D6"), thickness: 0.7pt),
  fill: c-wire,
  radius: 6pt,
  inset: 7pt,
)[
  #text(size: 8.8pt, weight: "semibold")[Variable tail schema]
  #v(3pt)
  #bytefield(
    bpr: 32,
    bytes(4, fill: c-state)[target_ids[]],
    bytes(4, fill: c-struct)[auth_tag[]],
    bytes(8, fill: c-control)[payload[]],
    bytes(4, fill: c-risk)[checksum],
  )
  #v(2pt)
  #text(size: 7.6pt, fill: rgb("#51606B"))[
    Tail order is fixed: `target_ids[target_count]`, then `auth_tag[auth_tag_len]`,
    then `payload[payload_len]`, then the trailing 4-byte additive `checksum`.
  ]
]

#v(5pt)

#grid(
  columns: (1fr, 1fr, 1fr),
  gutter: 6pt,
  field-caption(c-sync, "Sync / Frame Start", "Magic bytes 0..3, fixed ASCII marker 'SURY'."),
  field-caption(c-struct, "Structure / Length", "Header sizing, payload sizing, auth-tag sizing, checksum support."),
  field-caption(c-control, "Control / Semantic Routing", "QoS, family, type, schema, session and correlation metadata."),
  field-caption(c-state, "Identity / Targeting", "Source identity plus target scope, type, group and ID metadata."),
  field-caption(c-risk, "Integrity", "Final additive checksum over magic through payload end."),
  field-caption(c-wire, "Read This Diagram As", "A stable wire contract: fixed header is literal, tail is structural."),
)

#v(5pt)

#box(
  stroke: (paint: rgb("#B7AEA1"), thickness: 0.6pt),
  fill: c-wire,
  radius: 6pt,
  inset: 6pt,
)[ 
  #text(size: 7.7pt)[
    Fixed header size = 76 bytes.
    #mono[`header_len = 76 + 4 * target_count + auth_tag_len`].
    Payload interpretation is driven only by #mono[`(message_family, message_type, schema_version)`],
    not by transport or private frame numbering.
  ]
]

#v(6pt)

#box(
  stroke: (paint: rgb("#C9D1D6"), thickness: 0.7pt),
  fill: white,
  radius: 6pt,
  inset: 8pt,
)[
  #text(size: 10pt, weight: "bold")[1. 帧同步与固定头]
  #v(2pt)
  `magic` 占前 4 字节，当前固定为 #mono[`SURY`]。接收端首先据此完成帧同步和头部合法性初筛；
  如果 magic 不匹配，协议端应直接判为无效头并丢弃。紧随其后的固定头负责给出协议版本、
  头长度、payload 长度、QoS、消息族、会话与关联标识等核心控制信息，它定义了整条线包的结构边界。

  #v(5pt)
  #text(size: 10pt, weight: "bold")[2. 语义解释与可变尾部]
  #v(2pt)
  `target_ids` 与 `auth_tag` 让头部长度成为可变值，因此图中把尾部单独画成 structural schema，而不是假装成固定宽度。
  真正的业务体由 #mono[`message_family + message_type + schema_version`] 唯一解释；也就是说，同一条 TCP 或 UDP 路由本身
  不能承载隐含语义，协议解释权只来自信封头字段。

  #v(5pt)
  #text(size: 10pt, weight: "bold")[3. 完整性与实现提示]
  #v(2pt)
  `checksum` 位于尾部最后 4 字节，覆盖范围是从 magic 到 payload 末尾的全部字节。实现上应按“先校验 magic，
  再校验长度推导，再校验 checksum，最后进入 payload 解码”的顺序处理。对于当前仓库，这张图既是协议说明图，
  也可以直接作为 `ProtocolCodec` 编解码实现和联调日志字段核对的参考视图。
]
