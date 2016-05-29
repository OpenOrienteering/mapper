/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#include "tool_helpers.h"

#include <qmath.h>
#include <QApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include "gui/main_window.h"
#include "map_editor.h"
#include "core/map_grid.h"
#include "map_widget.h"
#include "object_text.h"
#include "settings.h"
#include "tool_draw_text.h"
#include "util.h"


// ### TextObjectEditorHelper ###

TextObjectEditorHelper::TextObjectEditorHelper(TextObject* object, MapEditorController* editor) : object(object), editor(editor)
{
	dragging = false;
	selection_start = 0;
	selection_end = 0;
	original_cursor_retrieved = false;
	
	editor->setEditingInProgress(true);
	editor->getWindow()->setShortcutsBlocked(true);
	
	// Show dock in floating state
	dock_widget = new TextObjectAlignmentDockWidget(object, (int)object->getHorizontalAlignment(), (int)object->getVerticalAlignment(), this, editor->getWindow());
	dock_widget->setFloating(true);
	dock_widget->show();
	dock_widget->resize(0, 0);
	dock_widget->setGeometry(editor->getWindow()->geometry().left() + 40, editor->getWindow()->geometry().top() + 100, dock_widget->width(), dock_widget->height());
	connect(dock_widget, SIGNAL(alignmentChanged(int,int)), this, SLOT(alignmentChanged(int,int)));
	
	// HACK to set the focus to an map widget again after it seems to get completely lost by adding the dock widget (on X11, at least)
	timer = new QTimer();
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(setFocus()));
	timer->start(20);
}
TextObjectEditorHelper::~TextObjectEditorHelper()
{
	delete timer;
	delete dock_widget;
	
	editor->setEditingInProgress(false);
	editor->getWindow()->setShortcutsBlocked(false);
}
void TextObjectEditorHelper::setFocus()
{
	editor->getWindow()->activateWindow();
	editor->getWindow()->setFocus();
	delete timer;
	timer = NULL;
}

bool TextObjectEditorHelper::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	click_position = object->calcTextPositionAt(map_coord, false);
	if (click_position >= 0)
	{
		if (selection_end != click_position || selection_start != click_position)
		{
			selection_start = click_position;
			selection_end = click_position;
			emit(selectionChanged(false));
		}
		
		dragging = false;
		return true;
	}

	return false;
}
bool TextObjectEditorHelper::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	bool mouse_down = event->buttons() & Qt::LeftButton;
	if (!mouse_down)
	{
		if (object->calcTextPositionAt(map_coord, true) >= 0)
		{
			if (!original_cursor_retrieved)
			{
				original_cursor = widget->cursor();
				original_cursor_retrieved = true;
			}
			widget->setCursor(Qt::IBeamCursor);
		}
		else
			widget->setCursor(original_cursor);
	}
	else
	{
		dragging = true;
		updateDragging(map_coord);
	}
	
	return true;
}
bool TextObjectEditorHelper::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (dragging)
	{
		updateDragging(map_coord);
		dragging = false;
		return true;
	}
	else if (object->calcTextPositionAt(map_coord, true) >= 0)
		return true;

	return false;
}

