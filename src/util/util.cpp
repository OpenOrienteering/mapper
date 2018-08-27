/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#include "util.h"

#include <QChar>
#include <QIODevice>
#include <QObject>
#include <QObjectList>
#include <QRect>
#include <QString>

#include "core/map_coord.h"

class QPointF;


namespace OpenOrienteering {

void blockSignalsRecursively(QObject* obj, bool block)
{
	if (!obj)
		return;
	obj->blockSignals(block);
	
	const QObjectList& list = obj->children();
	for (auto child : list)
		blockSignalsRecursively(child, block);
}



void rectInclude(QRectF& rect, const QPointF& point)
{
	if (point.x() < rect.left())
		rect.setLeft(point.x());
	else if (point.x() > rect.right())
		rect.setRight(point.x());
	
	if (point.y() < rect.top())
		rect.setTop(point.y());
	else if (point.y() > rect.bottom())
		rect.setBottom(point.y());
}

void rectIncludeSafe(QRectF& rect, const QPointF& point)
{
	if (rect.isValid())
		rectInclude(rect, point);
	else
		rect = QRectF(point.x(), point.y(), 0.0001, 0.0001);
}

void rectInclude(QRectF& rect, const QRectF& other_rect)
{
	if (other_rect.left() < rect.left())
		rect.setLeft(other_rect.left());
	if (other_rect.right() > rect.right())
		rect.setRight(other_rect.right());
	
	if (other_rect.top() < rect.top())
		rect.setTop(other_rect.top());
	if (other_rect.bottom() > rect.bottom())
		rect.setBottom(other_rect.bottom());
}

void rectIncludeSafe(QRectF& rect, const QRectF& other_rect)
{
	if (other_rect.isValid())
	{
		if (rect.isValid())
			rectInclude(rect, other_rect);
		else 
			rect = other_rect;
	}
}

void rectIncludeSafe(QRect& rect, const QRect& other_rect)
{
	if (other_rect.isValid())
	{
		if (rect.isValid())
		{
			if (other_rect.left() < rect.left())
				rect.setLeft(other_rect.left());
			if (other_rect.right() > rect.right())
				rect.setRight(other_rect.right());
			
			if (other_rect.top() < rect.top())
				rect.setTop(other_rect.top());
			if (other_rect.bottom() > rect.bottom())
				rect.setBottom(other_rect.bottom());
		}
		else 
			rect = other_rect;
	}
}

bool lineIntersectsRect(const QRectF& rect, const QPointF& p1, const QPointF& p2)
{
	if (rect.contains(p1) || rect.contains(p2))
		return true;
	
	// Left side
	if ((p1.x() > rect.left()) != (p2.x() > rect.left()))
	{
		if ((p2.x() == p1.x()) && !((p1.y() < rect.top() && p2.y() < rect.top()) || (p1.y() > rect.bottom() && p2.y() > rect.bottom())))
			return true;
		qreal intersection_y = p1.y() + (p2.y() - p1.y()) * (rect.left() - p1.x()) / (p2.x() - p1.x());
		if (intersection_y >= rect.top() && intersection_y <= rect.bottom())
			return true;
	}
	// Right side
	if ((p1.x() > rect.right()) != (p2.x() > rect.right()))
	{
		if ((p2.x() == p1.x()) && !((p1.y() < rect.top() && p2.y() < rect.top()) || (p1.y() > rect.bottom() && p2.y() > rect.bottom())))
			return true;
		qreal intersection_y = p1.y() + (p2.y() - p1.y()) * (rect.right() - p1.x()) / (p2.x() - p1.x());
		if (intersection_y >= rect.top() && intersection_y <= rect.bottom())
			return true;
	}
	
	// Top side
	if ((p1.y() > rect.top()) != (p2.y() > rect.top()))
	{
		if ((p2.y() == p1.y()) && !((p1.x() < rect.left() && p2.x() < rect.left()) || (p1.x() > rect.right() && p2.x() > rect.right())))
			return true;
		qreal intersection_x = p1.x() + (p2.x() - p1.x()) * (rect.top() - p1.y()) / (p2.y() - p1.y());
		if (intersection_x >= rect.left() && intersection_x <= rect.right())
			return true;
	}
	// Bottom side
	if ((p1.y() > rect.bottom()) != (p2.y() > rect.bottom()))
	{
		if ((p2.y() == p1.y()) && !((p1.x() < rect.left() && p2.x() < rect.left()) || (p1.x() > rect.right() && p2.x() > rect.right())))
			return true;
		qreal intersection_x = p1.x() + (p2.x() - p1.x()) * (rect.bottom() - p1.y()) / (p2.y() - p1.y());
		if (intersection_x >= rect.left() && intersection_x <= rect.right())
			return true;
	}
	
	return false;
}

double parameterOfPointOnLine(double x0, double y0, double dx, double dy, double x, double y, bool& ok)
{
	const double epsilon = 1e-5;
	ok = true;
	
	if (qAbs(dx) > qAbs(dy))
	{
		double param = (x - x0) / dx;
		if (qAbs(y0 + param * dy - y) < epsilon)
			return param;
	}
	else if (dy != 0)
	{
		double param = (y - y0) / dy;
		if (qAbs(x0 + param * dx - x) < epsilon)
			return param;
	}
	
	ok = false;
	return -1;
}

bool isPointOnSegment(const MapCoordF& seg_start, const MapCoordF& seg_end, const MapCoordF& point)
{
	bool ok;
	double param = parameterOfPointOnLine(seg_start.x(), seg_start.y(),
	                                      seg_end.x() - seg_start.x(), seg_end.y() - seg_start.y(),
	                                      point.x(), point.y(), ok);
	return ok && param >= 0 && param <= 1;
}

void loadString(QIODevice* file, QString& str)
{
	int length;
	
	file->read((char*)&length, sizeof(int));
	if (length > 0)
	{
		str.resize(length);
		file->read((char*)str.data(), length * sizeof(QChar));
	}
	else
		str.clear();
}


}  // namespace OpenOrienteering
