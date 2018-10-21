/*
 *    Copyright 2013 Thomas Schöps
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

#ifdef QT_POSITIONING_LIB

#include "fake_position_info_source.h"

#include <algorithm>

#include <QtGlobal>
#include <QtMath>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QObject>
#include <QTime>
#include <QTimerEvent>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/latlon.h"

#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
// QTBUG-65937
#include <QMetaType>
Q_DECLARE_METATYPE(QGeoPositionInfo);
#endif

namespace OpenOrienteering
{

FakePositionInfoSource::FakePositionInfoSource(Map& map, QObject* object)
: QGeoPositionInfoSource(object)
, map(map)
{
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
	// QTBUG-65937
	qRegisterMetaType<QGeoPositionInfo>();
#endif
}

OpenOrienteering::FakePositionInfoSource::~FakePositionInfoSource() = default;


QGeoPositionInfoSource::Error FakePositionInfoSource::error() const
{
	return NoError;
}

QGeoPositionInfo FakePositionInfoSource::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
	return info;
}

int FakePositionInfoSource::minimumUpdateInterval() const
{
	return 500;
}

QGeoPositionInfoSource::PositioningMethods FakePositionInfoSource::supportedPositioningMethods() const
{
	return SatellitePositioningMethods;
}

void FakePositionInfoSource::requestUpdate(int /*timeout*/)
{
	const auto now = QDateTime::currentDateTime();
	const auto offset = now.time().msecsSinceStartOfDay() / qreal(20000);
	const auto latlon = map.getGeoreferencing().getGeographicRefPoint();
	const auto position = QGeoCoordinate {
	                      latlon.latitude() + 0.001 * qSin(offset),
	                      latlon.longitude() + 0.001 * qCos(offset * 1.1),
	                      /* altitude */ 400 + 10 * qSin(1 + offset / 10) };
	info = {position, now};
	info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 12 + 7 * qSin(2 + offset));
	emit positionUpdated(info);
}

void FakePositionInfoSource::startUpdates()
{
	if (!timer_id)
		timer_id = startTimer(std::max(minimumUpdateInterval(), updateInterval()));
}

void FakePositionInfoSource::stopUpdates()
{
	if (!timer_id)
	{
		killTimer(timer_id);
		timer_id = 0;
	}
}

void FakePositionInfoSource::timerEvent(QTimerEvent* e)
{
	if (e->timerId() == timer_id)
		requestUpdate(0);
}


}  // namespace OpenOrienteering


#endif // QT_POSITIONING_LIB
