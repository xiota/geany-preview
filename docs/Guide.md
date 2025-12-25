# Preview Plugin for Geany

## User Guide

### Installation

> What operating systems does Preview work with?

This plugin is developed and tested on Linux.  It does not work on Windows and has not been tested on Mac.

> How do I install the Preview plugin?

* For Arch Linux, Garuda Linux, and some other Arch-based distros, prebuilt binaries are available from [Chaotic AUR](https://aur.chaotic.cx/).  They can be installed with `pacman`.

    Packages are also available on the [Arch User Repository](https://wiki.archlinux.org/title/Arch_User_Repository).

    * [`aur/geany-plugin-preview`](https://aur.archlinux.org/packages/geany-plugin-preview)
    * [`aur/geany-plugin-preview-git`](https://aur.archlinux.org/packages/geany-plugin-preview-git)

* Debian and Ubuntu users, see [Building on Debian or Ubuntu](Building-Debian.md).

* Fedora users, see [Building on Fedora](Building-Fedora.md).

* Users who are interested in building directly from source, see [Building from Source with Meson](Building-Meson.md).

> How do I start the Preview plugin?

After installation, Preview can be enabled in the Plugin Manager (*Tools/Plugin Manager*).  The sidebar must be visible (*View/Show Sidebar*).

### Configuration

> Are there any configuration options?

Many.  Open *Tools/Preview/Preferences*.  Each option has a brief description in a tooltip.

![plugin preferences](screenshot-preferences.png)

While `preview.conf` can be edited manually, doing so is not recommended after the 0.2.x rewrite.  To reset the plugin to default settings, delete the config file while Geany is not running.

> Can I customize the way documents are rendered?

You can edit style sheets in the configuration folder, accessible from *Tools/Preview/Open Config Folder*.  Create the files if they do not exist.

* `preview.css` – Applies to all files.
* Others are named after the corresponding format.
  * `asciidoc.css`
  * `dokuwiki.css`
  * `markdown.css`

Style sheets should be reloaded on demand.  Altering or switching documents may help trigger a refresh.

W3Schools has a [CSS Tutorial](https://www.w3schools.com/css/) that may be helpful.

> What if I want to tweak the existing style sheets?

They are located in the source tree, in the [`data`](https://github.com/xiota/geany-preview/tree/main/data) folder.  Copy the ones you want to modify to the config folder.

> Is there a dark theme?

Add a section to `preview.css` similar to the following:

```CSS
/* Default (light theme) */
body {
  background-color: #ffffff;
  color: #222222;
}

/* Dark theme override */
@media (prefers-color-scheme: dark) {
  body {
    background-color: #121212;
    color: #eeeeee;
  }
}
```

> What is "Fountain"?

Fountain is a text-based screenplay format that basically looks like a standard screenplay with everything flush-left.

Preview uses [ftn2xml](https://github.com/xiota/ftn2xml) to convert screenplays for display.  The syntax it recognizes is described at [Fountain Syntax](https://github.com/xiota/ftn2xml/blob/main/Fountain_Syntax.md).

> Can I add Fountain to filetypes recognized by Geany?

You can, but the benefits are limited to showing the type in the status bar, showing the extensions in the Open/Save dialogs, and referencing the type from plugins, such as Geanylua.  I do not know how to add scenes to the Symbols tab without patching and recompiling Geany. 

To add the Fountain document type:

1. Make sure the following is contained in `~/.config/geany/filetype_extensions.conf`

    ```ini
    [Extensions]
    Fountain=*.ftn;*.fountain;
    ```

2. Create `~/.config/geany/filedefs/filetypes.Fountain.conf`

    ```ini
    # For complete documentation of this file, please see Geany's main documentation
    [styling]
    # Edit these in the colorscheme .conf file instead

    [settings]
    lexer_filetype=none

    # default extension used when saving files
    extension=ftn

    # MIME type
    mime_type=text/x-fountain

    # sort tags by appearance
    symbol_list_sort_mode=1

    # multiline comments
    comment_open=/*
    comment_close=*/
    ```

### Usage

> How can I preview other document formats?

External programs are used to process other document types.

* [Asciidoctor](https://asciidoctor.org/) is used to process AsciiDoc files.

* [Pandoc](https://pandoc.org/) is used to process the following formats:

  * DocBook
  * LaTeX
  * reStructuredText
  * Textile
  * Txt2Tags
  * DokuWiki, MediaWiki, Tiki Wiki, TWiki

> Some Markdown editors autosave the document as it is edited.  Can the Preview plugin do that?

Auto saving documents is outside the scope of this plugin.  However, another plugin that does what you want is included with Geany: *Save Actions*

### General Questions

> Can Preview be used for web design?

While the Preview plugin can show HTML, it does not have features that web designers might want, like the devtools console. It is intended as an authoring tool for use with light-weight markup languages, like Markdown.

> Will Preview become part of geany-plugins?

No.  Inclusion would limit my ability to make changes.

> Is it possible to hide/show the sidebar based on document type, rather than display a message that the document is not supported?

Altering the sidebar's general behavior is outside the scope of this plugin.

However, changing sidebar behavior based on document type may be possible with a GeanyLua script.

> Is it possible to change the appearance of the sidebar tabs so that it is visibly recognizable when they have focus?

Altering the sidebar's general behavior is outside the scope of this plugin.

Changing widget appearance in response to focus changes is very difficult with GTK3.


### Troubleshooting

> I updated the plugin, and now the preview looks different.  What happened?

The plugin has been completely rewritten (version 0.2.x).  The style sheets in the config folder may need to be revised.  See [Configuration](#Configuration).

> Why don't HTML documents look the same in Preview as in the Chromium (and Firefox)?

The default stylesheet and "quirks" modes may be different from web browsers.  Try using a "stylesheet reset" to improve consistency across browsers.  If precise formatting is important, test documents in the target browsers.

> When Geany is first started, the tab is gray before the first Preview is generated.  Is there any way to change the color?

That is probably the color of the window before the WebView is fully loaded.

I do not know how to change it.

> Can Preview show changes without having to scroll?

Scroll position should be stable after version 0.2.1.
