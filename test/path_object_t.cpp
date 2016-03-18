/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015, 2016 Kai Pastor
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

#include "../src/map.h"
#include "../src/symbol_line.h"

class DummyPathObject : public PathObject
{
public:
	DummyPathObject() : PathObject()
	{
		// Set a dummy symbol to make the object calculate path coordinates
		setSymbol(Map::getCoveringRedLine(), true);
	}
};

PathObjectTest::PathObjectTest(QObject* parent): QObject(parent)
{
	// nothing
}

void PathObjectTest::initTestCase()
{
	Q_INIT_RESOURCE(resources);
	doStaticInitializations();
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

void PathObjectTest::calcIntersectionsTest()
{
	QFETCH(void*, v_path1);
	QFETCH(void*, v_path2);
	QFETCH(void*, v_predicted_intersections);
	PathObject* path1 = static_cast<PathObject*>(v_path1);
	PathObject* path2 = static_cast<PathObject*>(v_path2);
	PathObject::Intersections* predicted_intersections = static_cast<PathObject::Intersections*>(v_predicted_intersections);
	
	PathObject::Intersections actual_intersections;
	path1->calcAllIntersectionsWith(path2, actual_intersections);
	actual_intersections.normalize();
	
	if (actual_intersections.size() != predicted_intersections->size())
	{
		for (size_t i = 0; i < predicted_intersections->size(); ++i)
		{
			qDebug() << "# Predicted " << i;
			qDebug() << "  X: " << predicted_intersections->at(i).coord.x();
			qDebug() << "  Y: " << predicted_intersections->at(i).coord.y();
			qDebug() << "  len: " << predicted_intersections->at(i).length;
			qDebug() << "  other_len: " << predicted_intersections->at(i).other_length;
		}
		for (size_t i = 0; i < actual_intersections.size(); ++i)
		{
			qDebug() << "# Actual " << i;
			qDebug() << "  X: " << actual_intersections.at(i).coord.x();
			qDebug() << "  Y: " << actual_intersections.at(i).coord.y();
			qDebug() << "  len: " << actual_intersections.at(i).length;
			qDebug() << "  other_len: " << actual_intersections.at(i).other_length;
		}
	}
	QCOMPARE(actual_intersections.size(), predicted_intersections->size());
	for (size_t i = 0; i < qMin(actual_intersections.size(), predicted_intersections->size()); ++i)
	{
		QCOMPARE(actual_intersections[i].coord.x(), predicted_intersections->at(i).coord.x());
		QCOMPARE(actual_intersections[i].coord.y(), predicted_intersections->at(i).coord.y());
		QCOMPARE(actual_intersections[i].part_index, predicted_intersections->at(i).part_index);
		QCOMPARE(actual_intersections[i].length, predicted_intersections->at(i).length);
		QCOMPARE(actual_intersections[i].other_part_index, predicted_intersections->at(i).other_part_index);
		QCOMPARE(actual_intersections[i].other_length, predicted_intersections->at(i).other_length);
	}
}

void PathObjectTest::calcIntersectionsTest_data()
{
	QTest::addColumn<void*>("v_path1");
	QTest::addColumn<void*>("v_path2");
	QTest::addColumn<void*>("v_predicted_intersections");
	
	PathObject::Intersection intersection;
	intersection.part_index = 0;
	intersection.other_part_index = 0;
	
	
	// Line-Line perpendicular intersection
	DummyPathObject* perpendicular1 = new DummyPathObject();
	perpendicular1->addCoordinate(MapCoord(10, 20));
	perpendicular1->addCoordinate(MapCoord(30, 20));
	
	DummyPathObject* perpendicular2 = new DummyPathObject();
	perpendicular2->addCoordinate(MapCoord(20, 10));
	perpendicular2->addCoordinate(MapCoord(20, 30));
	
	PathObject::Intersections* intersections_perpendicular = new PathObject::Intersections();
	intersection.coord = MapCoordF(20, 20);
	intersection.length = 10;
	intersection.other_length = 10;
	intersections_perpendicular->push_back(intersection);
	
	QTest::newRow("Line-Line perpendicular") << (void*)perpendicular1 << (void*)perpendicular2 << (void*)intersections_perpendicular;
	
	
	// Line-Line perpendicular intersection, test reversed
	QTest::newRow("Line-Line perpendicular, test reversed") << (void*)perpendicular2 << (void*)perpendicular1 << (void*)intersections_perpendicular;
	
	
	// Line-Line perpendicular intersection, line1 reversed
	PathObject* perpendicular3 = perpendicular1->duplicate()->asPath();
	perpendicular3->reverse();
	QTest::newRow("Line-Line perpendicular, line1 reversed") << (void*)perpendicular2 << (void*)perpendicular3 << (void*)intersections_perpendicular;
	
	
	// Line-Line parallel intersection
	DummyPathObject* parallel1 = new DummyPathObject();
	parallel1->addCoordinate(MapCoord(10, 0));
	parallel1->addCoordinate(MapCoord(30, 0));
	parallel1->addCoordinate(MapCoord(50, 0));
	
	DummyPathObject* parallel2 = new DummyPathObject();
	parallel2->addCoordinate(MapCoord(20, 0));
	parallel2->addCoordinate(MapCoord(40, 0));
	parallel2->addCoordinate(MapCoord(60, 0));
	
	PathObject::Intersections* intersections_parallel = new PathObject::Intersections();
	intersection.coord = MapCoordF(20, 0);
	intersection.length = 10;
	intersection.other_length = 0;
	intersections_parallel->push_back(intersection);
	intersection.coord = MapCoordF(50, 0);
	intersection.length = 40;
	intersection.other_length = 30;
	intersections_parallel->push_back(intersection);
	
	QTest::newRow("Line-Line parallel") << (void*)parallel1 << (void*)parallel2 << (void*)intersections_parallel;
	
	
	// Intersection at parallel start / end
	DummyPathObject* parallel3 = new DummyPathObject();
	parallel3->addCoordinate(MapCoord(20, 10));
	parallel3->addCoordinate(MapCoord(20, 0));
	parallel3->addCoordinate(MapCoord(30, 0));
	parallel3->addCoordinate(MapCoord(30, -10));
	
	PathObject::Intersections* intersections_parallel_se = new PathObject::Intersections();
	intersection.coord = MapCoordF(20, 0);
	intersection.length = 10;
	intersection.other_length = 10;
	intersections_parallel_se->push_back(intersection);
	intersection.coord = MapCoordF(30, 0);
	intersection.length = 20;
	intersection.other_length = 20;
	intersections_parallel_se->push_back(intersection);
	
	QTest::newRow("Line-Line parallel with intersection at start / end") << (void*)parallel1 << (void*)parallel3 << (void*)intersections_parallel_se;
	
	
	// Intersection at point
	DummyPathObject* point1 = new DummyPathObject();
	point1->addCoordinate(MapCoord(10, 30));
	point1->addCoordinate(MapCoord(30, 30));
	point1->addCoordinate(MapCoord(50, 30));
	
	DummyPathObject* point2 = new DummyPathObject();
	point2->addCoordinate(MapCoord(30, 10));
	point2->addCoordinate(MapCoord(30, 30));
	point2->addCoordinate(MapCoord(30, 50));
	
	PathObject::Intersections* intersections_point = new PathObject::Intersections();
	intersection.coord = MapCoordF(30, 30);
	intersection.length = 20;
	intersection.other_length = 20;
	intersections_point->push_back(intersection);
	
	QTest::newRow("Intersection at point") << (void*)point1 << (void*)point2 << (void*)intersections_point;
	
	
	// Duplicate of object intersection
	DummyPathObject* duplicate1 = new DummyPathObject();
	duplicate1->addCoordinate(MapCoord(10, 30));
	duplicate1->addCoordinate(MapCoord(30, 30));
	duplicate1->addCoordinate(MapCoord(50, 50));
	duplicate1->addCoordinate(MapCoord(50, 70));
	duplicate1->update(); // for duplicate1->parts().front().getLength();
	
	PathObject::Intersections* intersections_duplicate = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 30);
	intersection.length = 0;
	intersection.other_length = 0;
	intersections_duplicate->push_back(intersection);
	intersection.coord = MapCoordF(50, 70);
	intersection.length = duplicate1->parts().front().length();
	intersection.other_length = duplicate1->parts().front().length();
	intersections_duplicate->push_back(intersection);
	
	QTest::newRow("Start/end intersections for duplicate") << (void*)duplicate1 << (void*)duplicate1 << (void*)intersections_duplicate;
	
	
	// Reversed duplicate
	PathObject* duplicate2 = duplicate1->duplicate()->asPath();
	duplicate2->reverse();
	
	PathObject::Intersections* intersections_duplicate_reversed = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 30);
	intersection.length = 0;
	intersection.other_length = duplicate1->parts().front().length();
	intersections_duplicate_reversed->push_back(intersection);
	intersection.coord = MapCoordF(50, 70);
	intersection.length = duplicate1->parts().front().length();
	intersection.other_length = 0;
	intersections_duplicate_reversed->push_back(intersection);
	
	QTest::newRow("Start/end intersections for reversed duplicate") << (void*)duplicate1 << (void*)duplicate2 << (void*)intersections_duplicate_reversed;
	
	
	// Duplicate of closed object intersection
	PathObject* duplicate1_closed = duplicate1->duplicate()->asPath();
	duplicate1_closed->parts().front().setClosed(true);
	
	PathObject::Intersections* no_intersections = new PathObject::Intersections();
	
	QTest::newRow("No intersections for closed duplicate") << (void*)duplicate1_closed << (void*)duplicate1_closed << (void*)no_intersections;
	
	
	// Parallel at start intersection
	DummyPathObject* ps1 = new DummyPathObject();
	ps1->addCoordinate(MapCoord(10, 10));
	ps1->addCoordinate(MapCoord(30, 10));
	ps1->addCoordinate(MapCoord(30, 30));
	ps1->addCoordinate(MapCoord(10, 30));
	ps1->parts().front().setClosed(true);
	
	DummyPathObject* ps2 = new DummyPathObject();
	ps2->addCoordinate(MapCoord(10, 10));
	ps2->addCoordinate(MapCoord(30, 10));
	
	PathObject::Intersections* intersections_ps = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 10);
	intersection.length = 0;
	intersection.other_length = 0;
	intersections_ps->push_back(intersection);
	intersection.coord = MapCoordF(30, 10);
	intersection.length = 20;
	intersection.other_length = 20;
	intersections_ps->push_back(intersection);
	intersection.coord = MapCoordF(10, 10);
	intersection.length = 80;
	intersection.other_length = 0;
	intersections_ps->push_back(intersection);
	
	QTest::newRow("Parallel at start intersection") << (void*)ps1 << (void*)ps2 << (void*)intersections_ps;
	
	
	// Parallel at end intersection
	DummyPathObject* pe1 = new DummyPathObject();
	pe1->addCoordinate(MapCoord(10, 10));
	pe1->addCoordinate(MapCoord(30, 10));
	pe1->addCoordinate(MapCoord(30, 30));
	pe1->addCoordinate(MapCoord(10, 30));
	pe1->parts().front().setClosed(true);
	
	DummyPathObject* pe2 = new DummyPathObject();
	pe2->addCoordinate(MapCoord(10, 30));
	pe2->addCoordinate(MapCoord(10, 10));
	
	PathObject::Intersections* intersections_pe = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 10);
	intersection.length = 0;
	intersection.other_length = 20;
	intersections_pe->push_back(intersection);
	intersection.coord = MapCoordF(10, 30);
	intersection.length = 60;
	intersection.other_length = 0;
	intersections_pe->push_back(intersection);
	intersection.coord = MapCoordF(10, 10);
	intersection.length = 80;
	intersection.other_length = 20;
	intersections_pe->push_back(intersection);
	
	QTest::newRow("Parallel at end intersection") << (void*)pe1 << (void*)pe2 << (void*)intersections_pe;
	
	
	// a inside b
	DummyPathObject* aib1 = new DummyPathObject();
	aib1->addCoordinate(MapCoord(10, 0));
	aib1->addCoordinate(MapCoord(30, 0));
	
	DummyPathObject* aib2 = new DummyPathObject();
	aib2->addCoordinate(MapCoord(0, 0));
	aib2->addCoordinate(MapCoord(40, 0));
	
	PathObject::Intersections* intersections_aib = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 0);
	intersection.length = 0;
	intersection.other_length = 10;
	intersections_aib->push_back(intersection);
	intersection.coord = MapCoordF(30, 0);
	intersection.length = 20;
	intersection.other_length = 30;
	intersections_aib->push_back(intersection);
	
	QTest::newRow("a inside b") << (void*)aib1 << (void*)aib2 << (void*)intersections_aib;
	
	
	// b inside a
	PathObject::Intersections* intersections_bia = new PathObject::Intersections();
	intersection.coord = MapCoordF(10, 0);
	intersection.length = 10;
	intersection.other_length = 0;
	intersections_bia->push_back(intersection);
	intersection.coord = MapCoordF(30, 0);
	intersection.length = 30;
	intersection.other_length = 20;
	intersections_bia->push_back(intersection);
	
	QTest::newRow("b inside a") << (void*)aib2 << (void*)aib1 << (void*)intersections_bia;
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
	QCOMPARE(path_coords.size(), (std::size_t)7u);
	
	for (std::size_t i = 0, end = path_coords.size(); i< end; ++i)
	{
		switch (i)
		{
		case 0:
			QCOMPARE(path_coords[i].index, 0u);
			QCOMPARE(path_coords[i].param, 0.0f);
			QCOMPARE(path_coords[i].clen, 0.0f);
			break;
		default:
			if (path_coords[i].param == 0.0)
				QVERIFY(path_coords[i-1].index < path_coords[i].index);
			else
				QVERIFY(path_coords[i-1].index == path_coords[i].index);
			QCOMPARE(path_coords[i].clen, 0.0f);
			break;
		case 9:
			QCOMPARE(path_coords[i].index, 9u);
			QCOMPARE(path_coords[i].param, 0.0f);
			QCOMPARE(path_coords[i].clen, 0.0f);
			break;
		}
		
		auto split = SplitPathCoord::at(path_coords, i);
		QCOMPARE((std::size_t)split.path_coord_index, i);
		auto tangent = split.tangentVector();
		Q_UNUSED(tangent)
	}
}
	

QTEST_GUILESS_MAIN(PathObjectTest)
