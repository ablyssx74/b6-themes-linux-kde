#include "XfwmTheme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QHash>
#include <QTextStream>

namespace B6 {

static QRect opaqueBounds(const QImage& image)
{
	int left = image.width(), top = image.height(), right = -1, bottom = -1;
	for (int y = 0; y < image.height(); y++) {
		const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
		for (int x = 0; x < image.width(); x++) {
			if (qAlpha(row[x]) == 0)
				continue;
			left = qMin(left, x);
			right = qMax(right, x);
			top = qMin(top, y);
			bottom = qMax(bottom, y);
		}
	}
	return right < 0 ? QRect() : QRect(left, top, right - left + 1, bottom - top + 1);
}

static bool darkColor(const QColor& color)
{
	return color.isValid() && color.lightnessF() < 0.5;
}

// An XPM names some of its colors ("c #EEEEE6 s active_color_2"): the window color, its highlight, its mid tone and its shadow.
// The shades keep their distance from the window color, scaled to the user's own.
void XfwmTheme::recolor(QImage& image, const QString& path) const
{
	if (!darkColor(fWindow) || !fReference)
		return;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	static const QRegularExpression named("c #([0-9A-Fa-f]{6}) s [a-z]+_(color|hilight|mid|shadow)_");
	QHash<QRgb, QRgb> map;
	const QRgb base = fWindow.rgb();
	const QRgb reference = fReference;
	QTextStream stream(&file);
	while (!stream.atEnd()) {
		const auto match = named.match(stream.readLine());
		if (!match.hasMatch())
			continue;
		const QRgb original = QColor("#" + match.captured(1)).rgb();
		auto channel = [](int o, int r, int b) {
			if (o <= r)
				return r > 0 ? b * o / r : b;
			return b + (255 - b) * (o - r) / (255 - r);
		};
		map[original] = qRgb(channel(qRed(original), qRed(reference), qRed(base)),
			channel(qGreen(original), qGreen(reference), qGreen(base)),
			channel(qBlue(original), qBlue(reference), qBlue(base)));
	}
	if (map.isEmpty())
		return;
	image = image.convertToFormat(QImage::Format_ARGB32);
	for (int y = 0; y < image.height(); y++) {
		QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
		for (int x = 0; x < image.width(); x++) {
			const auto found = map.constFind(row[x] | 0xff000000);
			if (found != map.constEnd() && qAlpha(row[x]) == 255)
				row[x] = found.value();
		}
	}
}

bool XfwmTheme::loadPiece(Piece& piece, const QString& baseName) const
{
	// xfwm4 takes a picture in either format; the PNG, when there is one, is the newer
	for (const char* extension : {".png", ".xpm"}) {
		const QString path = fDirectory + "/" + baseName + extension;
		if (!QFile::exists(path))
			continue;
		QImage image(path);
		if (image.isNull())
			continue;
		if (extension[1] == 'x')
			recolor(image, path);
		piece.image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		QImage straight = image.convertToFormat(QImage::Format_ARGB32);
		piece.opaque = opaqueBounds(straight);
		return true;
	}
	return false;
}

bool XfwmTheme::load(const QString& directory)
{
	fValid = false;
	fDirectory = directory;
	fName = QFileInfo(directory).fileName();
	if (!QFileInfo(directory + "/themerc").exists())
		return false;
	readThemerc(directory + "/themerc");
	if (darkColor(fWindow)) {
		fReference = 0;
		static const QRegularExpression color2("c #([0-9A-Fa-f]{6}) s active_color_2");
		const QFileInfoList files = QDir(directory).entryInfoList({"*.xpm"}, QDir::Files);
		for (const QFileInfo& info : files) {
			QFile file(info.filePath());
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				continue;
			const auto match = color2.match(QString::fromLatin1(file.readAll()));
			if (match.hasMatch()) {
				fReference = QColor("#" + match.captured(1)).rgb();
				break;
			}
		}
	}

	static const char* const states[2] = {"active", "inactive"};
	for (int s = 0; s < 2; s++) {
		const QString state = states[s];
		loadPiece(fTopLeft[s], "top-left-" + state);
		loadPiece(fTopRight[s], "top-right-" + state);
		for (int i = 0; i < 5; i++)
			loadPiece(fTitle[i][s], QString("title-%1-%2").arg(i + 1).arg(state));
		loadPiece(fLeft[s], "left-" + state);
		loadPiece(fRight[s], "right-" + state);
		loadPiece(fBottom[s], "bottom-" + state);
		loadPiece(fBottomLeft[s], "bottom-left-" + state);
		loadPiece(fBottomRight[s], "bottom-right-" + state);
	}

	static const char* const names[ButtonCount] = {"close", "maximize", "hide", "shade", "menu", "stick"};
	static const char* const suffixes[StateCount] = {"-active", "-inactive", "-prelight", "-prelight-inactive", "-pressed"};
	for (int b = 0; b < ButtonCount; b++) {
		for (int s = 0; s < StateCount; s++) {
			loadPiece(fButton[b][s], QString(names[b]) + suffixes[s]);
			loadPiece(fButtonToggled[b][s], QString(names[b]) + "-toggled" + suffixes[s]);
		}
		// a theme without a picture for a state uses the one of the plain state
		for (int s = Inactive; s < StateCount; s++) {
			if (fButton[b][s].isNull())
				fButton[b][s] = fButton[b][s == PrelightInactive ? Inactive : Active];
			if (fButtonToggled[b][s].isNull() && !fButtonToggled[b][Active].isNull())
				fButtonToggled[b][s] = fButtonToggled[b][s == PrelightInactive ? Inactive : Active];
		}
	}

	// no title tab pictures, no theme
	if (fTitle[2][0].isNull() || fLeft[0].isNull() || fRight[0].isNull() || fBottom[0].isNull())
		return false;
	fTitleHeight = fTitle[2][0].height();
	if (!fButton[Close][Active].isNull())
		fButtonWidth = fButton[Close][Active].width();
	fValid = true;
	return true;
}

void XfwmTheme::readThemerc(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	QTextStream stream(&file);
	while (!stream.atEnd()) {
		const QString line = stream.readLine().trimmed();
		const int equals = line.indexOf('=');
		if (line.startsWith('#') || equals < 0)
			continue;
		const QString key = line.left(equals).trimmed();
		const QString value = line.mid(equals + 1).trimmed();
		const bool truth = value == "true";
		if (key == "active_text_color")
			fActiveText = QColor(value);
		else if (key == "inactive_text_color")
			fInactiveText = QColor(value);
		else if (key == "title_vertical_offset_active")
			fOffsetActive = value.toInt();
		else if (key == "title_vertical_offset_inactive")
			fOffsetInactive = value.toInt();
		else if (key == "title_horizontal_offset")
			fTitleOffsetX = value.toInt();
		else if (key == "title_alignment")
			fAlignment = value == "left" ? 0 : (value == "right" ? 2 : 1);
		else if (key == "full_width_title")
			fFullWidth = truth;
		else if (key == "button_layout")
			fButtonLayout = value;
		else if (key == "button_offset")
			fButtonOffset = value.toInt();
		else if (key == "button_offset_right") {
			fButtonOffsetRight = value.toInt();
			fHasOffsetRight = true;
		} else if (key == "button_spacing")
			fButtonSpacing = value.toInt();
		else if (key == "buttons_in_tab")
			fButtonsInTab = truth;
	}
}

}	// namespace B6
