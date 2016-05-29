/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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

#include <QPainter>

#include "map_widget.h"
#include "renderable.h"
#include "settings.h"
#include "util.h"

QStringList TemplateMap::locked_maps;

const std::vector<QByteArray>& TemplateMap::supportedExtensions()
{
	static std::vector<QByteArray> extensions = { "ocd", "omap", "xmap" };
	return extensions;
}

TemplateMap::TemplateMap(const QString& path, Map* map)
: Template(path, map)
{
	// nothing
}

TemplateMap::~TemplateMap()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}

const char* TemplateMap::getTemplateType() const
{
	return "TemplateMap";
}

bool TemplateMap::isRasterGraphics() const
{
	return false;
}

bool TemplateMap::loadTemplateFileImpl(bool configuring)
{
	// Prevent unbounded recursive template loading
	if (locked_maps.contains(template_path))
		return true;
	
	locked_maps.append(template_path);
	std::unique_ptr<Map> new_template_map{ new Map() };
	bool new_template_valid = new_template_map->loadFrom(template_path, nullptr, nullptr, false, configuring);
	locked_maps.removeAll(template_path);
	
	if (new_template_valid)
	{
		// Remove all template's templates from memory
		// TODO: prevent loading and/or let user decide
		for (int i = new_template_map->getNumTemplates()-1; i >= 0; i--)
		{
			new_template_map->deleteTemplate(i);
		}
		
		template_map = std::move(new_template_map);
	}
	
	return new_template_valid;
}

bool TemplateMap::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(dialog_parent);
	
	// TODO: recursive template loading dialog
	
	// TODO: it would be possible to load maps as georeferenced if both maps are georeferenced
	is_georeferenced = false;
	out_center_in_view = false;
	
	return true;
}

void TemplateMap::unloadTemplateFileImpl()
{
	template_map.reset();
}

void TemplateMap::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, bool on_screen, float opacity) const
{
	if (!is_georeferenced)
		applyTemplateTransform(painter);
	
	if (Settings::getInstance().getSettingCached(Settings::MapDisplay_Antialiasing).toBool())
		painter->setRenderHint(QPainter::Antialiasing);
	
	QRectF transformed_clip_rect;
	if (!is_georeferenced)
	{
		rectIncludeSafe(transformed_clip_rect, mapToTemplate(MapCoordF(clip_rect.topLeft())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplate(MapCoordF(clip_rect.topRight())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplate(MapCoordF(clip_rect.bottomLeft())));
		rectIncludeSafe(transformed_clip_rect, mapToTemplate(MapCoordF(clip_rect.bottomRight())));
	}
	else
	{
		transformed_clip_rect = clip_rect;
	}
	
	RenderConfig::Options options;
	if (on_screen)
		options |= RenderConfig::Screen;
	RenderConfig config = { *(template_map.get()), transformed_clip_rect, scale, options, opacity };
	// TODO: introduce template-specific options, adjustable by the user, to allow changing some of these parameters
	template_map->draw(painter, config);
}

QRectF TemplateMap::getTemplateExtent() const
{
	// If the template is invalid, the extent is an empty rectangle.
	QRectF extent;
	if (template_map)
		extent = template_map->calculateExtent(false, false, nullptr);
	return extent;
}

Template* TemplateMap::duplicateImpl() const
{
	TemplateMap* copy = new TemplateMap(template_path, map);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl(false);
	return copy;
}

const Map* TemplateMap::templateMap() const
{
	return template_map.get();
}

Map* TemplateMap::templateMap()
{
	return template_map.get();
}

void TemplateMap::setTemplateMap(std::unique_ptr<Map>&& map)
{
	template_map = std::move(map);
}
