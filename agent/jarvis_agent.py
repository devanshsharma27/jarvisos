#!/usr/bin/env python3
"""
JarvisOS LLM agent
==================

Talks to a running JarvisOS over its serial port and lets you drive the kernel
in plain English:

    $ python3 agent/jarvis_agent.py
    you> how much memory does this machine have?
    [jarvis] meminfo
    Total reported : 131008 KB
    ...
    Claude: The VM has 127 MB of usable RAM, and 32,470 of its 32,736
    4 KB frames are still free.

The agent boots QEMU itself, connects to COM1, and gives Claude a single tool:
"run a command in the JarvisOS shell". Claude picks the commands, reads the
output, and explains it.

Without an API key it falls back to a small keyword matcher, so the demo still
works offline.

Requires: qemu-system-i386, and `pip install anthropic` for the LLM mode.
"""

import argparse
import os
import socket
import subprocess
import sys
import time

MODEL = os.environ.get("JARVIS_MODEL", "claude-opus-5")
PROMPT = "jarvis> "

SYSTEM_PROMPT = """\
You are the assistant built into JarvisOS, a small 32-bit x86 hobby operating
system running under QEMU. You control it through its serial console using the
run_shell_command tool.

Rules:
- Answer questions by running real commands and reading the real output. Never
  invent numbers.
- Run as many commands as you need, one per tool call.
- These commands are destructive or end the session: reboot, halt, fault. Only
  run them if the user clearly asked for that.
- Keep replies short. Give the answer first, then a sentence of context.

Here is the exact command list, printed by the running kernel:

{help_text}
"""

# Rules for --offline mode: first matching keyword set wins.
OFFLINE_RULES = [
    (("memory", "ram", "how much mem"), "meminfo"),
    (("memory map", "regions", "e820"), "memmap"),
    (("frame", "physical alloc"), "frames"),
    (("paging", "page table", "mmu"), "paging"),
    (("heap", "kmalloc", "malloc"), "heap"),
    (("task", "process", "scheduler", "running"), "ps"),
    (("uptime", "how long", "booted"), "uptime"),
    (("serial", "uart", "com1"), "serial"),
    (("version", "which version"), "version"),
    (("who", "what is this", "about"), "about"),
    (("help", "commands", "what can you do"), "help"),
]


