/*
 *    Copyright 2013-2017 Kai Pastor
 *    Copyright 2018 Libor Pecháček
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

#include "ocd_georef_fields.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include <QLatin1String>
#include <QPointF>
#include <QString>
#include <QtGlobal>
#include <QtNumeric>

#include "core/crs_template.h"
// translation context
#include "fileformats/ocd_file_import.h" // IWYU pragma: keep
#include "fileformats/ocd_file_export.h" // IWYU pragma: keep

namespace OpenOrienteering {

namespace {

enum struct MapperCrs
{
	Invalid,
	Local,
	Utm,
	GaussKrueger,
	Epsg,
};

constexpr struct
{
	MapperCrs key;
	const char* value;
} crs_string_map[] {
    { MapperCrs::Local, "Local" },
    { MapperCrs::Utm, "UTM" },
    { MapperCrs::GaussKrueger, "Gauss-Krueger, datum: Potsdam" },
    { MapperCrs::Epsg, "EPSG" },
};

/**
 * OcdGrid enum lists OCD country codes which happen to be thousands in the
 * `i' field value.
 */
enum struct OcdGrid
{
	Invalid = -1,
	Local = 1,
	Utm = 2,
	Austria = 3,
	Britain = 5,
	Finland = 6,
	Germany = 8,
	Sweden = 13,
	Switzerland = 14,
	Slovenia = 15,
	Australia = 20,
	Iceland = 38,
	Czechia = 46,
	Slovakia = 47,
	Hungary = 49,
	Brazil = 52,
	Israel = 53,
	GoogleTms = 56,
};

