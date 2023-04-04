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

#ifndef OPENORIENTEERING_COLOR_WHEEL_WIDGET_H
#define OPENORIENTEERING_COLOR_WHEEL_WIDGET_H

#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QImage>
#include <QLineF>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

class QLineEdit;
class QLineF;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPoint;
class QPointF;
class QResizeEvent;

namespace OpenOrienteering {


/**
 * \brief Display an analog widget that allows the selection of a HSV color
 *
 * It has an outer wheel to select the Hue and an internal square to select
 * Saturation and Lightness.
 */
class ColorWheel : public QWidget
{
	Q_OBJECT
public:
	explicit ColorWheel(QWidget* parent = 0);
	~ColorWheel();

	/// Get current color
	QColor color() const;

public slots:

	/// Set current color
	void setColor(QColor c);

signals:
	/**
	 * Emitted when the user selects a color or setColor is called
	 */
	void colorChanged(QColor);

	/**
	 * Emitted when the user selects a color
	 */
	void colorSelected(QColor);

	/**
	 * Emitted when the user releases from dragging
	 */
	void editingFinished();

protected:
	enum MouseStatus
	{
		Nothing,
		DragCircle,
		DragInside
	};

	qreal hue = {};
	qreal sat = {};
	qreal val = {};
	MouseStatus mouse_status = Nothing;
	bool background_is_dark = {};

	unsigned int wheel_width = 20;
	static constexpr qreal selector_radius = 6;

	QPixmap hue_ring;
	QImage inner_selector;

	void paintEvent(QPaintEvent*event) override;
	void mouseMoveEvent(QMouseEvent*event) override;
	void mousePressEvent(QMouseEvent*event) override;
	void mouseReleaseEvent(QMouseEvent*event) override;
	void resizeEvent(QResizeEvent*event) override;

	void renderHueRing();
	void renderValSatTriangle();
	qreal outerRadius() const;
	void drawRingEditor(double editor_hue, QPainter& painter, QColor color);
	qreal innerRadius() const;
	QLineF lineToPoint(const QPoint& p) const;
	qreal selectorImageAngle() const;
	QPointF selectorImageOffset();
	qreal triangleSide() const;
	qreal triangleHeight() const;
};


/**
 * \brief A helper widget that displays chosen color on its face.
 *
 * Intended as a color preview area in ColorWheelDialog.
 */
class ColorPreview : public QFrame
{
	Q_OBJECT

public:
	ColorPreview(QWidget* parent = nullptr);
	virtual void paintEvent(QPaintEvent* event) override;
	virtual QSize sizeHint() const override;
	QColor color();

public slots:
	void setColor(QColor c);

private:
	QColor display_color;
};


/**
 * \brief A minimalistic color selection dialog intended for on mobile devices.
 *
 * It offers a color wheel, color preview in a rectangle and input field for
 * a direct text entry. The programming interface tries to mimic QColorDialog.
 */
class ColorWheelDialog : public QDialog
{
	Q_OBJECT
public:
	ColorWheelDialog(QWidget* parent = nullptr,
	                 Qt::WindowFlags f = Qt::WindowFlags());
	QColor currentColor() const;
	void setCurrentColor(const QColor& c);
	static QColor getColor(const QColor& initial, QWidget* parent = nullptr);

private:
	ColorWheel* wheel = {};
	QLineEdit* text_edit = {};
};


} //  namespace OpenOrienteering

#endif
