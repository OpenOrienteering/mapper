/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_TRACK_H
#define OPENORIENTEERING_TEMPLATE_TRACK_H

#include <vector>

#include <QtGlobal>
#include <QObject>
#include <QRectF>
#include <QString>

#include "core/track.h"
#include "templates/template.h"

class QByteArray;
class QPainter;
class QRectF;
class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Georeferencing;
class Map;
class MapCoordF;
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
    ~TemplateTrack() override;
	const char* getTemplateType() const override {return "TemplateTrack";}
	bool isRasterGraphics() const override {return false;}
	
	bool saveTemplateFile() const override;
	
	bool loadTemplateFileImpl(bool configuring) override;
	bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view) override;
	void unloadTemplateFileImpl() override;
	
    void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const override;
	QRectF getTemplateExtent() const override;
    QRectF calculateTemplateBoundingBox() const override;
    int getTemplateBoundingBoxPixelBorder() override;
	
	bool hasAlpha() const override;
	
	/// Draws all tracks.
	void drawTracks(QPainter* painter, bool on_screen) const;
	
	/// Draws all waypoints.
	void drawWaypoints(QPainter* painter) const;
	
	/// Import the track as map object(s), returns true if something has been imported.
	/// TODO: should this be moved to the Track class?
	bool import(QWidget* dialog_parent = nullptr);
	
	/// Replaces the calls to pre/postLoadConfiguration() if creating a new GPS track.
	/// Assumes that the map's georeferencing is valid.
	void configureForGPSTrack();
	
	/// Returns the Track data object.
	inline Track& getTrack() {return track;}
	
public slots:
	void updateGeoreferencing();
	
protected:
	Template* duplicateImpl() const override;
	
    void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
    bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;
	
	/// Projects the track in non-georeferenced mode
	QString calculateLocalGeoreferencing() const;
	
	void applyProjectedCrsSpec();
	
	PathObject* importPathStart();
	void importPathEnd(PathObject* path);
	PointObject* importWaypoint(const MapCoordF& position, const QString &name = QString());
	
	
	Track track;
	QString track_crs_spec;
	QString projected_crs_spec;
	friend class OgrTemplate; // for migration
	std::unique_ptr<Georeferencing> preserved_georef;
	
private:
	Q_DISABLE_COPY(TemplateTrack)
};


}  // namespace OpenOrienteering

#endif