constexpr struct
{
	const OcdGrid ocd_grid_id;
	const int ocd_zone_id, epsg_code;
} grid_zone_epsg_codes[] {
    // sort with `LC_ALL=C sort -k 1db,1 -k 2n -s -t ,`
    { OcdGrid::Australia, 1, 28348 }, // GDA94 / MGA zone 48
    { OcdGrid::Australia, 2, 28349 }, // GDA94 / MGA zone 49
    { OcdGrid::Australia, 3, 28350 }, // GDA94 / MGA zone 50
    { OcdGrid::Australia, 4, 28351 }, // GDA94 / MGA zone 51
    { OcdGrid::Australia, 5, 28352 }, // GDA94 / MGA zone 52
    { OcdGrid::Australia, 6, 28353 }, // GDA94 / MGA zone 53
    { OcdGrid::Australia, 7, 28354 }, // GDA94 / MGA zone 54
    { OcdGrid::Australia, 8, 28355 }, // GDA94 / MGA zone 55
    { OcdGrid::Australia, 9, 28356 }, // GDA94 / MGA zone 56
    { OcdGrid::Australia, 10, 28357 }, // GDA94 / MGA zone 57
    { OcdGrid::Australia, 11, 28358 }, // GDA94 / MGA zone 58
    { OcdGrid::Austria, 28, 31257 }, // MGI / Austria GK M28
    { OcdGrid::Austria, 31, 31258 }, // MGI / Austria GK M31
    { OcdGrid::Austria, 34, 31259 }, // MGI / Austria GK M34
    { OcdGrid::Brazil, 4, 22521 }, // Corrego Alegre 1970-72 / UTM zone 21S
    { OcdGrid::Brazil, 5, 22522 }, // Corrego Alegre 1970-72 / UTM zone 22S
    { OcdGrid::Brazil, 6, 22523 }, // Corrego Alegre 1970-72 / UTM zone 23S
    { OcdGrid::Brazil, 7, 22524 }, // Corrego Alegre 1970-72 / UTM zone 24S
    { OcdGrid::Brazil, 8, 22525 }, // Corrego Alegre 1970-72 / UTM zone 25S
    { OcdGrid::Brazil, 9, 5875 }, // SAD69(96) / UTM zone 18S
    { OcdGrid::Brazil, 10, 5876 }, // SAD69(96) / UTM zone 19S
    { OcdGrid::Brazil, 11, 5877 }, // SAD69(96) / UTM zone 20S
    { OcdGrid::Brazil, 12, 5531 }, // SAD69(96) / UTM zone 21S
    { OcdGrid::Brazil, 13, 5858 }, // SAD69(96) / UTM zone 22S
    { OcdGrid::Brazil, 14, 5533 }, // SAD69(96) / UTM zone 23S
    { OcdGrid::Brazil, 15, 5534 }, // SAD69(96) / UTM zone 24S
    { OcdGrid::Brazil, 16, 5535 }, // SAD69(96) / UTM zone 25S
    { OcdGrid::Brazil, 17, 31978 }, // SIRGAS 2000 / UTM zone 18S
    { OcdGrid::Brazil, 18, 31979 }, // SIRGAS 2000 / UTM zone 19S
    { OcdGrid::Brazil, 19, 31980 }, // SIRGAS 2000 / UTM zone 20S
    { OcdGrid::Brazil, 20, 31981 }, // SIRGAS 2000 / UTM zone 21S
    { OcdGrid::Brazil, 21, 31982 }, // SIRGAS 2000 / UTM zone 22S
    { OcdGrid::Brazil, 22, 31983 }, // SIRGAS 2000 / UTM zone 23S
    { OcdGrid::Brazil, 23, 31984 }, // SIRGAS 2000 / UTM zone 24S
    { OcdGrid::Brazil, 24, 31985 }, // SIRGAS 2000 / UTM zone 25S
    { OcdGrid::Britain, 0, 27700 }, // OSGB 1936
    { OcdGrid::Czechia, 1, 5514 }, // S-JTSK Krovak East North
    { OcdGrid::Czechia, 2, 28403 }, // Pulkovo 1942 / Gauss-Kruger zone 3
    { OcdGrid::Finland, 2, 2392 }, // KKJ / Finland zone 2
    { OcdGrid::Finland, 5, 3067 }, // TM35FIN, ETRS89 / ETRS-TM35FIN
    { OcdGrid::Finland, 12, 3879 }, // ETRS89 / GK25FIN
    { OcdGrid::Finland, 25, 3132 }, // ETRS89 / ETRS-GK25FIN
    { OcdGrid::Germany, 2, 31466 }, // DHDN, Gauss-Krüger Zone 2
    { OcdGrid::Germany, 3, 31467 }, // DHDN, Gauss-Krüger Zone 3
    { OcdGrid::Germany, 4, 31468 }, // DHDN, Gauss-Krüger Zone 4
    { OcdGrid::Germany, 5, 31469 }, // DHDN, Gauss-Krüger Zone 5
    { OcdGrid::GoogleTms, 0, 900913 }, // Google Mercator
    { OcdGrid::Hungary, 0, 23700 }, // HD72 / EOV
    { OcdGrid::Iceland, 4, 3057 }, // ISN93 / Lambert 1993
    { OcdGrid::Israel, 0, 2039 }, // Israel 1993 / Israeli TM Grid
    { OcdGrid::Slovakia, 1, 5514 }, // S-JTSK Krovak East North
    { OcdGrid::Slovenia, 1, 3912 }, // MGI 1901 / Slovene National Grid
    { OcdGrid::Sweden, 2, 3006 }, // SWEREF99 TM
    { OcdGrid::Sweden, 3, 3007 }, // SWEREF99 12 00
    { OcdGrid::Sweden, 4, 3008 }, // SWEREF99 13 30
    { OcdGrid::Sweden, 5, 3009 }, // SWEREF99 15 00
    { OcdGrid::Sweden, 6, 3010 }, // SWEREF99 16 30
    { OcdGrid::Sweden, 7, 3011 }, // SWEREF99 18 00
    { OcdGrid::Sweden, 8, 3012 }, // SWEREF99 14 15
    { OcdGrid::Sweden, 9, 3013 }, // SWEREF99 15 45
    { OcdGrid::Sweden, 10, 3014 }, // SWEREF99 17 15
    { OcdGrid::Sweden, 11, 3015 }, // SWEREF99 18 45
    { OcdGrid::Sweden, 12, 3016 }, // SWEREF99 20 15
    { OcdGrid::Sweden, 13, 3017 }, // SWEREF99 21 45
    { OcdGrid::Sweden, 14, 3018 }, // SWEREF99 23 15
    { OcdGrid::Switzerland, 1, 21781 }, // CH1903 / LV03 -- Swiss CH1903 / LV03
    { OcdGrid::Switzerland, 2, 2056 }, // CH1903+ / LV95 -- Swiss CH1903+ / LV95
};

/**
 * Plain translation from OCD grid/zone combination into a Mapper
 * scheme.
 *
 * @see toOcd
 */
