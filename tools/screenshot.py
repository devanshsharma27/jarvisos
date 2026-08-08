#!/usr/bin/env python3
"""
Capture JarvisOS screenshots for the README.

Boots the kernel in QEMU, types a scripted set of commands over the serial
port, then asks the QEMU monitor to screendump the real VGA framebuffer.
QEMU writes PPM, which we convert to PNG here (no third-party libraries).

    python3 tools/screenshot.py            # write docs/img/*.png
    python3 tools/screenshot.py boot heap  # only these scenes
"""

import os
import socket
import struct
import subprocess
import sys
import time
import zlib

OUT_DIR = "docs/img"
SERIAL_PORT = 45400
MONITOR_PORT = 45401

# Each scene: the commands to type, then a screendump of whatever is on screen.
# Keep each list short enough that the output fits the 80x25 text console.
SCENES = {
    "boot": ["banner"],
    "memory": ["clear", "meminfo", "frames"],
    "paging": ["clear", "paging", "map 0xD0000000", "virt 0xD0000000"],
    "heap": ["clear", "kmalloc 128", "kmalloc 4096", "heap"],
    "tasks": ["clear", "spawn alpha", "spawn beta", "sleep 800", "ps", "uptime"],
    "help": ["clear", "help"],
}


def write_png(path, width, height, rgb):
    """Minimal 8-bit truecolour PNG writer."""
    rows = b"".join(
        b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height)
    )

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(rows, 9))
    png += chunk(b"IEND", b"")

    with open(path, "wb") as f:
        f.write(png)


def ppm_to_png(ppm_path, png_path):
    with open(ppm_path, "rb") as f:
        data = f.read()

    # Header: "P6\n<width> <height>\n255\n" (QEMU writes no comments).
    fields, offset = [], 0
    while len(fields) < 4:
        end = data.index(b"\n", offset)
        fields += data[offset:end].split()
        offset = end + 1

    if fields[0] != b"P6":
        raise ValueError(f"{ppm_path}: not a binary PPM")

    width, height = int(fields[1]), int(fields[2])
    write_png(png_path, width, height, data[offset:offset + width * height * 3])
    return width, height


class Machine:
    """QEMU with a serial console and a monitor socket."""

    def __init__(self):
        self.proc = subprocess.Popen(
            [
                "qemu-system-i386",
                "-kernel", "kernel.bin",
                "-m", "128M",
                "-display", "none",
                "-no-reboot",
                "-serial", f"tcp:127.0.0.1:{SERIAL_PORT},server,nowait",
                "-monitor", f"tcp:127.0.0.1:{MONITOR_PORT},server,nowait",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        time.sleep(1.5)

        self.serial = socket.create_connection(("127.0.0.1", SERIAL_PORT), timeout=10)
        self.monitor = socket.create_connection(("127.0.0.1", MONITOR_PORT), timeout=10)
        self.serial.settimeout(0.5)
        self.monitor.settimeout(0.5)
        time.sleep(2.0)          # let the kernel finish booting

    def type(self, command):
        self.serial.sendall(command.encode() + b"\r")
        # Commands like "sleep 800" take a while; drain until the line goes quiet.
        deadline = time.time() + 4.0
        while time.time() < deadline:
            try:
                if not self.serial.recv(4096):
                    break
            except socket.timeout:
                break

    def screendump(self, ppm_path):
        self.monitor.sendall(f"screendump {ppm_path}\n".encode())
        for _ in range(40):
            time.sleep(0.25)
            if os.path.exists(ppm_path) and os.path.getsize(ppm_path) > 0:
                time.sleep(0.25)
                return
        raise RuntimeError("QEMU never wrote the screendump")

    def close(self):
        for sock in (self.serial, self.monitor):
            try:
                sock.close()
            except OSError:
                pass
        self.proc.terminate()
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def capture(name, commands):
    machine = Machine()
    ppm = os.path.abspath(os.path.join(OUT_DIR, f"{name}.ppm"))
    png = os.path.join(OUT_DIR, f"{name}.png")

    try:
        for command in commands:
            machine.type(command)
        time.sleep(0.5)
        machine.screendump(ppm)
    finally:
        machine.close()

    width, height = ppm_to_png(ppm, png)
    os.remove(ppm)
    print(f"  {png}  ({width}x{height})")


def main():
    if not os.path.exists("kernel.bin"):
        sys.exit("error: kernel.bin not found — run 'make' first")

    os.makedirs(OUT_DIR, exist_ok=True)
    wanted = sys.argv[1:] or list(SCENES)

    for name in wanted:
        if name not in SCENES:
            sys.exit(f"unknown scene {name!r}; choose from {', '.join(SCENES)}")
        print(f"capturing {name}...")
        capture(name, SCENES[name])


if __name__ == "__main__":
    main()
