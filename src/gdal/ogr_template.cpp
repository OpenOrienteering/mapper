/*
 *    Copyright 2016-2020 Kai Pastor
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

#include "ogr_template.h"

#include <memory>
#include <utility>

#include <QtGlobal>
#include <QDialog>
#include <QLatin1String>
#include <QPoint>
#include <QPointF>
#include <QStringRef>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/track.h"
#include "fileformats/file_format.h"
#include "gdal/gdal_manager.h"
#include "gdal/ogr_file_format_p.h"
#include "gui/georeferencing_dialog.h"
#include "templates/template.h"
#include "templates/template_positioning_dialog.h"
#include "templates/template_track.h"


namespace OpenOrienteering {

namespace {
	
	namespace literal
	{
		const auto crs_spec           = QLatin1String("crs_spec");
		const auto georeferencing     = QLatin1String("georeferencing");
		const auto projected_crs_spec = QLatin1String("projected_crs_spec");
		const auto srs                = QLatin1String("srs");
		const auto map                = QLatin1String("map");
	}
	
	
	bool preserveRefPoints(Georeferencing& data_georef, const Georeferencing& initial_georef)
	{
		// Keep a configured local reference point from initial_georef?
		auto data_crs_spec = data_georef.getProjectedCRSSpec();
		if (initial_georef.getState() != Georeferencing::Geospatial
		    && initial_georef.getProjectedRefPoint() != QPointF{}
		    && data_georef.getState() == Georeferencing::Geospatial
		    && data_georef.getProjectedRefPoint() == QPointF{}
		    && data_georef.getMapRefPoint() == MapCoord{}
		    && data_crs_spec.contains(QLatin1String("+proj=ortho"))
		    && !data_crs_spec.contains(QLatin1String("+x_0="))
		    && !data_crs_spec.contains(QLatin1String("+y_0=")) )
		{
			data_crs_spec.append(QString::fromLatin1(" +x_0=%1 +y_0=%2")
			                     .arg(initial_georef.getProjectedRefPoint().x(), 0, 'f', 2)
			                     .arg(initial_georef.getProjectedRefPoint().y(), 0, 'f', 2) );
			data_georef.setProjectedCRS({}, data_crs_spec);
			data_georef.setProjectedRefPoint(initial_georef.getProjectedRefPoint());
			data_georef.setMapRefPoint(initial_georef.getMapRefPoint());
			return true;
		}
		return false;
	}
	
}  // namespace



// static
bool OgrTemplate::canRead(const QString& path)
{
	return OgrFileImport::canRead(path);
}


// static
const std::vector<QByteArray>& OgrTemplate::supportedExtensions()
{
	return GdalManager().supportedVectorImportExtensions();
}


std::unique_ptr<Georeferencing> OgrTemplate::getDataGeoreferencing(const QString& path, const Georeferencing& initial_georef)
{
	Map tmp_map;
	tmp_map.setGeoreferencing(initial_georef);
	OgrFileImport importer{ path, &tmp_map, nullptr, CoordinateSystem::DomainGround};
	importer.setGeoreferencingImportEnabled(true);
	importer.setLoadSymbolsOnly(true);
	if (!importer.doImport())
		return {};  // failure
	
	return std::make_unique<Georeferencing>(tmp_map.getGeoreferencing());  // success
}


OgrTemplate::OgrTemplate(const QString& path, Map* map)
: TemplateMap(path, map)
{
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &OgrTemplate::applySettings);
}

OgrTemplate::OgrTemplate(const OgrTemplate& proto)
: TemplateMap(proto)
, track_crs_spec(proto.track_crs_spec)
, projected_crs_spec(proto.projected_crs_spec)
, cs_domain(proto.cs_domain)
, template_track_compatibility(proto.template_track_compatibility)
, center_in_view(proto.center_in_view)
{
	if (proto.explicit_georef)
		explicit_georef = std::make_unique<Georeferencing>(*proto.explicit_georef);
	if (proto.map_configuration_georef)
		map_configuration_georef = std::make_unique<Georeferencing>(*proto.map_configuration_georef);
	
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &OgrTemplate::applySettings);
}

OgrTemplate::~OgrTemplate()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}


OgrTemplate* OgrTemplate::duplicate() const
{
	auto* copy = new OgrTemplate(*this);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl();
	return copy;
}


const char* OgrTemplate::getTemplateType() const
{
	return "OgrTemplate";
}


std::unique_ptr<Georeferencing> OgrTemplate::makeOrthographicGeoreferencing(const LatLon& center) const
{
	auto georef = std::make_unique<Georeferencing>();
	georef->setScaleDenominator(int(map->getGeoreferencing().getScaleDenominator()));
	georef->setProjectedCRS(QString{},
	                        QStringLiteral("+proj=ortho +datum=WGS84 +ellps=WGS84 +units=m +lat_0=%1 +lon_0=%2 +no_defs")
	                        .arg(QString::number(center.latitude(), 'f'), QString::number(center.longitude(), 'f')));
	georef->setProjectedRefPoint({}, false);
	return georef;
}

std::unique_ptr<Georeferencing> OgrTemplate::makeOrthographicGeoreferencing(const QString& path) const
{
	/// \todo Use the template's datum etc. instead of WGS84?
	auto const has_suitable_georeferencing = [this](const QString& path) -> bool {
		auto const georef = makeOrthographicGeoreferencing(LatLon{});
		return OgrFileImport::checkGeoreferencing(path, *georef);
	} (path);
	return has_suitable_georeferencing ? makeOrthographicGeoreferencing(OgrFileImport::calcAverageLatLon(path)) : nullptr;
}

std::unique_ptr<Georeferencing> OgrTemplate::makeOrthographicGeoreferencing() const
{
	// For a transition period, copy behaviour from
	// TemplateTrack::loadTemplateFileImpl(),
	// TemplateTrack::calculateLocalGeoreferencing().
	// This is an extra expense (by loading the data twice), but
	// only until the map (projected_crs_spec) is saved once.
	// If the TemplateTrack approach failed, use local approach.
	TemplateTrack track{template_path, map};
	if (track.track.loadFrom(template_path, false))
		return makeGeoreferencing(track.calculateLocalGeoreferencing());
	else
		return makeOrthographicGeoreferencing(template_path);
}

std::unique_ptr<Georeferencing> OgrTemplate::makeGeoreferencing(const QString& spec) const
{
	auto georef = std::make_unique<Georeferencing>();
	georef->setScaleDenominator(int(map->getScaleDenominator()));
	georef->setProjectedCRS(QString{}, spec);
	georef->setProjectedRefPoint({}, false, false);
	georef->setCombinedScaleFactor(1.0);
	georef->setGrivation(0.0);
	return georef;
}


bool OgrTemplate::preLoadSetup(QWidget* dialog_parent)
try
{
	template_track_compatibility = TemplateTrack::supports(template_path);
	is_georeferenced = false;
	explicit_georef.reset();
	map_configuration_georef.reset();
	track_crs_spec.clear();
	projected_crs_spec.clear();
	transform = {};
	updateTransformationMatrices();
	
	auto data_georef = std::unique_ptr<Georeferencing>();
	if (map->getGeoreferencing().getState() != Georeferencing::Geospatial)
	{
		data_georef = getDataGeoreferencing(template_path, map->getGeoreferencing());
		if (data_georef && data_georef->getState() == Georeferencing::Geospatial)
		{
			if (template_track_compatibility)
				data_georef->setGrivation(0);
		}
	}
	
	TemplatePositioningDialog dialog(getTemplateFilename(), *data_georef, dialog_parent);
	dialog.setWindowModality(Qt::WindowModal);
	do
	{
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
		cs_domain = dialog.csDomain();
		if (cs_domain != CoordinateSystem::DomainGeospatial)
			break;
		
		if (map->getGeoreferencing().getState() != Georeferencing::Geospatial)
		{
			// Make sure that the map is georeferenced.
			Georeferencing initial_georef(map->getGeoreferencing());
			auto keep_projected = false;
			if (data_georef && data_georef->getState() == Georeferencing::Geospatial)
			{
				initial_georef.setProjectedCRS(data_georef->getProjectedCRSId(), data_georef->getProjectedCRSSpec());
				if (!template_track_compatibility)
					keep_projected = preserveRefPoints(initial_georef, map->getGeoreferencing());
			}
			
			GeoreferencingDialog dialog(dialog_parent, map, &initial_georef);
			if (keep_projected)
				dialog.setKeepProjectedRefCoords();
			else
				dialog.setKeepGeographicRefCoords();
			dialog.exec();
		}
	}
	while (cs_domain == CoordinateSystem::DomainGeospatial
	       && map->getGeoreferencing().getState() != Georeferencing::Geospatial);
	
	switch (cs_domain)
	{
	case CoordinateSystem::DomainMap:
	case CoordinateSystem::DomainGround:
		// Is the template's SRS orthographic, or can it be converted?
		if (data_georef && !data_georef->getProjectedCRSSpec().contains(QLatin1String("+proj=ortho")))
		{
			data_georef.reset();
		}
		if (!data_georef)
		{
			data_georef = makeOrthographicGeoreferencing(template_path);
		}
		if (data_georef)
		{
			if (template_track_compatibility)
				data_georef->setGrivation(0);
			else
				preserveRefPoints(*data_georef, map->getGeoreferencing());
			explicit_georef = std::move(data_georef);
			// Data is to be transformed to the projected CRS.
			track_crs_spec = Georeferencing::geographic_crs_spec;
			projected_crs_spec = explicit_georef->getProjectedCRSSpec();
		}
		break;
		
	case CoordinateSystem::DomainGeospatial:
		is_georeferenced = true;
		data_crs_spec = dialog.currentCRSSpec();
		break;
	}
	
	center_in_view  = dialog.centerOnView();
	transform.template_scale_x = transform.template_scale_y = dialog.unitScaleFactor();
	updateTransformationMatrices();
	
	return true;
}
catch (FileFormatException& e)
{
	setErrorString(e.message());
	return false;
}


bool OgrTemplate::loadTemplateFileImpl()
try
{
	if (explicit_georef_pending)
	{
		// Need to create an orthographic projection during data loading.
		explicit_georef = makeOrthographicGeoreferencing();
		projected_crs_spec = explicit_georef->getProjectedCRSSpec();
		explicit_georef_pending = false;
	}
	
	auto new_template_map = std::make_unique<Map>();
	OgrFileImport importer{template_path, new_template_map.get(), nullptr, cs_domain};
	
	// Configure generation of renderables.
	updateView(*new_template_map);
	
	if (is_georeferenced || !explicit_georef)
	{
		new_template_map->setGeoreferencing(map->getGeoreferencing());
	}
	else
	{
		new_template_map->setGeoreferencing(*explicit_georef);
	}
	
	const auto pp0 = new_template_map->getGeoreferencing().getProjectedRefPoint();
	importer.setGeoreferencingImportEnabled(false);
	importer.setOverrideCrs(data_crs_spec);
	if (!importer.doImport())
	{
		setErrorString(importer.warnings().back());
		return false;
	}
	
	// MapCoord bounds handling may have moved the paper position of the
	// template data during import. The template position might need to be
	// adjusted accordingly.
	// However, this will happen again the next time the template is loaded.
	// So this adjustment must not affect the saved configuration.
	const auto pm0 = new_template_map->getGeoreferencing().toMapCoords(pp0);
	const auto pm1 = new_template_map->getGeoreferencing().getMapRefPoint();
	setTemplatePositionOffset(pm1 - pm0);
	
	setTemplateMap(std::move(new_template_map));
	
	const auto& warnings = importer.warnings();
	if (!warnings.empty())
	{
		QString message;
		message.reserve((warnings.back().length()+1) * int(warnings.size()));
		for (const auto& warning : warnings)
		{
			message.append(warning);
			message.append(QLatin1String{"\n"});
		}
		message.chop(1);
		setErrorString(message);
	}
	
	return true;
}
catch (FileFormatException& e)
{
	setErrorString(e.message());
	return false;
}


bool OgrTemplate::postLoadSetup(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(dialog_parent)
	out_center_in_view = center_in_view;
	return true;
}


bool OgrTemplate::canChangeTemplateGeoreferenced() const
{
	return false;
}



void OgrTemplate::mapTransformationChanged()
{
	if (is_georeferenced)
	{
		TemplateMap::mapTransformationChanged();
	}
	else if (explicit_georef)
	{
		template_track_compatibility = false;
	}
	else if (template_state == Template::Loaded)
	{
		template_track_compatibility = false;
		explicit_georef = std::make_unique<Georeferencing>(templateMap()->getGeoreferencing());
		resetTemplatePositionOffset();
	}
	else if (map_configuration_georef)
	{
		template_track_compatibility = false;
		explicit_georef = std::move(map_configuration_georef);
		resetTemplatePositionOffset();
	}
}



void OgrTemplate::applySettings()
{
	if (auto* template_map = templateMap())
		updateView(*template_map);
}

void OgrTemplate::updateView(Map& template_map)
{
	GdalManager manager;
	const auto enable_hatching = manager.isAreaHatchingEnabled();
	const auto enable_baseline = manager.isBaselineViewEnabled();
	
	bool dirty = false;
	if (template_map.isAreaHatchingEnabled() != enable_hatching)
	{
		template_map.setAreaHatchingEnabled(enable_hatching);
		dirty = true;
	}
	if (template_map.isBaselineViewEnabled() != enable_baseline)
	{
		template_map.setBaselineViewEnabled(enable_baseline);
		dirty = true;
	}
	
	if (dirty && templateMap() == &template_map)
	{
		template_map.updateAllObjects();
		setTemplateAreaDirty();
	}
}



bool OgrTemplate::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (xml.name() == literal::georeferencing)
	{
		explicit_georef = std::make_unique<Georeferencing>();
		explicit_georef->load(xml, false);
	}
	else if (xml.name() == literal::crs_spec)
	{
		track_crs_spec = xml.readElementText();
		template_track_compatibility = true;
	}
	else if (xml.name() == literal::projected_crs_spec)
	{
		projected_crs_spec = xml.readElementText();
		template_track_compatibility = true;
	}
	else if (xml.name() == literal::srs)
	{
		auto srs = xml.readElementText();
		if (srs == literal::map)
			cs_domain = CoordinateSystem::DomainMap;
	}
	else
	{
		xml.skipCurrentElement();
	}
	
	return true;
}

bool OgrTemplate::finishTypeSpecificTemplateConfiguration()
{
	if (!TemplateMap::finishTypeSpecificTemplateConfiguration())
		return false;
	
	if (template_track_compatibility)
	{
		if (is_georeferenced)
		{
			// Data is to be transformed to the map CRS directly.
			Q_ASSERT(projected_crs_spec.isEmpty());
		}
		else if (!track_crs_spec.contains(QLatin1String("+proj=latlong")))
		{
			// Nothing to do with this configuration
			Q_ASSERT(projected_crs_spec.isEmpty());
			if (!explicit_georef)
				map_configuration_georef = std::make_unique<Georeferencing>(map->getGeoreferencing());
		}
		else if (projected_crs_spec.isEmpty())
		{
			// Data is to be transformed to a projected CRS.
			// But we cannot calculate this georeferencing now,
			// because the template path is not yet validated.
			explicit_georef_pending = !explicit_georef;
		}
		else if (!explicit_georef)
		{
			// Data is to be transformed to the projected CRS.
			explicit_georef = makeGeoreferencing(projected_crs_spec);
		}
	}
	else if (!is_georeferenced && !explicit_georef)
	{
		map_configuration_georef = std::make_unique<Georeferencing>(map->getGeoreferencing());
	}
	return true;
}

void OgrTemplate::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (template_track_compatibility)
	{
		xml.writeTextElement(literal::crs_spec, track_crs_spec);
		if (!projected_crs_spec.isEmpty())
			xml.writeTextElement(literal::projected_crs_spec, projected_crs_spec);
	}
	else if (cs_domain == CoordinateSystem::DomainMap)
	{
		xml.writeTextElement(literal::srs, literal::map);
	}
	else if (explicit_georef)
	{
		explicit_georef->save(xml);
	}
}


}  // namespace OpenOrienteering
