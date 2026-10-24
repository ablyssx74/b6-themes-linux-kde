/*
 * An xfwm4 theme: the pictures of a window frame (title bar, corners, borders, buttons) and the few values of its themerc that
 * matter to a decoration. Only Qt Gui is needed, so the same code draws in the KWin plugin and in the preview tool.
 */
#ifndef B6_XFWM_THEME_H
#define B6_XFWM_THEME_H

#include <QColor>
#include <QImage>
#include <QRect>
#include <QString>

namespace B6 {

// the letters of a themerc button_layout: C close, M maximize, H hide (minimize), S shade, O window menu, T stick (on all desktops)
enum Button { Close = 0, Maximize, Hide, Shade, Menu, Stick, ButtonCount };
enum ButtonState { Active = 0, Inactive, Prelight, PrelightInactive, Pressed, StateCount };

// one picture of the frame; "None" pixels of an XPM are transparent
struct Piece {
	QImage image;
	QRect opaque;		// bounding box of the pixels that are not transparent

	bool isNull() const { return image.isNull(); }
	int width() const { return image.width(); }
	int height() const { return image.height(); }
};

class XfwmTheme {
public:
	bool load(const QString& directory);
	bool isValid() const { return fValid; }
	const QString& name() const { return fName; }

	// the frame, for the active (0) or the inactive (1) window
	const Piece& topLeft(bool active) const { return fTopLeft[active ? 0 : 1]; }
	const Piece& topRight(bool active) const { return fTopRight[active ? 0 : 1]; }
	// title-1 ... title-5: left end, left tile, the tab's tile under the text, right end, the bar after the tab
	const Piece& title(int index, bool active) const { return fTitle[index][active ? 0 : 1]; }
	const Piece& left(bool active) const { return fLeft[active ? 0 : 1]; }
	const Piece& right(bool active) const { return fRight[active ? 0 : 1]; }
	const Piece& bottom(bool active) const { return fBottom[active ? 0 : 1]; }
	const Piece& bottomLeft(bool active) const { return fBottomLeft[active ? 0 : 1]; }
	const Piece& bottomRight(bool active) const { return fBottomRight[active ? 0 : 1]; }
	// a button's picture; the toggled one is for a window that is maximized, shaded or on all desktops (xfwm4's "-toggled" files)
	const Piece& button(Button button, ButtonState state, bool toggled = false) const
	{
		return toggled && !fButtonToggled[button][state].isNull() ? fButtonToggled[button][state] : fButton[button][state];
	}

	// measurements taken from the pictures
	int titleHeight() const { return fTitleHeight; }
	int buttonWidth() const { return fButtonWidth; }

	// themerc
	QColor textColor(bool active) const { return active ? fActiveText : fInactiveText; }
	int titleOffset(bool active) const { return active ? fOffsetActive : fOffsetInactive; }
	int titleHorizontalOffset() const { return fTitleOffsetX; }
	int alignment() const { return fAlignment; }	// 0 left, 1 center, 2 right
	bool fullWidthTitle() const { return fFullWidth; }
	const QString& buttonLayout() const { return fButtonLayout; }
	bool buttonsInTab() const { return fButtonsInTab; }
	int buttonOffset() const { return fButtonOffset; }
	int buttonOffsetRight() const { return fHasOffsetRight ? fButtonOffsetRight : fButtonOffset; }
	int buttonSpacing() const { return fButtonSpacing; }

private:
	bool loadPiece(Piece& piece, const QString& baseName) const;
	void readThemerc(const QString& path);

	bool fValid = false;
	QString fDirectory;
	QString fName;
	Piece fTopLeft[2], fTopRight[2], fTitle[5][2], fLeft[2], fRight[2], fBottom[2], fBottomLeft[2], fBottomRight[2];
	Piece fButton[ButtonCount][StateCount];
	Piece fButtonToggled[ButtonCount][StateCount];

	int fTitleHeight = 24;
	int fButtonWidth = 12;
	QColor fActiveText = Qt::black, fInactiveText = Qt::black;
	int fOffsetActive = 0, fOffsetInactive = 0, fTitleOffsetX = 0;
	int fAlignment = 1;
	bool fFullWidth = true;
	QString fButtonLayout = "O|HMC";
	bool fButtonsInTab = false;
	int fButtonOffset = 0, fButtonOffsetRight = 0, fButtonSpacing = 0;
	bool fHasOffsetRight = false;
};

}	// namespace B6

#endif
