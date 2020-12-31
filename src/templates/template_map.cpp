/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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

#include <utility>

#include <QtGlobal>
#include <QByteArray>
#include <QPaintDevice>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <QTimer>
#include <QTransform>
#include <QVariant>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/renderables/renderable.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"
#include "gui/util_gui.h"
#include "util/transformation.h"
#include "util/util.h"


#ifdef __clang_analyzer__
#define singleShot(A, B, C) singleShot(A, B, #C) // NOLINT
#endif


namespace OpenOrienteering {

namespace {

/** 
 * Transform a template map to match the given map's georeferencing, using
 * an already determined transformation.
 */
void transformMap(Map& template_map, Map const& map, TemplateTransform const& transform)
{
	template_map.applyOnAllObjects(transform.makeObjectTransform());
	template_map.setGeoreferencing(map.getGeoreferencing());
	
	auto template_scale = (transform.template_scale_x + transform.template_scale_y) / 2;
	template_scale *= double(template_map.getScaleDenominator()) / map.getScaleDenominator();
	if (!qFuzzyCompare(template_scale, 1))
		template_map.scaleAllSymbols(template_scale);
}

}



QStringList TemplateMap::locked_maps;

const std::vector<QByteArray>& TemplateMap::supportedExtensions()
{
	static std::vector<QByteArray> extensions = { "ocd", "omap", "xmap" };
	return extensions;
}

TemplateMap::TemplateMap(const QString& path, Map* map)
: Template(path, map)
{
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateMap::mapProjectionChanged);
	// For connecting to virtual methods using PMF, we need to use a lambda.
	connect(&georef, &Georeferencing::transformationChanged, this, [this]() { mapTransformationChanged(); });
}

TemplateMap::TemplateMap(const TemplateMap& proto)
: Template(proto)
{
	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, &Georeferencing::projectionChanged, this, &TemplateMap::mapProjectionChanged);
	// For connecting to virtual methods using PMF, we need to use a lambda.
	connect(&georef, &Georeferencing::transformationChanged, this, [this]() { mapTransformationChanged(); });
}

TemplateMap::~TemplateMap()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}

TemplateMap* TemplateMap::duplicate() const
{
	auto* copy = new TemplateMap(*this);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl();
	return copy;
}


const char* TemplateMap::getTemplateType() const
{
	return "TemplateMap";
}

bool TemplateMap::isRasterGraphics() const
{
	return false;
}


bool TemplateMap::preLoadSetup(QWidget* /* dialogParent */)
{
	// Use the template map's georeferencing to calculate
	// a template transformation for non-georeferenced mode,
	// but don't stay in georeferenced mode after setup.
	is_georeferenced = true;
	block_georeferencing = true;
	return true;
}

bool TemplateMap::loadTemplateFileImpl()
{
	// Prevent unbounded recursive template loading
	if (locked_maps.contains(template_path))
		return true;
	
	auto new_template_map = std::make_unique<Map>(); 
	auto importer = FileFormats.makeImporter(template_path, *new_template_map, nullptr);
	locked_maps.append(template_path);  /// \todo Convert to RAII
	auto new_template_valid = importer && importer->doImport();
	locked_maps.removeAll(template_path);
	
	if (new_template_valid)
	{
		template_map = std::move(new_template_map);
		
		if (property(ocdTransformProperty()).toBool())
		{
			// Update the transformation without signalling dirty state.
			is_georeferenced = false;
			transform = transformForOcd();
			updateTransformationMatrices();
			setProperty(ocdTransformProperty(), false);
		}
		else if (is_georeferenced)
		{
			QTransform q_transform;
			if (!calculateTransformation(q_transform))
			{
				setErrorString(tr("Failed to transform the coordinates."));
				return false;
			}
			
			transform = TemplateTransform::fromQTransform(q_transform);
			if (georeferencedStateSupported())
			{
				transformMap(*template_map, *map, transform);
				transform = {};
			}
			else
			{
				// Owning map or template not georeferenced, or georeferencing
				// blocked, so we must change the state now, using the
				// calculated transformation.
				is_georeferenced = false;
			}
			updateTransformationMatrices();
			block_georeferencing = false;
		}
		else
		{
			block_georeferencing = false;
		}
	}
	else if (importer)
	{
		setErrorString(importer->warnings().back());
	}
	else
	{
		setErrorString(tr("Cannot load map file, aborting."));
	}
	
	return new_template_valid;
}

bool TemplateMap::postLoadSetup(QWidget* /* dialog_parent */, bool& out_center_in_view)
{
	auto const is_unconfigured = [](auto const& georef) {
		return georef.getState() != Georeferencing::Geospatial && georef.toProjectedCoords(MapCoordF{}) == QPointF{};
	};
	out_center_in_view = is_unconfigured(templateMap()->getGeoreferencing())
	                     || is_unconfigured(map->getGeoreferencing());
	return true;
}

void TemplateMap::unloadTemplateFileImpl()
{
	template_map.reset();
}

void TemplateMap::drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, qreal opacity) const
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


bool TemplateMap::hasAlpha() const
{
	return template_map && template_map->hasAlpha();
}


bool TemplateMap::canChangeTemplateGeoreferenced() const
{
	return getTemplateState() == Template::Loaded
	       && georeferencedStateSupported();
}

