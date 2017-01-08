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

#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "core/objects/text_object.h"
#include "util/util.h"
#include "../gui/main_window.h"
#include "../gui/widgets/text_alignment_widget.h"


TextObjectEditorHelper::TextObjectEditorHelper(TextObject* text_object, MapEditorController* editor)
: text_object { text_object }
, editor { editor }
, dock_widget { nullptr }
, anchor_position { 0 }
, cursor_position { 0 }
, line_selection_position { 0 }
, dragging { false }
, text_cursor_active { false }
{
	Q_ASSERT(text_object);
	Q_ASSERT(editor);
	
	auto window = editor->getWindow();
	window->setShortcutsBlocked(true);
	
	// Show dock in floating state
	dock_widget = new TextObjectAlignmentDockWidget(this, window);
	dock_widget->setFocusProxy(window); // Re-route input events
	dock_widget->setFloating(true);
	dock_widget->show();
	dock_widget->resize(0, 0);
	dock_widget->setGeometry({editor->getMainWidget()->mapTo(window, {}), dock_widget->size()});
	connect(dock_widget, &TextObjectAlignmentDockWidget::alignmentChanged, this, &TextObjectEditorHelper::setTextAlignment);
	connect(this, &QObject::destroyed, dock_widget, &QObject::deleteLater);
	
	// Workaround to set the focus to the map widget again after it was lost
	// to the new dock widget (on X11, at least)
	QTimer::singleShot(20, this, SLOT(claimFocus()));
}


TextObjectEditorHelper::~TextObjectEditorHelper()
{
	dock_widget->hide();
	dock_widget->deleteLater();
	
	editor->getWindow()->setShortcutsBlocked(false);
}


void TextObjectEditorHelper::claimFocus()
{
	auto widget = editor->getMainWidget();
	widget->activateWindow();
	widget->setFocus();
	updateCursor(widget, 0);
}



bool TextObjectEditorHelper::setSelection(int anchor, int cursor, int line_position)
{
	if (anchor != anchor_position
	    || cursor != cursor_position
	    || line_position != line_selection_position)
	{
		anchor_position = anchor;
		cursor_position = cursor;
		line_selection_position = line_position;
		return true;
	}
	return false;
}


QString TextObjectEditorHelper::selectionText() const
{
	const auto start = qMin(anchor_position, cursor_position);
	const auto length = qAbs(anchor_position - cursor_position);
	return text_object->getText().mid(start, length);
}


void TextObjectEditorHelper::replaceSelectionText(const QString& replacement)
{
	const auto selection_start = qMin(anchor_position, cursor_position);
	const auto selection_length = qAbs(anchor_position - cursor_position);
	
	const auto new_pos = selection_start + replacement.length();
	auto selection_changed = setSelection(new_pos, new_pos);
	
	auto text = text_object->getText();
	text.replace(selection_start, selection_length, replacement);
	if (text != text_object->getText()
	    || selection_changed)
	{
		text_object->setText(text);
		emit stateChanged();
	}
}



bool TextObjectEditorHelper::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget)
	
	if (event->button() == Qt::LeftButton)
	{
		auto click_pos = text_object->calcTextPositionAt(map_coord, false);
		if (click_pos >= 0)
		{
			auto anchor_pos = click_pos;
			if (event->modifiers() & Qt::ShiftModifier)
				anchor_pos = this->anchor_position;
			if (setSelection(anchor_pos, click_pos))
				emit stateChanged();
			dragging = false;
		}
		else
		{
			emit finished();
		}
		return true;
	}
		
	return false;
}


bool TextObjectEditorHelper::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	updateCursor(widget, text_object->calcTextPositionAt(map_coord, true));
	
	if (event->buttons() & Qt::LeftButton)
	{
		dragging = true;
		updateDragging(map_coord);
		return true;
	}
	
	return false;
}


bool TextObjectEditorHelper::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	Q_UNUSED(widget)
	
	if (event->button() == Qt::LeftButton)
	{
		if (dragging)
		{
			updateDragging(map_coord);
			dragging = false;
		}
		return true;
	}
	
	return false;
}



