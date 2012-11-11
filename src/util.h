/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef UTIL_H
#define UTIL_H

#include "map_coord.h"

#include <qmath.h>
#include <QDoubleValidator>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

#define BEZIER_KAPPA 0.5522847498
#define LOG2 0.30102999566398119521373889472449

/// Double validator for line edit widgets
class DoubleValidator : public QDoubleValidator
{
public:
	DoubleValidator(double bottom, double top = 10e10, QObject* parent = NULL, int decimals = 20);
	
	virtual State validate(QString& input, int& pos) const;
};

/// (Un-)blocks recursively all signals from a QObject and its child-objects.
void blockSignalsRecursively(QObject* obj, bool block);

/// Returns a practically "infinitely" big QRectF
inline QRectF infinteRectF()
{
	return QRectF(-10e10, -10e10, 20e10, 20e10);
}

/// Enlarges the rect to include the given point
void rectInclude(QRectF& rect, MapCoordF point); // does not work if rect is invalid
void rectInclude(QRectF& rect, QPointF point); // does not work if rect is invalid
void rectIncludeSafe(QRectF& rect, QPointF point); // checks if rect is invalid
/// Enlarges the rect to include the given rect
void rectInclude(QRectF& rect, const QRectF& other_rect); // does not work if rect is invalid
void rectIncludeSafe(QRectF& rect, const QRectF& other_rect); // checks if rect is invalid

/// Checks for line - rect intersection
bool lineIntersectsRect(const QRectF& rect, const QPointF& p1, const QPointF& p2);

/// Helper functions to save a string to a file and load it again
void saveString(QIODevice* file, const QString& str);
void loadString(QIODevice* file, QString& str);

// TODO: Refactor: put remaining stuff into this namespace, too
namespace Util
{
	/// See Util::gridOperation(). This function handles only parallel lines.
	template<typename T> void hatchingOperation(QRectF extent, double spacing, double offset, double rotation, T& processor)
	{
		// Make rotation unique
		rotation = fmod(1.0 * rotation, M_PI);
		if (rotation < 0)
			rotation = M_PI + rotation;
		assert(rotation >= 0 && rotation <= M_PI);
		
		if (qAbs(rotation - M_PI/2) < 0.0001)
		{
			// Special case: vertical lines
			double first = offset + ceil((extent.left() - offset) / (spacing)) * spacing;
			for (double cur = first; cur < extent.right(); cur += spacing)
			{
				processor.processLine(QPointF(cur, extent.top()), QPointF(cur, extent.bottom()));
			}
		}
		else if (qAbs(rotation - 0) < 0.0001)
		{
			// Special case: horizontal lines
			double first = offset + ceil((extent.top() - offset) / (spacing)) * spacing;
			for (double cur = first; cur < extent.bottom(); cur += spacing)
			{
				processor.processLine(QPointF(extent.left(), cur), QPointF(extent.right(), cur));
			}
		}
		else
		{
			// General case
			double xfactor = 1.0f / sin(rotation);
			double yfactor = 1.0f / cos(rotation);
			
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
				
				do
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
					processor.processLine(QPointF(start_x, start_y), QPointF(end_x, end_y));
					
					// Move to next position
					start_x += dist_x;
					end_y += dist_y;
				} while (true);
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
				
				do
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
					processor.processLine(QPointF(start_x, start_y), QPointF(end_x, end_y));
					
					// Move to next position
					start_x += dist_x;
					end_y += dist_y;
				} while (true);
			}
		}
	}
	
	/// Used to implement arbitrarily rotated grids which are constrained to an axis-aligned bounding box.
	/// This functions calls processor.processLine(QPointF a, QPointF b) for every line which is calculated to be in the given box.
	template<typename T> void gridOperation(QRectF extent, double horz_spacing, double vert_spacing,
											double horz_offset, double vert_offset, double rotation, T& processor)
	{
		hatchingOperation(extent, horz_spacing, horz_offset, rotation, processor);
		hatchingOperation(extent, vert_spacing, vert_offset, rotation + M_PI / 2, processor);
	}
}

#endif