bool TextObjectEditorHelper::keyPressEvent(QKeyEvent* event)
{
// FIXME: repair weird Shift+Left/Right - needs some cursor position
	if (event->key() == Qt::Key_Backspace)
	{
		QString text = object->getText();
		
		if (selection_end == 0)
			return false;
		if (selection_end != selection_start)
		{
			text.remove(selection_start, selection_end - selection_start);
			selection_end = selection_start;
		}
		else
		{
			text.remove(selection_start - 1, 1);
			--selection_end;
			--selection_start;
		}
		object->setText(text);
		emit(selectionChanged(true));
	}
	else if (event->key() == Qt::Key_Delete)
	{
		QString text = object->getText();
		
		if (selection_start == text.size())
			return false;
		if (selection_end != selection_start)
		{
			text.remove(selection_start, selection_end - selection_start);
			selection_end = selection_start;
		}
		else
			text.remove(selection_start, 1);
		object->setText(text);
		emit(selectionChanged(true));
	}
	else if (event->matches(QKeySequence::MoveToPreviousChar) || event->matches(QKeySequence::SelectPreviousChar))
	{
		if (selection_start == 0)
		{
			if (selection_end != 0)
			{
				selection_end = 0;
				emit(selectionChanged(false));
			}
			return true;
		}
		--selection_start;
		if (!event->matches(QKeySequence::SelectPreviousChar))
			selection_end = selection_start;
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::MoveToNextChar) || event->matches(QKeySequence::SelectNextChar))
	{
		if (selection_end == object->getText().length())
		{
			if (selection_start != object->getText().length())
			{
				selection_start = object->getText().length();
				emit(selectionChanged(false));
			}
			return true;
		}
		++selection_end;
		if (!event->matches(QKeySequence::SelectNextChar))
			selection_start = selection_end;
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::MoveToPreviousLine) || event->matches(QKeySequence::SelectPreviousLine))
	{
		int line_num = object->findLineForIndex(selection_start);
		TextObjectLineInfo* line_info = object->getLineInfo(line_num);
		if (line_info->start_index == 0)
			return true;
		
		double x = line_info->getX(selection_start);
		TextObjectLineInfo* prev_line_info = object->getLineInfo(line_num-1);
		double y = prev_line_info->line_y;
		x = qMax( prev_line_info->line_x, qMin(x, prev_line_info->line_x + prev_line_info->width));
		
		selection_start = object->calcTextPositionAt(QPointF(x,y), false);
		if (!event->matches(QKeySequence::SelectPreviousLine))
			selection_end = selection_start;
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::MoveToNextLine) || event->matches(QKeySequence::SelectNextLine))
	{
		int line_num = object->findLineForIndex(selection_end);
		TextObjectLineInfo* line_info = object->getLineInfo(line_num);
		if (line_info->end_index >= object->getText().length())
			return true;
		
		double x = line_info->getX(selection_end);
		TextObjectLineInfo* next_line_info = object->getLineInfo(line_num+1);
		double y = next_line_info->line_y;
		x = qMax( next_line_info->line_x, qMin(x, next_line_info->line_x + next_line_info->width));
		
		selection_end = object->calcTextPositionAt(QPointF(x,y), false);
		if (!event->matches(QKeySequence::SelectNextLine))
			selection_start = selection_end;
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::MoveToStartOfLine) || event->matches(QKeySequence::SelectStartOfLine) ||
		     event->matches(QKeySequence::MoveToStartOfDocument) || event->matches(QKeySequence::SelectStartOfDocument))
	{
		int destination = (event->matches(QKeySequence::MoveToStartOfDocument) || event->matches(QKeySequence::SelectStartOfDocument)) ?
		                      0 : (object->findLineInfoForIndex(selection_start).start_index);
		if (event->matches(QKeySequence::SelectStartOfLine) || event->matches(QKeySequence::SelectStartOfDocument))
		{
			if (selection_start == destination)
				return true;
			selection_start = destination;
		}
		else
		{
			if (selection_end == destination)
				return true;
			selection_start = destination;
			selection_end = destination;
		}
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::MoveToEndOfLine) || event->matches(QKeySequence::SelectEndOfLine) ||
		     event->matches(QKeySequence::MoveToEndOfDocument) || event->matches(QKeySequence::SelectEndOfDocument))
	{
		int destination;
		if (event->matches(QKeySequence::MoveToEndOfDocument) || event->matches(QKeySequence::SelectEndOfDocument))
			destination = object->getText().length();
		else
			destination = object->findLineInfoForIndex(selection_start).end_index;
		
		if (event->matches(QKeySequence::SelectEndOfLine) || event->matches(QKeySequence::SelectEndOfDocument))
		{
			if (selection_end == destination)
				return true;
			selection_end = destination;
		}
		else
		{
			if (selection_start == destination)
				return true;
			selection_start = destination;
			selection_end = destination;
		}
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::SelectAll))
	{
		if (selection_start == 0 && selection_end == object->getText().length())
			return true;
		selection_start = 0;
		selection_end = object->getText().length();
		emit(selectionChanged(false));
	}
	else if (event->matches(QKeySequence::Copy) || event->matches(QKeySequence::Cut))
	{
		if (selection_end - selection_start > 0)
		{
			QClipboard* clipboard = QApplication::clipboard();
			clipboard->setText(object->getText().mid(selection_start, selection_end - selection_start));
			
			if (event->matches(QKeySequence::Cut))
				insertText(QString{});
		}
	}
	else if (event->matches(QKeySequence::Paste))
	{
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mime_data = clipboard->mimeData();
		
		if (mime_data->hasText())
			insertText(clipboard->text());
	}
	else if (event->key() == Qt::Key_Tab)
		insertText(QString(QLatin1Char('\t')));
	else if (event->key() == Qt::Key_Return)
		insertText(QString(QLatin1Char('\n')));
	else if (!event->text().isEmpty() && event->text()[0].isPrint() )
		insertText(event->text());
	else
		return false;
	return true;
}