bool TextObjectEditorHelper::keyPressEvent(QKeyEvent* event)
{
	const auto text = text_object->getText();
	
	if (event->key() == Qt::Key_Backspace)
	{
		if (cursor_position != anchor_position)
		{
			replaceSelectionText({});
		}
		else if (cursor_position > 0)
		{
			setSelection(cursor_position-1, cursor_position);
			replaceSelectionText({});
		}
	}
	else if (event->key() == Qt::Key_Delete)
	{
		if (cursor_position != anchor_position)
		{
			replaceSelectionText({});
		}
		else if (cursor_position < text.size())
		{
			setSelection(cursor_position+1, cursor_position);
			replaceSelectionText({});
		}
	}
	else if (event->matches(QKeySequence::MoveToPreviousChar))
	{
		const auto new_pos = qMax(0, cursor_position - 1);
		if (setSelection(new_pos, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectPreviousChar))
	{
		const auto new_pos = qMax(0, cursor_position - 1);
		if (setSelection(anchor_position, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToNextChar))
	{
		const auto new_pos = qMin(text.length(), cursor_position + 1);
		if (setSelection(new_pos, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectNextChar))
	{
		const auto new_pos = qMin(text.length(), cursor_position + 1);
		if (setSelection(anchor_position, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToPreviousLine)
	         || event->matches(QKeySequence::SelectPreviousLine))
	{
		const auto first_line = text_object->getLineInfo(0);
		if (first_line->end_index >= cursor_position)
			return true;
		
		const auto start_line_num = text_object->findLineForIndex(line_selection_position);
		const auto start_line = text_object->getLineInfo(start_line_num);
		
		const auto prev_line_num = text_object->findLineForIndex(cursor_position) - 1;
		const auto prev_line = text_object->getLineInfo(prev_line_num);
		const auto point = QPointF {
		  qBound(prev_line->line_x, start_line->getX(line_selection_position), prev_line->line_x + prev_line->width),
		  prev_line->line_y
		};
		const auto new_pos = text_object->calcTextPositionAt(point, false);
		const auto new_anchor = event->matches(QKeySequence::SelectPreviousLine) ? anchor_position : new_pos;
		if (setSelection(new_anchor, new_pos, line_selection_position))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToNextLine)
	         || event->matches(QKeySequence::SelectNextLine))
	{
		const auto last_line = text_object->getLineInfo(text_object->getNumLines() - 1);
		if (last_line->start_index <= cursor_position)
			return true;
		
		const auto start_line_num = text_object->findLineForIndex(line_selection_position);
		const auto start_line = text_object->getLineInfo(start_line_num);
		
		const auto next_line_num = text_object->findLineForIndex(cursor_position) + 1;
		const auto next_line = text_object->getLineInfo(next_line_num);
		const auto point = QPointF {
		  qBound(next_line->line_x, start_line->getX(line_selection_position), next_line->line_x + next_line->width),
		  next_line->line_y
		};
		const auto new_pos = text_object->calcTextPositionAt(point, false);
		const auto new_anchor = event->matches(QKeySequence::SelectNextLine) ? anchor_position : new_pos;
		if (setSelection(new_anchor, new_pos, line_selection_position))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToStartOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).start_index;
		if (setSelection(new_pos, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectStartOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).start_index;
		if (setSelection(anchor_position, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToStartOfDocument))
	{
		if (setSelection(0, 0))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectStartOfDocument))
	{
		if (setSelection(anchor_position, 0))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToEndOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).end_index;
		if (setSelection(new_pos, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectEndOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).end_index;
		if (setSelection(anchor_position, new_pos))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::MoveToEndOfDocument))
	{
		if (setSelection(text.length(), text.length()))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectEndOfDocument))
	{
		if (setSelection(anchor_position, text.length()))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::SelectAll))
	{
		if (setSelection(0, text.length()))
			emit stateChanged();
	}
	else if (event->matches(QKeySequence::Copy)
	         || event->matches(QKeySequence::Cut))
	{
		auto selection = selectionText();
		if (!selection.isEmpty())
		{
			QClipboard* clipboard = QApplication::clipboard();
			clipboard->setText(selection);
			
			if (event->matches(QKeySequence::Cut))
				replaceSelectionText({});
		}
	}
	else if (event->matches(QKeySequence::Paste))
	{
		const auto clipboard = QApplication::clipboard();
		const auto mime_data = clipboard->mimeData();
		
		if (mime_data->hasText())
			replaceSelectionText(clipboard->text());
	}
	else if (event->key() == Qt::Key_Tab)
	{
		replaceSelectionText(QString(QLatin1Char('\t')));
	}
	else if (event->key() == Qt::Key_Return)
	{
		replaceSelectionText(QString(QLatin1Char('\n')));
	}
	else if (!event->text().isEmpty()
	         && event->text()[0].isPrint() )
	{
		replaceSelectionText(event->text());
	}
	
	return true;
}


