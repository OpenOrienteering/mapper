/*
 *    Copyright 2016-2017 Kai Pastor
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

#include <algorithm>
#include <iterator>
#include <memory>

#include <Qt>
#include <QtMath>
#include <QtGlobal>
#include <QByteArray>
#include <QDialog>
#include <QFile>
#include <QLatin1String>
#include <QPoint>
#include <QPointF>
#include <QStringRef>
#include <QTransform>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "fileformats/file_format.h"
#include "gdal/gdal_manager.h"
#include "gdal/ogr_file_format_p.h"
#include "gui/georeferencing_dialog.h"
#include "sensors/gps_track.h"
#include "templates/template.h"
#include "templates/template_positioning_dialog.h"
#include "templates/template_track.h"


namespace
{
	namespace literal
	{
		const auto crs_spec           = QLatin1String("crs_spec");
		const auto georeferencing     = QLatin1String("georeferencing");
		const auto projected_crs_spec = QLatin1String("projected_crs_spec");
	}
	
	
	std::unique_ptr<Georeferencing> getDataGeoreferencing(QFile& file, const Georeferencing& initial_georef)
	{
		Map tmp_map;
		tmp_map.setGeoreferencing(initial_georef);
		OgrFileImport importer{ &file, &tmp_map, nullptr, OgrFileImport::UnitOnGround};
		importer.setGeoreferencingImportEnabled(true);
		importer.doImport(true);
		
		return std::make_unique<Georeferencing>(tmp_map.getGeoreferencing());
	}
	
	
	bool preserveRefPoints(Georeferencing& data_georef, const Georeferencing& initial_georef)
	{
		// Keep a configured local reference point from initial_georef?
		auto data_crs_spec = data_georef.getProjectedCRSSpec();
		if ((!initial_georef.isValid() || initial_georef.isLocal())
		    && data_georef.isValid()
		    && !data_georef.isLocal()
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
	
}



const std::vector<QByteArray>& OgrTemplate::supportedExtensions()
{
	return GdalManager().supportedVectorExtensions();
}


OgrTemplate::OgrTemplate(const QString& path, Map* map)
: TemplateMap(path, map)
{
	// nothing else
}

OgrTemplate::~OgrTemplate()
{
	if (template_state == Loaded)
		unloadTemplateFile();
}


const char* OgrTemplate::getTemplateType() const
{
	return "OgrTemplate";
}



std::unique_ptr<Georeferencing> OgrTemplate::makeOrthographicGeoreferencing(QFile& file)
{
	// Is the template's SRS orthographic, or can it be converted?
	/// \todo Use the template's datum etc. instead of WGS84?
	auto georef = std::make_unique<Georeferencing>();
	georef->setScaleDenominator(int(map->getGeoreferencing().getScaleDenominator()));
	georef->setProjectedCRS(QString{}, QStringLiteral("+proj=ortho +datum=WGS84 +ellps=WGS84 +units=m +no_defs"));
	if (OgrFileImport::checkGeoreferencing(file, *georef))
	{
		auto center = OgrFileImport::calcAverageLatLon(file);
		georef->setProjectedCRS(QString{},
		                             QString::fromLatin1("+proj=ortho +datum=WGS84 +ellps=WGS84 +units=m +lat_0=%1 +lon_0=%2 +no_defs")
		                             .arg(center.latitude()).arg(center.longitude()));
		georef->setProjectedRefPoint({}, false);
	}
	else
	{
		georef.reset();
	}
	return georef;
}


bool OgrTemplate::preLoadConfiguration(QWidget* dialog_parent)
try
{
	is_georeferenced = false;
	explicit_georef.reset();
	track_crs_spec.clear();
	transform = {};
	updateTransformationMatrices();
	
	auto ends_with_any_of = [](const QString& path, const std::vector<QByteArray>& list) -> bool {
		using namespace std;
		return any_of(begin(list), end(list), [path](const QByteArray& extension) {
			return path.endsWith(QLatin1String(extension), Qt::CaseInsensitive);
		} );
	};
	template_track_compatibility = ends_with_any_of(template_path, TemplateTrack::supportedExtensions());
	
	QFile file{ template_path };
	
	auto data_georef = std::unique_ptr<Georeferencing>();
	auto& initial_georef = map->getGeoreferencing();
	if (!initial_georef.isValid() || initial_georef.isLocal())
	{
		// The map doesn't have a proper georeferencing.
		// Is there a good SRS in the data?
		try {
			data_georef = getDataGeoreferencing(file, initial_georef);
		}
		catch (FileFormatException&)
		{}
		
		if (data_georef && data_georef->isValid() && !data_georef->isLocal())
		{
			// If yes, does the user want to use this for the map?
			auto keep_projected = false;
			if (template_track_compatibility)
				data_georef->setGrivation(0);
			else
				keep_projected = preserveRefPoints(*data_georef, initial_georef);
			GeoreferencingDialog dialog(dialog_parent, map, data_georef.get());
			if (keep_projected)
				dialog.setKeepProjectedRefCoords();
			else
				dialog.setKeepGeographicRefCoords();
			dialog.exec();
		}
	}
	
	auto& georef = map->getGeoreferencing();  // initial_georef might be outdated.
	if (georef.isValid() && !georef.isLocal())
	{
		// The map has got a proper georeferencing.
		// Can the template's SRS be converted to the map's CRS?
		if (OgrFileImport::checkGeoreferencing(file, map->getGeoreferencing()))
		{
			is_georeferenced = true;
			return true;
		}
	}
	
	// Is the template's SRS orthographic, or can it be converted?
	if (data_georef && !data_georef->getProjectedCRSSpec().contains(QLatin1String("+proj=ortho")))
	{
		data_georef.reset();
	}
	if (!data_georef)
	{
		data_georef = makeOrthographicGeoreferencing(file);
	}
	if (data_georef)
	{
		if (template_track_compatibility)
			data_georef->setGrivation(0);
		else
			preserveRefPoints(*data_georef, initial_georef);
		explicit_georef = std::move(data_georef);
	}
	
	TemplatePositioningDialog dialog(dialog_parent);
	if (dialog.exec() == QDialog::Rejected)
		return false;
	
	center_in_view  = dialog.centerOnView();
	use_real_coords = dialog.useRealCoords();
	if (use_real_coords)
	{
		transform.template_scale_x = transform.template_scale_y = dialog.getUnitScale();
		updateTransformationMatrices();
	}
	return true;
}
catch (FileFormatException& e)
{
	setErrorString(e.message());
	return false;
}


bool OgrTemplate::loadTemplateFileImpl(bool configuring)
try
{
	QFile file{ template_path };
	auto new_template_map = std::make_unique<Map>();
	auto unit_type = use_real_coords ? OgrFileImport::UnitOnGround : OgrFileImport::UnitOnPaper;
	OgrFileImport importer{ &file, new_template_map.get(), nullptr, unit_type };
	
	const auto& georef = map->getGeoreferencing();
	
	if (template_track_compatibility)
	{
		if (configuring)
		{
			if (is_georeferenced)
			{
				// Data is to be transformed to the map CRS directly.
				track_crs_spec = Georeferencing::geographic_crs_spec;
				projected_crs_spec.clear();
			}
			else if (explicit_georef)
			{
				// Data is to be transformed to the projected CRS.
				track_crs_spec = Georeferencing::geographic_crs_spec;
				projected_crs_spec = explicit_georef->getProjectedCRSSpec();
			}
			else
			{
				track_crs_spec.clear();
				projected_crs_spec.clear();
			}
		}
		else
		{
			Q_ASSERT(!explicit_georef);
			if (is_georeferenced)
			{
				// Data is to be transformed to the map CRS directly.
				Q_ASSERT(projected_crs_spec.isEmpty());
			}
			else if (!track_crs_spec.contains(QLatin1String("+proj=latlong")))
			{
				// Nothing to do with this configuration
				Q_ASSERT(projected_crs_spec.isEmpty());
			}
			else
			{
				// Data is to be transformed to the projected CRS.
				if (projected_crs_spec.isEmpty())
				{
					// Need to create an orthographic projection.
					// For a transition period, copy behaviour from
					// TemplateTrack::loadTemplateFileImpl(),
					// TemplateTrack::calculateLocalGeoreferencing().
					// This is an extra expense (by loading the data twice), but
					// only until the map (projected_crs_spec) is saved once.
					TemplateTrack track{template_path, map};
					if (track.track.loadFrom(template_path, false))
					{
						projected_crs_spec = track.calculateLocalGeoreferencing();
					}
					else
					{
						// If the TemplateTrack approach failed, use local approach.
						explicit_georef = makeOrthographicGeoreferencing(file);
						projected_crs_spec = explicit_georef->getProjectedCRSSpec();
					}
				}
				
				if (!explicit_georef)
				{
					explicit_georef.reset(new Georeferencing());
					explicit_georef->setScaleDenominator(int(georef.getScaleDenominator()));
					explicit_georef->setProjectedCRS(QString{}, projected_crs_spec);
					explicit_georef->setProjectedRefPoint({}, false);
				}
			}
		}
	}
	
	
	if (is_georeferenced)
	{
		new_template_map->setGeoreferencing(georef);
		importer.setGeoreferencingImportEnabled(false);
	}
	else if (explicit_georef)
	{
		new_template_map->setGeoreferencing(*explicit_georef);
		importer.setGeoreferencingImportEnabled(false);
	}
	else
	{
		new_template_map->setGeoreferencing(georef);
		importer.setGeoreferencingImportEnabled(false);
	}
	
	importer.doImport(false, template_path);
	
	accounted_offset = {};
	if (is_georeferenced || !explicit_georef)
	{
		// Handle data which has been subject to bounds handling during doImport().
		// p1 := ref_point + offset; p2: = ref_point;
		auto p1 = georef.toMapCoords(new_template_map->getGeoreferencing().getProjectedRefPoint());
		auto p2 = georef.getMapRefPoint();
		if (p1 != p2)
		{
			accounted_offset = QPointF{p1 - p2};
			if (accounted_offset != QPointF{})
			{
				QTransform t;
				t.rotate(-qRadiansToDegrees(getTemplateRotation()));
				t.scale(getTemplateScaleX(), getTemplateScaleY());
				setTemplatePosition(templatePosition() + MapCoord{t.map(accounted_offset)});
			}
		}
	}
	
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


bool OgrTemplate::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(dialog_parent)
	out_center_in_view = center_in_view;
	return true;
}


OgrTemplate* OgrTemplate::duplicateImpl() const
{
	auto copy = new OgrTemplate(template_path, map);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl(false);
	return copy;
}


bool OgrTemplate::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	if (xml.name() == literal::georeferencing)
	{
		explicit_georef.reset(new Georeferencing());
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
	else
	{
		xml.skipCurrentElement();
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
	else if (explicit_georef)
	{
		explicit_georef->save(xml);
	}
}
