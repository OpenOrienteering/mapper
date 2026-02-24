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

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"

using namespace OpenOrienteering;

Q_DECLARE_METATYPE(MapCoord*)


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
}

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
	QCOMPARE(no_symbol.getMap(), static_cast<Map*>(nullptr));
	QCOMPARE(no_symbol.getSymbol(), static_cast<Symbol*>(nullptr));
	QVERIFY(no_symbol.getRawCoordinateVector().empty());
	QVERIFY(no_symbol.parts().empty());
	QCOMPARE(no_symbol.getPatternOrigin(), MapCoord());
	QCOMPARE(no_symbol.getPatternRotation(), qreal(0));
	
	{
		PathObject no_symbol_2{nullptr};
		QVERIFY(no_symbol_2.equals(&no_symbol, true));
	}
	
	PathObject empty_red{Map::getCoveringRedLine()};
	QCOMPARE(empty_red.getMap(), static_cast<Map*>(nullptr));
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
	QCOMPARE(inner_piece.getMap(), static_cast<Map*>(nullptr));  // Is this the desired behaviour?
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
		auto const& actual = line_from_first_part.parts().front();
		auto const& expected = expect_from_first_part.parts().front();
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
		auto const& actual = line_from_last_part.parts().front();
		auto const& expected = expect_from_last_part.parts().front();
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
	
	// Misc
	
	// "piece" at end of open part
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



void PathObjectTest::changePathBoundsBasicTest_data()
{
	QTest::addColumn<int>("dash_point_index");
	
	QTest::newRow("no dash point")        << -1;
	QTest::newRow("dash point at start")  <<  0;
	QTest::newRow("dash point at center") <<  1;
	QTest::newRow("dash point at end")    <<  2;
}

void PathObjectTest::changePathBoundsBasicTest()
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



void PathObjectTest::changePathBoundsTest_data()
{
	QTest::addColumn<int>("part_index");
	// new bounds by factor
	QTest::addColumn<float>("start_len");
	QTest::addColumn<float>("end_len");
	// expected number of coords
	QTest::addColumn<int>("expected_size_fwd");
	QTest::addColumn<int>("expected_size_rev");
	
	// The tested path has 6 coordinates and is 4+3+4+4+5 = 20 units long.
	// Thus 
	// - 0.2 is the 2nd coordinate,
	// - 0.6 and 0.7 are between 4rd and 5th coordinate,
	// - 0.9 is between 5th and 6th coordinate.
	QTest::newRow("0.0..1.0") << 0 << 0.0f << 1.0f << 6 << 6;
	QTest::newRow("0.0..0.9") << 0 << 0.0f << 0.9f << 6 << 2;
	QTest::newRow("0.0..0.7") << 0 << 0.0f << 0.7f << 5 << 3;
	QTest::newRow("0.0..0.2") << 0 << 0.0f << 0.2f << 2 << 5;
	QTest::newRow("0.0..0.0") << 0 << 0.0f << 0.0f << 6 << 6; // only for closed paths
	QTest::newRow("0.2..1.0") << 0 << 0.2f << 1.0f << 5 << 2;
	QTest::newRow("0.2..0.9") << 0 << 0.2f << 0.9f << 5 << 3;
	QTest::newRow("0.2..0.7") << 0 << 0.2f << 0.7f << 4 << 4;
	QTest::newRow("0.2..0.2") << 0 << 0.2f << 0.2f << 6 << 6; // only for closed paths
	QTest::newRow("0.6..1.0") << 0 << 0.6f << 1.0f << 3 << 5;
	QTest::newRow("0.6..0.9") << 0 << 0.6f << 0.9f << 3 << 6;
	QTest::newRow("0.6..0.7") << 0 << 0.6f << 0.7f << 2 << 7;
	QTest::newRow("0.6..0.6") << 0 << 0.6f << 0.6f << 7 << 7; // only for closed paths
	QTest::newRow("0.9..1.0") << 0 << 0.9f << 1.0f << 2 << 6;
	QTest::newRow("0.9..0.9") << 0 << 0.9f << 0.9f << 7 << 7;
	QTest::newRow("1.0..1.0") << 0 << 1.0f << 1.0f << 6 << 6; // only for closed paths
}

