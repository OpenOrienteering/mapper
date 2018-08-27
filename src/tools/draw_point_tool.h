/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015 Kai Pastor
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


#ifndef OPENORIENTEERING_DRAW_POINT_H
#define OPENORIENTEERING_DRAW_POINT_H

#include <memory>

#include <QObject>
#include <QPointF>

#include "tool_base.h"

class QAction;
class QEvent;
class QKeyEvent;
class QPainter;
class QPointF;
class QRectF;

namespace OpenOrienteering {

class MapCoordF;
class MapEditorController;
class MapRenderables;
class MapWidget;
class PointObject;
class Symbol;


/**
 * Tool to draw PointObjects.
 */
class DrawPointTool : public MapEditorToolBase
{
Q_OBJECT
public:
	DrawPointTool(MapEditorController* editor, QAction* tool_action);
	~DrawPointTool() override;
	
protected:
	void initImpl() override;
	
	/**
	 * Tells the tool that the editor's active symbol changed.
	 */
	void activeSymbolChanged(const Symbol* symbol);
	
	/**
	 * Tells the tool that a symbol was deleted from the map.
	 */
	void symbolDeleted(int pos, const Symbol* symbol);
	
	void updatePreviewObject(const MapCoordF& pos);
	
	void createObject();
	
	/**
	 * Calculates the object's rotation for the given mouse position.
	 * 
	 * Return 0 if the user didn't dragged the mouse away the minimum distance
	 * from the click point.
	 */
	double calculateRotation(const QPointF& mouse_pos, const MapCoordF& mouse_pos_map) const;

	void leaveEvent(QEvent* event) override;
	void mouseMove() override;
	void clickPress() override;
	void clickRelease() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
	
	std::unique_ptr<PointObject> preview_object;
	std::unique_ptr<MapRenderables> renderables;
};


}  // namespace OpenOrienteering

#endif
