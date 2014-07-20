/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013, 2014 Kai Pastor
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


#include "tool.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>

#include "gui/main_window.h"
#include "map.h"
#include "map_editor.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object.h"
#include "object_text.h"
#include "settings.h"
#include "tool_helpers.h"
#include "util.h"

// Utility function for MapEditorTool::findHoverPoint
inline
qreal distanceSquared(QPointF a, const QPointF& b)
{
	a -= b;
	return QPointF::dotProduct(a, a); // introduced in Qt 5.1
}


const QRgb MapEditorTool::inactive_color = qRgb(0, 0, 255);
const QRgb MapEditorTool::active_color = qRgb(255, 150, 0);
const QRgb MapEditorTool::selection_color = qRgb(210, 0, 229);

MapEditorTool::MapEditorTool(MapEditorController* editor, Type type, QAction* tool_action)
: QObject(NULL)
, editor(editor)
, tool_action(tool_action)
, tool_type(type)
, editing_in_progress(false)
, uses_touch_cursor(true)
, scale_factor(newScaleFactor())
, point_handles(scale_factor)
{
	if (tool_action)
		connect(tool_action, SIGNAL(destroyed()), this, SLOT(toolActionDestroyed()));
	
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(updateScaleFactor()));
}

MapEditorTool::~MapEditorTool()
{
	if (tool_action)
		tool_action->setChecked(false);
}

// slot
void MapEditorTool::toolActionDestroyed()
{
	tool_action = NULL;
}

void MapEditorTool::init()
{
	if (tool_action)
		tool_action->setChecked(true);
}

void MapEditorTool::deactivate()
{
	if (editor->getTool() == this)
	{
		if (toolType() == MapEditorTool::EditPoint)
			editor->setTool(NULL);
		else
			editor->setEditTool();
	}
	deleteLater();
}

void MapEditorTool::switchToDefaultDrawTool(Symbol* symbol) const
{
	editor->setTool(editor->getDefaultDrawToolForSymbol(symbol));
}

// virtual
void MapEditorTool::finishEditing()
{
	setEditingInProgress(false);
}

void MapEditorTool::setEditingInProgress(bool state)
{
	if (editing_in_progress != state)
	{
		editing_in_progress = state;
		editor->setEditingInProgress(state);
	}
}


void MapEditorTool::draw(QPainter* painter, MapWidget* widget)
{
	Q_UNUSED(painter);
	Q_UNUSED(widget);
	// nothing
}


bool MapEditorTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(map_coord);
	Q_UNUSED(widget);
	return false;
}

bool MapEditorTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(map_coord);
	Q_UNUSED(widget);
	return false;
}

bool MapEditorTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(map_coord);
	Q_UNUSED(widget);
	return false;
}

bool MapEditorTool::mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(event);
	Q_UNUSED(map_coord);
	Q_UNUSED(widget);
	return false;
}

void MapEditorTool::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	// nothing
}


bool MapEditorTool::keyPressEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

bool MapEditorTool::keyReleaseEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	return false;
}

void MapEditorTool::focusOutEvent(QFocusEvent* event)
{
	Q_UNUSED(event);
	// nothing
}


void MapEditorTool::useTouchCursor(bool enabled)
{
	uses_touch_cursor = enabled;
}

Map* MapEditorTool::map() const
{
	return editor->getMap();
}

MapWidget* MapEditorTool::mapWidget() const
{
	return editor->getMainWidget();
}

MainWindow* MapEditorTool::mainWindow() const
{
	return editor->getWindow();
}

QWidget* MapEditorTool::window() const
{
	return editor->getWindow();
}

bool MapEditorTool::isDrawTool() const
{
	switch (tool_type)
	{
	case DrawPoint:
	case DrawPath:
	case DrawCircle:
	case DrawRectangle:
	case DrawFreehand:
	case DrawText:      return true;
		
	default:            return false;
	}
}

void MapEditorTool::setStatusBarText(const QString& text)
{
	editor->getWindow()->setStatusBarText(text);
}

void MapEditorTool::drawSelectionBox(QPainter* painter, MapWidget* widget, const MapCoordF& corner1, const MapCoordF& corner2) const
{
	painter->setBrush(Qt::NoBrush);
	
	QPoint point1 = widget->mapToViewport(corner1).toPoint();
	QPoint point2 = widget->mapToViewport(corner2).toPoint();
	QPoint top_left = QPoint(qMin(point1.x(), point2.x()), qMin(point1.y(), point2.y()));
	QPoint bottom_right = QPoint(qMax(point1.x(), point2.x()), qMax(point1.y(), point2.y()));
	
	painter->setPen(QPen(QBrush(active_color), scaleFactor()));
	painter->drawRect(QRect(top_left, bottom_right - QPoint(1, 1)));
	painter->setPen(QPen(QBrush(qRgb(255, 255, 255)), scaleFactor()));
	painter->drawRect(QRect(top_left + QPoint(1, 1), bottom_right - QPoint(2, 2)));
}

void MapEditorTool::startEditingSelection(MapRenderables& old_renderables, std::vector< Object* >* undo_duplicates)
{
	Map* map = editor->getMap();
	
	// Temporarily take the edited objects out of the map so their map renderables are not updated, and make duplicates of them before for the edit step
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (undo_duplicates)
			undo_duplicates->push_back((*it)->duplicate());
		
		(*it)->setMap(NULL);
		
		// Cache old renderables until the object is inserted into the map again
		old_renderables.insertRenderablesOfObject(*it);
 		(*it)->takeRenderables();
	}
	
	editor->setEditingInProgress(true);
}

