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
#include <memory>

#include <QtMath>
#include <QtGlobal>
#include <QDialog>
#include <QFile>
#include <QLatin1String>
#include <QPointF>
#include <QRectF>
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
#include "templates/template.h"
#include "templates/template_positioning_dialog.h"


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


bool OgrTemplate::preLoadConfiguration(QWidget* dialog_parent)
try
{
	Q_ASSERT(!is_georeferenced);
	
	migrating_from_pre_v07 = false;
	
	auto& initial_georef = map->getGeoreferencing();
	if (!initial_georef.isValid() || initial_georef.isLocal())
	{
		// The map doesn't have a proper georeferencing.
		// Is there a good SRS in the data?
		QFile file{ template_path };
		Map tmp_map;
		tmp_map.setGeoreferencing(initial_georef);
		OgrFileImport importer{ &file, &tmp_map, nullptr, OgrFileImport::UnitOnGround};
		importer.setGeoreferencingImportEnabled(true);
		importer.doImport(true, template_path);
		
		auto& data_georef = tmp_map.getGeoreferencing();
		if (data_georef.isValid() && !data_georef.isLocal())
		{
			// If yes, does the user want to use this for the map?
			Georeferencing georef(data_georef);
			georef.setGeographicRefPoint(georef.toGeographicCoords(MapCoordF(map->calculateExtent().center())));
			GeoreferencingDialog dialog(dialog_parent, map, &georef);
			dialog.setKeepGeographicRefCoords();
			dialog.exec();
		}
	}
	
	auto& georef = map->getGeoreferencing();
	if (georef.isValid() && !georef.isLocal())
	{
		// Can the template's SRS be converted to the map's CRS?
		QFile file{ template_path };
		if (OgrFileImport::checkGeoreferencing(file, map->getGeoreferencing()))
		{
			is_georeferenced = true;
			crs_spec = georef.getProjectedCRSSpec();
			return true;
		}
		
		// Can the template's SRS be converted to WGS84?
		Georeferencing geographic_georef;
		geographic_georef.setScaleDenominator(int(georef.getScaleDenominator()));
		geographic_georef.setProjectedCRS(QString{}, QString::fromLatin1("+proj=latlong +datum=WGS84"));
		if (OgrFileImport::checkGeoreferencing(file, geographic_georef))
		{
			is_georeferenced = true;
			crs_spec = georef.getProjectedCRSSpec();
			return true;
		}
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
	if (migrating_from_pre_v07) 
		configuring = true;
	
	auto new_template_map = std::make_unique<Map>();
	const auto& georef = map->getGeoreferencing();
	new_template_map->setGeoreferencing(georef);
	
	QFile file{ template_path };
	
	auto is_geographic = [this](const QString& spec)->bool {
		return spec.contains(QLatin1String("+proj=latlong"));
	};
	if (!configuring
	    && !is_georeferenced
	    && is_geographic(crs_spec))
	{
		// Handle non-georeferenced geographic OgrTemplate data
		// by orthographic projection.
		// See TemplateTrack::calculateLocalGeoreferencing()
		auto center = OgrFileImport::calcAverageLatLon(file);
		auto ortho_georef = Georeferencing();
		ortho_georef.setScaleDenominator(int(georef.getScaleDenominator()));
		ortho_georef.setProjectedCRS(QString{}, QString::fromLatin1("+proj=ortho +datum=WGS84 +lat_0=%1 +lon_0=%2")
		                             .arg(center.latitude()).arg(center.longitude()));
		ortho_georef.setGeographicRefPoint(center, false);
		new_template_map->setGeoreferencing(ortho_georef);
	}
	
	auto unit_type = use_real_coords ? OgrFileImport::UnitOnGround : OgrFileImport::UnitOnPaper;
	OgrFileImport importer{ &file, new_template_map.get(), nullptr, unit_type };
	importer.setGeoreferencingImportEnabled(georef.isLocal() && configuring);
	
	importer.doImport(false, template_path);
	
	if (configuring)
	{
		const auto& new_georef = new_template_map->getGeoreferencing();
		crs_spec = new_georef.getProjectedCRSSpec();
		if (georef.isLocal() && crs_spec.contains(QLatin1String("+proj=ortho")))
		{
			// Geographic data that was projected automatically
			crs_spec = QString::fromLatin1("+proj=latlong +datum=WGS84");
		}
	}
	
	accounted_offset = {};
	if (is_georeferenced || !is_geographic(crs_spec))
	{
		// Handle data which has been subject to bounds handling during doImport().
		// p1 := ref_point + offset; p2: = ref_point;
		auto p1 = georef.toMapCoords(new_template_map->getGeoreferencing().getProjectedRefPoint());
		auto p2 = georef.getMapRefPoint();
		if (p1 != p2)
		{
			accounted_offset = QPointF{p1 - p2};
			if (!configuring || migrating_from_pre_v07)
			{
				QTransform t;
				t.rotate(-qRadiansToDegrees(getTemplateRotation()));
				t.scale(getTemplateScaleX(), getTemplateScaleY());
				setTemplatePosition(templatePosition() + MapCoord{t.map(accounted_offset)});
				migrating_from_pre_v07 = false;
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
	if (xml.name() == QLatin1String("crs_spec"))
	{
		migrating_from_pre_v07 = false;
		crs_spec = xml.readElementText();
	}
	else
	{
		xml.skipCurrentElement();
	}
	return true;
}

void OgrTemplate::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	xml.writeTextElement(QLatin1String("crs_spec"), crs_spec);
}
