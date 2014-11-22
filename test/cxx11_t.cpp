/*
 *    Copyright 2014 Kai Pastor
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


#include "cxx11_t.h"

namespace
{
	int one()
	{
		return 1;
	}
	
	template <class T>
	auto echo(T arg)->decltype(arg)
	{
		return arg;
	}
}

Cxx11Test::Cxx11Test(QObject* parent)
: QObject(parent)
{
	// nothing
}

void Cxx11Test::testAuto()
{
	
	auto a = one();
	QCOMPARE(a, 1);
	
	auto b = echo(2);
	QCOMPARE(b, 2);
	
	auto c = 3;
	QCOMPARE(c, 3);
	
	auto d = "4";
	QCOMPARE(d, "4");
}

void Cxx11Test::testInitializers()
{
	struct A
	{
		int a_int;
		double a_double;
	};
	
	A a = { 1, 2.0 };
	QCOMPARE(a.a_int, 1);
	QCOMPARE(a.a_double, 2.0);
}


QTEST_MAIN(Cxx11Test)
