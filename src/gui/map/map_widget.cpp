/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2020 Kai Pastor
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


#include "map_widget.h"

#include <cmath>
#include <stdexcept>

#include <QApplication>
#include <QtMath>
#include <QColor>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFlags>
#include <QFont>
#include <QGestureEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLatin1String>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QObjectList>
#include <QPainter>
#include <QPaintEvent>
#include <QPinchGesture>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QToolTip>
#include <QTouchEvent>
#include <QTransform>
#include <QVariant>
#include <QWheelEvent>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/renderables/renderable.h"
#include "core/symbols/symbol.h"  // IWYU pragma: keep
#include "gui/touch_cursor.h"
#include "gui/map/map_editor_activity.h"
#include "gui/widgets/action_grid_bar.h"
#include "gui/widgets/key_button_bar.h"
#include "gui/widgets/pie_menu.h"
#include "sensors/gps_display.h"
#include "sensors/gps_temporary_markers.h"
#include "templates/template.h"
#include "tools/tool.h"
#include "util/backports.h" // IWYU pragma: keep
#include "util/util.h"

class QGesture;
// IWYU pragma: no_forward_declare QPinchGesture


#ifdef __clang_analyzer__
#define singleShot(A, B, C) singleShot(A, B, #C) // NOLINT 
#endif


