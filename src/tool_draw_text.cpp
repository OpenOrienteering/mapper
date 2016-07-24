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


#include "tool_draw_text.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include "gui/modifier_key.h"
#include "map.h"
#include "map_editor.h"
#include "map_widget.h"
#include "object_text.h"
#include "object_undo.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_text.h"
#include "tool_edit.h"
#include "tool_helpers.h"
#include "util.h"


#if defined(__MINGW32__) and defined(DrawText)
// MinGW(64) winuser.h issue
#undef DrawText
#endif


DrawTextTool::DrawTextTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase { QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-text.png")), 11, 11 }, DrawText, editor, tool_button }
, drawing_symbol { editor->activeSymbol() }
, renderables    { map() }
, preview_text   { new TextObject(), { renderables } }
{
	connect(editor, &MapEditorController::activeSymbolChanged, this, &DrawTextTool::setDrawingSymbol);
}

void DrawTextTool::initImpl()
{
	preview_text->setSymbol(drawing_symbol, false);
}

DrawTextTool::~DrawTextTool()
{
	if (text_editor)
		finishEditing();
}

bool DrawTextTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
		return text_editor->mousePressEvent(event, map_coord, widget);
	
	return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}

bool DrawTextTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
		return text_editor->mouseMoveEvent(event, map_coord, widget);
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}

bool DrawTextTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	if (text_editor)
	{
		if (!text_editor->mouseReleaseEvent(event, map_coord, widget))
			finishEditing();
		return true;
	}
	
	return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
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
	
	return MapEditorToolBase::keyPressEvent(event);
}

bool DrawTextTool::keyReleaseEvent(QKeyEvent* event)
{
	if (text_editor)
	{
		if (text_editor->keyReleaseEvent(event))
			return true;
	}
	
	return MapEditorToolBase::keyReleaseEvent(event);
}

void DrawTextTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	painter->save();
	
	if (!preview_text->hasSingleAnchor())
	{
		painter->setPen(active_color);
		painter->setBrush(Qt::NoBrush);
		
		auto rect = widget->mapToViewport(QRectF(click_pos_map, constrained_pos_map));
		painter->drawRect(rect);
		rect.adjust(1, 1, -1, -1);
		painter->setPen(qRgb(255, 255, 255));
		painter->drawRect(rect);
	}
	
	widget->applyMapTransform(painter);
	
	float opacity = text_editor ? 1.0f : 0.5f;
	RenderConfig config = { *map(), widget->getMapView()->calculateViewedRect(widget->viewportToView(widget->rect())), widget->getMapView()->calculateFinalZoomFactor(), RenderConfig::Tool, opacity };
	renderables.draw(painter, config);
	
	if (text_editor)
		text_editor->draw(painter, widget);
	
	painter->restore();
}

void DrawTextTool::setDrawingSymbol(const Symbol* symbol)
{
	if (symbol != drawing_symbol)
	{
		// End current editing
		if (text_editor)
		{
			// Avoid using deleted symbol
			if (map()->findSymbolIndex(drawing_symbol) == -1)
				abortEditing();
			else
				finishEditing();
		}
		
		// Handle new symbol
		drawing_symbol = symbol;
		if (!symbol || symbol->isHidden())
			deactivate();
		else if (!preview_text->setSymbol(symbol, false))
			switchToDefaultDrawTool(symbol);
	}
}

void DrawTextTool::selectionChanged(bool text_change)
{
	Q_UNUSED(text_change);
	preview_text->setOutputDirty();
	updatePreviewText();
}


int DrawTextTool::updateDirtyRectImpl(QRectF& rect)
{
	rectIncludeSafe(rect, preview_text->getExtent());
	
	if (isDragging())
	{
		rectIncludeSafe(rect, click_pos_map);
		rectIncludeSafe(rect, constrained_pos_map);
	}
	else if (text_editor)
	{
		text_editor->includeDirtyRect(rect);
	}
	
	return 1;
}

