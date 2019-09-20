/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015, 2016, 2019 Kai Pastor
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

#include "path_object_t.h"

#include <QtTest>

#include "global.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"

using namespace OpenOrienteering;


namespace QTest
{

template<>
char* toString(const PathObject::Intersections& intersections)
{
	QByteArray ba;
	ba.reserve(1000);
	ba += "Elements: " + QByteArray::number(quint32(intersections.size())) + '\n';
	for (const auto& intersection : intersections)
	{
		ba += "  x:" + QByteArray::number(intersection.coord.x());
		ba +=  " y:" + QByteArray::number(intersection.coord.y());
		ba +=  " i:" + QByteArray::number(quint32(intersection.part_index));
		ba +=  " l:" + QByteArray::number(qreal(intersection.length));
		ba +=  " other_i:" + QByteArray::number(quint32(intersection.other_part_index));
		ba +=  " other_l:" + QByteArray::number(qreal(intersection.other_length));
		ba +=  '\n';
	}
	return qstrdup(ba.data());
}


}  // namespace QTest



namespace  {

bool equalXY(MapCoord const& lhs, MapCoord const& rhs) {
	return lhs.nativeX() == rhs.nativeX()
	       && lhs.nativeY() == rhs.nativeY();
};

PathObject::Intersections calculateIntersections(const PathObject& path1, const PathObject& path2)
{
	PathObject::Intersections actual_intersections;
	path1.calcAllIntersectionsWith(&path2, actual_intersections);
	actual_intersections.normalize();
	return actual_intersections;
}


}  // namespace



PathObjectTest::PathObjectTest(QObject* parent): QObject(parent)
{
	// nothing
}


void PathObjectTest::initTestCase()
{
	// Static map initializations
	Map map;
}



void PathObjectTest::mapCoordTest()
{
	auto vector = MapCoordF { 2.0, 0.0 };
	QCOMPARE(vector.x(), 2.0);
	QCOMPARE(vector.y(), 0.0);
	QCOMPARE(vector.length(), 2.0);
	QCOMPARE(vector.angle(), 0.0);
	
	vector = vector.perpRight();
	QCOMPARE(vector.x(), 0.0);
	QCOMPARE(vector.y(), 2.0); // This looks like left, but our y axis is mirrored.
	QCOMPARE(vector.length(), 2.0);
	QCOMPARE(vector.angle(), M_PI/2); // Remember, our y axis is mirrored.
}



void PathObjectTest::virtualPathTest()
{
	// Test open path
	auto coords = MapCoordVector { { 0.0, 0.0 }, { 2.0, 0.0 }, { 2.0, 1.0 }, { 2.0, -1.0 } };
	auto path = VirtualPath { coords };
	
	auto tangent_scaling = path.calculateTangentScaling(0);
	auto& scaling = tangent_scaling.second;
	auto right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.length(), 1.0);
	QCOMPARE(right.x(), 0.0);
	QCOMPARE(right.y(), 1.0); // This looks like left, but our y axis is mirrored.
	QCOMPARE(right.angle(), M_PI/2);
	QCOMPARE(scaling, 1.0);
	
	tangent_scaling = path.calculateTangentScaling(1);
	right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.x(), -sqrt(2.0)/2);
	QCOMPARE(right.y(), sqrt(2.0)/2);
	QCOMPARE(right.angle(), 3*M_PI/4);
	QCOMPARE(scaling, sqrt(2.0));
	
	tangent_scaling = path.calculateTangentScaling(2);
	right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.length(), 1.0);
	QCOMPARE(right.x(), 0.0);
	QCOMPARE(right.y(), 1.0);
	QCOMPARE(right.angle(), M_PI/2);
	QVERIFY(qIsInf(scaling));
	
	tangent_scaling = path.calculateTangentScaling(3);
	right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.length(), 1.0);
	QCOMPARE(right.x(), 1.0);
	QCOMPARE(right.y(), 0.0);
	QCOMPARE(right.angle(), 0.0);
	QCOMPARE(scaling, 1.0);
	
	// Test close point
	coords.emplace_back( 0.0, -1.0 );
	coords.emplace_back( 0.0, 0.0 );
	coords.back().setClosePoint(true);
	path.last_index += 2;
	
	tangent_scaling = path.calculateTangentScaling(0);
	right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.length(), 1.0);
	QCOMPARE(right.x(), -sqrt(2.0)/2);
	QCOMPARE(right.y(), sqrt(2.0)/2);
	QCOMPARE(right.angle(), 3*M_PI/4);
	QCOMPARE(scaling, sqrt(2.0));
	
	tangent_scaling = path.calculateTangentScaling(coords.size()-1);
	right = tangent_scaling.first.perpRight();
	right.normalize();
	QCOMPARE(right.length(), 1.0);
	QCOMPARE(right.x(), -sqrt(2.0)/2);
	QCOMPARE(right.y(), sqrt(2.0)/2);
	QCOMPARE(right.angle(), 3*M_PI/4);
	QCOMPARE(scaling, sqrt(2.0));
}



