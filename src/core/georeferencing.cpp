/*
 *    Copyright 2012-2020 Kai Pastor
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

#include <algorithm>
#include <cmath> // IWYU pragma: keep
#include <iterator>
#include <utility>

#include <QtGlobal>
#include <QtMath>
#include <QByteArray>
#include <QDebug>
#include <QDir> // IWYU pragma: keep
#include <QFileInfo>
#include <QLatin1String>
#include <QLocale>
#include <QPoint>
#include <QSignalBlocker>
#include <QStandardPaths> // IWYU pragma: keep
#include <QStringRef>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifdef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#  include <proj_api.h>
#else
#  include <proj.h>
#endif

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
	static const QLatin1String auxiliary_scale_factor{"auxiliary_scale_factor"};
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



namespace OpenOrienteering {

namespace
{
#ifdef Q_OS_ANDROID
	/**
	 * Provides a cache directory for RROJ resource files.
	 */
	const QDir& projCacheDirectory()
	{
		static auto const dir = []() -> QDir {
			auto cache = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
			auto proj = QStringLiteral("proj");
			if (!cache.exists(proj))
				cache.mkpath(proj);
			if (!cache.exists(proj))
				qDebug("Could not create a cache directory for PROJ resource files");
			cache.cd(proj);
			return cache;
		}();
		return dir;
	}
	
	/**
	 * Ensures PROJ resource file availability in the cache directory.
	 * 
	 * This function implements the interface required by
	 * proj_context_set_file_finder().
	 * 
	 * This functions checks if the requested file name is available in the
	 * cache directory. If not, it tries to copy the file from the proj
	 * subfolder of the assets folder to this temporary directory.
	 * 
	 * It always returns nullptr, letting PROJ find the file in the cache
	 * directory via the configured search path.
	 */
	extern "C"
	const char* projFileHelperAndroid(PJ_CONTEXT* /*ctx*/, const char* name, void* /*user_data*/)
	{
		auto const& cache = projCacheDirectory();
		if (cache.exists())
		{
			auto const name_string = QString::fromUtf8(name);
			auto const path = cache.filePath(name_string);
			if (!QFile::exists(path)
			    && !QFile::copy(QLatin1String("assets:/proj/") + name_string, path))
			{
				qDebug("Could not provide projection data file '%s'", name);
			}
		}
		return nullptr;
	}
