#!/usr/bin/env python3
"""
Flow_Fidget desktop bridge.

This script reads serial messages from the Arduino Uno and converts
`SCROLL:<signedSpeed>:<speed>` events into real mouse-wheel input for the
currently focused window.

Examples:
  python3 Test.py --list-ports
  python3 Test.py --port /dev/tty.usbmodem1101
  python3 Test.py --port COM3 --invert-y --scale 1.5
"""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
    from serial import SerialException
    from serial.tools import list_ports
except ImportError:
    serial = None
    SerialException = None
    list_ports = None

try:
    from pynput.mouse import Controller
except ImportError:
    Controller = None


READY_BANNER = "FLOW_FIDGET_READY"
MODE_PREFIX = "MODE:"
SCROLL_PREFIX = "SCROLL:"
DEFAULT_BAUD = 9600
DEFAULT_RECONNECT_DELAY = 2.0

SERIAL_ERROR_TYPES = (OSError,) if SerialException is None else (SerialException, OSError)


def parse_scroll_line(line: str):
    line = line.strip()
    if not line.startswith(SCROLL_PREFIX):
        return None

    parts = line.split(":")
    if len(parts) != 3:
        return None

    try:
        signed_speed = int(parts[1])
        speed = int(parts[2])
    except ValueError:
        return None

    if speed < 0:
        return None

    return signed_speed, speed


def compute_scroll_delta(signed_speed: int, scale: float, invert_y: bool) -> int:
    delta = signed_speed * scale
    if invert_y:
        delta *= -1

    rounded = int(round(delta))
    if rounded == 0 and signed_speed != 0 and scale > 0:
        return 1 if delta > 0 else -1

    return rounded


def require_serial_support() -> None:
    if serial is None or list_ports is None:
        raise RuntimeError(
            "pyserial is required. Install it with: python3 -m pip install pyserial"
        )


def require_mouse_support() -> None:
    if Controller is None:
        raise RuntimeError(
            "pynput is required. Install it with: python3 -m pip install pynput"
        )


def list_available_ports() -> int:
    require_serial_support()

    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 0

    for port in ports:
        print(f"{port.device}\t{port.description}")

    return 0


def handle_serial_line(line: str, mouse, scale: float, invert_y: bool) -> None:
    if not line:
        return

    if line == READY_BANNER:
        print("Device ready.")
        return

    if line.startswith(MODE_PREFIX):
        mode = line[len(MODE_PREFIX):].strip() or "UNKNOWN"
        print(f"Device mode: {mode}")
        return

    parsed = parse_scroll_line(line)
    if parsed is None:
        return

    signed_speed, _speed = parsed
    delta = compute_scroll_delta(signed_speed, scale, invert_y)
    if delta != 0:
        mouse.scroll(0, delta)


def open_serial_connection(port: str, baud: int):
    return serial.Serial(port, baudrate=baud, timeout=1)


def run_bridge(port: str, baud: int, scale: float, invert_y: bool) -> int:
    require_serial_support()
    require_mouse_support()

    mouse = Controller()

    while True:
        try:
            print(f"Connecting to {port} at {baud} baud...")
            with open_serial_connection(port, baud) as connection:
                print(f"Connected to {port}")

                while True:
                    raw_line = connection.readline()
                    if not raw_line:
                        continue

                    line = raw_line.decode("utf-8", errors="replace").strip()
                    handle_serial_line(line, mouse, scale, invert_y)

        except KeyboardInterrupt:
            print("Stopping bridge.")
            return 0
        except SERIAL_ERROR_TYPES as exc:
            print(f"Serial connection lost: {exc}")
            print(f"Retrying in {DEFAULT_RECONNECT_DELAY:.1f}s...")
            time.sleep(DEFAULT_RECONNECT_DELAY)


def positive_float(value: str) -> float:
    try:
        parsed = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid float value: {value}") from exc

    if parsed <= 0:
        raise argparse.ArgumentTypeError("value must be greater than 0")

    return parsed


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read Flow_Fidget serial events and inject OS scroll input."
    )
    parser.add_argument(
        "--port",
        help="Serial port for the Arduino, for example /dev/tty.usbmodem1101 or COM3.",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help="Serial baud rate. Default: 9600.",
    )
    parser.add_argument(
        "--invert-y",
        action="store_true",
        help="Invert the Y-axis scroll direction.",
    )
    parser.add_argument(
        "--scale",
        type=positive_float,
        default=1.0,
        help="Multiplier applied to each scroll delta.",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List available serial ports and exit.",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.list_ports:
        return list_available_ports()

    if not args.port:
        parser.error("--port is required unless --list-ports is used")

    try:
        return run_bridge(
            port=args.port,
            baud=args.baud,
            scale=args.scale,
            invert_y=args.invert_y,
        )
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
