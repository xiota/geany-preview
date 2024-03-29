# Building on Linux

The Preview plugin for Geany can be built and installed on Linux-based operating systems.

* It cannot currently be used on Windows because WebKit2GTK is unavailable.
* I do not have a Mac to try, but would expect it to work if the dependencies can be obtained.

## Building

On Debian/Ubuntu, the following command will install the build dependencies:

```
  sudo apt-get install build-essential autoconf geany libgtk-3-dev \
    libwebkit2gtk-4.0-dev libcmark-gfm-dev libcmark-gfm-extensions-dev \
    libpodofo-dev
```

Then to build, run the following in a terminal from the source directory:

```
  ./autogen.sh

  cd build-aux
  ../configure
  make
```

To change the install path, add `--prefix=/install/path` to `configure`.  The default location is `/usr/local`.

## Installing

To install, run `make install`.  To install to a different location, such as when building packages, use `make install DESTDIR="$pkgdir"`.  Files will be copied to `$pkgdir/install/path`.

If you are *not* also building Geany from source, you will need to make a symlink to the plugin for Geany to find.

```
  ln -s /usr/local/lib/geany/preview.so /usr/lib/x86_64-linux-gnu/geany/preview.so

```

To find the correct folder to make the link, locate an existing plugin.

```
  find /usr -name saveactions.so
```

Sometimes running `sudo ldconfig` can help resolve issues with shared object file locations.

## Uninstalling

To uninstall, run `make uninstall`.
