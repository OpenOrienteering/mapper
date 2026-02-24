/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2022 Kai Pastor
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

#include <memory>
#include <algorithm>

#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QLatin1String>
#include <QString>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/objects/object_query.h"
#include "core/symbols/point_symbol.h"

using namespace OpenOrienteering;

namespace QTest
{
	template<>
	char* toString(const ObjectQuery& query)
	{
		return qstrdup(query.toString().toLatin1());
	}
}


ObjectQueryTest::ObjectQueryTest(QObject* parent)
: QObject(parent)
{
	// nothing
}

const Object* ObjectQueryTest::testObject()
{
	static TextObject obj;
	if (obj.tags().empty())
	{
		KeyValueContainer tags;
		tags.insert_or_assign(QLatin1String("a"), QLatin1String("1"));
		tags.insert_or_assign(QLatin1String("b"), QLatin1String("2"));
		tags.insert_or_assign(QLatin1String("c"), QLatin1String("3"));
		tags.insert_or_assign(QLatin1String("abc"), QLatin1String("123"));
		obj.setTags(tags);
		obj.setText(QStringLiteral("ac 13"));
	}
	Q_ASSERT(!obj.tags().contains(QLatin1String("d")));
	return &obj;
}

void ObjectQueryTest::testIsQuery()
{
	auto object = testObject();

	ObjectQuery single_query_is_true{QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1")};
	QVERIFY(single_query_is_true(object) == true);
	QVERIFY(single_query_is_true == single_query_is_true);
	QVERIFY(single_query_is_true != ObjectQuery{});
	ObjectQuery single_query_is_false_1{QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2")};
	QVERIFY(single_query_is_false_1(object) == false);
	ObjectQuery single_query_is_false_2{QLatin1String("d"), ObjectQuery::OperatorIs, QLatin1String("1")}; // key d doesn't exist
	QVERIFY(single_query_is_false_2(object) == false);
	QVERIFY(single_query_is_false_2 != single_query_is_true);
	QVERIFY(single_query_is_true != single_query_is_false_2);
	
	auto query_copy_true = ObjectQuery(single_query_is_true);
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
	QVERIFY(single_query_or_both_true == single_query_or_both_true);
	QVERIFY(single_query_or_both_true != ObjectQuery{});

	true_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("1"));
	auto false_1 = ObjectQuery(QLatin1String("a"), ObjectQuery::OperatorIs, QLatin1String("2"));
	ObjectQuery single_query_or_left_true_right_false{std::move(true_1), ObjectQuery::OperatorOr, std::move(false_1)};
	QVERIFY(single_query_or_left_true_right_false(object) == true);
	QVERIFY(single_query_or_left_true_right_false != single_query_or_both_true);
	QVERIFY(single_query_or_both_true != single_query_or_left_true_right_false);

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
	
	auto query_copy_true = ObjectQuery(single_query_and_both_true);
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
	
	auto clone = ObjectQuery(single_query_is_true_1);
	QCOMPARE(clone, single_query_is_true_1);
	QVERIFY(clone != single_query_is_true_2);
	auto operands = clone.tagOperands();
	QVERIFY(operands);
	QCOMPARE(operands->value, QLatin1String("Bc"));
}

void ObjectQueryTest::testObjectText()
{
	auto object = testObject();

	ObjectQuery single_query_is_true_1{ObjectQuery::OperatorObjectText, QLatin1String("Bc")};
	QVERIFY(single_query_is_true_1(object) == false);
	ObjectQuery single_query_is_true_2{ObjectQuery::OperatorObjectText, QLatin1String("23")};
	QVERIFY(single_query_is_true_2(object) == false);
	ObjectQuery single_query_is_false_1{ObjectQuery::OperatorObjectText, QLatin1String("Ac")};
	QVERIFY(single_query_is_false_1(object) == true);
	ObjectQuery single_query_is_false_2{ObjectQuery::OperatorObjectText, QLatin1String("13")};
	QVERIFY(single_query_is_false_2(object) == true);
	
	auto clone = ObjectQuery(single_query_is_true_1);
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
	
	auto clone = ObjectQuery(symbol_query);
	QCOMPARE(clone, symbol_query);
	QVERIFY(clone != ObjectQuery{});
	operand = clone.symbolOperand();
	QVERIFY(operand);
	QCOMPARE(operand, &symbol_2);
	
	symbol_query = ObjectQuery(static_cast<Symbol*>(nullptr));
	QVERIFY(bool(symbol_query));
	QVERIFY(symbol_query(&object) == false);
	object.setSymbol(nullptr, true);
	QVERIFY(symbol_query(&object) == true);
}

void ObjectQueryTest::testNegation()
{
	PointSymbol symbol_1;
	PointObject object(&symbol_1);
	
	// variation of testSymbol
	QVERIFY(ObjectQuery::negation({&symbol_1})(&object) == false);
	
	PointSymbol symbol_2;
	QVERIFY(ObjectQuery::negation({&symbol_2})(&object) == true);
	
	object.setSymbol(&symbol_2, false);
	QVERIFY(ObjectQuery::negation({&symbol_2})(&object) == false);
}


void ObjectQueryTest::testToString()
{
	auto q = ObjectQuery(ObjectQuery::OperatorSearch, QStringLiteral("1"));
	QCOMPARE(q.toString(), QStringLiteral("\"1\""));
	
	q = ObjectQuery(ObjectQuery::OperatorSearch, QStringLiteral("1\"\\x"));
	QCOMPARE(q.toString(), QStringLiteral("\"1\\\"\\\\x\""));
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorOr,
                    {ObjectQuery::OperatorSearch, QStringLiteral("2")});
	QCOMPARE(q.toString(), QStringLiteral("\"1\" OR \"2\""));
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorOr,
	                {{ObjectQuery::OperatorSearch, QStringLiteral("2")},
	                 ObjectQuery::OperatorAnd,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("3")}});
	QCOMPARE(q.toString(), QStringLiteral("\"1\" OR \"2\" AND \"3\""));
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorAnd,
	                {{ObjectQuery::OperatorSearch, QStringLiteral("2")},
	                 ObjectQuery::OperatorOr,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("3")}});
	QCOMPARE(q.toString(), QStringLiteral("\"1\" AND (\"2\" OR \"3\")"));
	
	q = ObjectQuery({QStringLiteral("Layer"), ObjectQuery::OperatorIs, QStringLiteral("1")},
	                ObjectQuery::OperatorAnd,
	                {QStringLiteral("A B C"), ObjectQuery::OperatorIsNot, QStringLiteral("3")});
	QCOMPARE(q.toString(), QStringLiteral("Layer = \"1\" AND \"A B C\" != \"3\""));
	
	q = ObjectQuery(static_cast<Symbol*>(nullptr));
	QCOMPARE(q.toString(), QStringLiteral("SYMBOL \"\""));
	
	PointSymbol symbol_1;
	symbol_1.setNumberComponent(0, 123);
	q = ObjectQuery(static_cast<Symbol*>(&symbol_1));
	QCOMPARE(q.toString(), QStringLiteral("SYMBOL \"123\""));
	
	q = ObjectQuery(ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")}));
	QCOMPARE(q.toString(), QStringLiteral("NOT \"1\""));
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("2")}));
	QCOMPARE(q.toString(), QStringLiteral("\"1\" AND NOT \"2\""));
	
	q = ObjectQuery({{ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("2")})},
	                ObjectQuery::OperatorOr,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(q.toString(), QStringLiteral("\"1\" AND NOT \"2\" OR \"3\""));
	
	q = ObjectQuery(ObjectQuery::negation(ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")})),
	                ObjectQuery::OperatorAnd,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(q.toString(), QStringLiteral("NOT NOT \"1\" AND \"3\""));
}


void ObjectQueryTest::testParser()
{
	ObjectQueryParser p;
	
	ObjectQuery q = ObjectQuery(ObjectQuery::OperatorSearch, QStringLiteral("1"));
	QCOMPARE(p.parse(QStringLiteral("1")), q);
	QCOMPARE(p.parse(QStringLiteral("\t \"1\"  \t ")), q);
	
	q = ObjectQuery(QStringLiteral("A"), ObjectQuery::OperatorIs, QStringLiteral("1"));
	QCOMPARE(p.parse(QStringLiteral("A = 1")), q);
	QCOMPARE(p.parse(QStringLiteral("A = \"1\"")), q);
	QCOMPARE(p.parse(QStringLiteral("\"A\" = 1")), q);
	QCOMPARE(p.parse(QStringLiteral("\"A\"=	1")), q);
	QCOMPARE(p.parse(QStringLiteral(" (  A = 1)\t")), q);
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorOr,
	                {ObjectQuery::OperatorSearch, QStringLiteral("2")});
	QCOMPARE(p.parse(QStringLiteral("1 OR 2")), q);
	QCOMPARE(p.parse(QStringLiteral("\"1\" OR 2")), q);
	QCOMPARE(p.parse(QStringLiteral("(1 OR \"2\")")), q);
	QCOMPARE(p.parse(QStringLiteral("((1) OR (\"2\"))")), q);
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorOr,
	                {{ObjectQuery::OperatorSearch, QStringLiteral("2")},
	                 ObjectQuery::OperatorAnd,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("3")}});
	QCOMPARE(p.parse(QStringLiteral("1 OR 2 AND 3")), q);
	QCOMPARE(p.parse(QStringLiteral("(1 OR 2 AND 3)")), q);
	QCOMPARE(p.parse(QStringLiteral("1 OR (\"2\" AND 3)")), q);
	
	q = ObjectQuery({{ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("2")}},
	                ObjectQuery::OperatorOr,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(p.parse(QStringLiteral("1 AND 2 OR 3")), q);
	QCOMPARE(p.parse(QStringLiteral("(1 AND 2) OR 3")), q);
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                ObjectQuery::OperatorAnd,
	                {{ObjectQuery::OperatorSearch, QStringLiteral("2")},
	                 ObjectQuery::OperatorOr,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("3")}});
	QCOMPARE(p.parse(QStringLiteral("1 AND (2 OR 3)")), q);
	
	QVERIFY(p.parse(QStringLiteral("1 AND (2 OR 3")).getOperator() == ObjectQuery::OperatorInvalid);
	QVERIFY(p.parse(QStringLiteral("1 AND (2 OR 3))")).getOperator() == ObjectQuery::OperatorInvalid);
	QVERIFY(p.parse(QStringLiteral("(1 AND (2 OR 3)")).getOperator() == ObjectQuery::OperatorInvalid);
	
	q = p.parse(QStringLiteral("AND 2"));
	QVERIFY(q.getOperator() == ObjectQuery::OperatorInvalid);
	QCOMPARE(p.errorPos(), 0);
	
	q = p.parse(QStringLiteral("1 2"));
	QVERIFY(q.getOperator() == ObjectQuery::OperatorInvalid);
	QCOMPARE(p.errorPos(), 2);
	
	q = p.parse(QStringLiteral("2 AND OR"));
	QVERIFY(q.getOperator() == ObjectQuery::OperatorInvalid);
	QCOMPARE(p.errorPos(), 6);
	
	q = ObjectQuery(ObjectQuery::OperatorSearch, QStringLiteral("1\"\\x"));
	QCOMPARE(p.parse(QStringLiteral("\"1\\\"\\\\x\"")), q);
	
	q = ObjectQuery(static_cast<Symbol*>(nullptr));
	QCOMPARE(p.parse(QStringLiteral("SYMBOL \"\"")), q);
	
	q = ObjectQuery(ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")}));
	QCOMPARE(p.parse(QStringLiteral("NOT \"1\"")), q);
	
	q = ObjectQuery(ObjectQuery::negation(ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")})));
	QCOMPARE(p.parse(QStringLiteral("NOT NOT \"1\"")), q);
	QCOMPARE(p.parse(QStringLiteral("NOT (NOT \"1\")")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT NOT \"1\")")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT NOT (\"1\"))")), q);
	
	q = ObjectQuery({ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("2")}));
	QCOMPARE(p.parse(QStringLiteral("\"1\" AND NOT \"2\"")), q);
	
	q = ObjectQuery({{ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("2")})},
	                ObjectQuery::OperatorOr,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(p.parse(QStringLiteral("1 AND NOT 2 OR 3")), q);
	QCOMPARE(p.parse(QStringLiteral("1 AND (NOT 2) OR 3")), q);
	QCOMPARE(p.parse(QStringLiteral("(1 AND NOT 2) OR 3")), q);
	
	q = ObjectQuery({ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")}),
	                 ObjectQuery::OperatorAnd,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("2")}},
	                ObjectQuery::OperatorOr,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(p.parse(QStringLiteral("NOT 1 AND 2 OR 3")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT 1) AND 2 OR 3")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT 1 AND 2) OR 3")), q);
	
	q = ObjectQuery({{ObjectQuery::OperatorSearch, QStringLiteral("1")},
	                 ObjectQuery::OperatorAnd,
	                 {ObjectQuery::OperatorSearch, QStringLiteral("2")}},
	                ObjectQuery::OperatorOr,
	                ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("3")}));
	QCOMPARE(p.parse(QStringLiteral("1 AND 2 OR NOT 3")), q);
	QCOMPARE(p.parse(QStringLiteral("1 AND 2 OR (NOT 3)")), q);
	QCOMPARE(p.parse(QStringLiteral("(1 AND 2) OR NOT 3")), q);
	
	q = ObjectQuery(ObjectQuery::negation(ObjectQuery::negation({ObjectQuery::OperatorSearch, QStringLiteral("1")})),
	                ObjectQuery::OperatorAnd,
	                {ObjectQuery::OperatorSearch, QStringLiteral("3")});
	QCOMPARE(p.parse(QStringLiteral("NOT NOT \"1\" AND \"3\"")), q);
	QCOMPARE(p.parse(QStringLiteral("NOT (NOT \"1\") AND \"3\"")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT NOT \"1\") AND \"3\"")), q);
	QCOMPARE(p.parse(QStringLiteral("(NOT (NOT \"1\")) AND \"3\"")), q);
	
	q = ObjectQuery(QStringLiteral("A"), ObjectQuery::OperatorIs, QStringLiteral("\"1\""));	// "1"
	QCOMPARE(p.parse(QStringLiteral("A = \"\\\"1\\\"\"")), q);	// A = "\"1\""
	q = ObjectQuery(QStringLiteral("A"), ObjectQuery::OperatorIs, QStringLiteral("\"\\1\""));	// "\1"
	QCOMPARE(p.parse(QStringLiteral("A = \"\\\"\\\\1\\\"\"")), q);	// A = "\"\\1\""
	
	Map m;
	auto* symbol_1 = new PointSymbol();
	symbol_1->setNumberComponent(0, 123);
	m.addSymbol(symbol_1, 0);
	p.setMap(&m);
	
	q = ObjectQuery(static_cast<Symbol*>(symbol_1));
	QCOMPARE(p.parse(QStringLiteral("SYMBOL 123")), q);
	QCOMPARE(p.parse(QStringLiteral("SYMBOL \"123\"")), q);
	
	q = ObjectQuery(static_cast<Symbol*>(nullptr));
	QCOMPARE(p.parse(QStringLiteral("SYMBOL \"\"")), q);
}


/*
 * We don't need a real GUI window.
 */
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}


QTEST_MAIN(ObjectQueryTest)
