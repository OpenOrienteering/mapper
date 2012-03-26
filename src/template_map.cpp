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


#include "template_map.h"

#include <vector>
#include <algorithm>

#include <QtGui>
#include <QDebug>

#include "map_widget.h"

//#include "util.h"

QStringList TemplateMap::locked_maps;

TemplateMap::TemplateMap(const QString& filename, Map* map) : Template(filename, map), template_map(NULL)
{
	template_valid = changeTemplateFileImpl(filename);
}

TemplateMap::TemplateMap(const TemplateMap& other) : Template(other), template_map(NULL)
{
	QString filename = other.getTemplatePath();
	template_valid = changeTemplateFileImpl(filename);
}

TemplateMap::~TemplateMap()
{
	delete template_map;
}

Template* TemplateMap::duplicate()
{
	TemplateMap* copy = new TemplateMap(*this);
	return copy;
}

bool TemplateMap::open(QWidget* dialog_parent, MapView* main_view)
{
	// TODO: recursive template loading dialog
	
	return true;
}

void TemplateMap::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity) {}

void TemplateMap::drawTemplateUntransformed(QPainter* painter, const QRect& clip_rect, MapWidget* widget)
{
	if (widget == NULL)
		return;
	
	painter->save();
	painter->translate(widget->width() / 2.0, widget->height() / 2.0);
	widget->getMapView()->applyTransform(painter);
	bool use_antialiasing = widget->usesAntialiasing();
	if (use_antialiasing)
		painter->setRenderHint(QPainter::Antialiasing);
	template_map->draw(painter, clip_rect, !use_antialiasing, widget->getMapView()->calculateFinalZoomFactor(), false, widget->getMapView()->getTemplateVisibility(this)->opacity);
	painter->restore();
}

QRectF TemplateMap::getExtent()
{
    // If the image is invalid, the extent is an empty rectangle.
    if (!template_map) return QRectF();
	QRectF raw = template_map->getLayer(0)->calculateExtent();
	return QRectF(mapToTemplateQPoint(MapCoordF(raw.topLeft())), mapToTemplateQPoint(MapCoordF(raw.bottomRight())));
	return template_map->getLayer(0)->calculateExtent();
	
}

bool TemplateMap::changeTemplateFileImpl(const QString& filename)
{
	// prevent unbounded recursive template loading
	if (locked_maps.contains(filename))
		return true;
	
	locked_maps.append(filename);
	Map* new_template_map = new Map();
	bool new_template_valid = new_template_map->loadFrom(filename);
	locked_maps.removeAll(filename);
	
	if (! new_template_valid)
	{
		delete new_template_map;
		return false;
	}
	
	// remove all template's templates from memory
	for (int i=new_template_map->getNumTemplates()-1; i >= 0; i--)
	{
		new_template_map->deleteTemplate(i);
	}
	
	if (template_map)
		delete template_map;
	
	template_map = new_template_map;
	return true;
}

#include "template_map.moc"
