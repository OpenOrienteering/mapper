/*
 *    Copyright 2012 Thomas Schöps
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


#include "template_gps.h"

#include <QPainter>
#include <QMessageBox>

#include "map_widget.h"
#include "map_editor.h"
#include "georeferencing.h"

TemplateGPS::TemplateGPS(const QString& filename, Map* map, MapEditorController *controller) : Template(filename, map), controller(controller)
{
	if (!track.loadFrom(filename, false, controller))
		return;
	
	if (map->areGPSProjectionParametersSet())
	{
		track.changeProjectionParams(map->getGPSProjectionParameters());
		calculateExtent();
	}
	
	connect(map, SIGNAL(gpsProjectionParametersChanged()), this, SLOT(gpsProjectionParametersChanged()));
	
	template_valid = true;
}
TemplateGPS::TemplateGPS(const TemplateGPS& other) : Template(other)
{
	track = GPSTrack(other.track);
}
TemplateGPS::~TemplateGPS()
{
}

Template* TemplateGPS::duplicate()
{
	TemplateGPS* copy = new TemplateGPS(*this);
	return copy;
}

bool TemplateGPS::saveTemplateFile()
{
    return track.saveTo(template_path);
}

bool TemplateGPS::open(QWidget* dialog_parent, MapView* main_view)
{
	// If the transformation parameters were not set yet, it must be done now
	if (!map->areGPSProjectionParametersSet())
	{
		// Set default for plane center as some average of the track coordinates
		double avg_latitude = 0;
		double avg_longitude = 0;
		int num_samples = 0;
		
		int size = track.getNumWaypoints();
		for (int i = 0; i < size; ++i)
		{
			GPSPoint& point = track.getWaypoint(i);
			avg_latitude += point.gps_coord.latitude;
			avg_longitude += point.gps_coord.longitude;
			++num_samples;
		}
		for (int i = 0; i < track.getNumSegments(); ++i)
		{
			size = track.getSegmentPointCount(i);
			for (int k = 0; k < size; ++k)
			{
				GPSPoint& point = track.getSegmentPoint(i, k);
				avg_latitude += point.gps_coord.latitude;
				avg_longitude += point.gps_coord.longitude;
				++num_samples;
			}
		}
		
		GPSProjectionParameters params = map->getGPSProjectionParameters();
		params.center_latitude = (num_samples > 0) ? (avg_latitude / num_samples) : 0;
		params.center_longitude = (num_samples > 0) ? (avg_longitude / num_samples) : 0;
		
		// Show the parameter dialog
		GeoreferencingDialog dialog(dialog_parent, &params);
		dialog.setWindowModality(Qt::WindowModal);
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
		map->setGPSProjectionParameters(dialog.getParameters());	// this will call the according slot of this object to adjust the track
		calculateExtent();
	}
	
	// Check if there is already a GPS template loaded. If yes, take the transformation parameters from it
	int size = map->getNumTemplates();
	for (int i = 0; i < size; ++i)
	{
		Template* temp = map->getTemplate(i);
		if (temp->getTemplateType() == "TemplateGPS")
		{
			temp->getTransform(cur_trans);
			break;
		}
	}
	
	updateTransformationMatrices();
	return true;
}
void TemplateGPS::drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity)
{
	// Tracks
	// TODO: could speed that up by storing the template coords of the GPS points in a separate vector or caching the painter paths
	painter->setPen(qRgb(212, 0, 244));
	painter->setBrush(Qt::NoBrush);
	
	for (int i = 0; i < track.getNumSegments(); ++i)
	{
		QPainterPath path;
		int size = track.getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			GPSPoint& point = track.getSegmentPoint(i, k);
			
			if (k > 0)
				path.lineTo(point.map_coord.getX(), point.map_coord.getY());
			else
				path.moveTo(point.map_coord.getX(), point.map_coord.getY());
		}
		painter->drawPath(path);
	}
}
void TemplateGPS::drawTemplateUntransformed(QPainter* painter, const QRect& clip_rect, MapWidget* widget)
{
	// Waypoints
	painter->setRenderHint(QPainter::Antialiasing);
	
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(qRgb(255, 0, 0)));
	
	int size = track.getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		GPSPoint& point = track.getWaypoint(i);
		const QString& point_name = track.getWaypointName(i);
		
		QPointF viewport_coord = widget->mapToViewport(templateToMap(point.map_coord));
		painter->drawEllipse(viewport_coord, 2, 2);
		if (!point_name.isEmpty())
		{
			painter->setPen(qRgb(255, 0, 0));
			int width = painter->fontMetrics().width(point_name);
			painter->drawText(QRect(viewport_coord.x() - 0.5f*width,
									viewport_coord.y() - 2 - painter->fontMetrics().height(),
									width,
									painter->fontMetrics().height()),
							  Qt::AlignCenter,
							  point_name);
			painter->setPen(Qt::NoPen);
		}
	}
	
	painter->setRenderHint(QPainter::Antialiasing, false);
}
QRectF TemplateGPS::getExtent()
{
	return QRectF(-100000, -100000, 200000, 200000);	// "everything": 200x200km around the origin
}

double TemplateGPS::getTemplateFinalScaleX() const
{
	return cur_trans.template_scale_x * 1000 / map->getScaleDenominator();
}
double TemplateGPS::getTemplateFinalScaleY() const
{
	return cur_trans.template_scale_y * 1000 / map->getScaleDenominator();
}

bool TemplateGPS::import(MapEditorController *controller){
	int res = QMessageBox::question(controller->getWindow(), tr("Question"), tr("Should waypoints be treated as tracks?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if(res == QMessageBox::Yes){
		for(int i = 0; i < track.getNumWaypoints(); i++)
			controller->placePoint(track.getWaypoint(i).map_coord);
	}
	else{
		if(!controller->startNewPath())
			return false;
		for(int i = 0; i < track.getNumWaypoints(); i++)
			controller->addWayPoint(track.getWaypoint(i).map_coord.toMapCoord());
		controller->endPath();
	}
	for(int i = 0; i < track.getNumSegments(); i++){
		if(!controller->startNewPath())
			return false;
		for(int j = 0; j < track.getSegmentPointCount(i); j++)
			controller->addWayPoint(track.getSegmentPoint(i, j).map_coord.toMapCoord());
		controller->endPath();
	}
	return true;
}

void TemplateGPS::gpsProjectionParametersChanged()
{
	track.changeProjectionParams(map->getGPSProjectionParameters());
}

void TemplateGPS::calculateExtent()
{
	// TODO: could try to calculate an extent here, but if e.g. waypoint names are displayed this is not possible with the current possibility to specify the extent in template coordinates
}
bool TemplateGPS::changeTemplateFileImpl(const QString& filename)
{
	if (!track.loadFrom(filename, false, controller))
		return false;
	track.changeProjectionParams(map->getGPSProjectionParameters());
	calculateExtent();
	return true;
}

#include "template_gps.moc"
