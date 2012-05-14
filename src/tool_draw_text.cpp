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


#include "tool_draw_text.h"

#include <QApplication>
#include <QClipboard>
#include <QtGui>

#include "map_widget.h"
#include "map_undo.h"
#include "symbol_dock_widget.h"
#include "symbol.h"
#include "util.h"
#include "object_text.h"
#include "symbol_text.h"

QCursor* DrawTextTool::cursor = NULL;

DrawTextTool::DrawTextTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget) : MapEditorTool(editor, Other, tool_button), renderables(editor->getMap()), symbol_widget(symbol_widget)
{
	dragging = false;
	preview_text = NULL;
	text_editor = NULL;
	
	selectedSymbolsChanged();
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
	connect(editor->getMap(), SIGNAL(symbolChanged(int,Symbol*,Symbol*)), this, SLOT(symbolChanged(int,Symbol*,Symbol*)));
	connect(editor->getMap(), SIGNAL(symbolDeleted(int,Symbol*)), this, SLOT(symbolDeleted(int,Symbol*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-text.png"), 11, 11);
}
void DrawTextTool::init()
{
	updateStatusText();
}
DrawTextTool::~DrawTextTool()
{
	if (text_editor)
		finishEditing();
	deletePreviewObject();
}

bool DrawTextTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
		return text_editor->mousePressEvent(event, map_coord, widget);
	
	if (event->buttons() & Qt::LeftButton)
	{
		click_pos = event->pos();
		click_pos_map = map_coord;
		cur_pos = event->pos();
		cur_pos_map = map_coord;
		dragging = false;
		
		return true;
	}
	return false;
}
bool DrawTextTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
		return text_editor->mouseMoveEvent(event, map_coord, widget);
	
	bool mouse_down = event->buttons() & Qt::LeftButton;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	if (!mouse_down)
	{
		setPreviewLetter();
	}
	else // if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= QApplication::startDragDistance())
		{
			// Start dragging
			dragging = true;
		}
		
		if (dragging)
			updateDirtyRect();
	}
	return true;
}
bool DrawTextTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
	{
		if (text_editor->mouseReleaseEvent(event, map_coord, widget))
			return true;
		else
		{
			finishEditing();
			return true;
		}
	}
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (dragging)
	{
		// Create box text
		double width = qAbs(cur_pos_map.getX() - click_pos_map.getX());
		double height = qAbs(cur_pos_map.getY() - click_pos_map.getY());
		MapCoordF midpoint = MapCoordF(0.5f * (cur_pos_map.getX() + click_pos_map.getX()), 0.5f * (cur_pos_map.getY() + click_pos_map.getY()));
		preview_text->setBox(midpoint.getIntX(), midpoint.getIntY(), width, height);
		
		dragging = false;
		updateDirtyRect();
	}
	else
		setPreviewLetter();
	
	preview_text->setText("");

	// Create the TextObjectEditor
	text_editor = new TextObjectEditorHelper(preview_text, editor);
	connect(text_editor, SIGNAL(selectionChanged(bool)), this, SLOT(selectionChanged(bool)));
	
	updatePreviewObject();
	
	return true;
}
void DrawTextTool::leaveEvent(QEvent* event)
{
	if (!text_editor)
		editor->getMap()->clearDrawingBoundingBox();
}

bool DrawTextTool::keyPressEvent(QKeyEvent* event)
{
	if (text_editor)
	{
		if (text_editor->keyPressEvent(event))
			return true;
	}
	else if (event->key() == Qt::Key_Tab)
	{
		editor->setEditTool();
		return true;
	}
	
	return false;
}
bool DrawTextTool::keyReleaseEvent(QKeyEvent* event)
{
	if (text_editor)
	{
		if (text_editor->keyReleaseEvent(event))
			return true;
	}
	
	return false;
}

void DrawTextTool::draw(QPainter* painter, MapWidget* widget)
{
	if (preview_text && !dragging)
	{
		painter->save();
		widget->applyMapTransform(painter);
		
		float alpha = text_editor ? 1 : 0.5f;
		renderables.draw(painter, widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), true, widget->getMapView()->calculateFinalZoomFactor(), true, alpha);
		
		if (text_editor)
			text_editor->draw(painter, widget);
		
		painter->restore();
	}
	
	if (dragging)
	{
		painter->setPen(active_color);
		painter->setBrush(Qt::NoBrush);
		
		QPoint point1 = widget->mapToViewport(click_pos_map).toPoint();
		QPoint point2 = widget->mapToViewport(cur_pos_map).toPoint();
		QPoint top_left = QPoint(qMin(point1.x(), point2.x()), qMin(point1.y(), point2.y()));
		QPoint bottom_right = QPoint(qMax(point1.x(), point2.x()), qMax(point1.y(), point2.y()));
		
		painter->drawRect(QRect(top_left, bottom_right - QPoint(1, 1)));
		painter->setPen(qRgb(255, 255, 255));
		painter->drawRect(QRect(top_left + QPoint(1, 1), bottom_right - QPoint(2, 2)));
	}
}

