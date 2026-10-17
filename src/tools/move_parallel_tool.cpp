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


#include "move_parallel_tool.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <vector>

#include <QCursor>
#include <QFlags>
#include <QLocale>
#include <QPixmap>
#include <QtGlobal>
#include <QtMath>

#include "core/map.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/path_coord.h"
#include "core/renderables/renderable.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "gui/map/map_widget.h"
#include "tools/tool.h"
#include "tools/tool_base.h"


class QPainter;


namespace OpenOrienteering {


MoveParallelTool::MoveParallelTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase(scaledToScreen(QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-move-parallel.png")), 1, 1 }),
                    Other, editor, tool_action),
  highlight_renderables(new MapRenderables(map()))
{
	// nothing
}


MoveParallelTool::~MoveParallelTool()
{
	// nothing
}


void MoveParallelTool::updateStatusText()
{
	if (editingInProgress())
	{
		setStatusBarText(tr("<b>Move distance:</b> %1 mm")
		                 .arg(QLocale().toString(qFabs(move_distance), 'f', 1)));
	}
	else
	{
		setStatusBarText({});
	}
}


void MoveParallelTool::objectSelectionChangedImpl()
{
	if (map()->getNumSelectedObjects() == 0)
		deactivate();
	else
		updateHoverState();
}


/** \todo - Converge the updateHoverState() implementation across the
  * different tools (cut, edit line, edit point).
  */
void MoveParallelTool::updateHoverState()
{
	auto new_hover_state = HoverState { HoverFlag::OverNothing };
	const PathObject* new_hover_object = {};

	if (!map()->selectedObjects().empty())
	{
		auto const click_tolerance_sq = qPow(0.001 * cur_map_widget->getMapView()->pixelToLength(clickTolerance()), 2);
		auto best_distance_sq = std::numeric_limits<double>::max();

		for (auto* object : map()->selectedObjects())
		{
			if (object->getType() == Object::Path)
			{
				auto* path = object->asPath();
				auto closest = path->findClosestPointTo(cur_pos_map);

				if (closest.distance_squared >= +0.0 &&
				    closest.distance_squared < best_distance_sq &&
				    closest.distance_squared < qMax(click_tolerance_sq, qPow(path->getSymbol()->calculateLargestLineExtent(), 2)))
				{
					new_hover_state  = HoverFlag::OverPathEdge;
					new_hover_object = path;
					best_distance_sq = closest.distance_squared;
					path_drag_point  = closest.path_coord.pos;
					path_normal_vector = closest.tangent.normalVector();
					path_normal_vector.normalize();
				}
			}
		}
	}

	// Apply possible changes
	if (new_hover_state  != hover_state ||
	    new_hover_object != hover_object)
	{
		highlight_renderables->removeRenderablesOfObject(highlight_object.get(), false);
		
		hover_state  = new_hover_state;
		hover_object = const_cast<PathObject*>(new_hover_object);

		if (hover_state == HoverFlag::OverPathEdge && hover_object)
		{
			// Extract hover line
			highlight_object = std::unique_ptr<PathObject>(hover_object->duplicate());
			highlight_object->setSymbol(map()->getCoveringCombinedLine(), true);
			highlight_object->update();
			highlight_renderables->insertRenderablesOfObject(highlight_object.get());
		}

		effective_start_drag_distance = (hover_state == HoverFlag::OverNothing) ? startDragDistance() : 0;
		updateDirtyRect();
	}
}


void MoveParallelTool::mouseMove()
{
	updateHoverState();
}


void MoveParallelTool::dragStart()
{
	// Drag movement can start without prior mouse move event on the object.
	// E.g. position cursor on above an object border, select the tool and
	// immediately start a drag. This is why we update hover state here.
	updateHoverState();

	if (!hover_object)
		return;

	orig_hover_object = std::unique_ptr<PathObject>(hover_object->duplicate());
	startEditing(hover_object);
	updateStatusText();
}


void MoveParallelTool::dragMove()
{
	if (!hover_object)
		return;

	// taken from PathObject::findClosestPointOnBorder
	MapCoordVector border_flags;
	MapCoordVectorF border_coords;
	move_distance = MapCoordF::dotProduct(path_normal_vector, cur_pos_map - path_drag_point);

	hover_object->clearCoordinates();
	highlight_renderables->removeRenderablesOfObject(highlight_object.get(), false);
	highlight_object->clearCoordinates(); // TODO - can we share the path part vectors with hover object?
	for (auto const& part : orig_hover_object->parts())
	{
		LineSymbol::shiftCoordinates(part, -move_distance, 0,
		        LineSymbol::MiterJoin, border_flags, border_coords);

		auto start_new_part = true;
		std::transform(begin(border_flags), end(border_flags),
		               begin(border_coords), begin(border_flags),
		               [this, &start_new_part](auto coord, auto const& coord_f) {
			                coord.setX(coord_f.x());
							coord.setY(coord_f.y());
							hover_object->addCoordinate(coord, start_new_part);
							highlight_object->addCoordinate(coord, start_new_part);
							start_new_part = false;
							return coord;
		                });
	}

	highlight_object->update();
	highlight_renderables->insertRenderablesOfObject(highlight_object.get());
	updatePreviewObjectsAsynchronously();
	updateDirtyRect();
	updateStatusText();
}


void MoveParallelTool::dragFinish()
{
	if (!hover_object)
		return;

	finishEditing();
	updateStatusText();
}


void MoveParallelTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	auto num_selected_objects = map()->selectedObjects().size();
	if (num_selected_objects > 0)
	{
		drawSelectionOrPreviewObjects(painter, widget);

		if (!highlight_renderables->empty())
			map()->drawSelection(painter, true, widget, highlight_renderables.get(), true);
	}
}


}  // namespace OpenOrienteering