namespace OpenOrienteering {

namespace {

bool transformsMatch(const QTransform& lhs, const QTransform& rhs)
{
	return qFuzzyCompare(1.0 + lhs.m11(), 1.0 + rhs.m11())
	       && qFuzzyCompare(1.0 + lhs.m12(), 1.0 + rhs.m12())
	       && qFuzzyCompare(1.0 + lhs.m21(), 1.0 + rhs.m21())
	       && qFuzzyCompare(1.0 + lhs.m22(), 1.0 + rhs.m22())
	       && qFuzzyCompare(1.0 + lhs.dx(), 1.0 + rhs.dx())
	       && qFuzzyCompare(1.0 + lhs.dy(), 1.0 + rhs.dy());
}

}  // namespace

MapWidget::MapWidget(bool show_help, bool force_antialiasing, QWidget* parent)
 : QWidget(parent)
 , view(nullptr)
 , tool(nullptr)
 , activity(nullptr)
 , coords_type(MAP_COORDS)
 , cursorpos_label(nullptr)
 , show_help(show_help)
 , force_antialiasing(force_antialiasing)
 , dragging(false)
 , pinching(false)
 , pinching_factor(1.0)
 , pinching_angle(0.0)
 , auto_rotation_active(false)
 , below_template_cache_dirty_rect(rect())
 , above_template_cache_dirty_rect(rect())
 , map_cache_dirty_rect(rect())
 , drawing_dirty_rect_border(0)
 , activity_dirty_rect_border(0)
 , last_mouse_release_time(QTime::currentTime())
 , current_pressed_buttons(0)
 , gps_display(nullptr)
 , marker_display(nullptr)
{
	context_menu = new PieMenu(this);
// 	context_menu->setMinimumActionCount(8);
// 	context_menu->setIconSize(24);
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_AcceptTouchEvents, true);
	setGesturesEnabled(true);
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

MapWidget::~MapWidget()
{
	// nothing, not inlined
}

void MapWidget::setMapView(MapView* view)
{
	if (this->view != view)
	{
		if (this->view)
		{
			auto* map = view->getMap();
			map->removeMapWidget(this);
			disconnect(map);
			disconnect(this->view);
		}
		
		this->view = view;
		
		if (view)
		{
			connect(this->view, &MapView::viewChanged, this, &MapWidget::viewChanged);
			connect(this->view, &MapView::visibilityChanged, this, &MapWidget::visibilityChanged);
			connect(this->view, &MapView::panOffsetChanged, this, &MapWidget::setPanOffset);
			
			auto* map = this->view->getMap();
			map->addMapWidget(this);
			connect(map, &Map::colorAdded, this, &MapWidget::updatePlaceholder);
			connect(map, &Map::colorDeleted, this, &MapWidget::updatePlaceholder);
			connect(map, &Map::symbolAdded, this, &MapWidget::updatePlaceholder);
			connect(map, &Map::symbolDeleted, this, &MapWidget::updatePlaceholder);
			connect(map, &Map::templateAdded, this, &MapWidget::updatePlaceholder);
			connect(map, &Map::templateDeleted, this, &MapWidget::updatePlaceholder);
		}
		
		update();
	}
}

void MapWidget::setTool(MapEditorTool* tool)
{
	// Redraw if touch cursor usage changes
	bool redrawTouchCursor = (touch_cursor && this->tool && tool
		&& (this->tool->usesTouchCursor() || tool->usesTouchCursor()));

	this->tool = tool;
	
	if (tool)
		setCursor(tool->getCursor());
	else
		unsetCursor();
	if (redrawTouchCursor)
		touch_cursor->updateMapWidget(false);
}

void MapWidget::setActivity(MapEditorActivity* activity)
{
	this->activity = activity;
}


void MapWidget::setAutoRotationActive(bool active)
{
	auto_rotation_active = active;
}

void MapWidget::setGesturesEnabled(bool enabled)
{
	gestures_enabled = enabled;
	if (enabled)
	{
		grabGesture(Qt::PinchGesture);
	}
	else
	{
		ungrabGesture(Qt::PinchGesture);
	}
}


void MapWidget::applyMapTransform(QPainter* painter) const
{
	painter->translate(width() / 2.0 + getMapView()->panOffset().x(),
					   height() / 2.0 + getMapView()->panOffset().y());
	painter->setWorldTransform(getMapView()->worldTransform(), true);
}

QRectF MapWidget::viewportToView(const QRect& input) const
{
	return QRectF(input.left() - 0.5*width() - pan_offset.x(), input.top() - 0.5*height() - pan_offset.y(), input.width(), input.height());
}

QPointF MapWidget::viewportToView(const QPoint& input) const
{
	return QPointF(input.x() - 0.5*width() - pan_offset.x(), input.y() - 0.5*height() - pan_offset.y());
}

QPointF MapWidget::viewportToView(QPointF input) const
{
	input.rx() -= 0.5*width() + pan_offset.x();
	input.ry() -= 0.5*height() + pan_offset.y();
	return input;
}

QRectF MapWidget::viewToViewport(const QRectF& input) const
{
	return QRectF(input.left() + 0.5*width() + pan_offset.x(), input.top() + 0.5*height() + pan_offset.y(), input.width(), input.height());
}

QRectF MapWidget::viewToViewport(const QRect& input) const
{
	return QRectF(input.left() + 0.5*width() + pan_offset.x(), input.top() + 0.5*height() + pan_offset.y(), input.width(), input.height());
}

QPointF MapWidget::viewToViewport(const QPoint& input) const
{
	return QPointF(input.x() + 0.5*width() + pan_offset.x(), input.y() + 0.5*height() + pan_offset.y());
}

QPointF MapWidget::viewToViewport(QPointF input) const
{
	input.rx() += 0.5*width() + pan_offset.x();
	input.ry() += 0.5*height() + pan_offset.y();
	return input;
}


MapCoord MapWidget::viewportToMap(const QPoint& input) const
{
	return view->viewToMap(viewportToView(input));
}

MapCoordF MapWidget::viewportToMapF(const QPoint& input) const
{
	return view->viewToMapF(viewportToView(input));
}

MapCoordF MapWidget::viewportToMapF(const QPointF& input) const
{
	return view->viewToMapF(viewportToView(input));
}

QPointF MapWidget::mapToViewport(const MapCoord& input) const
{
	return viewToViewport(view->mapToView(input));
}

QPointF MapWidget::mapToViewport(const QPointF& input) const
{
	return viewToViewport(view->mapToView(input));
}

QRectF MapWidget::mapToViewport(const QRectF& input) const
{
	QRectF result;
	rectIncludeSafe(result, mapToViewport(input.topLeft()));
	rectIncludeSafe(result, mapToViewport(input.bottomRight()));
	if (view->getRotation() != 0)
	{
		rectIncludeSafe(result, mapToViewport(input.topRight()));
		rectIncludeSafe(result, mapToViewport(input.bottomLeft()));
	}
	return result;
}

void MapWidget::viewChanged(MapView::ChangeFlags changes)
{
	setDrawingBoundingBox(drawing_dirty_rect_map, drawing_dirty_rect_border, true);
	setActivityBoundingBox(activity_dirty_rect_map, activity_dirty_rect_border, true);
	updateEverything();
	if (changes.testFlag(MapView::ZoomChange))
		updateZoomDisplay();
}

void MapWidget::visibilityChanged(MapView::VisibilityFeature feature, bool active, Template* temp)
{
	switch (feature)
	{
	case MapView::VisibilityFeature::TemplateVisible:
		if (temp && temp->getTemplateState() == Template::Loaded)
		{
			auto const pos = getMapView()->getMap()->findTemplateIndex(temp);
			auto const template_area = temp->calculateTemplateBoundingBox();
			markTemplateCacheDirty(getMapView()->calculateViewBoundingBox(template_area),
			                       temp->getTemplateBoundingBoxPixelBorder(),
			                       pos >= getMapView()->getMap()->getFirstFrontTemplate());
		
		}
		else if (temp && temp->getTemplateState() == Template::Unloaded && active)
		{
			// The template must be loaded.
			QToolTip::showText(QCursor::pos(),
			                   qApp->translate("OpenOrienteering::MainWindow", "Opening %1")
			                   .arg(temp->getTemplateFilename()) );
			// Use a small delay so that some UI events can be processed first.
			QPointer<MapWidget> widget(this);
			QTimer::singleShot(10, temp, ([temp, widget]() {
				if (temp->getTemplateState() != Template::Loaded)
				{
					temp->loadTemplateFile();
					QToolTip::hideText();
					if (temp->getTemplateState() == Template::Invalid)
						QMessageBox::warning(widget.data(),
						                     qApp->translate("OpenOrienteering::MainWindow", "Error"),
						                     qApp->translate("OpenOrienteering::Importer", "Failed to load template '%1', reason: %2")
						                     .arg(temp->getTemplateFilename(), temp->errorString()) );
				}
			}));
		}
		break;
		
	case MapView::VisibilityFeature::GridVisible:
	case MapView::VisibilityFeature::MapVisible:
		map_cache_dirty_rect = rect();
		Q_FALLTHROUGH();
	case MapView::VisibilityFeature::AllTemplatesHidden:
		update();
		break;
		
	default:
		updateEverything();
		break;
	}
}

void MapWidget::setPanOffset(const QPoint& offset)
{
	pan_offset = offset;
	update();
}

void MapWidget::startDragging(const QPoint& cursor_pos)
{
	Q_ASSERT(!dragging);
	Q_ASSERT(!pinching);
	dragging = true;
	drag_start_pos = cursor_pos;
	normal_cursor  = cursor();
	setCursor(Qt::ClosedHandCursor);
}

void MapWidget::updateDragging(const QPoint& cursor_pos)
{
	Q_ASSERT(dragging);
	view->setPanOffset(cursor_pos - drag_start_pos);
}

void MapWidget::finishDragging(const QPoint& cursor_pos)
{
	Q_ASSERT(dragging);
	dragging = false;
	current_pressed_buttons = 0;
	view->finishPanning(cursor_pos - drag_start_pos);
	setCursor(normal_cursor);
}

void MapWidget::cancelDragging()
{
	dragging = false;
	current_pressed_buttons = 0;
	view->setPanOffset(QPoint());
	setCursor(normal_cursor);
}

qreal MapWidget::startPinching(const QPoint& center)
{
	Q_ASSERT(!dragging);
	Q_ASSERT(!pinching);
	pinching = true;
	drag_start_pos  = center;
	pinching_center = center;
	pinching_factor = 1.0;
	pinching_angle  = 0.0;
	return pinching_factor;
}

void MapWidget::updatePinching(const QPoint& center, qreal factor, qreal angle)
{
	Q_ASSERT(pinching);
	pinching_center = center;
	pinching_factor = factor;
	pinching_angle  = auto_rotation_active ? 0.0 : angle;
	updateZoomDisplay();
	update();
}

void MapWidget::finishPinching(const QPoint& center, qreal factor, qreal angle)
{
	pinching = false;
	current_pressed_buttons = 0;
	view->finishPanning(center - drag_start_pos);
	view->setZoom(factor * view->getZoom(), viewportToView(center));
	if (!auto_rotation_active && angle != 0.0)
	{
		double delta_rad = qDegreesToRadians(angle);
		QPointF rot_center_view = viewportToView(center);
		auto rot_center_map = MapCoordF(view->viewToMap(rot_center_view));
		auto old_center = MapCoordF(view->center());

		view->setRotation(view->getRotation() + delta_rad);

		// Adjust center so the pinch center stays fixed on screen
		auto offset = old_center - rot_center_map;
		auto cos_d = cos(delta_rad);
		auto sin_d = sin(delta_rad);
		auto new_center = rot_center_map + MapCoordF(
		     offset.x() * cos_d + offset.y() * sin_d,
		    -offset.x() * sin_d + offset.y() * cos_d);
		view->setCenter(MapCoord(new_center));
	}
}

void MapWidget::cancelPinching()
{
	pinching = false;
	current_pressed_buttons = 0;
	pinching_factor = 1.0;
	pinching_angle  = 0.0;
	update();
}

void MapWidget::moveMap(int steps_x, int steps_y)
{
	if (steps_x != 0 || steps_y != 0)
	{
		try
		{
			constexpr auto move_factor = 0.25;
			auto offset = MapCoord::fromNative64( qRound64(view->pixelToLength(width() * steps_x * move_factor)),
			                                      qRound64(view->pixelToLength(height() * steps_y * move_factor)) );
			view->setCenter(view->center() + offset);
		}
		catch (std::range_error&)
		{
			// Do nothing
		}
	}
}

void MapWidget::ensureVisibilityOfRect(QRectF map_rect, ZoomOption zoom_option)
{
	// Amount in pixels that is scrolled "too much" if the rect is not completely visible
	// TODO: change to absolute size using dpi value
	const int pixel_border = 70;
	auto viewport_rect = mapToViewport(map_rect).toAlignedRect();
	
	// TODO: this method assumes that the viewport is not rotated.
	
	if (rect().contains(viewport_rect.topLeft()) && rect().contains(viewport_rect.bottomRight()))
		return;
	
	auto offset = MapCoordF{ 0, 0 };
	
	if (viewport_rect.left() < 0)
		offset.rx() = view->pixelToLength(viewport_rect.left() - pixel_border) / 1000.0;
	else if (viewport_rect.right() > width())
		offset.rx() = view->pixelToLength(viewport_rect.right() - width() + pixel_border) / 1000.0;
	
	if (viewport_rect.top() < 0)
		offset.ry() = view->pixelToLength(viewport_rect.top() - pixel_border) / 1000.0;
	else if (viewport_rect.bottom() > height())
		offset.ry() = view->pixelToLength(viewport_rect.bottom() - height() + pixel_border) / 1000.0;
	
	if (!qIsNull(offset.lengthSquared()))
		view->setCenter(view->center() + offset);
	
	// If the rect is still not completely in view, we have to zoom out
	viewport_rect = mapToViewport(map_rect).toAlignedRect();
	if (!(rect().contains(viewport_rect.topLeft()) && rect().contains(viewport_rect.bottomRight())))
		adjustViewToRect(map_rect, zoom_option);
}

void MapWidget::adjustViewToRect(QRectF map_rect, ZoomOption zoom_option)
{
	view->setCenter(MapCoord{ map_rect.center() });
	
	if (map_rect.isValid())
	{
		// NOTE: The loop is an inelegant way to fight inaccuracies that occur somewhere ...
		const int pixel_border = 15;
		const float initial_zoom = view->getZoom();
		for (int i = 0; i < 10; ++i)
		{
			float zoom_factor = qMin(height() / (view->lengthToPixel(1000.0 * map_rect.height()) + 2*pixel_border),
			                         width() / (view->lengthToPixel(1000.0 * map_rect.width()) + 2*pixel_border));
			float zoom = view->getZoom() * zoom_factor;
			if (zoom_option == DiscreteZoom)
			{
				zoom = pow(2, 0.5 * floor(2.0 * (std::log2(zoom) - std::log2(initial_zoom))) + std::log2(initial_zoom));
			}
			view->setZoom(zoom);
		}
	}
}

void MapWidget::moveDirtyRect(QRect& dirty_rect, qreal x, qreal y)
{
	if (dirty_rect.isValid())
		dirty_rect = dirty_rect.translated(x, y).intersected(rect());
}

void MapWidget::markTemplateCacheDirty(const QRectF& view_rect, int pixel_border, bool front_cache)
{
	QRect& cache_dirty_rect = front_cache ? above_template_cache_dirty_rect : below_template_cache_dirty_rect;
	QRectF viewport_rect = viewToViewport(view_rect);
	QRect integer_rect = QRect(viewport_rect.left() - (1+pixel_border), viewport_rect.top() - (1+pixel_border),
							   viewport_rect.width() + 2*(1+pixel_border), viewport_rect.height() + 2*(1+pixel_border));
	
	if (!integer_rect.intersects(rect()))
		return;
	
	if (cache_dirty_rect.isValid())
		cache_dirty_rect = cache_dirty_rect.united(integer_rect);
	else
		cache_dirty_rect = integer_rect;
	
	update(integer_rect);
}

void MapWidget::markObjectAreaDirty(const QRectF& map_rect)
{
	updateMapRect(map_rect, 0, map_cache_dirty_rect);
}

void MapWidget::setDrawingBoundingBox(QRectF map_rect, int pixel_border, bool do_update)
{
	Q_UNUSED(do_update);
	clearDrawingBoundingBox();
	if (map_rect.isValid())
	{
		drawing_dirty_rect_map = map_rect;
		drawing_dirty_rect_border = pixel_border;
		updateMapRect(drawing_dirty_rect_map, drawing_dirty_rect_border, drawing_dirty_rect);
	}
}

void MapWidget::clearDrawingBoundingBox()
{
	drawing_dirty_rect_map.setWidth(0);
	if (drawing_dirty_rect.isValid())
	{
		update(drawing_dirty_rect);
		drawing_dirty_rect.setWidth(0);
	}
}

void MapWidget::setActivityBoundingBox(QRectF map_rect, int pixel_border, bool do_update)
{
	Q_UNUSED(do_update);
	clearActivityBoundingBox();
	if (map_rect.isValid())
	{
		activity_dirty_rect_map = map_rect;
		activity_dirty_rect_border = pixel_border;
		updateMapRect(activity_dirty_rect_map, activity_dirty_rect_border, activity_dirty_rect);
	}
}

void MapWidget::clearActivityBoundingBox()
{
	activity_dirty_rect_map.setWidth(0);
	if (activity_dirty_rect.isValid())
	{
		update(activity_dirty_rect);
		activity_dirty_rect.setWidth(0);
	}
}

void MapWidget::updateDrawing(const QRectF& map_rect, int pixel_border)
{
	QRect viewport_rect = calculateViewportBoundingBox(map_rect, pixel_border);
	
	if (viewport_rect.intersects(rect()))
		update(viewport_rect);
}

void MapWidget::updateMapRect(const QRectF& map_rect, int pixel_border, QRect& cache_dirty_rect)
{
	QRect viewport_rect = calculateViewportBoundingBox(map_rect, pixel_border);
	updateViewportRect(viewport_rect, cache_dirty_rect);
}

void MapWidget::updateViewportRect(QRect viewport_rect, QRect& cache_dirty_rect)
{
	if (viewport_rect.intersects(rect()))
	{
		if (cache_dirty_rect.isValid())
			cache_dirty_rect = cache_dirty_rect.united(viewport_rect);
		else
			cache_dirty_rect = viewport_rect;
		
		update(viewport_rect);
	}
}

void MapWidget::updateDrawingLater(const QRectF& map_rect, int pixel_border)
{
	QRect viewport_rect = calculateViewportBoundingBox(map_rect, pixel_border);
	
	if (viewport_rect.intersects(rect()))
	{
		if (!cached_update_rect.isValid())
		{
			// Start the update timer
			QTimer::singleShot(15, this, &MapWidget::updateDrawingLaterSlot);
		}
		
		// NOTE: this may require a mutex for concurrent access with updateDrawingLaterSlot()?
		rectIncludeSafe(cached_update_rect, viewport_rect);
	}
}

void MapWidget::updateDrawingLaterSlot()
{
	updateEverythingInRect(cached_update_rect);
	cached_update_rect = QRect();
}

void MapWidget::updateDeferredTemplateCaches()
{
	deferred_template_cache_update_pending = false;

	if (!view || view->areAllTemplatesHidden())
		return;

	bool updated = false;
	if (below_template_cache_dirty_rect.isValid() && isBelowTemplateVisible())
	{
		updateTemplateCache(below_template_cache, below_template_cache_dirty_rect, below_template_cache_state, 0, view->getMap()->getFirstFrontTemplate() - 1, true);
		updated = true;
	}

	if (above_template_cache_dirty_rect.isValid() && isAboveTemplateVisible())
	{
		updateTemplateCache(above_template_cache, above_template_cache_dirty_rect, above_template_cache_state, view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1, false);
		updated = true;
	}

	if (updated)
		update();
}

void MapWidget::updateEverything()
{
	map_cache_dirty_rect = rect();
	below_template_cache_dirty_rect = map_cache_dirty_rect;
	above_template_cache_dirty_rect = map_cache_dirty_rect;
	update(map_cache_dirty_rect);
}

void MapWidget::updateEverythingInRect(const QRect& dirty_rect)
{
	rectIncludeSafe(map_cache_dirty_rect, dirty_rect);
	rectIncludeSafe(below_template_cache_dirty_rect, dirty_rect);
	rectIncludeSafe(above_template_cache_dirty_rect, dirty_rect);
	update(dirty_rect);
}

QRect MapWidget::calculateViewportBoundingBox(const QRectF& map_rect, int pixel_border) const
{
	QRectF view_rect = view->calculateViewBoundingBox(map_rect);
	view_rect.adjust(-pixel_border, -pixel_border, +pixel_border, +pixel_border);
	return viewToViewport(view_rect).toAlignedRect();
}

void MapWidget::setZoomDisplay(std::function<void(const QString&)> setter)
{
	this->zoom_display = setter;
	updateZoomDisplay();
}

void MapWidget::setCursorposLabel(QLabel* cursorpos_label)
{
	this->cursorpos_label = cursorpos_label;
}

void MapWidget::updateZoomDisplay()
{
	if (zoom_display)
	{
		auto zoom = view->getZoom();
		if (pinching)
			zoom *= pinching_factor;
		zoom_display(tr("%1x", "Zoom factor").arg(zoom, 0, 'g', 3));
	}
}

void MapWidget::setCoordsDisplay(CoordsType type)
{
	coords_type = type;
	updateCursorposLabel(last_cursor_pos);
}

void MapWidget::updateCursorposLabel(const MapCoordF& pos)
{
	last_cursor_pos = pos;
	
	if (!cursorpos_label)
		return;
	
	if (coords_type == MAP_COORDS)
	{
		cursorpos_label->setText( QStringLiteral("%1 %2 (%3)").
		  arg(locale().toString(pos.x(), 'f', 2),
		      locale().toString(-pos.y(), 'f', 2),
		      tr("mm", "millimeters")) );
	}
	else
	{
		const Georeferencing& georef = view->getMap()->getGeoreferencing();
		bool ok = true;
		if (coords_type == PROJECTED_COORDS)
		{
			const QPointF projected_point(georef.toProjectedCoords(pos));
			if (qAbs(georef.getCombinedScaleFactor() - 1.0) < 0.02)
			{
				// Grid unit differs less than 2% from meter.
				cursorpos_label->setText(
				  QStringLiteral("%1 %2 (%3)").
				  arg(QString::number(projected_point.x(), 'f', 0),
				      QString::number(projected_point.y(), 'f', 0),
				      tr("m", "meters"))
				); 
			}
			else
			{
				cursorpos_label->setText(
				  QStringLiteral("%1 %2").
				  arg(QString::number(projected_point.x(), 'f', 0),
				      QString::number(projected_point.y(), 'f', 0))
				); 
			}
		}
		else if (coords_type == GEOGRAPHIC_COORDS)
		{
			const LatLon lat_lon(georef.toGeographicCoords(pos, &ok));
			cursorpos_label->setText(
			  QString::fromUtf8("%1° %2°").
			  arg(locale().toString(lat_lon.latitude(), 'f', 6),
			      locale().toString(lat_lon.longitude(), 'f', 6))
			); 
		}
		else if (coords_type == GEOGRAPHIC_COORDS_DMS)
		{
			const LatLon lat_lon(georef.toGeographicCoords(pos, &ok));
			cursorpos_label->setText(
			  QStringLiteral("%1 %2").
			  arg(georef.degToDMS(lat_lon.latitude()),
			      georef.degToDMS(lat_lon.longitude()))
			); 
		}
		else
		{
			// shall never happen
			ok = false;
		}
		
		if (!ok)
			cursorpos_label->setText(tr("Error"));
	}
}

int MapWidget::getTimeSinceLastInteraction()
{
	if (current_pressed_buttons != 0 || dragging || pinching)
		return 0;
	else
		return last_mouse_release_time.msecsTo(QTime::currentTime());
}

void MapWidget::setGPSDisplay(GPSDisplay* gps_display)
{
	this->gps_display = gps_display;
}

void MapWidget::setTemporaryMarkerDisplay(GPSTemporaryMarkers* marker_display)
{
	this->marker_display = marker_display;
}

QWidget* MapWidget::getContextMenu()
{
	return context_menu;
}

QSize MapWidget::sizeHint() const
{
    return QSize(640, 480);
}

void MapWidget::showHelpMessage(QPainter* painter, const QString& text) const
{
	painter->fillRect(rect(), QColor(Qt::gray));
	
	QFont font = painter->font();
	int pixel_size = font.pixelSize();
	if (pixel_size > 0)
	{
		font.setPixelSize(pixel_size * 2);
	}
	else
	{
		pixel_size = font.pointSize();
		font.setPointSize(pixel_size * 2);
	}
	font.setBold(true);
	painter->setFont(font);
	painter->drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, text);
}

