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

QT_BEGIN_NAMESPACE
class QRadioButton;
class QDoubleSpinBox;
class QDialogButtonBox;
QT_END_NAMESPACE

class PathObject;
class PointObject;

/// A template consisting of a set of tracks (polylines) and waypoints
class TemplateTrack : public Template
{
Q_OBJECT
public:
	TemplateTrack(const QString& path, Map* map);
    virtual ~TemplateTrack();
	virtual const QString getTemplateType() {return "TemplateTrack";}
	
	virtual bool saveTemplateFile();
	
	virtual bool loadTemplateFileImpl(bool configuring);
	virtual bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view);
	virtual void unloadTemplateFileImpl();
	
	virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity);
	virtual QRectF getTemplateExtent();
    virtual QRectF calculateTemplateBoundingBox();
    virtual int getTemplateBoundingBoxPixelBorder();
	
	
	/// Draws all tracks.
	void drawTracks(QPainter* painter);
	
	/// Draws all waypoints. Needs the transformation from map coords to paint device coords.
	void drawWaypoints(QPainter* painter, QTransform map_to_device);
	
	/// Import the track as map object(s), returns true if something has been imported.
	/// TODO: should this be moved to the Track class?
	bool import(QWidget* dialog_parent = NULL);
	
public slots:
	void updateGeoreferencing();
	
protected:
	virtual Template* duplicateImpl();
    virtual void saveTypeSpecificTemplateConfiguration(QIODevice* stream);
    virtual bool loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version);
    virtual void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml);
    virtual bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml);
	
	/// Projects the track in non-georeferenced mode
	void calculateLocalGeoreferencing();
	
	PathObject* importPathStart();
	void importPathEnd(PathObject* path);
	PointObject* importWaypoint(const MapCoordF& position);
	
	
	Track track;
	QString track_crs_spec;
};

class LocalCRSPositioningDialog : public QDialog
{
Q_OBJECT
public:
	LocalCRSPositioningDialog(TemplateTrack* temp, QWidget* parent = NULL);
	
	double getUnitScale() const;
	bool centerOnView() const;
	
private:
	QDoubleSpinBox* unit_scale_edit;
	QRadioButton* original_pos_radio;
	QRadioButton* view_center_radio;
	QDialogButtonBox* button_box;
};

#endif
