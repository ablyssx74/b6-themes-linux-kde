#ifndef B6_DECORATION_H
#define B6_DECORATION_H

#include "FrameRenderer.h"
#include "XfwmTheme.h"

#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButton>

#include <QVariantList>

namespace B6 {

// A button only has a place and a click: the picture is drawn with the rest of the frame, from the theme.
class DecorationButton : public KDecoration3::DecorationButton {
	Q_OBJECT
public:
	DecorationButton(KDecoration3::DecorationButtonType type, KDecoration3::Decoration* decoration, QObject* parent = nullptr);
	void paint(QPainter*, const QRectF&) override {}
};

class Decoration : public KDecoration3::Decoration {
	Q_OBJECT
public:
	explicit Decoration(QObject* parent = nullptr, const QVariantList& args = QVariantList());
	void paint(QPainter* painter, const QRectF& repaintArea) override;

protected:
	bool init() override;

private slots:
	void updateLayout();

private:
	static QString themeDirectory();
	FrameParams params() const;
	DecorationButton* buttonFor(Button button) const;

	XfwmTheme fTheme;
	FrameLayout fLayout;
	DecorationButton* fButtons[ButtonCount] = {};
};

}	// namespace B6

#endif
