WebP GDK Pixbuf Loader library
==============================

[![Packaging status](https://repology.org/badge/vertical-allrepos/webp-pixbuf-loader.svg?exclude_unsupported=1)](https://repology.org/project/webp-pixbuf-loader/versions)


Building from source
--------------------
  Build:
  ```
  meson builddir
  ninja -C builddir
  ```

  Build on Debian/Ubuntu:

  Install dependencies:
  ```
  sudo apt install libwebp-dev libgdk-pixbuf2.0-dev libgtk-3-dev meson build-essential
  ```
  (libgtk-3-dev is optional, used in development)

  Build:
  ```
  meson builddir -Dgdk_pixbuf_query_loaders_path=/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders
  ninja -C builddir
  ```

  Install:
  ```
  sudo ninja -C builddir install
  ```