void MapEditorTool::resetEditedObjects(std::vector< Object* >* undo_duplicates)
{
	assert(undo_duplicates);
	Map* map = editor->getMap();
	
	size_t i = 0;
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		*(*it) = *undo_duplicates->at(i);
		(*it)->setMap(NULL);
		++i;
	}
}

void MapEditorTool::finishEditingSelection(MapRenderables& renderables, MapRenderables& old_renderables, bool create_undo_step, std::vector< Object* >* undo_duplicates, bool delete_objects)
{
	ReplaceObjectsUndoStep* undo_step = create_undo_step ? new ReplaceObjectsUndoStep(editor->getMap()) : NULL;
	
	int i = 0;
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		if (!delete_objects)
		{
			(*it)->setMap(editor->getMap());
			(*it)->update(true);
		}
		
		if (create_undo_step)
			undo_step->addObject(*it, undo_duplicates->at(i));
		else
			delete undo_duplicates->at(i);
		++i;
	}
	renderables.clear();
	deleteOldSelectionRenderables(old_renderables, true);
	
	undo_duplicates->clear();
	if (create_undo_step)
		editor->getMap()->push(undo_step);
	
	editor->setEditingInProgress(false);
}

void MapEditorTool::updateSelectionEditPreview(MapRenderables& renderables)
{
	Map::ObjectSelection::const_iterator it_end = editor->getMap()->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = editor->getMap()->selectedObjectsBegin(); it != it_end; ++it)
	{
		(*it)->update(true);
		// NOTE: only necessary because of setMap(NULL) in startEditingSelection(..)
		renderables.insertRenderablesOfObject(*it);
	}
}

void MapEditorTool::deleteOldSelectionRenderables(MapRenderables& old_renderables, bool set_area_dirty)
{
	old_renderables.clear(set_area_dirty);
}

int MapEditorTool::findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QRectF* selection_extent, MapWidget* widget, MapCoordF* out_handle_pos)
{
	Q_UNUSED(selection_extent);
	
	float click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	const float click_tolerance_squared = click_tolerance * click_tolerance;
	
	if (object->getType() == Object::Point)
	{
		PointObject* point = reinterpret_cast<PointObject*>(object);
		if (distanceSquared(widget->mapToViewport(point->getCoordF()), cursor) <= click_tolerance_squared)
		{
			if (out_handle_pos)
				*out_handle_pos = point->getCoordF();
			return 0;
		}
	}
	else if (object->getType() == Object::Text)
	{
		TextObject* text = reinterpret_cast<TextObject*>(object);
		std::vector<QPointF> text_handles(text->controlPoints());
		for (std::size_t i = 0; i < text_handles.size(); ++i)
		{
			if (distanceSquared(widget->mapToViewport(text_handles[i]), cursor) <= click_tolerance_squared)
			{
				if (out_handle_pos)
					*out_handle_pos = MapCoordF(text_handles[i]);
				return i;
			}
		}
	}
	else if (object->getType() == Object::Path)
	{
		PathObject* path = reinterpret_cast<PathObject*>(object);
		int size = path->getCoordinateCount();
		
		int best_index = -1;
		float best_dist_sq = click_tolerance_squared;
		for (int i = size - 1; i >= 0; --i)
		{
			if (!path->getCoordinate(i).isClosePoint())
			{
				float distance_sq = distanceSquared(widget->mapToViewport(path->getCoordinate(i)), cursor);
				bool is_handle = (i >= 1 && path->getCoordinate(i - 1).isCurveStart()) ||
									(i >= 2 && path->getCoordinate(i - 2).isCurveStart());
				if (distance_sq < best_dist_sq || (distance_sq == best_dist_sq && is_handle))
				{
					best_index = i;
					best_dist_sq = distance_sq;
				}
			}
			if (!include_curve_handles && i >= 3 && path->getCoordinate(i - 3).isCurveStart())
				i -= 2;
		}
		if (best_index >= 0)
		{
			if (out_handle_pos)
				*out_handle_pos = MapCoordF(path->getCoordinate(best_index));
			return best_index;
		}
	}
	
	return -1;
}

bool MapEditorTool::drawMouseButtonHeld(QMouseEvent* event)
{
	if ((event->buttons() & Qt::LeftButton) ||
		(Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool() == true &&
		 event->buttons() & Qt::RightButton))
	{
		return true;
	}
	return false;
}

bool MapEditorTool::drawMouseButtonClicked(QMouseEvent* event)
{
	if ((event->button() == Qt::LeftButton) ||
		(Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool() == true &&
		 event->button() == Qt::RightButton))
	{
		return true;
	}
	return false;
}

// static
int MapEditorTool::newScaleFactor()
{
	// NOTE: The returned value must be supported by PointHandles::loadHandleImage() !
	int factor = 1;
	const float base_dpi = 96.0f;
	const float ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toFloat();
	if (ppi > base_dpi*2)
		factor = 4;
	else if (ppi > base_dpi)
		factor = 2;
	return factor;
}

// slot
void MapEditorTool::updateScaleFactor()
{
	scale_factor = newScaleFactor();
	point_handles = PointHandles(scale_factor);
}

