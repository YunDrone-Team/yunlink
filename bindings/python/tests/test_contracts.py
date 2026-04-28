import asyncio
import time
import unittest

import yunlink
from yunlink import AgentType, Runtime, RuntimeConfig


class RuntimeContractTests(unittest.TestCase):
    def test_wrap_native_error_maps_known_and_unknown_codes(self) -> None:
        known = yunlink._wrap_native_error(RuntimeError("YUNLINK_RESULT_NOT_FOUND"))
        self.assertIsInstance(known, yunlink.NotFoundError)
        self.assertEqual(known.code_name, "YUNLINK_RESULT_NOT_FOUND")

        unknown = yunlink._wrap_native_error(RuntimeError("YUNLINK_RESULT_SOMETHING_NEW"))
        self.assertIsInstance(unknown, yunlink.YunlinkError)
        self.assertNotIsInstance(unknown, yunlink.InvalidArgumentError)
        self.assertEqual(unknown.code_name, "YUNLINK_RESULT_SOMETHING_NEW")

    def test_connect_failure_maps_to_yunlink_error(self) -> None:
        runtime = Runtime.start(
            RuntimeConfig(15050, 15050, 15150, AgentType.GROUND_STATION, 9)
        )
        try:
            with self.assertRaises(yunlink.InvalidArgumentError):
                runtime.connect("256.0.0.1", 9)
        finally:
            runtime.close()

    def test_close_stops_background_poll_thread(self) -> None:
        runtime = Runtime.start(
            RuntimeConfig(15051, 15051, 15151, AgentType.GROUND_STATION, 10)
        )
        poll_thread = runtime._thread
        self.assertTrue(poll_thread.is_alive())

        runtime.close()

        deadline = time.time() + 1.0
        while time.time() < deadline and poll_thread.is_alive():
            time.sleep(0.01)
        self.assertFalse(poll_thread.is_alive())

    def test_close_releases_ports_for_restart(self) -> None:
        runtime = Runtime.start(
            RuntimeConfig(15052, 15052, 15152, AgentType.GROUND_STATION, 11)
        )
        runtime.close()

        restarted = Runtime.start(
            RuntimeConfig(15052, 15052, 15152, AgentType.GROUND_STATION, 11)
        )
        restarted.close()

    def test_poll_thread_surfaces_native_failure_as_error_event(self) -> None:
        class ExplodingCore:
            def __init__(self) -> None:
                self._raised = False

            def poll_event(self) -> None:
                if not self._raised:
                    time.sleep(0.05)
                    self._raised = True
                raise RuntimeError("YUNLINK_RESULT_CONNECT_ERROR")

            def stop(self) -> None:
                return None

        runtime = yunlink.Runtime(ExplodingCore())
        try:
            events = runtime.subscribe()
            event = events.get(timeout=1.0)
            self.assertIsInstance(event, yunlink.ErrorEvent)
            self.assertEqual(event.code, -1)
            self.assertEqual(event.message, "YUNLINK_RESULT_CONNECT_ERROR")
            self.assertIsInstance(runtime.last_poll_error(), yunlink.ConnectError)
        finally:
            runtime.close()

    def test_command_result_event_preserves_failure_metadata(self) -> None:
        event = yunlink._coerce_event(
            {
                "type": "command_result",
                "session_id": 42,
                "message_id": 9001,
                "correlation_id": 7001,
                "command_kind": 4,
                "phase": 5,
                "result_code": 13,
                "progress_percent": 0,
                "detail": "no-authority",
            }
        )

        self.assertIsInstance(event, yunlink.CommandResultEvent)
        self.assertEqual(event.session_id, 42)
        self.assertEqual(event.phase, 5)
        self.assertEqual(event.result_code, 13)
        self.assertEqual(event.detail, "no-authority")

    def test_sync_and_async_subscribers_share_domain_model(self) -> None:
        loop = asyncio.new_event_loop()
        self.addCleanup(loop.close)

        runtime = Runtime(yunlink.RuntimeCore())
        try:
            sync_events = runtime.subscribe()

            async def build_async_queue() -> asyncio.Queue:
                return runtime.subscribe_async()

            async_events = loop.run_until_complete(build_async_queue())
            payload = {
                "type": "command_result",
                "session_id": 8,
                "message_id": 80,
                "correlation_id": 80,
                "command_kind": 4,
                "phase": 4,
                "result_code": 0,
                "progress_percent": 100,
                "detail": "sync-async-shape",
            }
            event = yunlink._coerce_event(payload)
            sync_events.put(event)
            loop.call_soon_threadsafe(async_events.put_nowait, event)

            sync_event = sync_events.get(timeout=0.1)
            async_event = loop.run_until_complete(async_events.get())

            self.assertEqual(type(sync_event), type(async_event))
            self.assertEqual(sync_event.detail, async_event.detail)
            self.assertEqual(sync_event.correlation_id, async_event.correlation_id)
        finally:
            runtime.close()


if __name__ == "__main__":
    unittest.main()
