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


#include "tool.h"

#include <cstddef>
#include <limits>
#include <vector>

#include <QAction>
#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>

#include "settings.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "gui/main_window.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"


namespace OpenOrienteering {

namespace {

/// Utility function for MapEditorTool::findHoverPoint
inline
qreal distanceSquared(QPointF a, const QPointF& b)
{
	a -= b;
	return QPointF::dotProduct(a, a); // introduced in Qt 5.1
}


}  // namespace



const QRgb MapEditorTool::inactive_color = qRgb(0, 0, 255);
const QRgb MapEditorTool::active_color = qRgb(255, 150, 0);
const QRgb MapEditorTool::selection_color = qRgb(210, 0, 229);

MapEditorTool::MapEditorTool(MapEditorController* editor, Type type, QAction* tool_action)
: QObject(nullptr)
, editor(editor)
, tool_action(tool_action)
, tool_type(type)
{
	settingsChanged();
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &MapEditorTool::settingsChanged);
}

MapEditorTool::~MapEditorTool()
{
	if (tool_action)
		tool_action->setChecked(false);
}

void MapEditorTool::init()
{
	if (tool_action)
		tool_action->setChecked(true);
}

void MapEditorTool::deactivate()
{
	if (editor->getTool() == this)
	{
		if (toolType() == MapEditorTool::EditPoint)
			editor->setTool(nullptr);
		else
			editor->setEditTool();
	}
	deleteLater();
}

void MapEditorTool::switchToDefaultDrawTool(const Symbol* symbol) const
{
	editor->setTool(editor->getDefaultDrawToolForSymbol(symbol));
}

// virtual
void MapEditorTool::finishEditing()
{
	setEditingInProgress(false);
}

void MapEditorTool::setEditingInProgress(bool state)
{
	if (editing_in_progress != state)
	{
		editing_in_progress = state;
		editor->setEditingInProgress(state);
	}
}


void MapEditorTool::draw(QPainter* /*painter*/, MapWidget* /*widget*/)
{
	// nothing
}


bool MapEditorTool::mousePressEvent(QMouseEvent* /*event*/, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	return false;
}

bool MapEditorTool::mouseMoveEvent(QMouseEvent* /*event*/, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	return false;
}

bool MapEditorTool::mouseReleaseEvent(QMouseEvent* /*event*/, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	return false;
}

bool MapEditorTool::mouseDoubleClickEvent(QMouseEvent* /*event*/, const MapCoordF& /*map_coord*/, MapWidget* /*widget*/)
{
	return false;
}

void MapEditorTool::leaveEvent(QEvent* /*event*/)
{
	// nothing
}

bool MapEditorTool::keyPressEvent(QKeyEvent* /*event*/)
{
	return false;
}

bool MapEditorTool::keyReleaseEvent(QKeyEvent* /*event*/)
{
	return false;
}

void MapEditorTool::focusOutEvent(QFocusEvent* /*event*/)
{
	// nothing
}

bool MapEditorTool::inputMethodEvent(QInputMethodEvent* /*event*/)
{
	return false;
}

QVariant MapEditorTool::inputMethodQuery(Qt::InputMethodQuery /*property*/, const QVariant& /*argument*/) const
{
	return {};
}

bool MapEditorTool::gestureEvent(QGestureEvent* /*event*/, MapWidget* /*widget*/)
{
	return false;
}

void MapEditorTool::gestureStarted()
{
	// nothing
}


// static
QCursor MapEditorTool::scaledToScreen(const QCursor& unscaled_cursor)
{
	auto scale = Settings::getInstance().getSetting(Settings::General_PixelsPerInch).toReal() / 96;
	if (unscaled_cursor.shape() == Qt::BitmapCursor
	    && scale > 1.5)
	{
		// Need to scale our low res image for high DPI screen
		const auto unscaled_pixmap = unscaled_cursor.pixmap();
		const auto scaled_hotspot = QPointF{ unscaled_cursor.hotSpot() } * scale;
		return QCursor{ unscaled_pixmap.scaledToWidth(qRound(unscaled_pixmap.width() * scale), Qt::SmoothTransformation),
		                qRound(scaled_hotspot.x()), qRound(scaled_hotspot.y()) };
	}
	else
	{
		return unscaled_cursor;
	}
}


void MapEditorTool::useTouchCursor(bool enabled)
{
	uses_touch_cursor = enabled;
}

Map* MapEditorTool::map() const
{
	return editor->getMap();
}

MapWidget* MapEditorTool::mapWidget() const
{
	return editor->getMainWidget();
}

MainWindow* MapEditorTool::mainWindow() const
{
	return editor->getWindow();
}

QWidget* MapEditorTool::window() const
{
	return editor->getWindow();
}

bool MapEditorTool::isDrawTool() const
{
	switch (tool_type)
	{
	case DrawPoint:
	case DrawPath:
	case DrawCircle:
	case DrawRectangle:
	case DrawFreehand:
	case DrawText:      return true;
		
	default:            return false;
	}
}

