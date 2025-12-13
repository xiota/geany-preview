# Preview Plugin for Geany

## Building on Fedora

1. Open a terminal in a new empty working directory.

2. (optional) Update the system.

    ```
    sudo dnf upgrade --refresh
    ```

3. Install packaging tools.

    ```
    sudo dnf install rpm-build rpmdevtools
    ```

4. Download the `.spec` file.

    ```
    curl -L -o geany-plugins-preview.spec \
       https://github.com/xiota/geany-preview/raw/refs/heads/main/docs/geany-plugins-preview.spec
    ```

5. Install dependencies, listed in the `.spec` file.

    ```
    sudo dnf builddep geany-plugins-preview.spec
    ```

6. Download the tarball, as specified in the `.spec` file.

    ```
    spectool -g -R geany-plugins-preview.spec
    ```

7. Build the package.

    ```
    rpmbuild -ba geany-plugins-preview.spec
    ```

8. Install the package.

    ```
    sudo dnf install ~/rpmbuild/RPMS/*/geany-plugins-preview-[0-9]*.rpm
    ```
