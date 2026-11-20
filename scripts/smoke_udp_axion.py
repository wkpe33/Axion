"""Exercise UDP probing and source-port selection against a real loopback server."""
import argparse
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import tempfile
import time
import uuid

from smoke_axion import content, open_pipe, send, wait_for


def reserve_port():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    return sock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    args = parser.parse_args()
    build = args.build_dir.resolve(strict=True)
    report = build / "udp-smoke"
    report.mkdir(exist_ok=True)
    name = "axion-udp-" + uuid.uuid4().hex

    with tempfile.TemporaryDirectory(prefix="axion-udp-") as directory, reserve_port() as busy:
        root = Path(directory)
        client_dir = root / "client"
        server_dir = root / "server"
        for folder in (client_dir, server_dir):
            folder.mkdir()
            (folder / "storage.cfg").write_text("add_path $CURRENTDIR\nadd_path $DATADIR\n", encoding="utf-8")
        server_log = server_dir / "server.log"
        client_log = client_dir / "client.log"
        reload_log = client_dir / "reload.log"
        with reserve_port() as destination, reserve_port() as source, reserve_port() as second:
            remote = destination.getsockname()[1]
            local = source.getsockname()[1]
            alternative = second.getsockname()[1]
        occupied = busy.getsockname()[1]
        endpoint = f"127.0.0.1:{remote}"
        server = subprocess.Popen(
            [str(build / "DDNet-Server.exe"), f"logfile {server_log.as_posix()}",
             "bindaddr 127.0.0.1", f"sv_port {remote}", "sv_register 0", "sv_map Tutorial",
             "sv_name AxionUdpSmoke", f"sv_input_fifo {name}-server"],
            cwd=server_dir, creationflags=subprocess.CREATE_NO_WINDOW,
        )
        client = None
        server_pipe = client_pipe = None
        try:
            wait_for(lambda: "server name is 'AxionUdpSmoke'" in content(server_log), "Server did not start")
            server_pipe = wait_for(lambda: open_pipe(name + "-server"), "Server pipe did not open")
            client = subprocess.Popen(
                [str(build / "DDNet.exe"), f"logfile {client_log.as_posix()}",
                 "gfx_backend Vulkan", "gfx_rust_vulkan 1", "gfx_fullscreen 0",
                 "cl_show_welcome 0", f"cl_input_fifo {name}-client"],
                cwd=client_dir,
            )
            client_pipe = wait_for(lambda: open_pipe(name + "-client"), "Client pipe did not open")
            send(client_pipe, "axion_settings; bind f12 axion_settings")
            send(client_pipe, f"axion_udp_test {endpoint} 65536")
            wait_for(lambda: "invalid endpoint or port list" in content(client_log), "Invalid port was not rejected")
            send(client_pipe, f"axion_udp_test {endpoint} {local}; axion_udp_cancel")
            send(client_pipe, f"axion_udp_test {endpoint} {local},{alternative},{occupied}")
            wait_for(lambda: f"port={alternative} replies=6/6" in content(client_log), "No full replies on alternative port")
            wait_for(lambda: f"port={local} replies=6/6" in content(client_log), "No full replies on primary port")
            wait_for(lambda: re.search(rf"port={occupied} replies=0/0.*bind_failed=1", content(client_log)),
                     "Occupied port was not reported as a bind failure")
            send(client_pipe, f"axion_udp_auto 1; axion_udp_test {endpoint} {local},{occupied}")
            wait_for(lambda: content(client_log).count(f"port={local} replies=6/6") == 2, "Automatic test did not finish")
            send(client_pipe, f"connect {endpoint}")
            wait_for(lambda: f"main connection bound to configured source port {local}" in content(client_log),
                     "Automatic preference did not bind")
            wait_for(lambda: f"addr=<{{127.0.0.1:{local}}}>" in content(server_log), "Server did not observe the selected source port")
            send(client_pipe, "dummy_connect")
            wait_for(lambda: content(server_log).count("player has entered the game") >= 2, "Dummy did not join")
            for mode, setting in enumerate(("best", "cloud", "saiko", "delta"), start=1):
                send(client_pipe, f"ax_{setting}_input_amount 125; axion_input_mode {mode}")
                time.sleep(0.5)
                if client.poll() is not None:
                    raise RuntimeError(f"Client exited while connected in {setting} mode")
            send(client_pipe, f"axion_udp_prefer {endpoint} {alternative}; quit")
            if client.wait(timeout=30) != 0:
                raise RuntimeError("Client shutdown failed")
            saved = content(client_dir / "settings_ddnet.cfg")
            expected_settings = (
                f"axion_udp_prefer {endpoint} {alternative}", 'bind f12 "axion_settings"',
                "ui_settings_page 13", "axion_input_mode 4",
                *(f"ax_{mode}_input_amount 125" for mode in ("best", "cloud", "saiko", "delta")),
            )
            missing = [setting for setting in expected_settings if setting not in saved]
            if missing:
                raise RuntimeError(f"Missing saved settings: {missing}")
            frames = client_dir / "reload-frames.txt"
            subprocess.run(
                [str(build / "DDNet.exe"), f"logfile {reload_log.as_posix()}", f"connect {endpoint}",
                 f"benchmark_quit 5 {frames.as_posix()}"],
                cwd=client_dir, timeout=45, check=True,
            )
            if f"main connection bound to configured source port {alternative}" not in content(reload_log):
                raise RuntimeError("Saved source port was not applied on restart")
            if f"addr=<{{127.0.0.1:{alternative}}}>" not in content(server_log):
                raise RuntimeError("Server did not observe the manually selected source port")
            send(server_pipe, "shutdown")
            if server.wait(timeout=30) != 0:
                raise RuntimeError("Server shutdown failed")
            print(f"Passed UDP probing, bind failure, cancellation, auto/manual selection and persistence ({local}, {alternative}).")
        finally:
            for pipe in (client_pipe, server_pipe):
                if pipe is not None:
                    os.close(pipe)
            for child in (client, server):
                if child is not None and child.poll() is None:
                    child.terminate()
                    child.wait(timeout=10)
            for log in (client_log, reload_log, server_log):
                if log.exists():
                    shutil.copyfile(log, report / log.name)


if __name__ == "__main__":
    main()
