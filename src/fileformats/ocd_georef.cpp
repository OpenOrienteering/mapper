/*
 *    Copyright 2013-2017 Kai Pastor
 *    Copyright 2017 Libor Pecháček
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

#include "ocd_georef.h"

#include <list>

#include <QObject>

#include "core/crs_template.h"

Georeferencing OcdGeoref::georefFromString(const QString& param_string,
                                           std::list<QString>& warnings)
{
	const QChar* unicode = param_string.unicode();

	Georeferencing georef;
	QString combined_grid_zone;
	QPointF proj_ref_point;
	bool x_ok = false, y_ok = false;

	int i = param_string.indexOf(QLatin1Char('\t'), 0);
	; // skip first word for this entry type
	while (i >= 0)
	{
		bool ok;
		int next_i = param_string.indexOf(QLatin1Char('\t'), i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
		case 'm':
			{
				double scale = param_value.toDouble(&ok);
				if (ok && scale >= 0)
					georef.setScaleDenominator(qRound(scale));
			}
			break;
		case 'a':
			{
				double angle = param_value.toDouble(&ok);
				if (ok && qAbs(angle) >= 0.01)
					georef.setGrivation(angle);
			}
			break;
		case 'x':
			proj_ref_point.setX(param_value.toDouble(&x_ok));
			break;
		case 'y':
			proj_ref_point.setY(param_value.toDouble(&y_ok));
			break;
		case 'i':
			combined_grid_zone = param_value;
			break;
		case '\t':
			// empty item, fall through
		default:
			; // nothing
		}
		i = next_i;
	}

	if (!combined_grid_zone.isEmpty())
	{
		applyGridAndZone(georef, combined_grid_zone, warnings);
	}

	if (x_ok && y_ok)
	{
		georef.setProjectedRefPoint(proj_ref_point, false);
	}

	return georef;
}

void OcdGeoref::applyGridAndZone(Georeferencing& georef,
                                        const QString& combined_grid_zone,
                                        std::list<QString>& warnings)
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
		warnings.push_back(QObject::tr("Could not load the coordinate reference system '%1'.").arg(combined_grid_zone));
	}
	else
	{
		georef.setProjectedCRS(id, spec, std::move(values));
	}
}
