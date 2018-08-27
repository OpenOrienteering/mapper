/*
 *    Copyright 2014 Thomas Schöps
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


#ifndef OPENORIENTEERING_GPS_TEMPORARY_MARKERS_H
#define OPENORIENTEERING_GPS_TEMPORARY_MARKERS_H

#include <vector>

#include <QObject>
#include <QPointF>

class QPainter;
// IWYU pragma: no_forward_declare QPointF

namespace OpenOrienteering {

class MapCoordF;
class MapWidget;
class GPSDisplay;


/** Displays temporary markers recorded with GPS. */
class GPSTemporaryMarkers : public QObject
{
Q_OBJECT
public:
	GPSTemporaryMarkers(MapWidget* widget, GPSDisplay* gps_display);
	~GPSTemporaryMarkers() override;
	
	/** Returns false if no point was added due to not having a valid position yet. */
	bool addPoint();
	/** Starts recording a GPS path. */
	void startPath();
	/** Stops recording a GPS path. */
	void stopPath();
	/** Deletes all temporary markers. */
	void clear();
	
	/// This is called from the MapWidget drawing code to draw the markers.
	void paint(QPainter* painter);
	
public slots:
	void newGPSPosition(const MapCoordF& coord, float accuracy);
	
private:
	void updateMapWidget();
	
	bool recording_path;
	std::vector< QPointF > points;
	std::vector< std::vector< QPointF > > paths;
	GPSDisplay* gps_display;
	MapWidget* widget;
};


}  // namespace OpenOrienteering

#endif
