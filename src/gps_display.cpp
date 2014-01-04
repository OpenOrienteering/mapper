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

#if defined(ENABLE_POSITIONING)
	#include <QtPositioning/QGeoPositionInfoSource>
#endif
#include <QPainter>
#include <QDebug>
#include <qmath.h>
#include <QTimer>

#include "map_widget.h"
#include "georeferencing.h"
#include "util.h"
#include "compass.h"
#if defined(ANDROID)
	#include "gps_source_android.h"
#endif


GPSDisplay::GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing)
 : QObject()
 , georeferencing(georeferencing)
{
	this->widget = widget;
	
	gps_updated = false;
	tracking_lost = false;
	has_valid_position = false;
	
	distance_rings_enabled = false;
	heading_indicator_enabled = false;
	
#if defined(ENABLE_POSITIONING)
	#if defined(ANDROID)
		source = new AndroidGPSPositionSource(this);
	#else
		source = QGeoPositionInfoSource::createDefaultSource(this);
	#endif
	if (!source)
	{
		qDebug() << "Cannot create QGeoPositionInfoSource!";
		return;
	}
	
	source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
	source->setUpdateInterval(1000);
	connect(source, SIGNAL(positionUpdated(const QGeoPositionInfo&)), this, SLOT(positionUpdated(const QGeoPositionInfo&)), Qt::QueuedConnection);
	connect(source, SIGNAL(error(QGeoPositionInfoSource::Error)), this, SLOT(error(QGeoPositionInfoSource::Error)));
	connect(source, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
#else
	source = NULL;
	
	// DEBUG
	QTimer* debug_timer = new QTimer(this);
	connect(debug_timer, SIGNAL(timeout()), this, SLOT(debugPositionUpdate()));
	debug_timer->start(500);
#endif

	widget->setGPSDisplay(this);
}

GPSDisplay::~GPSDisplay()
{
	stopUpdates();
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
	has_valid_position = false;
#endif
}

void GPSDisplay::setVisible(bool visible)
{
	if (this->visible == visible)
		return;
	this->visible = visible;
	
	updateMapWidget();
}

void GPSDisplay::enableDistanceRings(bool enable)
{
	distance_rings_enabled = enable;
	if (visible && has_valid_position)
		updateMapWidget();
}

void GPSDisplay::enableHeadingIndicator(bool enable)
{
	if (enable && ! heading_indicator_enabled)
		Compass::getInstance().startUsage();
	else if (! enable && heading_indicator_enabled)
		Compass::getInstance().stopUsage();
	
	heading_indicator_enabled = enable;
}

void GPSDisplay::paint(QPainter* painter)
{
	if (!visible || !has_valid_position)
		return;
	
	// Get GPS position on map widget
	bool ok = true;
	MapCoordF gps_coord = calcLatestGPSCoord(ok);
	if (!ok)
		return;
	QPointF gps_pos = widget->mapToViewport(gps_coord);
	
	// Draw center dot or arrow
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(tracking_lost ? Qt::gray : Qt::red));
	if (heading_indicator_enabled)
	{
		const qreal arrow_length = Util::mmToPixelLogical(1.5f);
		const qreal heading_indicator_length = Util::mmToPixelLogical(9999.0f); // very long

		// For heading indicator, get azimuth from compass and calculate
		// the relative rotation to map view rotation, clockwise.
		float heading_rotation_deg = Compass::getInstance().getCurrentAzimuth() + 180.0f / M_PI * widget->getMapView()->getRotation();
		
		painter->save();
		painter->translate(gps_pos);
		painter->rotate(heading_rotation_deg);
		
		// Draw arrow
		static const QPointF arrow_points[4] = {
			QPointF(0, -arrow_length),
			QPointF(0.4f * arrow_length, 0.4f * arrow_length),
			QPointF(0, 0),
			QPointF(-0.4f * arrow_length, 0.4f * arrow_length)
		};
		painter->drawPolygon(arrow_points, 4);
		
		// Draw heading line
		painter->setPen(QPen(Qt::gray, Util::mmToPixelLogical(0.1f)));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), QPointF(0, -1 * heading_indicator_length));
		
		painter->restore();
	}
	else
	{
		const qreal dot_radius = Util::mmToPixelLogical(0.5f);
		painter->drawEllipse(gps_pos, dot_radius, dot_radius);
	}
	
	// Draw distance circles
	if (distance_rings_enabled)
	{
		const int num_distance_rings = 2;
		const float distance_ring_radius_meters = 10;
		
		float distance_ring_radius_pixels = widget->getMapView()->lengthToPixel(1000*1000 * distance_ring_radius_meters / georeferencing.getScaleDenominator());
		painter->setPen(QPen(Qt::gray, Util::mmToPixelLogical(0.1f)));
		painter->setBrush(Qt::NoBrush);
		for (int i = 0; i < num_distance_rings; ++ i)
		{
			float radius = (i + 1) * distance_ring_radius_pixels;
			painter->drawEllipse(gps_pos, radius, radius);
		}
	}
	
	// Draw accuracy circle
	if (latest_gps_coord_accuracy >= 0)
	{
		float accuracy_meters = latest_gps_coord_accuracy;
		float accuracy_pixels = widget->getMapView()->lengthToPixel(1000*1000 * accuracy_meters / georeferencing.getScaleDenominator());
		
		painter->setPen(QPen(tracking_lost ? Qt::gray : Qt::red, Util::mmToPixelLogical(0.2f)));
		painter->setBrush(Qt::NoBrush);
		painter->drawEllipse(gps_pos, accuracy_pixels, accuracy_pixels);
	}
}

