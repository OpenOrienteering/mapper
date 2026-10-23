/*
 *    Copyright 2025, 2026 Matthias Kühlewein
 *    Copyright 2025 Kai Pastor
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

#include <QtTest>
#include <QByteArray>
#include <QDataStream>
#include <QObject>

#include "core/map.h"
#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"

namespace {
	constexpr int convAngle(int angle) { return angle * 160; }
}

using namespace OpenOrienteering;

Q_DECLARE_METATYPE(MapCoordVector)

/**
 * @test Tests object functions.
 */
class ObjectTest : public QObject
{
	Q_OBJECT
	
private slots:
	void objectLengthTest_data()
	{
		QTest::addColumn<int>("map_scale");
		QTest::addColumn<MapCoordVector>("coords");
		QTest::addColumn<float>("paper_length");
		QTest::addColumn<float>("paper_area");
		
		auto coords = MapCoordVector{
		    MapCoord{0.0, 0.0}, MapCoord{0.0, 5.0}, MapCoord{10.0, 5.0}, MapCoord{10.0, 0.0}, MapCoord{0.0, 0.0, MapCoord::ClosePoint}
		};
		
		QTest::newRow("Line object 1 at 1:10000") << 10000 << coords << 30.0f << 50.0f;
		QTest::newRow("Line object 1 at 1:15000") << 15000 << coords << 30.0f << 50.0f;
	}
	
	void objectLengthTest()
	{
		QFETCH(int, map_scale);
		QFETCH(MapCoordVector, coords);
		QFETCH(float, paper_length);
		QFETCH(float, paper_area);
		
		Map map;
		map.setScaleDenominator(map_scale);
		
		PathObject obj;
		QCOMPARE(obj.getPaperLength(), 0.0f);
		QCOMPARE(obj.calculatePaperArea(), 0.0f);
		QVERIFY(obj.isAreaTooSmall() == false);
		QVERIFY(obj.isLineTooShort() == false);
		
		auto line_symbol = new LineSymbol();
		map.addSymbol(line_symbol, 0);
		PathObject rectangle{line_symbol, coords, &map};
		rectangle.updatePathCoords();
		
		QCOMPARE(rectangle.getPaperLength(), paper_length);
		QCOMPARE(rectangle.getRealLength(), paper_length*map_scale/1000.0f);
		
		line_symbol->setMinimumLength(paper_length*1000 - 1);
		QVERIFY(rectangle.isLineTooShort() == false);
		
		line_symbol->setMinimumLength(paper_length*1000 + 1);
		QVERIFY(rectangle.isLineTooShort() == true);
		
		QCOMPARE(rectangle.calculatePaperArea(), paper_area);
		QCOMPARE(rectangle.calculateRealArea(), paper_area*(map_scale/1000.0f)*(map_scale/1000.0f));
	}
	
	
	void objectAreaTest_data()
	{
		QTest::addColumn<int>("map_scale");
		QTest::addColumn<MapCoordVector>("coords");
		QTest::addColumn<float>("paper_length");
		QTest::addColumn<float>("paper_area");
		
		auto coords1 = MapCoordVector{
		    MapCoord{0.0, 0.0}, MapCoord{0.0, 5.0}, MapCoord{10.0, 5.0}, MapCoord{10.0, 0.0}, MapCoord{0.0, 0.0, MapCoord::ClosePoint}
		};
		
		QTest::newRow("Area object 1 at 1:10000") << 10000 << coords1 << 30.0f << 50.0f;
		QTest::newRow("Area object 1 at 1:15000") << 15000 << coords1 << 30.0f << 50.0f;
		
		auto coords2 = MapCoordVector{
		    MapCoord{0.0, 0.0}, MapCoord{0.0, 5.0}, MapCoord{10.0, 5.0}, MapCoord{10.0, 0.0}, MapCoord{0.0, 0.0, MapCoord::ClosePoint|MapCoord::HolePoint},
		    MapCoord{5.0, 2.0}, MapCoord{5.0, 4.0}, MapCoord{8.0, 4.0}, MapCoord{8.0, 2.0}, MapCoord{5.0, 2.0, MapCoord::ClosePoint}
		};
		
		QTest::newRow("Area object 2 at 1:10000") << 10000 << coords2 << 30.0f << 44.0f;
	}
	
