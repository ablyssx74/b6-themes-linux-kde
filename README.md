# b6-themes-linux-kde

xfwm4 window-frame themes as a KWin (Plasma 6) window decoration. The first theme is **b6+r**: the b6 frame with the close and
zoom buttons in the tab, the way Haiku's own decorator has them. The frame is drawn by the same rules as the Haiku port of these
themes (`XfwmDecorator`), so a window looks the same on both.

**Status: prototype.** One tab per window, b6+r only, no stacked tabs (KWin has no windows grouped in tabs), no shadows yet.

## Parts
- `src/` &ndash; theme loading and frame drawing. Qt Gui only, no KDE.
- `tools/preview.cpp` &ndash; `b6-preview <theme folder> <out.png> [scale]` draws frames (active, inactive, long titles, hover,
  pressed) to a PNG without KWin, so a change can be looked at without touching the desktop.
- `plugin/` &ndash; the KDecoration3 plugin (`org.kde.b6xfwm`). It only places the buttons and calls the drawing code.

## Build
```
cmake -S . -B build
cmake --build build
./build/b6-preview themes/b6+r b6r.png 2
```

The plugin is built when `KDecoration3` and `KF6CoreAddons` are found.

## Themes
The theme pictures are **not** in this repository: `themes/` is ignored. Take them from the xfwm4-themes collection
(GPL-3; the README of `b6` says it was inspired by a leaked build of an OS that is no longer around, so its artwork may not be
free to redistribute). Put a theme's folder (the one with `themerc`) in `themes/`, or anywhere, and point the plugin at it:

- `B6_THEME_DIR=/path/to/b6+r` &ndash; for a test, or
- `~/.local/share/b6-themes/b6+r` / `/usr/share/b6-themes/b6+r` &ndash; found automatically.

## Trying the plugin
A decoration plugin runs inside KWin, so a bug in it can take the compositor down. Look at it first in a separate process:

```
QT_PLUGIN_PATH=$PWD/build/plugins B6_THEME_DIR=$PWD/themes/b6+r kcmshell6 kwindecoration
```
(the plugin file has to be in `build/plugins/org.kde.kdecoration3/`), and only install it for the session once that works:

```
sudo cmake --install build
```
then pick **B6 (xfwm4)** in System Settings, Appearance, Window Decorations.
