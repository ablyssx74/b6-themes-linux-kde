# b6-themes-linux-kde

xfwm4 window-frame themes as a KWin (Plasma 6) window decoration. One plugin offers every theme folder it finds, listed in
System Settings as **xfwm4: <name>**. Ported so far: **b6**, **b6+**, **b6+r** (the b6 frame with the close and zoom buttons in the
tab, the way Haiku's own decorator has them), **b5+** and **crux**. The frame is drawn by the same rules as the Haiku port of these
themes (`XfwmDecorator`), so a window looks the same on both.

**Status: prototype.** One tab per window (KWin has no windows grouped in tabs), no title shadows, no shadows around windows yet.
Maximized windows are the bar alone, flush with the screen. The menu, "on all desktops", shade, minimize, maximize and close buttons
are all there when the theme has pictures for them.

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
free to redistribute). A theme is a folder with a `themerc`. The plugin lists every one it finds in:

- `~/.local/share/b6-themes/<name>` and `/usr/share/b6-themes/<name>`, and
- the folders in `B6_THEMES_PATH` (a colon separated list), for trying themes without installing them.

`b6-preview <theme folder> out.png` draws one.

## Trying the plugin
A decoration plugin runs inside KWin, so a bug in it can take the compositor down. Look at it first in a separate process:

```
QT_PLUGIN_PATH=$PWD/build/plugins B6_THEMES_PATH=$PWD/themes kcmshell6 kwindecoration
```
(the plugin file has to be in `build/plugins/org.kde.kdecoration3/`), and only install it for the session once that works:

```
sudo cmake --install build
```
then pick **B6 (xfwm4)** in System Settings, Appearance, Window Decorations.
