/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2016 Kai Pastor
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


#include "text_object_editor_helper.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include "../map_editor.h"
#include "../map_widget.h"
#include "../object_text.h"
#include "../util.h"
#include "../gui/main_window.h"
#include "../gui/widgets/text_alignment_widget.h"


TextObjectEditorHelper::TextObjectEditorHelper(TextObject* object, MapEditorController* editor) : object(object), editor(editor)
{
	dragging = false;
	selection_start = 0;
	selection_end = 0;
	original_cursor_retrieved = false;
	
	editor->setEditingInProgress(true);
	editor->getWindow()->setShortcutsBlocked(true);
	
	// Show dock in floating state
	dock_widget = new TextObjectAlignmentDockWidget(this, editor->getWindow());
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

