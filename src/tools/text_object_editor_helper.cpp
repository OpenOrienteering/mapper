/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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

#include <QtGlobal>
#include <QBrush>
#include <QChar>
#include <QClipboard>
#include <QEvent>
#include <QFlags>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLatin1Char>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QStyle>
#include <QTimer>
#include <QTransform>
#include <QVariant>
#include <QWidget>

#include "core/objects/text_object.h"
#include "gui/main_window.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/text_alignment_widget.h"
#include "util/util.h"


#ifdef MAPPER_DEVELOPMENT_BUILD
#  ifdef ANDROID
#    define DEBUG_INPUT_METHOD_SUPPORT
#  endif
#endif
//#  define INPUT_METHOD_BUGS_RESOLVED


namespace OpenOrienteering {

// ### TextObjectEditorHelper::BatchEdit ###

TextObjectEditorHelper::BatchEdit::BatchEdit(not_null<TextObjectEditorHelper*> editor, int actions)
: editor { editor }
{
	Q_ASSERT(editor);
	Q_ASSERT(editor->batch_editing >= 0);
	Q_ASSERT(editor->batch_editing < 5);
	
	++editor->batch_editing;
	if (editor->batch_editing == 1)
	{
		editor->state_change_action = actions;
		if (actions & CommitPreedit)
			editor->commitPreedit();
	}
}


TextObjectEditorHelper::BatchEdit::~BatchEdit()
{
	Q_ASSERT(editor->batch_editing);
	
	--editor->batch_editing;
	if (editor->batch_editing == 0 && editor->dirty)
	{
		editor->commitStateChange();
	}
}



// ### TextObjectEditorHelper ###

TextObjectEditorHelper::TextObjectEditorHelper(not_null<TextObject*> text_object, not_null<MapEditorController*> editor)
: text_object { text_object }
, editor { editor }
, dock_widget { nullptr }
, pristine_text { text_object->getText() }
, block_start { -1 }
, anchor_position { 0 }
, cursor_position { 0 }
, line_selection_position { 0 }
, preedit_cursor { 0 }
, batch_editing { 0 }
, state_change_action { NoInputMethodAction }
, dirty { true }
, dragging { false }
, text_cursor_active { false }
, update_input_method_enabled { true }
{
	Q_ASSERT(text_object);
	Q_ASSERT(editor);
	
	auto window = editor->getWindow();
	window->setShortcutsBlocked(true);
	
	// Show dock in floating state
	dock_widget = new TextObjectAlignmentDockWidget(this, window);
	dock_widget->setFocusProxy(window); // Re-route input events
	dock_widget->setFloating(true);
	dock_widget->resize(0, 0);
	dock_widget->window()->setGeometry({editor->getMainWidget()->mapToGlobal({}), dock_widget->size()});
	dock_widget->show();
	connect(dock_widget, &TextObjectAlignmentDockWidget::alignmentChanged, this, &TextObjectEditorHelper::setTextAlignment);
	connect(this, &QObject::destroyed, dock_widget, &QObject::deleteLater);
	
	// When the app is activated again, re-activate the input method.
	connect(qApp, &QGuiApplication::applicationStateChanged, this, &TextObjectEditorHelper::claimFocus, Qt::QueuedConnection);
	
	auto widget = editor->getMainWidget();
	widget->setAttribute(Qt::WA_InputMethodEnabled, true);
	//updateCursor(widget, 0);
	
#ifdef Q_OS_ANDROID
	claimFocus();
#else
	// Workaround to set the focus to the map widget again after it was lost
	// to the new dock widget (on X11, at least)
	QTimer::singleShot(20, this, SLOT(claimFocus()));  // clazy:exclude=old-style-connect
#endif
}


TextObjectEditorHelper::~TextObjectEditorHelper()
{
	dock_widget->hide();
	dock_widget->deleteLater();
	
	QGuiApplication::inputMethod()->hide();
	if (auto widget = editor->getMainWidget())
		widget->setAttribute(Qt::WA_InputMethodEnabled, false);
	if (auto window = editor->getWindow())
		window->setShortcutsBlocked(false);
}


void TextObjectEditorHelper::claimFocus()
{
	if (qApp->applicationState() == Qt::ApplicationActive)
	{
		auto widget = editor->getMainWidget();
		widget->activateWindow();
		widget->setFocus();
	}
}



inline
bool TextObjectEditorHelper::isPreediting() const
{
	return !preedit_string.isEmpty();
}


void TextObjectEditorHelper::commitPreedit()
{
	if (isPreediting())
	{
#ifdef DEBUG_INPUT_METHOD_SUPPORT
		qDebug("\n>>> inputMethod()->commit()");	
#endif
		QGuiApplication::inputMethod()->commit();
		if (Q_UNLIKELY(isPreediting()))
		{
			preedit_string.clear();
			preedit_cursor = 0;
			text_object->setText(pristine_text);
			dirty = true;
		}
	}
}


void TextObjectEditorHelper::commitStateChange()
{
	Q_ASSERT(anchor_position == qBound(0, anchor_position, pristine_text.length()));
	Q_ASSERT(cursor_position == qBound(0, cursor_position, pristine_text.length()));
	Q_ASSERT(line_selection_position == qBound(0, line_selection_position, pristine_text.length()));
	Q_ASSERT(preedit_cursor == qBound(0, preedit_cursor, preedit_string.length()));
	
#ifdef DEBUG_INPUT_METHOD_SUPPORT
	{
		auto surrounding_text = pristine_text.mid(blockStart(), blockEnd() - blockStart());
		qDebug("\n*** @%d:\"%s[%s|%s]%s\"\n    a..c: @+%d..%d, p: c+%d\n    action: %d", blockStart(),
		       qPrintable(pristine_text.mid(blockStart(), cursor_position - blockStart())),
		       qPrintable(preedit_string.left(preedit_cursor)),
		       qPrintable(preedit_string.mid(preedit_cursor)),
		       qPrintable(pristine_text.mid(cursor_position, blockEnd() - blockStart())),
		       anchor_position - blockStart(), cursor_position - blockStart(), preedit_cursor,
		       state_change_action & 0x03);
	}
#endif
	
	dirty = false;
	updateDisplayText();
	if (QGuiApplication::inputMethod()->isVisible())
	{
		auto im_query = Qt::ImQueryAll;
		switch (state_change_action & 0x03)
		{
		case ResetInputMethod:
#ifdef DEBUG_INPUT_METHOD_SUPPORT
			qDebug("\n>>> inputMethod()->reset()");	
#endif
			QGuiApplication::inputMethod()->reset();
			break;
		case UpdateInputProperties:
			im_query = Qt::ImQueryInput;
			// fall through
		case UpdateAllProperties:
#ifdef DEBUG_INPUT_METHOD_SUPPORT
			qDebug("\n>>> inputMethod()->update(%d)", im_query);	
#endif
			QGuiApplication::inputMethod()->update(im_query);
#ifdef Q_OS_ANDROID
			// QTBUG-58013
			Q_UNUSED(im_query)
#  ifdef DEBUG_INPUT_METHOD_SUPPORT
			qDebug("\n>>> cursorPositionChanged");	
#  endif
			editor->getMainWidget()->cursorPositionChanged();
#endif
			break;
		case NoInputMethodAction:
			;
		}
	}
	emit stateChanged();
}


void TextObjectEditorHelper::updateDisplayText()
{
	// Insert the preedit string into the displayed text.
	auto displayed_text = pristine_text;
	displayed_text.insert(cursor_position, preedit_string);
	if (text_object->getText() != displayed_text)
	{
		text_object->setText(displayed_text);
	}
}



bool TextObjectEditorHelper::setSelection(int anchor, int cursor)
{
	return setSelection(anchor, cursor, cursor);
}


bool TextObjectEditorHelper::setSelection(int anchor, int cursor, int line_position)
{
	Q_ASSERT(batch_editing);
	if (anchor != anchor_position
	    || cursor != cursor_position
	    || line_position != line_selection_position)
	{
		if (cursor <= block_start || cursor > cursor_position)
			block_start = -1;
		anchor_position = anchor;
		cursor_position = cursor;
		line_selection_position = line_position;
		dirty = true;
		return true;
	}
	return false;
}


QString TextObjectEditorHelper::selectionText() const
{
	const auto start = qMin(anchor_position, cursor_position);
	const auto length = qAbs(anchor_position - cursor_position);
	return pristine_text.mid(start, length);
}


void TextObjectEditorHelper::replaceSelectionText(const QString& replacement)
{
	Q_ASSERT(batch_editing);
	
	auto const old_text = pristine_text;
	
	auto selection_start = qMin(anchor_position, cursor_position);
	const auto selection_length = qAbs(anchor_position - cursor_position);
	pristine_text.replace(selection_start, selection_length, replacement);
	
	selection_start += replacement.length();
	setSelection(selection_start, selection_start);
	
	if (text_object->getText() != pristine_text
	    || old_text != pristine_text)
	{
		dirty = true;
	}
}


int TextObjectEditorHelper::blockStart() const
{
	if (block_start == -1)
	{
#ifndef INPUT_METHOD_BUGS_RESOLVED
		// There are strange input events when working with block_start > 0.
		block_start = 0;
#else
		for (int line = 0, num_lines = text_object->getNumLines(); line != num_lines; ++line)
		{
			auto line_start = text_object->getLineInfo(line)->start_index;
			if (line_start < cursor_position)
				block_start = line_start;
			else
				break;
		}
		qDebug("\n*** nblock_start: %d", block_start);
#endif
	}
	return block_start;
}


int TextObjectEditorHelper::blockEnd() const
{
	return qMin(pristine_text.length(), qMax(anchor_position, cursor_position) + 200);
}


QVariant TextObjectEditorHelper::inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const
{
	switch (property)
	{
	case Qt::ImHints:
		return { Qt::ImhMultiLine };
	case Qt::ImAnchorPosition:
		return { anchor_position - blockStart() };
	case Qt::ImCursorPosition:
		{
			const auto point = argument.toPointF();
			if (!point.isNull())
			{
				auto map_coord = editor->getMainWidget()->viewportToMapF(point);
				return { text_object->calcTextPositionAt(map_coord, false) - blockStart() };
			}
		}
		return { cursor_position - blockStart() };
	case Qt::ImCursorRectangle:
		return { cursorRectangle() };
	case Qt::ImSurroundingText:
		return pristine_text.mid(blockStart(), blockEnd() - blockStart());
	case Qt::ImCurrentSelection:
		return { selectionText() };
	case Qt::ImMaximumTextLength:
		return { }; // No limit.
#if QT_VERSION >= 0x050300
	case Qt::ImAbsolutePosition:
		{
			const auto point = argument.toPointF();
			if (!point.isNull())
			{
				auto map_coord = editor->getMainWidget()->viewportToMapF(point);
				return { text_object->calcTextPositionAt(map_coord, false) };
			}
		}
		return { cursor_position };
	case Qt::ImTextBeforeCursor:
		return pristine_text.mid(blockStart(), cursor_position - blockStart());
	case Qt::ImTextAfterCursor:
		return pristine_text.mid(cursor_position, blockEnd() - cursor_position);
#endif
	default:
		return { };
	}
}


bool TextObjectEditorHelper::inputMethodEvent(QInputMethodEvent* event)
{
#ifdef DEBUG_INPUT_METHOD_SUPPORT
	QString a;
	const auto att = event->attributes();
	for (const auto& attribute : att)
	{
		a += QString::fromLatin1("\n   att   %1: %2,%3").arg(attribute.type).arg(attribute.start).arg(attribute.length);
	}
	qDebug("\n>>> ime \"%s\" @ %d,%d [%s]%s", 
	       qPrintable(event->commitString()), event->replacementStart(), event->replacementLength(),
	       qPrintable(event->preeditString()),
	       qPrintable(a));
#endif
	
	// This method updates the editor to match the input method,
	// so it must not modify the input method.
	TextObjectEditorHelper::BatchEdit transaction(this, NoInputMethodAction);
	
	auto position = cursor_position;
	auto length = anchor_position - cursor_position;
	auto old_pristine_text = pristine_text;
	
	// Remove the current non-empty selection 
	// if we are not only moving the selection.
	if (length != 0)
	{
		if (event->replacementLength() > 0
		    || !event->commitString().isEmpty()
		    || !event->preeditString().isEmpty())
		{
			if (length < 0)
			{
				// Swap for QString operations
				position = anchor_position;
				length = -length;
			}
			pristine_text.remove(position, length);
		}
		length = 0;
	}
	
	// Insert the commit string for the given replacement length
	position += event->replacementStart();
	length = event->replacementLength();
	if (position < 0)
	{
		length = qMax(0, length + position);
		position = 0;
	}
	pristine_text.replace(position, length, event->commitString());
	position += event->commitString().length();
	length = 0;
	
	if (old_pristine_text != pristine_text)
	{
		dirty = true;
	}
	
	// Get the preedit string
	preedit_string = event->preeditString();
	auto new_preedit_cursor = preedit_string.length();
	
	// Before applying the new preedit string, we must handle the Selection attribute.
	// At the same occasion, we store the preedit cursor position.
	const auto attributes = event->attributes();
	for (const auto& attribute : attributes)
	{
		switch (attribute.type)
		{
		case QInputMethodEvent::Cursor:
			//if (attribute.length != 0)
			new_preedit_cursor = attribute.start;
			break;
		case QInputMethodEvent::Selection:
			// Selection refers to the text WITHOUT the preedit string.
			position = qBound(0, blockStart() + attribute.start, pristine_text.length());
			length = qBound(-position, attribute.length, pristine_text.length() - position);
			break;
		default:
			; // ignored
		}
	}
	
	// Update the selection and preedit cursor
	setSelection(position, position + length);
	if (preedit_cursor != new_preedit_cursor)
	{
		preedit_cursor = new_preedit_cursor;
		dirty = true;
	}
	
#ifdef DEBUG_INPUT_METHOD_SUPPORT
	qDebug("\n<<< ime");
#endif
	
	event->accept();
	return true;
}


bool TextObjectEditorHelper::sendMouseEventToInputContext(QEvent* event, const MapCoordF &map_coord)
{
	if (isPreediting()) {
		auto click_position = text_object->calcTextPositionAt(map_coord, false) - cursor_position;
		if (click_position >= 0 && click_position <= preedit_string.length())
		{
			if (event->type() == QEvent::MouseButtonRelease)
				QGuiApplication::inputMethod()->invokeAction(QInputMethod::Click, click_position);
			event->setAccepted(true);
			return true;
		}
	}
	return false;
}


bool TextObjectEditorHelper::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget)
	
	if (sendMouseEventToInputContext(event, map_coord))
		return true;
	
	if (event->button() == Qt::LeftButton)
	{
		const auto mouse_press_position = text_object->calcTextPositionAt(map_coord, false);
		
		if (mouse_press_position < 0)
		{
			// Click outside the text: Commit, done
			{
				commitPreedit();
				editor->getMainWidget()->setAttribute(Qt::WA_InputMethodEnabled, false);
				QGuiApplication::inputMethod()->hide();
			}
			emit finished();
		}
		else if (event->modifiers() & Qt::ShiftModifier)
		{
			// Click with Shift: Move the end of the selection
			TextObjectEditorHelper::BatchEdit transaction(this);
			setSelection(anchor_position, mouse_press_position);
		}
		else
		{
			// Other click: Commit, move the cursor
			TextObjectEditorHelper::BatchEdit transaction(this);
			setSelection(mouse_press_position, mouse_press_position);
		}
		return true;
	}
		
	return false;
}