void DrawTextTool::updateStatusText()
{
	QString text;
	if (text_editor)
	{
		text = tr("<b>%1</b>: Finish editing. ").arg(ModifierKey::escape());
	}
	else
	{
		text = tr("<b>Click</b>: Create a text object with a single anchor. <b>Drag</b>: Create a text box. ");
		if (!(active_modifiers & Qt::ShiftModifier))
			text += EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
	}
	setStatusBarText(text);
}

void DrawTextTool::updatePreviewText()
{
	auto text = preview_text.get();
	renderables.removeRenderablesOfObject(text, false);
	text->update();
	renderables.insertRenderablesOfObject(text);
	updateDirtyRect();
}

void DrawTextTool::setPreviewLetter()
{
	if (auto symbol = preview_text->getSymbol())
	{
		preview_text->setText(static_cast<const TextSymbol*>(symbol)->getIconText());
		updatePreviewText();
	}
}

void DrawTextTool::startEditing()
{
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	preview_text->setText(QString{});

	// Create the TextObjectEditor
	text_editor.reset(new TextObjectEditorHelper(preview_text.get(), editor));
	connect(text_editor.get(), &TextObjectEditorHelper::selectionChanged, this, &DrawTextTool::selectionChanged);
	
	editor->setEditingInProgress(true);
	
	updatePreviewText();
	updateStatusText();
}

void DrawTextTool::abortEditing()
{
	text_editor.reset(nullptr);
	
	renderables.removeRenderablesOfObject(preview_text.get(), false);
	map()->clearDrawingBoundingBox();
	
	editor->setEditingInProgress(false);
	
	preview_text->setAnchorPosition(cur_pos_map);
	setPreviewLetter();
	updateStatusText();
}

void DrawTextTool::finishEditing()
{
	text_editor.reset(nullptr);
	
	renderables.removeRenderablesOfObject(preview_text.get(), false);
	map()->clearDrawingBoundingBox();
	
	if (!preview_text->getText().isEmpty())
	{
		auto object = preview_text.release();
		int index = map()->addObject(object);
		map()->clearObjectSelection(false);
		map()->addObjectToSelection(object, true);
		map()->setObjectsDirty();
		
		DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map());
		undo_step->addObject(index);
		map()->push(undo_step);
		
		preview_text.reset(new TextObject(drawing_symbol));
	}
	
	editor->setEditingInProgress(false);
	updateStatusText();
}



void DrawTextTool::mouseMove()
{
	preview_text->setAnchorPosition(constrained_pos_map);
	setPreviewLetter();
}

void DrawTextTool::clickPress()
{
	preview_text->setAnchorPosition(constrained_pos_map);
	setPreviewLetter();
}

void DrawTextTool::clickRelease()
{
	startEditing();
}

void DrawTextTool::dragMove()
{
	auto p1 = click_pos_map;
	auto p2 = constrained_pos_map;
	auto width = qAbs(p1.x() - p2.x());
	auto height = qAbs(p1.y() - p2.y());
	auto midpoint = MapCoord { (p1 + p2) / 2 };
	preview_text->setBox(midpoint.nativeX(), midpoint.nativeY(), width, height);
	updatePreviewText();
}

void DrawTextTool::dragFinish()
{
	startEditing();
}

bool DrawTextTool::keyPress(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Shift:
		if (!text_editor)
		{
			snap_helper->setFilter(SnappingToolHelper::AllTypes);
			reapplyConstraintHelpers();
			return true;
		}
		break;
		
	case Qt::Key_Tab:
		deactivate();
		return true;
		
	default:
		; // nothing
	}
	return false;
}

bool DrawTextTool::keyRelease(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Shift:
		if (!text_editor)
		{
			snap_helper->setFilter(SnappingToolHelper::NoSnapping);
			reapplyConstraintHelpers();
			return true;
		}
		break;
		
	default:
		; // nothing
	}
	
	return false;
}


void DrawTextTool::objectSelectionChangedImpl()
{
	// nothing
}
