import time
import unittest

import yunlink
from yunlink import AgentType, Runtime, RuntimeConfig


class RuntimeContractTests(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
