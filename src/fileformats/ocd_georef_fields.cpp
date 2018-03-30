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

#include <vector>

#include <QLatin1String>
#include <QPointF>
#include <QStringRef>

#include "core/crs_template.h"
#include "core/georeferencing.h"

namespace OpenOrienteering {

namespace {

/**
 * Imports string 1039 field i.
 */
void applyGridAndZone(Georeferencing& georef,
                      const QString& combined_grid_zone,
                      const std::function<void(const QString&)>& warning_handler)
{
	bool zone_ok = false;
	const CRSTemplate* crs_template = nullptr;
	QString id;
	QString spec;
	std::vector<QString> values;

	if (combined_grid_zone.startsWith(QLatin1String("20")))
	{
		auto zone = combined_grid_zone.midRef(2).toUInt(&zone_ok);
		zone_ok &= (zone >= 1 && zone <= 60);
		if (zone_ok)
		{
			id = QLatin1String{"UTM"};
			crs_template = CRSTemplateRegistry().find(id);
			values.reserve(1);
			values.push_back(QString::number(zone));
		}
	}
	else if (combined_grid_zone.startsWith(QLatin1String("80")))
	{
		auto zone = combined_grid_zone.midRef(2).toUInt(&zone_ok);
		if (zone_ok)
		{
			id = QLatin1String{"Gauss-Krueger, datum: Potsdam"};
			crs_template = CRSTemplateRegistry().find(id);
			values.reserve(1);
			values.push_back(QString::number(zone));
		}
	}
	else if (combined_grid_zone == QLatin1String("6005"))
	{
		id = QLatin1String{"EPSG"};
		crs_template = CRSTemplateRegistry().find(id);
		values.reserve(1);
		values.push_back(QLatin1String{"3067"});
	}
	else if (combined_grid_zone == QLatin1String("14001"))
	{
		id = QLatin1String{"EPSG"};
		crs_template = CRSTemplateRegistry().find(id);
		values.reserve(1);
		values.push_back(QLatin1String{"21781"});
	}
	else if (combined_grid_zone == QLatin1String("1000"))
	{
		return;
	}

	if (crs_template)
	{
		spec = crs_template->specificationTemplate();
		auto param = crs_template->parameters().begin();
		for (const auto& value : values)
		{
			for (const auto& spec_value : (*param)->specValues(value))
			{
				spec = spec.arg(spec_value);
			}
			++param;
		}
	}

	if (spec.isEmpty())
	{
		warning_handler(QCoreApplication::translate("OcdFileImport", "Could not load the coordinate reference system '%1'.").arg(combined_grid_zone));
	}
	else
	{
		georef.setProjectedCRS(id, spec, std::move(values));
	}
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
		applyGridAndZone(georef, QString::number(i), warning_handler);

	QPointF proj_ref_point(x, y);
	georef.setProjectedRefPoint(proj_ref_point, false);
}

}  // namespace OpenOrienteering