void PathObjectTest::constructorTest()
{
	// PathObject(const Symbol* symbol = nullptr)
	
	PathObject no_symbol{};
	QCOMPARE(no_symbol.getMap(), nullptr);
	QCOMPARE(no_symbol.getSymbol(), nullptr);
	QVERIFY(no_symbol.getRawCoordinateVector().empty());
	QVERIFY(no_symbol.parts().empty());
	QCOMPARE(no_symbol.getPatternOrigin(), MapCoord());
	QCOMPARE(no_symbol.getPatternRotation(), qreal(0));
	
	{
		PathObject no_symbol_2{nullptr};
		QVERIFY(no_symbol_2.equals(&no_symbol, true));
	}
	
	PathObject empty_red{Map::getCoveringRedLine()};
	QCOMPARE(empty_red.getMap(), nullptr);
	QCOMPARE(empty_red.getSymbol(), Map::getCoveringRedLine());
	QVERIFY(empty_red.getRawCoordinateVector().empty());
	QVERIFY(empty_red.parts().empty());
	QCOMPARE(empty_red.getPatternOrigin(), MapCoord());
	QCOMPARE(empty_red.getPatternRotation(), qreal(0));
	QVERIFY(empty_red.equals(&no_symbol, false));
	QVERIFY(!empty_red.equals(&no_symbol, true));
	
	// PathObject(const Symbol* symbol, const MapCoordVector& coords, Map* map = nullptr)
	
	Map map;
	auto coords = MapCoordVector{
	    MapCoord{0.0, 0.0}, MapCoord{0.0, 5.0}, MapCoord{5.0, 0.0}, MapCoord{0.0, 0.0, MapCoord::ClosePoint|MapCoord::HolePoint},
	    MapCoord{1.0, 1.0}, MapCoord{0.0, 3.0}, MapCoord{3.0, 0.0}, MapCoord{1.0, 1.0, MapCoord::ClosePoint|MapCoord::HolePoint}
	};
	PathObject red_area{Map::getCoveringRedLine(), coords, &map};
	QCOMPARE(red_area.getMap(), &map);
	QCOMPARE(red_area.getSymbol(), Map::getCoveringRedLine());
	QCOMPARE(red_area.getRawCoordinateVector(), coords);
	QCOMPARE(red_area.parts().size(), std::size_t(2));
	QCOMPARE(red_area.getPatternOrigin(), MapCoord());
	QCOMPARE(red_area.getPatternRotation(), qreal(0));
	
	{
		PathObject other_area{Map::getCoveringWhiteLine(), coords};
		QVERIFY(other_area.getMap() != red_area.getMap()); // not tested by `equals`
		QVERIFY(other_area.equals(&red_area, false));
		QVERIFY(!other_area.equals(&red_area, true));
		
		other_area.setSymbol(red_area.getSymbol(), true);
		QVERIFY(other_area.equals(&red_area, false));
		QVERIFY(other_area.equals(&red_area, true));
		
		other_area.setPatternOrigin({1.0, 1.0});
		QVERIFY(!other_area.equals(&red_area, false));
		QVERIFY(!other_area.equals(&red_area, true));
		
		other_area.setPatternOrigin(red_area.getPatternOrigin());
		other_area.setPatternRotation(1.0);
		QVERIFY(!other_area.equals(&red_area, false));
		QVERIFY(!other_area.equals(&red_area, true));
		
		other_area.setPatternRotation(red_area.getPatternRotation());
		QVERIFY(other_area.equals(&red_area, false));
		QVERIFY(other_area.equals(&red_area, true));
	}
	
	// PathObject(const Symbol* symbol, const PathObject& proto, MapCoordVector::size_type piece)
	
	auto piece = MapCoordVector::size_type(5);
	QVERIFY(red_area.getRawCoordinateVector().size() > piece);
	PathObject inner_piece{Map::getCoveringWhiteLine(), red_area, piece};
	QCOMPARE(inner_piece.getMap(), nullptr);  // Is this the desired behaviour?
	QCOMPARE(inner_piece.getSymbol(), Map::getCoveringWhiteLine());
	QVERIFY(std::equal(
	            begin(inner_piece.getRawCoordinateVector()),
	            end(inner_piece.getRawCoordinateVector()),
	            begin(coords)+int(piece),
	            equalXY) );
	QCOMPARE(int(inner_piece.parts().size()), 1);
	QCOMPARE(inner_piece.getPatternOrigin(), MapCoord());
	QCOMPARE(inner_piece.getPatternRotation(), qreal(0));
	
	piece = 3;
	QVERIFY(red_area.getCoordinateRef(piece).isClosePoint());
	PathObject piece_from_end{nullptr, red_area, piece};
	QCOMPARE(piece_from_end.getCoordinateCount(), MapCoordVector::size_type(2));
	QVERIFY(std::equal(
	            begin(piece_from_end.getRawCoordinateVector()),
	            end(piece_from_end.getRawCoordinateVector()),
	            begin(coords),
	            equalXY) );
	
	// PathObject(const PathPart& proto_part)
	
	PathObject line_from_first_part{red_area.parts().front()};
	QVERIFY(std::equal(
	            begin(line_from_first_part.getRawCoordinateVector()),
	            end(line_from_first_part.getRawCoordinateVector()),
	            begin(coords),
	            equalXY) );
	QCOMPARE(line_from_first_part.parts().size(), std::size_t(1));
	{
		PathObject expect_from_first_part(red_area.getSymbol(), red_area.getRawCoordinateVector());
		expect_from_first_part.deletePart(1);
		auto actual = line_from_first_part.parts().front();
		auto expected = expect_from_first_part.parts().front();
		QCOMPARE(actual.size(), expected.size());
		QCOMPARE(actual.first_index, expected.first_index);
		QCOMPARE(actual.last_index, expected.last_index);
		for (auto i = 0u; i < actual.coords.size(); ++i)
		{
			QCOMPARE(actual.coords[i], expected.coords[i]);
			QCOMPARE(actual.coords.flags[i], expected.coords.flags[i]);
		}
		QVERIFY(std::equal(
		            begin(actual.path_coords),
		            end(actual.path_coords),
		            begin(expected.path_coords),
		            [](auto& lhs, auto& rhs) {
		                return lhs.pos == rhs.pos && lhs.index == rhs.index && lhs.param == rhs.param && lhs.clen == rhs.clen;
		            }) );
	}
	
	PathObject line_from_last_part{red_area.parts().back()};
	QVERIFY(std::equal(
	            begin(line_from_last_part.getRawCoordinateVector()),
	            end(line_from_last_part.getRawCoordinateVector()),
	            begin(coords)+red_area.parts().back().first_index,
	            equalXY) );
	QCOMPARE(line_from_last_part.parts().size(), std::size_t(1));
	{
		PathObject expect_from_last_part(red_area.getSymbol(), red_area.getRawCoordinateVector());
		expect_from_last_part.deletePart(0);
		auto actual = line_from_last_part.parts().front();
		auto expected = expect_from_last_part.parts().front();
		QCOMPARE(actual.size(), expected.size());
		QEXPECT_FAIL("", "first_index is incorrect for all but the first part", Continue);
		QCOMPARE(actual.first_index, expected.first_index);
		QEXPECT_FAIL("", "last_index is incorrect for all but the first part", Continue);
		QCOMPARE(actual.last_index, expected.last_index);
		for (auto i = 0u; i < actual.coords.size(); ++i)
		{
			QCOMPARE(actual.coords[i], expected.coords[i]);
			QCOMPARE(actual.coords.flags[i], expected.coords.flags[i]);
		}
		QVERIFY(std::equal(
		            begin(actual.path_coords),
		            end(actual.path_coords),
		            begin(expected.path_coords),
		            [](auto& lhs, auto& rhs) {
		                return lhs.pos == rhs.pos && lhs.index == rhs.index && lhs.param == rhs.param && lhs.clen == rhs.clen;
		            }) );
	}
	
	// Misc
	
	// "piece" at end of open part
	line_from_last_part.recalculateParts();  // See QEXPECT_FAIL above
	piece = line_from_last_part.parts().back().last_index;
	line_from_last_part.getCoordinateRef(piece) = MapCoord{4.0, 0.0, MapCoord::HolePoint};
	PathObject piece_from_open_end{nullptr, line_from_last_part, piece};
	QCOMPARE(piece_from_open_end.getCoordinateCount(), MapCoordVector::size_type(2));
	QVERIFY(std::equal(
	            begin(piece_from_open_end.getRawCoordinateVector()),
	            end(piece_from_open_end.getRawCoordinateVector()),
	            begin(line_from_last_part.getRawCoordinateVector()) + int(piece) - 1,
	            equalXY) );
}



