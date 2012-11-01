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

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map_widget.h"
#include "settings.h"
#include "util.h"

QStringList TemplateMap::locked_maps;

TemplateMap::TemplateMap(const QString& path, Map* map) : Template(path, map), template_map(NULL)
{
}
TemplateMap::~TemplateMap()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}

bool TemplateMap::loadTemplateFileImpl(bool configuring)
{
	// Prevent unbounded recursive template loading
	if (locked_maps.contains(template_path))
		return true;
	
	locked_maps.append(template_path);
	Map* new_template_map = new Map();
	bool new_template_valid = new_template_map->loadFrom(template_path, NULL, false, configuring);
	locked_maps.removeAll(template_path);
	
	if (!new_template_valid)
	{
		delete new_template_map;
		return false;
	}
	
	// Remove all template's templates from memory
	// TODO: prevent loading and/or let user decide
	for (int i = new_template_map->getNumTemplates()-1; i >= 0; i--)
	{
		new_template_map->deleteTemplate(i);
	}
	
	template_map = new_template_map;
	return true;
}

bool TemplateMap::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	// TODO: recursive template loading dialog
	
	// TODO: it would be possible to load maps as georeferenced if both maps are georeferenced
	is_georeferenced = false;
	out_center_in_view = false;
	
	return true;
}

void TemplateMap::unloadTemplateFileImpl()
{
	delete template_map;
	template_map = NULL;
}

void TemplateMap::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	
	if (Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool())
		painter->setRenderHint(QPainter::Antialiasing);
	
	QRectF transformed_clip_rect;
	if (!is_georeferenced)
	{
		rectIncludeSafe(transformed_clip_rect, mapToTemplateQPoint(MapCoordF(clip_rect.topLeft())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplateQPoint(MapCoordF(clip_rect.topRight())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplateQPoint(MapCoordF(clip_rect.bottomLeft())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplateQPoint(MapCoordF(clip_rect.bottomRight())));
	}
	else
	{
		// TODO!
	}
	
	template_map->draw(painter, transformed_clip_rect, false, scale, false, opacity);
}

QRectF TemplateMap::getTemplateExtent()
{
    // If the template is invalid, the extent is an empty rectangle.
    if (!template_map) return QRectF();
	return template_map->calculateExtent(false, false, NULL);
}

Template* TemplateMap::duplicateImpl()
{
	TemplateMap* copy = new TemplateMap(template_path, map);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl(false);
	return copy;
}
