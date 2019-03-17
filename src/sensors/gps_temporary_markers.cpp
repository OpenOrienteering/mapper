/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2015, 2018 Kai Pastor
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


#include "gps_temporary_markers.h"

#include <memory>

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <Qt>

#include "core/map_coord.h"
#include "core/map_view.h"
#include "gui/map/map_widget.h"
#include "gps_display.h"
#include "tools/tool.h"

// IWYU pragma: no_forward_declare QPointF


namespace OpenOrienteering {

GPSTemporaryMarkers::GPSTemporaryMarkers(MapWidget* widget, GPSDisplay* gps_display, QObject* parent)
: QObject(parent)
, gps_display(gps_display)
, widget(widget)
{
	connect(gps_display, &GPSDisplay::mapPositionUpdated, this, &GPSTemporaryMarkers::newGPSPosition);
	widget->setTemporaryMarkerDisplay(this);
}

GPSTemporaryMarkers::~GPSTemporaryMarkers()
{
	widget->setTemporaryMarkerDisplay(nullptr);
}

bool GPSTemporaryMarkers::addPoint()
{
	if (!gps_display->hasValidPosition())
		return false;
	
	points.push_back(gps_display->getLatestGPSCoord());
	updateMapWidget();
	return true;
}

void GPSTemporaryMarkers::startPath()
{
	paths.push_back(std::vector< QPointF >());
	recording_path = true;
	if (gps_display->hasValidPosition())
		newGPSPosition(gps_display->getLatestGPSCoord(), gps_display->getLatestGPSCoordAccuracy());
}

void GPSTemporaryMarkers::stopPath()
{
	recording_path = false;
}

void GPSTemporaryMarkers::clear()
{
	points.clear();
	paths.clear();
	updateMapWidget();
}

void GPSTemporaryMarkers::paint(QPainter* painter)
{
	painter->save();
	widget->applyMapTransform(painter);
	const auto scale_factor = 1 / widget->getMapView()->getZoom();
	
	// Draw paths
	painter->setBrush(Qt::NoBrush);

	painter->setPen(QPen(QBrush(qRgb(255, 255, 255)), scale_factor * 0.3));
	for (const auto& path : paths)
		painter->drawPolyline(path.data(), int(path.size()));
	
	painter->setPen(QPen(QBrush(MapEditorTool::active_color), scale_factor * 0.2));
	for (const auto& path : paths)
		painter->drawPolyline(path.data(), int(path.size()));
	
	// Draw points
	painter->setPen(Qt::NoPen);
	
	painter->setBrush(QBrush(qRgb(255, 255, 255)));
	auto point_radius = scale_factor * 0.5;
	for (const auto& point : points)
		painter->drawEllipse(point, point_radius, point_radius);
	
	painter->setBrush(QBrush(MapEditorTool::inactive_color));
	point_radius = scale_factor * 0.4;
	for (const auto& point : points)
		painter->drawEllipse(point, point_radius, point_radius);
	
	painter->restore();
}

void GPSTemporaryMarkers::newGPSPosition(const MapCoordF& coord, float accuracy)
{
	Q_UNUSED(accuracy);

	if (recording_path && ! paths.empty())
	{
		paths.back().push_back(coord);
		updateMapWidget();
	}
}

void GPSTemporaryMarkers::updateMapWidget()
{
	/// \todo Limit the updated area here
	widget->update();
}


}  // namespace OpenOrienteering
