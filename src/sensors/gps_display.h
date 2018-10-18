/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2016, 2018 Kai Pastor
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


#ifndef OPENORIENTEERING_GPS_DISPLAY_H
#define OPENORIENTEERING_GPS_DISPLAY_H

#include <QtGlobal>
#include <QObject>
#if defined(QT_POSITIONING_LIB)
#  include <QGeoPositionInfo>  // IWYU pragma: keep
#  include <QGeoPositionInfoSource>  // IWYU pragma: keep
#else
class QGeoPositionInfoSource;  // IWYU pragma: keep
#endif

#include "core/map_coord.h"

class QPainter;
class QTimerEvent;

namespace OpenOrienteering {

class Georeferencing;
class MapWidget;


/**
 * Retrieves the GPS position and displays a marker at this position on a MapWidget.
 * 
 * \todo Use qreal instead of float (in all sensor code) for consistency with Qt.
 */
class GPSDisplay : public QObject
{
Q_OBJECT
public:
	/// Creates a GPS display for the given map widget and georeferencing.
	GPSDisplay(MapWidget* widget, const Georeferencing& georeferencing, QObject* parent = nullptr);
	/// Destructor, removes the GPS display from the map widget.
	~GPSDisplay() override;
	
	/**
	 * Checks if GPS is enabled, and may guide the user to the device settings.
	 * 
	 * If GPS is not enabled in the device settings, it asks the user whether he
	 * wishes to open the device's location settings dialog.
	 * (At the moment, this is implemented for Android only.)
	 * 
	 * Returns true if GPS is enabled, but also when the settings dialog remains
	 * open when returning from this function and the final setting is unknown.
	 */
	bool checkGPSEnabled();
	
	/// Starts regular position updates. This will issue redraws of the map widget.
	void startUpdates();
	/// Stops regular position updates.
	void stopUpdates();
	
	/// Sets GPS marker visibility (true by default)
	void setVisible(bool visible);
	/// Returns GPS marker visibility
	bool isVisible() const { return visible; }
	
	/// Sets whether distance rings are drawn
	void enableDistanceRings(bool enable);
	/// Sets whether the current heading from the Compass is used to draw a heading indicator.
	void enableHeadingIndicator(bool enable);
	
	/// This is called from the MapWidget drawing code to draw the GPS position marker.
	void paint(QPainter* painter);
	
	/// Returns if a valid position was received since the last call to startUpdates().
	bool hasValidPosition() const { return has_valid_position; }
	/// Returns the latest received GPS coord. Check hasValidPosition() beforehand!
	const MapCoordF& getLatestGPSCoord() const { return latest_gps_coord; }
	/// Returns the accuracy of the latest received GPS coord, or -1 if unknown. Check hasValidPosition() beforehand!
	float getLatestGPSCoordAccuracy() const { return latest_gps_coord_accuracy; }
	
	/// Starts quick blinking for one or more seconds.
	void startBlinking(int seconds);
	
	/// Stops blinking.
	void stopBlinking();
	
	/// Returns true while blinking is active.
	bool isBlinking() const { return blink_count > 0; }
	
protected:
	/// Handles blinking.
	void timerEvent(QTimerEvent* e) override;
	
signals:
	/// Is emitted whenever a new position update happens.
	/// If the accuracy is unknown, -1 will be given.
	void mapPositionUpdated(const MapCoordF& coord, float accuracy);
	
	/// Like mapPositionUpdated(), but gives the values as
	/// latitude / longitude in degrees and also gives altitude
	/// (meters above sea level; -9999 is unknown)
	void latLonUpdated(double latitude, double longitude, double altitude, float accuracy);
	
	/// Is emitted when updates are interrupted after previously being active,
	/// due to loss of satellite reception or another error such as the user
	/// turning off the GPS receiver.
	void positionUpdatesInterrupted();
	
private slots:
#if defined(QT_POSITIONING_LIB)
    void positionUpdated(const QGeoPositionInfo& info);
	void error(QGeoPositionInfoSource::Error positioningError);
	void updateTimeout();
#endif
	void debugPositionUpdate();
	
private:
	MapCoordF calcLatestGPSCoord(bool& ok);
	void updateMapWidget();
	
	/**
	 * A lightweight utility for sinusoidal pulsating opacity.
	 * 
	 * This class depends on another object's QObject::timerEvent() override
	 * to advance the pulsation state and eventually stop the activity.
	 * 
	 * \see QBasicTimer
	 */
	class PulsatingOpacity
	{
	public:
		/// Returns true when the pulsation is active.
		bool isActive() const { return bool(timer_id); }
		/// Starts a timer on the given object.
		void start(QObject& object);
		/// Stops the timer on the given object.
		void stop(QObject& object);
		/// Returns the ID of the active timer.
		int timerId() const { return timer_id; }
		
		/// Advances the pulsation state.
		bool advance();
		/// Returns the current opacity.
		qreal current() const;
		
	private:
		int timer_id = 0;
		quint8 index = 0;
	};
	
	MapWidget* widget;
	const Georeferencing& georeferencing;
	QGeoPositionInfoSource* source = nullptr;
#if defined(QT_POSITIONING_LIB)
	QGeoPositionInfo latest_pos_info;
#endif
	MapCoordF latest_gps_coord;
	float latest_gps_coord_accuracy = 0;
	PulsatingOpacity pulsating_opacity;
	int blink_count = 0;
	bool tracking_lost             = false;
	bool has_valid_position        = false;
	bool gps_updated               = false;
	bool visible                   = false;
	bool distance_rings_enabled    = false;
	bool heading_indicator_enabled = false;
};


}  // namespace OpenOrienteering

#endif
