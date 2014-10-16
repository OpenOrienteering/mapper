/*
 *    Copyright 2012, 2013, 2014 Kai Pastor
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

#include <qmath.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSpinBox>
#include <QLineEdit>

#include <proj_api.h>

#include "crs_template.h"
#include "../file_format.h"
#include "../file_format_xml.h"
#include "../mapper_resource.h"
#include "../util_gui.h"
#include "../util/xml_stream_util.h"


// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String georeferencing("georeferencing");
	
	static const QLatin1String scale("scale");
	static const QLatin1String declination("declination");
	static const QLatin1String grivation("grivation");
	
	static const QLatin1String ref_point("ref_point");
	static const QLatin1String ref_point_deg("ref_point_deg");
	static const QLatin1String projected_crs("projected_crs");
	static const QLatin1String geographic_crs("geographic_crs");
	static const QLatin1String spec("spec");
	static const QLatin1String parameter("parameter");
	
	static const QLatin1String x("x");
	static const QLatin1String y("y");
	
	static const QLatin1String id("id");
	static const QLatin1String language("language");
	
	static const QLatin1String lat("lat");
	static const QLatin1String lon("lon");
	
	static const QLatin1String proj_4("PROJ.4");
	static const QLatin1String geographic_coordinates("Geographic coordinates");
}



//### Georeferencing ###

const QString Georeferencing::geographic_crs_spec("+proj=latlong +datum=WGS84");

Georeferencing::Georeferencing()
: state(ScaleOnly),
  scale_denominator(0),
  declination(0.0),
  grivation(0.0),
  grivation_error(0.0),
  map_ref_point(0, 0),
  projected_ref_point(0, 0)
{
	updateTransformation();
	
	projected_crs_id = "Local";
	projected_crs  = NULL;
	*pj_get_errno_ref() = 0;
	geographic_crs = pj_init_plus(geographic_crs_spec.toLatin1());
	if (0 != *pj_get_errno_ref())
	{
		QStringList locations = MapperResource::getLocations(MapperResource::PROJ_DATA);
		Q_FOREACH(QString location, locations)
		{
			if (geographic_crs != NULL)
				pj_free(geographic_crs);
			QByteArray pj_searchpath = QDir::toNativeSeparators(location).toLocal8Bit();
			const char* pj_searchpath_list = pj_searchpath.constData();
			pj_set_searchpath(1, &pj_searchpath_list);
			*pj_get_errno_ref() = 0;
			geographic_crs = pj_init_plus(geographic_crs_spec.toLatin1());
			if (0 == *pj_get_errno_ref())
				break;
		}
	}	
	Q_ASSERT(geographic_crs != NULL);
}

Georeferencing::Georeferencing(const Georeferencing& other)
: QObject(),
  state(other.state),
  scale_denominator(other.scale_denominator),
  declination(other.declination),
  grivation(other.grivation),
  grivation_error(other.grivation_error),
  map_ref_point(other.map_ref_point),
  projected_ref_point(other.projected_ref_point),
  projected_crs_id(other.projected_crs_id),
  projected_crs_spec(other.projected_crs_spec),
  projected_crs_parameters(other.projected_crs_parameters),
  geographic_ref_point(other.geographic_ref_point)
{
	updateTransformation();
	
	*pj_get_errno_ref() = 0;
	geographic_crs = pj_init_plus(geographic_crs_spec.toLatin1());
	Q_ASSERT(geographic_crs != NULL);
	projected_crs  = pj_init_plus(projected_crs_spec.toLatin1());
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
	state                    = other.state;
	scale_denominator        = other.scale_denominator;
	declination              = other.declination;
	grivation                = other.grivation;
	grivation_error          = other.grivation_error;
	map_ref_point            = other.map_ref_point;
	projected_ref_point      = other.projected_ref_point;
	from_projected           = other.from_projected;
	to_projected             = other.to_projected;
	projected_ref_point      = other.projected_ref_point;
	projected_crs_id         = other.projected_crs_id;
	projected_crs_spec       = other.projected_crs_spec;
	projected_crs_parameters = other.projected_crs_parameters;
	geographic_ref_point     = other.geographic_ref_point;
	
	if (projected_crs != NULL)
		pj_free(projected_crs);
	*pj_get_errno_ref() = 0;
	projected_crs       = pj_init_plus(projected_crs_spec.toLatin1());
	
	emit stateChanged();
	emit transformationChanged();
	emit declinationChanged();
	emit projectionChanged();
	
	return *this;
}

void Georeferencing::load(QXmlStreamReader& xml, bool load_scale_only) throw (FileFormatException)
{
	Q_ASSERT(xml.name() == literal::georeferencing);
	
	XmlElementReader georef_element(xml);
	scale_denominator = georef_element.attribute<int>(literal::scale);
	if (scale_denominator <= 0)
		throw FileFormatException(tr("Map scale specification invalid or missing."));
	state = ScaleOnly;
	if (load_scale_only)
	{
		xml.skipCurrentElement();
	}
	else
	{
		if (georef_element.hasAttribute(literal::declination))
			declination = roundDeclination(georef_element.attribute<double>(literal::declination));
		if (georef_element.hasAttribute(literal::grivation))
		{
			grivation = roundDeclination(georef_element.attribute<double>(literal::grivation));
			grivation_error = georef_element.attribute<double>(literal::grivation) - grivation;
		}
		
		while (xml.readNextStartElement())
		{
			if (xml.name() == literal::ref_point)
			{
				XmlElementReader ref_point_element(xml);
				map_ref_point.setX(ref_point_element.attribute<double>(literal::x));
				map_ref_point.setY(ref_point_element.attribute<double>(literal::y));
			}
			else if (xml.name() == literal::projected_crs)
			{
				XmlElementReader crs_element(xml);
				state = Local;
				projected_crs_id = crs_element.attribute<QString>(literal::id);
				while (xml.readNextStartElement())
				{
					XmlElementReader current_element(xml);
					if (xml.name() == literal::spec)
					{
						const QString language = current_element.attribute<QString>(literal::language);
						if (language != literal::proj_4)
							throw FileFormatException(tr("Unknown CRS specification language: %1").arg(language));
						projected_crs_spec = xml.readElementText();
					}
					else if (xml.name() == literal::parameter)
					{
						projected_crs_parameters.push_back(xml.readElementText());
					}
					else if (xml.name() == literal::ref_point)
					{
						projected_ref_point.setX(current_element.attribute<double>(literal::x));
						projected_ref_point.setY(current_element.attribute<double>(literal::y));
					}
					else // unknown
					{
						; // nothing
					}
				}
			}
			else if (xml.name() == literal::geographic_crs)
			{
				state = Normal;
				while (xml.readNextStartElement())
				{
					XmlElementReader current_element(xml);
					if (xml.name() == literal::spec)
					{
						const QString language = current_element.attribute<QString>(literal::language);
						if (language != literal::proj_4)
							throw FileFormatException(tr("Unknown CRS specification language: %1").arg(language));
						QString geographic_crs_spec = xml.readElementText();
						if (Georeferencing::geographic_crs_spec != geographic_crs_spec)
							throw FileFormatException(tr("Unsupported geographic CRS specification: %1").arg(geographic_crs_spec));
					}
					else if (xml.name() == literal::ref_point)
					{
						// Legacy, latitude/longitude in radiant
						double latitude  = current_element.attribute<double>(literal::lat);
						double longitude = current_element.attribute<double>(literal::lon);
						geographic_ref_point = LatLon::fromRadiant(latitude, longitude);
					}
					else if (xml.name() == literal::ref_point_deg)
					{
						// Legacy, latitude/longitude in degrees
						double latitude  = current_element.attribute<double>(literal::lat);
						double longitude = current_element.attribute<double>(literal::lon);
						geographic_ref_point = LatLon(latitude, longitude);
					}
					else // unknown
					{
						; // nothing
					}
				}
			}
			else // unknown
			{
				; // nothing
			}
		}
	}
	
	emit stateChanged();
	updateTransformation();
	emit declinationChanged();
	*pj_get_errno_ref() = 0;
	projected_crs = pj_init_plus(projected_crs_spec.toLatin1());
	emit projectionChanged();
}

void Georeferencing::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter georef_element(xml, literal::georeferencing);
	georef_element.writeAttribute(literal::scale, scale_denominator);
	if (state != ScaleOnly)
	{
		georef_element.writeAttribute(literal::declination, declination, declinationPrecision());
		georef_element.writeAttribute(literal::grivation, grivation, declinationPrecision());
		
		{
			XmlElementWriter ref_point_element(xml, literal::ref_point);
			ref_point_element.writeAttribute(literal::x, map_ref_point.xd());
			ref_point_element.writeAttribute(literal::y, map_ref_point.yd());
		}
		
		{
			XmlElementWriter crs_element(xml, literal::projected_crs);
			crs_element.writeAttribute(literal::id, projected_crs_id);
			{
				XmlElementWriter spec_element(xml, literal::spec);
				spec_element.writeAttribute(literal::language, literal::proj_4);
				xml.writeCharacters(projected_crs_spec);
			}
			for (size_t i = 0; i < projected_crs_parameters.size(); ++i)
			{
				XmlElementWriter parameter_element(xml, literal::parameter);
				xml.writeCharacters(projected_crs_parameters[i]);
				Q_UNUSED(parameter_element); // Suppress compiler warnings
			}
			{
				XmlElementWriter ref_point_element(xml, literal::ref_point);
				ref_point_element.writeAttribute(literal::x, projected_ref_point.x(), 6);
				ref_point_element.writeAttribute(literal::y, projected_ref_point.y(), 6);
			}
		}
		
		if (state == Normal)
		{
			XmlElementWriter crs_element(xml, literal::geographic_crs);
			crs_element.writeAttribute(literal::id, literal::geographic_coordinates);
			{
				XmlElementWriter spec_element(xml, literal::spec);
				spec_element.writeAttribute(literal::language, literal::proj_4);
				xml.writeCharacters(geographic_crs_spec);
			}
			if (XMLFileFormat::active_version < 6)
			{
				// Legacy compatibility
				XmlElementWriter ref_point_element(xml, literal::ref_point);
				ref_point_element.writeAttribute(literal::lat, degToRad(geographic_ref_point.latitude()), 10);
				ref_point_element.writeAttribute(literal::lon, degToRad(geographic_ref_point.longitude()), 10);
			}
			{
				XmlElementWriter ref_point_element(xml, literal::ref_point_deg);
				ref_point_element.writeAttribute(literal::lat, geographic_ref_point.latitude(), 8);
				ref_point_element.writeAttribute(literal::lon, geographic_ref_point.longitude(), 8);
			}
		}
	}
}


void Georeferencing::setState(Georeferencing::State value)
{
	if (state != value)
	{
		state = value;
		updateTransformation();
		
		if (state != Normal)
			setProjectedCRS("Local");
		
		emit stateChanged();
	}
}

void Georeferencing::setScaleDenominator(int value)
{
	Q_ASSERT(value > 0);
	if (scale_denominator != (unsigned int)value)
	{
		scale_denominator = value;
		updateTransformation();
	}
}

void Georeferencing::setDeclination(double value)
{
	double declination = roundDeclination(value);
	double grivation = declination - getConvergence();
	setDeclinationAndGrivation(declination, grivation);
}

void Georeferencing::setGrivation(double value)
{
	double grivation = roundDeclination(value);
	double declination = grivation + getConvergence();
	setDeclinationAndGrivation(declination, grivation);
}

void Georeferencing::setDeclinationAndGrivation(double declination, double grivation)
{
	bool declination_change = declination != this->declination;
	if (declination_change || grivation != this->grivation)
	{
		this->declination = declination;
		this->grivation   = grivation;
		this->grivation_error = 0.0;
		
		if (state == ScaleOnly)
			setState(Local);
		
		updateTransformation();
		
		if (declination_change)
			emit declinationChanged();
	}
}

void Georeferencing::setMapRefPoint(MapCoord point)
{
	if (map_ref_point != point)
	{
		map_ref_point = point;
		
		if (state == ScaleOnly)
			setState(Local);
		
		updateTransformation();
	}
}

void Georeferencing::setProjectedRefPoint(QPointF point, bool update_grivation)
{
	if (projected_ref_point != point || state == Normal)
	{
		projected_ref_point = point;
		bool ok;
		LatLon new_geo_ref_point;
		
		switch (state)
		{
		default:
			Q_ASSERT(false && "Undefined georeferencing state");
			// fall through
		case ScaleOnly:
			setState(Local);
			// fall through
		case Local:
			break;
		case Normal:
			new_geo_ref_point = toGeographicCoords(point, &ok);
			if (ok && new_geo_ref_point != geographic_ref_point)
			{
				geographic_ref_point = new_geo_ref_point;
				if (update_grivation)
					updateGrivation();
				emit projectionChanged();
			}
		}
		updateTransformation();
	}
}

QString Georeferencing::getProjectedCRSName() const
{
	if (state == ScaleOnly)
		return tr("Scale only");
	else if (state == Local)
		return tr("Local");
	
	CRSTemplate* temp = CRSTemplate::getCRSTemplate(projected_crs_id);
	if (temp)
		return temp->getName();
	else
		return tr("Projected");
}

double Georeferencing::getConvergence() const
{
	if (state != Normal || !isValid())
		return 0.0;
	
	// Second point on the same meridian
	const double delta_phi = 360.0 / 40000.0;  // roughly 1 km
	double other_latitude = geographic_ref_point.latitude();
	other_latitude +=  (other_latitude < 0.0) ? delta_phi : -delta_phi;
	const double same_longitude = geographic_ref_point.longitude();
	QPointF projected_other = toProjectedCoords(LatLon(other_latitude, same_longitude));
	
	double denominator = projected_other.y() - projected_ref_point.y();
	if (fabs(denominator) < 0.00000000001)
		return 0.0;
	
	return roundDeclination(RAD_TO_DEG * atan((projected_ref_point.x() - projected_other.x()) / denominator));
}

void Georeferencing::setGeographicRefPoint(LatLon lat_lon, bool update_grivation)
{
	bool geo_ref_point_changed = geographic_ref_point != lat_lon;
	if (geo_ref_point_changed || state == Normal)
	{
		geographic_ref_point = lat_lon;
		if (state != Normal)
			setState(Normal);
		
		bool ok;
		QPointF new_projected_ref = toProjectedCoords(lat_lon, &ok);
		if (ok && new_projected_ref != projected_ref_point)
		{
			projected_ref_point = new_projected_ref;
			if (update_grivation)
				updateGrivation();
			updateTransformation();
			emit projectionChanged();
		}
		else if (geo_ref_point_changed)
		{
			emit projectionChanged();
		}
	}
}

void Georeferencing::updateTransformation()
{
	QTransform transform;
	if (state != ScaleOnly)
	{
		transform.translate(projected_ref_point.x(), projected_ref_point.y());
		transform.rotate(-grivation);
	}
	double scale = double(scale_denominator) / 1000.0;
	transform.scale(scale, -scale);
	if (state != ScaleOnly)
	{
		transform.translate(-map_ref_point.xd(), -map_ref_point.yd());
	}
	
	if (to_projected != transform)
	{
		to_projected   = transform;
		from_projected = transform.inverted();
		emit transformationChanged();
	}
}

void Georeferencing::updateGrivation()
{
	setDeclination(declination);
}

void Georeferencing::initDeclination()
{
	setGrivation(grivation);
}

void Georeferencing::setTransformationDirectly(const QTransform& transform)
{
	if (state == ScaleOnly)
	{
		state = Local;
		emit stateChanged();
	}
	if (transform != to_projected)
	{
		to_projected = transform;
		from_projected = to_projected.inverted();
		emit transformationChanged();
	}
}

bool Georeferencing::setProjectedCRS(const QString& id, const QString& spec, std::vector< QString > params)
{
	// Default return value if no change is neccessary
	bool ok = (state == Normal || projected_crs_spec.isEmpty());
	
	// Changes in params shall already be recorded in spec
	if (projected_crs_id != id || projected_crs_spec != spec || (!ok && !spec.isEmpty()) )
	{
		if (projected_crs != NULL)
			pj_free(projected_crs);
		
		projected_crs_id = id;
		projected_crs_spec = spec;
		if (projected_crs_spec.isEmpty())
		{
			projected_crs_parameters.clear();
			projected_crs = NULL;
			ok = (state != Normal);
		}
		else
		{
			projected_crs_parameters.swap(params); // params was passed by value!
			*pj_get_errno_ref() = 0;
			projected_crs = pj_init_plus(projected_crs_spec.toLatin1());
			ok = (0 == *pj_get_errno_ref());
			if (ok && state != Normal)
				setState(Normal);
		}
		
		emit projectionChanged();
	}
	
	return ok;
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
	return MapCoordF(from_projected.map(projected_coords)).toMapCoord();
}

MapCoordF Georeferencing::toMapCoordF(const QPointF& projected_coords) const
{
	return MapCoordF(from_projected.map(projected_coords));
}

LatLon Georeferencing::toGeographicCoords(const MapCoordF& map_coords, bool* ok) const
{
	return toGeographicCoords(toProjectedCoords(map_coords), ok);
}

LatLon Georeferencing::toGeographicCoords(const QPointF& projected_coords, bool* ok) const
{
	if (ok != NULL)
		*ok = false;

	double easting = projected_coords.x(), northing = projected_coords.y();
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(projected_crs, geographic_crs, 1, 1, &easting, &northing, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return LatLon::fromRadiant(northing, easting);
}

QPointF Georeferencing::toProjectedCoords(const LatLon& lat_lon, bool* ok) const
{
	if (ok != NULL)
		*ok = false;
	
	double easting = degToRad(lat_lon.longitude()), northing = degToRad(lat_lon.latitude());
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(geographic_crs, projected_crs, 1, 1, &easting, &northing, NULL);
		if (ok != NULL) 
			*ok = (ret == 0);
	}
	return QPointF(easting, northing);
}

MapCoord Georeferencing::toMapCoords(const LatLon& lat_lon, bool* ok) const
{
	return toMapCoords(toProjectedCoords(lat_lon, ok));
}

MapCoordF Georeferencing::toMapCoordF(const LatLon& lat_lon, bool* ok) const
{
	return toMapCoordF(toProjectedCoords(lat_lon, ok));
}

MapCoordF Georeferencing::toMapCoordF(Georeferencing* other, const MapCoordF& map_coords, bool* ok) const
{
	if (other == NULL)
	{
		if (ok)
			*ok = true;
		return map_coords;
	}
	else if (isLocal() || getState() == ScaleOnly ||
		other->isLocal() || other->getState() == ScaleOnly)
	{
		if (ok)
			*ok = true;
		return toMapCoordF(other->toProjectedCoords(map_coords));
	}
	else
	{
		if (ok != NULL)
			*ok = false;
		
		QPointF projected_coords = other->toProjectedCoords(map_coords);
		double easting = projected_coords.x(), northing = projected_coords.y();
		if (projected_crs && other->projected_crs) {
			// Direct transformation:
			//int ret = pj_transform(other->projected_crs, projected_crs, 1, 1, &easting, &northing, NULL);
			// Use geographic coordinates as intermediate step to enforce
			// that coordinates are assumed to have WGS84 datum if datum is specified in only one CRS spec:
			int ret = pj_transform(other->projected_crs, geographic_crs, 1, 1, &easting, &northing, NULL);
			ret |= pj_transform(geographic_crs, projected_crs, 1, 1, &easting, &northing, NULL);
			
			if (ret != 0)
			{
				if (ok != NULL) 
					*ok = false;
				return MapCoordF(easting, northing);
			}
			if (ok != NULL)
				*ok = true;
		}
		return toMapCoordF(QPointF(easting, northing));
	}
}

QString Georeferencing::getErrorText() const
{
	int err_no = *pj_get_errno_ref();
	return (err_no == 0) ? QString() : QString::fromLatin1(pj_strerrno(err_no));
}

double Georeferencing::radToDeg(double val)
{
	return RAD_TO_DEG * val;
}

double Georeferencing::degToRad(double val)
{
	return DEG_TO_RAD * val;
}

QString Georeferencing::degToDMS(double val)
{
	qint64 tmp = val * 360000;
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
	  << " " << georef.declination
	  << " " << georef.grivation
	  << "deg, " << georef.projected_crs_id
	  << " (" << georef.projected_crs_spec
	  << ") " << QString::number(georef.projected_ref_point.x(), 'f', 8) << "," << QString::number(georef.projected_ref_point.y(), 'f', 8);
	if (georef.getState() == Georeferencing::ScaleOnly)
		dbg.nospace() << ", scale only)";
	else if (georef.isLocal())
		dbg.nospace() << ", local)";
	else
		dbg.nospace() << ", geographic)";
	
	return dbg.space();
}

#if defined(Q_OS_ANDROID)

QScopedPointer<QTemporaryDir> temp_dir;  // removed upon destruction
QByteArray c_string;  // buffer for const char*

extern "C"
{
	/**
	 * @brief Provides required files for Proj.4 library.
	 * 
	 * This C function implements the interface required by pj_set_finder().
	 * 
	 * This functions checks if the requested file name is available in a
	 * temporary directory. If not, it tries to copy the file from the proj
	 * subfolder of the assets folder to this temporary directory.
	 * 
	 * If the file exists in the temporary folder (or copying was successful)
	 * this function returns the full path of this file as a C string.
	 * This string becomes invalid the next time this function is called.
	 * Otherwise it returns NULL.
	 */
	const char* projFileHelperAndroid(const char *name)
	{
		if (temp_dir->isValid())
		{
			QString path = QDir(temp_dir->path()).filePath(name);
			QFile file(path);
			if (file.exists() || QFile::copy(QString("assets:/proj/") % QLatin1String(name), path))
			{
				c_string = path.toLocal8Bit();
				return c_string.constData();
			}
		}
		qDebug() << "Could not projection data file" << name;
		return NULL;
	}
	
	void registerProjFileHelper()
	{
		// TemporaryDir must not be constructed before QApplication
		temp_dir.reset(new QTemporaryDir());
		if (temp_dir->isValid())
		{
			pj_set_finder(&projFileHelperAndroid);
		}
		else
		{
			qDebug() << "Could not create a temporary directory for projection data.";
		}
	}
}

#endif
