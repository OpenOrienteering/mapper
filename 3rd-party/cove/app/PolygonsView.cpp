/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 * Copyright 2020 Kai Pastor
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PolygonsView.h"

#include <memory>
#include <utility>

#include <Qt>
#include <QBrush>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>

#include "libvectorizer/Polygons.h"

#include "QImageView.h"

class QPaintEvent;
class QWidget;

namespace cove {
//@{
//! \ingroup gui

/*! \class PolyImageWidget
 * \brief ImageWidget drawing \a PolygonList above the image.
 *
 * Helper class for PolygonsView.
 */

PolyImageWidget::PolyImageWidget(QWidget* parent)
	: ImageWidget(parent)
{
}

PolyImageWidget::~PolyImageWidget() = default;

/*! Return currently set PolygonList.  \sa setPolygons(PolygonList& p)
  */
const PolygonList& PolyImageWidget::polygons() const
{
	return polygonsList;
}

/*! What PolygonList to draw.
 * \bug Behaves differently from setImage().  Creates its own copy of
 * PolygonList and polygons() then returns another copy.
 */
void PolyImageWidget::setPolygons(PolygonList p)
{
	polygonsList = std::move(p);
	update();
}

/*! paintEvent called by Qt engine, calls inherited ImageWidget::paintEvent and
 * then draws lines. */
void PolyImageWidget::paintEvent(QPaintEvent* pe)
{
	ImageWidget::paintEvent(pe);
	if (!dispRealPaintEnabled) return;

	if (polygonsList.empty()) return;

	auto const pen = QPen(Qt::cyan);
	auto const brush = QBrush(Qt::red);
	auto const marker = QRectF(QPointF(-1.5, -1.5) / dispMagnification,
	                           QSizeF(3.0, 3.0) / dispMagnification);

	auto const event_rect = pe->rect();
	auto const event_rectf = QRectF(
	                             event_rect.left() / dispMagnification + marker.left(),
	                             event_rect.top() / dispMagnification + marker.top(),
	                             event_rect.width() / dispMagnification + marker.width(),
	                             event_rect.height() / dispMagnification + marker.height());

	QPainter p(this);
	p.translate(dispMagnification / 2, dispMagnification / 2);  // offset for aliased painting
	p.scale(dispMagnification, dispMagnification);

	for (auto const& polygon : polygonsList)
	{
		// when polygon does not interfere with current repainted area, skip it
		if (!event_rectf.intersects(polygon.boundingRect()))
			continue;

		// draw line
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
		if (polygon.isClosed())
			p.drawPolygon(polygon.data(), int(polygon.size()));
		else
			p.drawPolyline(polygon.data(), int(polygon.size()));

		// draw squares
		p.setPen(Qt::NoPen);
		p.setBrush(brush);
		for (auto const& pt : polygon)
		{
			if (event_rectf.contains(pt))
				p.drawRect(QRectF(pt + marker.topLeft(), marker.size()));
		}
	}
}

/*! \class PolygonsView
 * \brief Provides scrollable view of an QImage with vectors over it.
 */

/*! Default constructor.
 */
PolygonsView::PolygonsView(QWidget* parent)
	: QImageView(parent)
{
	iw = std::make_unique<PolyImageWidget>();
	setWidget(iw.get());
}

PolygonsView::~PolygonsView() = default;

/*! Gets the polygonList that is drawn over the image.
 * \sa setPolygons(PolygonList* p)
 */
const PolygonList& PolygonsView::polygons() const
{
	return static_cast<PolyImageWidget*>(iw.get())->polygons();
}

/*! Sets the polygonList that is drawn over the image.
 */
void PolygonsView::setPolygons(PolygonList p)
{
	static_cast<PolyImageWidget*>(iw.get())->setPolygons(std::move(p));
}


}  // namespace cove

//@}
