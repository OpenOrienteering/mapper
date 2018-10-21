/*
 *    Copyright 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_FAKE_POSITION_INFO_SOURCE_H
#define OPENORIENTEERING_FAKE_POSITION_INFO_SOURCE_H

#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>

class QObject;
class QTimerEvent;

namespace OpenOrienteering
{

class Map;


/**
 * A position info source that provides fake data for a given map.
 * 
 * This is a development tools It needs to be enabled at CMake configuration
 * time by setting `MAPPER_FAKE_POSITION_SOURCE` to true.
 */
class FakePositionInfoSource : public QGeoPositionInfoSource
{
public:
	FakePositionInfoSource(Map& map, QObject* object);
	~FakePositionInfoSource() override;
	
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
	Map& map;
	QGeoPositionInfo info;
	int timer_id = 0;
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_FAKE_POSITION_INFO_SOURCE_H
