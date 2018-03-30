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
#include <vector>

#include <QObject>
#include <QPointF>
#include <QString>
#include <QtGlobal>
#include <QtTest>

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
	        && qAbs(lhs.ref_point.x() - rhs.ref_point.x()) < 5e-9
	        && qAbs(lhs.ref_point.y() - rhs.ref_point.y()) < 5e-9
	        && lhs.crs_id == rhs.crs_id
	        && std::equal(lhs.crs_parameters.begin(), lhs.crs_parameters.begin(), rhs.crs_parameters.begin());
}

namespace QTest
{

template<>
char* toString(const GeorefCoreData &r)
{
	auto ret = QString(QStringLiteral("%1;%2;%3;%4,%5;%6:%7"))
	           .arg(r.scale)
	           .arg(r.declination)
	           .arg(r.grivation)
	           .arg(r.ref_point.x(), 0, 'f', 8)
	           .arg(r.ref_point.y(), 0, 'f', 8)
	           .arg(r.crs_id)
	           .arg(r.crs_parameters.join(QLatin1Char(',')));

	return qstrdup(qUtf8Printable(ret));
}

}  // namesapce QTest

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
		for (auto s : georef.getProjectedCRSParameters())
			crs_params.push_back(s);

		auto converted = GeorefCoreData {
		                 georef.getScaleDenominator(),
		                 georef.getDeclination(),
		                 georef.getGrivation(),
		                 georef.getProjectedRefPoint(),
		                 georef.getProjectedCRSId(),
		                 crs_params };

		QCOMPARE(converted, georef_result);
		QCOMPARE(warnings.size(), n_warnings);
	}
};

QTEST_GUILESS_MAIN(GeoreferenceMappingTest)

#include "georef_ocd_mapping_t.moc"  // IWYU pragma: keep
