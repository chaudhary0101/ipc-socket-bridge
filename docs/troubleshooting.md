# Troubleshooting

## Bridge Fails To Start

Check that the configured TCP port is free:

```bash
ss -ltnp | grep 9090
```

If the UNIX socket path already exists from an abnormal shutdown, the bridge removes it on startup. If the directory does not exist, create it or update `unix_socket_path`.

## Clients Connect But Receive Nothing

Start the TCP receiver before sending IPC messages. The current implementation drops messages when no destination client is writable and records an error metric. This mirrors a simple live-routing gateway; it is not a persistent broker.

## FIFO Permissions

The FIFO is created with mode `0660`. Ensure the producing process runs as the same user or group. For system deployment, create a dedicated runtime directory and group.

## SIGPIPE Crashes

`SignalHandler` ignores `SIGPIPE`, and send calls use `MSG_NOSIGNAL`. If a custom client crashes, verify it also handles broken pipes when writing to sockets.

## Valgrind Reports System Library Allocations

Focus on definitely-lost bytes from bridge-owned code. Some runtime and Google Test allocations may appear as still reachable. Use `--leak-check=full --show-leak-kinds=all` for detailed attribution.
