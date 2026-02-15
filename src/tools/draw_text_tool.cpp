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


#include "draw_text_tool.h"

#include <QtGlobal>
#include <QCursor>
#include <QFlags>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QRectF>
#include <QRgb>
#include <QString>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_view.h"
#include "core/objects/text_object.h"
#include "core/renderables/renderable.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "gui/modifier_key.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "tools/edit_tool.h"
#include "tools/text_object_editor_helper.h"
#include "tools/tool.h"
#include "tools/tool_helpers.h"
#include "undo/object_undo.h"
#include "util/util.h"


#if defined(__MINGW32__) && defined(DrawText)
// MinGW(64) winuser.h issue
#undef DrawText
#endif


namespace OpenOrienteering {

DrawTextTool::DrawTextTool(MapEditorController* editor, QAction* tool_action)
: MapEditorToolBase { QCursor{ QPixmap(QString::fromLatin1(":/images/cursor-draw-text.png")), 11, 11 }, DrawText, editor, tool_action }
, drawing_symbol { editor->activeSymbol() }
, renderables    { map() }
, preview_text   { new TextObject(), { renderables } }
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
	
	if (editor->isInMobileMode())
	{
		// Create key replacement bar
		key_button_bar = new KeyButtonBar(editor->getMainWidget());
		//: Snap to existing objects
		key_button_bar->addModifierButton(Qt::ShiftModifier, tr("Snap"));
		editor->showPopupWidget(key_button_bar, QString{});
	}
}


void DrawTextTool::objectSelectionChangedImpl()
{
	// nothing
}


void DrawTextTool::startEditing()
{
	if (key_button_bar)
		key_button_bar->hide();
	
	snap_helper->setFilter(SnappingToolHelper::NoSnapping);
	preview_text->setText(QString{});

	// Create the TextObjectEditor
	text_editor.reset(new TextObjectEditorHelper(preview_text.get(), editor));
	connect(text_editor.get(), &TextObjectEditorHelper::stateChanged, this, &DrawTextTool::updatePreviewText);
	connect(text_editor.get(), &TextObjectEditorHelper::finished, this, &DrawTextTool::finishEditing);
	
	MapEditorToolBase::startEditing();
	
	updatePreviewText();
	updateStatusText();
}

void DrawTextTool::abortEditing()
{
	text_editor.reset(nullptr);
	
	renderables.removeRenderablesOfObject(preview_text.get(), false);
	map()->clearDrawingBoundingBox();
	
	MapEditorToolBase::abortEditing();
	resetWaitingForMouseRelease();
	
	if (key_button_bar)
	{
		key_button_bar->show();
		if (key_button_bar->keyboardModifiers().testFlag(Qt::ShiftModifier))
			snap_helper->setFilter(SnappingToolHelper::AllTypes);
	}
}

void DrawTextTool::finishEditing()
{
	updateDirtyRect();
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
		
		auto undo_step = new DeleteObjectsUndoStep(map());
		undo_step->addObject(index);
		map()->push(undo_step);
		
		preview_text.reset(new TextObject(drawing_symbol));
	}
	
	MapEditorToolBase::finishEditing();
	
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
	
	if (key_button_bar)
	{
		key_button_bar->show();
		if (key_button_bar->keyboardModifiers().testFlag(Qt::ShiftModifier))
			snap_helper->setFilter(SnappingToolHelper::AllTypes);
	}
}

bool DrawTextTool::mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
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

bool DrawTextTool::mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
{
	mousePositionEvent(event, map_coord, widget);
	
	if (text_editor && text_editor->mouseMoveEvent(event, map_coord, widget))
		return true;
	
	if (waiting_for_mouse_release)
		resetWaitingForMouseRelease();
	
	return MapEditorToolBase::mouseMoveEvent(event, map_coord, widget);
}

bool DrawTextTool::mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget)
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


bool DrawTextTool::inputMethodEvent(QInputMethodEvent* event)
{
	if (text_editor && text_editor->inputMethodEvent(event))
		return true;
	
	return MapEditorTool::inputMethodEvent(event);
}

QVariant DrawTextTool::inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const
{
	auto result = QVariant { };
	if (text_editor)
		result = text_editor->inputMethodQuery(property, argument);
	return result;
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
	if (text_editor && text_editor->keyPressEvent(event))
	{
		return true;
	}
	
	switch (event->key())
	{
	case Qt::Key_Escape:
		if (text_editor)
		{
			if (event->modifiers() & Qt::ControlModifier)
				abortEditing();
			else
				finishEditing();
			return true;
		}
		break;
		
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
		
	}
	return false;
}

bool DrawTextTool::keyRelease(QKeyEvent* event)
{
	if (text_editor && text_editor->keyReleaseEvent(event))
	{
		return true;
	}
	
	switch (event->key())
	{
	case Qt::Key_Shift:
		if (!text_editor)
		{
			snap_helper->setFilter(SnappingToolHelper::NoSnapping);
			reapplyConstraintHelpers();
		}
		return false; // not consuming Shift
		
	}
	return false;
}


void DrawTextTool::leaveEvent(QEvent* /*event*/)
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
		if (editor->isInMobileMode())
		{
			// Expecting touch screen instead of mouse
			preview_text->setText({});
		}
		else
		{
			preview_text->setText(static_cast<const TextSymbol*>(symbol)->getIconText());
		}
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
			text += ::OpenOrienteering::EditTool::tr("<b>%1</b>: Snap to existing objects. ").arg(ModifierKey::shift());
	}
	setStatusBarText(text);
}


}  // namespace OpenOrienteering
