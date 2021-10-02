# Questions and Answers

## General

> Can Preview be used for web design?

While the Preview plugin can also show HTML, it does not have features that web designers might want, like the devtools console. It is intended as an authoring tool for use with light-weight markup languages, like Markdown. It gives Geany features similar to those found in formiko and retext. If you want to try them, they can both be installed from the repository with apt.

> Will Preview become part of geany-plugins?

I currently do not intend to submit this plugin to geany-plugins because doing so would limit my ability to add features and fix bugs. Of course, this is open source, so they are free to make a copy.

## Usage

> How can I preview documents other than HTML or Markdown?

The Preview plugin uses external programs to process other document types.

* [Asciidoctor](https://asciidoctor.org/) is used to process AsciiDoc files.

* [Pandoc](https://pandoc.org/) is used to process the following formats:

  + DocBook
  + LaTeX
  + reStructuredText
  + Textile
  + Txt2Tags
  + DokuWiki, MediaWiki, Tiki Wiki, TWiki

* [Screenplain](https://github.com/vilcans/screenplain) is used to process screenplays written in [Fountain](https://www.fountain.io/).

## Configuration

> Are there any configuration options?

There is a configuration file `preview.conf` that can be edited.  It is located at `~/.config/geany/plugins/preview/`.  The options are documented with comments.

For convenience, there are buttons to access the file and config folder.  They may be reached from the Geany menu: *Edit/Plugin Preferences/Preview*.

![convenience buttons](geany-plugin-preferences.png)

> My config file doesn't have some options mentioned on this page.  How do I use them?

You probably have an old config file from before some features were added.  You can update the file in the plugin preferences:

1. Click the button that says "Reset Config".  This will replace the old config file with one that has the new options.

2. Click the button that says "Save Config".  This will save your current settings (from your old config) into the new config file.

3. Click "Edit Config" to edit the new settings.

> Can I customize the way documents are rendered?

You can edit `css` files in the configuration folder.  Create the files if they do not exist.

* `asciidoctor.css`
* `markdown.css`
* `pandoc.css`
* `pandoc-markdown.css`
* `pandoc-rst.css`
* `pandoc-t2t.css`
* `pandoc-textile.css`
* `screenplain.css`

W3Schools has a [CSS Tutorial](https://www.w3schools.com/css/) that you may find helpful.

> Is there a dark theme?

Change the `extra_css` option in the config file.
```
extra_css=dark.css
```

> Can Preview show changes without having to scroll?

I would like to synchronize the scrollbars, but don't currently know how.

For long documents, there is a snippets mode that shows a preview for only a small section of the document. You can edit the settings in the config file to determine how long the document is before snippets activates and how much of the document it shows.

## Installation

> How do I install the Preview plugin?

The Preview plugin for Geany can be installed on Ubuntu 21.04 and 21.10 via PPA.
```
sudo add-apt-repository ppa:xiota/geany-plugins
sudo apt-get update
```

> How do I build from source?

There is a separate [document](docs/Building_and_Installing.md) with details to build on Linux. The plugin cannot be built on Windows because WebKit2GTK is not available, and I do not have a Mac to test with MacOS.

## Problems

> Why does the plugin sometimes say "Unable to process type" or display files with strange formatting?

For performance, updates are disabled when the preview is not visible or the document is not actively being edited.  This sometimes results in the glitches you've noticed.  The Preview will update normally when the document is edited.  Sometimes switching documents also helps.