bool MapWidget::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::Gesture:
		gestureEvent(static_cast<QGestureEvent*>(event));
		return event->isAccepted();
		
	case QEvent::TabletPress:
		finger_touch_active = false;
		break;
	
	case QEvent::TouchBegin:
	{
		auto* te = static_cast<QTouchEvent*>(event);
		finger_touch_active = true;
		for (const auto& tp : te->touchPoints())
		{
			if (tp.flags() & QTouchEvent::TouchPoint::Pen)
			{
				finger_touch_active = false;
				break;
			}
		}
		if (te->touchPoints().count() >= 2)
			return true;
		break;
	}
	case QEvent::TouchUpdate:
		if (static_cast<QTouchEvent*>(event)->touchPoints().count() >= 2)
			return true;
		break;
	case QEvent::TouchEnd:
	case QEvent::TouchCancel:
		finger_touch_active = false;
		if (static_cast<QTouchEvent*>(event)->touchPoints().count() >= 2)
			return true;
		break;
		
	case QEvent::KeyPress:
		// No focus changing in QWidget::event if Tab is handled by tool.
		if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Tab
		    && keyPressEventFilter(static_cast<QKeyEvent*>(event)))
			return true;
		break;
		
	default:
		; // nothing
	}
	
    return QWidget::event(event);
}

