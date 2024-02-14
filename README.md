WebP GDK Pixbuf Loader library
==============================
![](https://github.com/aruiz/webp-pixbuf-loader/actions/workflows/linux.yml/badge.svg)
![](https://github.com/aruiz/webp-pixbuf-loader/actions/workflows/macos.yml/badge.svg)
![](https://github.com/aruiz/webp-pixbuf-loader/actions/workflows/msys2.yml/badge.svg)

[![Packaging status](https://repology.org/badge/vertical-allrepos/webp-pixbuf-loader.svg?exclude_unsupported=1)](https://repology.org/project/webp-pixbuf-loader/versions)


Building from source
--------------------
  Build:
  ```
  meson builddir
  ninja -C builddir
  ```

  Install dependencies on Debian:
  ```
  sudo apt install libwebp-dev libgdk-pixbuf2.0-dev libgtk-3-dev meson build-essential
  ```
  (libgtk-3-dev is optional, used in development)

  Install dependencies on Fedora:
  ```
  sudo dnf install libwebp-devel gdk-pixbuf2-devel meson gcc
  ```

  Install dependencies on macOS via brew:
  ```sh
  $ brew install webp gdk-pixbuf meson ninja pkg-config
  ```

  Install:
  ```
  sudo ninja -C builddir install
  ```
