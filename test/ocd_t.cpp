/*
 *    Copyright 2018-2019 Kai Pastor
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
#include <iterator>
#include <numeric>
#include <stdexcept>

#include <QtGlobal>
#include <QtTest>
#include <QObject>
#include <QString>

#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v9.h"



/**
 * @test Unit test for the generic Ocd format support
 * 
 */
class OcdTest : public QObject
{
Q_OBJECT
	
private slots:
	void initTestCase()
	{
		// nothing yet
	}
	
	
	/**
	 * Tests basic properties of OCD V8 icon compression.
	 */
	void compressedOcdIconTest()
	{
		using std::begin;
		using std::end;
		
		{
			Ocd::IconV9 icon;
			
			// White background
			std::fill(begin(icon.bits), end(icon.bits), 124);
			QCOMPARE(icon.compress().uncompress(), icon);
			
			// Black bar on white background
			std::fill(begin(icon.bits)+44, begin(icon.bits)+88, 0);
			QCOMPARE(icon.compress().uncompress(), icon);
			
			// 250 non-repeating bytes of input, followed by white
			// => cannot compress to less than 256 bytes
			std::iota(begin(icon.bits), begin(icon.bits)+125, 0);
			std::transform(begin(icon.bits), begin(icon.bits)+63, begin(icon.bits)+125, [](auto value) {
				return value * 2;
			});
			std::transform(begin(icon.bits), begin(icon.bits)+62, begin(icon.bits)+188, [](auto value) {
				return value * 2 + 1;
			});
			QVERIFY_EXCEPTION_THROWN(icon.compress(), std::length_error);
			
			// 197 non-repeating bytes of input, followed by white
			// => cannot compress to less than 256 bytes
			std::fill(begin(icon.bits)+197, end(icon.bits), 124);
			QVERIFY_EXCEPTION_THROWN(icon.compress(), std::length_error);
			
			// 196 non-repeating bytes of input, followed by white
			std::fill(begin(icon.bits)+196, end(icon.bits), 124);
			QCOMPARE(icon.compress().uncompress(), icon);
			
			QCOMPARE(int(*std::max_element(begin(icon.bits), end(icon.bits))), 124);
		}		
		{
			Ocd::IconV8 icon {};
			QVERIFY_EXCEPTION_THROWN(icon.uncompress(), std::domain_error);
		}
	}
	
};  // class OcdTest



QTEST_APPLESS_MAIN(OcdTest)
#include "ocd_t.moc"  // IWYU pragma: keep
