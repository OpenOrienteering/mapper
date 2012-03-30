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

#include <QtGui>

#include "map_widget.h"

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

void TemplateMap::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	painter->setRenderHint(QPainter::Antialiasing); // TODO: Make antialiasing configurable
	template_map->draw(painter, clip_rect, false, scale, false, opacity);
}

QRectF TemplateMap::getExtent()
{
    // If the template is invalid, the extent is an empty rectangle.
    if (!template_map) return QRectF();
	return template_map->calculateExtent(false, false, NULL);
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
	// TODO: prevent loading and/or let user decide
	for (int i=new_template_map->getNumTemplates()-1; i >= 0; i--)
	{
		new_template_map->deleteTemplate(i);
	}
	
	delete template_map;
	
	template_map = new_template_map;
	return true;
}

#include "template_map.moc"