bool TextObjectEditorHelper::keyReleaseEvent(QKeyEvent* event)
{
	Q_UNUSED(event);
	// Nothing ... yet?
	return false;
}

void TextObjectEditorHelper::draw(QPainter* painter, MapWidget* widget)
{
	Q_UNUSED(widget);
	
	// Draw selection overlay
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(0, 0, 255)));
	painter->setOpacity(0.55f);
	painter->setTransform(object->calcTextToMapTransform(), true);
	
	QRectF selection_rect;
	int line = 0;
	while (getNextLinesSelectionRect(line, selection_rect))
		painter->drawRect(selection_rect);
}
void TextObjectEditorHelper::includeDirtyRect(QRectF& rect)
{
	QTransform transform = object->calcTextToMapTransform();
	QRectF selection_rect;
	int line = 0;
	while (getNextLinesSelectionRect(line, selection_rect))
		rectIncludeSafe(rect, transform.mapRect(selection_rect));
}

void TextObjectEditorHelper::alignmentChanged(int horz, int vert)
{
	object->setHorizontalAlignment((TextObject::HorizontalAlignment)horz);
	object->setVerticalAlignment((TextObject::VerticalAlignment)vert);
	emit(selectionChanged(false));
}

void TextObjectEditorHelper::insertText(QString insertion)
{
	QString text = object->getText();
	
	text = text.replace(selection_start, selection_end - selection_start, insertion);
	selection_start += insertion.length();
	selection_end = selection_start;
	
	object->setText(text);
	emit(selectionChanged(true));
}
void TextObjectEditorHelper::updateDragging(MapCoordF map_coord)
{
	int drag_position = object->calcTextPositionAt(map_coord, false);
	if (drag_position >= 0)
	{
		int new_start = qMin(click_position, drag_position);
		int new_end = qMax(click_position, drag_position);
		if (selection_end != new_end || selection_start != new_start)
		{
			selection_start = new_start;
			selection_end = new_end;
			emit(selectionChanged(false));
		}
	}
}
bool TextObjectEditorHelper::getNextLinesSelectionRect(int& line, QRectF& out)
{
	for (; line < object->getNumLines(); ++line)
	{
		TextObjectLineInfo* line_info = object->getLineInfo(line);
		if (line_info->end_index + 1 < selection_start)
			continue;
		if (selection_end < line_info->start_index)
			break;
		
		int start_index = qMax(selection_start, line_info->start_index);
		int end_index = qMax(qMin(line_info->end_index, selection_end), line_info->start_index);
		
		float left, right;
		if (start_index == end_index)
		{
			// the regular cursor
// FIXME: force minimum visible size 
			float delta = 0.045f * line_info->ascent;
			left = line_info->getX(start_index) - delta;
			right = left + delta + delta;
		}
		else
		{
			left = line_info->getX(start_index);
			right = line_info->getX(end_index);
		}
		
		++line;
		out = QRectF(left, line_info->line_y - line_info->ascent, right - left, line_info->ascent + line_info->descent);
		return true;
	}
	return false;
}


// ### ConstrainAngleToolHelper ###

