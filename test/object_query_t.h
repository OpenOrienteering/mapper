/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2020, 2025 Kai Pastor
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

#ifndef OPENORIENTEERING_OBJECT_QUERY_T_H
#define OPENORIENTEERING_OBJECT_QUERY_T_H

#include <QObject>

namespace OpenOrienteering { class Object; }
using OpenOrienteering::Object;


/**
 * @test Tests object query
 */
class ObjectQueryTest : public QObject
{
Q_OBJECT
public:
	explicit ObjectQueryTest(QObject* parent = nullptr);
	
private slots:
	void testIsQuery();
	void testIsNotQuery();
	void testContainsQuery();
	void testOrQuery();
	void testAndQuery();
	void testSearch();
	void testObjectText();
	void testSymbol();
	void testNegation();
	void testToString();
	void testParser();
	void testFindObjects();

private:
	const Object* testObject();
};

#endif
