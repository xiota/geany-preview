# Preview Plugin for Geany

This plugin provides a basic preview of documents written in several markup formats.

![screenshot](screenshot.png)

## Features

* Updates the preview as the document is edited.
* Supports multiple document formats:
  + AsciiDoc
  + Markdown (GFM)
  + DocBook
  + HTML
  + LaTeX
  + reStructuredText
  + Textile
  + Txt2Tags
  + Wiki
    - DokuWiki
    - MediaWiki (Wikipedia)
    - Tiki Wiki
    - TWiki

## Usage

The preview is active by default for documents with a supported filetype.  Updates are delayed when the preview is not visible.  To disable the preview completely without unloading the plugin, change the filetype to one that is not supported, like a programming language.  The filetype will be automatically redetected as the correct type the next time the document is opened.

## Requirements

This plugin depends on the following libraries and programs:

* [Geany](https://geany.org/)
* [GTK+](http://www.gtk.org)
* [WebKit2GTK](http://webkitgtk.org)
* [Pandoc](https://pandoc.org/)
* [libcmark-gfm](https://github.com/github/cmark-gfm)
* [Asciidoctor](https://asciidoctor.org/)

## Building and Installing

On Debian/Ubuntu, the following command will install the dependencies:

```
  sudo apt install geany libgtk-3-dev libwebkit2gtk-4.0-dev \
    libcmark-gfm-dev libcmark-gfm-extensions-dev pandoc asciidoctor
```

Then to build, run the following in a terminal from the source directory:

```
  ./autogen.sh
  cd build-aux
  ../configure
  make
```

To install, run `make install`.  The plugin will be copied to `/usr/local/lib/geany/`.  To uninstall, run `make uninstall` or delete the files `preview.*`.

## License

The Preview Geany plugin is licensed under the [GPLv2](COPYING) or later.  It uses code from the [Code Format](https://github.com/codebrainz/code-format/) and [Markdown](https://plugins.geany.org/markdown.html) plugins written by [Michael Brush](https://github.com/codebrainz).
