# kbuf

A simple Linux character device driver that exposes a 1 KiB in-kernel buffer
through `/dev/kbuf`. Good for learning the basics of character drivers.

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
sudo insmod kbuf.ko
echo "hello" | sudo tee /dev/kbuf
sudo cat /dev/kbuf
sudo rmmod kbuf
```

## Clean

```sh
make clean        # cross-compile
make clean_host   # host
```

## License

GPL-2.0