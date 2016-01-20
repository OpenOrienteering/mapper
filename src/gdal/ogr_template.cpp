/*
 *    Copyright 2016 Kai Pastor
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

#include "ogr_file_format_p.h"
#include "../map.h"


namespace
{
	/**
	 * OGR driver registration utility.
	 */
	class OgrDriverSetup
	{
	public:
		OgrDriverSetup()
		{
			OGRRegisterAll();
		}
	};
}


const std::vector<QByteArray>& OgrTemplate::supportedExtensions()
{
	/// \todo Build from driver list in GDAL/OGR >= 2.0
	static std::vector<QByteArray> extensions = { "dxf" };
	return extensions;
}


OgrTemplate::OgrTemplate(const QString& path, Map* map)
: TemplateMap(path, map)
{
	static OgrDriverSetup registered;
}

OgrTemplate::~OgrTemplate()
{
	// nothing, not inlined
}


QString OgrTemplate::getTemplateType() const
{
	return QLatin1String("OgrTemplate");
}


bool OgrTemplate::loadTemplateFileImpl(bool configuring)
{
	Q_UNUSED(configuring);
	
	std::unique_ptr<Map> new_template_map{ new Map() };
	new_template_map->setGeoreferencing(map->getGeoreferencing());
	
	QFile stream{ template_path };
	OgrFileImport importer{ &stream, new_template_map.get(), nullptr };
	importer.doImport(false, template_path);
	
	setTemplateMap(std::move(new_template_map));
	return true;
}


Template* OgrTemplate::duplicateImpl() const
{
	OgrTemplate* copy = new OgrTemplate(template_path, map);
	if (template_state == Loaded)
		copy->loadTemplateFileImpl(false);
	return copy;
}
