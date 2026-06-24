# Performance

The bridge is designed for small to medium control-plane messages such as automotive HVAC state, infotainment commands, diagnostics events, and telecom service notifications.

## I/O Model

Both IPC and TCP sides use non-blocking file descriptors with `epoll`. This avoids blocking a worker thread on one slow client and keeps descriptor readiness handling predictable under multiple connections.

## Queue Backpressure

`ThreadSafeQueue` is bounded by `queue_capacity`. Producers wait when the queue is full, which prevents unbounded memory growth during bursts. This is a practical embedded Linux tradeoff: bounded latency is preferable to uncontrolled heap expansion.

## Payload Limits

`max_payload_bytes` rejects oversized frames early. The default of 64 KiB is intentionally conservative for control messages. Large bulk data should be transferred with a different channel and referenced by metadata.

## Optimization Targets

- Reduce payload copies with scatter/gather writes if profiling shows serialization overhead.
- Add per-client output buffers if slow TCP clients should not drop frames.
- Pin bridge threads only after measuring scheduler behavior on the target hardware.
- Use `perf`, `strace`, and Valgrind Massif to validate CPU, syscall, and memory behavior.

## Practical Benchmark Scenario

Run the bridge, start one TCP receiver, and send 10,000 IPC messages through `ipc_client_sender` in a loop. Compare metrics in `bridge.log` before and after the run. The most important counters are `messages`, `bytes`, and `errors`.
