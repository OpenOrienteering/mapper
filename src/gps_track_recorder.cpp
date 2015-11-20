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


#include "gps_track_recorder.h"

#include "gps_display.h"
#include "template_track.h"
#include "map.h"
#include "map_widget.h"


GPSTrackRecorder::GPSTrackRecorder(GPSDisplay* gps_display, TemplateTrack* target_template, int draw_update_interval_milliseconds, MapWidget* widget)
 : QObject()
{
	this->gps_display = gps_display;
	this->target_template = target_template;
	this->widget = widget;
	
	track_changed_since_last_update = false;
	is_active = true;
	
	// Start with a new segment
	target_template->getTrack().finishCurrentSegment();
	
	connect(gps_display, SIGNAL(latLonUpdated(double,double,double,float)), this, SLOT(newPosition(double,double,double,float)));
	connect(gps_display, SIGNAL(positionUpdatesInterrupted()), this, SLOT(positionUpdatesInterrupted()));
	connect(target_template->getMap(), SIGNAL(templateDeleted(int,Template*)), this, SLOT(templateDeleted(int,Template*)));
	
	if (draw_update_interval_milliseconds > 0)
	{
		connect(&draw_update_timer, SIGNAL(timeout()), this, SLOT(drawUpdate()));
		draw_update_timer.start(draw_update_interval_milliseconds);
	}
}

void GPSTrackRecorder::newPosition(double latitude, double longitude, double altitude, float accuracy)
{
	TrackPoint new_point(
		LatLon(latitude, longitude),
		QDateTime::currentDateTime(),
		altitude,
		-1,
		accuracy
	);
	target_template->getTrack().appendTrackPoint(new_point);
	target_template->setHasUnsavedChanges(true);
	track_changed_since_last_update = true;
}

void GPSTrackRecorder::positionUpdatesInterrupted()
{
	target_template->getTrack().finishCurrentSegment();
	target_template->setHasUnsavedChanges(true);
	track_changed_since_last_update = true;
}

void GPSTrackRecorder::templateDeleted(int pos, Template* old_temp)
{
	Q_UNUSED(pos);
	if (!is_active)
		return;
	
	if (old_temp == target_template)
	{
		// Deactivate
		gps_display->disconnect(this);
		draw_update_timer.stop();
		is_active = false;
	}
}

void GPSTrackRecorder::drawUpdate()
{
	if (!is_active)
		return;
	
	if (track_changed_since_last_update)
	{
		if (widget->getMapView()->isTemplateVisible(target_template))
			target_template->setTemplateAreaDirty();
		
		track_changed_since_last_update = false;
	}
}
