/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017, 2019 Kai Pastor
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

#include <QtGlobal>
#include <QObject>
#include <QObjectList>
#include <QRect>

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


void Util::hatchingOperation(const QRectF& extent, double spacing, double offset, double rotation,
                             std::function<void (const QPointF&, const QPointF&)>& process_line)
{
	// Make rotation unique
	rotation = fmod(rotation, 2 * M_PI);
	if (rotation < 0)
		rotation += 2 * M_PI;
	if (rotation >= M_PI)
	{
		rotation -= M_PI;
		offset = -offset;
	}
	Q_ASSERT(rotation >= 0 && rotation <= M_PI);
	
	if (qAbs(rotation - M_PI/2) < 0.0001)
	{
		// Special case: vertical lines
		double cur = offset + ceil((extent.left() - offset) / (spacing)) * spacing;
		auto const count = qCeil(extent.width() / spacing);
		for (auto i = 0; i < count; ++i)
		{
			process_line(QPointF(cur, extent.top()), QPointF(cur, extent.bottom()));
			cur += spacing;
		}
	}
	else if (rotation < 0.0001 || rotation > M_PI - 0.0001)
	{
		// Special case: horizontal lines
		if (rotation > M_PI/2)
			offset = -offset;
		auto cur = offset + ceil((extent.top() - offset) / (spacing)) * spacing;
		auto const count = qCeil(extent.height() / spacing);
		for (auto i = 0; i < count; ++i)
		{
			process_line(QPointF(extent.left(), cur), QPointF(extent.right(), cur));
			cur += spacing;
		}
	}
	else
	{
		// General case
		double xfactor = 1.0 / sin(rotation);
		double yfactor = 1.0 / cos(rotation);
		
		double dist_x = xfactor * spacing;
		double dist_y = yfactor * spacing;
		double offset_x = xfactor * offset;
		double offset_y = yfactor * offset;
		
		if (rotation < M_PI/2)
		{
			// Start with the upper left corner
			offset_x += (-extent.top()) / tan(rotation);
			offset_y -= extent.left() * tan(rotation);
			double start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			double start_y = extent.top();
			double end_x = extent.left();
			double end_y = offset_y + ceil((extent.y() - offset_y) / dist_y) * dist_y;
			
			for (auto i = 0; i < 100000; ++i)
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y > extent.bottom())
				{
					end_x += ((end_y - extent.bottom()) / dist_y) * dist_x;
					end_y = extent.bottom();
				}
				
				if (start_y > extent.bottom())
					break;
				
				// Process the line
				process_line(QPointF(start_x, start_y), QPointF(end_x, end_y));
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			}
		}
		else
		{
			// Start with left lower corner
			offset_x += (-extent.bottom()) / tan(rotation);
			offset_y -= extent.x() * tan(rotation);
			double start_x = offset_x + ceil((extent.x() - offset_x) / dist_x) * dist_x;
			double start_y = extent.bottom();
			double end_x = extent.x();
			double end_y = offset_y + ceil((extent.bottom() - offset_y) / dist_y) * dist_y;
			
			for (auto i = 0; i < 100000; ++i)
			{
				// Correct coordinates
				if (start_x > extent.right())
				{
					start_y += ((start_x - extent.right()) / dist_x) * dist_y;
					start_x = extent.right();
				}
				if (end_y < extent.y())
				{
					end_x += ((end_y - extent.y()) / dist_y) * dist_x;
					end_y = extent.y();
				}
				
				if (start_y < extent.y())
					break;
				
				// Process the line
				process_line(QPointF(start_x, start_y), QPointF(end_x, end_y));
				
				// Move to next position
				start_x += dist_x;
				end_y += dist_y;
			}
		}
	}
}


bool Util::pointsFormCorner(const MapCoord& point1, const MapCoord& anchor_point,
                            const MapCoord& point2, const qreal quantum_size)
{
	const MapCoordF segment1 { point2 - anchor_point };
	const MapCoordF segment2 { point1 - anchor_point };
	
	if (MapCoordF::dotProduct(segment1, segment2) > 0)
	{
		// both handles are poiniting into the same half-space
		// therefore anchor point is surely a corner
		return true;
	}
	
	// dot product based point-to-line distance calculation
	auto perp_to_seg1 = segment1.normalVector();
	perp_to_seg1.normalize();
	const auto distance = MapCoordF::dotProduct(perp_to_seg1, segment2);
	
	return qAbs(distance) > quantum_size;
}


}  // namespace OpenOrienteering
