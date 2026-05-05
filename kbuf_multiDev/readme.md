# multi_kbuf

A simple Linux multi-device character driver that exposes 4 independent
in-kernel buffers as `/dev/multiKbuf-1` … `/dev/multiKbuf-4`, each with
its own size and access permissions (RDONLY / WRONLY / RDWR).

## Build

Cross-compile (edit `KERNEL_DIR` in the Makefile first):

```sh
make
```

Build against the host kernel:

```sh
make all_host
```

## Usage

```sh
sudo insmod multi_kbuf.ko
ls -l /dev/multiKbuf-*
sudo cat /dev/multiKbuf-1                    # RDONLY  (1024 B)
echo "hi" | sudo tee /dev/multiKbuf-2        # WRONLY  ( 512 B)
echo "hi" | sudo tee /dev/multiKbuf-3        # RDWR    (1024 B)
sudo cat /dev/multiKbuf-3
sudo rmmod multi_kbuf
```

## Clean

```sh
make clean        # cross-compile
make clean_host   # host
```

## License

GPL-2.0