ConstrainAngleToolHelper::ConstrainAngleToolHelper()
 : active_angle(-1),
   center(MapCoordF(0, 0)),
   active(true),
   have_default_angles_only(false)
{
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
}

ConstrainAngleToolHelper::ConstrainAngleToolHelper(const MapCoordF& center)
 : active_angle(-1),
   center(center),
   active(true),
   have_default_angles_only(false)
{
}

ConstrainAngleToolHelper::~ConstrainAngleToolHelper()
{
	// nothing, not inlined!
}

void ConstrainAngleToolHelper::setCenter(const MapCoordF& center)
{
	if (this->center != center)
	{
		this->center = center;
		emit displayChanged();
	}
}
void ConstrainAngleToolHelper::addAngle(double angle)
{
	angle = fmod(angle, 2*M_PI);
	if (angle < 0)
	{
		angle = 2*M_PI + angle;
		if (angle == 2*M_PI)
			angle = 0;	// can happen because of numerical inaccuracy
	}
	
	// Fuzzy compare for existing angle as otherwise very similar angles will be added
	for (std::set<double>::const_iterator it = angles.begin(), end = angles.end(); it != end; ++it)
	{
		const double min_delta = 0.25 * 2*M_PI / 360;	// minimum 1/4 degree difference
		if (qAbs(angle - *it) < min_delta || qAbs(angle - *it) > 2*M_PI - min_delta)
			return;
	}
	
	angles.insert(angle);
	have_default_angles_only = false;
}
void ConstrainAngleToolHelper::addAngles(double base, double stepping)
{
	for (double angle = base; angle < base + 2*M_PI; angle += stepping)
		addAngle(angle);
}
void ConstrainAngleToolHelper::addAnglesDeg(double base, double stepping)
{
	for (double angle = base; angle < base + 360; angle += stepping)
		addAngle(angle * M_PI / 180);
}
void ConstrainAngleToolHelper::addDefaultAnglesDeg(double base)
{
	addAnglesDeg(base, Settings::getInstance().getSettingCached(Settings::MapEditor_FixedAngleStepping).toDouble());
	have_default_angles_only = true;
	default_angles_base = base;
}
void ConstrainAngleToolHelper::clearAngles()
{
	angles.clear();
	have_default_angles_only = false;
	if (active_angle != -1)
	{
		active_angle = -1;
		emitActiveAngleChanged();
	}
}

double ConstrainAngleToolHelper::getConstrainedCursorPos(const QPoint& in_pos, QPointF& out_pos, MapWidget* widget)
{
	MapCoordF in_pos_map = widget->viewportToMapF(in_pos);
	MapCoordF out_pos_map;
	double angle = getConstrainedCursorPosMap(in_pos_map, out_pos_map);
	out_pos = widget->mapToViewport(out_pos_map);
	return angle;
}
double ConstrainAngleToolHelper::getConstrainedCursorPosMap(const MapCoordF& in_pos, MapCoordF& out_pos)
{
	MapCoordF to_cursor = in_pos - center;
	double in_angle = -1 * to_cursor.angle();
	if (!active)
	{
		out_pos = in_pos;
		return in_angle;
	}
	
	double lower_angle = in_angle, lower_angle_delta = 999;
	double higher_angle = in_angle, higher_angle_delta = -999;
	for (std::set<double>::const_iterator it = angles.begin(), end = angles.end(); it != end; ++it)
	{
		double delta = in_angle - *it;
		if (delta < -M_PI)
			delta = in_angle - (*it - 2*M_PI);
		else if (delta > M_PI)
			delta = (in_angle - 2*M_PI) - *it;
		
		if (delta > 0)
		{
			if (delta < lower_angle_delta)
			{
				lower_angle_delta = delta;
				lower_angle = *it;
			}
		}
		else
		{
			if (delta > higher_angle_delta)
			{
				higher_angle_delta = delta;
				higher_angle = *it;
			}
		}
	}
	
	double new_active_angle;
	if (lower_angle_delta < -1 * higher_angle_delta)
		new_active_angle = lower_angle;
	else
		new_active_angle = higher_angle;
	if (new_active_angle != active_angle)
	{
		emitActiveAngleChanged();
		active_angle = new_active_angle;
	}
	
	MapCoordF unit_direction = MapCoordF(cos(active_angle), -sin(active_angle));
	to_cursor = MapCoordF::dotProduct(unit_direction, to_cursor) * unit_direction;
	out_pos = center + to_cursor;
	
	return active_angle;
}
double ConstrainAngleToolHelper::getConstrainedCursorPositions(const MapCoordF& in_pos_map, MapCoordF& out_pos_map, QPointF& out_pos, MapWidget* widget)
{
	double angle = getConstrainedCursorPosMap(in_pos_map, out_pos_map);
	out_pos = widget->mapToViewport(out_pos_map);
	return angle;
}

