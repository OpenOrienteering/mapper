/*
 *    Copyright 2013 Thomas Schöps
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

#ifdef QT_POSITIONING_LIB

#include "fake_position_source.h"

#include <algorithm>

#include <QtGlobal>
#include <QtMath>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QTime>
#include <QTimerEvent>


namespace OpenOrienteering
{

QGeoCoordinate FakePositionSource::initial_reference  {};

// static
void FakePositionSource::setReferencePoint(const QGeoCoordinate &reference)
{
	FakePositionSource::initial_reference = reference;
}


FakePositionSource::~FakePositionSource() = default;

FakePositionSource::FakePositionSource(QObject* object)
: FakePositionSource(initial_reference, object)
{}

FakePositionSource::FakePositionSource(const QGeoCoordinate& reference, QObject* object)
: QGeoPositionInfoSource(object)
, reference(reference)
{}


QGeoPositionInfoSource::Error FakePositionSource::error() const
{
	return NoError;
}

QGeoPositionInfo FakePositionSource::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
	return info;
}

int FakePositionSource::minimumUpdateInterval() const
{
	return 500;
}

QGeoPositionInfoSource::PositioningMethods FakePositionSource::supportedPositioningMethods() const
{
	return SatellitePositioningMethods;
}

void FakePositionSource::requestUpdate(int /*timeout*/)
{
	const auto now = QDateTime::currentDateTime();
	const auto offset = now.time().msecsSinceStartOfDay() / qreal(20000);
	const auto position = QGeoCoordinate {
	                      reference.latitude() + 0.001 * qSin(offset),
	                      reference.longitude() + 0.001 * qCos(offset * 1.1),
	                      reference.altitude() + 10 * qSin(1 + offset / 10) };
	info = {position, now};
	info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 12 + 7 * qSin(2 + offset));
	emit positionUpdated(info);
}

void FakePositionSource::startUpdates()
{
	if (!timer_id)
	{
		timer_id = startTimer(std::max(minimumUpdateInterval(), updateInterval()));
	}
}

void FakePositionSource::stopUpdates()
{
	if (timer_id)
	{
		killTimer(timer_id);
		timer_id = 0;
	}
}

void FakePositionSource::timerEvent(QTimerEvent* e)
{
	if (e->timerId() == timer_id)
	{
		requestUpdate(0);
	}
}


}  // namespace OpenOrienteering


#endif // QT_POSITIONING_LIB
