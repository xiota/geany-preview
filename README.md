# Preview Plugin for Geany

This plugin provides a basic preview of documents written in several markup formats.

![screenshot](screenshot.png)

## Features

* Updates the preview as the document is edited.
* Supports multiple document formats:
  - AsciiDoc
  - Markdown
  - DocBook
  - HTML
  - LaTeX
  - reStructuredText
  - Txt2Tags

## Usage

The preview is active by default for documents with a supported filetype.  To disable the preview without unloading the plugin, change the filetype to one that is not supported, like a programming language.  The filetype will be automatically redetected as the correct type the next time the document is opened.

## Building and Installing

To build, run the following commands from the source directory:

```
  ./autogen.sh
  cd build-aux
  ../configure
  make
```

To install, run `make install`.  The plugin will be copied to `/usr/local/lib/geany/`.  To uninstall, run `make uninstall` or delete the files `preview.*`.

## Requirements

This plugin depends on the following libraries and programs:

* [Geany](https://geany.org/)
* [GTK+](http://www.gtk.org)
* [WebKit2GTK](http://webkitgtk.org)
* [Pandoc](https://pandoc.org/)
* [libcmark-gfm](https://github.com/github/cmark-gfm)
* [Asciidoctor](https://asciidoctor.org/)

## License

The Preview Geany plugin is licensed under the [GPLv2](COPYING) or later.  It uses code from [Code Format](https://github.com/codebrainz/code-format/) and [Markdown](https://plugins.geany.org/markdown.html) written by Michael Brush.
