/*
 *    Copyright 2013 Thomas Schöps
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

#include "gps_display.h"

#include <QDebug>

#include "map_widget.h"
#include "georeferencing.h"
#include "util.h"

#if defined(ENABLE_POSITIONING)
	#include <QtPositioning/QGeoPositionInfoSource>
#endif
#include <QPainter>

GPSDisplay::GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing)
 : QObject()
 , georeferencing(georeferencing)
{
	this->widget = widget;
	
	gps_updated = false;
	tracking_lost = false;
	
#if defined(ENABLE_POSITIONING)
	source = QGeoPositionInfoSource::createDefaultSource(this);
	if (!source)
	{
		qDebug() << "Cannot create QGeoPositionInfoSource!";
		return;
	}
	
	source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
	source->setUpdateInterval(1000);
	connect(source, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
	connect(source, SIGNAL(error(QGeoPositionInfoSource::Error)), this, SLOT(error(QGeoPositionInfoSource::Error)));
	connect(source, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
#else
	source = NULL;
#endif
	
	widget->setGPSDisplay(this);
}

GPSDisplay::~GPSDisplay()
{
	widget->setGPSDisplay(NULL);
}

void GPSDisplay::startUpdates()
{
#if defined(ENABLE_POSITIONING)
	source->startUpdates();
#endif
}

void GPSDisplay::stopUpdates()
{
#if defined(ENABLE_POSITIONING)
	source->stopUpdates();
#endif
}

void GPSDisplay::setVisible(bool visible)
{
	if (this->visible == visible)
		return;
	this->visible = visible;
	
	updateMapWidget();
}

void GPSDisplay::paint(QPainter* painter)
{
	if (!visible || !source)
		return;
	
	// Get GPS position on map widget
	bool ok = true;
	MapCoordF gps_coord = getLatestGPSCoord(ok);
	if (!ok)
		return;
	QPointF gps_pos = widget->mapToViewport(gps_coord);
	
	// Draw dot
	qreal dot_radius = Util::mmToPixelLogical(0.5f);
// 	painter->setPen(QPen(tracking_lost ? Qt::gray : Qt::red, Util::mmToPixelLogical(0.5f)));
// 	painter->setBrush(Qt::NoBrush);
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(tracking_lost ? Qt::gray : Qt::red));
	painter->drawEllipse(gps_pos, dot_radius, dot_radius);
}

#if defined(ENABLE_POSITIONING)
void GPSDisplay::positionUpdated(const QGeoPositionInfo& info)
{
	Q_UNUSED(info);

	gps_updated = true;
	tracking_lost = false;
	
	updateMapWidget();
}

void GPSDisplay::error(QGeoPositionInfoSource::Error positioningError)
{
	if (positioningError != QGeoPositionInfoSource::NoError)
	{
		if (!tracking_lost)
			updateMapWidget();
		tracking_lost = true;
	}
}

void GPSDisplay::updateTimeout()
{
	// Lost satellite fix
	if (!tracking_lost)
		updateMapWidget();
	tracking_lost = true;
}
#endif

MapCoordF GPSDisplay::getLatestGPSCoord(bool& ok)
{
#if defined(ENABLE_POSITIONING)
	if (!gps_updated)
		return latest_gps_coord;
	
	QGeoPositionInfo pos_info = source->lastKnownPosition(true);
	QGeoCoordinate qgeo_coord = pos_info.coordinate();
	if (!qgeo_coord.isValid())
	{
		ok = false;
		return latest_gps_coord;
	}
	
	LatLon latlon(qgeo_coord.latitude(), qgeo_coord.longitude(), true);
	latest_gps_coord = georeferencing.toMapCoordF(latlon, &ok);
	if (!ok)
	{
		qDebug() << "GPSDisplay::getLatestGPSCoord(): Cannot convert LatLon to MapCoordF!";
		return latest_gps_coord;
	}
	
	gps_updated = false;
	ok = true;
	return latest_gps_coord;
#else
	ok = false;
	return latest_gps_coord;
#endif
}

void GPSDisplay::updateMapWidget()
{
	// TODO: Limit update region to union of old and new bounding rect
	widget->update();
}
