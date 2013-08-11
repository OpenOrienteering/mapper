/*
 *    Copyright 2013 Thomas Schöps
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

class SymbolWidget;
class MapView;
class PathObject;

/** 
 * Tool to fill bounded areas with PathObjects.
 */
class FillTool : public MapEditorToolBase
{
Q_OBJECT
public:
	FillTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	virtual ~FillTool();
	
protected slots:
	void selectedSymbolsChanged();
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
protected:
	struct PathSection
	{
		PathObject* object;
		float start_clen;
		float end_clen;
	};
	
	virtual void updateStatusText();
	virtual void objectSelectionChangedImpl();
	
	virtual void clickPress();
	
	QImage rasterizeMap(const QRectF& extent, QTransform& out_transform);
	void drawObjectIDs(Map* map, QPainter* painter, QRectF bounding_box, float scaling);
	bool traceBoundary(QImage image, QPoint start_pixel, QPoint test_pixel, std::vector< QPoint >& out_boundary);
	bool fillBoundary(const QImage& image, const std::vector< QPoint >& boundary, QTransform image_to_map);
	
	SymbolWidget* symbol_widget;
	Symbol* last_used_symbol;
};

#endif
