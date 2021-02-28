/*
 *    Copyright 2020-2021 Kai Pastor
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

#ifndef OPENORIENTEERING_KML_EXPORT_EXPORT_H
#define OPENORIENTEERING_KML_EXPORT_EXPORT_H

#include <vector>

#include "fileformats/file_import_export.h"

class QString;

namespace OpenOrienteering {

class LatLon;
class Map;
class MapCoord;
class MapView;
class PathObject;


/**
 * This class generates KML course files for MapRunF.
 * 
 * This export handles a single path object and outputs placemarks for start
 * (S1), finish (F1), and controls in between (starting from number 1).
 */
class KmlCourseExport : public Exporter
{
public:
	static QString formatDescription();
	static QString filenameExtension();
	
	~KmlCourseExport();
	
	KmlCourseExport(const QString& path, const Map* map, const MapView* view);
	
protected:
	bool exportImplementation() override;
	
	void writeKml(const PathObject& object);
	
	void writeKmlPlacemarks(const std::vector<MapCoord>& coords);
	
	void writeKmlPlacemark(const MapCoord& coord, const char* name, const char* description);
	
	void writeCoordinates(const LatLon& latlon);
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_KML_EXPORT_EXPORT_H
