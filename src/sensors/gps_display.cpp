/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2014, 2016, 2018 Kai Pastor
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

#if defined(Q_OS_ANDROID)
#  include <jni.h>
#  include <QAndroidJniObject>
#endif
#if defined(QT_POSITIONING_LIB)
#  include <QGeoCoordinate>
#  include <QGeoPositionInfoSource>  // IWYU pragma: keep
#endif

#include <algorithm>
#include <type_traits>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QColor>
#include <QFlags>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QTime>
#include <QTimer>  // IWYU pragma: keep
#include <QTimerEvent>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map_view.h"
#include "gui/util_gui.h"
#include "gui/map/map_widget.h"
#include "sensors/compass.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

namespace {

// Opacities as understood by QPainter::setOpacity().
static qreal opacity_curve[] = { 0.8, 1.0, 0.8, 0.5, 0.2, 0.0, 0.2, 0.5 };

}  // namespace


void GPSDisplay::PulsatingOpacity::start(QObject& object)
{
	if (!timer_id)
		timer_id = object.startTimer(1000 / std::extent<decltype(opacity_curve)>::value);
}

void GPSDisplay::PulsatingOpacity::stop(QObject& object)
{
	if (timer_id)
	{
		object.killTimer(timer_id);
		*this = {};
	}
}

bool GPSDisplay::PulsatingOpacity::advance()
{
	if (isActive())
	{
		++index;
		if (index < std::extent<decltype(opacity_curve)>::value)
			return true;
		index = 0;
	}
	return false;
}

qreal GPSDisplay::PulsatingOpacity::current() const
{
	return opacity_curve[index];
}



GPSDisplay::GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing, QObject* parent)
 : QObject(parent)
 , widget(widget)
 , georeferencing(georeferencing)
{
#if defined(QT_POSITIONING_LIB)
	source = QGeoPositionInfoSource::createDefaultSource(this);
	if (!source)
	{
		qDebug("Cannot create QGeoPositionInfoSource!");
		return;
	}
	
	source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
	source->setUpdateInterval(1000);
	connect(source, &QGeoPositionInfoSource::positionUpdated, this, &GPSDisplay::positionUpdated, Qt::QueuedConnection);
	connect(source, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error), this, &GPSDisplay::error);
	connect(source, &QGeoPositionInfoSource::updateTimeout, this, &GPSDisplay::updateTimeout);
#elif defined(MAPPER_DEVELOPMENT_BUILD)
	// DEBUG
	auto* debug_timer = new QTimer(this);
	connect(debug_timer, &QTimer::timeout, this, &GPSDisplay::debugPositionUpdate);
	debug_timer->start(500);
	visible = true;
#endif

	widget->setGPSDisplay(this);
}

