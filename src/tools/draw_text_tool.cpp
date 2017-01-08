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


#include "draw_text_tool.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include "gui/modifier_key.h"
#include "core/map.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "core/objects/text_object.h"
#include "object_undo.h"
#include "core/renderables/renderable.h"
#include "settings.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "edit_tool.h"
#include "tool_helpers.h"
#include "util.h"
#include "tools/text_object_editor_helper.h"


#if defined(__MINGW32__) and defined(DrawText)
// MinGW(64) winuser.h issue
#undef DrawText
#endif


DrawTextTool::DrawTextTool(MapEditorController* editor, QAction* tool_button)
: MapEditorToolBase { QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-text.png")), 11, 11 }, DrawText, editor, tool_button }
, drawing_symbol { editor->activeSymbol() }
, renderables    { map() }
, preview_text   { new TextObject(), { renderables } }
, waiting_for_mouse_release { false }
{
	connect(editor, &MapEditorController::activeSymbolChanged, this, &DrawTextTool::setDrawingSymbol);
}

DrawTextTool::~DrawTextTool()
{
	if (text_editor)
		finishEditing();
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


void DrawTextTool::initImpl()
{
	preview_text->setSymbol(drawing_symbol, false);
}


void DrawTextTool::objectSelectionChangedImpl()
{
	// nothing
}


void DrawTextTool::startEditing()
{
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	preview_text->setText(QString{});

	// Create the TextObjectEditor
	text_editor.reset(new TextObjectEditorHelper(preview_text.get(), editor));
	connect(text_editor.get(), &TextObjectEditorHelper::stateChanged, this, &DrawTextTool::updatePreviewText);
	connect(text_editor.get(), &TextObjectEditorHelper::finished, this, &DrawTextTool::finishEditing);
	
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
	resetWaitingForMouseRelease();
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
	
	if (QGuiApplication::mouseButtons())
	{
		waiting_for_mouse_release = true;
		cur_map_widget->setCursor({});
		updateStatusText();
	}
	else
	{
		resetWaitingForMouseRelease();
	}
}

bool DrawTextTool::mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	mousePositionEvent(event, map_coord, widget);
	
	if (text_editor && text_editor->mousePressEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
	{
		if (event->buttons() & ~event->button())
			return true;
		resetWaitingForMouseRelease();
	}
	
	return MapEditorToolBase::mousePressEvent(event, map_coord, widget);
}

bool DrawTextTool::mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	mousePositionEvent(event, map_coord, widget);
	
	if (text_editor && text_editor->mouseMoveEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
		resetWaitingForMouseRelease();
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}

bool DrawTextTool::mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget)
{
	mousePositionEvent(event, map_coord, widget);
	
	if (text_editor && text_editor->mouseReleaseEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
	{
		if (!event->buttons())
			resetWaitingForMouseRelease();
		return true;
	}
	
	return MapEditorToolBase::mouseReleaseEvent(event, map_coord, widget);
}


bool DrawTextTool::keyPressEvent(QKeyEvent* event)
{
	if (text_editor)
	{
		if (event->key() == Qt::Key_Escape)
		{
			if (event->modifiers() & Qt::ControlModifier)
				abortEditing();
			else
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
	if (text_editor && text_editor->keyReleaseEvent(event))
	{
		return true;
	}
	
	return MapEditorToolBase::keyReleaseEvent(event);
}


void DrawTextTool::mouseMove()
{
	if (!text_editor)
		updatePreview();
}

void DrawTextTool::clickPress()
{
	updatePreview();
}

void DrawTextTool::clickRelease()
{
	startEditing();
}

void DrawTextTool::dragMove()
{
	auto p1 = constrained_click_pos_map;
	auto p2 = constrained_pos_map;
	auto width = qAbs(p1.x() - p2.x());
	auto height = qAbs(p1.y() - p2.y());
	auto midpoint = MapCoord { (p1 + p2) / 2 };
	preview_text->setBox(midpoint.nativeX(), midpoint.nativeY(), width, height);
	updatePreviewText();
	updateStatusText();
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


void DrawTextTool::leaveEvent(QEvent*)
{
	if (!text_editor)
		map()->clearDrawingBoundingBox();
}


void DrawTextTool::resetWaitingForMouseRelease()
{
	waiting_for_mouse_release = false;
	cur_map_widget->setCursor(getCursor());
	updatePreview();
	updateStatusText();
}


void DrawTextTool::updatePreview()
{
	preview_text->setAnchorPosition(constrained_pos_map);
	if (auto symbol = preview_text->getSymbol())
	{
		preview_text->setText(static_cast<const TextSymbol*>(symbol)->getIconText());
		updatePreviewText();
	}
}

void DrawTextTool::updatePreviewText()
{
	auto text = preview_text.get();
	renderables.removeRenderablesOfObject(text, false);
	text->update();
	renderables.insertRenderablesOfObject(text);
	updateDirtyRect();
}

int DrawTextTool::updateDirtyRectImpl(QRectF& rect)
{
	rectIncludeSafe(rect, preview_text->getExtent());
	
	if (isDragging())
	{
		rectIncludeSafe(rect, constrained_click_pos_map);
		rectIncludeSafe(rect, constrained_pos_map);
	}
	else if (text_editor)
	{
		text_editor->includeDirtyRect(rect);
		if (!preview_text->hasSingleAnchor())
		{
			const auto center = preview_text->getAnchorCoordF();
			const auto width = preview_text->getBoxWidth();
			const auto height = preview_text->getBoxHeight();
			rectIncludeSafe(rect, {center.x()-width/2, center.y()-height/2});
			rectIncludeSafe(rect, {center.x()+width/2, center.y()+height/2});
		}
	}
	
	return 1;
}

void DrawTextTool::drawImpl(QPainter* painter, MapWidget* widget)
{
	painter->save();
	
	if (!preview_text->hasSingleAnchor())
	{
		painter->setPen(active_color);
		painter->setBrush(Qt::NoBrush);
		
		// Default is what is fast (and stable!) while dragging.
		auto map_rect = QRectF { click_pos_map, constrained_pos_map };
		if (!isDragging())
		{
			// This is the actual frame, but it would be shaky while dragging,
			// because of discretization in non-antialiased drawing.
			const auto center = preview_text->getAnchorCoordF();
			const auto width = preview_text->getBoxWidth();
			const auto height = preview_text->getBoxHeight();
			map_rect = QRectF { center.x()-width/2, center.y()-height/2, width, height };
		}
		auto rect = widget->mapToViewport(map_rect);
		
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

void DrawTextTool::updateStatusText()
{
	QString text;
	if (text_editor)
	{
		text = tr("<b>%1</b>: Finish editing. ").arg(ModifierKey::escape()) +
		       tr("<b>%1+%2</b>: Cancel editing. ").arg(ModifierKey::control(), ModifierKey::escape());
	}
	else if (!waiting_for_mouse_release)
	{
		if (!isDragging())
			text = tr("<b>Click</b>: Create a text object with a single anchor. <b>Drag</b>: Create a text box. ");
		if (!(active_modifiers & Qt::ShiftModifier))
			text += EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
	}
	setStatusBarText(text);
}
