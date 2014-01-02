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


#include "gps_temporary_markers.h"

#include <QPainter>

#include "map_widget.h"
#include "gps_display.h"
#include "tool.h"
#include "util.h"


GPSTemporaryMarkers::GPSTemporaryMarkers(MapWidget* widget, GPSDisplay* gps_display): QObject()
{
	this->widget = widget;
	this->gps_display = gps_display;
	recording_path = false;
	
	connect(gps_display, SIGNAL(mapPositionUpdated(MapCoordF,float)), this, SLOT(newGPSPosition(MapCoordF,float)));
	
	widget->setTemporaryMarkerDisplay(this);
}

GPSTemporaryMarkers::~GPSTemporaryMarkers()
{
	widget->setTemporaryMarkerDisplay(NULL);
}

void GPSTemporaryMarkers::addPoint()
{
	if (gps_display->hasValidPosition())
	{
		points.push_back(gps_display->getLatestGPSCoord().toQPointF());
		updateMapWidget();
	}
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
	float scale_factor = 1.0f / widget->getMapView()->getZoom();
	
	// Draw paths
	painter->setBrush(Qt::NoBrush);

	painter->setPen(QPen(QBrush(qRgb(255, 255, 255)), scale_factor * 0.3f));
	for (size_t path_number = 0; path_number < paths.size(); ++ path_number)
		painter->drawPolyline(paths[path_number].data(), paths[path_number].size());
	
	painter->setPen(QPen(QBrush(MapEditorTool::active_color), scale_factor * 0.2f));
	for (size_t path_number = 0; path_number < paths.size(); ++ path_number)
		painter->drawPolyline(paths[path_number].data(), paths[path_number].size());
	
	// Draw points
	painter->setPen(Qt::NoPen);
	
	painter->setBrush(QBrush(qRgb(255, 255, 255)));
	float point_radius = scale_factor * 0.5f;
	for (size_t point_number = 0; point_number < points.size(); ++ point_number)
		painter->drawEllipse(points[point_number], point_radius, point_radius);
	
	painter->setBrush(QBrush(MapEditorTool::inactive_color));
	point_radius = scale_factor * 0.4f;
	for (size_t point_number = 0; point_number < points.size(); ++ point_number)
		painter->drawEllipse(points[point_number], point_radius, point_radius);
	
	painter->restore();
}

void GPSTemporaryMarkers::newGPSPosition(MapCoordF coord, float accuracy)
{
	Q_UNUSED(accuracy);

	if (recording_path && ! paths.empty())
	{
		std::vector< QPointF >& path_coords = paths.back();
		path_coords.push_back(coord.toQPointF());
		updateMapWidget();
	}
}

void GPSTemporaryMarkers::updateMapWidget()
{
	// NOTE: could limit the updated area here
	widget->update();
}
