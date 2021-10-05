# Preview Plugin for Geany

This plugin provides a preview pane for Geany to view the formatting of several light-weight markup languages as they are edited.

Supported document types include AsciiDoc, DocBook, Fountain, HTML, LaTeX, Markdown, MediaWiki, reStructuredText, Textile, and Txt2Tags.
 
![screenshot](docs/screenshot-908.jpg)

## Features

* The preview is updated as the document is edited
* HTML and Markdown are supported directly
* Other document formats can be processed with auxiliary programs
* Rendering of each format can customized with stylesheets
* The preview of large documents can be limited to the area being edited

## Usage

The preview is active by default for documents with supported file types.  To display formats other than HTML or Markdown, auxiliary programs are needed.  If they are unavailable, no preview will be displayed for their respective formats.

More about usage and configuration is available in a [separate document](docs/Questions_and_Answers.md).

## Installation

The Preview plugin for Geany can be installed on Ubuntu 21.04 and 21.10 via PPA.
```
  sudo add-apt-repository ppa:xiota/geany-plugins
  sudo apt-get update
  sudo apt-get install geany-plugin-preview
```

Preview can also be [built and installed](docs/Building_and_Installing.md), but does not currently work on Windows because WebKit2GTK is not available.  I also do not have a Mac to test.

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