bool TextObjectEditorHelper::keyReleaseEvent(QKeyEvent* event)
{
	Q_UNUSED(event)
	
	return true;
}



void TextObjectEditorHelper::draw(QPainter* painter, MapWidget* widget)
{
	Q_UNUSED(widget)
	
	// Draw selection overlay
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(0, 0, 255)));
	painter->setOpacity(0.55);
	painter->setTransform(text_object->calcTextToMapTransform(), true);
	
	foreachLineSelectionRect([painter](const QRectF& selection_rect) { 
		painter->drawRect(selection_rect);
	});
}


void TextObjectEditorHelper::includeDirtyRect(QRectF& rect) const
{
	const auto transform = text_object->calcTextToMapTransform();
	foreachLineSelectionRect([&transform, &rect](const QRectF& selection_rect) {
		rectIncludeSafe(rect, transform.mapRect(selection_rect));
	});
}



void TextObjectEditorHelper::setTextAlignment(int h_alignment, int v_alignment)
{
	if (int(text_object->getHorizontalAlignment()) != h_alignment
	    || int(text_object->getVerticalAlignment()) != v_alignment)
	{
		text_object->setHorizontalAlignment(TextObject::HorizontalAlignment(h_alignment));
		text_object->setVerticalAlignment(TextObject::VerticalAlignment(v_alignment));
		emit stateChanged();
	}
}



void TextObjectEditorHelper::updateCursor(QWidget* widget, int position)
{
	if (position >= 0 && !text_cursor_active)
	{
		widget->setCursor({Qt::IBeamCursor});
		text_cursor_active = true;
	}
	else if (position < 0 && text_cursor_active)
	{
		widget->setCursor({});
		text_cursor_active = false;
	}
}



void TextObjectEditorHelper::updateDragging(MapCoordF map_coord)
{
	const auto drag_position = text_object->calcTextPositionAt(map_coord, false);
	if (drag_position >= 0 && setSelection(anchor_position, drag_position))
	{
		emit stateChanged();
	}
}



void TextObjectEditorHelper::foreachLineSelectionRect(std::function<void(const QRectF&)> worker) const
{
	const auto selection_start = qMin(anchor_position, cursor_position);
	const auto selection_end   = qMax(anchor_position, cursor_position);
	for (int line = 0, end = text_object->getNumLines(); line != end; ++line)
	{
		const auto line_info = text_object->getLineInfo(line);
		if (line_info->end_index + 1 < selection_start)
			continue;
		if (selection_end < line_info->start_index)
			break;
		
		const auto start_index = qMax(selection_start, line_info->start_index);
		/// \todo Check for simplification, qMin(selection_end, line_info->end_index)
		const auto end_index = qMax(qMin(line_info->end_index, selection_end), line_info->start_index);
		
		const auto delta = 0.045 * line_info->ascent;
		auto left = line_info->getX(start_index);
		auto width = delta * 2;
		if (start_index == end_index)
		{
			// the regular cursor
			/// \todo force minimum visible size 
			left -= delta;
		}
		else
		{
			width = line_info->getX(end_index) - left;
		}
		
		worker({left, line_info->line_y - line_info->ascent, width, line_info->ascent + line_info->descent});
	}
}

