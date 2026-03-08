from __future__ import annotations

import importlib.util
import pathlib
import unittest


PROJECT_DIR = pathlib.Path(__file__).resolve().parents[1]
BRIDGE_PATH = PROJECT_DIR / "Test.py"

spec = importlib.util.spec_from_file_location("flow_fidget_bridge", BRIDGE_PATH)
bridge = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(bridge)


class FakeMouse:
    def __init__(self) -> None:
        self.events = []

    def scroll(self, dx: int, dy: int) -> None:
        self.events.append((dx, dy))


class BridgeProtocolTests(unittest.TestCase):
    def test_parse_scroll_line_matches_firmware_format(self) -> None:
        self.assertEqual(bridge.parse_scroll_line("SCROLL:4:4"), (4, 4))
        self.assertEqual(bridge.parse_scroll_line("SCROLL:-2:2"), (-2, 2))

    def test_parse_scroll_line_rejects_non_scroll_messages(self) -> None:
        self.assertIsNone(bridge.parse_scroll_line("FLOW_FIDGET_READY"))
        self.assertIsNone(bridge.parse_scroll_line("MODE:SCROLL"))
        self.assertIsNone(bridge.parse_scroll_line("System: Scroll Mode"))
        self.assertIsNone(bridge.parse_scroll_line("SCROLL:not-a-number:2"))

    def test_handle_serial_line_ignores_non_scroll_lines(self) -> None:
        mouse = FakeMouse()

        bridge.handle_serial_line("FLOW_FIDGET_READY", mouse, 1.0, False)
        bridge.handle_serial_line("MODE:SCROLL", mouse, 1.0, False)
        bridge.handle_serial_line("System: Scroll Mode", mouse, 1.0, False)

        self.assertEqual(mouse.events, [])

    def test_handle_serial_line_emits_scroll_for_firmware_event(self) -> None:
        mouse = FakeMouse()

        bridge.handle_serial_line("SCROLL:3:3", mouse, 1.0, False)
        bridge.handle_serial_line("SCROLL:-2:2", mouse, 1.0, False)

        self.assertEqual(mouse.events, [(0, 3), (0, -2)])

    def test_invert_y_flips_direction(self) -> None:
        mouse = FakeMouse()

        bridge.handle_serial_line("SCROLL:5:5", mouse, 1.0, True)

        self.assertEqual(mouse.events, [(0, -5)])


if __name__ == "__main__":
    unittest.main()