	void objectAreaTest()
	{
		QFETCH(int, map_scale);
		QFETCH(MapCoordVector, coords);
		QFETCH(float, paper_length);
		QFETCH(float, paper_area);
		
		Map map;
		map.setScaleDenominator(map_scale);
		
		PathObject obj;
		QCOMPARE(obj.getPaperLength(), 0.0f);
		QCOMPARE(obj.calculatePaperArea(), 0.0f);
		QVERIFY(obj.isAreaTooSmall() == false);
		QVERIFY(obj.isLineTooShort() == false);
		
		auto area_symbol = new AreaSymbol();
		map.addSymbol(area_symbol, 0);
		PathObject rectangle_area{area_symbol, coords, &map};
		rectangle_area.updatePathCoords();
		
		QCOMPARE(rectangle_area.getPaperLength(), paper_length);
		QCOMPARE(rectangle_area.getRealLength(), paper_length*map_scale/1000.0f);
		
		QCOMPARE(rectangle_area.calculatePaperArea(), paper_area);
		QCOMPARE(rectangle_area.calculateRealArea(), paper_area*(map_scale/1000.0f)*(map_scale/1000.0f));
		
		area_symbol->setMinimumArea(paper_area*1000 - 1);
		QVERIFY(rectangle_area.isAreaTooSmall() == false);
		
		area_symbol->setMinimumArea(paper_area*1000 + 1);
		QVERIFY(rectangle_area.isAreaTooSmall() == true);
	}
	
	void calcTextCoordTransformTest()
	{
		struct RotatableTextSymbol : public TextSymbol {
			RotatableTextSymbol() : TextSymbol() { setRotatable (true); }
		} text_symbol;
		QVERIFY(text_symbol.isRotatable());
		QVERIFY(text_symbol.calculateInternalScaling() > 0);
		
		TextObject object(&text_symbol);
		object.setText(QLatin1String("ABC"));
		object.setRotation(1.5);
		
		// The anchor on the map is 0,0 on the text, regardless of rotation.
		auto const anchor_map = QPointF{10.0, 10.0};
		auto const anchor_text = QPointF{0.0, 0.0};
		object.setAnchorPosition(MapCoordF(anchor_map));
		
		auto to_text = object.calcMapToTextTransform();
		QVERIFY(!to_text.isIdentity());
		
		auto to_map = object.calcTextToMapTransform();
		QVERIFY(!to_map.isIdentity());
		
		QCOMPARE(to_text.map(anchor_map), anchor_text);
		QCOMPARE(to_map.map(anchor_text), anchor_map);
		
		// Another point in the rotated text, and subject to rotation.
		auto const anchor_text_2 = QPointF{10.0, 5.0};
		QCOMPARE(to_text.map(to_map.map(anchor_text_2)), anchor_text_2);
	}
	
