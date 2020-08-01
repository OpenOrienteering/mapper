/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_TOOL_PAINT_H
#define OPENORIENTEERING_TEMPLATE_TOOL_PAINT_H

#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QColor>
#include <QFlags>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>

#include "core/map_coord.h"
#include "tools/tool.h"

class QAction;
class QCursor;
class QMouseEvent;
class QPainter;

namespace OpenOrienteering {

class ActionGridBar;
class MapEditorController;
class MapWidget;
class Template;


/**
 * A tool to paint on image templates.
 */
class PaintOnTemplateTool : public MapEditorTool
{
Q_OBJECT
public:
	enum ErasingOption {
		NoErasing = 0,
		ExplicitErasing = 1,
		RightMouseButtonErasing = 2
	};
	Q_DECLARE_FLAGS(ErasingOptions, ErasingOption)
	
	PaintOnTemplateTool(MapEditorController* editor, QAction* tool_action);
	~PaintOnTemplateTool() override;
	
	void setTemplate(Template* temp);
	
	void init() override;
	const QCursor& getCursor() const override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	void draw(QPainter* painter, MapWidget* widget) override;
	
protected:
	void templateAboutToBeDeleted(int pos, Template* temp);
	
public:
	bool fillAreas() const { return fill_areas; }
	void setFillAreas(bool enabled);
	
	const QColor& color() const { return paint_color; }
	void setColor(const QColor& color);
	
	void undoSelected();
	void redoSelected();

protected:
	ActionGridBar* makeToolBar();
	
private:
	ErasingOptions erasing = {};
	bool dragging = false;
	bool fill_areas = false;
	QColor paint_color = Qt::black;
	QRectF map_bbox;
	std::vector<MapCoordF> coords;
	
	Template* temp = nullptr;
	QPointer<ActionGridBar> widget;
	
	static int erase_width;
	
	Q_DISABLE_COPY(PaintOnTemplateTool)
};


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::PaintOnTemplateTool::ErasingOptions)


#endif
