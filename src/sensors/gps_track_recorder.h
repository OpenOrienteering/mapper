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


#ifndef OPENORIENTEERING_GPS_TRACK_RECORDER_H
#define OPENORIENTEERING_GPS_TRACK_RECORDER_H

#include <QObject>
#include <QTimer>

namespace OpenOrienteering {

class MapWidget;
class Template;
class TemplateTrack;
class GPSDisplay;


/** Records GPS tracks into a TemplateTrack. */
class GPSTrackRecorder : public QObject
{
Q_OBJECT
public:
	GPSTrackRecorder(GPSDisplay* gps_display, TemplateTrack* target_template, int draw_update_interval_milliseconds = -1, MapWidget* widget = nullptr);

public slots:
	void newPosition(double latitude, double longitude, double altitude, float accuracy);
	void positionUpdatesInterrupted();
	void templateDeleted(int pos, const OpenOrienteering::Template* old_temp);
	void drawUpdate();
	
private:
	GPSDisplay* gps_display;
	TemplateTrack* target_template;
	MapWidget* widget;
	QTimer draw_update_timer;
	bool track_changed_since_last_update;
	bool is_active;
};


}  // namespace OpenOrienteering

#endif