void DrawTextTool::selectedSymbolsChanged()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	if (symbol == NULL || symbol->getType() != Symbol::Text || symbol->isHidden())
	{
		if (text_editor)
			finishEditing();
		
		if (symbol && symbol->isHidden())
			editor->setEditTool();
		else
			editor->setTool(editor->getDefaultDrawToolForSymbol(symbol));
		return;
	}
	
	drawing_symbol = symbol;
	if (text_editor)
	{
		preview_text->setSymbol(drawing_symbol, true);
		updatePreviewObject();
	}
	else
		deletePreviewObject();
}
void DrawTextTool::symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol)
{
	if (old_symbol == drawing_symbol)
		selectedSymbolsChanged();
}
void DrawTextTool::symbolDeleted(int pos, Symbol* old_symbol)
{
	if (old_symbol == drawing_symbol)
	{
		preview_text->setText("");
		editor->setEditTool();
	}
}

void DrawTextTool::selectionChanged(bool text_change)
{
	updatePreviewObject();
}

void DrawTextTool::updateDirtyRect()
{
	QRectF rect;
	
	if (preview_text && !dragging)
	{
		rectIncludeSafe(rect, preview_text->getExtent());
		if (text_editor)
			text_editor->includeDirtyRect(rect);
	}
	
	if (dragging)
	{
		rectIncludeSafe(rect, click_pos_map.toQPointF());
		rectIncludeSafe(rect, cur_pos_map.toQPointF());
	}
	
	if (rect.isValid())
		editor->getMap()->setDrawingBoundingBox(rect, 1, true);
	else
		editor->getMap()->clearDrawingBoundingBox();
}
void DrawTextTool::updateStatusText()
{
	setStatusBarText(tr("<b>Click</b> to write text with a single anchor, <b>Drag</b> to create a text box"));
}
void DrawTextTool::updatePreviewObject()
{
	renderables.removeRenderablesOfObject(preview_text, false);
	preview_text->update(true);
	renderables.insertRenderablesOfObject(preview_text);
	updateDirtyRect();
}
void DrawTextTool::deletePreviewObject()
{
	if (preview_text)
	{
		renderables.removeRenderablesOfObject(preview_text, false);
		delete preview_text;
		preview_text = NULL;
	}
}
void DrawTextTool::setPreviewLetter()
{
	if (!preview_text)
	{
		preview_text = new TextObject(drawing_symbol);
		preview_text->setText(tr("A"));
	}
	
	preview_text->setAnchorPosition(cur_pos_map);
	updatePreviewObject();
}
void DrawTextTool::finishEditing()
{
	delete text_editor;
	text_editor = NULL;
	
	renderables.removeRenderablesOfObject(preview_text, false);
	editor->getMap()->clearDrawingBoundingBox();
	
	if (preview_text->getText().isEmpty())
		preview_text->setText(tr("A"));
	else
	{
		int index = editor->getMap()->addObject(preview_text);
		editor->getMap()->clearObjectSelection(false);
		editor->getMap()->addObjectToSelection(preview_text, true);
		
		DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(editor->getMap());
		undo_step->addObject(index);
		editor->getMap()->objectUndoManager().addNewUndoStep(undo_step);
		
		preview_text = NULL;
	}
}

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

// ### TextObjectAlignmentDockWidget ###

