/*
 *    Copyright 2012 Thomas Schöps
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
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

#include "map_editor.h"
#include "tool_draw_text.h"
#include "object_text.h"
#include "map_widget.h"
#include "util.h"
#include "settings.h"


// ### TextObjectEditorHelper ###

TextObjectEditorHelper::TextObjectEditorHelper(TextObject* object, MapEditorController* editor) : object(object), editor(editor)
{
	dragging = false;
	selection_start = 0;
	selection_end = 0;
	original_cursor_retrieved = false;
	
	editor->setEditingInProgress(true);
	editor->getWindow()->setShortcutsEnabled(false);
	
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
	editor->getWindow()->setShortcutsEnabled(true);
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
				insertText("");
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
		insertText("\t");
	else if (event->key() == Qt::Key_Return)
		insertText("\n");
	else if (!event->text().isEmpty() && event->text()[0].isPrint() )
		insertText(event->text());
	else
		return false;
	return true;
}

bool TextObjectEditorHelper::keyReleaseEvent(QKeyEvent* event)
{
	// Nothing ... yet?
	return false;
}

void TextObjectEditorHelper::draw(QPainter* painter, MapWidget* widget)
{
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
	double in_angle = -1 * to_cursor.getAngle();
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
	to_cursor = unit_direction.dot(to_cursor) * unit_direction;
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
		if (!this-active || (this->center != center))
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
		if (!this-active)
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
	rectIncludeSafe(rect, center.toQPointF());
}

void ConstrainAngleToolHelper::settingsChanged()
{
	if (have_default_angles_only)
	{
		clearAngles();
		addDefaultAnglesDeg(default_angles_base);
	}
}
