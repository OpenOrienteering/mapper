#
#    Copyright 2019 Kai Pastor
#
#    This file is part of OpenOrienteering.
#
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

Add-Type -AssemblyName System.Device
$watcher = New-Object System.Device.Location.GeoCoordinateWatcher -arg 'High'

# Event handling

$statusChangedAction = {
	$status_ = $EventArgs.Status
	$line_ = "Status;$status_"
    $line_ | Write-Host
}
Register-ObjectEvent -InputObject $watcher -EventName StatusChanged -Action $statusChangedAction | Out-Null

$positionChangedAction = {
	$time_ = $EventArgs.Position.Timestamp.UtcDateTime
	$loc_ = $EventArgs.Position.Location
	$lat_ = $loc_.Latitude
	$lon_ = $loc_.Longitude
	$alt_ = $loc_.Altitude
	$hac_ = $loc_.HorizontalAccuracy
	$vac_ = $loc_.VerticalAccuracy
	# "" string interpretation does not use the culture, -f does.
	$line_ = "Position;{0:u};$lat_;$lon_;$alt_;$hac_;$vac_" -f ($time_)
    $line_ | Write-Host
}
Register-ObjectEvent -InputObject $watcher -EventName PositionChanged -Action $positionChangedAction | Out-Null

# Start

$watcher.start()

# Output permission status

$permission_ = $watcher.Permission
"Permission;$permission_" | Write-Host
