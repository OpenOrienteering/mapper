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


#ifndef _OPENORIENTEERING_COMPASS_DISPLAY_H_
#define _OPENORIENTEERING_COMPASS_DISPLAY_H_

#include <QObject>
#include <QTime>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE
class MapWidget;


/**
 * Displays a compass on a MapWidget based on digital compass readings.
 */
class CompassDisplay : public QObject
{
Q_OBJECT
public:
	/// Creates a compass display for the given map widget.
	CompassDisplay(MapWidget* map_widget);
	/// Destructor, removes the compass display from the map widget.
	~CompassDisplay();
	
	/// Enables or disables compass display (by default it is disabled).
	void enable(bool enabled);
	
	/// This is called from the MapWidget drawing code to draw the compass.
	void paint(QPainter* painter);
	
public slots:
	/// Called internally to update the value
	void valueChanged(float azimuth_deg);
	
private:
	void updateMapWidget();
	QRectF calcBoundingBox();
	
	
	bool have_value;
	qreal value_azimuth;
	qreal value_calibration;
	QTime last_update_time;
	
	bool enabled;
	MapWidget* widget;
};

#endif
