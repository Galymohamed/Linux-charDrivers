# linux-charDrivers

A collection of simple Linux character device drivers.

## Drivers

- [`kbuf/`](./kbuf) — Single-device character driver backed by a 1 KiB kernel buffer.
- [`kbuf_multiDev/`](./kbuf_multiDev) — Multi-device character driver managing 4 buffers with per-device size and permissions (RDONLY / WRONLY / RDWR).

## License

GPL-2.0
