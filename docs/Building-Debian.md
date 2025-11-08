# Preview Plugin for Geany

## Building on Debian or Ubuntu

1. Open a terminal in a clean working directory.

2. Clone the repo, and change into the source path.

    ```
    git clone https://github.com/xiota/geany-preview.git
    cd geany-preview
    ```

3. Update your system.

    ```
    sudo apt-get update
    sudo apt-get dist-upgrade
    ```

4. Install dependencies, listed in `debian/control`.

    ```
    sudo apt-get build-dep ./
    ```

5. Build the package.

    ```
    dpkg-buildpackage -us -uc
    ```

6. Install the package.

    ```
    sudo apt install ../geany-plugin-preview_*.deb
    ```