void PathObjectTest::copyFromTest_data()
{
	QTest::addColumn<int>("dash_point_index");
	
	QTest::newRow("no dash point")        << -1;
	QTest::newRow("dash point at start")  <<  0;
	QTest::newRow("dash point at center") <<  1;
	QTest::newRow("dash point at end")    <<  2;
}

void PathObjectTest::copyFromTest()
{
	// Simple path
	PathObject proto{Map::getCoveringRedLine()};
	proto.addCoordinate({0.0, 2.0});
	proto.addCoordinate({4.0, 2.0});
	proto.addCoordinate({4.0, -2.0});
	
	QFETCH(int, dash_point_index);
	for (std::size_t i = 0; i < proto.getCoordinateCount(); ++i)
		proto.getCoordinateRef(i).setDashPoint(int(i) == dash_point_index);
	
	{
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		QCOMPARE(path.getCoordinateCount(), proto.getCoordinateCount());
		
		for (std::size_t i = 0; i < path.getCoordinateCount(); ++i)
			QCOMPARE(path.getCoordinate(i), proto.getCoordinate(i));
	}
	
	// Closed path with a hole
	proto.addCoordinate({0.0, 2.0, MapCoord::ClosePoint});
	proto.addCoordinate({0.5, 1.5, MapCoord::HolePoint});
	proto.addCoordinate({3.5, 1.5});
	proto.addCoordinate({3.5, 0.0});
	proto.addCoordinate({0.5, 1.5, MapCoord::ClosePoint});
	
	{
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		QCOMPARE(path.getCoordinateCount(), proto.getCoordinateCount());
		
		for (std::size_t i = 0; i < path.getCoordinateCount(); ++i)
			QCOMPARE(path.getCoordinate(i), proto.getCoordinate(i));
	}
}