TextObjectAlignmentDockWidget::TextObjectAlignmentDockWidget(TextObject* object, int horz_default, int vert_default, TextObjectEditorHelper* text_editor, QWidget* parent)
 : QDockWidget(tr("Alignment"), parent), object(object), text_editor(text_editor)
{
	setFocusPolicy(Qt::NoFocus);
	setFeatures(features() & ~QDockWidget::DockWidgetClosable);
	
	QWidget* widget = new QWidget();
	
	addHorzButton(0, ":/images/text-align-left.png", horz_default);
	addHorzButton(1, ":/images/text-align-hcenter.png", horz_default);
	addHorzButton(2, ":/images/text-align-right.png", horz_default);
	
	addVertButton(0, ":/images/text-align-top.png", vert_default);
	addVertButton(1, ":/images/text-align-vcenter.png", vert_default);
	addVertButton(2, ":/images/text-align-baseline.png", vert_default);
	addVertButton(3, ":/images/text-align-bottom.png", vert_default);
	
	QHBoxLayout* horz_layout = new QHBoxLayout();
	horz_layout->setMargin(0);
	horz_layout->setSpacing(0);
	horz_layout->addStretch(1);
	for (int i = 0; i < 3; ++i)
		horz_layout->addWidget(horz_buttons[i]);
	horz_layout->addStretch(1);
	
	QHBoxLayout* vert_layout = new QHBoxLayout();
	vert_layout->setMargin(0);
	vert_layout->setSpacing(0);
	for (int i = 0; i <4; ++i)
		vert_layout->addWidget(vert_buttons[i]);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addLayout(horz_layout);
	layout->addLayout(vert_layout);
	widget->setLayout(layout);
	
	setWidget(widget);
	
	QSignalMapper* horz_mapper = new QSignalMapper(this);
	connect(horz_mapper, SIGNAL(mapped(int)), this, SLOT(horzClicked(int)));
	for (int i = 0; i < 3; ++i)
	{
		horz_mapper->setMapping(horz_buttons[i], i);
		connect(horz_buttons[i], SIGNAL(clicked()), horz_mapper, SLOT(map()));
	}
	
	QSignalMapper* vert_mapper = new QSignalMapper(this);
	connect(vert_mapper, SIGNAL(mapped(int)), this, SLOT(vertClicked(int)));
	for (int i = 0; i < 4; ++i)
	{
		vert_mapper->setMapping(vert_buttons[i], i);
		connect(vert_buttons[i], SIGNAL(clicked()), vert_mapper, SLOT(map()));
	}
}
bool TextObjectAlignmentDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride)
		event->accept();
	
	return QDockWidget::event(event);
}
void TextObjectAlignmentDockWidget::keyPressEvent(QKeyEvent* event)
{
	if (text_editor->keyPressEvent(event))
		event->accept();
	else
		QWidget::keyPressEvent(event);
}
void TextObjectAlignmentDockWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (text_editor->keyReleaseEvent(event))
		event->accept();
	else
		QWidget::keyReleaseEvent(event);
}
void TextObjectAlignmentDockWidget::addHorzButton(int index, const QString& icon_path, int horz_default)
{
	const TextObject::HorizontalAlignment horz_array[] = {TextObject::AlignLeft, TextObject::AlignHCenter, TextObject::AlignRight};
	const QString tooltips[] = {tr("Left"), tr("Center"), tr("Right")};
	
	horz_buttons[index] = new QPushButton(QIcon(icon_path), "");
	horz_buttons[index]->setFocusPolicy(Qt::NoFocus);
	horz_buttons[index]->setCheckable(true);
	horz_buttons[index]->setToolTip(tooltips[index]);
	if (horz_default == horz_array[index])
	{
		horz_buttons[index]->setChecked(true);
		horz_index = index;
	}	
}
void TextObjectAlignmentDockWidget::addVertButton(int index, const QString& icon_path, int vert_default)
{
	const TextObject::VerticalAlignment vert_array[] = {TextObject::AlignTop, TextObject::AlignVCenter, TextObject::AlignBaseline, TextObject::AlignBottom};
	const QString tooltips[] = {tr("Top"), tr("Center"), tr("Baseline"), tr("Bottom")};
	
	vert_buttons[index] = new QPushButton(QIcon(icon_path), "");
	vert_buttons[index]->setFocusPolicy(Qt::NoFocus);
	vert_buttons[index]->setCheckable(true);
	vert_buttons[index]->setToolTip(tooltips[index]);
	if (vert_default == vert_array[index])
	{
		vert_buttons[index]->setChecked(true);
		vert_index = index;
	}	
}

void TextObjectAlignmentDockWidget::horzClicked(int index)
{
	for (int i = 0; i < 3; ++i)
		horz_buttons[i]->setChecked(index == i);
	horz_index = index;
	emitAlignmentChanged();
}
void TextObjectAlignmentDockWidget::vertClicked(int index)
{
	for (int i = 0; i < 4; ++i)
		vert_buttons[i]->setChecked(index == i);
	vert_index = index;
	emitAlignmentChanged();
}

void TextObjectAlignmentDockWidget::emitAlignmentChanged()
{
	const TextObject::HorizontalAlignment horz_array[] = {TextObject::AlignLeft, TextObject::AlignHCenter, TextObject::AlignRight};
	const TextObject::VerticalAlignment vert_array[] = {TextObject::AlignTop, TextObject::AlignVCenter, TextObject::AlignBaseline, TextObject::AlignBottom};
	
	emit(alignmentChanged((int)horz_array[horz_index], (int)vert_array[vert_index]));
}

#include "tool_draw_text.moc"
