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

#include "fake_position_plugin.h"

#include "fake_position_source.h"


namespace OpenOrienteering
{

FakePositionPlugin::FakePositionPlugin(QObject* parent)
: QObject(parent)
{}

FakePositionPlugin::~FakePositionPlugin() = default;


QGeoAreaMonitorSource* FakePositionPlugin::areaMonitor(QObject* /* parent */)
{
	return nullptr;
}

QGeoPositionInfoSource* FakePositionPlugin::positionInfoSource(QObject* parent)
{
	return new FakePositionSource(parent);
}

QGeoSatelliteInfoSource* FakePositionPlugin::satelliteInfoSource(QObject* /* parent */)
{
	return nullptr;
}


}  // namespace OpenOrienteering
