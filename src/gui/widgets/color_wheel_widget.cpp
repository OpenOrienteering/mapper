/*
 *    Copyright (C) 2013-2020 Mattia Basaglia
 *    Copyright 2021 Libor Pecháček
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "color_wheel_widget.h"

#include <QAbstractButton>
#include <QBrush>
#include <QConicalGradient>
#include <QFlags>
#include <QGradientStops>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPoint>
#include <QPolygonF>
#include <QPushButton>
#include <QRect>
#include <QRectF>
#include <QRgb>
#include <QSizeF>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <QtGlobal>
#include <QtMath>

class QPaintEvent;
class QResizeEvent;


namespace OpenOrienteering {


ColorWheel::ColorWheel(QWidget* parent)
    : QWidget(parent)
{
	background_is_dark = palette().window().color().valueF() < 0.5;
}


ColorWheel::~ColorWheel() = default;


// Get the user selected color
QColor ColorWheel::color() const
{
	return QColor::fromHsvF(hue, sat, val);
}


// Set the widget selection color
void ColorWheel::setColor(QColor c)
{
	auto oldh = hue;
	// render red triangle for achromatic colors
	hue = c.hsvHueF() >= 0 ? c.hsvHueF() : 0;
	sat = c.hsvSaturationF();
	val = c.valueF();

	if (!qFuzzyCompare(oldh + 1, hue + 1))
		renderValSatTriangle();
	
	update();
	emit colorChanged(c);
}


void ColorWheel::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.translate(geometry().width() / 2, geometry().height() / 2);

	// hue wheel
	if (hue_ring.isNull())
		renderHueRing();

	painter.drawPixmap(-outerRadius(), -outerRadius(), hue_ring);

	// hue selector
	drawRingEditor(hue, painter, Qt::black);

	// lum-sat triangle
	if (inner_selector.isNull())
		renderValSatTriangle();

	qreal side = triangleSide();
	qreal height = triangleHeight();

	painter.rotate(selectorImageAngle());
	painter.translate(selectorImageOffset());

	qreal slice_h = side * val;
	qreal ymin = side / 2 - slice_h / 2;

	// clip with the triangular shape
	auto clip = QPainterPath();
	clip.addPolygon(QPolygonF(QVector<QPointF> { {0, side / 2},
	                                             {height, 0},
	                                             {height, side} }));
	painter.setClipPath(clip);

	painter.drawImage(QRectF(QPointF(0, 0), QPointF(height, side)),
	                  inner_selector);
	painter.setClipping(false);

	// lum-sat selector
	// we define the color of the selecto based on the background color of the
	// widget in order to improve to contrast

	auto const selector_position = QPointF(val * height, ymin + sat * slice_h);
	auto const selector_ring_color = (background_is_dark && (val < 0.65 || sat > 0.43))
	                           || val <= 0.5 ? Qt::white : Qt::black;
	painter.setPen(QPen(selector_ring_color, 3));
	painter.setBrush(Qt::NoBrush);
	painter.drawEllipse(selector_position, selector_radius, selector_radius);
}


void ColorWheel::mouseMoveEvent(QMouseEvent* event)
{
	if (mouse_status == DragCircle)
	{
		hue = lineToPoint(event->pos()).angle() / 360.0;
		renderValSatTriangle();
	}
	else if (mouse_status == DragInside)
	{
		QLineF glob_mouse_ln = lineToPoint(event->pos());
		QLineF center_mouse_ln(QPointF(0, 0),
		                       glob_mouse_ln.p2() - glob_mouse_ln.p1());

		center_mouse_ln.setAngle(center_mouse_ln.angle() + selectorImageAngle());
		center_mouse_ln.setP2(center_mouse_ln.p2() - selectorImageOffset());

		QPointF pt = center_mouse_ln.p2();

		qreal side = triangleSide();
		val = qBound(0.0, pt.x() / triangleHeight(), 1.0);
		qreal slice_h = side * val;

		qreal ycenter = side / 2;
		qreal ymin = ycenter - slice_h / 2;

		if (slice_h > 0) sat = qBound(0.0, (pt.y() - ymin) / slice_h, 1.0);
	}
	else
	{
		return;
	}

	emit colorSelected(color());
	emit colorChanged(color());
	update();
}

void ColorWheel::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		setFocus();
		QLineF ray = lineToPoint(event->pos());
		if (ray.length() <= innerRadius())
			mouse_status = DragInside;
		else if (ray.length() <= outerRadius())
			mouse_status = DragCircle;

		// Update the color
		mouseMoveEvent(event);
	}
}

void ColorWheel::mouseReleaseEvent(QMouseEvent* event)
{
	mouseMoveEvent(event);
	mouse_status = Nothing;
	if (event->button() == Qt::LeftButton) emit editingFinished();
}


void ColorWheel::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event)

	wheel_width = qMax(outerRadius() * 0.08, 10.0);

	renderHueRing();
	renderValSatTriangle();
}


/// Update the outer ring that displays the hue selector
void ColorWheel::renderHueRing()
{
	hue_ring = QPixmap(outerRadius() * 2, outerRadius() * 2);
	hue_ring.fill(Qt::transparent);
	QPainter painter(&hue_ring);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_Source);

	const int hue_stops = 24;
	QConicalGradient gradient_hue(0, 0, 0);
	if (gradient_hue.stops().size() < hue_stops)
	{
		for (auto counter = 0U; counter < hue_stops; ++counter)
		{
			auto a = double(counter) / (hue_stops - 1);
			gradient_hue.setColorAt(a, QColor::fromHsvF(a, 1, 1));
		}
	}

	painter.translate(outerRadius(), outerRadius());

	painter.setPen(Qt::NoPen);
	painter.setBrush(QBrush(gradient_hue));
	painter.drawEllipse(QPointF(0, 0), outerRadius(), outerRadius());

	painter.setBrush(Qt::transparent);
	painter.drawEllipse(QPointF(0, 0), innerRadius(), innerRadius());
}

/**
 * \brief renders the selector as a triangle
 * \note It's the same as a square with the edge with value=0 collapsed to a
 * single point
 */
