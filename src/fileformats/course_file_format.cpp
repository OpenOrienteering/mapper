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

#include "course_file_format.h"

// IWYU pragma: no_include <algorithm>
#include <memory>

#include <QString>

#include "fileformats/file_import_export.h"
#include "fileformats/kml_course_export.h"


namespace OpenOrienteering {

template <class Exporter>
std::unique_ptr<CourseFileFormat> makeFileFormat(FileFormat::FileType type, const char* id)
{
	auto builder = [](const QString& path, const Map* map, const MapView* view) {
		return std::make_unique<Exporter>(path, map, view);
	};
	auto const description = Exporter::formatDescription();
	auto const file_extension = Exporter::filenameExtension();
	return std::make_unique<CourseFileFormat>(type, id, description, file_extension, builder);
}


// static
std::vector<std::unique_ptr<CourseFileFormat>> CourseFileFormat::makeAll()
{
	std::vector<std::unique_ptr<CourseFileFormat>> result;
	result.reserve(2);
	
	result.push_back(makeFileFormat<KmlCourseExport>(SimpleCourseFile, "simple-kml-course"));
	return result;
}


CourseFileFormat::CourseFileFormat(FileType type, const char* id, const QString& description, const QString& file_extension, ExporterBuilder exporter_builder)
: FileFormat { type, id, description, file_extension, FileFormat::Feature::FileExport | FileFormat::Feature::WritingLossy }
, make_exporter { exporter_builder }
{
	// Nothing
}


std::unique_ptr<Exporter> CourseFileFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	return make_exporter(path, map, view);
}


}  // namespace OpenOrienteering
