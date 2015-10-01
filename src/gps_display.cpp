/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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

#if defined(QT_POSITIONING_LIB)
#  include <QtPositioning/QGeoPositionInfoSource>
#endif
#if defined(Q_OS_ANDROID)
#  include <jni.h>
#  include <QtAndroidExtras/QAndroidJniObject>
#endif
#include <QPainter>
#include <QDebug>
#include <qmath.h>
#include <QTimer>

#include "core/georeferencing.h"
#include "map_widget.h"
#include "util.h"
#include "compass.h"

GPSDisplay::GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing)
 : QObject()
 , visible(false)
 , source(NULL)
 , widget(widget)
 , georeferencing(georeferencing)
{
	gps_updated = false;
	tracking_lost = false;
	has_valid_position = false;
	
	distance_rings_enabled = false;
	heading_indicator_enabled = false;
	
#if defined(QT_POSITIONING_LIB)
	source = QGeoPositionInfoSource::createDefaultSource(this);
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
#elif defined(MAPPER_DEVELOPMENT_BUILD)
	// DEBUG
	QTimer* debug_timer = new QTimer(this);
	connect(debug_timer, SIGNAL(timeout()), this, SLOT(debugPositionUpdate()));
	debug_timer->start(500);
	visible = true;
#endif

	widget->setGPSDisplay(this);
}

GPSDisplay::~GPSDisplay()
{
	stopUpdates();
	widget->setGPSDisplay(NULL);
}

bool GPSDisplay::checkGPSEnabled()
{
#if defined(Q_OS_ANDROID)
	static bool translation_initialized = false;
	if (!translation_initialized)
	{
		QAndroidJniObject gps_disabled_string = QAndroidJniObject::fromString(tr("GPS is disabled in the device settings. Open settings now?"));
		QAndroidJniObject yes_string = QAndroidJniObject::fromString(tr("Yes"));
		QAndroidJniObject no_string  = QAndroidJniObject::fromString(tr("No"));
		QAndroidJniObject::callStaticMethod<void>(
			"org/openorienteering/mapper/MapperActivity",
			"setTranslatableStrings",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
			yes_string.object<jstring>(),
			no_string.object<jstring>(),
			gps_disabled_string.object<jstring>());
		translation_initialized = true;
	}
	
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
                                       "checkGPSEnabled",
                                       "()V");
#endif
	return true;
}

void GPSDisplay::startUpdates()
{
#if defined(QT_POSITIONING_LIB)
	if (source)
	{
		checkGPSEnabled();
		source->startUpdates();
	}
#endif
}

void GPSDisplay::stopUpdates()
{
#if defined(QT_POSITIONING_LIB)
	if (source)
	{
		source->stopUpdates();
		has_valid_position = false;
	}
#endif
}

void GPSDisplay::setVisible(bool visible)
{
	if (this->visible != visible)
	{
		this->visible = visible;
		updateMapWidget();
	}
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
		
		float distance_ring_radius_pixels = widget->getMapView()->lengthToPixel(100000.0 * distance_ring_radius_meters / georeferencing.getScaleDenominator());
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
		float accuracy_pixels = widget->getMapView()->lengthToPixel(1000000.0 * accuracy_meters / georeferencing.getScaleDenominator());
		
		painter->setPen(QPen(tracking_lost ? Qt::gray : Qt::red, Util::mmToPixelLogical(0.2f)));
		painter->setBrush(Qt::NoBrush);
		painter->drawEllipse(gps_pos, accuracy_pixels, accuracy_pixels);
	}
}

#if defined(QT_POSITIONING_LIB)
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
			emit latLonUpdated(latLon.latitude(), latLon.longitude(), altitude, accuracy);
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
#if defined(QT_POSITIONING_LIB)
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
	
	LatLon latlon(qgeo_coord.latitude(), qgeo_coord.longitude());
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
