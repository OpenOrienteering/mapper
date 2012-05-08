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
#include "map_undo.h"
#include "object.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "georeferencing_dialog.h"

TemplateGPS::TemplateGPS(const QString& filename, Map* map)
: Template(filename, map)
{
	if (!track.loadFrom(filename, false))
		return;

	const Georeferencing& georef = map->getGeoreferencing();
	track.changeGeoreferencing(georef); // TODO: check if this can be moved to inititalization
	if (!georef.isLocal())
		calculateExtent();
	
	connect(&georef, SIGNAL(projectionChanged()), this, SLOT(updateGeoreferencing()));
	connect(&georef, SIGNAL(transformationChanged()), this, SLOT(updateGeoreferencing()));
	
	template_valid = true;
}

TemplateGPS::TemplateGPS(const TemplateGPS& other) : Template(other)
{
	track = GPSTrack(other.track);

	const Georeferencing& georef = map->getGeoreferencing();
	connect(&georef, SIGNAL(projectionChanged()), this, SLOT(updateGeoreferencing()));
	connect(&georef, SIGNAL(transformationChanged()), this, SLOT(updateGeoreferencing()));
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
	if (map->getGeoreferencing().isLocal())
	{
		// Set default for plane center as some average of the track coordinates
		double avg_latitude = 0;
		double avg_longitude = 0;
		int num_samples = 0;
		
		int size = track.getNumWaypoints();
		for (int i = 0; i < size; ++i)
		{
			const GPSPoint& point = track.getWaypoint(i);
			avg_latitude += point.gps_coord.latitude;
			avg_longitude += point.gps_coord.longitude;
			++num_samples;
		}
		for (int i = 0; i < track.getNumSegments(); ++i)
		{
			size = track.getSegmentPointCount(i);
			for (int k = 0; k < size; ++k)
			{
				const GPSPoint& point = track.getSegmentPoint(i, k);
				avg_latitude += point.gps_coord.latitude;
				avg_longitude += point.gps_coord.longitude;
				++num_samples;
			}
		}
		
		double center_latitude = (num_samples > 0) ? (avg_latitude / num_samples) : 0;
		double center_longitude = (num_samples > 0) ? (avg_longitude / num_samples) : 0;
		
		Georeferencing georef(map->getGeoreferencing());
		georef.setGeographicRefPoint(LatLon(center_latitude, center_longitude));
		
		// Show the parameter dialog
		GeoreferencingDialog dialog(dialog_parent, *map, &georef);
		dialog.setWindowModality(Qt::WindowModal);
		if (dialog.exec() == QDialog::Rejected)
			return false;
		
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
			const GPSPoint& point = track.getSegmentPoint(i, k);
			
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
		const GPSPoint& point = track.getWaypoint(i);
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

PathObject* TemplateGPS::importPathStart()
{
	PathObject* path = new PathObject();
	path->setSymbol(map->getUndefinedLine(), true);
	return path;
}

void TemplateGPS::importPathEnd(PathObject* path)
{
	map->addObject(path);
	map->addObjectToSelection(path, false);
}

PointObject* TemplateGPS::importWaypoint(const MapCoordF& position)
{
	PointObject* point = new PointObject(map->getUndefinedPoint());
	point->setPosition(position);
	map->addObject(point);
	map->addObjectToSelection(point, false);
	return point;
}

bool TemplateGPS::import(QWidget* dialog_parent)
{
	if (track.getNumWaypoints() == 0 && track.getNumSegments() == 0)
	{
		QMessageBox::critical(dialog_parent, tr("Error"), tr("The path is empty, there is nothing to import!"));
		return false;
	}
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapLayer* layer = map->getCurrentLayer();
	std::vector< Object* > result;
	
	map->clearObjectSelection(false);
	
	if (track.getNumWaypoints() > 0)
	{
		int res = QMessageBox::question(dialog_parent, tr("Question"), tr("Should the waypoints be imported as a line going through all points?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (res == QMessageBox::No)
		{
			for (int i = 0; i < track.getNumWaypoints(); i++)
				result.push_back(importWaypoint(templateToMap(track.getWaypoint(i).map_coord)));
		}
		else
		{
			PathObject* path = importPathStart();
			for (int i = 0; i < track.getNumWaypoints(); i++)
				path->addCoordinate(templateToMap(track.getWaypoint(i).map_coord).toMapCoord());
			importPathEnd(path);
			result.push_back(path);
		}
	}
	
	for (int i = 0; i < track.getNumSegments(); i++)
	{
		PathObject* path = importPathStart();
		for (int j = 0; j < track.getSegmentPointCount(i); j++)
			path->addCoordinate(templateToMap(track.getSegmentPoint(i, j).map_coord).toMapCoord());
		importPathEnd(path);
		result.push_back(path);
	}
	
	for (int i = 0; i < (int)result.size(); ++i) // keep as separate loop to get the correct order
		undo_step->addObject(layer->findObjectIndex(result[i]));
	
	map->objectUndoManager().addNewUndoStep(undo_step);
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();		// TODO: is this necessary here?
	
	return true;
}

void TemplateGPS::updateGeoreferencing()
{
	track.changeGeoreferencing(map->getGeoreferencing());
	// TODO: redraw template
}

void TemplateGPS::calculateExtent()
{
	// TODO: could try to calculate an extent here, but if e.g. waypoint names are displayed this is not possible with the current possibility to specify the extent in template coordinates
}
bool TemplateGPS::changeTemplateFileImpl(const QString& filename)
{
	if (!track.loadFrom(filename, false))
		return false;
	track.changeGeoreferencing(map->getGeoreferencing());
	calculateExtent();
	return true;
}

#include "template_gps.moc"