class JarvisConsole:
    """A serial connection to a running JarvisOS."""

    def __init__(self, kernel="kernel.bin", memory="128M", port=4444, attach=False):
        self.proc = None

        if not attach:
            if not os.path.exists(kernel):
                sys.exit(f"error: {kernel} not found — run 'make' first")
            self.proc = subprocess.Popen(
                [
                    "qemu-system-i386",
                    "-kernel", kernel,
                    "-m", memory,
                    "-display", "none",
                    "-no-reboot",
                    "-serial", f"tcp:127.0.0.1:{port},server,nowait",
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            time.sleep(1.0)   # give QEMU a moment to open the listening socket

        self.sock = socket.create_connection(("127.0.0.1", port), timeout=10)
        self.sock.settimeout(0.5)
        self.banner = self._read_until_prompt(first=True)

    # ---- low-level I/O ---------------------------------------------

    def _read_until_prompt(self, first=False, limit=20.0):
        """Read until the shell prompt comes back, or we stop hearing anything."""
        out = ""
        deadline = time.time() + limit
        quiet_until = time.time() + (3.0 if first else 1.5)

        while time.time() < deadline:
            try:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                out += chunk.decode("utf-8", "replace")
                quiet_until = time.time() + 0.4
            except socket.timeout:
                if out.endswith(PROMPT) or time.time() > quiet_until:
                    break
        return out

    def run(self, command):
        """Send one command line, return whatever the kernel printed back."""
        self.sock.sendall(command.encode() + b"\r")
        raw = self._read_until_prompt()

        # Strip the echoed command and the trailing prompt.
        text = raw.replace("\r\n", "\n")
        if text.startswith(command):
            text = text[len(command):]
        if text.endswith(PROMPT):
            text = text[: -len(PROMPT)]
        return text.strip("\n") or "(no output)"

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass
        if self.proc:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()


# The tool below needs the live console; the CLI sets this once at startup.
CONSOLE = None


def ask_offline(question):
    """No API key? Match a keyword and run the obvious command."""
    q = question.lower()
    for keywords, command in OFFLINE_RULES:
        if any(k in q for k in keywords):
            print(f"[jarvis] {command}")
            print(CONSOLE.run(command))
            return
    print("offline mode: no rule matched. Try asking about memory, paging,")
    print("the heap, tasks, uptime, or the serial port.")


def ask_claude(client, run_shell_command, history, question):
    """One turn of conversation: Claude runs commands until it can answer."""
    history.append({"role": "user", "content": question})

    runner = client.beta.messages.tool_runner(
        model=MODEL,
        max_tokens=2048,
        system=SYSTEM_PROMPT.format(help_text=CONSOLE.run("help")),
        tools=[run_shell_command],
        messages=history,
    )

    for message in runner:
        for block in message.content:
            if block.type == "text" and block.text.strip():
                print(f"Claude: {block.text.strip()}")
            elif block.type == "tool_use":
                print(f"[jarvis] {block.input.get('command', '')}")
        history.append({"role": "assistant", "content": message.content})

        response = runner.generate_tool_call_response()
        if response is not None:
            history.append(response)


def build_client():
    """Return (client, tool) for LLM mode, or (None, None) to go offline."""
    if not (os.environ.get("ANTHROPIC_API_KEY") or os.environ.get("ANTHROPIC_AUTH_TOKEN")):
        print("No ANTHROPIC_API_KEY set — running in offline keyword mode.\n")
        return None, None

    try:
        from anthropic import Anthropic, beta_tool
    except ImportError:
        print("The 'anthropic' package is not installed (pip install anthropic).")
        print("Running in offline keyword mode.\n")
        return None, None

    @beta_tool
    def run_shell_command(command: str) -> str:
        """Run one command in the JarvisOS shell and return exactly what it printed.

        Args:
            command: A full command line, e.g. "meminfo", "kmalloc 128", "spawn worker".
        """
        return CONSOLE.run(command)   # the loop above echoes the command line

    return Anthropic(), run_shell_command


def main():
    global CONSOLE

    parser = argparse.ArgumentParser(description="Natural-language shell for JarvisOS")
    parser.add_argument("question", nargs="*", help="ask one question and exit")
    parser.add_argument("--kernel", default="kernel.bin", help="kernel image to boot")
    parser.add_argument("--memory", default="128M", help="RAM to give the VM")
    parser.add_argument("--port", type=int, default=4444, help="serial TCP port")
    parser.add_argument("--attach", action="store_true",
                        help="connect to a QEMU already listening on --port")
    parser.add_argument("--offline", action="store_true",
                        help="skip the LLM and use keyword matching")
    args = parser.parse_args()

    client, tool = (None, None) if args.offline else build_client()

    print(f"Booting JarvisOS ({args.kernel}, {args.memory})...")
    CONSOLE = JarvisConsole(args.kernel, args.memory, args.port, args.attach)

    version = CONSOLE.run("version")
    print(f"Connected to {version}")
    print("Ask a question, or type 'quit'.\n")

    history = []

    def handle(question):
        if client:
            ask_claude(client, tool, history, question)
        else:
            ask_offline(question)

    try:
        if args.question:
            handle(" ".join(args.question))
            return

        while True:
            try:
                question = input("you> ").strip()
            except EOFError:
                break
            if not question:
                continue
            if question.lower() in ("quit", "exit"):
                break
            handle(question)
            print()
    except KeyboardInterrupt:
        pass
    finally:
        print("\nShutting down the VM.")
        CONSOLE.close()


if __name__ == "__main__":
    main()
