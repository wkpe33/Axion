import argparse
import os
import re
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time
import uuid


def wait_for(check, message, timeout=35):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        value = check()
        if value:
            return value
        time.sleep(0.2)
    raise RuntimeError(message)

def content(path):
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""

def open_pipe(name):
    try:
        return os.open("\\\\.\\pipe\\" + name, os.O_RDWR | os.O_BINARY)
    except OSError:
        return None

def send(pipe, command):
    os.write(pipe, command.encode("utf-8") + b"\n")

def main():
    parser = argparse.ArgumentParser(description="Smoke-test Axion on a loopback DDNet server.")
    parser.add_argument("build_dir", type=Path)
    args = parser.parse_args()
    build = args.build_dir.resolve(strict=True)
    report = build / "smoke"
    report.mkdir(exist_ok=True)
    name = "axion-smoke-" + uuid.uuid4().hex
    with tempfile.TemporaryDirectory(prefix="axion-smoke-") as directory:
        root = Path(directory)
        server_dir = root / "server"
        client_dir = root / "client"
        for work in (server_dir, client_dir):
            work.mkdir()
            (work / "storage.cfg").write_text("add_path $CURRENTDIR\nadd_path $DATADIR\n", encoding="utf-8")
        server_log = server_dir / "server.log"
        client_log = client_dir / "client.log"
        reload_log = client_dir / "reload.log"
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        server = subprocess.Popen(
            [str(build / "DDNet-Server.exe"), f"logfile {server_log.as_posix()}",
             "bindaddr 127.0.0.1", f"sv_port {port}", "sv_register 0",
             "sv_map Tutorial", "sv_name AxionSmoke", f"sv_input_fifo {name}-server"],
            cwd=server_dir, creationflags=subprocess.CREATE_NO_WINDOW,
        )
        client = None
        client_pipe = None
        server_pipe = None
        try:
            wait_for(lambda: "server name is 'AxionSmoke'" in content(server_log), "Server did not start")
            server_pipe = wait_for(lambda: open_pipe(name + "-server"), "Server pipe did not open")
            client = subprocess.Popen(
                [str(build / "DDNet.exe"), f"logfile {client_log.as_posix()}",
                 "gfx_backend Vulkan", "gfx_rust_vulkan 1", "gfx_fullscreen 0",
                 "gfx_screen_width 1280", "gfx_screen_height 720", "cl_show_welcome 0",
                 "cl_menu_map jungle", "cl_performance_hud 1", "dbg_gfx 3", "loglevel 1",
                 f"cl_input_fifo {name}-client", "player_name AxionSmoke", "dummy_name AxionDummy"],
                cwd=client_dir,
            )
            client_pipe = wait_for(lambda: open_pipe(name + "-client"), "Client pipe did not open")
            wait_for(lambda: "Using Rust Vulkan presentation." in content(client_log), "Rust path did not activate")
            time.sleep(2)
            send(client_pipe, "performance_status")
            metrics = wait_for(
                lambda: re.search(r"CPU render ([\d.]+) ms +GPU draw ([\d.]+) ms +RAM ([\d.]+) MiB",
                                  content(client_log)),
                "CPU, GPU and RAM diagnostics were not printed",
            )
            if not all(float(value) > 0 for value in metrics.groups()):
                raise RuntimeError(f"Expected positive measurements: {metrics.group(0)}")
            print(metrics.group(0))
            recreates = content(client_log).count("Recreating swap chain.")
            send(client_pipe, "gfx_screen_width 1024; gfx_screen_height 768")
            wait_for(lambda: content(client_log).count("Recreating swap chain.") > recreates, "No Vulkan swapchain recreation")
            time.sleep(2)
            send(client_pipe, f"connect 127.0.0.1:{port}")
            wait_for(lambda: content(server_log).count("player has entered the game") >= 1, "Client failed to join")
            send(client_pipe, "dummy_connect")
            wait_for(lambda: content(server_log).count("player has entered the game") >= 2, "Dummy failed to join")
            time.sleep(2)
            send(client_pipe, "quit")
            if client.wait(timeout=30) != 0:
                raise RuntimeError("Client did not exit cleanly")
            send(server_pipe, "shutdown")
            if server.wait(timeout=30) != 0:
                raise RuntimeError("Server did not exit cleanly")
            frames = client_dir / "reload-frames.txt"
            subprocess.run(
                [str(build / "DDNet.exe"), f"logfile {reload_log.as_posix()}",
                 f"benchmark_quit 5 {frames.as_posix()}"],
                cwd=client_dir, timeout=40, check=True,
            )
            if "Using Rust Vulkan presentation." not in content(reload_log) or not frames.exists():
                raise RuntimeError("Saved Vulkan settings were not loaded after restart")
            print("Passed Vulkan startup, timing diagnostics, resize, map load, dummy, settings reload and clean shutdown")
        finally:
            for pipe in (client_pipe, server_pipe):
                if pipe is not None:
                    os.close(pipe)
            for process in (client, server):
                if process is not None and process.poll() is None:
                    process.terminate()
                    process.wait(timeout=10)
            for path in (client_log, reload_log, server_log):
                if path.exists():
                    shutil.copyfile(path, report / path.name)

if __name__ == "__main__":
    main()