void MapWidget::gestureEvent(QGestureEvent* event)
{
	if (tool && tool->gestureEvent(event, this))
	{
		event->accept();
		return;
	}
	
	if (QGesture* gesture = event->gesture(Qt::PinchGesture))
	{
		QPinchGesture* pinch = static_cast<QPinchGesture *>(gesture);
		QPoint center = pinch->centerPoint().toPoint();
		qreal factor = pinch->totalScaleFactor();
		qreal rotation_angle = pinch->totalRotationAngle();
		switch (pinch->state())
		{
		case Qt::GestureStarted:
			if (dragging && !touch_pan_only)
				cancelDragging();
			if (pinching)
				cancelPinching();
			if (tool)
				tool->gestureStarted();
			factor = startPinching(center);
			pinch->setTotalScaleFactor(factor);
			break;
		case Qt::GestureUpdated:
			updatePinching(center, factor, rotation_angle);
			break;
		case Qt::GestureFinished:
			finishPinching(center, factor, rotation_angle);
			break;
		case Qt::GestureCanceled:
			cancelPinching();
			break;
		default:
			Q_UNREACHABLE(); // unknown gesture state
		}
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MapWidget::paintEvent(QPaintEvent* event)
{
	// Draw on the widget
	QPainter painter(this);
	QRect exposed = event->rect();
	
	if (!view)
	{
		painter.fillRect(exposed, QColor(Qt::gray));
		return;
	}
	
	// No colors, symbols, or objects? Provide a little help message ...
	bool no_contents = view->getMap()->getNumObjects() == 0 && view->getMap()->getNumTemplates() == 0 && !view->isGridVisible();
	
	QTransform transform = painter.worldTransform();

	// Update all dirty caches
	// TODO: It would be an idea to do these updates in a background thread and use the old caches in the meantime
	updateAllDirtyCaches();
	
	QRect target = exposed;
	if (pinching)
	{
		// Build pinch transform (may include rotation)
		QTransform pinch_xf;
		pinch_xf.translate(pinching_center.x(), pinching_center.y());
		if (pinching_angle != 0.0)
			pinch_xf.rotate(pinching_angle);
		pinch_xf.scale(pinching_factor, pinching_factor);
		pinch_xf.translate(-drag_start_pos.x(), -drag_start_pos.y());

		// Check if there are uncovered corners (zoom-out or rotation)
		QPolygon covered = pinch_xf.mapToPolygon(exposed);
		QRegion uncovered = QRegion(exposed) - QRegion(covered);
		if (uncovered.isEmpty() || !drawPinchUncoveredRegion(painter, exposed))
			painter.fillRect(exposed, QColor(Qt::gray));

		painter.translate(pinching_center.x(), pinching_center.y());
		if (pinching_angle != 0.0)
			painter.rotate(pinching_angle);
		painter.scale(pinching_factor, pinching_factor);
		painter.translate(-drag_start_pos.x(), -drag_start_pos.y());
	}
	else if (pan_offset != QPoint())
	{
		if (!drawPanUncoveredRegion(painter, exposed))
		{
			if (pan_offset.x() > 0)
				painter.fillRect(QRect(0, pan_offset.y(), pan_offset.x(), height() - pan_offset.y()), QColor(Qt::gray));
			else if (pan_offset.x() < 0)
				painter.fillRect(QRect(width() + pan_offset.x(), pan_offset.y(), -pan_offset.x(), height() - pan_offset.y()), QColor(Qt::gray));
			if (pan_offset.y() > 0)
				painter.fillRect(QRect(0, 0, width(), pan_offset.y()), QColor(Qt::gray));
			else if (pan_offset.y() < 0)
				painter.fillRect(QRect(0, height() + pan_offset.y(), width(), -pan_offset.y()), QColor(Qt::gray));
		}
		target.translate(pan_offset);
	}
	
	if (!view->areAllTemplatesHidden() && isBelowTemplateVisible() && !below_template_cache.isNull() && view->getMap()->getFirstFrontTemplate() > 0)
	{
		drawTemplateCache(painter, below_template_cache, below_template_cache_state, target, exposed, true);
	}
	else if (show_help && no_contents)
	{
		painter.save();
		painter.setTransform(transform);
		if (view->getMap()->getNumColors() == 0)
			showHelpMessage(&painter, tr("Empty map!\n\nStart by defining some colors:\nSelect Symbols -> Color window to\nopen the color dialog and\ndefine the colors there."));
		else if (view->getMap()->getNumSymbols() == 0)
			showHelpMessage(&painter, tr("No symbols!\n\nNow define some symbols:\nRight-click in the symbol bar\nand select \"New symbol\"\nto create one."));
		else
			showHelpMessage(&painter, tr("Ready to draw!\n\nStart drawing or load a base map.\nTo load a base map, click\nTemplates -> Open template...") + QLatin1String("\n\n") + tr("Hint: Hold the middle mouse button to drag the map,\nzoom using the mouse wheel, if available."));
		painter.restore();
	}
	else
	{
		painter.fillRect(target, Qt::white);
	}
	
	const auto map_visibility = view->effectiveMapVisibility();
	if (!map_cache.isNull() && map_visibility.visible)
	{
		qreal saved_opacity = painter.opacity();
		painter.setOpacity(map_visibility.opacity);
		painter.drawImage(target, map_cache, exposed);
		painter.setOpacity(saved_opacity);
	}
	
	if (!view->areAllTemplatesHidden() && isAboveTemplateVisible() && !above_template_cache.isNull() && view->getMap()->getNumTemplates() - view->getMap()->getFirstFrontTemplate() > 0)
		drawTemplateCache(painter, above_template_cache, above_template_cache_state, target, exposed);
	
	//painter.setClipRect(exposed);
	
	// Show current drawings
	if (activity_dirty_rect.isValid())
		activity->draw(&painter, this);
	
	if (drawing_dirty_rect.isValid())
		tool->draw(&painter, this);
	
	
	// Draw temporary GPS marker display
	if (marker_display)
		marker_display->paint(&painter);
	
	// Draw GPS display
	if (gps_display)
		gps_display->paint(&painter);
	
	// Draw touch cursor
	if (touch_cursor && tool && tool->usesTouchCursor())
		touch_cursor->paint(&painter);
	
	
	painter.setWorldTransform(transform, false);
}

void MapWidget::resizeEvent(QResizeEvent* event)
{
	map_cache_dirty_rect = rect();
	below_template_cache_dirty_rect = map_cache_dirty_rect;
	above_template_cache_dirty_rect = map_cache_dirty_rect;
	
	if (map_cache.width() < map_cache_dirty_rect.width() ||
	    map_cache.height() < map_cache_dirty_rect.height())
	{
		map_cache = QImage();
		below_template_cache = QImage();
		above_template_cache = QImage();
		below_template_cache_state.valid = false;
		above_template_cache_state.valid = false;
	}
	
	for (QObject* const child : children())
	{
		if (QWidget* child_widget = qobject_cast<ActionGridBar*>(child))
		{
			child_widget->resize(event->size().width(), child_widget->sizeHint().height());
		}
		else if (QWidget* child_widget = qobject_cast<KeyButtonBar*>(child))
		{
			QSize size = child_widget->sizeHint();
			QRect map_widget_rect = rect();
			child_widget->setGeometry(
				qMax(0, qRound(map_widget_rect.center().x() - 0.5f * size.width())),
				qMax(0, map_widget_rect.bottom() - size.height()),
				qMin(size.width(), map_widget_rect.width()),
				qMin(size.height(), map_widget_rect.height()) );
		}
	}
	
	QWidget::resizeEvent(event);
}


void MapWidget::mousePressEvent(QMouseEvent* event)
{
	current_pressed_buttons = event->buttons();
	// Touch-pan-only: intercept before touch cursor and tool
	if (touch_pan_only && finger_touch_active && event->button() == Qt::LeftButton)
	{
		startDragging(event->pos());
		event->accept();
		return;
	}
	if (touch_cursor && tool && tool->usesTouchCursor())
	{
		touch_cursor->mousePressEvent(event);
		if (event->type() == QEvent::MouseMove)
		{
			_mouseMoveEvent(event);
			return;
		}
	}
	_mousePressEvent(event);
}


void MapWidget::_mousePressEvent(QMouseEvent* event)
{
	if (dragging || pinching)
	{
		event->accept();
		return;
	}
	
	if (tool && tool->mousePressEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
	
	if (event->button() == Qt::MiddleButton)
	{
		startDragging(event->pos());
		event->accept();
	}
	else if (event->button() == Qt::RightButton)
	{
		if (!context_menu->isEmpty())
			context_menu->popup(event->globalPos());
	}
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
	// Touch-pan-only: skip touch cursor and tool
	if (touch_pan_only && finger_touch_active)
	{
		if (dragging)
			updateDragging(event->pos());
		event->accept();
		return;
	}
	if (touch_cursor && tool && tool->usesTouchCursor())
	{
		if (!touch_cursor->mouseMoveEvent(event))
			return;
	}
	_mouseMoveEvent(event);
}

void MapWidget::_mouseMoveEvent(QMouseEvent* event)
{
	if (pinching)
	{
		event->accept();
		return;
	}
	else if (dragging)
	{
		updateDragging(event->pos());
		return;
	}
	else
    {
		updateCursorposLabel(view->viewToMapF(viewportToView(event->pos())));
    }
	
	if (tool && tool->mouseMoveEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	current_pressed_buttons = event->buttons();
	last_mouse_release_time = QTime::currentTime();
	// Touch-pan-only: skip touch cursor and tool
	if (touch_pan_only && finger_touch_active)
	{
		if (dragging)
			finishDragging(event->pos());
		event->accept();
		return;
	}
	if (touch_cursor && tool && tool->usesTouchCursor())
	{
		if (!touch_cursor->mouseReleaseEvent(event))
			return;
	}
	_mouseReleaseEvent(event);
}

void MapWidget::_mouseReleaseEvent(QMouseEvent* event)
{
	if (dragging)
	{
		finishDragging(event->pos());
		event->accept();
		return;
	}
	
	if (tool && tool->mouseReleaseEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	// Touch-pan-only: ignore touch double-click
	if (touch_pan_only && finger_touch_active)
	{
		event->accept();
		return;
	}
	if (touch_cursor && tool && tool->usesTouchCursor())
	{
		if (!touch_cursor->mouseDoubleClickEvent(event))
			return;
	}
	_mouseDoubleClickEvent(event);
}

void MapWidget::_mouseDoubleClickEvent(QMouseEvent* event)
{
	if (tool && tool->mouseDoubleClickEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
	
	QWidget::mouseDoubleClickEvent(event);
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
	if (event->orientation() == Qt::Vertical)
	{
		if (view)
		{
			auto degrees = event->delta() / 8.0;
			auto num_steps = degrees / 15.0;
			auto cursor_pos_view = viewportToView(event->pos());
			bool preserve_cursor_pos = (event->modifiers() & Qt::ControlModifier) == 0;
			if (num_steps < 0 && !Settings::getInstance().getSettingCached(Settings::MapEditor_ZoomOutAwayFromCursor).toBool())
				preserve_cursor_pos = !preserve_cursor_pos;
			if (preserve_cursor_pos)
			{
				view->zoomSteps(num_steps, cursor_pos_view);
			}
			else
			{
				view->zoomSteps(num_steps);
				updateCursorposLabel(view->viewToMapF(cursor_pos_view));
			}
			
			// Send a mouse move event to the current tool as zooming out can move the mouse position on the map
			if (tool)
			{
				QMouseEvent mouse_event{ QEvent::HoverMove, event->pos(), Qt::NoButton, QApplication::mouseButtons(), Qt::NoModifier };
				tool->mouseMoveEvent(&mouse_event, view->viewToMapF(cursor_pos_view), this);
			}
		}
		
		event->accept();
	}
	else
		event->ignore();
}

void MapWidget::leaveEvent(QEvent* event)
{
	if (tool)
		tool->leaveEvent(event);
}

bool MapWidget::keyPressEventFilter(QKeyEvent* event)
{
	if (tool && tool->keyPressEvent(event))
	{
		return true;
	}
	
	switch (event->key())
	{
	case Qt::Key_F6:
		if (dragging)
			finishDragging(mapFromGlobal(QCursor::pos()));
		else
			startDragging(mapFromGlobal(QCursor::pos()));
		return true;
		
	case Qt::Key_Up:
		moveMap(0, -1);
		return true;
		
	case Qt::Key_Down:
		moveMap(0, 1);
		return true;
		
	case Qt::Key_Left:
		moveMap(-1, 0);
		return true;
		
	case Qt::Key_Right:
		moveMap(1, 0);
		return true;
		
	default:
		return false;
	}
}

bool MapWidget::keyReleaseEventFilter(QKeyEvent* event)
{
	if (tool && tool->keyReleaseEvent(event))
	{
		return true; // NOLINT
	}
	
	return false;
}

QVariant MapWidget::inputMethodQuery(Qt::InputMethodQuery property) const
{
	return inputMethodQuery(property, {});
}

QVariant MapWidget::inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const
{
	QVariant result;
	if (tool)
		result = tool->inputMethodQuery(property, argument);
	if (!result.isValid())
		result = QWidget::inputMethodQuery(property);
	return result;
}

void MapWidget::inputMethodEvent(QInputMethodEvent* event)
{
	if (tool)
		tool->inputMethodEvent(event);
}

void MapWidget::enableTouchCursor(bool enabled)
{
	if (enabled && !touch_cursor)
	{
		touch_cursor.reset(new TouchCursor(this));
	}
	else if (!enabled && touch_cursor)
	{
		touch_cursor->updateMapWidget(false);
		touch_cursor.reset(nullptr);
	}
}

void MapWidget::focusOutEvent(QFocusEvent* event)
{
	if (tool)
		tool->focusOutEvent(event);
	QWidget::focusOutEvent(event);
}

void MapWidget::contextMenuEvent(QContextMenuEvent* event)
{
	if (event->reason() == QContextMenuEvent::Mouse)
	{
		// HACK: Ignore context menu events caused by the mouse, because right click
		// events need to be sent to the current tool first.
		event->ignore();
		return;
	}
	
	if (!context_menu->isEmpty())
		context_menu->popup(event->globalPos());
	
	event->accept();
}

void MapWidget::updatePlaceholder()
{
	if (!show_help)
		return;
	
	auto const* map = view->getMap();
	if (map->getNumObjects() > 1)
		return;
	
	if (map->getNumColors() < 2
	    || map->getNumSymbols() < 2
	    || map->getNumTemplates() < 2)
	{
		update();
	}
}

bool MapWidget::containsVisibleTemplate(int first_template, int last_template) const
{
	if (first_template > last_template)
		return false;	// no template visible
		
	Map* map = view->getMap();
	for (int i = first_template; i <= last_template; ++i)
	{
		if (view->isTemplateVisible(map->getTemplate(i)))
			return true;
	}
	
	return false;
}

inline
bool MapWidget::isAboveTemplateVisible() const
{
	return containsVisibleTemplate(view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1);
}

inline
bool MapWidget::isBelowTemplateVisible() const
{
	return containsVisibleTemplate(0, view->getMap()->getFirstFrontTemplate() - 1);
}

QTransform MapWidget::mapToViewportTransform() const
{
	const auto origin = mapToViewport(QPointF(0.0, 0.0));
	const auto x_axis = mapToViewport(QPointF(1.0, 0.0));
	const auto y_axis = mapToViewport(QPointF(0.0, 1.0));
	return QTransform(x_axis.x() - origin.x(),
	                  x_axis.y() - origin.y(),
	                  y_axis.x() - origin.x(),
	                  y_axis.y() - origin.y(),
	                  origin.x(),
	                  origin.y());
}

void MapWidget::recordTemplateCacheState(TemplateCacheViewState& state, const QImage& cache) const
{
	// Record the transform that was actually used to paint the cache:
	// translate(width/2, height/2) * worldTransform — without pan_offset.
	// mapToViewportTransform() includes pan_offset, so we must remove it.
	auto transform = mapToViewportTransform();
	if (pan_offset != QPoint())
		transform *= QTransform::fromTranslate(-pan_offset.x(), -pan_offset.y());
	state.map_to_viewport = transform;
	state.cache_size = cache.size();
	state.valid = !cache.isNull();
}

bool MapWidget::canReuseTemplateCache(const QImage& cache, const TemplateCacheViewState& state) const
{
	return state.valid && !cache.isNull() && state.cache_size == size();
}

bool MapWidget::shouldDeferTemplateCacheUpdate(const QImage& cache, const QRect& dirty_rect, const TemplateCacheViewState& state) const
{
	return dirty_rect == rect() && canReuseTemplateCache(cache, state);
}

void MapWidget::drawTemplateCache(QPainter& painter, const QImage& cache, const TemplateCacheViewState& state, const QRect& target, const QRect& exposed, bool use_background) const
{
	if (!canReuseTemplateCache(cache, state))
	{
		painter.drawImage(target, cache, exposed);
		return;
	}

	// Compare using cache-space transforms (without pan_offset) so that
	// pure panning uses the efficient target-shift path, not the correction path.
	auto current_cache_transform = mapToViewportTransform();
	if (pan_offset != QPoint())
		current_cache_transform *= QTransform::fromTranslate(-pan_offset.x(), -pan_offset.y());

	if (transformsMatch(state.map_to_viewport, current_cache_transform))
	{
		// View state unchanged — target already includes pan_offset shift.
		painter.drawImage(target, cache, exposed);
		return;
	}

	bool invertible = false;
	const auto cache_correction = current_cache_transform * state.map_to_viewport.inverted(&invertible);
	if (!invertible)
	{
		painter.drawImage(target, cache, exposed);
		return;
	}

	// Fill background before drawing the corrected cache so that uncovered
	// corner areas (from the rotation/zoom correction) show the expected
	// background instead of the widget background.  Only done in this
	// correction path to avoid unnecessary redraws in the normal path.
	if (use_background)
		painter.fillRect(target, Qt::white);

	painter.save();
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	// First apply pan_offset shift (target vs exposed), then cache correction.
	painter.translate(target.x() - exposed.x(), target.y() - exposed.y());
	painter.setWorldTransform(cache_correction, true);
	painter.drawImage(QPointF(0.0, 0.0), cache);
	painter.restore();
}

bool MapWidget::drawPinchUncoveredRegion(QPainter& painter, const QRect& exposed) const
{
	Q_ASSERT(pinching);

	int rendering_level = Settings::getInstance().getSettingCached(Settings::MapDisplay_GestureExtraRendering).toInt();
	if (rendering_level <= 0)
		return false;

	// The pinch transform scales (and optionally rotates) the cache around
	// pinching_center.  We use the exact same composite transform that
	// paintEvent applies to the cache to guarantee perfect alignment.

	// 1. Build the pinch-to-viewport transform (same as paintEvent sets on the painter)
	QTransform pinch_xf;
	pinch_xf.translate(pinching_center.x(), pinching_center.y());
	if (pinching_angle != 0.0)
		pinch_xf.rotate(pinching_angle);
	pinch_xf.scale(pinching_factor, pinching_factor);
	pinch_xf.translate(-drag_start_pos.x(), -drag_start_pos.y());

	// 2. Determine the area covered by the scaled/rotated cache
	QRect cache_rect = exposed;
	QPolygon covered = pinch_xf.mapToPolygon(cache_rect);
	QRegion uncovered = QRegion(exposed) - QRegion(covered);
	if (uncovered.isEmpty())
		return true;

	// 3. Set up the composite transform
	auto setupTransform = [&](QPainter& p) {
		p.translate(pinching_center.x(), pinching_center.y());
		if (pinching_angle != 0.0)
			p.rotate(pinching_angle);
		p.scale(pinching_factor, pinching_factor);
		p.translate(-drag_start_pos.x(), -drag_start_pos.y());
		p.translate(width() / 2.0, height() / 2.0);
		p.setWorldTransform(view->worldTransform(), true);
	};

	// Compute map_rect from the inverse of the full composite transform
	QTransform composite = pinch_xf;
	composite.translate(width() / 2.0, height() / 2.0);
	composite = view->worldTransform() * composite;
	bool invertible = false;
	QTransform inv = composite.inverted(&invertible);
	if (!invertible)
		return false;
	QRectF map_rect = inv.mapRect(QRectF(exposed));

	double ppm = view->calculateFinalZoomFactor() * pinching_factor;

	// 4. Draw into the uncovered region
	painter.save();
	painter.setClipRegion(uncovered);

	// Below templates (with white background)
	Map* map = view->getMap();
	bool has_below_templates = !view->areAllTemplatesHidden()
	                           && isBelowTemplateVisible()
	                           && map->getFirstFrontTemplate() > 0;
	if (has_below_templates)
	{
		painter.fillRect(exposed, Qt::white);
		painter.save();
		setupTransform(painter);
		map->drawTemplates(&painter, map_rect, 0, map->getFirstFrontTemplate() - 1, view, true);
		painter.restore();
	}
	else
	{
		painter.fillRect(exposed, Qt::white);
	}

	// Map objects (only at full rendering level)
	if (rendering_level >= 2)
	{
		const auto map_visibility = view->effectiveMapVisibility();
		if (map_visibility.visible)
		{
			painter.save();
			qreal saved_opacity = painter.opacity();
			painter.setOpacity(map_visibility.opacity);

			RenderConfig::Options options(RenderConfig::Screen | RenderConfig::HelperSymbols);
			bool use_antialiasing = force_antialiasing || Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool();
			if (use_antialiasing)
				painter.setRenderHint(QPainter::Antialiasing);
			else
				options |= RenderConfig::DisableAntialiasing | RenderConfig::ForceMinSize;

			setupTransform(painter);
			RenderConfig config = { *map, map_rect, ppm, options, 1.0 };
			map->draw(&painter, config);

			painter.setOpacity(saved_opacity);
			painter.restore();
		}
	}

	// Above templates
	if (!view->areAllTemplatesHidden() && isAboveTemplateVisible()
	    && map->getNumTemplates() - map->getFirstFrontTemplate() > 0)
	{
		painter.save();
		setupTransform(painter);
		map->drawTemplates(&painter, map_rect, map->getFirstFrontTemplate(), map->getNumTemplates() - 1, view, true);
		painter.restore();
	}

	painter.restore();
	return true;
}

bool MapWidget::drawPanUncoveredRegion(QPainter& painter, const QRect& exposed) const
{
	Q_ASSERT(pan_offset != QPoint());

	int rendering_level = Settings::getInstance().getSettingCached(Settings::MapDisplay_GestureExtraRendering).toInt();
	if (rendering_level <= 0)
		return false;

	// The cache is drawn at target = exposed.translated(pan_offset).
	// The uncovered region is what's in exposed but not in the shifted target.
	QRegion uncovered = QRegion(exposed) - QRegion(exposed).translated(pan_offset);
	if (uncovered.isEmpty())
		return true;

	// Use the same composite transform as the panned cache:
	//   translate(pan_offset) * translate(w/2, h/2) * worldTransform
	auto setupTransform = [&](QPainter& p) {
		p.translate(pan_offset.x(), pan_offset.y());
		p.translate(width() / 2.0, height() / 2.0);
		p.setWorldTransform(view->worldTransform(), true);
	};

	// Compute map_rect from the inverse of the full composite transform
	QTransform composite;
	composite.translate(pan_offset.x(), pan_offset.y());
	composite.translate(width() / 2.0, height() / 2.0);
	composite = view->worldTransform() * composite;
	bool invertible = false;
	QTransform inv = composite.inverted(&invertible);
	if (!invertible)
		return false;
	QRectF map_rect = inv.mapRect(QRectF(exposed));

	double ppm = view->calculateFinalZoomFactor();

	painter.save();
	painter.setClipRegion(uncovered);

	// Below templates (with white background)
	Map* map = view->getMap();
	bool has_below_templates = !view->areAllTemplatesHidden()
	                           && isBelowTemplateVisible()
	                           && map->getFirstFrontTemplate() > 0;
	if (has_below_templates)
	{
		painter.fillRect(exposed, Qt::white);
		painter.save();
		setupTransform(painter);
		map->drawTemplates(&painter, map_rect, 0, map->getFirstFrontTemplate() - 1, view, true);
		painter.restore();
	}
	else
	{
		painter.fillRect(exposed, Qt::white);
	}

	// Map objects (only at full rendering level)
	if (rendering_level >= 2)
	{
		const auto map_visibility = view->effectiveMapVisibility();
		if (map_visibility.visible)
		{
			painter.save();
			qreal saved_opacity = painter.opacity();
			painter.setOpacity(map_visibility.opacity);

			RenderConfig::Options options(RenderConfig::Screen | RenderConfig::HelperSymbols);
			bool use_antialiasing = force_antialiasing || Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool();
			if (use_antialiasing)
				painter.setRenderHint(QPainter::Antialiasing);
			else
				options |= RenderConfig::DisableAntialiasing | RenderConfig::ForceMinSize;

			setupTransform(painter);
			RenderConfig config = { *map, map_rect, ppm, options, 1.0 };
			map->draw(&painter, config);

			painter.setOpacity(saved_opacity);
			painter.restore();
		}
	}

	// Above templates
	if (!view->areAllTemplatesHidden() && isAboveTemplateVisible()
	    && map->getNumTemplates() - map->getFirstFrontTemplate() > 0)
	{
		painter.save();
		setupTransform(painter);
		map->drawTemplates(&painter, map_rect, map->getFirstFrontTemplate(), map->getNumTemplates() - 1, view, true);
		painter.restore();
	}

	painter.restore();
	return true;
}

void MapWidget::updateTemplateCache(QImage& cache, QRect& dirty_rect, TemplateCacheViewState& state, int first_template, int last_template, bool use_background)
{
	Q_ASSERT(containsVisibleTemplate(first_template, last_template));

	if (cache.isNull())
	{
		// Lazy allocation of cache image
		cache = QImage(size(), QImage::Format_ARGB32_Premultiplied);
		dirty_rect = rect();
	}
	else
	{
		// Make sure not to use a bigger draw rect than necessary
		dirty_rect = dirty_rect.intersected(rect());
	}
		
	// Start drawing
	QPainter painter(&cache);
	painter.setClipRect(dirty_rect);
	
	// Fill with background color (TODO: make configurable)
	if (use_background)
		painter.fillRect(dirty_rect, Qt::white);
	else
	{
		QPainter::CompositionMode mode = painter.compositionMode();
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
		painter.fillRect(dirty_rect, Qt::transparent);
		painter.setCompositionMode(mode);
	}
	
	// Draw templates
	painter.translate(width() / 2.0, height() / 2.0);
	painter.setWorldTransform(view->worldTransform(), true);
	
	Map* map = view->getMap();
	// Compute the viewed rect without pan_offset, because the cache painting
	// transform (translate(width/2, height/2) * worldTransform) does not
	// include pan_offset.  Using viewportToView() would shift the clip rect,
	// causing TemplateImage::drawTemplate to clip the image to a wrong region.
	QRectF cache_view_rect(dirty_rect.left() - 0.5 * width(),
	                       dirty_rect.top() - 0.5 * height(),
	                       dirty_rect.width(),
	                       dirty_rect.height());
	QRectF map_view_rect = view->calculateViewedRect(cache_view_rect);

	map->drawTemplates(&painter, map_view_rect, first_template, last_template, view, true);

	dirty_rect.setWidth(-1); // => !dirty_rect.isValid()
	recordTemplateCacheState(state, cache);
}

void MapWidget::updateMapCache(bool use_background)
{
	if (map_cache.isNull())
	{
		// Lazy allocation of cache image
		map_cache = QImage(size(), QImage::Format_ARGB32_Premultiplied);
		map_cache_dirty_rect = rect();
	}
	else
	{
		// Make sure not to use a bigger draw rect than necessary
		map_cache_dirty_rect = map_cache_dirty_rect.intersected(rect());
	}
	
	// Start drawing
	QPainter painter;
	painter.begin(&map_cache);
	painter.setClipRect(map_cache_dirty_rect);
	
	// Fill with background color (TODO: make configurable)
	if (use_background)
	{
		painter.fillRect(map_cache_dirty_rect, Qt::white);
	}
	else
	{
		QPainter::CompositionMode mode = painter.compositionMode();
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
		painter.fillRect(map_cache_dirty_rect, Qt::transparent);
		painter.setCompositionMode(mode);
	}
	
	RenderConfig::Options options(RenderConfig::Screen | RenderConfig::HelperSymbols);
	bool use_antialiasing = force_antialiasing || Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool();
	if (use_antialiasing)
		painter.setRenderHint(QPainter::Antialiasing);
	else
		options |= RenderConfig::DisableAntialiasing | RenderConfig::ForceMinSize;
		
	Map* map = view->getMap();
	QRectF map_view_rect = view->calculateViewedRect(viewportToView(map_cache_dirty_rect));

	RenderConfig config = { *map, map_view_rect, view->calculateFinalZoomFactor(), options, 1.0 };
	
	painter.translate(width() / 2.0, height() / 2.0);
	painter.setWorldTransform(view->worldTransform(), true);
#ifndef Q_OS_ANDROID
	if (view->isOverprintingSimulationEnabled())
		map->drawOverprintingSimulation(&painter, config);
	else
#endif
		map->draw(&painter, config);
	
	if (view->isGridVisible())
		map->drawGrid(&painter, map_view_rect);
	
	// Finish drawing
	painter.end();
	
	map_cache_dirty_rect.setWidth(-1); // => !map_cache_dirty_rect.isValid()
}

void MapWidget::updateAllDirtyCaches(bool allow_deferred_template_updates)
{
	if (map_cache_dirty_rect.isValid())
		updateMapCache(false);

	if (!view->areAllTemplatesHidden())
	{
		if (below_template_cache_dirty_rect.isValid() && isBelowTemplateVisible())
		{
			if (allow_deferred_template_updates && shouldDeferTemplateCacheUpdate(below_template_cache, below_template_cache_dirty_rect, below_template_cache_state))
			{
				if (!deferred_template_cache_update_pending)
				{
					deferred_template_cache_update_pending = true;
					QTimer::singleShot(0, this, &MapWidget::updateDeferredTemplateCaches);
				}
			}
			else
			{
				updateTemplateCache(below_template_cache, below_template_cache_dirty_rect, below_template_cache_state, 0, view->getMap()->getFirstFrontTemplate() - 1, true);
			}
		}

		if (above_template_cache_dirty_rect.isValid() && isAboveTemplateVisible())
		{
			if (allow_deferred_template_updates && shouldDeferTemplateCacheUpdate(above_template_cache, above_template_cache_dirty_rect, above_template_cache_state))
			{
				if (!deferred_template_cache_update_pending)
				{
					deferred_template_cache_update_pending = true;
					QTimer::singleShot(0, this, &MapWidget::updateDeferredTemplateCaches);
				}
			}
			else
			{
				updateTemplateCache(above_template_cache, above_template_cache_dirty_rect, above_template_cache_state, view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1, false);
			}
		}
	}
}

void MapWidget::shiftCache(int sx, int sy, QImage& cache)
{
	if (!cache.isNull())
	{
		QImage new_cache(cache.size(), cache.format());
		QPainter painter(&new_cache);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(sx, sy, cache);
		painter.end();
		cache = new_cache;
	}
}

void MapWidget::shiftCache(int sx, int sy, QPixmap& cache)
{
	if (!cache.isNull())
	{
		cache.scroll(sx, sy, cache.rect());
	}
}


}  // namespace OpenOrienteering
