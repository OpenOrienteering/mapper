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


#ifndef _OPENORIENTEERING_TEMPLATE_GPS_H_
#define _OPENORIENTEERING_TEMPLATE_GPS_H_

#include "template.h"
#include "gps_track.h"

class MapEditorController;

/// A GPS track + waypoints used as template
class TemplateGPS : public Template
{
Q_OBJECT
public:
	TemplateGPS(const QString& filename, Map* map, MapEditorController *controller);
	TemplateGPS(const TemplateGPS& other);
    virtual ~TemplateGPS();
    virtual Template* duplicate();
	virtual const QString getTemplateType() {return "TemplateGPS";}
    virtual bool saveTemplateFile();
	
    virtual bool open(QWidget* dialog_parent, MapView* main_view);
    virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity);
    virtual void drawTemplateUntransformed(QPainter* painter, const QRect& clip_rect, MapWidget* widget);
    virtual QRectF getExtent();
	
    virtual double getTemplateFinalScaleX() const;
    virtual double getTemplateFinalScaleY() const;

	bool import(MapEditorController *controller);
	
public slots:
	void gpsProjectionParametersChanged();
	
protected:
	void calculateExtent();
	virtual bool changeTemplateFileImpl(const QString& filename);
	
	GPSTrack track;
	MapEditorController *controller;
};

#endif
