/*
 *    Copyright 2019 Kai Pastor
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

#include "powershell_position_plugin.h"

#include "powershell_position_source.h"


namespace OpenOrienteering
{

PowershellPositionPlugin::PowershellPositionPlugin(QObject* parent)
: QObject(parent)
{}

PowershellPositionPlugin::~PowershellPositionPlugin() = default;


QGeoAreaMonitorSource* PowershellPositionPlugin::areaMonitor(QObject* /* parent */)
{
	return nullptr;
}

QGeoPositionInfoSource* PowershellPositionPlugin::positionInfoSource(QObject* parent)
{
	return new PowershellPositionSource(parent);
}

QGeoSatelliteInfoSource* PowershellPositionPlugin::satelliteInfoSource(QObject* /* parent */)
{
	return nullptr;
}


}  // namespace OpenOrienteering
