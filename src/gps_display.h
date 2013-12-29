/*
 *    Copyright 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_GPS_DISPLAY_H_
#define _OPENORIENTEERING_GPS_DISPLAY_H_

#include <QObject>
#if defined(ENABLE_POSITIONING)
	#include <QtPositioning/QGeoPositionInfo>
	#include <QtPositioning/QGeoPositionInfoSource>
#endif

#include "map_coord.h"

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE
class MapWidget;
class Georeferencing;
class QGeoPositionInfo;
class QGeoPositionInfoSource;


/**
 * Retrieves the GPS position and displays a marker at this position on a MapWidget.
 */
class GPSDisplay : public QObject
{
Q_OBJECT
public:
	/// Creates a GPS display for the given map widget and georeferencing.
	GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing);
	/// Destructor, removes the GPS display from the map widget.
	~GPSDisplay();
	
	/// Starts regular position updates. This will issue redraws of the map widget.
	void startUpdates();
	/// Stops regular position updates.
	void stopUpdates();
	
	/// Sets GPS marker visibility (true by default)
	void setVisible(bool visible);
	/// Returns GPS marker visibility
	inline bool isVisible() const {return visible;}
	
	/// This is called from the MapWidget drawing code to draw the GPS position marker.
	void paint(QPainter* painter);
	
private slots:
#if defined(ENABLE_POSITIONING)
    void positionUpdated(const QGeoPositionInfo& info);
	void error(QGeoPositionInfoSource::Error positioningError);
	void updateTimeout();
#endif
	
private:
	MapCoordF getLatestGPSCoord(bool& ok);
	void updateMapWidget();
	
	bool gps_updated;
#if defined(ENABLE_POSITIONING)
	QGeoPositionInfo latest_pos_info;
#endif
	MapCoordF latest_gps_coord;
	bool tracking_lost;
	bool has_valid_position;
	
	bool visible;
	QGeoPositionInfoSource* source;
	MapWidget* widget;
	const Georeferencing& georeferencing;
};

#endif