#if defined(ENABLE_POSITIONING)
void GPSDisplay::positionUpdated(const QGeoPositionInfo& info)
{
	Q_UNUSED(info);

	gps_updated = true;
	tracking_lost = false;
	has_valid_position = true;
	
	bool ok = false;
	calcLatestGPSCoord(ok);
	if (ok)
	{
		emit mapPositionUpdated(latest_gps_coord, latest_gps_coord_accuracy);
		emit latLonUpdated(
			info.coordinate().latitude(),
			info.coordinate().longitude(),
			(info.coordinate().type() == QGeoCoordinate::Coordinate3D) ? info.coordinate().altitude() : -9999,
			latest_gps_coord_accuracy
		);
	}
	
	updateMapWidget();
}

void GPSDisplay::error(QGeoPositionInfoSource::Error positioningError)
{
	if (positioningError != QGeoPositionInfoSource::NoError)
	{
		if (!tracking_lost)
		{
			tracking_lost = true;
			emit positionUpdatesInterrupted();
			updateMapWidget();
		}
	}
}

void GPSDisplay::updateTimeout()
{
	// Lost satellite fix
	if (!tracking_lost)
	{
		tracking_lost = true;
		emit positionUpdatesInterrupted();
		updateMapWidget();
	}
}
#endif

void GPSDisplay::debugPositionUpdate()
{
#if QT_VERSION >= 0x050200
	if (! visible)
		return;
	
	QTime now = QTime::currentTime();
	float offset = now.msecsSinceStartOfDay() / (float)(10 * 1000);
	float accuracy = 12 + 7 * qSin(2 + offset);
	float altitude = 400 + 10 * qSin(1 + 0.1f * offset);
	
	MapCoordF coord(30 * qSin(0.5f * offset), 30 * qCos(0.53f * offset));
	emit mapPositionUpdated(coord, accuracy);
	
	if (georeferencing.isValid() && ! georeferencing.isLocal())
	{
		bool ok;
		LatLon latLon = georeferencing.toGeographicCoords(coord, &ok);
		if (ok)
		{
			emit latLonUpdated(latLon.getLatitudeInDegrees(), latLon.getLongitudeInDegrees(), altitude, accuracy);
		}
	}
	
	gps_updated = true;
	tracking_lost = false;
	has_valid_position = true;
	latest_gps_coord = coord;
	latest_gps_coord_accuracy = accuracy;
	updateMapWidget();
#endif
}

MapCoordF GPSDisplay::calcLatestGPSCoord(bool& ok)
{
#if defined(ENABLE_POSITIONING)
	if (!has_valid_position)
	{
		ok = false;
		return latest_gps_coord;
	}
	if (!gps_updated)
	{
		ok = true;
		return latest_gps_coord;
	}
	
	latest_pos_info = source->lastKnownPosition(true);
	latest_gps_coord_accuracy = latest_pos_info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) ? latest_pos_info.attribute(QGeoPositionInfo::HorizontalAccuracy) : -1;
	
	QGeoCoordinate qgeo_coord = latest_pos_info.coordinate();
	if (!qgeo_coord.isValid())
	{
		ok = false;
		return latest_gps_coord;
	}
	
	LatLon latlon(qgeo_coord.latitude(), qgeo_coord.longitude(), true);
	latest_gps_coord = georeferencing.toMapCoordF(latlon, &ok);
	if (!ok)
	{
		qDebug() << "GPSDisplay::calcLatestGPSCoord(): Cannot convert LatLon to MapCoordF!";
		return latest_gps_coord;
	}
	
	gps_updated = false;
	ok = true;
	return latest_gps_coord;
#else
	ok = has_valid_position;
	return latest_gps_coord;
#endif
}

void GPSDisplay::updateMapWidget()
{
	// TODO: Limit update region to union of old and new bounding rect
	widget->update();
}
