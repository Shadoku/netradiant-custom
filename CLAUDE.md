# NetRadiant-Custom

A Qt5 level editor for id Tech games (`radiant/`) plus the map compiler tools (`tools/`).
Shared code is in `libs/` and `include/`; loadable modules are in `plugins/` and `contrib/`.
`COMPILING` has the upstream build notes.

## Building on Linux

Tested on Ubuntu 24.04.

Install the dependencies:

```bash
apt-get install -y qtbase5-dev libqt5svg5-dev libxml2-dev libglib2.0-dev libpng-dev libjpeg-dev libwebp-dev libassimp-dev zlib1g-dev libgl-dev g++-14
```

Build the editor with its modules and plugins:

```bash
make -j4 BUILD=release DOWNLOAD_GAMEPACKS=no DEPENDENCIES_CHECK=off CXX=g++-14 CC=gcc-14 binaries-radiant
```

- Use GCC 14 or newer. GCC 13 fails with "taking address of temporary array" in
  `plugins/vfspk3/vfs.cpp` and `contrib/bobtoolz/misc.h`; `-fpermissive` does not help.
- `binaries-radiant` builds the editor only. `make` with no target also builds the tools
  (q3map2 and others) and installs data; that full build has not been tried here.
- `binaries-radiant-all` is not a real target and builds nothing.
- Output goes to `install/`, which is in `.gitignore`. Object files are written next to the sources.

`binaries-radiant` does not copy the data files. Do it by hand:

```bash
cp -r setup/data/tools/* install/
```

```bash
echo 6 > install/RADIANT_MAJOR && echo 0 > install/RADIANT_MINOR
```

After changing anything under `setup/data/tools/` (themes, icons), copy it to `install/` again.

## Running

Radiant needs at least one gamepack in `install/gamepacks/`: a `games/<name>.game` file and a
`<name>.game/` directory. `download-gamepacks.sh` lists the sources. The Quake 3 pack is at
`https://gitlab.com/netradiant/gamepacks/quake3-mapeditor-support.git`; copy its `games/*` to
`install/gamepacks/games/` and its `q3.game/` to `install/gamepacks/`. Most gamepacks are
proprietary, so do not commit them.

Preferences can be set on the command line, which skips the first-run dialogs:
`-global-<name> <value>` for global preferences and `-<game>.game-<name> <value>` for per-game ones.

To run without a display and take a screenshot (needs `xvfb` and ImageMagick; Mesa's software
renderer provides OpenGL):

```bash
Xvfb :99 -screen 0 1280x800x24 &
```

```bash
DISPLAY=:99 install/radiant.x86_64 -global-gamefile q3.game -global-gamePrompt false -global-GUITheme 7 -q3.game-EnginePath /path/to/q3/ &
```

```bash
DISPLAY=:99 import -window root shot.png
```

Wait about 20 seconds after launch before taking the screenshot. Settings are saved in
`~/.netradiant/1.6.0/`.

## Committed binaries

`install/radiant.x86_64`, `install/modules/*.so` and `install/plugins/*.so` are committed
(force-added past `.gitignore`). They are a Linux x86-64 release build made on Ubuntu 24.04 with
GCC 14, linked against that system's Qt 5 and other libraries. They do not update themselves:
rebuild and commit them again after changing the source.

## Theming

- **GUI themes**: a fixed list in `radiant/theme.cpp` (`ETheme` and the name list in
  `theme_construct_menu` must stay in the same order). Each theme sets a Qt style, a palette, a
  `.qss` file from `setup/data/tools/themes/` and the light or dark icon set. The chosen theme
  is saved as the `GUITheme` global preference, by index.
- **Images in a `.qss`**: referenced through a Qt search-path prefix registered in `theme.cpp`
  (for example `url(gtkr:themes/gtkradiant14/frame_out.svg)`), so paths do not depend on the
  working directory.
- **Viewport colours**: JSON presets in `setup/data/tools/themes/_colors/`. Every `.json` file
  there appears in Misc → Theming → Viewports Theme; the keys are listed in `g_ColoursMenu` in
  `radiant/colors.cpp`.
- **Icons**: SVG files in `setup/data/tools/bitmaps/`. The light icon set is generated at startup
  by `Bitmaps_generateLight` in `libs/gtkutil/image.cpp`, which swaps a few fixed colours.
- **Classic icons**: `setup/data/tools/bitmaps_classic/` is a Qt icon theme of 16px PNG icons
  (with 2x and 3x copies enlarged without smoothing), used only by the GtkRadiant 1.4 GUI theme.
  Its `index.theme` inherits the light icon set, so an icon it lacks falls back to the SVG one.
  Most icons are GtkRadiant originals (`install/bitmaps/` in `TTimo/GtkRadiant`, GPL); the rest
  are NetRadiant's older icons from `setup/data/tools-png/` and `setup/data/tools-src/`. That
  theme's `.qss` sets the toolbar icon size to 16px to match.
- **Window layout**: a separate preference (`g_Layout_viewStyle` in `radiant/mainframe.cpp`).
  Regular is the classic arrangement: XY view and console on the left, camera and texture
  browser on the right.

A quick way to check a `.qss` without building Radiant is a small PySide6 script that sets the
Fusion style, the same palette and the stylesheet on a mock window, run with
`QT_QPA_PLATFORM=offscreen`.