bool TextObjectEditorHelper::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	updateCursor(widget, text_object->calcTextPositionAt(map_coord, true));
	
	if (event->buttons() & Qt::LeftButton)
	{
		TextObjectEditorHelper::BatchEdit transaction(this);
		dragging = true;
		updateDragging(map_coord);
		return true;
	}
	
	if (sendMouseEventToInputContext(event, map_coord))
		return true;  // NOLINT
	
	return false;
}


bool TextObjectEditorHelper::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	Q_UNUSED(widget)
	
	if (sendMouseEventToInputContext(event, map_coord))
		return true;
	
	if (event->button() == Qt::LeftButton)
	{
		TextObjectEditorHelper::BatchEdit transaction(this);
		if (dragging)
		{
			updateDragging(map_coord);
			dragging = false;
		}
		
		if (qApp->focusObject() == widget
		    && !QGuiApplication::inputMethod()->isVisible()
		    && widget->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel) == QStyle::RSIP_OnMouseClick)
		{
#ifdef Q_OS_ANDROID
			// Workaround for QTBUG-58063: Move to the end of the word
			if (!isPreediting() && cursor_position == anchor_position)
			{
				while (cursor_position < pristine_text.length()
					   && pristine_text.at(cursor_position).isLetterOrNumber())
				{
					++cursor_position;
				}
				setSelection(cursor_position, cursor_position);
			}
#endif
			QGuiApplication::inputMethod()->show();
		}
		
		return true;
	}
	
	return false;
}



