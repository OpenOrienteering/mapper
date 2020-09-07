/*
 *    Copyright 2018-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_FAKE_POSITION_SOURCE_H
#define OPENORIENTEERING_FAKE_POSITION_SOURCE_H

#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QObject>
#include <QString>

class QTimerEvent;

namespace OpenOrienteering
{


/**
 * A position info source that provides fake data.
 * 
 * This is a development tool providing fake positions near an initial
 * reference point.
 */
class FakePositionSource : public QGeoPositionInfoSource
{
	Q_OBJECT
	
public:
	/**
	 * Sets an initial reference point for objects constructed without
	 * explicit reference coordinate parameter.
	 */
	static void setReferencePoint(const QGeoCoordinate& reference);
	
	~FakePositionSource() override;
	FakePositionSource(QObject* object);
	FakePositionSource(const QGeoCoordinate& reference, QObject* object);
	
	FakePositionSource(const FakePositionSource&) = delete;
	FakePositionSource(FakePositionSource&&) = delete;
	FakePositionSource& operator=(const FakePositionSource&) = delete;
	FakePositionSource& operator=(FakePositionSource&&) = delete;
	
	QGeoPositionInfoSource::Error error() const override;
	QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const override;
	int minimumUpdateInterval() const override;
	QGeoPositionInfoSource::PositioningMethods supportedPositioningMethods() const override;
	
	void requestUpdate(int timeout) override;
	void startUpdates() override;
	void stopUpdates() override;
	
protected:
	void timerEvent(QTimerEvent* e) override;
	
private:
	static QGeoCoordinate initial_reference;
	QGeoCoordinate reference;
	QGeoPositionInfo info;
	int timer_id = 0;
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_FAKE_POSITION_SOURCE_H
