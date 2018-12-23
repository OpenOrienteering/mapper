/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_TOOL_CUT_HOLE_H
#define OPENORIENTEERING_TOOL_CUT_HOLE_H

#include <QObject>
#include <QRectF>

#include "tool.h"

class QAction;
class QCursor;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class DrawLineAndAreaTool;
class MapCoordF;
class MapEditorController;
class MapWidget;
class PathObject;


/** Tool to cut holes into area objects */
class CutHoleTool : public MapEditorTool
{
Q_OBJECT
public:
	/** Enum of different hole types. The CutHoleTool can be used with each of
	 *  the corresponding drawing tools. */
	enum HoleType
	{
		Path = 0,
		Circle = 1,
		Rect = 2
	};
	
	CutHoleTool(MapEditorController* editor, QAction* tool_action, CutHoleTool::HoleType hole_type);
	~CutHoleTool() override;
	
	void init() override;
	const QCursor& getCursor() const override;
	void finishEditing() override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	void leaveEvent(QEvent* event) override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	bool keyReleaseEvent(QKeyEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	
	void draw(QPainter* painter, MapWidget* widget) override;
	
public slots:
	void objectSelectionChanged();
	void pathDirtyRectChanged(const QRectF& rect);
	void pathAborted();
	void pathFinished(PathObject* hole_path);
	
protected:
	void updateStatusText();
	void updateDirtyRect(const QRectF* path_rect = nullptr);
	
	CutHoleTool::HoleType hole_type;
	DrawLineAndAreaTool* path_tool;
	MapWidget* edit_widget;
};


}  // namespace OpenOrienteering
#endif