void PathObjectTest::changePathBoundsTest_data()
{
	QTest::addColumn<int>("dash_point_index");
	
	QTest::newRow("no dash point")        << -1;
	QTest::newRow("dash point at start")  <<  0;
	QTest::newRow("dash point at center") <<  1;
	QTest::newRow("dash point at end")    <<  2;
}

void PathObjectTest::changePathBoundsTest()
{
	PathObject proto{Map::getCoveringRedLine()};
	proto.addCoordinate({0.0, 2.0});
	proto.addCoordinate({2.0, 0.0});
	proto.addCoordinate({4.0, -2.0, MapCoord::HolePoint});
	
	QFETCH(int, dash_point_index);
	for (std::size_t i = 0; i < proto.getCoordinateCount(); ++i)
		proto.getCoordinateRef(i).setDashPoint(int(i) == dash_point_index);
	
	proto.updatePathCoords();
	
	{
		// full path
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(0, 0.0, path.parts().front().length());
		QCOMPARE(path.getCoordinate(0), proto.getCoordinate(0));
		QCOMPARE(path.getCoordinate(1), proto.getCoordinate(1));
		QCOMPARE(path.getCoordinate(2), proto.getCoordinate(2));
	}
	
	{
		// sub-path starting at begin
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(0, 0.0, 4.0);
		QCOMPARE(path.getCoordinate(0), proto.getCoordinate(0));
		QCOMPARE(path.getCoordinate(1), proto.getCoordinate(1));
	}
	
	{
		// sub-path ending at original end
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(0, 2.0, path.parts().front().length());
		QCOMPARE(path.getCoordinate(1), proto.getCoordinate(1));
		QCOMPARE(path.getCoordinate(2), proto.getCoordinate(2));
	}
	
	{
		// inner sub-path
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(0, 2.0, 4.0);
		QCOMPARE(path.getCoordinate(1), proto.getCoordinate(1));
	}
}



