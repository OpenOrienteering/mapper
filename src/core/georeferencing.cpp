/*
 *    Copyright 2012-2017 Kai Pastor
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

#include <cstddef>
#include <algorithm>
#include <iterator>
#include <utility>

#include <QtGlobal>
#include <QByteArray>
#include <QDebug>
#include <QDir> // IWYU pragma: keep
#include <QFileInfo>
#include <QLatin1String>
#include <QLocale>
#include <QPoint>
#include <QSignalBlocker>
#include <QStringRef>
#include <QTemporaryDir> // IWYU pragma: keep
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <proj_api.h>

#include "core/crs_template.h"
#include "fileformats/file_format.h"
#include "fileformats/xml_file_format.h"
#include "util/xml_stream_util.h"


// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String georeferencing("georeferencing");
	
	static const QLatin1String scale("scale");
	static const QLatin1String grid_scale_factor{"grid_scale_factor"};
	static const QLatin1String declination("declination");
	static const QLatin1String grivation("grivation");
	static const QLatin1String grid_compensation("grid_compensation");
	static const QLatin1String m11("m11");
	static const QLatin1String m12("m12");
	static const QLatin1String m21("m21");
	static const QLatin1String m22("m22");
	
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



namespace OpenOrienteering {

namespace
{
	/** Helper for PROJ.4 initialization.
	 *
	 * To be used as static object in the right place.
	 */
	class ProjSetup
	{
	public:
		ProjSetup()
		{
			auto proj_data = QFileInfo(QLatin1String("data:/proj"));
			if (proj_data.exists())
			{
				static const auto location = proj_data.absoluteFilePath().toLocal8Bit();
				static auto data = location.constData();
				pj_set_searchpath(1, &data);
			}
			
#if defined(Q_OS_ANDROID)
			// Register file finder function needed by Proj.4
			registerProjFileHelper();
#endif
		}
	};
	
	/** Helper for PROJ.4 initialization.
	 * 
	 * This helper adds "+no_defs" if it is not already part of the
	 * specification. It also takes care of resetting the pj errno.
	 */
	projPJ pj_init_plus_no_defs(const QString& spec)
	{
		auto spec_latin1 = spec.toLatin1();
		if (!spec_latin1.contains("+no_defs"))
			spec_latin1.append(" +no_defs");
		
		*pj_get_errno_ref() = 0;
		return pj_init_plus(spec_latin1);
	}
	
	
	/**
	 * List of substitutions for specifications which are known to be broken in Proj.4.
	 */
	const char* spec_substitutions[][2] = {
	    // #542, S-JTSK (Greenwich) / Krovak East North
	    { "+init=epsg:5514", "+proj=krovak"
	                         " +lat_0=49.5 +lon_0=24.83333333333333"
	                         " +alpha=30.28813972222222 +k=0.9999"
	                         " +x_0=0 +y_0=0"
	                         " +ellps=bessel"
	                         " +pm=greenwich"
	                         " +towgs84=542.5,89.2,456.9,5.517,2.275,5.516,6.96"
	                         " +units=m"
	                         " +no_defs"
	    },
	};
	
	
}  // namespace



//### Georeferencing ###

const QString Georeferencing::geographic_crs_spec(QString::fromLatin1("+proj=latlong +datum=WGS84"));

Georeferencing::Georeferencing(bool use_grid_compensation)
: state(Local),
  scale_denominator{1000},
  grid_scale_factor{1.0},
  use_grid_compensation(use_grid_compensation),
  grid_compensation(),
  declination(0.0),
  grivation(0.0),
  grivation_error(0.0),
  map_ref_point(0, 0),
  projected_ref_point(0, 0)
{
	static ProjSetup run_once;
	
	updateTransformation();
	
	projected_crs_id = QString::fromLatin1("Local");
	projected_crs  = nullptr;
	geographic_crs = pj_init_plus_no_defs(geographic_crs_spec);
	Q_ASSERT(geographic_crs);
}

