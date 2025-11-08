# Preview Plugin for Geany

## Building on Fedora

1. Open a terminal in a clean working directory.

2. Update your system, and install packaging tools.

    ```
    sudo dnf upgrade --refresh
    sudo dnf install rpm-build rpmdevtools
    ```

3. Download the `.spec` file.

    ```
    curl -L -o geany-plugins-preview.spec \
       https://github.com/xiota/geany-preview/raw/refs/heads/main/geany-plugins-preview.spec
    ```

4. Install dependencies, listed in the `.spec` file.

    ```
    sudo dnf builddep geany-plugins-preview.spec
    ```

5. Download the tarball, and build the package.

    ```
    spectool -g -R geany-plugins-preview.spec
    rpmbuild -ba geany-plugins-preview.spec
    ```

6. Install the package.

    ```
    sudo dnf install ~/rpmbuild/RPMS/*/geany-plugins-preview-[0-9]*.rpm
    ```
