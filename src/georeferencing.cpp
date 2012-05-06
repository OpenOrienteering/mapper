/*
 *    Copyright 2012 Kai Pastor
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


#include "georeferencing.h"

#include <cassert>

#include <QLocale>
#include <QDebug>

#include <proj_api.h>

const QString Georeferencing::geographic_crs_spec("+proj=latlong +datum=WGS84");

Georeferencing::Georeferencing()
: scale_denominator(0), grivation(0.0), projected_ref_point(0, 0)
{
	updateTransformation();
	
	projected_crs  = NULL;
	geographic_crs = pj_init_plus(geographic_crs_spec.toAscii());
	assert(geographic_crs != NULL);
}

Georeferencing::Georeferencing(const Georeferencing& other)
: scale_denominator(other.scale_denominator), 
  grivation(other.grivation),
  projected_ref_point(other.projected_ref_point),
  projected_crs_id(other.projected_crs_id),
  projected_crs_spec(other.projected_crs_spec)
{
	updateTransformation();
	
	projected_crs  = pj_init_plus(projected_crs_spec.toAscii());
	geographic_crs = pj_init_plus(geographic_crs_spec.toAscii());
	assert(geographic_crs != NULL);
}

Georeferencing::~Georeferencing()
{
	if (projected_crs != NULL)
		pj_free(projected_crs);
	if (geographic_crs != NULL)
		pj_free(geographic_crs);
}

Georeferencing& Georeferencing::operator=(const Georeferencing& other)
{
	scale_denominator   = other.scale_denominator;
	grivation           = other.grivation;
	projected_ref_point = other.projected_ref_point;
	from_projected      = other.from_projected;
	to_projected        = other.to_projected;
	projected_ref_point = other.projected_ref_point;
	projected_crs_id    = other.projected_crs_id;
	projected_crs_spec  = other.projected_crs_spec;
	
	if (projected_crs != NULL)
		pj_free(projected_crs);
	projected_crs       = pj_init_plus(projected_crs_spec.toAscii());
	
	emit transformationChanged();
	emit projectionChanged();
	
	return *this;
}

void Georeferencing::setScaleDenominator(int value)
{
	scale_denominator = value;
	updateTransformation();
}

void Georeferencing::setGrivation(double value)
{
	grivation = value;
	updateTransformation();
}

void Georeferencing::setProjectedRefPoint(QPointF point)
{
	projected_ref_point = point;
	updateTransformation();
}

void Georeferencing::updateTransformation()
{
	to_projected.reset();
	double scale = double(scale_denominator) / 1000.0;
	to_projected.translate(projected_ref_point.x(), projected_ref_point.y());
	to_projected.rotate(-grivation);
	to_projected.scale(scale, -scale);
	
	from_projected = to_projected.inverted(); // FIXME: not tested
	
	emit transformationChanged();
}

bool Georeferencing::setProjectedCRS(const QString& id, const QString& spec)
{
	if (projected_crs != NULL)
		pj_free(projected_crs);
	
	this->projected_crs_id = id;
	this->projected_crs_spec = spec;
	projected_crs = pj_init_plus(projected_crs_spec.toAscii());
	
	emit projectionChanged();
	return projected_crs != NULL;
}

QPointF Georeferencing::toProjectedCoords(const MapCoord& map_coords) const
{
	return to_projected.map(map_coords.toQPointF());
}

QPointF Georeferencing::toProjectedCoords(const MapCoordF& map_coords) const
{
	return to_projected.map(map_coords.toQPointF());
}

MapCoord Georeferencing::toMapCoords(const QPointF& projected_coords) const
{
	return MapCoordF(from_projected.map(projected_coords)).toMapCoord(); // FIXME: not tested
}

QPointF Georeferencing::toGeographicCoords(const MapCoordF& map_coords, bool* ok) const
{
	return toGeographicCoords(toProjectedCoords(map_coords), ok);
}

QPointF Georeferencing::toGeographicCoords(const QPointF& projected_coords, bool* ok) const
{
	if (ok != NULL)
		*ok = false;

	double x = projected_coords.x(), y = projected_coords.y();
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(projected_crs, geographic_crs, 1, 1, &x, &y, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return QPointF(x, y);
}

QPointF Georeferencing::toProjectedCoords(const QPointF& lat_lon, bool* ok) const
{
	if (ok != NULL)
		*ok = false;
	
	double lat = lat_lon.x(), lon = lat_lon.y();
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(geographic_crs, projected_crs, 1, 1, &lat, &lon, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return QPointF(lat, lon);
}

double Georeferencing::radToDeg(double val) const
{
	return RAD_TO_DEG * val;
}

QString Georeferencing::radToDMS(double val) const
{
	qint64 tmp = RAD_TO_DEG * val * 360000;
	int csec = tmp % 6000;
	tmp = tmp / 6000;
	int min = tmp % 60;
	int deg = tmp / 60;
	QString ret = QString::fromUtf8("%1°%2'%3\"").arg(deg).arg(min).arg(QLocale().toString(csec/100.0,'f',2));
	return ret;
}

QDebug operator<<(QDebug dbg, const Georeferencing &georef)
{
	dbg.nospace() 
	  << "Georeferencing(1:" << georef.scale_denominator
	  << " " << georef.grivation
	  << "deg, " << georef.projected_crs_id
	  << " (" << georef.projected_crs_spec
	  << ") " << georef.projected_ref_point.x() << "," << georef.projected_ref_point.y();
	if (georef.isLocal())
		dbg.nospace() << ", local)";
	else
		dbg.nospace() << ", geographic)";
	
	return dbg.space();
}

#include "georeferencing.moc"