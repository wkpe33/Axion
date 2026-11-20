"""Isolated loopback session for visual/manual Axion release verification."""
import json, os, socket, subprocess, time, uuid
from pathlib import Path
from smoke_axion import wait_for, content, open_pipe, send
build=Path(__file__).resolve().parents[1]/"build-next"
root=build/("theme-check-"+uuid.uuid4().hex[:8]);root.mkdir()
server_dir=root/"server";client_dir=root/"client"
for folder in (server_dir,client_dir):
 folder.mkdir();(folder/"storage.cfg").write_text("add_path $CURRENTDIR\nadd_path $DATADIR\n",encoding="utf-8")
with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sock:
 sock.bind(("127.0.0.1",0));port=sock.getsockname()[1]
name="ax-theme-"+uuid.uuid4().hex[:8]
server_log=server_dir/"server.log";client_log=client_dir/"client.log"
server=subprocess.Popen([str(build/"DDNet-Server.exe"),f"logfile {server_log.as_posix()}","bindaddr 127.0.0.1",f"sv_port {port}","sv_register 0","sv_map Tutorial","sv_name AxionThemeCheck","sv_invite_frequency 0","sv_team_change_delay 0",f"sv_input_fifo {name}-server"],cwd=server_dir,creationflags=subprocess.CREATE_NO_WINDOW,stdout=open(server_dir/"stdout.log","w"),stderr=subprocess.STDOUT)
client=None;sp=cp=None
try:
 wait_for(lambda:"server name is 'AxionThemeCheck'" in content(server_log),"Server didn't start")
 sp=wait_for(lambda:open_pipe(name+"-server"),"No server pipe")
 client=subprocess.Popen([str(build/"DDNet.exe"),f"logfile {client_log.as_posix()}","gfx_backend Vulkan","gfx_rust_vulkan 1","gfx_fullscreen 0","gfx_screen_width 1280","gfx_screen_height 720","gfx_vsync 0","cl_refresh_rate 240","cl_refresh_rate_inactive 60","cl_show_welcome 0","ui_mousesens 100","player_name AxionMain","dummy_name AxionDummy","dummy_skin bluekitty","axion_startup_profile 1","ax_music_player 1",f"cl_input_fifo {name}-client",f"connect 127.0.0.1:{port}"],cwd=client_dir,stdout=open(client_dir/"stdout.log","w"),stderr=subprocess.STDOUT)
 cp=wait_for(lambda:open_pipe(name+"-client"),"No client pipe")
 wait_for(lambda:"player has entered the game" in content(server_log),"Main didn't join")
 send(cp,"dummy_connect")
 wait_for(lambda:content(server_log).count("player has entered the game")>=2,"Dummy didn't join")
 meta={"root":str(root),"client":str(client_dir),"server":str(server_dir),"client_pipe":name+"-client","server_pipe":name+"-server","port":port,"pid":client.pid,"server_pid":server.pid}
 (build/"theme-check-current.json").write_text(json.dumps(meta,indent=2),encoding="utf-8")
 print(json.dumps(meta),flush=True)
 end=time.monotonic()+1800
 while time.monotonic()<end and not (root/"stop").exists() and client.poll() is None:
  time.sleep(.2)
 if client.poll() is None:send(cp,"quit")
 client.wait(timeout=20)
 send(sp,"shutdown");server.wait(timeout=20)
 print("Client exit",client.returncode,"server exit",server.returncode,flush=True)
finally:
 for pipe in (cp,sp):
  if pipe is not None:os.close(pipe)
 for process in (client,server):
  if process is not None and process.poll() is None:process.terminate();process.wait(timeout=10)
