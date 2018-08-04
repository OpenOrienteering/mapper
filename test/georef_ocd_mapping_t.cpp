/*
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

#include <algorithm>
#include <iterator>
#include <vector>

#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QLatin1Char>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QStringList>

#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "fileformats/ocd_georef_fields.h"
#include "fileformats/xml_file_format.h"

using namespace OpenOrienteering;

// mock stuff to satisfy link-time dependencies
int XMLFileFormat::active_version = 6;

Q_DECLARE_METATYPE(OcdGeorefFields)

struct GeorefCoreData
{
	unsigned int scale;
	double declination;
	double grivation;
	QPointF ref_point;
	QString crs_id;
	QStringList crs_parameters;
};

Q_DECLARE_METATYPE(GeorefCoreData)

bool operator==(const GeorefCoreData& lhs, const GeorefCoreData& rhs)
{
	return lhs.scale == rhs.scale
	        && lhs.declination == rhs.declination
	        && lhs.grivation == rhs.grivation
	        && lhs.ref_point == rhs.ref_point
	        && lhs.crs_id == rhs.crs_id
	        && std::equal(lhs.crs_parameters.begin(), lhs.crs_parameters.begin(), rhs.crs_parameters.begin());
}

namespace QTest
{

template<>
char* toString(const GeorefCoreData &r) // NOLINT : declared parameter name is 't'
{
	auto ret = QString(QStringLiteral("%1;%2;%3;%4,%5;%6:%7"))
	           .arg(r.scale)
	           .arg(r.declination)
	           .arg(r.grivation)
	           .arg(r.ref_point.x(), 0, 'f', 8)
	           .arg(r.ref_point.y(), 0, 'f', 8)
	           .arg(r.crs_id,
	                r.crs_parameters.join(QLatin1Char(',')));

	return qstrdup(qUtf8Printable(ret));
}

template<>
char* toString(const OcdGeorefFields &f) // NOLINT : declared parameter name is 't'
{
	auto ret = QString(QStringLiteral("m:%1; x:%2; y:%3; a:%4; i:%5; r:%6"))
	           .arg(f.m)
	           .arg(f.x)
	           .arg(f.y)
	           .arg(f.a, 0, 'f', 8)
	           .arg(f.i)
	           .arg(f.r);

	return qstrdup(qUtf8Printable(ret));
}

}  // namesapce QTest

namespace {

void commonTestData()
{
	QTest::addColumn<OcdGeorefFields>("ocd_string_fields");
	QTest::addColumn<GeorefCoreData>("georef_result");
	QTest::addColumn<unsigned>("n_warnings"); // number of warnings expected

	QTest::newRow("Sibelius Monumentti - Finland, TM35FIN, ETRS89/ETRS-TM35FIN")
	        << OcdGeorefFields { 10.7, 15000, 384268, 6673512, 6005, 1 }
	        << GeorefCoreData { 15000, 10.7, 10.7, { 384268, 6673512 }, QStringLiteral("EPSG"), { QStringLiteral("3067") } }
	        << 0U;
	QTest::newRow("CERN - Switzerland, CH1903")
	        << OcdGeorefFields { 2.72000000, 15000, 495133, 129609, 14001, 1 }
	        << GeorefCoreData { 15000, 2.72, 2.72, { 495133, 129609 }, QStringLiteral("EPSG"), { QStringLiteral("21781") } }
	        << 0U;
	QTest::newRow("Launch Pad 39A - USA, Florida, WGS 84/UTM zone 17")
	        << OcdGeorefFields { -7.06000000, 15000, 538705, 3164567, 2017, 1 }
	        << GeorefCoreData { 15000, -7.06, -7.06, { 538705, 3164567 }, QStringLiteral("UTM"), { QStringLiteral("17") } }
	        << 0U;
	QTest::newRow("Brandenburger Tor - Germany, DHDN/3-degree Gauss-Kruger zone 4")
	        << OcdGeorefFields { 2.84, 15000, 4571442, 5807888, 8004, 1 }
	        << GeorefCoreData { 15000, 2.84, 2.84, { 4571442, 5807888 }, QStringLiteral("Gauss-Krueger, datum: Potsdam"), { QStringLiteral("4") } }
	        << 0U;
	QTest::newRow("Brandenburger Tor - Germany, WGS 84/UTM zone 33N")
	        << OcdGeorefFields { 5.22000000, 15000, 367205, 5807281, 2033, 1 }
	        << GeorefCoreData { 15000, 5.22, 5.22, { 367205, 5807281 }, QStringLiteral("UTM"), { QStringLiteral("33") } }
	        << 0U;
	QTest::newRow("Nasca airport - Peru, WGS 84/UTM zone 18S")
	        << OcdGeorefFields { -2.70000000, 15000, 504606, 8358028, -2018, 1 }
	        << GeorefCoreData { 15000, -2.70, -2.70, { 504606, 8358028 }, QStringLiteral("UTM"), { QStringLiteral("18 S") } }
	        << 0U;
	QTest::newRow("Wellington airport - New Zealand, WGS 84/UTM zone 60S")
	        << OcdGeorefFields { 21.14000000, 15000, 316493, 5423682, -2060, 1 }
	        << GeorefCoreData { 15000, 21.14, 21.14, { 316493, 5423682 }, QStringLiteral("UTM"), { QStringLiteral("60 S") } }
	        << 0U;
}

}  // anonymous namespace

class GeoreferenceMappingTest : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		// empty
	}
	void testOcdToMapper_data()
	{
		commonTestData();

		QTest::newRow("OCD CRS Local")
		        << OcdGeorefFields { 15.35000000, 15000, 5556666, 6667777, 1000, 1 }
		        << GeorefCoreData { 15000, 15.35, 15.35, { 5556666, 6667777 }, QStringLiteral("Local"), { } }
		        << 0U;
		// exercise error paths
		QTest::newRow("Unknown OCD grid/zone combination")
		        << OcdGeorefFields { 15.35000000, 15000, 5556666, 6667777, 99999, 1 }
		        << GeorefCoreData { 15000, 15.35, 15.35, { 5556666, 6667777 }, QStringLiteral("Local"), { } }
		        << 1U;
	}

	void testOcdToMapper()
	{
		QFETCH(OcdGeorefFields, ocd_string_fields);
		QFETCH(GeorefCoreData, georef_result);
		QFETCH(unsigned, n_warnings);

		std::vector<QString> warnings;
		auto add_warning = [&warnings](auto w) { warnings.push_back(w); };
		Georeferencing georef;

		ocd_string_fields.setupGeoref(georef, add_warning);

		QStringList crs_params;
		for (const auto& s : georef.getProjectedCRSParameters())
			crs_params.push_back(s);

		auto converted = GeorefCoreData {
		                 georef.getScaleDenominator(),
		                 georef.getDeclination(),
		                 georef.getGrivation(),
		                 georef.getProjectedRefPoint(),
		                 georef.getProjectedCRSId(),
		                 crs_params };

		QCOMPARE(converted, georef_result);
		QCOMPARE(unsigned(warnings.size()), n_warnings);
	}

	void testMapperToOcd_data()
	{
		commonTestData();

		QTest::newRow("Mapper CRS Local")
		        << OcdGeorefFields { 21.32000000, 15000, 0, 0, 1000, 1 }
		        << GeorefCoreData { 15000, 21.32, 21.32, { 0, 0 }, QStringLiteral("Local"), { } }
		        << 0U;
		QTest::newRow("Mapper UTM 41N expressed as EPSG code")
		        << OcdGeorefFields { 2.30000000, 10000, 285016, 5542944, 2041, 1 }
		        << GeorefCoreData { 10000, 2.3, 2.3, { 285016, 5542944 }, QStringLiteral("EPSG"), { QStringLiteral("32641") } }
		        << 0U;
		QTest::newRow("Mapper UTM 21S expressed as EPSG code")
		        << OcdGeorefFields { -2.70000000, 15000, 285016, 4457056, -2021, 1 }
		        << GeorefCoreData { 15000, -2.70, -2.70, { 285016, 4457056 }, QStringLiteral("EPSG"), { QStringLiteral("32721") } }
		        << 0U;
		// exercise error paths
		QTest::newRow("Invalid Mapper georef - missing param")
		        << OcdGeorefFields { 2.3, 10000, 50, 60, 1000, 0 }
		        << GeorefCoreData { 10000, 2.3, 2.3, { 50, 60 }, QStringLiteral("EPSG"), { } }
		        << 1U;
		QTest::newRow("Invalid Mapper georef - unparsable param")
		        << OcdGeorefFields { 2.3, 10000, 50, 60, 1000, 0 }
		        << GeorefCoreData { 10000, 2.3, 2.3, { 50, 60 }, QStringLiteral("EPSG"), { QStringLiteral("not_an_int") } }
		        << 1U;
		QTest::newRow("Invalid Mapper georef - non-CRS EPSG code")
		        << OcdGeorefFields { 2.3, 10000, 50, 60, 1000, 0 }
		        << GeorefCoreData { 10000, 2.3, 2.3, { 50, 60 }, QStringLiteral("EPSG"), { QStringLiteral("1080") } }
		        << 1U;
		QTest::newRow("Invalid Mapper georef - invalid CRS name")
		        << OcdGeorefFields { 2.3, 10000, 50, 60, 1000, 0 }
		        << GeorefCoreData { 10000, 2.3, 2.3, { 50, 60 }, QStringLiteral("no_such_crs"), { } }
		        << 1U;
	}

	void testMapperToOcd()
	{
		QFETCH(OcdGeorefFields, ocd_string_fields);
		QFETCH(GeorefCoreData, georef_result);
		QFETCH(unsigned, n_warnings);

		std::vector<QString> warnings;
		auto add_warning = [&warnings](auto w) { warnings.push_back(w); };
		Georeferencing georef;

		auto crs_template = CRSTemplateRegistry().find(georef_result.crs_id);

		std::vector<QString> crs_params;
		crs_params.insert(end(crs_params),
		                  georef_result.crs_parameters.cbegin(),
		                  georef_result.crs_parameters.cend());

		QString spec;
		if (crs_template)
		{
			spec = crs_template->specificationTemplate();
			auto param = begin(crs_template->parameters());

			for (const auto& value : crs_params)
			{
				for (const auto& spec_value : (*param)->specValues(value))
				{
					spec = spec.arg(spec_value);
				}
				++param;
			}
		}

		georef.setProjectedCRS(georef_result.crs_id, spec, crs_params);
		georef.setProjectedRefPoint(georef_result.ref_point, false);
		georef.setScaleDenominator(georef_result.scale);
		georef.setDeclination(georef_result.declination);
		georef.setGrivation(georef_result.grivation);

		auto converted = OcdGeorefFields::fromGeoref(georef, add_warning);

		QCOMPARE(converted, ocd_string_fields);
		QCOMPARE(unsigned(warnings.size()), n_warnings);
	}
};

QTEST_GUILESS_MAIN(GeoreferenceMappingTest)

#include "georef_ocd_mapping_t.moc"  // IWYU pragma: keep
