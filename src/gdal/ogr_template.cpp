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

#include <QFile>
#include <QXmlStreamReader>

#include "gdal_manager.h"
#include "ogr_file_format_p.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "templates/template_positioning_dialog.h"


const std::vector<QByteArray>& OgrTemplate::supportedExtensions()
{
	return GdalManager().supportedVectorExtensions();
}


OgrTemplate::OgrTemplate(const QString& path, Map* map)
: TemplateMap(path, map)
, migrating_from_pre_v07{ true }
, use_real_coords{ true }
, center_in_view{ false }
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
{
	migrating_from_pre_v07 = false;
	
	auto show_dialog = true;
	
	QFile file{ template_path };
	try
	{
		show_dialog = !OgrFileImport::checkGeoreferencing(file, map->getGeoreferencing());
	}
	catch (FileFormatException& e)
	{
		setErrorString(e.message());
		return false;
	}
	
	if (show_dialog)
	{
		TemplatePositioningDialog dialog(dialog_parent);
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
		use_real_coords = dialog.useRealCoords();
		if (use_real_coords)
		{
			transform.template_scale_x = transform.template_scale_y = dialog.getUnitScale();
			updateTransformationMatrices();
		}
		center_in_view = dialog.centerOnView();
	}
	return true;
}


bool OgrTemplate::loadTemplateFileImpl(bool configuring)
{
	if (migrating_from_pre_v07) 
		configuring = true;
	
	std::unique_ptr<Map> new_template_map{ new Map() };
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
		// Handle non-georeferenced geographic TemplateTrack data
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
	
	try
	{
		importer.doImport(false, template_path);
	}
	catch (FileFormatException& e)
	{
		setErrorString(e.message());
		return false;
	}
	
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
				t.rotate(-getTemplateRotation() * (180 / M_PI));
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
			message.append(QLatin1Char{'\n'});
		}
		message.chop(1);
		setErrorString(message);
	}
	
	return true;
}


bool OgrTemplate::postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view)
{
	Q_UNUSED(dialog_parent)
	is_georeferenced = false;
	out_center_in_view = center_in_view;
	return true;
}


Template* OgrTemplate::duplicateImpl() const
{
	OgrTemplate* copy = new OgrTemplate(template_path, map);
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
