/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_TOOL_MOVE_H
#define OPENORIENTEERING_TEMPLATE_TOOL_MOVE_H

#include <QtGlobal>
#include <QObject>

#include "core/map_coord.h"
#include "tools/tool.h"

class QAction;
class QCursor;
class QMouseEvent;

namespace OpenOrienteering {

class MapEditorController;
class MapWidget;
class Template;


/** Tool to move a template by hand. */
class TemplateMoveTool : public MapEditorTool
{
Q_OBJECT
public:
	TemplateMoveTool(Template* templ, MapEditorController* editor, QAction* tool_action = nullptr);
	~TemplateMoveTool();
	
	void init() override;
	const QCursor& getCursor() const override;
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
public slots:
	void templateDeleted(int index, const Template* temp);
	
private:
	void updateDragging(const MapCoordF& mouse_pos_map);
	
	Template* templ;
	bool dragging = false;
	MapCoordF click_pos_map;
	
	Q_DISABLE_COPY(TemplateMoveTool)
};


}  // namespace OpenOrienteering

#endif
