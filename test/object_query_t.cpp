/*
 *    Copyright 2016 Mitchell Krome
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

#include "object_query_t.h"

#include <QString>
#include <QtTest/QtTest>

#include "../src/object.h"
#include "../src/object_query.h"

ObjectQueryTest::ObjectQueryTest(QObject* parent)
: QObject(parent)
{
	// nothing
}

Object* ObjectQueryTest::makeTestObject()
{
	Object* object = new PathObject();
	QHash<QString, QString> tags{
		{QLatin1String("a"), QLatin1String("a")}
		,{QLatin1String("b"), QLatin1String("b")}
		,{QLatin1String("c"), QLatin1String("c")}
		,{QLatin1String("abc"), QLatin1String("abc")}
	};

	object->setTags(tags);
	return object;
}

void ObjectQueryTest::testIsQuery()
{
	Object* object = makeTestObject();

	ObjectQuery single_query_is_true{QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")};
	QVERIFY(single_query_is_true(object) == true);
	ObjectQuery single_query_is_false{QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")};
	QVERIFY(single_query_is_false(object) == false);
}

void ObjectQueryTest::testNotQuery()
{
	Object* object = makeTestObject();

	ObjectQuery single_query_is_not_true_1{QLatin1String("a"), ObjectQuery::NOT_OP, QLatin1String("b")};
	QVERIFY(single_query_is_not_true_1(object) == true);
	ObjectQuery single_query_is_not_true_2{QLatin1String("d"), ObjectQuery::NOT_OP, QLatin1String("a")}; // key d doesn't exist
	QVERIFY(single_query_is_not_true_2(object) == true);
	ObjectQuery single_query_is_not_false{QLatin1String("a"), ObjectQuery::NOT_OP, QLatin1String("a")};
	QVERIFY(single_query_is_not_false(object) == false);
}

void ObjectQueryTest::testContainsQuery()
{
	Object* object = makeTestObject();

	ObjectQuery single_query_contains_true_1{QLatin1String("abc"), ObjectQuery::CONTAINS_OP, QLatin1String("c")};
	QVERIFY(single_query_contains_true_1(object) == true);
	ObjectQuery single_query_contains_true_2{QLatin1String("abc"), ObjectQuery::CONTAINS_OP, QLatin1String("bc")};
	QVERIFY(single_query_contains_true_2(object) == true);
	ObjectQuery single_query_contains_true_3{QLatin1String("abc"), ObjectQuery::CONTAINS_OP, QLatin1String("abc")};
	QVERIFY(single_query_contains_true_3(object) == true);
	ObjectQuery single_query_contains_true_4{QLatin1String("abc"), ObjectQuery::CONTAINS_OP, QLatin1String("")}; // check for space used to check if tag exists
	QVERIFY(single_query_contains_true_4(object) == true);
	ObjectQuery single_query_contains_false_1{QLatin1String("abc"), ObjectQuery::CONTAINS_OP, QLatin1String("abcd")};
	QVERIFY(single_query_contains_false_1(object) == false);
	ObjectQuery single_query_contains_false_2{QLatin1String("d"), ObjectQuery::CONTAINS_OP, QLatin1String("")}; // key d doesn't exist
	QVERIFY(single_query_contains_false_2(object) == false);
}

void ObjectQueryTest::testOrQuery()
{
	Object* object = makeTestObject();

	auto true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	auto true_2 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	ObjectQuery single_query_or_both_true{std::move(true_1), ObjectQuery::OR_OP, std::move(true_2)};
	QVERIFY(single_query_or_both_true(object) == true);

	true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	auto false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_or_left_true_right_false{std::move(true_1), ObjectQuery::OR_OP, std::move(false_1)};
	QVERIFY(single_query_or_left_true_right_false(object) == true);

	true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_or_right_true_left_false{std::move(false_1), ObjectQuery::OR_OP, std::move(true_1)};
	QVERIFY(single_query_or_right_true_left_false(object) == true);

	false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	auto false_2 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_or_both_false{std::move(false_1), ObjectQuery::OR_OP, std::move(false_2)};
	QVERIFY(single_query_or_both_false(object) == false);
}

void ObjectQueryTest::testAndQuery()
{
	Object* object = makeTestObject();

	auto true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	auto true_2 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	ObjectQuery single_query_and_both_true{std::move(true_1), ObjectQuery::AND_OP, std::move(true_2)};
	QVERIFY(single_query_and_both_true(object) == true);

	true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	auto false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_and_left_true_right_false{std::move(true_1), ObjectQuery::AND_OP, std::move(false_1)};
	QVERIFY(single_query_and_left_true_right_false(object) == false);

	true_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("a")));
	false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_and_right_true_left_false{std::move(false_1), ObjectQuery::AND_OP, std::move(true_1)};
	QVERIFY(single_query_and_right_true_left_false(object) == false);

	false_1 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	auto false_2 = std::unique_ptr<ObjectQuery>(new ObjectQuery(QLatin1String("a"), ObjectQuery::IS_OP, QLatin1String("b")));
	ObjectQuery single_query_and_both_false{std::move(false_1), ObjectQuery::AND_OP, std::move(false_2)};
	QVERIFY(single_query_and_both_false(object) == false);
}

QTEST_APPLESS_MAIN(ObjectQueryTest)
