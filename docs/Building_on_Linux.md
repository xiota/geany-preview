# Building and Installing

The Preview plugin for Geany can be built and installed on Linux-based operating systems.

* It cannot currently be used on Windows because WebKit2GTK is unavailable.
* I do not have a Mac to try, but would expect it to work if the dependencies can be obtained.

## Building

On Debian/Ubuntu, the following command will install the build dependencies:

```
  sudo apt-get install build-essential autoconf geany libgtk-3-dev \
    libwebkit2gtk-4.0-dev libcmark-gfm-dev libcmark-gfm-extensions-dev
```

Then to build, run the following in a terminal from the source directory:

```
  ./autogen.sh
  cd build-aux
  make
```

## Installing

To install, run `make install`.  To uninstall, run `make uninstall`.

To view formats other than HTML or Markdown, install some auxiliary programs:

```
  sudo apt-get install pandoc asciidoctor
```

To view screenplays written in Fountain, install `screenplain`.  Normally `pip` may be used.  However, the version currently available through `pip` does not work with stdio, which this plugin uses to get output from external programs.  The version in the git repository has been fixed.  To install it, run the following commands:

```
  git clone https://github.com/vilcans/screenplain.git
  cd screenplain
  ./setup.py  bdist_wheel
  pip install dist/screenplain-*.whl
```

To uninstall, use `pip uninstall screenplain`
