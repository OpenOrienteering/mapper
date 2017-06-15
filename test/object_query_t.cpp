/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017 Kai Pastor
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

#include "core/objects/object.h"
#include "core/objects/object_query.h"
#include "core/symbols/point_symbol.h"


ObjectQueryTest::ObjectQueryTest(QObject* parent)
: QObject(parent)
{
	// nothing
}

const Object* ObjectQueryTest::testObject()
{
	static PathObject obj;
	if (obj.tags().isEmpty())
	{
		obj.setTags( {
		  { QLatin1String("a"), QLatin1String("1") },
		  { QLatin1String("b"), QLatin1String("2") },
		  { QLatin1String("c"), QLatin1String("3") },
		  { QLatin1String("abc"), QLatin1String("123") }
		});
	}
	Q_ASSERT(!obj.tags().contains(QLatin1String("d")));
	return &obj;
}

void ObjectQueryTest::testIsQuery()
{
	auto object = testObject();

	ObjectQuery single_query_is_true{QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1")};
	QVERIFY(single_query_is_true(object) == true);
	ObjectQuery single_query_is_false_1{QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2")};
	QVERIFY(single_query_is_false_1(object) == false);
	ObjectQuery single_query_is_false_2{QLatin1String("d"), ObjectQuery::OperatorIs, QLatin1String("1")}; // key d doesn't exist
	QVERIFY(single_query_is_false_2(object) == false);
	
	auto query_copy_true = single_query_is_true;
	QCOMPARE(query_copy_true.getOperator(), ObjectQuery::OperatorIs);
	QCOMPARE(query_copy_true, single_query_is_true);
	QVERIFY(query_copy_true(object) == true);
	QVERIFY(query_copy_true != single_query_is_false_1);
	
	auto query_moved_false = std::move(single_query_is_false_2);
	QCOMPARE(query_moved_false.getOperator(), ObjectQuery::OperatorIs);
	QCOMPARE(single_query_is_false_2.getOperator(), ObjectQuery::OperatorInvalid);
	QVERIFY(query_moved_false(object) == false);
	QVERIFY(query_moved_false != single_query_is_false_2);
	
	auto operands = query_moved_false.tagOperands();
	QVERIFY(operands);
	QCOMPARE(operands->key, QString::fromLatin1("d"));
	QCOMPARE(operands->value, QString::fromLatin1("1"));
}

void ObjectQueryTest::testIsNotQuery()
{
	auto object = testObject();

	ObjectQuery single_query_is_not_true_1{QLatin1String("a"), ObjectQuery::OperatorIsNot, QLatin1String("2")};
	QVERIFY(single_query_is_not_true_1(object) == true);
	ObjectQuery single_query_is_not_true_2{QLatin1String("d"), ObjectQuery::OperatorIsNot, QLatin1String("1")}; // key d doesn't exist
	QVERIFY(single_query_is_not_true_2(object) == true);
	ObjectQuery single_query_is_not_false{QLatin1String("a"), ObjectQuery::OperatorIsNot, QLatin1String("1")};
	QVERIFY(single_query_is_not_false(object) == false);
}

void ObjectQueryTest::testContainsQuery()
{
	auto object = testObject();

	ObjectQuery single_query_contains_true_0{QLatin1String("abc"), ObjectQuery::OperatorContains, QLatin1String("")};
	QVERIFY(single_query_contains_true_0(object) == true);
	ObjectQuery single_query_contains_true_1{QLatin1String("abc"), ObjectQuery::OperatorContains, QLatin1String("3")};
	QVERIFY(single_query_contains_true_1(object) == true);
	ObjectQuery single_query_contains_true_2{QLatin1String("abc"), ObjectQuery::OperatorContains, QLatin1String("12")};
	QVERIFY(single_query_contains_true_2(object) == true);
	ObjectQuery single_query_contains_true_3{QLatin1String("abc"), ObjectQuery::OperatorContains, QLatin1String("123")};
	QVERIFY(single_query_contains_true_3(object) == true);
	ObjectQuery single_query_contains_false_1{QLatin1String("abc"), ObjectQuery::OperatorContains, QLatin1String("1234")};
	QVERIFY(single_query_contains_false_1(object) == false);
	ObjectQuery single_query_contains_false_2{QLatin1String("d"), ObjectQuery::OperatorContains, QLatin1String("")}; // key d doesn't exist
	QVERIFY(single_query_contains_false_2(object) == false);
}

void ObjectQueryTest::testOrQuery()
{
	auto object = testObject();

	auto true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	auto true_2 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	ObjectQuery single_query_or_both_true{std::move(true_1), ObjectQuery::OperatorOr, std::move(true_2)};
	QVERIFY(single_query_or_both_true(object) == true);

	true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	auto false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_or_left_true_right_false{std::move(true_1), ObjectQuery::OperatorOr, std::move(false_1)};
	QVERIFY(single_query_or_left_true_right_false(object) == true);

	true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_or_right_true_left_false{std::move(false_1), ObjectQuery::OperatorOr, std::move(true_1)};
	QVERIFY(single_query_or_right_true_left_false(object) == true);

	false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	auto false_2 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_or_both_false{std::move(false_1), ObjectQuery::OperatorOr, std::move(false_2)};
	QVERIFY(single_query_or_both_false(object) == false);
	
	auto operands = single_query_or_both_false.logicalOperands();
	QVERIFY(operands);
	QVERIFY(operands->first.get());
	QCOMPARE(operands->first->getOperator(), ObjectQuery::OperatorIs);
	QVERIFY(operands->second.get());
	QCOMPARE(operands->second->getOperator(), ObjectQuery::OperatorIs);
}

void ObjectQueryTest::testAndQuery()
{
	auto object = testObject();

	auto true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	auto true_2 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	ObjectQuery single_query_and_both_true{std::move(true_1), ObjectQuery::OperatorAnd, std::move(true_2)};
	QVERIFY(single_query_and_both_true(object) == true);

	true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	auto false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_and_left_true_right_false{std::move(true_1), ObjectQuery::OperatorAnd, std::move(false_1)};
	QVERIFY(single_query_and_left_true_right_false(object) == false);

	true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_and_right_true_left_false{std::move(false_1), ObjectQuery::OperatorAnd, std::move(true_1)};
	QVERIFY(single_query_and_right_true_left_false(object) == false);

	false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	auto false_2 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_and_both_false{std::move(false_1), ObjectQuery::OperatorAnd, std::move(false_2)};
	QVERIFY(single_query_and_both_false(object) == false);
	
	auto query_and_invalid_1 = ObjectQuery(true_1, ObjectQuery::OperatorAnd, ObjectQuery());
	QCOMPARE(query_and_invalid_1.getOperator(), ObjectQuery::OperatorInvalid);
	QVERIFY(!query_and_invalid_1);
	
	auto query_and_invalid_2 = ObjectQuery(query_and_invalid_1, ObjectQuery::OperatorAnd, true_2);
	QCOMPARE(query_and_invalid_2.getOperator(), ObjectQuery::OperatorInvalid);
	QVERIFY(!query_and_invalid_2);
	
	auto query_copy_true = single_query_and_both_true;
	QCOMPARE(query_copy_true.getOperator(), ObjectQuery::OperatorAnd);
	QCOMPARE(query_copy_true, single_query_and_both_true);
	QVERIFY(query_copy_true(object) == true);
	QVERIFY(query_copy_true != single_query_and_left_true_right_false);
	
	auto query_moved_false = std::move(single_query_and_right_true_left_false);
	QCOMPARE(query_moved_false.getOperator(), ObjectQuery::OperatorAnd);
	QCOMPARE(single_query_and_right_true_left_false.getOperator(), ObjectQuery::OperatorInvalid);
	QVERIFY(query_moved_false(object) == false);
	
	auto query_and_invalid_3 = ObjectQuery(std::move(query_and_invalid_1), ObjectQuery::OperatorAnd, std::move(true_2));
	QCOMPARE(query_and_invalid_3.getOperator(), ObjectQuery::OperatorInvalid);
	QVERIFY(!query_and_invalid_3);
}

void ObjectQueryTest::testSearch()
{
	auto object = testObject();

	ObjectQuery single_query_is_true_1{ObjectQuery::OperatorSearch, QLatin1String("Bc")};
	QVERIFY(single_query_is_true_1(object) == true);
	ObjectQuery single_query_is_true_2{ObjectQuery::OperatorSearch, QLatin1String("23")};
	QVERIFY(single_query_is_true_2(object) == true);
	ObjectQuery single_query_is_false_1{ObjectQuery::OperatorSearch, QLatin1String("Ac")};
	QVERIFY(single_query_is_false_1(object) == false);
	ObjectQuery single_query_is_false_2{ObjectQuery::OperatorSearch, QLatin1String("13")};
	QVERIFY(single_query_is_false_2(object) == false);
	
	auto clone = single_query_is_true_1;
	QCOMPARE(clone, single_query_is_true_1);
	QVERIFY(clone != single_query_is_true_2);
	auto operands = clone.tagOperands();
	QVERIFY(operands);
	QCOMPARE(operands->value, QLatin1String("Bc"));
}

void ObjectQueryTest::testSymbol()
{
	PointSymbol symbol_1;
	PointObject object(&symbol_1);
	
	auto symbol_query = ObjectQuery(&symbol_1);
	QCOMPARE(symbol_query.getOperator(), ObjectQuery::OperatorSymbol);
	QVERIFY(symbol_query(&object) == true);
	
	PointSymbol symbol_2;
	symbol_query = ObjectQuery(&symbol_2);
	QVERIFY(symbol_query(&object) == false);
	
	object.setSymbol(&symbol_2, false);
	QVERIFY(symbol_query(&object) == true);
	
	auto operand = symbol_query.symbolOperand();
	QVERIFY(operand);
	QCOMPARE(operand, &symbol_2);
	
	auto clone = symbol_query;
	QCOMPARE(clone, symbol_query);
	QVERIFY(clone != ObjectQuery{});
	operand = clone.symbolOperand();
	QVERIFY(operand);
	QCOMPARE(operand, &symbol_2);
}

QTEST_APPLESS_MAIN(ObjectQueryTest)