void PathObjectTest::splitLineTest_data()
{
	QTest::addColumn<int>("dash_point_index");
	
	QTest::newRow("no dash point")        << -1;
	QTest::newRow("dash point at start")  <<  0;
	QTest::newRow("dash point at center") <<  1;
	QTest::newRow("dash point at end")    <<  2;
}

void PathObjectTest::splitLineTest()
{
	PathObject proto{Map::getCoveringRedLine()};
	proto.addCoordinate({0.0, 2.0});
	proto.addCoordinate({2.0, 0.0});
	proto.addCoordinate({4.0, -2.0, MapCoord::HolePoint});
	
	QFETCH(int, dash_point_index);
	if (dash_point_index >= 0)
		proto.getCoordinateRef(std::size_t(dash_point_index)).setDashPoint(true);
	
	auto object_path_coord = ObjectPathCoord { &proto };
	auto const distance_sq = object_path_coord.findClosestPointTo({1.0, 1.0});
	QVERIFY(qIsNull(distance_sq));
	QCOMPARE(object_path_coord.pos, MapCoordF(1.0, 1.0));
	
	auto const split_paths = proto.splitLineAt(object_path_coord);
	QCOMPARE(split_paths.size(), std::size_t(2));
	
	auto const *split_0 = split_paths[0];
	QCOMPARE(split_0->getCoordinateCount(), std::size_t(2));
	QCOMPARE(split_0->getCoordinate(0), proto.getCoordinate(0));
	QCOMPARE(split_0->getCoordinate(1), MapCoord(1.0, 1.0, MapCoord::HolePoint));
	delete split_0;
	
	auto const *split_1 = split_paths[1];
	QCOMPARE(split_1->getCoordinateCount(), std::size_t(3));
	QCOMPARE(split_1->getCoordinate(0), MapCoord(1.0, 1.0));
	QCOMPARE(split_1->getCoordinate(1), proto.getCoordinate(1));
	QCOMPARE(split_1->getCoordinate(2), proto.getCoordinate(2));
	delete split_1;
}



