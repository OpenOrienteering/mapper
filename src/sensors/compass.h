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


#ifndef OPENORIENTEERING_COMPASS_H
#define OPENORIENTEERING_COMPASS_H

#include <memory>

#include <QObject>

class QMetaMethod;

namespace OpenOrienteering {

class CompassPrivate;


/** Provides access to the device's compass. Singleton class. */
class Compass : public QObject
{
Q_OBJECT
friend class CompassPrivate;

private:
	Compass();
	~Compass() override;

public:
	/** Singleton accessor method. Constructs the object on first use. */
	static Compass& getInstance();
	
	/** Adds a reference. If this is the first one, initializes the compass. */
	void startUsage();
	
	/** Dereferences compass usage. */
	void stopUsage();
	
	/** Returns the most recent azimuth value
	 *  (in degrees clockwise from north; updated approx. every 30 milliseconds). */
	float getCurrentAzimuth();
	
	/** Connects to the azimuthChanged(float azimuth_degrees) signal. This ensures to use a queued
	 *  connection, which is important because the data provider runs on another
	 *  thread. Updates are delivered approx. every 30 milliseconds. */
	void connectToAzimuthChanges(const QObject* receiver, const char* slot);
	
	/** Disconnects the given receiver from azimuth changes. */
	void disconnectFromAzimuthChanges(const QObject* receiver);
	
signals:
	/** Emitted regularly with the current azimuth value (in degrees).
	 *  Preferably use connectToAzimuthChanges() to connect to this signal. */
	void azimuthChanged(float azimuth);
	
protected:
	void connectNotify(const QMetaMethod& signal) override;
	
	void disconnectNotify(const QMetaMethod& signal) override;
	
private:
	void emitAzimuthChanged(float value);
	
	std::unique_ptr<CompassPrivate> p;
	int reference_counter = 0;
};


}  // namespace OpenOrienteering

#endif
