#!/usr/bin/env python3
"""Drive Aurora's host UI and retain real frames, object trees and assertions.

Architecture reference: GLinnik21/plx-native (docs/building.md, tools/sim-smoke.py).
This is an original Aurora adapter, not an LG firmware emulator.
"""
from __future__ import annotations
import argparse
import html
import json
import os
from pathlib import Path
import queue
import signal
import subprocess
import sys
import threading
import time
from PIL import Image, ImageStat


class Simulator:
    def __init__(self, binary: Path, output: Path):
        self.messages = queue.Queue()
        screen = os.environ.get("AURORA_UI_SIM_SIZE", "1920x1080")
        environment = dict(os.environ, HOME=str(output / "state"),
                           XDG_DATA_HOME=str(output / "state/data"),
                           XDG_CONFIG_HOME=str(output / "state/config"),
                           XDG_CACHE_DIR=str(output / "state/cache"))
        (output / "state").mkdir()
        self.process = subprocess.Popen(
            ["xvfb-run", "-a", "-s", f"-screen 0 {screen}x24 -nolisten tcp", str(binary), str(output)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", errors="replace", env=environment, start_new_session=True)
        self.reader = threading.Thread(target=self.read, args=(output / "process.log",), daemon=True)
        self.reader.start()
        try:
            if self.response(45) != "READY":
                raise RuntimeError("Simulator did not become ready")
        except Exception:
            if self.process.poll() is None:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait(timeout=5)
            self.reader.join(timeout=2)
            raise

    def read(self, log: Path):
        with log.open("w", encoding="utf-8") as stream:
            for line in self.process.stdout:
                stream.write(line)
                stream.flush()
                if line.startswith("AURORA_SIM\t"):
                    self.messages.put(line.strip().split("\t", 1)[1])
        self.messages.put(None)

    def response(self, timeout=15):
        try:
            result = self.messages.get(timeout=timeout)
        except queue.Empty:
            raise TimeoutError("UI command timed out; see process.log")
        if result is None:
            raise RuntimeError(f"Simulator exited unexpectedly (status {self.process.poll()}); see process.log")
        return result

    def command(self, command: str):
        if "\n" in command or "\r" in command or len(command.encode()) >= 4094:
            raise ValueError("Invalid or oversized UI command")
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()
        if self.response() != "OK":
            raise AssertionError(f"UI rejected: {command}")

    def close(self):
        if self.process.poll() is None:
            try:
                self.command("quit")
                self.process.wait(timeout=15)
            except Exception:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait(timeout=5)
        self.reader.join(timeout=2)
        if self.process.returncode != 0:
            raise RuntimeError(f"Simulator exited with {self.process.returncode}")


def verify_png(path: Path, expected_size=(1920, 1080)):
    with Image.open(path) as image:
        image.verify()
    with Image.open(path) as image:
        if image.size != expected_size:
            raise AssertionError(f"Wrong capture dimensions: {image.size}")
        if max(ImageStat.Stat(image.convert("RGB")).stddev) < 1:
            raise AssertionError("Capture is blank")


def write_report(output: Path, report: dict):
    (output / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    cards = []
    for step in report["steps"]:
        title = html.escape(step["command"])
        image = step.get("image")
        cards.append(f'<article><h2>{step["index"]}. {title}</h2>' +
                     (f'<a href="{image}"><img src="{image}"></a>' if image else "") +
                     f'<p>{html.escape(step["status"])}</p></article>')
    (output / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Aurora UI run</title>'
        '<style>body{background:#16191d;color:#eee;font:16px system-ui;max-width:1100px;margin:30px auto}'
        'article{padding:12px;margin:16px 0;background:#23272d}img{width:100%}h2{font-size:18px}</style>'
        '<h1>Aurora desktop UI simulation</h1><p>SDL/LVGL + dummy decoder; LG hardware is not emulated.</p>'
        f'<p>Status: {html.escape(report["status"])}</p>' +
        (f'<pre>{html.escape(report["error"])}</pre>' if "error" in report else "") + "".join(cards))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--scenario", type=Path, default=Path(__file__).with_name("smoke.json"))
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        parser.error("Output directory must be empty to avoid mixing runs")
    scenario = json.loads(args.scenario.read_text())
    report = {"status": "running", "scenario": scenario["name"], "hardware_emulated": False, "steps": []}
    sim = None
    try:
        sim = Simulator(args.binary, output)
        frame_size = tuple(map(int, os.environ.get("AURORA_UI_SIM_SIZE", "1920x1080").split("x")))
        for index, step in enumerate(scenario["steps"], 1):
            if len(step) != 1:
                raise ValueError("Each scenario step must have one command")
            action, argument = next(iter(step.items()))
            if action not in {"wait", "key", "click", "click_text", "text", "expect", "absent", "action", "wheel", "resize"}:
                raise ValueError(f"Unknown action: {action}")
            command = action + " " + str(argument)
            entry = {"index": index, "command": command, "status": "running"}
            report["steps"].append(entry)
            started = time.monotonic()
            try:
                sim.command(command)
                if action == "resize":
                    frame_size = tuple(map(int, str(argument).split()))
                entry["status"] = "passed"
            except Exception:
                entry["status"] = "failed"
                raise
            finally:
                entry["elapsed_ms"] = round((time.monotonic() - started) * 1000)
                if sim.process.poll() is None:
                    image = f"step-{index:03d}.png"
                    sim.command("shot " + image)
                    verify_png(output / image, frame_size)
                    entry["image"] = image
                    sim.command(f"tree step-{index:03d}.json")
        report["status"] = "passed"
    except Exception as error:
        report["status"] = "failed"
        report["error"] = str(error)
    finally:
        if sim:
            try:
                sim.close()
            except Exception as error:
                report["status"] = "failed"
                report["shutdown_error"] = str(error)
        write_report(output, report)
    print(f'{report["status"]}: {output / "index.html"}')
    if "error" in report:
        print(report["error"], file=sys.stderr)
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