std::pair<const MapperCrs, const int> fromOcd(const int combined_ocd_grid_zone)
{
	// while OCD codes can be negative, we are keeping grid_id always positive
	// to match the enums; signum is carried in zone_id
	const int ocd_grid_id = qAbs(combined_ocd_grid_zone / 1000),
	          ocd_zone_id = qAbs(combined_ocd_grid_zone % 1000)
	                        * (combined_ocd_grid_zone < 0 ? -1 : 1);

	switch (static_cast<OcdGrid>(ocd_grid_id))
	{
	case OcdGrid::Local:
		return { MapperCrs::Local, 0 };

	case OcdGrid::Utm:
		if (qAbs(ocd_zone_id) >= 1 && qAbs(ocd_zone_id) <= 60)
			return { MapperCrs::Utm, ocd_zone_id };
		break;

	case OcdGrid::Germany:
		// DHDN, Gauss-Krüger, Zone 2-5
		if (ocd_zone_id >= 2 && ocd_zone_id <= 5)
			return { MapperCrs::GaussKrueger, ocd_zone_id };
		break;

	default:
		for (const auto& record : grid_zone_epsg_codes)
		{
			if (static_cast<int>(record.ocd_grid_id) == ocd_grid_id
			    && record.ocd_zone_id == ocd_zone_id)
				return { MapperCrs::Epsg, record.epsg_code };
		}
	}

	return { MapperCrs::Invalid, 0 };
}

/**
 * Imports string 1039 field i.
 */
void applyGridAndZone(Georeferencing& georef,
                      const int combined_ocd_grid_zone,
                      const std::function<void(const QString&)>& warning_handler)
{
	// Mapper has multiple ways of expressing some of the CRS's
	// we are working with the first match
	auto result = fromOcd(combined_ocd_grid_zone);

	if (result.first == MapperCrs::Invalid)
	{
		warning_handler(::OpenOrienteering::OcdFileImport::tr(
		                    "Could not load the coordinate reference system '%1'.")
		                .arg(combined_ocd_grid_zone));
		return;
	}

	if (result.first == MapperCrs::Local)
		return;

	// look up CRS template by its unique name
	auto entry_it = std::find_if(std::begin(crs_string_map),
	                             std::end(crs_string_map),
	                             [&result](auto e) { return e.key == result.first; });
	if (entry_it == std::end(crs_string_map))
	{
		qWarning("Internal program error: unknown CRS id (%d)", static_cast<int>(result.first));
		return;
	}
	const QString crs_id_string = QLatin1String(entry_it->value);
	auto crs_template = CRSTemplateRegistry().find(crs_id_string);

	if (!crs_template)
	{
		qWarning("Internal program error: unknown CRS description string (%s)", qUtf8Printable(crs_id_string));
		return;
	}

	// get PROJ.4 spec template from the CRS
	auto spec = crs_template->specificationTemplate();

	// we are handling templates with single parameter only get template's first
	// parameter - it can be an instance of UTMZoneParameter, IntRangeParameter
	// (with "factor" and "bias") or FullSpecParameter
	auto param = crs_template->parameters()[0];

	// run every parameter value through the parameter processor which
	// converts it into a string list that can be consumed by the above
	// CRS specification template
	const QString crs_param_string =
	        result.first != MapperCrs::Utm || result.second >= 0
	        ? QString::number(result.second) // common case, plain number
	        : QString::number(-result.second) + QLatin1String(" S"); // UTM South zones

	for (const auto& spec_value : param->specValues(crs_param_string))
		spec = spec.arg(spec_value);

	// `spec` and the pair <`crs_id_string`, `crs_param_string`> both describe
	// the same thing at this place
	georef.setProjectedCRS(crs_id_string, spec, { crs_param_string });
}

/**
 * Combine grid/zone components into OCD code.
 */
int combineGridZone(const OcdGrid grid_id, const int zone_id)
{
	int i = static_cast<int>(grid_id) * 1000 + qAbs(zone_id);
	// propagate signum which we carry in zone_id
	return i * (zone_id < 0 ? -1 : 1);
}

/**
 * Provide an OCD grid/zone combination for a Mapper georeferencing.
 *
 * @see fromOcd
 */
