# Preview Plugin for Geany

## Building from Source with Meson

1. Install dependencies:

    * **Geany** – the editor itself, to run the plugin
        * requires GTK3
    * **Meson** – the build system
        * requires Ninja and pkgconfig
    * **WebKitGTK** – the webview, to show the preview
    * **cmark-gfm** – to render Markdown documents
    * **tomlplusplus** – to save and restore configuration

    Optional:

    * **PoDoFo** – to export PDF from Fountain documents
    * **Asciidoctor** – to render AsciiDoc files
    * **Pandoc** – to render other document formats

    Package names and other dependencies may differ, based on distro.

    The build system should display a notification for missing depends.  Install them as needed.

2. Open a terminal in a clean working directory.

3. Clone the repo.

    ```
    git clone https://github.com/xiota/geany-preview.git
    ```

4. Configure and build with meson.

    ```
    meson setup build geany-preview
    meson compile -C build
    ```

5.  Installing directly from source is not recommended.  Creating a package so that a package manager can track the install is highly recommended.

6. However, if you wish to proceed with installation.

    ```
    meson install -C build
    ```
