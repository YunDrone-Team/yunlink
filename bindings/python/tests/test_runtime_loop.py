import asyncio
import queue
import time
import unittest

from yunlink import (
    AgentType,
    CommandResultEvent,
    ControlSource,
    GotoCommand,
    Runtime,
    RuntimeConfig,
    TargetSelector,
    VehicleCoreState,
    VehicleCoreStateEvent,
)


class RuntimeLoopTests(unittest.TestCase):
    def test_sync_runtime_loop_roundtrip(self) -> None:
        air = Runtime.start(RuntimeConfig(15030, 15030, 15130, AgentType.UAV, 1))
        ground = Runtime.start(
            RuntimeConfig(15031, 15031, 15131, AgentType.GROUND_STATION, 7)
        )
        try:
            events = ground.subscribe()
            peer = ground.connect("127.0.0.1", 15130)
            session = ground.open_session(peer, "python-ground")
            target = TargetSelector.entity(AgentType.UAV, 1)

            ground.request_authority(
                peer, session, target, ControlSource.GROUND_STATION, 3000, False
            )

            deadline = time.time() + 3.0
            lease = None
            while time.time() < deadline:
                lease = air.current_authority()
                if lease and lease.session_id == session.session_id:
                    break
                time.sleep(0.02)
            self.assertIsNotNone(lease)
            ground.renew_authority(
                peer, session, target, ControlSource.GROUND_STATION, 4500
            )

            command = ground.publish_goto(
                peer, session, target, GotoCommand(5.0, 1.0, 3.0, 0.25)
            )
            self.assertEqual(command.session_id, session.session_id)

            air.publish_vehicle_core_state(
                lease.peer,
                TargetSelector.entity(AgentType.GROUND_STATION, 7),
                VehicleCoreState(True, 3, 11.0, 12.0, 13.0, 0.1, 0.2, 0.3, 76.5),
                session.session_id,
            )

            saw_state = False
            result_count = 0
            deadline = time.time() + 3.0
            while time.time() < deadline:
                try:
                    event = events.get(timeout=0.1)
                except queue.Empty:
                    continue
                if isinstance(event, CommandResultEvent) and event.session_id == session.session_id:
                    result_count += 1
                if (
                    isinstance(event, VehicleCoreStateEvent)
                    and event.session_id == session.session_id
                    and abs(event.battery_percent - 76.5) < 1e-6
                ):
                    saw_state = True
                if saw_state and result_count >= 4:
                    break

            self.assertTrue(saw_state)
            self.assertGreaterEqual(result_count, 4)
        finally:
            ground.close()
            air.close()


class AsyncBridgeTests(unittest.IsolatedAsyncioTestCase):
    async def test_asyncio_bridge_receives_state_event(self) -> None:
        air = Runtime.start(RuntimeConfig(15040, 15040, 15140, AgentType.UAV, 1))
        ground = Runtime.start(
            RuntimeConfig(15041, 15041, 15141, AgentType.GROUND_STATION, 7)
        )
        try:
            async_events = ground.subscribe_async()
            peer = ground.connect("127.0.0.1", 15140)
            session = ground.open_session(peer, "python-async-ground")
            target = TargetSelector.entity(AgentType.UAV, 1)
            ground.request_authority(
                peer, session, target, ControlSource.GROUND_STATION, 3000, False
            )

            deadline = time.time() + 3.0
            lease = None
            while time.time() < deadline:
                lease = air.current_authority()
                if lease and lease.session_id == session.session_id:
                    break
                await asyncio.sleep(0.02)
            self.assertIsNotNone(lease)
            ground.renew_authority(
                peer, session, target, ControlSource.GROUND_STATION, 4500
            )

            air.publish_vehicle_core_state(
                lease.peer,
                TargetSelector.entity(AgentType.GROUND_STATION, 7),
                VehicleCoreState(True, 3, 1.0, 2.0, 3.0, 0.1, 0.2, 0.3, 88.0),
                session.session_id,
            )

            while True:
                event = await asyncio.wait_for(async_events.get(), timeout=3.0)
                if (
                    isinstance(event, VehicleCoreStateEvent)
                    and event.session_id == session.session_id
                    and abs(event.battery_percent - 88.0) < 1e-6
                ):
                    break
        finally:
            ground.close()
            air.close()


if __name__ == "__main__":
    unittest.main()
