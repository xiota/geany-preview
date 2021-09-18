# Preview Plugin for Geany

This plugin provides a basic preview of documents written in several markup formats.

![screenshot](docs/screenshot-908.jpg)

## Features

* The preview is updated as the document is edited
* HTML and Markdown are supported directly
* Other document formats can be processed with auxiliary programs
* Each format can have its own stylesheet
* Preview can be limited to a small portion of large documents

## Usage

The preview is active by default for documents with a supported file type.  For performance, updates are disabled when the preview is not visible.  It will be updated again when the document is edited.

To display formats other than HTML or Markdown requires the use of auxiliary programs.  If they are not installed, no preview will be displayed.  If Geany is running in a terminal, a warning message about the missing program will be printed to stderr.

* [Asciidoctor](https://asciidoctor.org/) is used to process AsciiDoc files.

* [Pandoc](https://pandoc.org/) is used to process the following formats:

  + DocBook
  + LaTeX
  + reStructuredText
  + Textile
  + Txt2Tags
  + DokuWiki, MediaWiki, Tiki Wiki, TWiki

* [Screenplain](https://github.com/vilcans/screenplain) is used to process screenplays written in [Fountain](https://www.fountain.io/).

Custom stylesheets and other settings may changed by editing files in the `geany/plugins/preview` config folder.  Missing config files will be recreated when they're needed.

## Building and Installing

The Preview plugin for Geany can be [built and installed](docs/Building_and_Installing.md) on Linux-based operating systems.  It has not been tested on MacOS, and it does not currently work on Windows because WebKit2GTK is not available on that platform.

## Requirements

This plugin depends on the following libraries and programs:

* [Geany](https://geany.org/)
* [GTK/Glib](http://www.gtk.org)
* [libcmark-gfm](https://github.com/github/cmark-gfm)
* [WebKit2GTK](http://webkitgtk.org)

Optional auxiliary programs may also be used to process additional formats:

* [Asciidoctor](https://asciidoctor.org/)
* [Pandoc](https://pandoc.org/)
* [Screenplain](https://github.com/vilcans/screenplain)

## License

The Preview plugin for Geany is licensed under the [GPLv2](COPYING) or later.  It uses code from the [Code Format](https://github.com/codebrainz/code-format/) and [Markdown](https://plugins.geany.org/markdown.html) plugins written by [Michael Brush](https://github.com/codebrainz).
