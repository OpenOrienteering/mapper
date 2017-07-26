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


#include "encoding_t.h"

#include <QtTest>
#include <QLatin1String>
#include <QLocale>
#include <QString>
#include <QTextCodec>

#include "util/encoding.h"


EncodingTest::EncodingTest(QObject* parent)
: QObject(parent)
{
	// nothing
}


void EncodingTest::testCodepageForLanguage_data()
{
	// We use the row data tag (const char*) for the language.
	QTest::addColumn<QString>("encoding");
	QTest::newRow("cs")    << QString::fromLatin1("Windows-1250");
	QTest::newRow("cs_x")  << QString::fromLatin1("Windows-1250");
	QTest::newRow("ru")    << QString::fromLatin1("Windows-1251");
	QTest::newRow("ru_RU") << QString::fromLatin1("Windows-1251");
	QTest::newRow("de")    << QString::fromLatin1("Windows-1252");
	QTest::newRow("??")    << QString::fromLatin1("Windows-1252");
}

void EncodingTest::testCodepageForLanguage()
{
	auto language = QTest::currentDataTag();
	QFETCH(QString, encoding);
	QCOMPARE(QLatin1String(Util::codepageForLanguage(QString::fromLatin1(language))), encoding);
}


void EncodingTest::testCodecForName()
{
	QVERIFY(Util::codecForName("Windows-1250") == Util::codecForName("Windows-1250"));
	QVERIFY(Util::codecForName("Windows-1250") != Util::codecForName("Windows-1251"));
	QVERIFY(Util::codecForName("Windows-1250") == QTextCodec::codecForName("Windows-1250"));
	QVERIFY(Util::codecForName("Windows-1250") != QTextCodec::codecForName("Windows-1251"));
	QVERIFY(Util::codecForName("Default") == QTextCodec::codecForName(Util::codepageForLanguage(QLocale().name())));
}


QTEST_APPLESS_MAIN(EncodingTest)