GPSDisplay::~GPSDisplay()
{
	stopUpdates();
	widget->setGPSDisplay(nullptr);
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
	
	const auto one_mm = Util::mmToPixelLogical(1);
	const auto mmToPixelLogical = [one_mm](qreal mm) { return mm * one_mm; };
	
	const auto flags = painter->renderHints();
	painter->setRenderHints(flags | QPainter::Antialiasing);
	const auto opacity = painter->opacity();
	painter->setOpacity(pulsating_opacity.current() * opacity);
	
	// Highlight markers by white framing.
	const auto framing = QColor(Qt::white);
	const auto foreground = QColor(tracking_lost ? Qt::gray : Qt::red);
	
	// Draw center dot or arrow
	if (heading_indicator_enabled)
	{
		// For heading indicator, get azimuth from compass and calculate
		// the relative rotation to map view rotation, clockwise.
		const auto heading_rotation_deg = qreal(Compass::getInstance().getCurrentAzimuth())
		                                  + qRadiansToDegrees(widget->getMapView()->getRotation());
		
		painter->save();
		painter->translate(gps_pos);
		painter->rotate(heading_rotation_deg);
		painter->scale(one_mm, one_mm);
		
		// Draw arrow
		static const QPointF arrow_points[4] = {
		    { 0, -2.5 },
		    { 1, 1 },
		    { 0, 0 },
		    { -1, 1 }
		};
		painter->setPen(QPen{framing, mmToPixelLogical(0.1)});
		painter->setBrush(foreground);
		painter->drawPolygon(arrow_points, std::extent<decltype(arrow_points)>::value);
		
		// Draw heading line
		painter->setPen(QPen(Qt::gray, 0.2));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), QPointF(0, -500)); // very long
		
		painter->restore();
	}
	else
	{
		const auto dot_radius = mmToPixelLogical(0.6);
		painter->setPen(QPen{framing, mmToPixelLogical(0.3)});
		painter->setBrush(foreground);
		painter->drawEllipse(gps_pos, dot_radius, dot_radius);
		
		const auto five_mm = mmToPixelLogical(5);
		const auto ten_mm = 2 * five_mm;
		const auto draw_crosshairs = [painter, gps_pos, five_mm, ten_mm]() {
			painter->drawLine(gps_pos - QPointF{five_mm, 0}, gps_pos - QPointF{ten_mm, 0});
			painter->drawLine(gps_pos + QPointF{five_mm, 0}, gps_pos + QPointF{ten_mm, 0});
			painter->drawLine(gps_pos - QPointF{0, five_mm}, gps_pos - QPointF{0, ten_mm});
			painter->drawLine(gps_pos + QPointF{0, five_mm}, gps_pos + QPointF{0, ten_mm});
		};
		painter->setBrush(Qt::NoBrush);
		painter->setPen(QPen(framing, mmToPixelLogical(1)));
		draw_crosshairs();
		painter->setPen(QPen(foreground, mmToPixelLogical(0.5)));
		draw_crosshairs();
	}
	
	auto meters_to_pixels = widget->getMapView()->lengthToPixel(qreal(1000000) / georeferencing.getScaleDenominator());
	// Draw distance circles
	if (distance_rings_enabled)
	{
		const auto num_distance_rings = 2;
		const auto distance_ring_radius_meters = 10;
		const auto distance_ring_radius_pixels = distance_ring_radius_meters * meters_to_pixels;
		painter->setPen(QPen(Qt::gray, mmToPixelLogical(0.1)));
		painter->setBrush(Qt::NoBrush);
		auto radius = distance_ring_radius_pixels;
		for (int i = 0; i < num_distance_rings; ++i)
		{
			painter->drawEllipse(gps_pos, radius, radius);
			radius += distance_ring_radius_pixels;
		}
	}
	
	// Draw accuracy circle
	if (latest_gps_coord_accuracy >= 0)
	{
		const auto accuracy_pixels = qreal(latest_gps_coord_accuracy) * meters_to_pixels;
		painter->setBrush(Qt::NoBrush);
		painter->setPen(QPen(framing, mmToPixelLogical(1)));
		painter->drawEllipse(gps_pos, accuracy_pixels, accuracy_pixels);
		painter->setPen(QPen(foreground, mmToPixelLogical(0.5)));
		painter->drawEllipse(gps_pos, accuracy_pixels, accuracy_pixels);
	}
	
	painter->setOpacity(opacity);
	painter->setRenderHints(QPainter::Antialiasing);
}


void GPSDisplay::startBlinking(int seconds)
{
	blink_count = std::max(1, seconds);
	pulsating_opacity.start(*this);
}

void GPSDisplay::stopBlinking()
{
	blink_count = 0;
	pulsating_opacity.stop(*this);
}

void GPSDisplay::timerEvent(QTimerEvent* e)
{
	if (e->timerId() == pulsating_opacity.timerId())
	{
		if (!pulsating_opacity.advance())
		{
			--blink_count;
			if (blink_count <= 0)
				stopBlinking();
		}
		updateMapWidget();
	}
}


#if defined(QT_POSITIONING_LIB)
void GPSDisplay::positionUpdated(const QGeoPositionInfo& info)
{
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
#if MAPPER_DEVELOPMENT_BUILD
	if (!visible)
		return;
	
	QTime now = QTime::currentTime();
	const auto offset = now.msecsSinceStartOfDay() / qreal(10 * 1000);
	const auto accuracy = float(12 + 7 * qSin(2 + offset));
	const auto altitude = 400 + 10 * qSin(1 + offset / 10);
	
	const auto coord = MapCoordF(30 * qSin(0.5 * offset), 30 * qCos(0.53 * offset));
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
	latest_gps_coord_accuracy = latest_pos_info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)
	                            ? float(latest_pos_info.attribute(QGeoPositionInfo::HorizontalAccuracy))
	                            : -1;
	
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
		qDebug("GPSDisplay::calcLatestGPSCoord(): Cannot convert LatLon to MapCoordF!");
		return latest_gps_coord;
	}
	
	gps_updated = false;
	ok = true;
#else
	ok = has_valid_position;
#endif
	return latest_gps_coord;
}

void GPSDisplay::updateMapWidget()
{
	// TODO: Limit update region to union of old and new bounding rect
	widget->update();
}


}  // namespace OpenOrienteering
