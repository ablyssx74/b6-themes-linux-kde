#include "B6Decoration.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>
#include <KPluginFactory>

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QStandardPaths>

K_PLUGIN_FACTORY_WITH_JSON(B6Factory, "b6decoration.json", registerPlugin<B6::Decoration>();)

namespace B6 {

using KDecoration3::DecorationButtonType;

DecorationButton::DecorationButton(DecorationButtonType type, KDecoration3::Decoration* decoration, QObject* parent)
	: KDecoration3::DecorationButton(type, decoration, parent)
{
}

Decoration::Decoration(QObject* parent, const QVariantList& args)
	: KDecoration3::Decoration(parent, args)
{
}

// The folder of the theme: B6_THEME_DIR when it holds a theme, else the data folders (~/.local/share, /usr/share).
QString Decoration::themeDirectory()
{
	const QString fromEnvironment = QString::fromLocal8Bit(qgetenv("B6_THEME_DIR"));
	if (!fromEnvironment.isEmpty() && QFileInfo::exists(fromEnvironment + "/themerc"))
		return fromEnvironment;
	return QStandardPaths::locate(QStandardPaths::GenericDataLocation, "b6-themes/b6+r", QStandardPaths::LocateDirectory);
}

bool Decoration::init()
{
	const QString directory = themeDirectory();
	if (directory.isEmpty() || !fTheme.load(directory)) {
		qWarning("b6xfwm: no theme found (B6_THEME_DIR=\"%s\", and none in the data folders under b6-themes/b6+r)",
			qgetenv("B6_THEME_DIR").constData());
		return false;
	}

	setOpaque(false);

	// the three buttons the theme asks for (b6+r: close at the left end of the tab, zoom at its right end)
	auto make = [this](Button button, DecorationButtonType type) {
		fButtons[button] = new DecorationButton(type, this, this);
		connect(fButtons[button], &KDecoration3::DecorationButton::hoveredChanged, this, [this]() { update(); });
		connect(fButtons[button], &KDecoration3::DecorationButton::pressedChanged, this, [this]() { update(); });
	};
	make(Close, DecorationButtonType::Close);
	make(Maximize, DecorationButtonType::Maximize);
	make(Hide, DecorationButtonType::Minimize);

	KDecoration3::DecoratedWindow* w = window();
	connect(w, &KDecoration3::DecoratedWindow::activeChanged, this, [this]() { update(); });
	connect(w, &KDecoration3::DecoratedWindow::captionChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::sizeChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::closeableChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::maximizeableChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::minimizeableChanged, this, &Decoration::updateLayout);
	connect(settings().get(), &KDecoration3::DecorationSettings::fontChanged, this, &Decoration::updateLayout);

	updateLayout();
	return true;
}

FrameParams Decoration::params() const
{
	KDecoration3::DecoratedWindow* w = window();
	FrameParams p;
	p.clientSize = QSizeF(w->size()).toSize();
	p.active = w->isActive();
	p.title = w->caption();
	p.font = settings()->font();
	p.hasClose = w->isCloseable();
	p.hasMaximize = w->isMaximizeable();
	p.hasHide = w->isMinimizeable();
	p.hovered = p.pressed = -1;
	for (int b = 0; b < ButtonCount; b++) {
		if (fButtons[b] && fButtons[b]->isHovered())
			p.hovered = b;
		if (fButtons[b] && fButtons[b]->isPressed())
			p.pressed = b;
	}
	return p;
}

void Decoration::updateLayout()
{
	fLayout = computeLayout(fTheme, params());
	setBorders(QMarginsF(fLayout.borders));
	setTitleBar(QRectF(fLayout.titleBar));
	for (int b = 0; b < ButtonCount; b++) {
		if (!fButtons[b])
			continue;
		const QRect rect = fLayout.buttons[b];
		fButtons[b]->setVisible(!rect.isEmpty());
		fButtons[b]->setGeometry(QRectF(fLayout.toDecoration(rect)));
	}
	update();
}

void Decoration::paint(QPainter* painter, const QRectF&)
{
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	paintFrame(*painter, fTheme, params(), fLayout);
}

}	// namespace B6

#include "B6Decoration.moc"