void ConstrainAngleToolHelper::setActive(bool active, const MapCoordF& center)
{
	if (active)
	{
		if (!this->active || (this->center != center))
			emit displayChanged();
		this->center = center;
	}
	else
	{
		if (active_angle != -1)
		{
			active_angle = -1;
			emitActiveAngleChanged();
		}
		else
			emit displayChanged();
	}
	this->active = active;
}
void ConstrainAngleToolHelper::setActive(bool active)
{
	if (active)
	{
		if (!this->active)
			emit displayChanged();
	}
	else
	{
		if (active_angle != -1)
		{
			active_angle = -1;
			emitActiveAngleChanged();
		}
		else
			emit displayChanged();
	}
	this->active = active;
}

void ConstrainAngleToolHelper::draw(QPainter* painter, MapWidget* widget)
{
	const float opacity = 0.5f;
	
	if (!active) return;
	QPointF center_point = widget->mapToViewport(center);
	painter->setOpacity(opacity);
	painter->setPen(MapEditorTool::inactive_color);
	painter->setBrush(Qt::NoBrush);
	for (std::set<double>::const_iterator it = angles.begin(), end = angles.end(); it != end; ++it)
	{
		if (*it == active_angle)
		{
			painter->setPen(MapEditorTool::active_color);
			painter->setOpacity(1.0f);
		}
		
		QPointF outer_point = center_point + getDisplayRadius() * QPointF(cos(*it), -sin(*it));
		painter->drawLine(center_point, outer_point);
		
		if (*it == active_angle)
		{
			painter->setPen(MapEditorTool::inactive_color);
			painter->setOpacity(opacity);
		}
	}
	painter->setOpacity(1.0f);
}
void ConstrainAngleToolHelper::includeDirtyRect(QRectF& rect)
{
	if (!active) return;
	rectIncludeSafe(rect, center);
}

void ConstrainAngleToolHelper::settingsChanged()
{
	if (have_default_angles_only)
	{
		clearAngles();
		addDefaultAnglesDeg(default_angles_base);
	}
}


// ### SnappingToolHelper ###

SnappingToolHelper::SnappingToolHelper(MapEditorTool* tool, SnapObjects filter)
 : QObject(NULL),
   filter(filter),
   snapped_type(NoSnapping),
   map(tool->map()),
   point_handles(tool->scaleFactor())
{
}

void SnappingToolHelper::setFilter(SnapObjects filter)
{
	this->filter = filter;
	if (!(filter & snapped_type))
		snapped_type = NoSnapping;
}
SnappingToolHelper::SnapObjects SnappingToolHelper::getFilter() const
{
	return filter;
}

