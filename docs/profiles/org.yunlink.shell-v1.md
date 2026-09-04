# org.yunlink.shell v1

`org.yunlink.shell@1.0` is an optional Endpoint-level interactive Shell
contract. It reuses YunLink RPC for lifecycle and input, and StreamSample for
output. It does not introduce a Core message family or any vehicle/ROS
semantics.

The client can open the current Endpoint user's default login shell, write raw
input bytes, resize its PTY, and close it. The client cannot select a command,
shell executable, operating-system user, or privilege escalation method.

The output stream UID is `<endpoint_uid>.shell`, uses type `ShellOutput` with
`raw-bytes` encoding, and carries `shell.session_uid`. EOF frames may also carry
`shell.eof`, `shell.exit_code`, and `shell.dropped_bytes` metadata.

Implementations must validate terminal sizes (`20..400` columns and `5..200`
rows), cap one write request at 16 KiB, reject stale session UIDs, and authorize
the output subscription independently of knowledge of its stream UID.

This Profile grants the permissions of the process user on the remote machine.
Deployments should keep it disabled unless remote diagnostic Shell access is
explicitly intended.