void PathObjectTest::changePathBoundsTest()
{
	QFETCH(int, part_index);
	QFETCH(float, start_len);
	QFETCH(float, end_len);
	QFETCH(int, expected_size_fwd);
	QFETCH(int, expected_size_rev);
	
	// For 0.0 or 1.0, products are not precise enough.
	auto exact_length = [](float factor, float length) -> float {
		if (factor == 0.0f)
			return 0.0f;
		if (factor == 1.0f)
			return length;
		return factor * length;
	};
	
	// This path is 4+3+4+4+5 = 20 units long.
	PathObject proto{Map::getCoveringRedLine()};
	proto.addCoordinate({0.0, 4.0});
	proto.addCoordinate({0.0, 8.0});
	proto.addCoordinate({3.0, 8.0});
	proto.addCoordinate({3.0, 4.0});
	proto.addCoordinate({3.0, 0.0});
	proto.addCoordinate({0.0, 4.0, MapCoord::HolePoint});
	proto.updatePathCoords();
	auto orig_len = proto.parts().front().length();
	QCOMPARE(orig_len, 20.0f);
	
	// open path
	if (start_len < end_len)
	{
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(std::size_t(part_index), exact_length(start_len, orig_len), exact_length(end_len, orig_len));
		path.updatePathCoords();
		
		QVERIFY(!path.parts().front().isClosed());
		QCOMPARE(int(path.getCoordinateCount()), expected_size_fwd);
		if (end_len == start_len)
			QCOMPARE(path.parts().front().length(), exact_length(1.0, orig_len));
		else
			QCOMPARE(path.parts().front().length(), exact_length(end_len - start_len, orig_len));
		
		// First and last new coord may differ, but the sequence in between must match.
		if (expected_size_fwd > 2)
		{
			auto first = begin(path.getRawCoordinateVector()) + 1;
			auto last = end(path.getRawCoordinateVector()) - 1;
			auto seq = std::find_end(
			               begin(proto.getRawCoordinateVector()), end(proto.getRawCoordinateVector()),
			               first, last );
			QVERIFY(seq != end(proto.getRawCoordinateVector()));
		}
	}
	
	// closed path
	proto.getCoordinateRef(5).setClosePoint(true);
	QVERIFY(proto.parts().front().isClosed());
	{
		// forward
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(std::size_t(part_index), exact_length(start_len, orig_len), exact_length(end_len, orig_len));
		path.updatePathCoords();
		
		QVERIFY(!path.parts().front().isClosed());
		QCOMPARE(int(path.getCoordinateCount()), expected_size_fwd);
		if (end_len == start_len)
		{
			QCOMPARE(path.parts().front().length(), exact_length(1.0, orig_len));
			
			auto last_0 = end(path.getRawCoordinateVector()) - 1;
			auto cut_0 = std::find(begin(path.getRawCoordinateVector()), last_0, proto.getRawCoordinateVector().front());
			QVERIFY(cut_0 != last_0);
			QVERIFY(std::equal(
			            cut_0, last_0,
			            begin(proto.getRawCoordinateVector())) );
			
			auto last_1 = end(proto.getRawCoordinateVector()) - 1;
			auto cut_1 = std::find(begin(proto.getRawCoordinateVector()), last_1, path.getRawCoordinateVector()[1]);
			QVERIFY(cut_1 != last_1);
			QVERIFY(std::equal(
			            cut_1, last_1,
			            begin(path.getRawCoordinateVector()) + 1) );
		}
		else
		{
			QCOMPARE(path.parts().front().length(), exact_length(end_len - start_len, orig_len));
			
			// First and last new coord may differ, but the sequence in between must match.
			if (expected_size_fwd > 2)
			{
				auto first = begin(path.getRawCoordinateVector()) + 1;
				auto last = end(path.getRawCoordinateVector()) - 1;
				auto seq = std::find_end(
				               begin(proto.getRawCoordinateVector()), end(proto.getRawCoordinateVector()),
				               first, last );
				QVERIFY(seq != end(proto.getRawCoordinateVector()));
			}
		}
	}
	{
		// reverse
		PathObject path{Map::getCoveringRedLine()};
		path.copyFrom(proto);
		path.changePathBounds(std::size_t(part_index), exact_length(end_len, orig_len), exact_length(start_len, orig_len));
		path.updatePathCoords();
		
		QVERIFY(!path.parts().front().isClosed());
		QCOMPARE(int(path.getCoordinateCount()), expected_size_rev);
		if (end_len == start_len || (start_len == 0.0f && end_len == 1.0f))
		{
			QCOMPARE(path.parts().front().length(), exact_length(1.0, orig_len));
			
			auto last_0 = end(path.getRawCoordinateVector()) - 1;
			auto cut_0 = std::find(begin(path.getRawCoordinateVector()), last_0, proto.getRawCoordinateVector().front());
			QVERIFY(cut_0 != last_0);
			QVERIFY(std::equal(
			            cut_0, last_0,
			            begin(proto.getRawCoordinateVector())) );
			
			auto last_1 = end(proto.getRawCoordinateVector()) - 1;
			auto cut_1 = std::find(begin(proto.getRawCoordinateVector()), last_1, path.getRawCoordinateVector()[1]);
			QVERIFY(cut_1 != last_1);
			QVERIFY(std::equal(
			            cut_1, last_1,
			            begin(path.getRawCoordinateVector()) + 1) );
		}
		else
		{
			QCOMPARE(path.parts().front().length(), exact_length(1.0f + start_len - end_len, orig_len));
			
			// First and last new coord may differ, but the sequence in between must match.
			if (start_len > 0.0f)
			{
				auto last_0 = end(path.getRawCoordinateVector()) - 1;
				auto cut_0 = std::find(begin(path.getRawCoordinateVector()), last_0, proto.getRawCoordinateVector().front());
				QVERIFY(cut_0 != last_0);
				QVERIFY(std::equal(
				            cut_0, last_0,
				            begin(proto.getRawCoordinateVector())) );
			}
			if (end_len <= 0.75f)
			{
				auto last_1 = end(proto.getRawCoordinateVector()) - 1;
				auto cut_1 = std::find(begin(proto.getRawCoordinateVector()), last_1, path.getRawCoordinateVector()[1]);
				QVERIFY(cut_1 != last_1);
				QVERIFY(std::equal(
				            cut_1, last_1,
				            begin(path.getRawCoordinateVector()) + 1) );
			}
		}
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



void PathObjectTest::removeFromLineTest_data()
{
	QTest::addColumn<int>("part_index");
	QTest::addColumn<float>("cut_begin");
	QTest::addColumn<float>("cut_end");
	QTest::addColumn<int>("expected_size");
	
	QTest::newRow("0.0..1.0") << 0 << 0.0f << 1.0f << 0;
	QTest::newRow("0.0..0.4") << 0 << 0.0f << 0.4f << 1;
	QTest::newRow("0.0..0.0") << 0 << 0.0f << 0.0f << 1;
	QTest::newRow("0.4..1.0") << 0 << 0.4f << 1.0f << 1;
	QTest::newRow("0.4..0.6") << 0 << 0.4f << 0.6f << 2;
	QTest::newRow("0.4..0.4") << 0 << 0.4f << 0.4f << 2;
	QTest::newRow("1.0..1.0") << 0 << 1.0f << 1.0f << 1;
}

void PathObjectTest::removeFromLineTest()
{
	QFETCH(int, part_index);
	QFETCH(float, cut_begin);
	QFETCH(float, cut_end);
	QFETCH(int, expected_size);
	
	auto compare_head = [](auto& actual, auto& expected) {
		QVERIFY(actual.getCoordinateCount() <= expected.getCoordinateCount());
		QVERIFY(std::equal(
		            begin(actual.getRawCoordinateVector()),
		            end(actual.getRawCoordinateVector()) - 1,   // cut point may differ
		            begin(expected.getRawCoordinateVector()),
		            equalXY) );
	};
	
	auto compare_tail = [](auto& actual, auto& expected) {
		QVERIFY(actual.getCoordinateCount() <= expected.getCoordinateCount());
		QVERIFY(std::equal(
		            rbegin(actual.getRawCoordinateVector()),
		            rend(actual.getRawCoordinateVector()) - 1,  // cut point may differ
		            rbegin(expected.getRawCoordinateVector()),
		            equalXY) );
	};
	
	auto compare_mixed = [](auto& actual, auto& expected, bool forward) {
		QVERIFY(actual.getCoordinateCount() <= expected.getCoordinateCount() + 2);
		auto expected_join = expected.getCoordinate(0);
		auto join = std::find_if(
		                begin(actual.getRawCoordinateVector()),
		                end(actual.getRawCoordinateVector()),
		                [expected_join](auto& current) { return equalXY(current, expected_join); } );
		
		if (!forward)
		{
			QEXPECT_FAIL("0.4..0.6", "Removing from closed path across the closing point", Abort);
		}
		QVERIFY(join != end(actual.getRawCoordinateVector()));
		
		QVERIFY(std::equal(
		            join,
		            end(actual.getRawCoordinateVector()) - 1,   // cut point may differ
		            begin(expected.getRawCoordinateVector()),
		            equalXY) );
		
		auto rjoin = std::find_if(
		                rbegin(actual.getRawCoordinateVector()),
		                rend(actual.getRawCoordinateVector()),
		                [expected_join](auto& current) { return equalXY(current, expected_join); } );
		QVERIFY(rjoin != rend(actual.getRawCoordinateVector()));
		QVERIFY(std::equal(
		            rjoin,
		            rend(actual.getRawCoordinateVector()) -1,   // cut point may differ
		            rbegin(expected.getRawCoordinateVector()),
		            equalXY) );
	};
	
	// For 0.0 or 1.0, products are not precise enough.
	auto exact_length = [](float factor, float length) -> float {
		if (factor == 0.0f)
			return 0.0f;
		if (factor == 1.0f)
			return length;
		return factor * length;
	};
	
	{
		// open path
		PathObject proto{Map::getCoveringRedLine()};
		proto.addCoordinate({0.0, 2.0});
		proto.addCoordinate({2.0, 0.0});
		proto.addCoordinate({4.0, -2.0, MapCoord::HolePoint});
		proto.updatePathCoords();
		auto orig_len = proto.parts().front().length();
		
		auto remaining = proto.removeFromLine(std::size_t(part_index), exact_length(cut_begin, orig_len), exact_length(cut_end, orig_len));
		QCOMPARE(int(remaining.size()), expected_size);
		for (auto* object : remaining)
			object->updatePathCoords();
		if (cut_begin > 0.0f)
			compare_head(*remaining.front(), proto);
		if (cut_end < 1.0f)
			compare_tail(*remaining.back(), proto);
		qDeleteAll(remaining);
	}
	
	{
		// closed path
		PathObject proto{Map::getCoveringRedLine()};
		proto.addCoordinate({0.0, 2.0});
		proto.addCoordinate({4.0, 2.0});
		proto.addCoordinate({4.0, -2.0});
		proto.addCoordinate({0.0, 2.0, MapCoord::HolePoint | MapCoord::ClosePoint});
		proto.updatePathCoords();
		auto orig_len = proto.parts().front().length();
		
		auto remaining = proto.removeFromLine(std::size_t(part_index), exact_length(cut_begin, orig_len), exact_length(cut_end, orig_len));
		QCOMPARE(int(remaining.size()), std::min(1, expected_size));
		if (expected_size > 0)
			compare_mixed(*remaining.front(), proto, true);
		qDeleteAll(remaining);
		
		remaining = proto.removeFromLine(std::size_t(part_index), exact_length(cut_end, orig_len), exact_length(cut_begin, orig_len));
		QCOMPARE(int(remaining.size()), 1);
		compare_mixed(*remaining.front(), proto, false);
		qDeleteAll(remaining);
	}
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



void PathObjectTest::recalculatePartsTest_data()
{
	static MapCoord coords[] = {
		{ 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, MapCoord::HolePoint },
		{ 2.0, 2.0 }, { 2.0, 3.0 }, { 3.0, 3.0, MapCoord::HolePoint },
		{ 4.0, 4.0, MapCoord::HolePoint },
		{ 6.0, 6.0 }, { 6.0, 5.0 },
	};
	QTest::addColumn<MapCoord*>("first");
	QTest::addColumn<MapCoord*>("last");  // STL sense
	QTest::addColumn<int>("expected_size");
	
	QTest::newRow("0.") << coords+0 << coords+0 << 0;
	QTest::newRow("1.") << coords+0 << coords+1 << 1;  // maybe not desired
	QTest::newRow("2.") << coords+0 << coords+2 << 1;  // maybe not desired for areas
	QTest::newRow("3.") << coords+0 << coords+3 << 1;
	QTest::newRow("4.") << coords+0 << coords+4 << 1;
	QTest::newRow("5.") << coords+0 << coords+5 << 1;
	QTest::newRow("5;1.") << coords+0 << coords+6 << 2;  // maybe not desired
	QTest::newRow("5;2.") << coords+0 << coords+7 << 2;  // maybe not desired for areas
	QTest::newRow("5;3.") << coords+0 << coords+8 << 2;
	QTest::newRow("5;3;1.") << coords+0 << coords+9 << 3;  // maybe not desired
	QTest::newRow("5;3;1;1.") << coords+0 << coords+10 << 4;  // maybe not desired
}

void PathObjectTest::recalculatePartsTest()
{
	QFETCH(MapCoord*, first);
	QFETCH(MapCoord*, last);
	QFETCH(int, expected_size);
	
	// The constructor calls PathObject::recalculateParts().
	PathObject path { nullptr, MapCoordVector(first, last), nullptr };
	{
		auto& initial_parts = path.parts();
		QCOMPARE(initial_parts.size(), expected_size);
		
		for (auto& initial_part : initial_parts)
		{
			QVERIFY(initial_part.first_index <= initial_part.last_index);
		}
	}
	
	path.updatePathCoords();
	{
		auto& updated_parts = path.parts();
		QCOMPARE(updated_parts.size(), expected_size);
		
		for (auto& updated_part : updated_parts)
		{
			QVERIFY(updated_part.first_index <= updated_part.last_index);
		}
	}
}



/*
 * We don't need a real GUI window.
 */
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(PathObjectTest)
