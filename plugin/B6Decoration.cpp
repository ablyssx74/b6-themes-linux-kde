#include "B6Decoration.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>
#include <KPluginFactory>

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QStandardPaths>

K_PLUGIN_FACTORY_WITH_JSON(B6Factory, "b6decoration.json", registerPlugin<B6::Decoration>(); registerPlugin<B6::ThemeProvider>();)

namespace B6 {

using KDecoration3::DecorationButtonType;

static const char kPluginId[] = "org.kde.b6xfwm";

DecorationButton::DecorationButton(DecorationButtonType type, KDecoration3::Decoration* decoration, QObject* parent)
	: KDecoration3::DecorationButton(type, decoration, parent)
{
}

// the folders that hold themes: B6_THEMES_PATH (a colon separated list, for trying a theme without installing it), then the data
// folders (~/.local/share/b6-themes, /usr/share/b6-themes)
static QStringList themeRoots()
{
	QStringList roots = QString::fromLocal8Bit(qgetenv("B6_THEMES_PATH")).split(':', Qt::SkipEmptyParts);
	roots += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "b6-themes", QStandardPaths::LocateDirectory);
	return roots;
}

QStringList Decoration::themeNames()
{
	QStringList names;
	for (const QString& root : themeRoots()) {
		for (const QString& entry : QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
			if (QFileInfo::exists(root + "/" + entry + "/themerc") && !names.contains(entry))
				names << entry;
		}
	}
	return names;
}

QString Decoration::themeDirectory(const QString& name)
{
	for (const QString& root : themeRoots()) {
		if (QFileInfo::exists(root + "/" + name + "/themerc"))
			return root + "/" + name;
	}
	return QString();
}

ThemeProvider::ThemeProvider(QObject* parent, const QVariantList&)
	: KDecoration3::DecorationThemeProvider(parent)
{
}

QList<KDecoration3::DecorationThemeMetaData> ThemeProvider::themes() const
{
	QList<KDecoration3::DecorationThemeMetaData> themes;
	for (const QString& name : Decoration::themeNames()) {
		KDecoration3::DecorationThemeMetaData data;
		data.setPluginId(kPluginId);
		data.setThemeName(name);
		data.setVisibleName("xfwm4: " + name);
		data.setBorderSize(KDecoration3::BorderSize::None);
		themes << data;
	}
	return themes;
}

Decoration::Decoration(QObject* parent, const QVariantList& args)
	: KDecoration3::Decoration(parent, args)
{
	// the theme the user picked arrives as {"theme": name}
	for (const QVariant& arg : args) {
		const QString theme = arg.toMap().value("theme").toString();
		if (!theme.isEmpty())
			fThemeName = theme;
	}
}

bool Decoration::init()
{
	// a name from an old setting (or none at all) falls back to the first theme there is
	QString directory = themeDirectory(fThemeName);
	if (directory.isEmpty()) {
		const QStringList names = themeNames();
		if (!names.isEmpty()) {
			fThemeName = names.contains("b6+r") ? QString("b6+r") : names.first();
			directory = themeDirectory(fThemeName);
		}
	}
	fTheme.setWindowColor(window()->color(QPalette::Active, QPalette::Window));
	if (directory.isEmpty() || !fTheme.load(directory)) {
		qWarning("b6xfwm: no theme \"%s\" found in the data folders (b6-themes/<name>/themerc) or in B6_THEMES_PATH",
			qPrintable(fThemeName));
		return false;
	}

	setOpaque(false);

	auto make = [this](Button button, DecorationButtonType type) {
		fButtons[button] = new DecorationButton(type, this, this);
		connect(fButtons[button], &KDecoration3::DecorationButton::hoveredChanged, this, [this]() { update(); });
		connect(fButtons[button], &KDecoration3::DecorationButton::pressedChanged, this, [this]() { update(); });
	};
	make(Close, DecorationButtonType::Close);
	make(Maximize, DecorationButtonType::Maximize);
	make(Hide, DecorationButtonType::Minimize);
	make(Shade, DecorationButtonType::Shade);
	make(Menu, DecorationButtonType::Menu);
	make(Stick, DecorationButtonType::OnAllDesktops);

	KDecoration3::DecoratedWindow* w = window();
	connect(w, &KDecoration3::DecoratedWindow::activeChanged, this, [this]() { update(); });
	connect(w, &KDecoration3::DecoratedWindow::paletteChanged, this, [this]() {
		fTheme.setWindowColor(window()->color(QPalette::Active, QPalette::Window));
		fTheme.load(themeDirectory(fThemeName));
		update();
	});
	connect(w, &KDecoration3::DecoratedWindow::captionChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::sizeChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::shadedChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::onAllDesktopsChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::closeableChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::maximizeableChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::minimizeableChanged, this, &Decoration::updateLayout);
	connect(w, &KDecoration3::DecoratedWindow::shadeableChanged, this, &Decoration::updateLayout);
	connect(settings().get(), &KDecoration3::DecorationSettings::fontChanged, this, &Decoration::updateLayout);
	connect(settings().get(), &KDecoration3::DecorationSettings::onAllDesktopsAvailableChanged, this, &Decoration::updateLayout);

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
	p.maximized = w->isMaximized();
	p.has[Close] = w->isCloseable();
	p.has[Maximize] = w->isMaximizeable();
	p.has[Hide] = w->isMinimizeable();
	p.has[Shade] = w->isShadeable();
	p.has[Menu] = true;
	p.has[Stick] = settings()->isOnAllDesktopsAvailable();
	p.toggled[Maximize] = w->isMaximized();
	p.toggled[Shade] = w->isShaded();
	p.toggled[Stick] = w->isOnAllDesktops();
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
	setBorders(QMarginsF(frameBorders(fTheme, params())));
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
