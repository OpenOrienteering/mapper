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

#ifndef OPENORIENTEERING_OGR_TEMPLATE_H
#define OPENORIENTEERING_OGR_TEMPLATE_H

#include "../template_map.h"

#include "../core/georeferencing.h"


/**
 * Template displaying a file supported by OGR.
 */
class OgrTemplate : public TemplateMap
{
Q_OBJECT
public:
	static const std::vector<QByteArray>& supportedExtensions();
	
	
	OgrTemplate(const QString& path, Map* map);
	
	~OgrTemplate() override;
	
	
	QString getTemplateType() const override;
	
	
	bool loadTemplateFileImpl(bool configuring) override;
	
protected:
	Template* duplicateImpl() const override;
};

#endif // OPENORIENTEERING_OGR_TEMPLATE_H
