/*
 *    Copyright 2021 Kai Pastor
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

#ifndef OPENORIENTEERING_COURSE_FILE_FORMAT_H
#define OPENORIENTEERING_COURSE_FILE_FORMAT_H

#include <functional>
#include <memory>
#include <vector>

#include "fileformats/file_format.h"

class QString;


namespace OpenOrienteering {

class Exporter;
class Map;
class MapView;

/**
 * A family of formats representing courses.
 */
class CourseFileFormat : public FileFormat
{
public:
	using ExporterBuilder = std::function<std::unique_ptr<Exporter> (const QString&, const Map*, const MapView*)>;
	
	/**
	 * Returns a container of all supported variants of this format.
	 */
	static std::vector<std::unique_ptr<CourseFileFormat>> makeAll();
	
	
	/**
	 * Constructs a new CourseFileFormat.
	 */
	CourseFileFormat(FileType type, const char* id, const QString& description, const QString& file_extension, ExporterBuilder exporter_builder);
	
	
	/// \copydoc FileFormat::makeExporter()
	std::unique_ptr<Exporter> makeExporter(const QString& path, const Map* map, const MapView* view) const override;
	
private:
	ExporterBuilder make_exporter;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_COURSE_FILE_FORMAT_H
