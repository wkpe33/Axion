# UDP source-port testing

Select a numeric DDNet 0.6 endpoint and open **Test UDP** next to Refresh
and Connect. The popup shows the server address, an editable custom port,
and separate rows for Auto, 40000, 45000, 50000, and Custom. Auto uses an
OS-assigned source port. A row's **Test** button measures that source port.
**Test All** measures the listed ports and any valid custom port.

Click **Select** next to a successful result to save its source port for the next main
connection. Choosing the Auto result clears the saved override. A custom
port may be any value from 1 to 65535 that the operating system allows
the client to bind.

**Auto port** samples 64 high local ports against the selected server.
It confirms up to three responsive candidates with six probes each.
Click **Select port** next to the confirmed candidate to save it. **Cancel** releases probe sockets.

Axion sends at most one probe every 550 ms and allows 500 ms for a reply.
Each reply must match the destination endpoint and request token. RTT
includes client polling delay and server handling time. Gameplay latency
can differ. A background worker owns the probe sockets and publishes progress to the browser. The Cancel button requests cancellation immediately. The saved port
takes effect on the next main connection and leaves the destination and
dummy connection ports alone.

## Reproduce the loopback check

```powershell
build-next/testrunner.exe --gtest_filter=UdpPortTest.*
```

The tests use a loopback UDP responder and verify background probing,
cancellation, and starting another test after cancellation.