Georeferencing::Georeferencing(const Georeferencing& other)
: QObject(),
  state(other.state),
  scale_denominator(other.scale_denominator),
  grid_scale_factor{other.grid_scale_factor},
  use_grid_compensation(other.use_grid_compensation),
  grid_compensation(other.grid_compensation),
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
	
	geographic_crs = pj_init_plus_no_defs(geographic_crs_spec);
	Q_ASSERT(geographic_crs);
	projected_crs  = pj_init_plus_no_defs(projected_crs_spec);
}

Georeferencing::~Georeferencing()
{
	if (projected_crs)
		pj_free(projected_crs);
	if (geographic_crs)
		pj_free(geographic_crs);
}

Georeferencing& Georeferencing::operator=(const Georeferencing& other)
{
	if (&other == this)
		return *this;
	
	state                    = other.state;
	scale_denominator        = other.scale_denominator;
	grid_scale_factor        = other.grid_scale_factor;
	use_grid_compensation    = other.use_grid_compensation;
	grid_compensation        = other.grid_compensation;
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
	
	if (projected_crs)
		pj_free(projected_crs);
	projected_crs       = pj_init_plus_no_defs(projected_crs_spec);
	
	emit stateChanged();
	emit transformationChanged();
	emit declinationChanged();
	emit gridScaleFactorChanged();
	emit projectionChanged();
	
	return *this;
}



bool Georeferencing::isGeographic() const
{
	return projected_crs && pj_is_latlong(projected_crs);
}



