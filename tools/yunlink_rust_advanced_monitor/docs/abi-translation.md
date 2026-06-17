# ABI Translation Notes

This prototype keeps the production path clean:

```text
MonitorApp
  calls RuntimeClient
    calls yunlink::Runtime
      calls yunlink_sys::yunlink_* extern functions
        calls exported C ABI in libyunlink_ffi
          calls the C++ yunlink runtime
```

Example command path:

```text
Runtime::publish_goto
  -> yunlink_sys::yunlink_command_publish_goto
  -> yunlink_command_publish_goto(...)
  -> C++ CommandPublisher::publish_goto(...)
```

The UI code does not import `yunlink-sys`. The only monitor module that imports
`yunlink-sys` is `ffi_explain`, and it only reads ABI metadata for display.
