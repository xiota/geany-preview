# Preview Plugin for Geany

## Building on Debian or Ubuntu

1. Open a terminal in a new empty working directory.

2. (optional) Update the system.

    ```
    sudo apt-get update
    sudo apt-get dist-upgrade
    ```

3. Clone the repo, and change into the source path.

    ```
    git clone https://github.com/xiota/geany-preview.git
    cd geany-preview
    ```

4. (optional) If rebuilding, can start by updating the source, skipping earlier steps.

    ```
    cd geany-preview
    git clean -fdx
    git reset --hard
    git pull
    ```

5. Install dependencies.  The following command, as written, collects and installs them from `debian/control`.

    ```
    sudo apt-get build-dep ./
    ```

6. Build the package.

    ```
    dpkg-buildpackage -us -uc
    ```

7. Install the package.  The following command assumes starting with a clean working directory (step 1).  If rebuiding from updated source (step 4), use the exact filename of the `.deb` that was just built.

    ```
    sudo apt install ../geany-plugin-preview_*.deb
    ```