	void cutPointCircleTest()
	{
		PointSymbol point_symbol;
		PointObject object(&point_symbol);
		
		auto cut_circle = object.asPoint()->getCutCircle();
		QVERIFY(cut_circle.getNumArcs() == 0);
		
		cut_circle.addGap(45, 90);
		QVERIFY(cut_circle.getNumArcs() == 1);
		QVERIFY(!cut_circle.isAngleInAnyGap(44));
		QVERIFY(cut_circle.isAngleInAnyGap(45));
		QVERIFY(cut_circle.isAngleInAnyGap(90));
		QVERIFY(!cut_circle.isAngleInAnyGap(91));
		
		cut_circle.addGap(10, 20);
		QVERIFY(cut_circle.getNumArcs() == 2);
		QVERIFY(!cut_circle.isAngleInAnyGap(9));
		QVERIFY(cut_circle.isAngleInAnyGap(10));
		QVERIFY(cut_circle.isAngleInAnyGap(20));
		QVERIFY(!cut_circle.isAngleInAnyGap(21));
		
		cut_circle.addGap(30, 40);
		QVERIFY(cut_circle.getNumArcs() == 3);
		
		cut_circle.addGap(5, 50);
		QVERIFY(cut_circle.getNumArcs() == 1);
		QVERIFY(!cut_circle.isAngleInAnyGap(4));
		QVERIFY(cut_circle.isAngleInAnyGap(5));
		QVERIFY(cut_circle.isAngleInAnyGap(90));
		QVERIFY(!cut_circle.isAngleInAnyGap(91));
		
		cut_circle.addGap(80, 100);
		QVERIFY(cut_circle.getNumArcs() == 1);
		QVERIFY(cut_circle.isAngleInAnyGap(100));
		QVERIFY(!cut_circle.isAngleInAnyGap(101));
		
		// delete gap using last gap position
		cut_circle.deleteGap(100);
		QVERIFY(cut_circle.getNumArcs() == 0);
		
		// testing gap at fullcircle, i.e. vector (1,0)
		cut_circle.addGap(convAngle(350), convAngle(10));
		QVERIFY(cut_circle.getNumArcs() == 1);
		QVERIFY(!cut_circle.isAngleInAnyGap(convAngle(349)));
		QVERIFY(cut_circle.isAngleInAnyGap(convAngle(350)));
		QVERIFY(cut_circle.isAngleInAnyGap(convAngle(10)));
		QVERIFY(!cut_circle.isAngleInAnyGap(convAngle(11)));
		
		// delete gap using first gap position
		cut_circle.deleteGap(convAngle(350));
		QVERIFY(cut_circle.getNumArcs() == 0);
		
		// testing that arcs are sorted if order changes because of cutting
		cut_circle.addGap(convAngle(340), convAngle(350));
		QVERIFY(cut_circle.getNumArcs() == 1);
		
		cut_circle.addGap(convAngle(50), convAngle(90));
		QVERIFY(cut_circle.getNumArcs() == 2);
		QVERIFY(!cut_circle.isAngleInAnyGap(convAngle(49)));
		QVERIFY(cut_circle.isAngleInAnyGap(convAngle(50)));
		QVERIFY(cut_circle.isAngleInAnyGap(convAngle(90)));
		QVERIFY(!cut_circle.isAngleInAnyGap(convAngle(91)));
		
		// this cut will change the order of arcs:
		cut_circle.addGap(convAngle(345), convAngle(10));
		auto arcs = cut_circle.getArcs();
		QVERIFY(std::is_sorted(begin(arcs), end(arcs)));
		cut_circle.deleteGap(convAngle(350));
		
		// testing that arcs are sorted if order changes because of cutting, different execution path
		cut_circle.addGap(convAngle(340), convAngle(350));
		cut_circle.addGap(convAngle(300), convAngle(310));
		cut_circle.addGap(convAngle(250), convAngle(260));
		QVERIFY(cut_circle.getNumArcs() == 4);
		cut_circle.addGap(convAngle(200), convAngle(10));
		QVERIFY(cut_circle.getNumArcs() == 2);
		arcs = cut_circle.getArcs();
		QVERIFY(std::is_sorted(begin(arcs), end(arcs)));
		
		// .ocd roundtrip test
		std::vector<std::pair<int, int>> ocd_gaps = { {0x0004, 0x037F}, {0x070F, 0x0A8C} };
		cut_circle.importFromOCD(ocd_gaps);
		QVERIFY(cut_circle.getNumArcs() == int(ocd_gaps.size()));
		QByteArray byte_array;
		cut_circle.exportToOCD(byte_array);
		QDataStream stream(byte_array);
		std::pair<int, int> gap_angles;
		std::vector<std::pair<int, int>> ocd_gaps_import;
		
		for (int i = 0; i < int(ocd_gaps.size()); ++i)
		{
			stream.readRawData(reinterpret_cast<char*>(&gap_angles.first), sizeof(gap_angles.first));
			stream.readRawData(reinterpret_cast<char*>(&gap_angles.second), sizeof(gap_angles.second));
			ocd_gaps_import.emplace_back(gap_angles);
		}
		QVERIFY(ocd_gaps == ocd_gaps_import);
		
		// test duplication and check for equality
		auto duplicate_object = object.duplicate();
		QVERIFY(object.equals(duplicate_object, true));
	}
	
};  // class ObjectTest

/*
 * We don't need a real GUI window.
 */
namespace {
	auto const Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}

QTEST_MAIN(ObjectTest)
#include "object_t.moc"  // IWYU pragma: keep