void ColorWheel::renderValSatTriangle()
{
	auto size = QSizeF(triangleHeight(), triangleSide()); // selector size
	auto max_size = 128;

	if (size.height() > max_size) size *= max_size / size.height();

	qreal ycenter = size.height() / 2;

	QSize isize = size.toSize();
	inner_selector = QImage(isize, QImage::Format_RGB32);

	for (int x = 0; x < isize.width(); x++)
	{
		qreal pval = x / size.height();
		qreal slice_h = size.height() * pval;
		for (int y = 0; y < isize.height(); y++)
		{
			qreal ymin = ycenter - slice_h / 2;
			qreal psat = qBound(0.0, (y - ymin) / slice_h, 1.0);
			auto color = QColor::fromHsvF(hue, psat, pval);
			inner_selector.setPixel(x, y, color.rgb());
		}
	}
}


/// Calculate outer wheel radius from widget center
qreal ColorWheel::outerRadius() const
{
	auto radius = qMin(geometry().width(), geometry().height()) / 2;
	return radius;
}


/// Calculate inner wheel radius from widget center
qreal ColorWheel::innerRadius() const
{
	return outerRadius() - wheel_width;
}


/// Draw the outer color wheel marker
void ColorWheel::drawRingEditor(double editor_hue, QPainter& painter,
                                  QColor color)
{
	painter.setPen(QPen(color, 3));
	painter.setBrush(Qt::NoBrush);
	QLineF ray(0, 0, outerRadius(), 0);
	ray.setAngle(editor_hue * 360);
	QPointF h1 = ray.p2();
	ray.setLength(innerRadius());
	QPointF h2 = ray.p2();
	painter.drawLine(h1, h2);
}

/// Construct line from the center to a given point
QLineF ColorWheel::lineToPoint(const QPoint& p) const
{
	return QLineF(geometry().width() / 2, geometry().height() / 2,
	              p.x(), p.y());
}

/// Rotation of the selector image
qreal ColorWheel::selectorImageAngle() const
{
	return -hue * 360 - 60;
}

/// Offset of the selector image
QPointF ColorWheel::selectorImageOffset()
{
	return QPointF(-innerRadius(), -triangleSide() / 2);
}

/// Calculate the side of the inner triangle
qreal ColorWheel::triangleSide() const
{
	return innerRadius() * qSqrt(3);
}

/// Calculate the height of the inner triangle
qreal ColorWheel::triangleHeight() const
{
	return innerRadius() * 3 / 2;
}


ColorPreview::ColorPreview(QWidget* parent)
    : QFrame(parent)
{
	setFrameStyle(QFrame::Panel | QFrame::Plain);
	setLineWidth(2);
}


/// Get widget face color
QColor ColorPreview::color()
{
	return display_color;
}


/// Set widget face color
void ColorPreview::setColor(QColor c)
{
	display_color = c;
	update();
}


void ColorPreview::paintEvent(QPaintEvent* event)
{
	QFrame::paintEvent(event);

	QPainter painter(this);
	painter.fillRect(contentsRect(), QBrush(display_color));
}


QSize ColorPreview::sizeHint() const
{
	return QSize(24, 24);
}


ColorWheelDialog::ColorWheelDialog(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
	auto* main_layout = new QVBoxLayout();
	wheel = new ColorWheel();
	main_layout->addWidget(wheel);
	auto* mid_layout = new QHBoxLayout();
	auto* color_preview = new ColorPreview();
	color_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	mid_layout->addWidget(color_preview);
	text_edit = new QLineEdit();
	text_edit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	mid_layout->addWidget(text_edit);
	main_layout->addLayout(mid_layout);
	auto* ok_button = new QPushButton(tr("Select"));
	ok_button->setDefault(true);
	main_layout->addWidget(ok_button);
	setLayout(main_layout);

	connect(wheel, &ColorWheel::colorSelected, this, [this](QColor c) {
		text_edit->setText(c.name().right(6).toUpper());
	});
	connect(text_edit, &QLineEdit::textEdited, this,
	        [this](const QString& text) {
		        const auto c = QRgb(text.toUInt(nullptr, 16));
				wheel->setColor(c);
	        });
	connect(wheel, &ColorWheel::colorChanged, color_preview,
	        &ColorPreview::setColor);
	connect(ok_button, &QPushButton::clicked, this, &ColorWheelDialog::accept);
}


QColor ColorWheelDialog::currentColor() const
{
	return result() == QDialog::Accepted ? wheel->color() : QColor();
}


void ColorWheelDialog::setCurrentColor(const QColor& c)
{
	wheel->setColor(c);
	text_edit->setText(c.name().right(6).toUpper());
	// color preview receives the update from the color wheel
}


QColor ColorWheelDialog::getColor(const QColor& initial, QWidget* parent)
{
	ColorWheelDialog dlg(parent);
	dlg.setCurrentColor(initial);
	dlg.setWindowState((dlg.windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen)) 
	               | Qt::WindowMaximized);
	dlg.exec();
	return dlg.currentColor();
}


} //  namespace OpenOrienteering
