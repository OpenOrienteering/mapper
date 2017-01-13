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


const std::vector<QByteArray>& OgrTemplate::supportedExtensions()
{
	return GdalManager().supportedVectorExtensions();
}


OgrTemplate::OgrTemplate(const QString& path, Map* map)
: TemplateMap(path, map)
, migrating_from_template_track(false)
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


bool OgrTemplate::loadTemplateFileImpl(bool configuring)
{
	Q_UNUSED(configuring);
	
	std::unique_ptr<Map> new_template_map{ new Map() };
	new_template_map->setGeoreferencing(map->getGeoreferencing());
	
	QFile stream{ template_path };
	try
	{
		OgrFileImport importer{ &stream, new_template_map.get(), nullptr, migrating_from_template_track };
		importer.doImport(false, template_path);
		setTemplateMap(std::move(new_template_map));
		
		if (!importer.warnings().empty())
		{
			QString message;
			message.reserve((importer.warnings().back().length()+1) * importer.warnings().size());
			for (auto& warning : importer.warnings())
			{
				message.append(warning);
				message.append(QLatin1Char{'\n'});
			}
			message.chop(1);
			setErrorString(message);
		}
		
		return true;
	}
	catch (FileFormatException& e)
	{
		setErrorString(QString::fromUtf8(e.what()));
		return false;
	}
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
		migrating_from_template_track = true;
	}
	xml.skipCurrentElement();
	return true;
}

void OgrTemplate::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (migrating_from_template_track)
	{
		xml.writeEmptyElement(QLatin1String("crs_spec"));
	}
}