void MapEditorTool::setStatusBarText(const QString& text)
{
	editor->getWindow()->setStatusBarText(text);
}

void MapEditorTool::drawSelectionBox(QPainter* painter, MapWidget* widget, const MapCoordF& corner1, const MapCoordF& corner2) const
{
	painter->setBrush(Qt::NoBrush);
	
	QPoint point1 = widget->mapToViewport(corner1).toPoint();
	QPoint point2 = widget->mapToViewport(corner2).toPoint();
	QPoint top_left = QPoint(qMin(point1.x(), point2.x()), qMin(point1.y(), point2.y()));
	QPoint bottom_right = QPoint(qMax(point1.x(), point2.x()), qMax(point1.y(), point2.y()));
	
	painter->setPen(QPen(QBrush(active_color), scaleFactor()));
	painter->drawRect(QRect(top_left, bottom_right - QPoint(1, 1)));
	painter->setPen(QPen(QBrush(qRgb(255, 255, 255)), scaleFactor()));
	painter->drawRect(QRect(top_left + QPoint(1, 1), bottom_right - QPoint(2, 2)));
}



MapCoordVector::size_type MapEditorTool::findHoverPoint(const QPointF& cursor, const MapWidget* widget, const Object* object, bool include_curve_handles, MapCoordF* out_handle_pos) const
{
	const auto click_tolerance_squared = click_tolerance * click_tolerance;
	auto best_index = std::numeric_limits<MapCoordVector::size_type>::max();
	
	if (object->getType() == Object::Point)
	{
		const PointObject* point = reinterpret_cast<const PointObject*>(object);
		if (distanceSquared(widget->mapToViewport(point->getCoordF()), cursor) <= click_tolerance_squared)
		{
			if (out_handle_pos)
				*out_handle_pos = point->getCoordF();
			return 0;
		}
	}
	else if (object->getType() == Object::Text)
	{
		const TextObject* text = reinterpret_cast<const TextObject*>(object);
		std::vector<QPointF> text_handles(text->controlPoints());
		for (std::size_t i = 0; i < text_handles.size(); ++i)
		{
			if (distanceSquared(widget->mapToViewport(text_handles[i]), cursor) <= click_tolerance_squared)
			{
				if (out_handle_pos)
					*out_handle_pos = MapCoordF(text_handles[i]);
				return i;
			}
		}
	}
	else if (object->getType() == Object::Path)
	{
		const auto* path = static_cast<const PathObject*>(object);
		auto size = path->getCoordinateCount();
		
		auto best_dist_sq = click_tolerance_squared;
		for (auto i = size - 1; i < size; --i)
		{
			if (!path->getCoordinate(i).isClosePoint())
			{
				auto distance_sq = distanceSquared(widget->mapToViewport(path->getCoordinate(i)), cursor);
				bool is_handle = (i >= 1 && path->getCoordinate(i - 1).isCurveStart())
				                 || (i >= 2 && path->getCoordinate(i - 2).isCurveStart() );
				if (distance_sq < best_dist_sq || (is_handle && qIsNull(distance_sq - best_dist_sq)))
				{
					best_index = i;
					best_dist_sq = distance_sq;
				}
			}
			if (!include_curve_handles && i >= 3 && path->getCoordinate(i - 3).isCurveStart())
				i -= 2;
		}
		
		if (out_handle_pos &&
		    best_index < std::numeric_limits<MapCoordVector::size_type>::max())
		{
			*out_handle_pos = MapCoordF(path->getCoordinate(best_index));
		}
	}
	
	return best_index;
}

bool MapEditorTool::containsDrawingButtons(Qt::MouseButtons buttons) const
{
	if (buttons.testFlag(Qt::LeftButton)) 
		return true;
	
	return (draw_on_right_click && buttons.testFlag(Qt::RightButton) && editingInProgress());
}

bool MapEditorTool::isDrawingButton(Qt::MouseButton button) const
{
	if (button == Qt::LeftButton) 
		return true;
	
	return (draw_on_right_click && button == Qt::RightButton && editingInProgress());
}


namespace
{
	/**
	 * Returns the drawing scale value for the current pixel-per-inch setting.
	 */
	unsigned int calculateScaleFactor()
	{
		// NOTE: The returned value must be supported by PointHandles::loadHandleImage() !
		const auto base_dpi = qreal(96);
		const auto ppi = Settings::getInstance().getSettingCached(Settings::General_PixelsPerInch).toReal();
		if (ppi <= base_dpi)
			return 1;
		else if (ppi <= base_dpi*2)
			return 2;
		else
			return 4;
	}
	
}  // namespace


// slot
void MapEditorTool::settingsChanged()
{
	click_tolerance = Settings::getInstance().getMapEditorClickTolerancePx();
	start_drag_distance = Settings::getInstance().getStartDragDistancePx();
	scale_factor = calculateScaleFactor();
	point_handles.setScaleFactor(scale_factor);
	draw_on_right_click = Settings::getInstance().getSettingCached(Settings::MapEditor_DrawLastPointOnRightClick).toBool();
}


}  // namespace OpenOrienteering
