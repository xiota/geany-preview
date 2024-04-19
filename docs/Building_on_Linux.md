# Building on Linux

The Preview plugin for Geany can be built and installed on Linux-based operating systems.

* It cannot be used on Windows because WebKit2GTK is unavailable.
* It is untested on Mac, but should work if the dependencies are available.

## Building

On Debian and Ubuntu, the following command should install the dependencies:

```bash
  sudo apt install build-essential meson geany libgtk-3-dev \
    libwebkit2gtk-4.1-dev libcmark-gfm-dev libcmark-gfm-extensions-dev \
    libpodofo-dev
```

Then run the following in a terminal from the source directory:

```bash
  make
```

The makefile is a front-end to `meson`.  If you prefer, you may use `meson` directly:

```bash
  meson setup build
  meson compile -C build
```

If `meson compile` is not available, you may need to use `ninja` instead.

```bash
ninja -C build
```

## Installing

The makefile contains an install target.  To install to a specific location, set `DESTDIR`.  Otherwise, the default location is `/usr/lib/geany/`.  

```bash
DESTDIR=/path/to/install/files make install
```

Depending on distro, symlinks may be needed for geany to find the plugin.

```bash
  ln -s /usr/lib/geany/preview.so /usr/lib/x86_64-linux-gnu/geany/preview.so

```

To find the plugin folder, locate an existing plugin.

```bash
  find /usr -name saveactions.so
```

Sometimes running `sudo ldconfig` can help resolve issues with shared object file locations.

## Uninstalling

To uninstall, delete the files and folders listed in `build/meson-logs/install-log.txt`.

Running `make uninstall` might work.

Don't forget to remove manually created symlinks.