MapCoord SnappingToolHelper::snapToObject(MapCoordF position, MapWidget* widget, SnappingToolHelperSnapInfo* info, Object* exclude_object, float snap_distance)
{
	if (snap_distance < 0)
		snap_distance = 0.001f * widget->getMapView()->pixelToLength(Settings::getInstance().getMapEditorSnapDistancePx());
	float closest_distance_sq = snap_distance * snap_distance;
	auto result_position = MapCoord { position };
	SnappingToolHelperSnapInfo result_info;
	result_info.type = NoSnapping;
	result_info.object = NULL;
	result_info.coord_index = -1;
	result_info.path_coord.pos = MapCoordF(0, 0);
	result_info.path_coord.index = -1;
	result_info.path_coord.clen = -1;
	result_info.path_coord.param = -1;
	
	if (filter & (ObjectCorners | ObjectPaths))
	{
		// Find map objects at the given position
		SelectionInfoVector objects;
		map->findAllObjectsAt(position, snap_distance, true, false, false, true, objects);
		
		// Find closest snap spot from map objects
		for (SelectionInfoVector::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		{
			Object* object = it->second;
			if (object == exclude_object)
				continue;
			
			float distance_sq;
			if (object->getType() == Object::Point && filter & ObjectCorners)
			{
				PointObject* point = object->asPoint();
				distance_sq = point->getCoordF().distanceSquaredTo(position);
				if (distance_sq < closest_distance_sq)
				{
					closest_distance_sq = distance_sq;
					result_position = point->getCoord();
					result_info.type = ObjectCorners;
					result_info.object = object;
					result_info.coord_index = 0;
				}
			}
			else if (object->getType() == Object::Path)
			{
				PathObject* path = object->asPath();
				if (filter & ObjectPaths)
				{
					PathCoord path_coord;
					path->calcClosestPointOnPath(position, distance_sq, path_coord);
					if (distance_sq < closest_distance_sq)
					{
						closest_distance_sq = distance_sq;
						result_position = MapCoord(path_coord.pos);
						result_info.object = object;
						if (path_coord.param == 0.0)
						{
							result_info.type = ObjectCorners;
							result_info.coord_index = path_coord.index;
						}
						else
						{
							result_info.type = ObjectPaths;
							result_info.coord_index = -1;
							result_info.path_coord = path_coord;
						}
					}
				}
				else
				{
					MapCoordVector::size_type index;
					path->calcClosestCoordinate(position, distance_sq, index);
					if (distance_sq < closest_distance_sq)
					{
						closest_distance_sq = distance_sq;
						result_position = path->getCoordinate(index);
						result_info.type = ObjectCorners;
						result_info.object = object;
						result_info.coord_index = index;
					}
				}
			}
			else if (object->getType() == Object::Text)
			{
				// No snapping to texts
				continue;
			}
		}
	}
	
	// Find closest grid snap position
	if ((filter & GridCorners) && widget->getMapView()->isGridVisible() &&
		map->getGrid().isSnappingEnabled() && map->getGrid().getDisplayMode() == MapGrid::AllLines)
	{
		MapCoordF closest_grid_point = map->getGrid().getClosestPointOnGrid(position, map);
		float distance_sq = closest_grid_point.distanceSquaredTo(position);
		if (distance_sq < closest_distance_sq)
		{
			closest_distance_sq = distance_sq;
			result_position = MapCoord(closest_grid_point);
			result_info.type = GridCorners;
			result_info.object = NULL;
			result_info.coord_index = -1;
		}
	}
	
	// Return
	if (snap_mark != result_position || snapped_type != result_info.type)
	{
		snap_mark = result_position;
		snapped_type = result_info.type;
		emit displayChanged();
	}
	
	if (info != NULL)
		*info = result_info;
	return result_position;
}

bool SnappingToolHelper::snapToDirection(MapCoordF position, MapWidget* widget, ConstrainAngleToolHelper* angle_tool, MapCoord* out_snap_position)
{
	// As getting a direction from the map grid is not supported, remove grid from filter
	int filter_grid = filter & GridCorners;
	filter = (SnapObjects)(filter & ~filter_grid);
	
	// Snap to position
	SnappingToolHelperSnapInfo info;
	MapCoord snap_position = snapToObject(position, widget, &info);
	if (out_snap_position)
		*out_snap_position = snap_position;
	
	// Add grid to filter again, if it was there originally
	filter = (SnapObjects)(filter | filter_grid);
	
	// Get direction from result
	switch (info.type)
	{
	case ObjectCorners:
		if (info.object->getType() == Object::Point)
		{
			const PointObject* point = info.object->asPoint();
			angle_tool->clearAngles();
			angle_tool->addAngles(point->getRotation() - M_PI/2, M_PI/2);
			return true;
		}
		else if (info.object->getType() == Object::Path)
		{
			const PathObject* path = info.object->asPath();
			angle_tool->clearAngles();
			bool ok;
			// Forward tangent
			MapCoordF tangent = path->findPartForIndex(info.coord_index)->calculateTangent(info.coord_index, false, ok);
			if (ok)
				angle_tool->addAngles(-tangent.angle(), M_PI/2);
			// Backward tangent
			tangent = path->findPartForIndex(info.coord_index)->calculateTangent(info.coord_index, true, ok);
			if (ok)
				angle_tool->addAngles(-tangent.angle(), M_PI/2);
			return true;
		}
		return false;
		
	case ObjectPaths:
		{
			const PathObject* path = info.object->asPath();
			angle_tool->clearAngles();
			auto part   = path->findPartForIndex(info.path_coord.index);
			auto split  = SplitPathCoord::at(part->path_coords, info.path_coord.clen);
			auto right = split.tangentVector().perpRight();
			angle_tool->addAngles(-right.angle(), M_PI/2);
		}
		return true;
		
	default:
		return false;
	}
}

void SnappingToolHelper::draw(QPainter* painter, MapWidget* widget)
{
	if (snapped_type != NoSnapping)
	{
		point_handles.draw( painter, widget->mapToViewport(snap_mark),
									   (snapped_type == ObjectPaths) ? PointHandles::NormalHandle : PointHandles::EndHandle,
									   PointHandles::NormalHandleState );
	}
}

void SnappingToolHelper::includeDirtyRect(QRectF& rect)
{
	if (snapped_type != NoSnapping)
		rectIncludeSafe(rect, QPointF(snap_mark));
}


// ### FollowPathToolHelper ###

FollowPathToolHelper::FollowPathToolHelper()
: path{nullptr}
{
	// nothing else
}

void FollowPathToolHelper::startFollowingFromCoord(const PathObject* path, MapCoordVector::size_type coord_index)
{
	path->update();
	PathCoord coord = path->findPathCoordForIndex(coord_index);
	startFollowingFromPathCoord(path, coord);
}

void FollowPathToolHelper::startFollowingFromPathCoord(const PathObject* path, const PathCoord& coord)
{
	path->update();
	this->path = path;
	
	start_clen = coord.clen;
	end_clen   = start_clen;
	part_index = path->findPartIndexForIndex(coord.index);
	drag_forward = true;
}

std::unique_ptr<PathObject> FollowPathToolHelper::updateFollowing(const PathCoord& end_coord)
{
	std::unique_ptr<PathObject> result;
	
	if (path && path->findPartIndexForIndex(end_coord.index) == part_index)
	{
		// Update end_clen
		auto new_end_clen = end_coord.clen;
		const auto& part = path->parts()[part_index];
		if (part.isClosed())
		{
			// Positive length to add to end_clen to get to new_end_clen with wrapping
			auto path_length = part.length();
			auto forward_diff = fmod_pos(new_end_clen - end_clen, path_length);
			auto delta_forward = forward_diff >= 0 && forward_diff < 0.5f * path_length;
			
			if (delta_forward
			    && !drag_forward
			    && fmod_pos(end_clen - start_clen, path_length) > 0.5f * path_length
			    && fmod_pos(new_end_clen - start_clen, path_length) <= 0.5f * path_length )
			{
				drag_forward = true;
			}
			else if (!delta_forward
			         && drag_forward
			         && fmod_pos(end_clen - start_clen, path_length) <= 0.5f * path_length
			         && fmod_pos(new_end_clen - start_clen, path_length) > 0.5f * path_length)
			{
				drag_forward = false;
			}
		}
		else
		{
			drag_forward = new_end_clen >= start_clen;
		}
		
		end_clen = new_end_clen;
		if (end_clen != start_clen)
		{
			// Create output path
			result.reset(new PathObject { part });
			if (drag_forward)
			{
				result->changePathBounds(0, start_clen, end_clen);
			}
			else
			{
				result->changePathBounds(0, end_clen, start_clen);
				result->reverse();
			}
		}
	}
	
	return result;
}
