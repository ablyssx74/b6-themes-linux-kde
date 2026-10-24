#include "FrameRenderer.h"

#include <QFontMetrics>

#include <cmath>

namespace B6 {

namespace {

bool present(const FrameParams& p, Button b)
{
	return b >= 0 && b < ButtonCount && p.has[b];
}

// the letters of a button layout, split at the '|': buttons to the left and to the right of it, in order
void splitLayout(const XfwmTheme& theme, const FrameParams& p, QList<Button>& left, QList<Button>& right)
{
	const QString layout = theme.buttonLayout();
	int divider = layout.indexOf('|');
	if (divider < 0)
		divider = layout.size();
	for (int i = 0; i < layout.size(); i++) {
		Button b = ButtonCount;
		switch (layout[i].toLatin1()) {
			case 'C': b = Close; break;
			case 'M': b = Maximize; break;
			case 'H': b = Hide; break;
			case 'S': b = Shade; break;
			case 'O': b = Menu; break;
			case 'T': b = Stick; break;
			default: break;
		}
		if (b == ButtonCount || !present(p, b))
			continue;
		if (i < divider)
			left.append(b);
		else if (i > divider)
			right.append(b);
	}
}

void blit(QPainter& painter, const Piece& piece, int x, int y)
{
	if (!piece.isNull())
		painter.drawImage(x, y, piece.image);
}

// tiles a picture over an area, the last tile cut off at the end of the area
void blitTiled(QPainter& painter, const Piece& piece, const QRect& area, bool horizontal)
{
	if (piece.isNull() || area.isEmpty())
		return;
	painter.save();
	painter.setClipRect(area, Qt::IntersectClip);
	if (horizontal) {
		for (int x = area.left(); x <= area.right(); x += piece.width())
			painter.drawImage(x, area.top(), piece.image);
	} else {
		for (int y = area.top(); y <= area.bottom(); y += piece.height())
			painter.drawImage(area.left(), y, piece.image);
	}
	painter.restore();
}

}	// namespace

FrameLayout computeLayout(const XfwmTheme& theme, const FrameParams& params)
{
	FrameLayout layout;
	const bool active = params.active;
	// a maximized window has the bar and nothing else: no sides, no bottom
	const int leftWidth = params.maximized ? 0 : theme.left(active).width();
	const int rightWidth = params.maximized ? 0 : theme.right(active).width();
	const int bottomHeight = params.maximized ? 0 : theme.bottom(active).height();
	const int titleHeight = theme.titleHeight();

	layout.borders = QMargins(leftWidth, titleHeight, rightWidth, bottomHeight);
	layout.size = QSize(leftWidth + params.clientSize.width() + rightWidth, titleHeight + params.clientSize.height() + bottomHeight);
	layout.client = QRect(QPoint(leftWidth, titleHeight), params.clientSize);

	const int width = layout.size.width();
	layout.topRightX = params.maximized ? width : width - theme.topRight(active).width();

	QList<Button> leftButtons, rightButtons;
	splitLayout(theme, params, leftButtons, rightButtons);
	const int buttonWidth = theme.buttonWidth();
	const int spacing = theme.buttonSpacing();

	// the tab: its two ends and the room for the title, which gets what is left of the width
	const int cap = theme.topLeft(active).width();
	const int before = theme.title(0, active).width() + theme.title(1, active).width();
	const int after = theme.title(3, active).width();
	const int roomForButtons = theme.buttonsInTab() && !rightButtons.isEmpty()
		? qMax(0, int(rightButtons.size()) * (buttonWidth + spacing) - 2) : 0;
	const int fixed = cap + before + after + roomForButtons;
	const int available = qMax(0, layout.topRightX);

	const QFontMetrics metrics(params.font);
	const int wanted = params.title.isEmpty() ? 0 : metrics.horizontalAdvance(params.title);
	int tabWidth = fixed + wanted;
	if (theme.fullWidthTitle() && tabWidth < available)
		tabWidth = available;
	if (tabWidth > available)
		tabWidth = qMax(fixed, available);

	layout.tab = QRect(0, 0, tabWidth, titleHeight);
	layout.fillLeft = cap + before;
	layout.fillWidth = qMax(0, tabWidth - (cap + before + after));
	layout.restStart = tabWidth;
	layout.text = QRect(layout.fillLeft, 0, qMax(0, tabWidth - fixed), titleHeight);

	// the buttons
	if (theme.buttonsInTab()) {
		int rx = tabWidth - theme.buttonOffsetRight();
		for (int i = rightButtons.size() - 1; i >= 0; i--) {
			layout.buttons[rightButtons[i]] = QRect(rx - buttonWidth, 0, buttonWidth, titleHeight);
			rx -= buttonWidth + spacing;
		}
		int lx = cap + theme.buttonOffset();
		for (Button b : leftButtons) {
			layout.buttons[b] = QRect(lx, 0, buttonWidth, titleHeight);
			lx += buttonWidth + spacing;
		}
	} else {
		int x = layout.client.right() - theme.buttonOffset();
		for (int i = rightButtons.size() - 1; i >= 0; i--) {
			layout.buttons[rightButtons[i]] = QRect(x - buttonWidth + 1, 0, buttonWidth, titleHeight);
			x -= buttonWidth + spacing;
		}
		x = layout.client.left() + theme.buttonOffset();
		for (Button b : leftButtons) {
			layout.buttons[b] = QRect(x, 0, buttonWidth, titleHeight);
			x += buttonWidth + spacing;
		}
		// the title keeps clear of the buttons at the ends of the bar
		int textLeft = layout.text.left(), textRight = layout.text.right() + 1;
		for (Button b : leftButtons)
			textLeft = qMax(textLeft, layout.buttons[b].right() + 1);
		for (Button b : rightButtons)
			textRight = qMin(textRight, layout.buttons[b].left());
		layout.text.setLeft(textLeft);
		layout.text.setRight(qMax(textLeft, textRight) - 1);
	}

	// The pictures have see-through margins (the left border of b6 is 14 wide and only its inner 5 columns are drawn). They are
	// not part of the window: everything is moved so that the visible frame starts at 0,0.
	const QSize full = layout.size;
	layout.themeSize = full;
	auto outer = [](const Piece& piece, int x, int y) { return piece.isNull() ? QRect() : piece.opaque.translated(x, y); };
	QRect visible;
	visible = visible.united(outer(theme.topLeft(active), 0, 0));
	visible = visible.united(outer(theme.title(2, active), layout.fillLeft, 0));
	if (params.maximized) {
		const Piece& rest = theme.title(4, active);
		visible = visible.united(QRect(0, rest.opaque.top(), width, rest.opaque.height()));
	} else {
		visible = visible.united(outer(theme.topRight(active), layout.topRightX, 0));
		visible = visible.united(outer(theme.left(active), 0, layout.client.top()));
		visible = visible.united(outer(theme.right(active), layout.client.right() + 1 - theme.right(active).opaque.left(), layout.client.top()));
		visible = visible.united(outer(theme.bottomLeft(active), 0, full.height() - theme.bottomLeft(active).height()));
		visible = visible.united(outer(theme.bottomRight(active), width - theme.bottomRight(active).width(), full.height() - theme.bottomRight(active).height()));
		visible = visible.united(outer(theme.bottom(active), theme.bottomLeft(active).width(), full.height() - theme.bottom(active).height()));
	}
	layout.origin = visible.topLeft();
	layout.size = visible.size();
	QRect client = layout.toDecoration(layout.client);
	// (a maximized window's pictures are only the bar: the client area below it is part of the window all the same, and the
	// borders at the sides and the bottom are none)
	if (params.maximized)
		layout.size.setHeight(qMax(layout.size.height(), client.bottom() + 1));
	layout.borders = QMargins(client.left(), client.top(), layout.size.width() - client.right() - 1, layout.size.height() - client.bottom() - 1);
	layout.titleBar = QRect(0, 0, layout.size.width(), client.top());
	return layout;
}

void paintFrame(QPainter& painter, const XfwmTheme& theme, const FrameParams& params, const FrameLayout& layout)
{
	const bool active = params.active;
	const Piece& left = theme.left(active);
	const Piece& right = theme.right(active);
	const Piece& bottom = theme.bottom(active);
	const Piece& bottomLeft = theme.bottomLeft(active);
	const Piece& bottomRight = theme.bottomRight(active);
	const QRect& client = layout.client;
	const int width = layout.themeSize.width();		// the frame in the theme's own coordinates
	const int height = layout.themeSize.height();

	painter.save();
	painter.translate(-layout.origin);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	// the sides and the bottom, then the bottom corners over them
	if (!params.maximized) {
		blitTiled(painter, left, QRect(0, client.top(), left.width(), client.height()), false);
		blitTiled(painter, right, QRect(client.right() + 1 - right.opaque.left(), client.top(), right.width(), client.height()), false);
		const int bottomY = client.bottom() + 1 - bottom.opaque.top();
		blitTiled(painter, bottom, QRect(bottomLeft.width(), bottomY, width - bottomLeft.width() - bottomRight.width(), bottom.height()), true);
		blit(painter, bottomLeft, 0, height - bottomLeft.height());
		blit(painter, bottomRight, width - bottomRight.width(), height - bottomRight.height());
	}

	// the bar after the tab, and its right end
	const Piece& rest = theme.title(4, active);
	if (layout.restStart < layout.topRightX)
		blitTiled(painter, rest, QRect(layout.restStart, 0, layout.topRightX - layout.restStart, rest.height()), true);
	if (!params.maximized)
		blit(painter, theme.topRight(active), layout.topRightX, 0);

	// the tab
	int x = 0;
	blit(painter, theme.topLeft(active), x, 0);
	x += theme.topLeft(active).width();
	blit(painter, theme.title(0, active), x, 0);
	x += theme.title(0, active).width();
	blit(painter, theme.title(1, active), x, 0);
	const Piece& middle = theme.title(2, active);
	if (layout.fillWidth > 0)
		blitTiled(painter, middle, QRect(layout.fillLeft, 0, layout.fillWidth, middle.height()), true);
	const Piece& end = theme.title(3, active);
	blit(painter, end, layout.tab.width() - end.width(), 0);

	// the title
	if (!params.title.isEmpty() && layout.text.width() > 0) {
		painter.save();
		painter.setFont(params.font);
		const QFontMetrics metrics(params.font);
		const QString text = metrics.horizontalAdvance(params.title) <= layout.text.width()
			? params.title : metrics.elidedText(params.title, Qt::ElideRight, layout.text.width());
		const int used = metrics.horizontalAdvance(text);
		int shift;
		if (theme.alignment() == 1)
			shift = (layout.text.width() - used) / 2;
		else if (theme.alignment() == 2)
			shift = layout.text.width() - used - theme.titleHorizontalOffset();
		else
			shift = theme.titleHorizontalOffset();
		shift = qBound(0, shift, qMax(0, layout.text.width() - used));
		const double textHeight = metrics.ascent() + metrics.descent();
		const int baseline = int(std::floor((layout.titleBar.height() - textHeight) / 2 + metrics.ascent() + theme.titleOffset(active) + 0.5));
		painter.setPen(theme.textColor(active));
		painter.drawText(layout.text.left() + shift, baseline, text);
		painter.restore();
	}

	// the buttons
	for (int b = 0; b < ButtonCount; b++) {
		if (layout.buttons[b].isEmpty() || !present(params, Button(b)))
			continue;
		ButtonState state;
		if (params.pressed == b)
			state = Pressed;
		else if (params.hovered == b)
			state = active ? Prelight : PrelightInactive;
		else
			state = active ? Active : Inactive;
		blit(painter, theme.button(Button(b), state, params.toggled[b]), layout.buttons[b].left(), layout.buttons[b].top());
	}
	painter.restore();
}

}	// namespace B6
