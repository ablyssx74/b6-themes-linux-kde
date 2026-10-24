/*
 * Lays out and paints the frame of a window from an xfwm4 theme: borders, title tab with its buttons, the bar beside it.
 * The rules are those of the Haiku port of the same themes (decorators XfwmDecorator), for a window with one tab.
 * No KDE here, only Qt Gui, so the preview tool can draw exactly what the KWin plugin draws.
 */
#ifndef B6_FRAME_RENDERER_H
#define B6_FRAME_RENDERER_H

#include "XfwmTheme.h"

#include <QFont>
#include <QMargins>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>

namespace B6 {

struct FrameParams {
	QSize clientSize;
	bool active = true;
	QString title;
	QFont font;
	bool hasClose = true, hasMaximize = true, hasHide = true, hasShade = false;
	int hovered = -1;		// a Button, or -1
	int pressed = -1;
};

struct FrameLayout {
	// Everything but size, borders and titleBar is in the theme's own coordinates, where the pictures have see-through margins; the
	// decoration is the part of that which is not see-through, starting at origin. toDecoration() converts a rectangle.
	QSize size;				// of the whole decoration
	QSize themeSize;		// of the frame with the margins
	QMargins borders;		// around the client, in the decoration
	QPoint origin;
	QRect titleBar;			// in the decoration: the bar, where the pointer moves the window
	QRect client;
	QRect tab;				// x and width of the tab, the height of the bar
	QRect text;				// where the title is drawn
	QRect buttons[ButtonCount];
	QRect toDecoration(const QRect& rect) const { return rect.translated(-origin); }
	int topRightX = 0;
	int restStart = 0;
	int fillLeft = 0, fillWidth = 0;
};

FrameLayout computeLayout(const XfwmTheme& theme, const FrameParams& params);
void paintFrame(QPainter& painter, const XfwmTheme& theme, const FrameParams& params, const FrameLayout& layout);

}	// namespace B6

#endif
