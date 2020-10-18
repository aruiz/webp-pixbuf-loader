WebM GDK Pixbuf Loader library
==============================

[![Packaging status](https://repology.org/badge/vertical-allrepos/webp-pixbuf-loader.svg)](https://repology.org/project/webp-pixbuf-loader/versions)


Building from source
--------------------
  Build:
  ```
  meson builddir
  ninja -C builddir
  ```

  Build on Debian/Ubuntu:
  ```
  meson builddir -Dgdk_pixbuf_query_loaders_path=/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders
  ninja -C builddir
  ```

  Install:
  ```
  sudo ninja -C builddir install
  ```