int toOcd(const MapperCrs crs_unique_id,
          const int crs_param)
{
	switch (crs_unique_id)
	{
	case MapperCrs::Local:
		return combineGridZone(OcdGrid::Local, 0);
		break;

	case MapperCrs::Utm:
		if (qAbs(crs_param) >= 1 && qAbs(crs_param) <= 60)
			return combineGridZone(OcdGrid::Utm, crs_param);
		break;

	case MapperCrs::GaussKrueger:
		if (crs_param >= 2 && crs_param <= 5)
			return combineGridZone(OcdGrid::Germany, crs_param);
		break;

	case MapperCrs::Epsg:
		// handle UTM separately, the rest is in the table
		if (crs_param >= 32601 && crs_param <= 32660)
			return combineGridZone(OcdGrid::Utm, crs_param - 32600);
		else if (crs_param >= 32701 && crs_param <= 32760)
			return combineGridZone(OcdGrid::Utm, 32700 - crs_param);

		for (const auto& record : grid_zone_epsg_codes)
		{
			if (record.epsg_code == crs_param)
				return combineGridZone(record.ocd_grid_id, record.ocd_zone_id);
		}
		break;
	default:
		qWarning("Internal program error: translateMapperToOcd() should not"
		         " be fed with invalid values (%d)", static_cast<int>(crs_unique_id));
	}

	return combineGridZone(OcdGrid::Invalid, 0);
}

}  // anonymous namespace

bool operator==(const OcdGeorefFields& lhs, const OcdGeorefFields& rhs)
{
	return lhs.i == rhs.i
	        && lhs.m == rhs.m
	        && lhs.x == rhs.x
	        && lhs.y == rhs.y
	        && ((qIsNaN(lhs.a) && qIsNaN(rhs.a)) || qAbs(lhs.a - rhs.a) < 5e-9) // 8-digit precision or both NaN's
	        && lhs.r == rhs.r;
}

void OcdGeorefFields::setupGeoref(Georeferencing& georef,
                                  const std::function<void (const QString&)>& warning_handler) const
{
	if (m > 0)
		georef.setScaleDenominator(m);

	if (qIsFinite(a) && qAbs(a) >= 0.01)
		georef.setGrivation(a);

	if (r)
		applyGridAndZone(georef, i, warning_handler);

	QPointF proj_ref_point(x, y);
	georef.setProjectedRefPoint(proj_ref_point, false);
}

OcdGeorefFields OcdGeorefFields::fromGeoref(const Georeferencing& georef,
                                            const std::function<void (const QString&)>& warning_handler)
{
	OcdGeorefFields retval;

	// store known values early, they can be useful even if CRS translation fails
	retval.a = georef.getGrivation();
	retval.m = georef.getScaleDenominator();
	const QPointF offset(georef.toProjectedCoords(MapCoord{}));
	retval.x = qRound(offset.x()); // OCD easting and northing is integer
	retval.y = qRound(offset.y());

	// attempt translation from Mapper CRS reference into OCD one
	auto crs_id_string = georef.getProjectedCRSId();
	auto crs_param_strings = georef.getProjectedCRSParameters();
	auto report_warning = [&]() {
		QStringList params;
		std::for_each(begin(crs_param_strings), end(crs_param_strings),
		              [&](const QString& s) { params.append(s); });
		warning_handler(::OpenOrienteering::OcdFileExport::tr(
		                    "Could not translate coordinate reference system '%1:%2'.")
		                .arg(crs_id_string, params.join(QLatin1Char(','))));
	};

	// translate CRS string into a MapperCrs value
	auto entry_it = std::find_if(std::begin(crs_string_map),
	                             std::end(crs_string_map),
	                             [&crs_id_string](auto e) { return QLatin1String(e.value) == crs_id_string; });
	if (entry_it == std::end(crs_string_map))
	{
		report_warning();
		return retval;
	}
	const MapperCrs crs_id = entry_it->key;

	int crs_param = 0;

	if (crs_id != MapperCrs::Local)
	{
		// analyze CRS parameters
		if (crs_param_strings.empty())
		{
			report_warning();
			return retval;
		}

		bool param_conversion_ok;
		auto param_string = crs_param_strings[0];
		switch (crs_id)
		{
		case MapperCrs::Utm:
			if (param_string.endsWith(QLatin1String(" S")))
			{
				param_string.remove(QLatin1String(" S"));
				param_string.prepend(QLatin1String("-"));
			}
			else
			{
				param_string.remove(QLatin1String(" N"));
			}
			// fallthrough

		default:
			crs_param = param_string.toInt(&param_conversion_ok);
		}

		if (!param_conversion_ok)
		{
			report_warning();
			return retval;
		}
	}

	auto combined_ocd_grid_zone = toOcd(crs_id, crs_param);

	if (combined_ocd_grid_zone/1000 == static_cast<int>(OcdGrid::Invalid))
	{
		report_warning();
		return retval;
	}

	retval.i = combined_ocd_grid_zone;
	retval.r = 1; // we are done

	return retval;
}

}  // namespace OpenOrienteering
