/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2018 Kai Pastor
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

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVarLengthArray>

#include "core/track.h"
#include "templates/template.h"

class QByteArray;
class QPainter;
// IWYU pragma: no_forward_declare QPointF
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


/**
 * A template for displaying a GPX track.
 * 
 * A track is a set of track point segments and waypoints.
 * 
 * Normally, the geographic data from the track will be projected using the
 * map's georeferencing (georeferenced mode). However, it is also possible to
 * use a custom projection (non-georeferenced mode).
 * 
 * \see Track
 */
class TemplateTrack : public Template
{
	Q_OBJECT
	Q_DISABLE_COPY(TemplateTrack)
	
public:
	/**
	 * Returns the filename extensions supported by this template class.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	TemplateTrack(const QString& path, Map* map);
	~TemplateTrack() override;
	
	const char* getTemplateType() const override;
	bool isRasterGraphics() const override;
	
	bool saveTemplateFile() const override;
	
	bool loadTemplateFileImpl(bool configuring) override;
	bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view) override;
	void unloadTemplateFileImpl() override;
	
	void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const override;
	QRectF calculateTemplateBoundingBox() const override;
	int getTemplateBoundingBoxPixelBorder() override;
	
	bool hasAlpha() const override;
	
	/// Draws all tracks.
	void drawTracks(QPainter* painter, bool on_screen, qreal opacity) const;
	
	/// Draws all waypoints.
	void drawWaypoints(QPainter* painter, qreal opacity) const;
	
	/// Import the track as map object(s), returns true if something has been imported.
	/// TODO: should this be moved to the Track class?
	bool import(QWidget* dialog_parent = nullptr);
	
	/// Replaces the calls to pre/postLoadConfiguration() if creating a new GPS track.
	/// Assumes that the map's georeferencing is valid.
	void configureForGPSTrack();
	
	/// Returns the Track data object.
	inline Track& getTrack() {return track;}
	
public slots:
	void trackChanged(Track::TrackChange change, const TrackPoint& point);
	
	void updateGeoreferencing();
	
protected:
	Template* duplicateImpl() const override;
	
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;
	
	/// Calculates a simple projection for non-georeferenced track usage.
	QString calculateLocalGeoreferencing() const;
	
	/// Sets a custom projection, for non-georeferenced track usage.
	void setCustomProjection(const QString& projected_crs_spec);
	
	/// Returns the georeferencing to be used for projecting the track data.
	const Georeferencing& georeferencing() const;
	
	/// Updates the cached projected data from the geographic track data.
	void projectTrack();
	
	PathObject* importPathStart();
	void importPathEnd(PathObject* path);
	PointObject* importWaypoint(const MapCoordF& position, const QString &name = QString());
	
	
	Track track;
	std::unique_ptr<Georeferencing> projected_georef;
	std::vector<QPointF> waypoints;
	QVarLengthArray<QPainterPath, 4> track_segments;
	
	friend class OgrTemplate;                          // for migration
	
	// The following attributes might be removed in the future.
	std::unique_ptr<Georeferencing> preserved_georef;  // legacy
	QString track_crs_spec;                            // legacy
};


}  // namespace OpenOrienteering

#endif