#endif
	
	
	/** Helper for PROJ initialization.
	 *
	 * To be used as static object in the right place.
	 */
	class ProjSetup
	{
	public:
		ProjSetup()
		{
#ifdef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
			auto proj_data = QFileInfo(QLatin1String("data:/proj"));
			if (proj_data.exists())
			{
				static auto const location = proj_data.absoluteFilePath().toLocal8Bit();
				static auto* data = location.constData();
				pj_set_searchpath(1, &data);
			}
			
			// no Android file finder support for deprecated PROJ API
#else
			proj_context_use_proj4_init_rules(PJ_DEFAULT_CTX, 1);

#if defined(Q_OS_ANDROID)
			// Register file finder function needed by Proj.4
			proj_context_set_file_finder(nullptr, &projFileHelperAndroid, nullptr);
			auto proj_data = QFileInfo(projCacheDirectory().path());
#else
			auto proj_data = QFileInfo(QLatin1String("data:/proj"));
#endif  // defined(Q_OS_ANDROID)
			if (proj_data.exists())
			{
				static auto const location = proj_data.absoluteFilePath().toLocal8Bit();
				static auto* const data = location.constData();
				proj_context_set_search_paths(nullptr, 1, &data);
			}
#endif  // ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
		}
	};
	
	
	/**
	 * List of substitutions for specifications which are known to be broken in PROJ.
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



//### ProjTransform ###

#ifdef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H

ProjTransform::ProjTransform(ProjTransformData* pj) noexcept
: pj{pj}
{}

ProjTransform::ProjTransform(ProjTransform&& other) noexcept
{
	operator=(std::move(other));
}

ProjTransform::ProjTransform(const QString& crs_spec)
{
	auto spec_latin1 = crs_spec.toLatin1();
	if (!spec_latin1.contains("+no_defs"))
		spec_latin1.append(" +no_defs");
	
	*pj_get_errno_ref() = 0;
	pj = pj_init_plus(spec_latin1);
}

ProjTransform::~ProjTransform()
{
	if (pj)
		pj_free(pj);
}

ProjTransform& ProjTransform::operator=(ProjTransform&& other) noexcept
{
	std::swap(pj, other.pj);
	return *this;
}

// static
ProjTransform ProjTransform::crs(const QString& crs_spec)
{
	ProjTransform result;
	auto crs_spec_latin1 = crs_spec.toLatin1();
	if (!crs_spec_latin1.contains("+no_defs"))
		crs_spec_latin1.append(" +no_defs");
	result.pj = pj_init_plus(crs_spec_latin1);
	return result;
}

bool ProjTransform::isValid() const noexcept
{
	return pj != nullptr;
}

bool ProjTransform::isGeographic() const
{
	return isValid() && pj_is_latlong(pj);
}

QPointF ProjTransform::forward(const LatLon& lat_lon, bool* ok) const
{
	static auto const geographic_crs = ProjTransform(Georeferencing::geographic_crs_spec);
	
	double easting = qDegreesToRadians(lat_lon.longitude()), northing = qDegreesToRadians(lat_lon.latitude());
	if (geographic_crs.isValid())
	{
		auto ret = pj_transform(geographic_crs.pj, pj, 1, 1, &easting, &northing, nullptr);
		if (ok)
			*ok = (ret == 0);
	}
	else if (ok)
	{
		*ok = false;
	}
	return {easting, northing};
}

LatLon ProjTransform::inverse(const QPointF& projected_coords, bool* ok) const
{
	static auto const geographic_crs = ProjTransform(Georeferencing::geographic_crs_spec);
	
	double easting = projected_coords.x(), northing = projected_coords.y();
	if (geographic_crs.isValid())
	{
		auto ret = pj_transform(pj, geographic_crs.pj, 1, 1, &easting, &northing, nullptr);
		if (ok)
			*ok = (ret == 0);
	}
	else if (ok)
	{
		*ok = false;
	}
	return LatLon::fromRadiant(northing, easting);
}

QString ProjTransform::errorText() const
{
	auto err_no = *pj_get_errno_ref();
	return (err_no == 0) ? QString() : QString::fromLatin1(pj_strerrno(err_no));
}

#else

ProjTransform::ProjTransform(ProjTransformData* pj) noexcept
: pj{pj}
{}

ProjTransform::ProjTransform(ProjTransform&& other) noexcept
{
	operator=(std::move(other));
}

ProjTransform::ProjTransform(const QString& crs_spec)
{
	if (crs_spec.isEmpty())
		return;
	
	auto spec_latin1 = crs_spec.toLatin1();
#ifdef PROJ_ISSUE_1573
	// Cf. https://github.com/OSGeo/PROJ/pull/1573
	spec_latin1.replace("+datum=potsdam", "+ellps=bessel +nadgrids=@BETA2007.gsb");
#endif
	pj = proj_create_crs_to_crs(PJ_DEFAULT_CTX, Georeferencing::geographic_crs_spec.toLatin1(), spec_latin1, nullptr);
	if (pj)
		operator=({proj_normalize_for_visualization(PJ_DEFAULT_CTX, pj)});
}

ProjTransform::~ProjTransform()
{
	if (pj)
		proj_destroy(pj);
}

ProjTransform& ProjTransform::operator=(ProjTransform&& other) noexcept
{
	std::swap(pj, other.pj);
	return *this;
}

// static
ProjTransform ProjTransform::crs(const QString& crs_spec)
{
	ProjTransform result;
	auto crs_spec_utf8 = crs_spec.toUtf8();
#ifdef PROJ_ISSUE_1573
	// Cf. https://github.com/OSGeo/PROJ/pull/1573
	crs_spec_utf8.replace("+datum=potsdam", "+ellps=bessel +nadgrids=@BETA2007.gsb");
#endif
	result.pj = proj_create(PJ_DEFAULT_CTX, crs_spec_utf8);
	return result;
}

bool ProjTransform::isValid() const noexcept
{
	return pj != nullptr;
}

bool ProjTransform::isGeographic() const
{
	if (!isValid())
		return false;
	
	switch (proj_get_type(pj))
	{
	case PJ_TYPE_GEOGRAPHIC_CRS:
	case PJ_TYPE_GEOGRAPHIC_2D_CRS:
	case PJ_TYPE_GEOGRAPHIC_3D_CRS:
		return true;
	default:
		return false;
	}

}

QPointF ProjTransform::forward(const LatLon& lat_lon, bool* ok) const
{
	proj_errno_reset(pj);
	auto pj_coord = proj_trans(pj, PJ_FWD, proj_coord(lat_lon.longitude(), lat_lon.latitude(), 0, HUGE_VAL));
	if (ok)
		*ok = proj_errno(pj) == 0;
	return {pj_coord.xy.x, pj_coord.xy.y};
}

LatLon ProjTransform::inverse(const QPointF& projected_coords, bool* ok) const
{
	proj_errno_reset(pj);
	auto pj_coord = proj_trans(pj, PJ_INV, proj_coord(projected_coords.x(), projected_coords.y(), 0, HUGE_VAL));
	if (ok)
		*ok = proj_errno(pj) == 0;
	return {pj_coord.lp.phi, pj_coord.lp.lam};
}

QString ProjTransform::errorText() const
{
	auto err_no = proj_errno(pj);
	return (err_no == 0) ? QString() : QString::fromLatin1(proj_errno_string(err_no));
}

#endif



//### Georeferencing ###

const QString Georeferencing::geographic_crs_spec(QString::fromLatin1("+proj=latlong +datum=WGS84"));

Georeferencing::Georeferencing()
: state(Local),
  scale_denominator{1000},
  combined_scale_factor{1.0},
  auxiliary_scale_factor{1.0},
  grid_scale_factor{1.0},
  declination(0.0),
  grivation(0.0),
  grivation_error(0.0),
  convergence(0.0),
  map_ref_point(0, 0),
  projected_ref_point(0, 0)
{
	static ProjSetup run_once;
	
	updateTransformation();
	
	projected_crs_id = QString::fromLatin1("Local");
}

Georeferencing::Georeferencing(const Georeferencing& other)
: QObject(),
  state(other.state),
  scale_denominator(other.scale_denominator),
  combined_scale_factor{other.combined_scale_factor},
  auxiliary_scale_factor{other.auxiliary_scale_factor},
  grid_scale_factor(other.grid_scale_factor),
  declination(other.declination),
  grivation(other.grivation),
  grivation_error(other.grivation_error),
  convergence(other.convergence),
  map_ref_point(other.map_ref_point),
  projected_ref_point(other.projected_ref_point),
  projected_crs_id(other.projected_crs_id),
  projected_crs_spec(other.projected_crs_spec),
  projected_crs_parameters(other.projected_crs_parameters),
  proj_transform(projected_crs_spec),
  geographic_ref_point(other.geographic_ref_point)
{
	updateTransformation();
}

Georeferencing::~Georeferencing() = default;

Georeferencing& Georeferencing::operator=(const Georeferencing& other)
{
	if (&other == this)
		return *this;
	
	state                    = other.state;
	scale_denominator        = other.scale_denominator;
	combined_scale_factor    = other.combined_scale_factor;
	auxiliary_scale_factor   = other.auxiliary_scale_factor;
	grid_scale_factor        = other.grid_scale_factor;
	declination              = other.declination;
	grivation                = other.grivation;
	grivation_error          = other.grivation_error;
	convergence              = other.convergence;
	map_ref_point            = other.map_ref_point;
	projected_ref_point      = other.projected_ref_point;
	from_projected           = other.from_projected;
	to_projected             = other.to_projected;
	projected_ref_point      = other.projected_ref_point;
	projected_crs_id         = other.projected_crs_id;
	projected_crs_spec       = other.projected_crs_spec;
	projected_crs_parameters = other.projected_crs_parameters;
	proj_transform           = ProjTransform(other.projected_crs_spec);
	geographic_ref_point     = other.geographic_ref_point;
	
	emit stateChanged();
	emit transformationChanged();
	emit declinationChanged();
	emit auxiliaryScaleFactorChanged();
	emit projectionChanged();
	
	return *this;
}



bool Georeferencing::isGeographic() const
{
	return ProjTransform::crs(getProjectedCRSSpec()).isGeographic();
}



void Georeferencing::load(QXmlStreamReader& xml, bool load_scale_only)
{
	Q_ASSERT(xml.name() == literal::georeferencing);
	
	{
		// Reset to default values
		const QSignalBlocker block(this);
		*this = Georeferencing();
	}
	
	XmlElementReader georef_element(xml);
	scale_denominator = georef_element.attribute<int>(literal::scale);
	if (scale_denominator <= 0)
		throw FileFormatException(tr("Map scale specification invalid or missing."));
	
	if (georef_element.hasAttribute(literal::grid_scale_factor))
	{
		combined_scale_factor = roundScaleFactor(georef_element.attribute<double>(literal::grid_scale_factor));
		if (combined_scale_factor <= 0.0)
			throw FileFormatException(tr("Invalid grid scale factor: %1").arg(QString::number(combined_scale_factor)));
	}
	state = Local;
	if (load_scale_only)
	{
		xml.skipCurrentElement();
	}
	else
	{
		if (georef_element.hasAttribute(literal::auxiliary_scale_factor))
		{
			auxiliary_scale_factor = roundScaleFactor(georef_element.attribute<double>(literal::auxiliary_scale_factor));
			if (auxiliary_scale_factor <= 0.0)
				throw FileFormatException(tr("Invalid auxiliary scale factor: %1").arg(QString::number(auxiliary_scale_factor)));
		}
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
			else // unknown
			{
				xml.skipCurrentElement();
			}
		}
	}
	
	emit stateChanged();
	updateTransformation();
	emit declinationChanged();
	if (!projected_crs_spec.isEmpty())
	{
		proj_transform = {projected_crs_spec};
		if (proj_transform.isValid())
		{
			state = Normal;
		}
		updateGridCompensation();
		if (!georef_element.hasAttribute(literal::auxiliary_scale_factor))
		{
			initAuxiliaryScaleFactor();
		}
		emit auxiliaryScaleFactorChanged();
		if (proj_transform.isValid())
		{
			emit stateChanged();
		}
	}
	emit projectionChanged();
}

void Georeferencing::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter georef_element(xml, literal::georeferencing);
	georef_element.writeAttribute(literal::scale, scale_denominator);
	if (combined_scale_factor != 1.0 || auxiliary_scale_factor != 1.0)
	{
		if (combined_scale_factor != 1.0)
			georef_element.writeAttribute(literal::grid_scale_factor, combined_scale_factor, scaleFactorPrecision());
		georef_element.writeAttribute(literal::auxiliary_scale_factor, auxiliary_scale_factor, scaleFactorPrecision());
	}
	if (!qIsNull(declination))
		georef_element.writeAttribute(literal::declination, declination, declinationPrecision());
	if (!qIsNull(grivation))
		georef_element.writeAttribute(literal::grivation, grivation, declinationPrecision());
		
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
		{
			setProjectedCRS(QStringLiteral("Local"), {});
			updateGridCompensation();
		}
		
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

void Georeferencing::setCombinedScaleFactor(double value)
{
	Q_ASSERT(value > 0);
	double combined_scale_factor = roundScaleFactor(value);
	double auxiliary_scale_factor = roundScaleFactor(value / grid_scale_factor);
	setScaleFactors(combined_scale_factor, auxiliary_scale_factor);
}

void Georeferencing::setAuxiliaryScaleFactor(double value)
{
	Q_ASSERT(value > 0);
	double auxiliary_scale_factor = roundScaleFactor(value);
	double combined_scale_factor = roundScaleFactor(value * grid_scale_factor);
	setScaleFactors(combined_scale_factor, auxiliary_scale_factor);
}

void Georeferencing::setScaleFactors(double combined_scale_factor, double auxiliary_scale_factor)
{
	bool combined_change = combined_scale_factor != this->combined_scale_factor;
	bool auxiliary_change = auxiliary_scale_factor != this->auxiliary_scale_factor;
	if (combined_change || auxiliary_change)
	{
		this->auxiliary_scale_factor = auxiliary_scale_factor;
		this->combined_scale_factor = combined_scale_factor;
		if (combined_change)
			updateTransformation();
		if (auxiliary_change)
			emit auxiliaryScaleFactorChanged();
	}
}

void Georeferencing::setDeclination(double value)
{
	double declination = roundDeclination(value);
	double grivation = roundDeclination(value - convergence);
	setDeclinationAndGrivation(declination, grivation);
}

void Georeferencing::setGrivation(double value)
{
	double grivation = roundDeclination(value);
	double declination = roundDeclination(value + convergence);
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
		if (grivation_change)
			updateTransformation();
		
		if (declination_change)
			emit declinationChanged();
	}
}

void Georeferencing::setMapRefPoint(const MapCoord& point)
{
	if (map_ref_point != point)
	{
		map_ref_point = point;
		updateTransformation();
	}
}

void Georeferencing::setProjectedRefPoint(const QPointF& point, bool update_grivation, bool update_scale_factor)
{
	if (projected_ref_point != point || state == Normal)
	{
		projected_ref_point = point;
		bool ok = {};
		LatLon new_geo_ref_point;
		
		switch (state)
		{
		default:
			qWarning("Undefined georeferencing state");
			Q_FALLTHROUGH();
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
					updateCombinedScaleFactor();
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

double Georeferencing::getConvergence() const
{
	return convergence;
}

void Georeferencing::updateGridCompensation()
{
	convergence = 0.0;
	grid_scale_factor = 1.0;

	if (state != Normal || !isValid())
		return;

	const double delta = 1000.0; // meters

	QString local_crs_spec = QString::fromLatin1("+proj=sterea +lat_0=%1 +lon_0=%2 +ellps=WGS84 +units=m")
	        .arg(geographic_ref_point.latitude(), 0, 'f')
	        .arg(geographic_ref_point.longitude(), 0, 'f');
	ProjTransform local_proj_transform(local_crs_spec);
	if (!local_proj_transform.isValid())
		return;

	// Determine 1 km baselines west-east and south-north on the ellipsoid.
	bool ok_east_point = false;
	const QPointF east_projected_coords {delta/2, 0};
	const LatLon east_point = local_proj_transform.inverse(east_projected_coords, &ok_east_point);

	bool ok_north_point = false;
	const QPointF north_projected_coords {0, delta/2};
	const LatLon north_point = local_proj_transform.inverse(north_projected_coords, &ok_north_point);

	bool ok_west_point = false;
	const QPointF west_projected_coords {-delta/2, 0};
	const LatLon west_point = local_proj_transform.inverse(west_projected_coords, &ok_west_point);

	bool ok_south_point = false;
	const QPointF south_projected_coords {0, -delta/2};
	const LatLon south_point = local_proj_transform.inverse(south_projected_coords, &ok_south_point);

	if (!(ok_east_point && ok_north_point && ok_west_point && ok_south_point))
		return;

	// Get projected coordinates on same meridian and on same parallel around reference point.
	bool ok_east = false;
	QPointF projected_east  = toProjectedCoords(east_point,  &ok_east);
	bool ok_north = false;
	QPointF projected_north = toProjectedCoords(north_point, &ok_north);
	bool ok_west = false;
	QPointF projected_west  = toProjectedCoords(west_point,  &ok_west);
	bool ok_south = false;
	QPointF projected_south = toProjectedCoords(south_point, &ok_south);
	if (!(ok_east && ok_north && ok_west && ok_south))
		return;

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
		return;

	// This is the angle between true azimuth and grid azimuth.
	// In case of deformation, the convergence varies with direction and this is an average.
	convergence = qRadiansToDegrees(atan2(d_northing_dx - d_easting_dy,
	                                      d_easting_dx + d_northing_dy));

	// This is the scale factor from true distance to grid distance.
	// In case of deformation, the scale factor varies with direction and this is an average.
	grid_scale_factor = sqrt(determinant);
}

void Georeferencing::setGeographicRefPoint(LatLon lat_lon, bool update_grivation, bool update_scale_factor)
{
	bool geo_ref_point_changed = geographic_ref_point != lat_lon;
	if (geo_ref_point_changed || state == Normal)
	{
		geographic_ref_point = lat_lon;
		if (state != Normal)
			setState(Normal);
		
		bool ok = {};
		QPointF new_projected_ref = toProjectedCoords(lat_lon, &ok);
		if (ok && new_projected_ref != projected_ref_point)
		{
			projected_ref_point = new_projected_ref;
			updateGridCompensation();
			if (update_grivation)
				updateGrivation();
			if (update_scale_factor)
				updateCombinedScaleFactor();
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
	// Use the grivation and combined scale factor.
	QTransform transform;
	transform.translate(projected_ref_point.x(), projected_ref_point.y());
	transform.rotate(-grivation);
	double scale = combined_scale_factor * scale_denominator / 1000.0; // to meters
	transform.scale(scale, -scale);
	transform.translate(-map_ref_point.x(), -map_ref_point.y());
	
	if (to_projected != transform)
	{
		to_projected   = transform;
		from_projected = transform.inverted();
		emit transformationChanged();
	}
}

void Georeferencing::updateCombinedScaleFactor()
{
	setAuxiliaryScaleFactor(auxiliary_scale_factor);
}

void Georeferencing::initAuxiliaryScaleFactor()
{
	setCombinedScaleFactor(combined_scale_factor);
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

const QTransform& Georeferencing::mapToProjected() const
{
	return to_projected;
}

const QTransform& Georeferencing::projectedToMap() const
{
	return from_projected;
}



bool Georeferencing::setProjectedCRS(const QString& id, QString spec, std::vector<QString> params)
{
	// Default return value if no change is necessary
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
		projected_crs_id = id;
		projected_crs_spec = spec;
		if (projected_crs_spec.isEmpty())
		{
			projected_crs_parameters.clear();
			proj_transform = {};
			ok = (state != Normal);
		}
		else
		{
			projected_crs_parameters.swap(params); // params was passed by value!
			proj_transform = {projected_crs_spec};
			ok = proj_transform.isValid();
			if (ok && state != Normal)
				setState(Normal);
		}
		if (!ok)
			updateGridCompensation();
		
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
	return proj_transform.isValid() ? proj_transform.inverse(projected_coords, ok) : LatLon{};
}

QPointF Georeferencing::toProjectedCoords(const LatLon& lat_lon, bool* ok) const
{
	return proj_transform.isValid() ? proj_transform.forward(lat_lon, ok) : QPointF{};
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
	
	if (isLocal() || other->isLocal())
	{
		if (ok)
			*ok = true;
		return toMapCoordF(other->toProjectedCoords(map_coords));
	}
	
	bool ok_forward = proj_transform.isValid();
	bool ok_inverse = other->proj_transform.isValid();
	QPointF projected = other->toProjectedCoords(map_coords);
	if (ok_forward && ok_inverse) {
		// Use geographic coordinates as intermediate step to enforce
		// that coordinates are assumed to have WGS84 datum if datum is specified in only one CRS spec.
		/// \todo Use direct pipeline instead of intermediate WGS84
		bool ok_inverse, ok_forward;
		projected = proj_transform.forward(other->proj_transform.inverse(projected, &ok_inverse), &ok_forward);
	}
	if (ok)
		*ok = ok_inverse && ok_forward;
	return toMapCoordF(projected);
}

QString Georeferencing::getErrorText() const
{
	return proj_transform.errorText();
}

double Georeferencing::radToDeg(double val)
{
	return qRadiansToDegrees(val);
}

double Georeferencing::degToRad(double val)
{
	return qDegreesToRadians(val);
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
	  << " " << georef.combined_scale_factor
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