bool TextObjectEditorHelper::keyPressEvent(QKeyEvent* event)
{
	TextObjectEditorHelper::BatchEdit transaction(this);
	if (event->key() == Qt::Key_Backspace)
	{
		if (cursor_position > 0 && cursor_position == anchor_position)
		{
			setSelection(cursor_position-1, cursor_position);
		}
		replaceSelectionText({});
	}
	else if (event->key() == Qt::Key_Delete)
	{
		if (cursor_position < pristine_text.size() && cursor_position == anchor_position)
		{
			setSelection(cursor_position+1, cursor_position);
		}
		replaceSelectionText({});
	}
	else if (event->matches(QKeySequence::MoveToPreviousChar))
	{
		const auto new_pos = qMax(0, cursor_position - 1);
		setSelection(new_pos, new_pos);
	}
	else if (event->matches(QKeySequence::SelectPreviousChar))
	{
		const auto new_pos = qMax(0, cursor_position - 1);
		setSelection(anchor_position, new_pos);
	}
	else if (event->matches(QKeySequence::MoveToNextChar))
	{
		const auto new_pos = qMin(pristine_text.length(), cursor_position + 1);
		setSelection(new_pos, new_pos);
	}
	else if (event->matches(QKeySequence::SelectNextChar))
	{
		const auto new_pos = qMin(pristine_text.length(), cursor_position + 1);
		setSelection(anchor_position, new_pos);
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
		setSelection(new_anchor, new_pos, line_selection_position);
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
		setSelection(new_anchor, new_pos, line_selection_position);
	}
	else if (event->matches(QKeySequence::MoveToStartOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).start_index;
		setSelection(new_pos, new_pos);
	}
	else if (event->matches(QKeySequence::SelectStartOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).start_index;
		setSelection(anchor_position, new_pos);
	}
	else if (event->matches(QKeySequence::MoveToStartOfDocument))
	{
		setSelection(0, 0);
	}
	else if (event->matches(QKeySequence::SelectStartOfDocument))
	{
		setSelection(anchor_position, 0);
	}
	else if (event->matches(QKeySequence::MoveToEndOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).end_index;
		setSelection(new_pos, new_pos);
	}
	else if (event->matches(QKeySequence::SelectEndOfLine))
	{
		auto new_pos = text_object->findLineInfoForIndex(cursor_position).end_index;
		setSelection(anchor_position, new_pos);
	}
	else if (event->matches(QKeySequence::MoveToEndOfDocument))
	{
		setSelection(pristine_text.length(), pristine_text.length());
	}
	else if (event->matches(QKeySequence::SelectEndOfDocument))
	{
		setSelection(anchor_position, pristine_text.length());
	}
	else if (event->matches(QKeySequence::SelectAll))
	{
		setSelection(0, pristine_text.length());
	}
	else if (event->matches(QKeySequence::Copy)
	         || event->matches(QKeySequence::Cut))
	{
		auto selection = selectionText();
		if (!selection.isEmpty())
		{
			QClipboard* clipboard = QGuiApplication::clipboard();
			clipboard->setText(selection);
			
			if (event->matches(QKeySequence::Cut))
				replaceSelectionText({});
		}
	}
	else if (event->matches(QKeySequence::Paste))
	{
		const auto* clipboard = QGuiApplication::clipboard();
		const auto* mime_data = clipboard->mimeData();
		
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
	else if (event->key() == Qt::Key_Escape)
	{
		return false;
	}
	else if (!event->text().isEmpty()
	         && event->text().at(0).isPrint() )
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
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(0, 0, 255)));
	painter->setTransform(text_object->calcTextToMapTransform(), true);
	
	auto drawRect = [painter](const QRectF& selection_rect) { 
		painter->drawRect(selection_rect);
	};
	
	// Draw preedit area or selection
	auto begin = qMin(anchor_position, cursor_position);
	auto end   = qMax(anchor_position, cursor_position) + preedit_string.length();
	if (begin != end)
	{
		painter->setOpacity(isPreediting() ? 0.3 : 0.55);
		foreachLineRect(begin, end, drawRect);
	}
	
	// Draw cursor
	painter->setOpacity(0.55);
	begin = cursor_position + preedit_cursor;
	foreachLineRect(begin, begin, drawRect);
}


