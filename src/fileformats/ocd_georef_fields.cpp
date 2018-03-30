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
#include <QtGlobal>
#include <QtNumeric>

#include "core/crs_template.h"
// translation context
#include "fileformats/ocd_file_import.h" // IWYU pragma: keep

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
	Local = 1,
	Utm = 2,
	Finland = 6,
	Germany = 8,
	Switzerland = 14,
};

constexpr struct
{
	const OcdGrid ocd_grid_id;
	const int ocd_zone_id, epsg_code;
} grid_zone_epsg_codes[] {
    { OcdGrid::Finland, 5, 3067 }, // TM35FIN, ETRS89 / ETRS-TM35FIN
    { OcdGrid::Switzerland, 1, 21781 }, // CH1903/LV03, CH1903/LV03
    { OcdGrid::Germany, 2, 31466 }, // DHDN, Gauss-Krüger Zone 2
    { OcdGrid::Germany, 3, 31467 }, // DHDN, Gauss-Krüger Zone 3
    { OcdGrid::Germany, 4, 31468 }, // DHDN, Gauss-Krüger Zone 4
    { OcdGrid::Germany, 5, 31469 }, // DHDN, Gauss-Krüger Zone 5
};

/**
 * Plain translation from OCD grid/zone combination into a Mapper
 * scheme.
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

}  // anonymous namespace

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

}  // namespace OpenOrienteering
