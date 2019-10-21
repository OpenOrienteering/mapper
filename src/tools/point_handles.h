/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015, 2017 Kai Pastor
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

#ifndef OPENORIENTEERING_POINT_HANDLES_H
#define OPENORIENTEERING_POINT_HANDLES_H


#include <limits>

#include <QImage>
#include <QPointF>  // IWYU pragma: no_forward_declare QPointF
#include <QRgb>

#include "core/map_coord.h"

class QPainter;

namespace OpenOrienteering {

class MapWidget;
class Object;


/**
 * @brief This class deals with anchor and control points for objects.
 */
class PointHandles
{
public:
	/**
	 * @brief Types of point handle.
	 * 
	 * The numbers correspond to the columns in point-handles.png
	 */
	enum PointHandleType
	{
		StartHandle  = 0,
		EndHandle    = 1,
		NormalHandle = 2,
		CurveHandle  = 3,
		DashHandle   = 4
	};
	
	/**
	 * @brief States of point handles.
	 * 
	 * The numbers correspond to the rows in point-handles.png
	 */
	enum PointHandleState
	{
		NormalHandleState   = 0,
		ActiveHandleState   = 1,
		SelectedHandleState = 2,
		DisabledHandleState = 3
	};
	
	/**
	 * Default constructor.
	 * 
	 * The resulting object is in a valid stable, but not really useful
	 * until calling setScaleFactor().
	 */
	PointHandles() noexcept;
	
	/**
	 * Constructs a point handles utility for the given scale factor.
	 */
	PointHandles(unsigned int factor);
	
	
	/**
	 * @brief The factor by which all drawing shall be scaled.
	 * 
	 * The control point handles image matches this factor, too.
	 * Currently, this value is either 0, 1, 2, or 4.
	 * (0 represents a default constructed object.)
	 */
	unsigned int scaleFactor() const { return scale_factor; }
	
	/**
	 * Sets the scale factor.
	 * 
	 * Currently, valid factors are 1, 2, or 4.
	 */
	void setScaleFactor(unsigned int factor);
	
	
	/**
	 * The number of covered pixels around the center of a point handle.
	 */
	int displayRadius() const { return display_radius; }
	
	
	/**
	 * @brief The control point handles image.
	 * 
	 * The image matches the current scale factor.
	 */
	const QImage image() const { return handle_image; }
	
	
	/**
	 * @brief Returns the color in which point handles with the given state will be displayed.
	 */
	static QRgb stateColor(PointHandleState state);
	
	
	/**
	 * @brief Draws a single handle.
	 */
	void draw(QPainter* painter, const QPointF& position, PointHandleType type, PointHandleState state) const;
	
	/**
	 * @brief Draws all point handles for the given object.
	 * 
	 * @param painter QPainter object with correct transformation set.
	 * @param widget Map widget which provides coordinate transformations.
	 * @param object Object to draw point handles for.
	 * @param hover_point Index of a point which should be drawn in 'active' state. Pass out-of-bounds value to disable.
	 * @param draw_curve_handles If false, curve handles for path objects are not drawn.
	 * @param base_state The state in which all points except hover_point should be drawn.
	 */
	void draw(QPainter* painter,
	        const MapWidget* widget,
	        const Object* object,
	        MapCoordVector::size_type hover_point = std::numeric_limits<MapCoordVector::size_type>::max(),
	        bool draw_curve_handles = true,
	        PointHandleState base_state = NormalHandleState
	) const;
	
	/**
	 * @brief Draws a point handle line.
	 * 
	 * The curve handle line connects a curve anchor point and a controlling handle.
	 */
	void drawCurveHandleLine(QPainter* painter, QPointF anchor_point, QPointF curve_handle, PointHandleType type, PointHandleState state) const;
	
private:
	/**
	 * @brief Loads and returns the image for a for a particular scale factor.
	 * 
	 * Since scale depends on the pixels-per-inch setting and doesn't change
	 * very often, the image is loaded only when the scale changes and returned
	 * from an internal cache otherwise.
	 */
	static const QImage loadHandleImage(unsigned int factor);
	
	
	QImage handle_image;
	unsigned int scale_factor = 0;
	int display_radius = 0;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_POINT_HANDLES_H
