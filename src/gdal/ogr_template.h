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

#ifndef OPENORIENTEERING_OGR_TEMPLATE_H
#define OPENORIENTEERING_OGR_TEMPLATE_H

#include <memory>
#include <vector>

#include <QObject>
#include <QString>

#include "templates/template_map.h"

class QByteArray;
class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Georeferencing;
class Map;


/**
 * A Template which displays a file supported by OGR
 * (geospatial vector data).
 */
class OgrTemplate : public TemplateMap
{
Q_OBJECT
public:
	static bool canRead(const QString& path);
	
	static const std::vector<QByteArray>& supportedExtensions();
	
	static std::unique_ptr<Georeferencing> getDataGeoreferencing(const QString& path, const Georeferencing& initial_georef);
	
	
	OgrTemplate(const QString& path, Map* map);
	
protected:
	OgrTemplate(const OgrTemplate& proto);
	
public:
	~OgrTemplate() override;
	
	OgrTemplate* duplicate() const override;
	
	
	const char* getTemplateType() const override;
	
	std::unique_ptr<Georeferencing> makeOrthographicGeoreferencing(const QString& path) const;
	std::unique_ptr<Georeferencing> makeOrthographicGeoreferencing() const;
	std::unique_ptr<Georeferencing> makeGeoreferencing(const QString& spec) const;
	
	
	bool preLoadSetup(QWidget* dialog_parent) override;
	
	/**
	 * Loads the geospatial vector data into the template_map.
	 * 
	 * If is_georereferenced is true, the template_map will be configured to use
	 * the georeferencing of the map given in the constructor, and OgrFileImportFormat
	 * will let OGR do coordinate transformations as needed.
	 * 
	 * If is_georeferenced is false and an explicit_georef is defined, the
	 * template_map will be configured to use this particular georeferencing
	 * to produce a projection of the original data.
	 * 
	 * Otherwise, the data will be handled as raw map or paper data, depending on
	 * use_real_coords.
	 */
	bool loadTemplateFileImpl() override;
	
	bool postLoadSetup(QWidget* dialog_parent, bool& out_center_in_view) override;
	
	
	bool canChangeTemplateGeoreferenced() const override;
	
	
	void applySettings();
	
protected:
	void updateView(Map& template_map);
	
	void mapTransformationChanged() override;
	
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;
	bool finishTypeSpecificTemplateConfiguration() override;
	
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	
private:
	std::unique_ptr<Georeferencing> explicit_georef;
	std::unique_ptr<Georeferencing> map_configuration_georef;
	QString track_crs_spec;           // (limited) TemplateTrack compatibility
	QString projected_crs_spec;       // (limited) TemplateTrack compatibility
	bool template_track_compatibility { false };  //  transient
	bool explicit_georef_pending      { false };  //  transient
	bool use_real_coords              { true };   //  transient
	bool center_in_view               { false };  //  transient
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OGR_TEMPLATE_H
