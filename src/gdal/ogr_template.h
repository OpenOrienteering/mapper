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

#ifndef OPENORIENTEERING_OGR_TEMPLATE_H
#define OPENORIENTEERING_OGR_TEMPLATE_H

#include "templates/template_map.h"

#include "core/georeferencing.h"


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
	
	
	const char* getTemplateType() const override;
	
	bool preLoadConfiguration(QWidget* dialog_parent) override;
	
	bool loadTemplateFileImpl(bool configuring) override;
	
	bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view) override;
	
protected:
	Template* duplicateImpl() const override;
	
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;
	
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	
private:
	QString crs_spec;
	bool migrating_from_pre_v07;  /// Some files saved with unstable snapshots < v0.7
	bool use_real_coords;
	bool center_in_view;
};

#endif // OPENORIENTEERING_OGR_TEMPLATE_H
