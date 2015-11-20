/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_TOOL_FILL_H_
#define _OPENORIENTEERING_TOOL_FILL_H_

#include "tool_base.h"

class MapView;
class PathObject;

/** 
 * Tool to fill bounded areas with PathObjects.
 */
class FillTool : public MapEditorToolBase
{
Q_OBJECT
public:
	FillTool(MapEditorController* editor, QAction* tool_action);
	virtual ~FillTool();
	
protected slots:
	void setDrawingSymbol(const Symbol* symbol);
	
protected:
	/**
	 * Helper structure used to represent a section of a traced path
	 * while constructing the fill object.
	 */
	struct PathSection
	{
		PathObject* object;
		int part;
		float start_clen;
		float end_clen;
	};
	
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void clickPress();
	
	/**
	 * Tries to apply the fill tool at the current click position,
	 * rasterizing the given extent of the map.
	 * Returns -1 for abort, 0 for unsuccesful, 1 for succesful.
	 */
	int fill(const QRectF& extent);
	
	/**
	 * Rasterizes an area of the current map part with the given extent into an image.
	 * Encodes object ids as colors, where the object with index 0 has color (0, 0, 0, 255),
	 * the object with index 1 has (1, 0, 0, 255), and so on. The background is transparent.
	 * Returns the image and the used map-to-image transform.
	 */
	QImage rasterizeMap(const QRectF& extent, QTransform& out_transform);
	
	/**
	 * Helper method for rasterizeMap().
	 */
	void drawObjectIDs(Map* map, QPainter* painter, QRectF bounding_box, float scaling);
	
	/**
	 * Traces the boundary around an "island" in the given image, starting from the
	 * start_pixel / test_pixel pair, where start_pixel must reference a free (transparent)
	 * pixel and test_pixel a 4-adjacent obstructed pixel of the island to trace.
	 * Returns the found boundary as a vector of pixel positions. Returns:
	 * -1 if running out of the image borders,
	 *  0 if the tracing fails because the start is not included in the shape,
	 *  1 if the tracing succeeds.
	 */
	int traceBoundary(QImage image, QPoint start_pixel, QPoint test_pixel, std::vector< QPoint >& out_boundary);
	
	/**
	 * Creates a fill object for the given image, boundary vector (of pixel positions) and transform.
	 * Returns false if the creation fails.
	 */
	bool fillBoundary(const QImage& image, const std::vector< QPoint >& boundary, QTransform image_to_map);
	
	const Symbol* drawing_symbol;
};

#endif
