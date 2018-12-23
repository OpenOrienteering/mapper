/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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

#include <algorithm>

#include <QtGlobal>
#include <QByteArray>
#include <QPaintEngine>
#include <QPainter>
#include <QRectF>
#include <QStringList>
#include <QTransform>
#include <QVariant>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/renderables/renderable.h"
#include "gui/util_gui.h"
#include "util/transformation.h"
#include "util/util.h"


namespace OpenOrienteering {

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

bool TemplateMap::postLoadConfiguration(QWidget* /* dialog_parent */, bool& out_center_in_view)
{
	// Instead of dealing with the map as being (possibly) georeferenced,
	// we simply use the both georeferencings to calculate a transformation
	// between the coordinate systems.
	is_georeferenced = false;
	out_center_in_view = false;
	calculateTransformation();
	
	/// \todo recursive template loading dialog
	
	return true;
}

void TemplateMap::unloadTemplateFileImpl()
{
	template_map.reset();
}

void TemplateMap::drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const
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
	auto scaling = scale;
	if (on_screen)
	{
		options |= RenderConfig::Screen;
		/// \todo Get the actual screen's resolution.
		scaling = Util::mmToPixelPhysical(scale);
	}
	else
	{
		auto dpi = painter->device()->physicalDpiX();
		if (!dpi)
			dpi = painter->device()->logicalDpiX();
		if (dpi > 0)
			scaling *= dpi / 25.4;
	}
	RenderConfig config = { *template_map, transformed_clip_rect, scaling, options, qreal(opacity) };
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
	auto copy = new TemplateMap(template_path, map);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl(false);
	return copy;
}


bool TemplateMap::hasAlpha() const
{
	return template_map && template_map->hasAlpha();
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

void TemplateMap::calculateTransformation()
{
	const auto& georef = template_map->getGeoreferencing();
	const auto src_origin = MapCoordF { georef.getMapRefPoint() };
	
	bool ok0, ok1, ok2;
	QTransform q_transform;
	PassPointList passpoints;
	passpoints.resize(3);
	passpoints[0].src_coords  = src_origin;
	passpoints[0].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[0].src_coords, &ok0);
	passpoints[1].src_coords  = src_origin + MapCoordF { 128.0, 0.0 }; // 128 mm off horizontally
	passpoints[1].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[1].src_coords, &ok1);
	passpoints[2].src_coords  = src_origin + MapCoordF { 0.0, 128.0 }; // 128 mm off vertically
	passpoints[2].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[2].src_coords, &ok2);
	if (ok0 && ok1 && ok2
	    && passpoints.estimateNonIsometricSimilarityTransform(&q_transform))
	{
		transform = TemplateTransform::fromQTransform(q_transform);
		updateTransformationMatrices();
	}
	else
	{
		qDebug("updateTransform() failed");
		/// \todo proper error message
	}
}


}   // namespace OpenOrienteering