void Georeferencing::load(QXmlStreamReader& xml, bool load_scale_only)
{
	Q_ASSERT(xml.name() == literal::georeferencing);
	
	{
		// Reset to default values
		const QSignalBlocker block(this);
		*this = Georeferencing(false);
	}
	
	XmlElementReader georef_element(xml);
	scale_denominator = georef_element.attribute<int>(literal::scale);
	if (scale_denominator <= 0)
		throw FileFormatException(tr("Map scale specification invalid or missing."));
	
	if (georef_element.hasAttribute(literal::grid_scale_factor))
	{
		grid_scale_factor = roundScaleFactor(georef_element.attribute<double>(literal::grid_scale_factor));
		if (grid_scale_factor <= 0.0)
			throw FileFormatException(tr("Invalid grid scale factor: %1").arg(QString::number(grid_scale_factor)));
	}
	state = Local;
	if (load_scale_only)
	{
		use_grid_compensation = true;
		grid_compensation *= grid_scale_factor;
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
			else if (xml.name() == literal::grid_compensation)
			{
				use_grid_compensation = true;
				XmlElementReader compensation_element(xml);
				double m11 = roundScaleFactor(compensation_element.attribute<double>(literal::m11));
				double m12 = roundScaleFactor(compensation_element.attribute<double>(literal::m12));
				double m21 = roundScaleFactor(compensation_element.attribute<double>(literal::m21));
				double m22 = roundScaleFactor(compensation_element.attribute<double>(literal::m22));
				grid_compensation = QTransform(m11, m12, m21, m22, 0.0, 0.0);
				if (grid_compensation.determinant() < 0.00000000001)
					throw FileFormatException(tr("Georeferencing grid compensation invalid."));
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
	emit gridScaleFactorChanged();
	if (!projected_crs_spec.isEmpty())
	{
		if (projected_crs)
			pj_free(projected_crs);
		projected_crs = pj_init_plus_no_defs(projected_crs_spec);
		if (0 == *pj_get_errno_ref())
		{
			state = Normal;
			emit stateChanged();
		}
		if (!use_grid_compensation)
		{
			updateGridCompensation();
		}
	}
	emit projectionChanged();
}

void Georeferencing::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter georef_element(xml, literal::georeferencing);
	georef_element.writeAttribute(literal::scale, scale_denominator);
	if (grid_scale_factor != 1.0)
		georef_element.writeAttribute(literal::grid_scale_factor, grid_scale_factor, scaleFactorPrecision());
	if (!qIsNull(declination))
		georef_element.writeAttribute(literal::declination, declination, declinationPrecision());
	if (!qIsNull(grivation))
		georef_element.writeAttribute(literal::grivation, grivation, declinationPrecision());
	if (use_grid_compensation)
	{
		XmlElementWriter compensation_element(xml, literal::grid_compensation);
		compensation_element.writeAttribute(literal::m11, grid_compensation.m11(), scaleFactorPrecision());
		compensation_element.writeAttribute(literal::m12, grid_compensation.m12(), scaleFactorPrecision());
		compensation_element.writeAttribute(literal::m21, grid_compensation.m21(), scaleFactorPrecision());
		compensation_element.writeAttribute(literal::m22, grid_compensation.m22(), scaleFactorPrecision());
	}
		
	if (!qIsNull(map_ref_point.lengthSquared()))
	{
		XmlElementWriter ref_point_element(xml, literal::ref_point);
		ref_point_element.writeAttribute(literal::x, map_ref_point.x());
		ref_point_element.writeAttribute(literal::y, map_ref_point.y());
	}
	
	{
		XmlElementWriter crs_element(xml, literal::projected_crs);
		if (!projected_crs_id.isEmpty())
			crs_element.writeAttribute(literal::id, projected_crs_id);
		
		if (!projected_crs_spec.isEmpty())
		{
			XmlElementWriter spec_element(xml, literal::spec);
			spec_element.writeAttribute(literal::language, literal::proj_4);
			xml.writeCharacters(projected_crs_spec);
		}
		
		if (!projected_crs_parameters.empty())
		{
			for (const auto& projected_crs_parameter : projected_crs_parameters)
			{
				XmlElementWriter parameter_element(xml, literal::parameter);
				xml.writeCharacters(projected_crs_parameter);
				Q_UNUSED(parameter_element); // Suppress compiler warnings
			}
		}
		
		if (!qIsNull(projected_ref_point.manhattanLength()))
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


void Georeferencing::setState(Georeferencing::State value)
{
	if (state != value)
	{
		state = value;
		updateTransformation();
		
		if (state != Normal)
			setProjectedCRS(QStringLiteral("Local"), {});
		
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

void Georeferencing::setGridScaleFactor(double value)
{
	Q_ASSERT(value > 0);
	double grid_scale_factor = roundScaleFactor(value);
	if (grid_scale_factor != this->grid_scale_factor)
	{
		this->grid_scale_factor = grid_scale_factor;
		if (!use_grid_compensation)
		{
			updateTransformation();
			emit gridScaleFactorChanged();
		}
	}
}

void Georeferencing::updateGridScaleFactor()
{
	setGridScaleFactor(scaleFactorOfCompensation(grid_compensation));
}

void Georeferencing::useGridCompensation(bool use_grid_compensation)
{
	if (use_grid_compensation != this->use_grid_compensation)
	{
		this->use_grid_compensation = use_grid_compensation;
		if (use_grid_compensation)
			updateGridCompensation();
		updateTransformation();
	}
}

void Georeferencing::setDeclination(double value)
{
	double declination = roundDeclination(value);
	double grivation = roundDeclination(value - convergenceOfCompensation(grid_compensation));
	setDeclinationAndGrivation(declination, grivation);
}

void Georeferencing::setGrivation(double value)
{
	double grivation = roundDeclination(value);
	double declination = roundDeclination(value + convergenceOfCompensation(grid_compensation));
	setDeclinationAndGrivation(declination, grivation);
}

void Georeferencing::setDeclinationAndGrivation(double declination, double grivation)
{
	bool declination_change = declination != this->declination;
	bool grivation_change = grivation != this->grivation;
	if (declination_change || grivation_change)
	{
		this->declination = declination;
		this->grivation   = grivation;
		this->grivation_error = 0.0;
		if ((use_grid_compensation && declination_change) || (!use_grid_compensation && grivation_change))
			updateTransformation();
		
		if (declination_change)
			emit declinationChanged();
	}
}

void Georeferencing::updateGridCompensation()
{
	QTransform grid_compensation = getGridCompensation();
	if (grid_compensation != this->grid_compensation)
	{
		this->grid_compensation = grid_compensation;
		
		if (use_grid_compensation)
			emit gridScaleFactorChanged();
		updateTransformation();
	}
}

void Georeferencing::setMapRefPoint(MapCoord point)
{
	if (map_ref_point != point)
	{
		map_ref_point = point;
		updateTransformation();
	}
}

void Georeferencing::setProjectedRefPoint(QPointF point, bool update_grivation, bool update_scale_factor)
{
	if (projected_ref_point != point || state == Normal)
	{
		projected_ref_point = point;
		bool ok;
		LatLon new_geo_ref_point;
		
		switch (state)
		{
		default:
			qWarning("Undefined georeferencing state");
			// fall through
		case Local:
			break;
		case Normal:
			new_geo_ref_point = toGeographicCoords(point, &ok);
			if (ok && new_geo_ref_point != geographic_ref_point)
			{
				geographic_ref_point = new_geo_ref_point;
				updateGridCompensation();
				if (update_grivation)
					updateGrivation();
				if (update_scale_factor)
				{
					if (use_grid_compensation)
						updateGridScaleFactor();
				}
				emit projectionChanged();
			}
		}
		updateTransformation();
	}
}

QString Georeferencing::getProjectedCRSName() const
{
	QString name = tr("Local");
	if (auto temp = CRSTemplateRegistry().find(projected_crs_id))
		name = temp->name();
	
	return name;
}

QString Georeferencing::getProjectedCoordinatesName() const
{
	QString name = tr("Local coordinates");
	if (auto temp = CRSTemplateRegistry().find(projected_crs_id))
		name = temp->coordinatesName(projected_crs_parameters);
	
	return name;
}

QTransform Georeferencing::getGridCompensation() const
{
	if (state != Normal || !isValid())
		return QTransform();

	const double delta = 1000.0; // meters

	QString local_crs_spec = QString::fromLatin1("+proj=sterea +lat_0=%1 +lon_0=%2 +ellps=WGS84 +units=m")
	        .arg(geographic_ref_point.latitude(), 0, 'f')
	        .arg(geographic_ref_point.longitude(), 0, 'f');
	projPJ local_crs = pj_init_plus_no_defs(local_crs_spec);
	if (!local_crs)
		return QTransform();

	// Determine 1 km baselines west-east and south-north on the ellipsoid.
	double easting = delta/2, northing = 0;
	int east_ret = pj_transform(local_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
	const LatLon east_point = LatLon::fromRadiant(northing, easting);

	easting = 0, northing = delta/2;
	int north_ret = pj_transform(local_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
	const LatLon north_point = LatLon::fromRadiant(northing, easting);

	easting = -delta/2, northing = 0;
	int west_ret = pj_transform(local_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
	const LatLon west_point = LatLon::fromRadiant(northing, easting);

	easting = 0, northing = -delta/2;
	int south_ret = pj_transform(local_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
	const LatLon south_point = LatLon::fromRadiant(northing, easting);

	if (!(east_ret == 0 && north_ret == 0 && west_ret == 0 && south_ret == 0))
		return QTransform();

	// Get projected coordinates on same meridian and on same parallel around reference point.
	bool ok_east = 0;
	QPointF projected_east  = toProjectedCoords(east_point,  &ok_east);
	bool ok_north = 0;
	QPointF projected_north = toProjectedCoords(north_point, &ok_north);
	bool ok_west = 0;
	QPointF projected_west  = toProjectedCoords(west_point,  &ok_west);
	bool ok_south = 0;
	QPointF projected_south = toProjectedCoords(south_point, &ok_south);
	if (!(ok_east && ok_north && ok_west && ok_south))
	{
		return QTransform();
	}

	// Points on the same meridian
	const double d_northing_dy = (projected_north.y() - projected_south.y()) / delta;
	const double d_easting_dy = (projected_north.x() - projected_south.x()) / delta;
	// Points on the same parallel
	const double d_northing_dx = (projected_east.y() - projected_west.y()) / delta;
	const double d_easting_dx = (projected_east.x() - projected_west.x()) / delta;

	// A transform with a tiny (or negative) determinant is nonsense for a map, and
	// would cause blow-ups.
	const double determinant = d_easting_dx*d_northing_dy - d_northing_dx*d_easting_dy;
	if (determinant < 0.00000000001)
	{
		return QTransform();
	}

	// Transform from model coordinates to grid coordinates.
	return QTransform(roundScaleFactor(d_easting_dx), roundScaleFactor(d_northing_dx), roundScaleFactor(d_easting_dy), roundScaleFactor(d_northing_dy), 0.0, 0.0);
}

double Georeferencing::convergenceOfCompensation(const QTransform &grid_compensation)
{
	// This is the angle between true azimuth and grid azimuth.
	// In case of deformation, the convergence varies with direction and this is an average.
	return RAD_TO_DEG * atan2(grid_compensation.m12()-grid_compensation.m21(),
	                          grid_compensation.m11()+grid_compensation.m22());
}

double Georeferencing::scaleFactorOfCompensation(const QTransform &grid_compensation)
{
	// This is the scale factor from true distance to grid distance.
	// In case of deformation, the scale factor varies with direction and this is an average.
	return sqrt(grid_compensation.determinant());
}

double Georeferencing::getConvergence() const
{
	return roundDeclination(convergenceOfCompensation(getGridCompensation()));
}

void Georeferencing::setGeographicRefPoint(LatLon lat_lon, bool update_grivation, bool update_scale_factor)
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
			updateGridCompensation();
			if (update_grivation)
				updateGrivation();
			if (update_scale_factor)
				updateGridScaleFactor();
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
	// When using complete grid compensation, apply the declination.
	// Otherwise the user-specified grid scale factor and grivation are used,
	// taking the place of any grid scale factor or convergence
	// present in the compensation matrix.

	QTransform projected_translation(1, 0, 0, 1, projected_ref_point.x(), projected_ref_point.y());
	QTransform transform;
	double scale = scale_denominator / 1000.0; // to meters
	if (use_grid_compensation)
	{
		transform = grid_compensation * projected_translation;  // includes convergence and grid scale factor
		transform.rotate(-declination);
	}
	else
	{
		transform = projected_translation;
		scale *= grid_scale_factor;
		transform.rotate(-grivation);
	}
	transform.scale(scale, -scale);
	transform.translate(-map_ref_point.x(), -map_ref_point.y());
	
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
	if (transform != to_projected)
	{
		to_projected = transform;
		from_projected = to_projected.inverted();
		emit transformationChanged();
	}
}

QTransform Georeferencing::mapToProjected() const
{
	return to_projected;
}

QTransform Georeferencing::projectedToMap() const
{
	return from_projected;
}



bool Georeferencing::setProjectedCRS(const QString& id, QString spec, std::vector<QString> params)
{
	// Default return value if no change is neccessary
	bool ok = (state == Normal || projected_crs_spec.isEmpty());
	
	for (const auto& substitution : spec_substitutions)
	{
		if (QLatin1String(substitution[0]) == spec)
		{
			spec = QString::fromLatin1(substitution[1]);
			break;
		}
	}
	
	// Changes in params shall already be recorded in spec
	if (projected_crs_id != id
	    || projected_crs_spec != spec
	    || projected_crs_parameters.size() != params.size()
	    || !std::equal(begin(params), end(params), begin(projected_crs_parameters))
	    || (!ok && !spec.isEmpty()) )
	{
		if (projected_crs)
			pj_free(projected_crs);
		
		projected_crs_id = id;
		projected_crs_spec = spec;
		if (projected_crs_spec.isEmpty())
		{
			projected_crs_parameters.clear();
			projected_crs = nullptr;
			ok = (state != Normal);
		}
		else
		{
			projected_crs_parameters.swap(params); // params was passed by value!
			projected_crs = pj_init_plus_no_defs(projected_crs_spec);
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
	return to_projected.map(QPointF(map_coords));
}

QPointF Georeferencing::toProjectedCoords(const MapCoordF& map_coords) const
{
	return to_projected.map(map_coords);
}

MapCoord Georeferencing::toMapCoords(const QPointF& projected_coords) const
{
	return MapCoord(from_projected.map(projected_coords));
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
	if (ok)
		*ok = false;

	double easting = projected_coords.x(), northing = projected_coords.y();
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(projected_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
		if (ok) 
			*ok = (ret == 0);
	}
	return LatLon::fromRadiant(northing, easting);
}

QPointF Georeferencing::toProjectedCoords(const LatLon& lat_lon, bool* ok) const
{
	if (ok)
		*ok = false;
	
	double easting = degToRad(lat_lon.longitude()), northing = degToRad(lat_lon.latitude());
	if (projected_crs && geographic_crs) {
		int ret = pj_transform(geographic_crs, projected_crs, 1, 1, &easting, &northing, nullptr);
		if (ok) 
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

MapCoordF Georeferencing::toMapCoordF(const Georeferencing* other, const MapCoordF& map_coords, bool* ok) const
{
	if (!other)
	{
		if (ok)
			*ok = true;
		return map_coords;
	}
	else if (isLocal() || other->isLocal())
	{
		if (ok)
			*ok = true;
		return toMapCoordF(other->toProjectedCoords(map_coords));
	}
	else
	{
		if (ok)
			*ok = false;
		
		QPointF projected_coords = other->toProjectedCoords(map_coords);
		double easting = projected_coords.x(), northing = projected_coords.y();
		if (projected_crs && other->projected_crs) {
			// Direct transformation:
			//int ret = pj_transform(other->projected_crs, projected_crs, 1, 1, &easting, &northing, nullptr);
			// Use geographic coordinates as intermediate step to enforce
			// that coordinates are assumed to have WGS84 datum if datum is specified in only one CRS spec:
			int ret = pj_transform(other->projected_crs, geographic_crs, 1, 1, &easting, &northing, nullptr);
			ret |= pj_transform(geographic_crs, projected_crs, 1, 1, &easting, &northing, nullptr);
			
			if (ret != 0)
			{
				if (ok) 
					*ok = false;
				return MapCoordF(easting, northing);
			}
			if (ok)
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
	  << " " << georef.grid_scale_factor
	  << " " << georef.declination
	  << " " << georef.grivation
	  << "deg, " << georef.projected_crs_id
	  << " (" << georef.projected_crs_spec
	  << ") " << QString::number(georef.projected_ref_point.x(), 'f', 8) << "," << QString::number(georef.projected_ref_point.y(), 'f', 8);
	if (georef.isLocal())
		dbg.nospace() << ", local)";
	else
		dbg.nospace() << ", geographic)";
	
	return dbg.space();
}


}  // namespace OpenOrienteering



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
	 * Otherwise it returns nullptr.
	 */
	const char* projFileHelperAndroid(const char *name)
	{
		if (temp_dir->isValid())
		{
			QString path = QDir(temp_dir->path()).filePath(QString::fromUtf8(name));
			QFile file(path);
			if (file.exists() || QFile::copy(QLatin1String("assets:/proj/") + QLatin1String(name), path))
			{
				c_string = path.toLocal8Bit();
				return c_string.constData();
			}
		}
		qDebug() << "Could not projection data file" << name;
		return nullptr;
	}
	
	void registerProjFileHelper()
	{
		// QTemporaryDir must not be constructed before QApplication
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

#endif  // defined(Q_OS_ANDROID)