bool TemplateMap::trySetTemplateGeoreferenced(bool value, QWidget* /*dialog_parent*/)
{
	if (canChangeTemplateGeoreferenced()
	    && isTemplateGeoreferenced() != value)
	{
		// Loaded state is implied by TemplateMap::canChangeTemplateGeoreferenced().
		Q_ASSERT(getTemplateState() == Template::Loaded);
		
		setTemplateAreaDirty();
		if (value == false)
		{
			// The currently loaded map data is transformed. Reload pristine
			// data, and calculate the positioning as if loaded georeferenced.
			block_georeferencing = true;
			reload();
		}
		else
		{
			// The currently loaded map data is pristine. Immediately perform
			// the same transformation as if loaded regularly.
			QTransform q_transform;
			if (!calculateTransformation(q_transform))
			{
				setErrorString(tr("Failed to transform the coordinates."));
				return false;
			}
			
			is_georeferenced = true;
			transformMap(*template_map, *map, TemplateTransform::fromQTransform(q_transform));
			transform = {};
			updateTransformationMatrices();
			setTemplateAreaDirty();
		}
		map->emitTemplateChanged(this);
	}
	return isTemplateGeoreferenced() == value;
}


const Map* TemplateMap::templateMap() const
{
	return template_map.get();
}

Map* TemplateMap::templateMap()
{
	return template_map.get();
}

std::unique_ptr<Map> TemplateMap::takeTemplateMap()
{
	std::unique_ptr<Map> result;
	if (template_state == Loaded)
	{
		swap(result, template_map);
		setTemplateState(Unloaded);
		emit templateStateChanged();
	}
	return result;
}

void TemplateMap::setTemplateMap(std::unique_ptr<Map>&& map)
{
	template_map = std::move(map);
}


void TemplateMap::mapProjectionChanged()
{
	if (is_georeferenced && template_state == Template::Loaded)
		reloadLater();
}

void TemplateMap::mapTransformationChanged()
{
	if (is_georeferenced)
	{
		if (template_state != Template::Loaded)
			return;
		
		auto const& templ_georef = templateMap()->getGeoreferencing();
		auto const& map_georef = map->getGeoreferencing();
		if (templateMap()->getScaleDenominator() == map->getScaleDenominator()
		    && templ_georef.getProjectedCRSSpec() == map_georef.getProjectedCRSSpec())
		{
			auto const t = templ_georef.mapToProjected() * map_georef.projectedToMap();
			templateMap()->applyOnAllObjects([&t](Object* o) { o->transform(t); });
			templateMap()->setGeoreferencing(map_georef);
		}
		else
		{
			// If the projected CRS changed: The necessary transformation
			//   involves at least two CRS and might give imprecise results.
			// If the scale changed: We can't know how to correctly scale
			//   symbol dimensions.
			// Reloading is a slow but safe way to get the same results as they
			// will appear when opening the file the next time.
			reloadLater();
		}
	}
}


void TemplateMap::reloadLater()
{
	if (reload_pending)
		return;
	if (template_state == Loaded)
		templateMap()->clear(); // no expensive operations before reloading
	QTimer::singleShot(0, this, &TemplateMap::reload);
	reload_pending = true;
}

// slot
void TemplateMap::reload()
{
	if (template_state == Loaded)
		unloadTemplateFile();
	loadTemplateFile();
	reload_pending = false;
}


bool TemplateMap::calculateTransformation(QTransform& q_transform) const
{
	if (!template_map)
		return false;
	
	const auto& georef = template_map->getGeoreferencing();
	const auto src_origin = MapCoordF { georef.getMapRefPoint() };
	
	bool ok0, ok1, ok2;
	PassPointList passpoints;
	passpoints.resize(3);
	passpoints[0].src_coords  = src_origin;
	passpoints[0].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[0].src_coords, &ok0);
	passpoints[1].src_coords  = src_origin + MapCoordF { 128.0, 0.0 }; // 128 mm off horizontally
	passpoints[1].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[1].src_coords, &ok1);
	passpoints[2].src_coords  = src_origin + MapCoordF { 0.0, 128.0 }; // 128 mm off vertically
	passpoints[2].dest_coords = map->getGeoreferencing().toMapCoordF(&georef, passpoints[2].src_coords, &ok2);
	return ok0 && ok1 && ok2 && passpoints.estimateNonIsometricSimilarityTransform(&q_transform);
}


TemplateTransform TemplateMap::transformForOcd() const
{
	auto t = transform;
	if (templateMap())
	{
		auto const template_origin = templateMap()->getGeoreferencing().toProjectedCoords(MapCoordF{});
		auto const offset = getMap()->getGeoreferencing().toMapCoords(template_origin);
		t = {offset.nativeX(), offset.nativeY()};
	}
	return t;
}

const char* TemplateMap::ocdTransformProperty()
{
	static const char* name = "TemplateMap::transformForOcd";
	return name;
}


bool TemplateMap::georeferencedStateSupported() const
{
	auto const test_transform = [](auto const& src, auto const& dest) {
		// Cf. calculateTransformation()
		bool success = false;
		void(dest.toMapCoordF(&src, MapCoordF{src.getMapRefPoint()}, &success));
		return success;
	};
	return !block_georeferencing
	       && map->getGeoreferencing().getState() == Georeferencing::Geospatial
	       && templateMap()
	       && templateMap()->getGeoreferencing().getState() == Georeferencing::Geospatial
	       && test_transform(templateMap()->getGeoreferencing(), map->getGeoreferencing());
}


}   // namespace OpenOrienteering