void PathObjectTest::calcIntersectionsTest()
{
	{
		// Line-Line perpendicular intersection
		PathObject perpendicular1{Map::getCoveringRedLine()};
		perpendicular1.addCoordinate(MapCoord(10, 20));
		perpendicular1.addCoordinate(MapCoord(30, 20));
		
		PathObject perpendicular2{Map::getCoveringRedLine()};
		perpendicular2.addCoordinate(MapCoord(20, 10));
		perpendicular2.addCoordinate(MapCoord(20, 30));
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(20, 20);
		intersection.length = 10;
		intersection.other_length = 10;
		
		PathObject::Intersections intersections_perpendicular;
		intersections_perpendicular.reserve(1);
		intersections_perpendicular.push_back(intersection);
		
		QCOMPARE(calculateIntersections(perpendicular1, perpendicular2), intersections_perpendicular);
		QCOMPARE(calculateIntersections(perpendicular2, perpendicular1), intersections_perpendicular);
		
		perpendicular1.reverse();
		QCOMPARE(calculateIntersections(perpendicular1, perpendicular2), intersections_perpendicular);
	}
	
	{
		// Intersection at explicit common point
		PathObject common_point_path1{Map::getCoveringRedLine()};
		common_point_path1.addCoordinate(MapCoord(10, 30));
		common_point_path1.addCoordinate(MapCoord(30, 30));
		common_point_path1.addCoordinate(MapCoord(50, 30));
		
		PathObject common_point_path2{Map::getCoveringRedLine()};
		common_point_path2.addCoordinate(MapCoord(30, 10));
		common_point_path2.addCoordinate(MapCoord(30, 30));
		common_point_path2.addCoordinate(MapCoord(30, 50));
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(30, 30);
		intersection.length = 20;
		intersection.other_length = 20;
		
		PathObject::Intersections intersections_point;
		intersections_point.reserve(1);
		intersections_point.push_back(intersection);
		
		QCOMPARE(calculateIntersections(common_point_path1, common_point_path2), intersections_point);
	}
	
	{
		// Line-Line parallel intersection
		PathObject parallel1{Map::getCoveringRedLine()};
		parallel1.addCoordinate(MapCoord(10, 0));
		parallel1.addCoordinate(MapCoord(30, 0));
		parallel1.addCoordinate(MapCoord(50, 0));
		
		PathObject parallel2{Map::getCoveringRedLine()};
		parallel2.addCoordinate(MapCoord(20, 0));
		parallel2.addCoordinate(MapCoord(40, 0));
		parallel2.addCoordinate(MapCoord(60, 0));
		
		PathObject::Intersections intersections_parallel;
		intersections_parallel.reserve(2);
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(20, 0);
		intersection.length = 10;
		intersection.other_length = 0;
		intersections_parallel.push_back(intersection);
		
		intersection.coord = MapCoordF(50, 0);
		intersection.length = 40;
		intersection.other_length = 30;
		intersections_parallel.push_back(intersection);
		
		QCOMPARE(calculateIntersections(parallel1, parallel2), intersections_parallel);
		
		// Intersection at parallel start / end
		PathObject parallel3{Map::getCoveringRedLine()};
		parallel3.addCoordinate(MapCoord(20, 10));
		parallel3.addCoordinate(MapCoord(20, 0));
		parallel3.addCoordinate(MapCoord(30, 0));
		parallel3.addCoordinate(MapCoord(30, -10));
		
		PathObject::Intersections intersections_parallel_se;
		intersections_parallel.reserve(2);
		
		intersection.coord = MapCoordF(20, 0);
		intersection.length = 10;
		intersection.other_length = 10;
		intersections_parallel_se.push_back(intersection);
		
		intersection.coord = MapCoordF(30, 0);
		intersection.length = 20;
		intersection.other_length = 20;
		intersections_parallel_se.push_back(intersection);
		
		QCOMPARE(calculateIntersections(parallel1, parallel3), intersections_parallel_se);
	}
	
	
	{
		// Duplicate of object intersection
		PathObject duplicate1{Map::getCoveringRedLine()};
		duplicate1.addCoordinate(MapCoord(10, 30));
		duplicate1.addCoordinate(MapCoord(30, 30));
		duplicate1.addCoordinate(MapCoord(50, 50));
		duplicate1.addCoordinate(MapCoord(50, 70));
		duplicate1.update(); // for duplicate1.parts().front().getLength();
		
		{
			PathObject::Intersections intersections_duplicate;
			intersections_duplicate.reserve(2);
			
			PathObject::Intersection intersection{};
			intersection.coord = MapCoordF(10, 30);
			intersection.length = 0;
			intersection.other_length = 0;
			intersections_duplicate.push_back(intersection);
			
			intersection.coord = MapCoordF(50, 70);
			intersection.length = duplicate1.parts().front().length();
			intersection.other_length = duplicate1.parts().front().length();
			intersections_duplicate.push_back(intersection);
			
			QCOMPARE(calculateIntersections(duplicate1, duplicate1), intersections_duplicate);
		}
		
		{
			// Reversed duplicate
			PathObject duplicate2{Map::getCoveringRedLine()};
			duplicate2.copyFrom(duplicate1);
			duplicate2.reverse();
			
			PathObject::Intersections intersections_duplicate_reversed;
			intersections_duplicate_reversed.reserve(2);
			
			PathObject::Intersection intersection{};
			intersection.coord = MapCoordF(10, 30);
			intersection.length = 0;
			intersection.other_length = duplicate1.parts().front().length();
			intersections_duplicate_reversed.push_back(intersection);
			
			intersection.coord = MapCoordF(50, 70);
			intersection.length = duplicate1.parts().front().length();
			intersection.other_length = 0;
			intersections_duplicate_reversed.push_back(intersection);
			
			QCOMPARE(calculateIntersections(duplicate1, duplicate2), intersections_duplicate_reversed);
		}
		
		{
			// Duplicate of closed object intersection
			PathObject duplicate1_closed{Map::getCoveringRedLine()};
			duplicate1_closed.copyFrom(duplicate1);
			duplicate1_closed.parts().front().setClosed(true);
			
			PathObject::Intersections no_intersections;
			
			QCOMPARE(calculateIntersections(duplicate1_closed, duplicate1_closed), no_intersections);
		}
	}
	
	{
		// Parallel at start intersection
		PathObject ps1{Map::getCoveringRedLine()};
		ps1.addCoordinate(MapCoord(10, 10));
		ps1.addCoordinate(MapCoord(30, 10));
		ps1.addCoordinate(MapCoord(30, 30));
		ps1.addCoordinate(MapCoord(10, 30));
		ps1.parts().front().setClosed(true);
		
		PathObject ps2{Map::getCoveringRedLine()};
		ps2.addCoordinate(MapCoord(10, 10));
		ps2.addCoordinate(MapCoord(30, 10));
		
		PathObject::Intersections intersections_ps;
		intersections_ps.reserve(3);
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(10, 10);
		intersection.length = 0;
		intersection.other_length = 0;
		intersections_ps.push_back(intersection);
		
		intersection.coord = MapCoordF(30, 10);
		intersection.length = 20;
		intersection.other_length = 20;
		intersections_ps.push_back(intersection);
		
		intersection.coord = MapCoordF(10, 10);
		intersection.length = 80;
		intersection.other_length = 0;
		intersections_ps.push_back(intersection);
		
		QCOMPARE(calculateIntersections(ps1, ps2), intersections_ps);
	}
	
	{
		// Parallel at end intersection
		PathObject pe1{Map::getCoveringRedLine()};
		pe1.addCoordinate(MapCoord(10, 10));
		pe1.addCoordinate(MapCoord(30, 10));
		pe1.addCoordinate(MapCoord(30, 30));
		pe1.addCoordinate(MapCoord(10, 30));
		pe1.parts().front().setClosed(true);
		
		PathObject pe2{Map::getCoveringRedLine()};
		pe2.addCoordinate(MapCoord(10, 30));
		pe2.addCoordinate(MapCoord(10, 10));
		
		PathObject::Intersections intersections_pe;
		intersections_pe.reserve(3);
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(10, 10);
		intersection.length = 0;
		intersection.other_length = 20;
		intersections_pe.push_back(intersection);
		
		intersection.coord = MapCoordF(10, 30);
		intersection.length = 60;
		intersection.other_length = 0;
		intersections_pe.push_back(intersection);
		
		intersection.coord = MapCoordF(10, 10);
		intersection.length = 80;
		intersection.other_length = 20;
		intersections_pe.push_back(intersection);
		
		QCOMPARE(calculateIntersections(pe1, pe2), intersections_pe);
	}
	
	{
		// a inside b
		PathObject aib1{Map::getCoveringRedLine()};
		aib1.addCoordinate(MapCoord(10, 0));
		aib1.addCoordinate(MapCoord(30, 0));
		
		PathObject aib2{Map::getCoveringRedLine()};
		aib2.addCoordinate(MapCoord(0, 0));
		aib2.addCoordinate(MapCoord(40, 0));
		
		PathObject::Intersections intersections_aib;
		intersections_aib.reserve(2);
		
		PathObject::Intersection intersection{};
		intersection.coord = MapCoordF(10, 0);
		intersection.length = 0;
		intersection.other_length = 10;
		intersections_aib.push_back(intersection);
		
		intersection.coord = MapCoordF(30, 0);
		intersection.length = 20;
		intersection.other_length = 30;
		intersections_aib.push_back(intersection);
		
		QCOMPARE(calculateIntersections(aib1, aib2), intersections_aib);
		
		
		// b inside a
		PathObject::Intersections intersections_bia;
		intersections_bia.reserve(2);
		
		intersection.coord = MapCoordF(10, 0);
		intersection.length = 10;
		intersection.other_length = 0;
		intersections_bia.push_back(intersection);
		
		intersection.coord = MapCoordF(30, 0);
		intersection.length = 30;
		intersection.other_length = 20;
		intersections_bia.push_back(intersection);
		
		QCOMPARE(calculateIntersections(aib2, aib1), intersections_bia);
	}
}