void TextObjectEditorHelper::includeDirtyRect(QRectF& rect) const
{
	const auto transform = text_object->calcTextToMapTransform();
	const auto begin = qMin(anchor_position, cursor_position);
	const auto end   = qMax(anchor_position, cursor_position) + preedit_string.length();
	foreachLineRect(begin, end, [&transform, &rect](const QRectF& selection_rect) {
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



void TextObjectEditorHelper::updateDragging(const MapCoordF& map_coord)
{
	Q_ASSERT(batch_editing);
	const auto drag_position = text_object->calcTextPositionAt(map_coord, false);
	if (drag_position >= 0)
		setSelection(anchor_position, drag_position);
}



QRectF TextObjectEditorHelper::cursorRectangle() const
{
	const auto cursor_pos = cursor_position + preedit_cursor;
	QRectF rect;
	foreachLineRect(cursor_pos, cursor_pos, [&rect](const QRectF& selection_rect) {
		rect = selection_rect;
	});
	return editor->getMainWidget()->mapToViewport(text_object->calcTextToMapTransform().mapRect(rect));
}


void TextObjectEditorHelper::foreachLineRect(int begin, int end, const std::function<void (const QRectF&)>& worker) const
{
	Q_ASSERT(begin <= end);
	for (int line = 0, num_lines = text_object->getNumLines(); line != num_lines; ++line)
	{
		const auto* line_info = text_object->getLineInfo(line);
		if (line_info->end_index + 1 < begin)
			continue;
		if (end < line_info->start_index)
			break;
		
		const auto start_index = qMax(begin, line_info->start_index);
		/// \todo Check for simplification, qMin(selection_end, line_info->end_index)
		const auto end_index = qMax(qMin(line_info->end_index, end), line_info->start_index);
		
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


}  // namespace OpenOrienteering
