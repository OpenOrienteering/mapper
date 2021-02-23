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
#include <cstddef>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <type_traits>

#include <QtGlobal>
#include <QtTest>
#include <QByteArray>
#include <QChar>
#include <QObject>
#include <QString>

#include "fileformats/ocd_types.h"
#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v9.h"



/**
 * @test Unit test for the generic Ocd format support
 * 
 * The tests of the various string implementations use a string length of four
 * elements. Test data and test implementation are designed to ensure that
 * - assignments of shorter, max length, and more than max length characters
 *   are covered,
 * - unused storage is filled with zero (for privacy)
 * - data following the string storage is not accidentally overwritten.
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
	
	
	
	/*
	 * Ocd::PascalString<N> represents a string of N bytes, preceded by a byte
	 * giving the length. So a trailing zero is not required.
	 */
	void pascalStringTest_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QByteArray>("expected");
		
		QTest::newRow("1")         << QByteArray("1")         << QByteArray("1");
		QTest::newRow("123")       << QByteArray("123")       << QByteArray("123");
		QTest::newRow("1234")      << QByteArray("1234")      << QByteArray("1234");
		QTest::newRow("123456789") << QByteArray("123456789") << QByteArray("1234");
		QTest::newRow("257x A")    << QByteArray(257, 'A')    << QByteArray(4, 'A');
	}
	
	/**
	 * Tests the legacy pascal string implementation, Ocd::PascalString<N>.
	 */
	void pascalStringTest()
	{
		struct {
			Ocd::PascalString<4> string;
			char trailing[4];
		} t;
		
		auto* const expected_trailing = "xyz";
		qstrncpy(t.trailing, expected_trailing, 4);
		
		QFETCH(QByteArray, input);
		QFETCH(QByteArray, expected);
		
		t.string = input;
		QCOMPARE(t.string.length, static_cast<unsigned char>(expected.length()));
		QVERIFY(qstrncmp(t.string.data, expected.data(), t.string.length) == 0);
		
		using std::begin; using std::end;
		QVERIFY(std::all_of(begin(t.string.data)+t.string.length, end(t.string.data), [](auto c){ return c == 0; }));
		
		QCOMPARE(t.trailing, expected_trailing);
	}
	
	
	
	void Utf8PascalStringTest_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QByteArray>("expected");
		
		QTest::newRow("1")         << QByteArray("1")         << QByteArray("1");
		QTest::newRow("123")       << QByteArray("123")       << QByteArray("123");
		QTest::newRow("1234")      << QByteArray("1234")      << QByteArray("1234");
		QTest::newRow("123456789") << QByteArray("123456789") << QByteArray("1234");
		QTest::newRow("257x A")    << QByteArray(257, 'A')    << QByteArray(4, 'A');
		
		// trailing two-byte character
		QTest::newRow("123Cent")   << QByteArray("123¢")      << QByteArray("123");
		QTest::newRow("12Cent")    << QByteArray("12¢")       << QByteArray("12¢");
		
		// trailing three-byte character
		QTest::newRow("123Euro")   << QByteArray("123€")      << QByteArray("123");
		QTest::newRow("12Euro")    << QByteArray("12€")       << QByteArray("12");
		QTest::newRow("1Euro")     << QByteArray("1€")        << QByteArray("1€");
		
		// trailing four-byte character
		QTest::newRow("123Hwair")  << QByteArray("123𐍈")      << QByteArray("123");
		QTest::newRow("12Hwair")   << QByteArray("12𐍈")       << QByteArray("12");
		QTest::newRow("1Hwair")    << QByteArray("1𐍈")        << QByteArray("1");
		QTest::newRow("Hwair")     << QByteArray("𐍈")         << QByteArray("𐍈");
	}
	
	/**
	 * Tests the UTF-8 pascal string implementation, Ocd::Utf8PascalString<N>.
	 */
	void Utf8PascalStringTest()
	{
		struct {
			Ocd::Utf8PascalString<4> string;
			char trailing[4];
		} t;
		
		auto* const expected_trailing = "xyz";
		qstrncpy(t.trailing, expected_trailing, 4);
		
		QFETCH(QByteArray, input);
		QFETCH(QByteArray, expected);
		
		t.string = QString::fromUtf8(input);
		QCOMPARE(t.string.length, static_cast<unsigned char>(expected.length()));
		QVERIFY(qstrncmp(t.string.data, expected.data(), t.string.length) == 0);
		
		using std::begin; using std::end;
		QVERIFY(std::all_of(begin(t.string.data)+t.string.length, end(t.string.data), [](auto c){ return c == 0; }));
		
		QCOMPARE(t.trailing, expected_trailing);
	}
	
	
	
	void Utf16PascalStringTest_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("expected");
		
		QTest::newRow("1")         << QString::fromUtf8("1")         << QString::fromUtf8("1");
		QTest::newRow("123")       << QString::fromUtf8("123")       << QString::fromUtf8("123");
		QTest::newRow("1234")      << QString::fromUtf8("1234")      << QString::fromUtf8("123");
		QTest::newRow("123456789") << QString::fromUtf8("123456789") << QString::fromUtf8("123");
		
		// trailing surrogate pair
		QTest::newRow("12Yee")     << QString::fromUtf8("12𐐷")       << QString::fromUtf8("12");
		QTest::newRow("1Yee")      << QString::fromUtf8("1𐐷")        << QString::fromUtf8("1𐐷");
	}
	
	/**
	 * Tests the UTF-16 pascal string implementation, Ocd::Utf16PascalString<N>.
	 */
	void Utf16PascalStringTest()
	{
		struct {
			Ocd::Utf16PascalString<4> string;
			char trailing[4];
		} t;
		
		auto* const expected_trailing = "xyz";
		qstrncpy(t.trailing, expected_trailing, 4);
		
		QFETCH(QString, input);
		QFETCH(QString, expected);
		
		Q_ASSERT(static_cast<std::size_t>(expected.length()) < std::extent<decltype(t.string.data)>::value);
		
		t.string = input;
		QCOMPARE(QString::fromRawData(t.string.data, expected.length()), expected);
		
		using std::begin; using std::end;
		QVERIFY(std::all_of(begin(t.string.data)+expected.length(), end(t.string.data), [](auto c){ return c == 0; }));
		
		QCOMPARE(t.trailing, expected_trailing);
	}
	
};  // class OcdTest



QTEST_APPLESS_MAIN(OcdTest)
#include "ocd_t.moc"  // IWYU pragma: keep
