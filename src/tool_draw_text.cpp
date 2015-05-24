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


#include "tool_draw_text.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalMapper>
#include <QVBoxLayout>

#include "map.h"
#include "map_editor.h"
#include "object_undo.h"
#include "map_widget.h"
#include "object_text.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_text.h"
#include "tool_helpers.h"
#include "util.h"
#include "gui/modifier_key.h"

#if defined(__MINGW32__) and defined(DrawText)
// MinGW(64) winuser.h issue
#undef DrawText
#endif

QCursor* DrawTextTool::cursor = NULL;

DrawTextTool::DrawTextTool(MapEditorController* editor, QAction* tool_button)
: MapEditorTool(editor, DrawText, tool_button)
, drawing_symbol(editor->activeSymbol())
, dragging(false)
, preview_text(NULL)
, text_editor(NULL)
, renderables(new MapRenderables(map()))
{
	connect(editor, SIGNAL(activeSymbolChanged(const Symbol*)), this, SLOT(setDrawingSymbol(const Symbol*)));
	
	if (!cursor)
		cursor = new QCursor(QPixmap(":/images/cursor-draw-text.png"), 11, 11);
}

void DrawTextTool::init()
{
	updateStatusText();
	
	MapEditorTool::init();
}

DrawTextTool::~DrawTextTool()
{
	if (text_editor)
		finishEditing();
	deletePreviewText();
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
	bool mouse_down = event->buttons() & Qt::LeftButton;
	cur_pos = event->pos();
	cur_pos_map = map_coord;
	
	if (text_editor)
		return text_editor->mouseMoveEvent(event, map_coord, widget);
	
	if (!mouse_down)
	{
		setPreviewLetter();
	}
	else // if (mouse_down)
	{
		if (!dragging && (event->pos() - click_pos).manhattanLength() >= Settings::getInstance().getStartDragDistancePx())
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
			cur_pos = event->pos();
			cur_pos_map = map_coord;
			finishEditing();
			return true;
		}
	}
	
	if (event->button() != Qt::LeftButton)
		return false;
	
	if (dragging)
	{
		// Create box text
		double width = qAbs(cur_pos_map.x() - click_pos_map.x());
		double height = qAbs(cur_pos_map.y() - click_pos_map.y());
		auto midpoint = MapCoord { 0.5 * (cur_pos_map + click_pos_map) };
		preview_text->setBox(midpoint.rawX(), midpoint.rawY(), width, height);
		
		dragging = false;
		updateDirtyRect();
	}
	else
		setPreviewLetter();
	
	preview_text->setText("");

	// Create the TextObjectEditor
	text_editor = new TextObjectEditorHelper(preview_text, editor);
	connect(text_editor, SIGNAL(selectionChanged(bool)), this, SLOT(selectionChanged(bool)));
	
	updatePreviewText();
	updateStatusText();
	
	return true;
}

void DrawTextTool::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	
	if (!text_editor)
		map()->clearDrawingBoundingBox();
}

bool DrawTextTool::keyPressEvent(QKeyEvent* event)
{
	if (text_editor)
	{
		if (event->key() == Qt::Key_Escape)
		{
			finishEditing();
			return true;
		}
		else if (text_editor->keyPressEvent(event))
		{
			return true;
		}
	}
	else if (event->key() == Qt::Key_Tab)
	{
		deactivate();
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
		
		float opacity = text_editor ? 1.0f : 0.5f;
		RenderConfig config = { *map(), widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), widget->getMapView()->calculateFinalZoomFactor(), RenderConfig::Tool, opacity };
		renderables->draw(painter, config);
		
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

void DrawTextTool::setDrawingSymbol(const Symbol* symbol)
{
	// Avoid using deleted symbol
	if (map()->findSymbolIndex(drawing_symbol) == -1)
		symbol = NULL;
	
	// End current editing
	if (text_editor)
	{
		if (symbol)
			finishEditing();
		else
			abortEditing();
	}
	
	// Handle new symbol
	drawing_symbol = symbol;
	deletePreviewText();
	
	if (!symbol)
		deactivate();
	else if (symbol->isHidden())
		deactivate();
	else if (symbol->getType() != Symbol::Text)
		switchToDefaultDrawTool(symbol);
}

void DrawTextTool::selectionChanged(bool text_change)
{
	Q_UNUSED(text_change);
	preview_text->setOutputDirty(); // TODO: Check if neccessary here.
	updatePreviewText();
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
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, cur_pos_map);
	}
	
	if (rect.isValid())
		map()->setDrawingBoundingBox(rect, 1, true);
	else
		map()->clearDrawingBoundingBox();
}

void DrawTextTool::updateStatusText()
{
	if (text_editor)
		setStatusBarText(tr("<b>%1</b>: Finish editing. ").arg(ModifierKey::escape()));
	else
		setStatusBarText(tr("<b>Click</b>: Create a text object with a single anchor. <b>Drag</b>: Create a text box. "));
}

void DrawTextTool::updatePreviewText()
{
	renderables->removeRenderablesOfObject(preview_text, false);
	preview_text->update();
	renderables->insertRenderablesOfObject(preview_text);
	updateDirtyRect();
}

void DrawTextTool::deletePreviewText()
{
	if (preview_text)
	{
		renderables->removeRenderablesOfObject(preview_text, false);
		delete preview_text;
		preview_text = NULL;
	}
}

void DrawTextTool::setPreviewLetter()
{
	if (!preview_text)
	{
		preview_text = new TextObject(drawing_symbol);
	}
	
	preview_text->setText(static_cast<const TextSymbol*>(drawing_symbol)->getIconText());
	preview_text->setAnchorPosition(cur_pos_map);
	updatePreviewText();
}

void DrawTextTool::abortEditing()
{
	delete text_editor;
	text_editor = NULL;
	
	renderables->removeRenderablesOfObject(preview_text, false);
	map()->clearDrawingBoundingBox();
	
	setPreviewLetter();
	updateStatusText();
}

void DrawTextTool::finishEditing()
{
	delete text_editor;
	text_editor = NULL;
	
	renderables->removeRenderablesOfObject(preview_text, false);
	map()->clearDrawingBoundingBox();
	
	if (!preview_text->getText().isEmpty())
	{
		int index = map()->addObject(preview_text);
		map()->clearObjectSelection(false);
		map()->addObjectToSelection(preview_text, true);
		map()->setObjectsDirty();
		
		DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
		undo_step->addObject(index);
		map()->push(undo_step);
		
		preview_text = NULL;
	}
	
	setPreviewLetter();
	updateStatusText();
	
	MapEditorTool::finishEditing();
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
