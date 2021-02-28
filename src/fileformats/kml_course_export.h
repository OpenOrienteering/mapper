/*
 *    Copyright 2020 Kai Pastor
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

#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QString>

class QSaveFile;

namespace OpenOrienteering {

class LatLon;
class Map;
class MapCoord;
class PathObject;


/**
 * This class generates KML course files for MapRunF.
 * 
 * This export handles a single path object and outputs placemarks for start
 * (S1), finish (F1), and controls in between (starting from number 1).
 * 
 * Due to its special mode of operation, this class is not implemented as a
 * subclass of Exporter. But the API is kept similar.
 */
class KmlCourseExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::KmlCourseExport)
	
public:
	~KmlCourseExport();
	
	KmlCourseExport(const Map& map);
	
	bool doExport(const QString& filepath);
	
	QString errorString() const;
	
protected:
	void writeKml(const PathObject& object);
	
	void writeKmlPlacemarks(const std::vector<MapCoord>& coords);
	
	void writeKmlPlacemark(const MapCoord& coord, const char* name, const char* description);
	
	void writeCoordinates(const LatLon& latlon);
	
private:
	std::unique_ptr<QSaveFile> device;
	const Map& map;
	QString error_string;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_KML_EXPORT_EXPORT_H
