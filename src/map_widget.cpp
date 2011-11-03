/*
 *    Copyright 2011 Thomas Schöps
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

#include <assert.h>

#include <QtGui>
#include "map.h"
#include "map_editor.h"
#include "template.h"

MapWidget::MapWidget(QWidget* parent) : QWidget(parent)
{
	view = NULL;
	tool = NULL;
	below_template_cache = NULL;
	above_template_cache = NULL;
	drawing_dirty_rect_old = QRect();
	drawing_dirty_rect_new = QRectF();
	drawing_dirty_rect_new_border = -1;
	activity_dirty_rect_old = QRect();
	activity_dirty_rect_new = QRectF();
	activity_dirty_rect_new_border = -1;
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
}
MapWidget::~MapWidget()
{
	if (view)
		view->getMap()->removeMapWidget(this);
	
	delete below_template_cache;
	delete above_template_cache;
}

void MapWidget::setMapView(MapView* view)
{
	if (this->view != view)
	{
		if (this->view)
			this->view->getMap()->removeMapWidget(this);
		
		this->view = view;
		
		if (view)
			view->getMap()->addMapWidget(this);
		
		update();
	}
}
void MapWidget::setTool(MapEditorTool* tool)
{
	this->tool = tool;
	
	if (tool)
		setCursor(*tool->getCursor());
	else
		unsetCursor();
}
void MapWidget::setActivity(MapEditorActivity* activity)
{
	this->activity = activity;
}

QRectF MapWidget::viewportToView(const QRect& input)
{
	return QRectF(input.left() - 0.5*width(), input.top() - 0.5*height(), input.width(), input.height());
}
QPointF MapWidget::viewportToView(QPoint input)
{
	return QPointF(input.x() - 0.5*width(), input.y() - 0.5*height());
}
QRectF MapWidget::viewToViewport(const QRectF& input)
{
	return QRectF(input.left() + 0.5*width(), input.top() + 0.5*height(), input.width(), input.height());
}
QRectF MapWidget::viewToViewport(const QRect& input)
{
	return QRectF(input.left() + 0.5*width(), input.top() + 0.5*height(), input.width(), input.height());
}
QPointF MapWidget::viewToViewport(QPoint input)
{
	return QPointF(input.x() + 0.5*width(), input.y() + 0.5*height());
}
QPointF MapWidget::viewToViewport(QPointF input)
{
	return QPointF(input.x() + 0.5*width(), input.y() + 0.5*height());
}

MapCoord MapWidget::viewportToMap(QPoint input)
{
	return view->viewToMap(viewportToView(input));
}
MapCoordF MapWidget::viewportToMapF(QPoint input)
{
	return view->viewToMapF(viewportToView(input));
}
QPointF MapWidget::mapToViewport(MapCoord input)
{
	return viewToViewport(view->mapToView(input));
}
QPointF MapWidget::mapToViewport(MapCoordF input)
{
	return viewToViewport(view->mapToView(input));
}

void MapWidget::markTemplateCacheDirty(QRectF view_rect, bool front_cache)
{
	QRect* cache_dirty_rect = front_cache ? &above_template_cache_dirty_rect : &below_template_cache_dirty_rect;
	QRectF viewport_rect = viewToViewport(view_rect);
	QRect integer_rect = QRect(viewport_rect.left() - 1, viewport_rect.top() - 1, viewport_rect.width() + 2, viewport_rect.height() + 2);
	
	if (!integer_rect.intersects(rect()))
		return;
	
	if (cache_dirty_rect->isValid())
		*cache_dirty_rect = cache_dirty_rect->united(integer_rect);
	else
		*cache_dirty_rect = integer_rect;
	
	update(integer_rect);
}
void MapWidget::setDrawingBoundingBox(QRectF map_rect, int pixel_border, bool do_update)
{
	setDynamicBoundingBox(map_rect, pixel_border, drawing_dirty_rect_old, drawing_dirty_rect_new, drawing_dirty_rect_new_border, do_update);
}
void MapWidget::clearDrawingBoundingBox()
{
	clearDynamicBoundingBox(drawing_dirty_rect_old, drawing_dirty_rect_new, drawing_dirty_rect_new_border);
}
void MapWidget::setActivityBoundingBox(QRectF map_rect, int pixel_border, bool do_update)
{
	setDynamicBoundingBox(map_rect, pixel_border, activity_dirty_rect_old, activity_dirty_rect_new, activity_dirty_rect_new_border, do_update);
}
void MapWidget::clearActivityBoundingBox()
{
	clearDynamicBoundingBox(activity_dirty_rect_old, activity_dirty_rect_new, activity_dirty_rect_new_border);
}
void MapWidget::updateDrawing(QRectF map_rect, int pixel_border)
{
	QRect viewport_rect = calculateViewportBoundingBox(map_rect, pixel_border);
	
	if (viewport_rect.intersects(rect()))
		update(viewport_rect);
}
void MapWidget::setDynamicBoundingBox(QRectF map_rect, int pixel_border, QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border, bool do_update)
{
	dirty_rect_new = map_rect;
	dirty_rect_new_border = pixel_border;
	
	if (!do_update)
		return;
	
	QRect viewport_rect = calculateViewportBoundingBox(map_rect, pixel_border);
	
	if (!viewport_rect.intersects(rect()))
	{
		if (dirty_rect_old.isValid() && dirty_rect_old.intersects(rect()))
			update(dirty_rect_old);
		return;
	}
	
	if (dirty_rect_old.isValid())
		update(dirty_rect_old.united(viewport_rect));
	else
		update(viewport_rect);
}
void MapWidget::clearDynamicBoundingBox(QRect& dirty_rect_old, QRectF& dirty_rect_new, int& dirty_rect_new_border)
{
	if (!dirty_rect_new.isValid() && dirty_rect_new_border < 0)
		return;
	
	dirty_rect_new = QRectF();
	dirty_rect_new_border = -1;
	
	if (dirty_rect_old.isValid() && dirty_rect_old.intersects(rect()))
		update(dirty_rect_old);
}

QRect MapWidget::calculateViewportBoundingBox(QRectF map_rect, int pixel_border)
{
	QRectF view_rect = view->calculateViewBoundingBox(map_rect);
	QRectF viewport_rect = viewToViewport(view_rect);
	QRect integer_rect = QRect(viewport_rect.left() - (1+pixel_border), viewport_rect.top() - (1+pixel_border),
							   viewport_rect.width() + 2*(1+pixel_border), viewport_rect.height() + 2*(1+pixel_border));
	return integer_rect;
}

void MapWidget::paintEvent(QPaintEvent* event)
{
	// Draw on the widget
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	// Background color
	painter.fillRect(event->rect(), QColor(Qt::gray));	// TODO: do this more intelligently based on drag x/y
	
	// No colors defined? Provide a litte help message ... TODO: reactivate
	/*if (view && view->getMap()->getNumColors() == 0)
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- Empty map -\nStart by defining some colors:\nSelect Symbols -> Color window to\nopen the color dialog and\ndefine the colors there."));
	}
	else if (true) // TODO; No symbols defined?
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- No symbols -\nNow define some symbols:\nRight-click in the symbol bar\nand select \"New\" to create\na new symbol."));
	}
	else if (view && view->getMap()->getNumTemplates() == 0 && NoObjectsTODO)	// No templates defined?
	{
		QFont font = painter.font();
		font.setPointSize(2 * font.pointSize());
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, tr("- Ready to draw -\nStart drawing or load a base map.\nTo load a base map, click\nTemplates -> Open template..."));
	}
	else if (view)
	{*/
		// Update all dirty caches
		// TODO: It would be an idea to do these updates in a background thread and use the old caches in the meantime
		if (below_template_cache_dirty_rect.isValid())
			updateTemplateCache(below_template_cache, below_template_cache_dirty_rect, 0, view->getMap()->getFirstFrontTemplate() - 1, true);
		if (above_template_cache_dirty_rect.isValid())
			updateTemplateCache(above_template_cache, above_template_cache_dirty_rect, view->getMap()->getFirstFrontTemplate(), view->getMap()->getNumTemplates() - 1, false);
		
		// Draw caches
		if (below_template_cache && view->getMap()->getFirstFrontTemplate() > 0)
			painter.drawImage(QPoint(0, 0), *below_template_cache, rect());
			//painter.drawPixmap(QPoint(view->getCameraDragX(), view->getCameraDragY()), *below_template_cache, rect());
		
		// TODO: Map cache
		
		if (above_template_cache && view->getMap()->getNumTemplates() - view->getMap()->getFirstFrontTemplate() > 0)
			painter.drawImage(QPoint(0, 0), *above_template_cache, rect());
			//painter.drawPixmap(QPoint(view->getCameraDragX(), view->getCameraDragY()), *above_template_cache, rect());
		
		// Show current drawings
		if (activity_dirty_rect_new.isValid() || activity_dirty_rect_new_border >= 0)
		{
			QRect viewport_dirty_rect = calculateViewportBoundingBox(activity_dirty_rect_new, activity_dirty_rect_new_border);
			
			if (viewport_dirty_rect.intersects(event->rect()))
			{
				painter.setClipRect(viewport_dirty_rect.intersected(event->rect()));
				activity->draw(&painter, this);
			}
			
			activity_dirty_rect_old = viewport_dirty_rect;
		}
		
		if (drawing_dirty_rect_new.isValid() || drawing_dirty_rect_new_border >= 0)
		{
			QRect viewport_dirty_rect = calculateViewportBoundingBox(drawing_dirty_rect_new, drawing_dirty_rect_new_border);
			
			if (viewport_dirty_rect.intersects(event->rect()))
			{
				painter.setClipRect(viewport_dirty_rect.intersected(event->rect()));
				tool->draw(&painter, this);
			}
			
			drawing_dirty_rect_old = viewport_dirty_rect;
		}
	//}
	
	painter.end();
}
void MapWidget::resizeEvent(QResizeEvent* event)
{
	if (below_template_cache && below_template_cache->size() != event->size())
	{
		delete below_template_cache;
		below_template_cache = NULL;
		below_template_cache_dirty_rect = rect();
	}
	if (above_template_cache && above_template_cache->size() != event->size())
	{
		delete above_template_cache;
		above_template_cache = NULL;
		above_template_cache_dirty_rect = rect();
	}
}
void MapWidget::mousePressEvent(QMouseEvent* event)
{
	if (tool && tool->mousePressEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
    QWidget::mousePressEvent(event);
}
void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (tool && tool->mouseMoveEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
    QWidget::mouseMoveEvent(event);
}
void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (tool && tool->mouseReleaseEvent(event, view->viewToMapF(viewportToView(event->pos())), this))
	{
		event->accept();
		return;
	}
    QWidget::mouseReleaseEvent(event);
}
void MapWidget::wheelEvent(QWheelEvent* event)
{
    QWidget::wheelEvent(event);
}

