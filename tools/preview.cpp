/*
 * Draws window frames from a theme to a PNG, without KWin: the same layout and painting code the plugin uses.
 *   b6-preview <theme directory> <output.png> [scale]
 */
#include "FrameRenderer.h"

#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>

#include <cstdio>
#include <cstdlib>

using namespace B6;

static QImage drawCase(const XfwmTheme& theme, FrameParams params, const QColor& desktop)
{
	const FrameLayout layout = computeLayout(theme, params);
	QImage frame(layout.size, QImage::Format_ARGB32_Premultiplied);
	const QRect client = layout.toDecoration(layout.client);
	frame.fill(Qt::transparent);
	{
		QPainter painter(&frame);
		paintFrame(painter, theme, params, layout);
	}
	// the window's own content, over the frame, as the compositor does
	QImage sheet(layout.size + QSize(16, 16), QImage::Format_ARGB32_Premultiplied);
	sheet.fill(desktop);
	QPainter painter(&sheet);
	painter.drawImage(8, 8, frame);
	painter.fillRect(client.translated(8, 8), QColor(0xf2, 0xf2, 0xf2));
	painter.setPen(QColor(0x66, 0x66, 0x66));
	painter.drawText(client.translated(8, 8), Qt::AlignCenter, "window content");
	return sheet;
}

int main(int argc, char** argv)
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	if (argc < 3) {
		fprintf(stderr, "usage: %s <theme directory> <output.png> [scale]\n", argv[0]);
		return 2;
	}
	XfwmTheme theme;
	if (!theme.load(argv[1])) {
		fprintf(stderr, "cannot load the theme in %s\n", argv[1]);
		return 1;
	}
	const int scale = argc > 3 ? atoi(argv[3]) : 1;

	printf("theme %s: title height %d, button width %d, buttons \"%s\"%s\n", qPrintable(theme.name()), theme.titleHeight(),
		theme.buttonWidth(), qPrintable(theme.buttonLayout()), theme.buttonsInTab() ? " (in the tab)" : "");
	for (bool active : {true, false}) {
		printf("  %s: left %dx%d (opaque x %d..%d), right %dx%d (opaque x %d..%d), bottom %dx%d, top-left %dx%d, top-right %dx%d\n",
			active ? "active" : "inactive", theme.left(active).width(), theme.left(active).height(), theme.left(active).opaque.left(),
			theme.left(active).opaque.right(), theme.right(active).width(), theme.right(active).height(), theme.right(active).opaque.left(),
			theme.right(active).opaque.right(), theme.bottom(active).width(), theme.bottom(active).height(), theme.topLeft(active).width(),
			theme.topLeft(active).height(), theme.topRight(active).width(), theme.topRight(active).height());
	}

	FrameParams base;
	base.font = QFont("Noto Sans", 10);
	base.font.setStyleStrategy(QFont::PreferAntialias);
	base.clientSize = QSize(420, 160);
	base.title = "Terminal";

	QList<QImage> cases;
	const QColor desktop(0x5a, 0x7a, 0x9a);
	FrameParams p = base;
	cases << drawCase(theme, p, desktop);
	p = base; p.active = false; cases << drawCase(theme, p, desktop);
	p = base; p.title = "A much longer window title that has to be cut off somewhere"; cases << drawCase(theme, p, desktop);
	p = base; p.clientSize = QSize(200, 120); p.title = "Files — b6+r"; cases << drawCase(theme, p, desktop);
	p = base; p.hovered = Close; cases << drawCase(theme, p, desktop);
	p = base; p.hovered = Maximize; p.pressed = -1; cases << drawCase(theme, p, desktop);
	p = base; p.pressed = Close; cases << drawCase(theme, p, desktop);
	p = base; p.pressed = Maximize; cases << drawCase(theme, p, desktop);
	p = base; p.active = false; p.hovered = Close; cases << drawCase(theme, p, desktop);
	p = base; p.title = ""; cases << drawCase(theme, p, desktop);

	int width = 0, height = 0;
	for (const QImage& c : cases) {
		width = qMax(width, c.width());
		height += c.height();
	}
	QImage out(width, height, QImage::Format_ARGB32_Premultiplied);
	out.fill(desktop);
	QPainter painter(&out);
	int y = 0;
	for (const QImage& c : cases) {
		painter.drawImage(0, y, c);
		y += c.height();
	}
	painter.end();
	if (scale > 1)
		out = out.scaled(out.size() * scale, Qt::KeepAspectRatio, Qt::FastTransformation);
	if (!out.save(argv[2])) {
		fprintf(stderr, "cannot write %s\n", argv[2]);
		return 1;
	}
	printf("wrote %s (%dx%d)\n", argv[2], out.width(), out.height());
	return 0;
}
