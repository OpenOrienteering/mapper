/*
 *    Copyright 2021 Libor Pecháček
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


#ifndef OPENORIENTEERING_MOVE_PARALLEL_TOOL_H
#define OPENORIENTEERING_MOVE_PARALLEL_TOOL_H

#include <QObject>
#include <QString>
#include <QtGlobal>

#include <memory>

#include "core/map_coord.h"
#include "tools/edit_tool.h"
#include "tools/tool_base.h"

class QAction;
class QPainter;


namespace OpenOrienteering {

class MapEditorController;
class MapRenderables;
class MapWidget;
class PathObject;

/**
 * A tool to edit lines of PathObjects.
 */
class MoveParallelTool : public MapEditorToolBase
{
	using HoverFlag = EditTool::HoverFlag;
	using HoverState = EditTool::HoverState;
	
Q_OBJECT
public:
	MoveParallelTool(MapEditorController* editor, QAction* tool_action);
	~MoveParallelTool();

protected:
	void mouseMove() override;
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	void updateHoverState();
	
	void updateStatusText() override;
	void objectSelectionChangedImpl() override;
	
private:
	/**
	 * An object created for the current hover_line.
	 */
	std::unique_ptr<PathObject> highlight_object;
	std::unique_ptr<MapRenderables> highlight_renderables;

	/**
	 * Provides general information on what is hovered over.
	 */
	HoverState hover_state = HoverFlag::OverNothing;

	/**
	 * Object which is hovered over (if any).
	 */
	PathObject* hover_object = {};

	/**
	 * A copy of the original object which we shift. The new object is
	 * always a direct modification of the initial copy, so that we
	 * don't accumulate errors.
	 */
	std::unique_ptr<PathObject> orig_hover_object;

	/**
	 * Closest point on the dragged path. We use this point to calculate
	 * the object shift distance.
	 */
	MapCoordF path_drag_point;

	/**
	 * Normal vector at the closest point of the highlighted path. We use the
	 * normal vector for distance calsulation in dragMove().
	 */
	MapCoordF path_normal_vector;

	/**
	  * The move distance is exposed from dragMove() for the purpose of
	  * status text updates.
	  */
	qreal move_distance = {};
};


}  // namespace OpenOrienteering

#endif