void MapWidget::keyPressEvent(QKeyEvent* event)
{
	if (tool && tool->keyPressEvent(event))
		return;
    QWidget::keyPressEvent(event);
}
void MapWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (tool && tool->keyReleaseEvent(event))
		return;
    QWidget::keyReleaseEvent(event);
}

void MapWidget::updateTemplateCache(QImage*& cache, QRect& dirty_rect, int first_template, int last_template, bool use_background)
{
	if (first_template > last_template)
		return;	// no template visible
	
	if (!cache)
	{
		// Lazy allocation of cache image
		cache = new QImage(size(), QImage::Format_ARGB32_Premultiplied);
		dirty_rect = rect();
		
		//if (!use_background)
		//	cache->fill(QColor(Qt::transparent));
	}
	
	// Make sure not to use a bigger draw rect than necessary
	dirty_rect = dirty_rect.intersected(rect());
	
	// Start drawing
	QPainter painter;
	painter.begin(cache);
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
	Map* map = view->getMap();
	
	painter.translate(width() / 2.0, height() / 2.0);
	view->applyTransform(&painter);
	QRectF base_view_rect = view->calculateViewedRect(viewportToView(dirty_rect));
	
	for (int i = first_template; i <= last_template; ++i)
	{
		Template* templ = map->getTemplate(i);
		float scale = view->getZoom() * std::max(templ->getTemplateScaleX(), templ->getTemplateScaleY());
		
		QRectF view_rect;
		if (templ->getTemplateRotation() != 0)
			view_rect = QRectF(-9e42, -9e42, 9e42, 9e42);	// TODO: transform base_view_rect (map coords) using template transform to template coords
		else
		{
			view_rect.setLeft((base_view_rect.x() / templ->getTemplateScaleX()) - templ->getTemplateX());
			view_rect.setTop((base_view_rect.y() / templ->getTemplateScaleY()) - templ->getTemplateY());
			view_rect.setRight((base_view_rect.right() / templ->getTemplateScaleX()) - templ->getTemplateX());
			view_rect.setBottom((base_view_rect.bottom() / templ->getTemplateScaleY()) - templ->getTemplateY());
		}
		
		painter.save();
		templ->applyTemplateTransform(&painter);
		templ->drawTemplate(&painter, view_rect, scale);
		painter.restore();
	}
	
	painter.end();
	
	dirty_rect.setWidth(-1);
	assert(!dirty_rect.isValid());
}

#include "map_widget.moc"
