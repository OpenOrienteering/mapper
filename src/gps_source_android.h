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


#ifndef _OPENORIENTEERING_GPS_SOURCE_ANDROID_H_
#define _OPENORIENTEERING_GPS_SOURCE_ANDROID_H_

#include <QtPositioning/QGeoPositionInfoSource>

/** QGeoPositionInfoSource implementation for Android using GPS.
 *  ATTENTION: there must not exist two instances of this class at the same time. */
class AndroidGPSPositionSource : public QGeoPositionInfoSource
{
Q_OBJECT
public:
	AndroidGPSPositionSource(QObject* parent = 0);

	virtual QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;

	virtual PositioningMethods supportedPositioningMethods() const;
	virtual int minimumUpdateInterval() const;
	virtual Error error() const;
	
	// Internal methods
	void positionUpdatedCallback(float latitude, float longitude, float altitude, float horizontal_stddev);
	void setError();
	void setUpdateTimeout();

public slots:
	virtual void startUpdates();
	virtual void stopUpdates();

	virtual void requestUpdate(int timeout = 5000);

private:
	bool has_error;
	bool updates_started;
	QGeoPositionInfo lastPosition;
};

#endif
