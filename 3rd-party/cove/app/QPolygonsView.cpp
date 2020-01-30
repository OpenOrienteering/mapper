/*
 * Copyright (c) 2005-2019 Libor Pecháček.
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

#include "QPolygonsView.h"

#include <iterator>
#include <memory>

#include <Qt>
#include <QBrush>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>

#include "libvectorizer/Polygons.h"

#include "QImageView.h"

class QPaintEvent;
class QWidget;

namespace cove {
//@{
//! \ingroup gui

/*! \class PaintablePolygonList
  \brief PolygonList holding the QPainterPaths with the polygons. */

PaintablePolygonList::PaintablePolygonList() = default;

PaintablePolygonList::PaintablePolygonList(const PolygonList& pl)
{
	*this = pl;
}

//! Assignment operator, resizes paths vector and assigns empty paths to the
//! elements.
PaintablePolygonList& PaintablePolygonList::operator=(const PolygonList& pl)
{
	*static_cast<PolygonList*>(this) = pl;
	painterPaths.resize(pl.size());
	for (auto& i : painterPaths)
		i = QPainterPath();
	return *this;
}

const QPainterPath& PaintablePolygonList::getConstPainterPath(
	const Polygons::PolygonList::iterator& it)
{
	int d = distance(begin(), it);
	return painterPaths[d];
}

void PaintablePolygonList::setConstPainterPath(
	const Polygons::PolygonList::iterator& it, const QPainterPath& pa)
{
	int d = distance(begin(), it);
	painterPaths[d] = pa;
}

/*! \class PolyImageWidget
 * \brief ImageWidget drawing \a Polygons::PolygonList above the image.
 *
 * Helper class for QPolygonsView.
 */

PolyImageWidget::PolyImageWidget(QWidget* parent)
	: ImageWidget(parent)
{
}

/*! Return currently set PolygonList.  \sa setPolygons(Polygons::PolygonList& p)
  */
const Polygons::PolygonList& PolyImageWidget::polygons() const
{
	return polygonsList;
}

/*! What PolygonList to draw.
  \bug Behaves differently from setImage().  Creates its own copy of
  PolygonList and polygons() then returns another copy.
  */
void PolyImageWidget::setPolygons(const Polygons::PolygonList& p)
{
	polygonsList = p;

	update();
}

/*! paintEvent called by Qt engine, calls inherited ImageWidget::paintEvent and
 * then draws lines. */
void PolyImageWidget::paintEvent(QPaintEvent* pe)
{
	ImageWidget::paintEvent(pe);
	if (!dispRealPaintEnabled) return;

	if (polygonsList.empty()) return;

	QPainter p(this);
	QRect r = pe->rect();

	for (Polygons::PolygonList::iterator i = polygonsList.begin();
		 i != polygonsList.end(); i++)
	{
		// when polygon does not interfere with current repainted area, skip it
		QRect polyBoundRect =
			QRect(i->boundingRect().topLeft() * dispMagnification,
				  i->boundingRect().bottomRight() * dispMagnification);
		if (!r.intersects(polyBoundRect)) continue;

		p.save();
		p.scale(dispMagnification, dispMagnification);
		p.setPen(QPen(Qt::cyan));
		const QPainterPath& pp = polygonsList.getConstPainterPath(i);
		if (pp.isEmpty())
		{
			QPainterPath newPP;

			Polygon::const_iterator j = i->begin();
			newPP.moveTo(i->begin()->x() + 0.5, i->begin()->y() + 0.5);
			for (; j != i->end(); ++j)
			{
				newPP.lineTo(j->x() + 0.5, j->y() + 0.5);
			}

			if (i->isClosed())
			{
				newPP.closeSubpath();
			}

			polygonsList.setConstPainterPath(i, newPP);
			p.drawPath(newPP);
		}
		else
		{
			p.drawPath(pp);
		}

		p.restore();
		// draw squares
		QBrush brushNormal(Qt::red);
		for (Polygon::const_iterator j = i->begin(); j != i->end();
			 ++j)
		{
			QPoint pt(int((j->x() + 0.5) * dispMagnification),
			          int((j->y() + 0.5) * dispMagnification));
			if (r.contains(pt)) p.fillRect(QRect(pt, QSize(3, 3)), brushNormal);
		}
	}
}

/*! \class QPolygonsView
		\brief Provides scrollable view of an QImage with vectors over it.
 */

/*! Default constructor.
 */
QPolygonsView::QPolygonsView(QWidget* parent)
	: QImageView(parent)
{
	iw = std::make_unique<PolyImageWidget>();
	setWidget(iw.get());
}

/*! Gets the polygonList that is drawn over the image.
 * \sa setPolygons(Polygons::PolygonList* p)
 */
const Polygons::PolygonList& QPolygonsView::polygons() const
{
	return static_cast<PolyImageWidget*>(iw.get())->polygons();
}

/*! Sets the polygonList that is drawn over the image.
 */
void QPolygonsView::setPolygons(const Polygons::PolygonList& p)
{
	static_cast<PolyImageWidget*>(iw.get())->setPolygons(p);
}
} // cove

//@}