void PathObjectTest::atypicalPathTest()
{
	// This is a zero-length closed path of three arcs.
	MapCoordVector coords(10, {2.0, 2.0});
	coords[0].setCurveStart(true);
	coords[3].setCurveStart(true);
	coords[6].setCurveStart(true);
	coords[9].setClosePoint(true);
	coords[9].setHolePoint(true);
	
	PathCoordVector path_coords { coords };
	path_coords.update(0);
	QCOMPARE(path_coords.size(), std::size_t(7));
	
	for (std::size_t i = 0, end = path_coords.size(); i< end; ++i)
	{
		switch (i)
		{
		case 0:
		case 2:
		case 4:
		case 6:
			QCOMPARE(std::size_t(path_coords[i].index), (i/2)*3);
			QCOMPARE(path_coords[i].param, 0.0f);
			QCOMPARE(path_coords[i].clen, 0.0f);
			break;
		case 1:
		case 3:
		case 5:
			QCOMPARE(path_coords[i].index, path_coords[i-1].index);
			QVERIFY(path_coords[i].param > 0.0f);
			QVERIFY(path_coords[i].param < 1.0f);
			QCOMPARE(path_coords[i].clen, 0.0f);
			break;
		default:
			Q_UNREACHABLE();
		}
		
		auto split = SplitPathCoord::at(path_coords, i);
		QCOMPARE(std::size_t(split.path_coord_index), i);
		auto tangent = split.tangentVector();
		QVERIFY(qIsNull(tangent.lengthSquared()));
	}
}



/*
 * We don't need a real GUI window.
 */
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(PathObjectTest)
