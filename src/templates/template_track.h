/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_TEMPLATE_TRACK_H_
#define _OPENORIENTEERING_TEMPLATE_TRACK_H_

#include "template.h"

#include <QDialog>

#include "sensors/gps_track.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QRadioButton;
QT_END_NAMESPACE

class PathObject;
class PointObject;

/** A template consisting of a set of tracks (polylines) and waypoints */
class TemplateTrack : public Template
{
Q_OBJECT
public:
	/**
	 * Returns the filename extensions supported by this template class.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	TemplateTrack(const QString& path, Map* map);
    virtual ~TemplateTrack();
	virtual const char* getTemplateType() const {return "TemplateTrack";}
	virtual bool isRasterGraphics() const {return false;}
	
	virtual bool saveTemplateFile() const;
	
	virtual bool loadTemplateFileImpl(bool configuring);
	virtual bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view);
	virtual void unloadTemplateFileImpl();
	
    virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, bool on_screen, float opacity) const;
	virtual QRectF getTemplateExtent() const;
    virtual QRectF calculateTemplateBoundingBox() const;
    virtual int getTemplateBoundingBoxPixelBorder();
	
	
	/// Draws all tracks.
	void drawTracks(QPainter* painter, bool on_screen) const;
	
	/// Draws all waypoints.
	void drawWaypoints(QPainter* painter) const;
	
	/// Import the track as map object(s), returns true if something has been imported.
	/// TODO: should this be moved to the Track class?
	bool import(QWidget* dialog_parent = NULL);
	
	/// Replaces the calls to pre/postLoadConfiguration() if creating a new GPS track.
	/// Assumes that the map's georeferencing is valid.
	void configureForGPSTrack();
	
	/// Returns the Track data object.
	inline Track& getTrack() {return track;}
	
public slots:
	void updateGeoreferencing();
	
protected:
	virtual Template* duplicateImpl() const;
    virtual bool loadTypeSpecificTemplateConfiguration(QIODevice* stream, int version);
    virtual void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const;
    virtual bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml);
	
	/// Projects the track in non-georeferenced mode
	void calculateLocalGeoreferencing();
	
	PathObject* importPathStart();
	void importPathEnd(PathObject* path);
	PointObject* importWaypoint(const MapCoordF& position, const QString &name = QString());
	
	
	Track track;
	QString track_crs_spec;
};

/**
 * Dialog allowing for local positioning of a track (without georeferencing).
 */
class LocalCRSPositioningDialog : public QDialog
{
Q_OBJECT
public:
	LocalCRSPositioningDialog(TemplateTrack* temp, QWidget* parent = NULL);
	
	bool useRealCoords() const;
	double getUnitScale() const;
	bool centerOnView() const;
	
private:
	QComboBox* coord_system_box;
	QDoubleSpinBox* unit_scale_edit;
	QRadioButton* original_pos_radio;
	QRadioButton* view_center_radio;
	QDialogButtonBox* button_box;
};

#endif
