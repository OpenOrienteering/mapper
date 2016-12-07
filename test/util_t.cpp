/*
 *    Copyright 2016 Kai Pastor
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


#include "util_t.h"

#include "../src/util.h"


UtilTest::UtilTest(QObject* parent)
: QObject(parent)
{
	// nothing else
}


void UtilTest::initTestCase()
{
	QRectF rect;
	
	rect = {};
	QVERIFY(!rect.isValid());
	
	rect = { 0, 0, 1, 1};
	QVERIFY(rect.isValid());
}


void UtilTest::rectIncludeTest()
{
	QRectF rect;
	
	// rectInclude on invalid rect
	rect = {};
	rectInclude(rect, QPointF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectInclude(rect, QRectF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectInclude(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
	
	// rectInclude on valid rect
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QRectF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
}

void UtilTest::rectIncludeSafeTest()
{
	QRectF rect;
	
	// rectIncludeSafe on invalid rect
	rect = {};
	rectIncludeSafe(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = {};
	rectIncludeSafe(rect, QRectF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectIncludeSafe(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
	
	// rectIncludeSafe on valid rect
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QRectF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
}


QTEST_APPLESS_MAIN(UtilTest)
