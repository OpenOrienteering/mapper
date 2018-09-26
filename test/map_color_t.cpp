/*
 *    Copyright 2012, 2013 Kai Pastor
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


#include "map_color_t.h"

#include <memory>

#include <Qt>
#include <QtTest>
#include <QByteArray>
#include <QColor>
#include <QLatin1String>
#include <QScopedPointer>
#include <QString>

#include "core/map_color.h"

using namespace OpenOrienteering;


namespace QTest
{
	template<>
	char* toString(const MapColor::ColorMethod& method)
	{
		QByteArray ba;
		switch (method)
		{
			case MapColor::UndefinedMethod: ba = "UndefinedMethod"; break;
			case MapColor::CustomColor:     ba = "CustomColor"; break;
			case MapColor::SpotColor:       ba = "SpotColor"; break;
			case MapColor::CmykColor:       ba = "CmykColor"; break;
			case MapColor::RgbColor:        ba = "RgbColor"; break;
			default:                        ba += "unknown " + QString::number(method).toLatin1();
		}
		return qstrdup(ba.data());
	}
	
	template<>
	char* toString(const MapColor& c)
	{
		auto spot_method = QTest::toString(c.getSpotColorMethod());
		auto cmyk_method = QTest::toString(c.getCmykColorMethod());
		auto rgb_method  = QTest::toString(c.getRgbColorMethod());
		
		QByteArray ba;
		ba.reserve(1000);
		ba += "MapColor(";
		ba += QByteArray::number(c.getPriority());
		ba += " ";
		ba += c.getName().toLocal8Bit();
		ba += " (SPOT: ";
		ba += spot_method;
		ba += " ";
		ba += c.getSpotColorName().toLocal8Bit();
		ba += (c.getKnockout() ? " k.o." : " ---");
		ba += " [";
		ba += QByteArray::number(c.getOpacity()*100.0,'f',0);
		ba += "] CMYK: ";
		ba += cmyk_method;
		const MapColorCmyk& cmyk = c.getCmyk();
		ba += QString::fromLatin1(" %1/%2/%3/%4").arg(cmyk.c*100.0,0,'f',1).arg(cmyk.m*100.0,0,'f',1).arg(cmyk.y*100.0,0,'f',1).arg(cmyk.k*100.0,0,'f',1).toLocal8Bit();
		ba += " RGB: ";
		ba += rgb_method;
		const MapColorRgb& rgb = c.getRgb();
		ba += QString::fromLatin1(" %1/%2/%3").arg(rgb.r*255.0,0,'f',0).arg(rgb.g*255.0,0,'f',0).arg(rgb.b*255.0,0,'f',0).toLocal8Bit();
		ba += ")";
		
		delete [] spot_method;
		delete [] cmyk_method;
		delete [] rgb_method;
		
		return qstrdup(ba.data());
	}
}


MapColorTest::MapColorTest(QObject* parent)
: QObject(parent)
{
	// nothing
}

void MapColorTest::constructorTest()
{
	MapColor default_color;
	QVERIFY(default_color.isBlack());
	QVERIFY(default_color.getCmyk().isBlack());
	QVERIFY(default_color.getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(default_color), QColor(Qt::black));
	QCOMPARE(default_color.getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(default_color.getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(default_color.getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(default_color.getPriority(),        (int)MapColor::Undefined);
	
	MapColor priority_9_color(9);
	QVERIFY(priority_9_color.isBlack());
	QVERIFY(priority_9_color.getCmyk().isBlack());
	QVERIFY(priority_9_color.getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(priority_9_color), QColor(Qt::black));
	QCOMPARE(priority_9_color.getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(priority_9_color.getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(priority_9_color.getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(priority_9_color.getPriority(),        9);
	
	MapColor named_color(QString::fromLatin1("Name of the color"), 7);
	QVERIFY(named_color.isBlack());
	QVERIFY(named_color.getCmyk().isBlack());
	QVERIFY(named_color.getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(named_color), QColor(Qt::black));
	QCOMPARE(named_color.getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(named_color.getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(named_color.getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(named_color.getPriority(),        7);
	QCOMPARE(named_color.getName(),            QString::fromLatin1("Name of the color"));
	
	MapColor* duplicate_color = named_color.duplicate();
	QVERIFY(duplicate_color != nullptr);
	QVERIFY(duplicate_color->isBlack());
	QVERIFY(duplicate_color->getCmyk().isBlack());
	QVERIFY(duplicate_color->getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(*duplicate_color), QColor(Qt::black));
	QCOMPARE(duplicate_color->getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(duplicate_color->getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(duplicate_color->getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(duplicate_color->getPriority(),        7);
	QCOMPARE(duplicate_color->getName(),            QString::fromLatin1("Name of the color"));
	QVERIFY(duplicate_color->equals(named_color, true));
	delete duplicate_color;
	
	// Test default copy constructor.
	MapColor copy_constructed_color(named_color);
	duplicate_color = &copy_constructed_color;
	QVERIFY(duplicate_color != nullptr);
	QVERIFY(duplicate_color->isBlack());
	QVERIFY(duplicate_color->getCmyk().isBlack());
	QVERIFY(duplicate_color->getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(*duplicate_color), QColor(Qt::black));
	QCOMPARE(duplicate_color->getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(duplicate_color->getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(duplicate_color->getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(duplicate_color->getPriority(),        7);
	QCOMPARE(duplicate_color->getName(),            QString::fromLatin1("Name of the color"));
	QVERIFY(duplicate_color->equals(named_color, true));
	duplicate_color = nullptr;
}

void MapColorTest::equalsTest()
{
	MapColor black(QString::fromLatin1("Black"), 0);
	
	// Difference in priority: equals' result depends on second attribute. 
	MapColor black_1(QString::fromLatin1("Black"), 1);
	QVERIFY(black_1.equals(black, false));
	QVERIFY(black.equals(black_1, false));
	QVERIFY(!black_1.equals(black, true));
	QVERIFY(!black.equals(black_1, true));
	
	// Difference in case of name: equals operates case-insensitive.
	black_1.setName(QString::fromLatin1("BLACK"));
	QVERIFY(black_1.equals(black, false));
	QVERIFY(!black_1.equals(black, true));
	
	// Difference in knockout attribute, spot color method undefined
	QVERIFY(black_1.getSpotColorMethod() == MapColor::UndefinedMethod);
	black_1.setKnockout(!black.getKnockout());
	QVERIFY(!black_1.getKnockout()); // must not be set
	QVERIFY(black_1.equals(black, false));
	QVERIFY(!black_1.equals(black, true));
	
	// Difference in knockout attribute, spot color method defined
	black_1.setSpotColorName(QString::fromLatin1("BLACK"));
	QVERIFY(black_1.getSpotColorMethod() == MapColor::SpotColor);
	black_1.setKnockout(!black.getKnockout());
	QVERIFY(black_1.getKnockout()); // must not be set
	QVERIFY(!black_1.equals(black, false));
	QVERIFY(!black_1.equals(black, true));
	
	// Difference in name.
	MapColor blue(QString::fromLatin1("Blue"), 0);
	QVERIFY(!blue.equals(black, false));
	QVERIFY(!black.equals(blue, false));
	QVERIFY(!blue.equals(black, true));
	QVERIFY(!black.equals(blue, true));
	
	// Equality after assignment (simple case).
	black_1 = black;
	QVERIFY(black_1.equals(black, false));
	QVERIFY(black.equals(black_1, false));
	QVERIFY(black_1.equals(black, true));
	QVERIFY(black.equals(black_1, true));
}

void MapColorTest::spotColorTest()
{
	QScopedPointer<MapColor> duplicate;
	
	// Initalizing a spot color.
	MapColor spot_cyan(QString::fromLatin1("Cyan"), 0);
	spot_cyan.setSpotColorName(QString::fromLatin1("CYAN"));
	QCOMPARE(spot_cyan.getSpotColorMethod(), MapColor::SpotColor);
	QCOMPARE(spot_cyan.getSpotColorName(), QString(QString::fromLatin1("CYAN")));
	QCOMPARE(spot_cyan.getScreenAngle(), 0.0);
	QVERIFY(spot_cyan.getScreenFrequency() <= 0);
	
	spot_cyan.setCmyk(MapColorCmyk(1.0, 0.0, 0.0, 0.0));
	QCOMPARE(spot_cyan.getSpotColorMethod(), MapColor::SpotColor);  // unchanged
	QCOMPARE(spot_cyan.getCmykColorMethod(), MapColor::CustomColor);
	QVERIFY(!spot_cyan.getCmyk().isBlack());
	QVERIFY(!spot_cyan.getCmyk().isWhite());
	QVERIFY(!spot_cyan.getRgb().isBlack());
	QVERIFY(!spot_cyan.getRgb().isWhite());
	
	// Cloning a spot color.
	MapColor spot_cyan_copy(spot_cyan);
	QCOMPARE(spot_cyan_copy, spot_cyan);
	duplicate.reset(spot_cyan_copy.duplicate());
	QCOMPARE(*duplicate, spot_cyan_copy);
	
	// Renaming the clone's spot color name.
	spot_cyan_copy.setSpotColorName(QString::fromLatin1("CYAN2"));
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::SpotColor);
	QCOMPARE(spot_cyan_copy.getSpotColorName(), QString(QString::fromLatin1("CYAN2")));        // new
	QVERIFY(!spot_cyan_copy.equals(spot_cyan, true));
	QCOMPARE(spot_cyan_copy.getCmykColorMethod(), MapColor::CustomColor); // unchanged
	QCOMPARE(spot_cyan_copy.getCmyk(), spot_cyan.getCmyk());              // unchanged
	
	// Setting a composition
	SpotColorComponents composition;
	composition.push_back(SpotColorComponent(&spot_cyan, 1.0));
	spot_cyan_copy.setSpotColorComposition(composition);
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::CustomColor); // new
	QVERIFY(spot_cyan_copy.getSpotColorName() != QLatin1String("CYAN2")); // new
	QCOMPARE(spot_cyan_copy.getCmykColorMethod(), MapColor::CustomColor); // unchanged
	QCOMPARE(spot_cyan_copy.getCmyk(), spot_cyan.getCmyk());              // unchanged
	
	// Determining CMYK from spot colors
	spot_cyan_copy.setCmykFromSpotColors();
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::CustomColor); // unchanged
	QCOMPARE(spot_cyan_copy.getCmykColorMethod(), MapColor::SpotColor);   // new
	QCOMPARE(spot_cyan_copy.getCmyk(), spot_cyan.getCmyk());              // unchanged
	
	// Setting a halftoning
	composition.front().factor = 0.5f;
	spot_cyan_copy.setSpotColorComposition(composition);
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::CustomColor); // unchanged
	QCOMPARE(spot_cyan_copy.getCmykColorMethod(), MapColor::SpotColor);   // unchanged
	QVERIFY(spot_cyan_copy.getCmyk() != spot_cyan.getCmyk());             // new
	QCOMPARE(spot_cyan_copy.getCmyk().c, 0.5f);                           // computed!
	
	// Copy and compare spot color composition
	MapColor compo_copy(spot_cyan_copy);
	QCOMPARE(compo_copy.getSpotColorMethod(), MapColor::CustomColor);
	QCOMPARE(compo_copy.getCmykColorMethod(), MapColor::SpotColor);
	QCOMPARE(compo_copy.getCmyk().c, 0.5f);
	QCOMPARE(compo_copy, spot_cyan_copy);
	duplicate.reset(spot_cyan_copy.duplicate());
	QCOMPARE(*duplicate, spot_cyan_copy);
	
	MapColor spot_yellow(QString::fromLatin1("Yellow"), 8);
	spot_yellow.setCmyk(MapColorCmyk(0.0, 0.0, 1.0, 0.0));
	
	composition.clear();
	composition.push_back(SpotColorComponent(&spot_cyan, 0.8));
	composition.push_back(SpotColorComponent(&spot_yellow, 0.8));
	spot_cyan_copy.setSpotColorComposition(composition);
	composition.clear();
	composition.push_back(SpotColorComponent(&spot_yellow, 0.8));
	composition.push_back(SpotColorComponent(&spot_cyan, 0.8));
	compo_copy.setSpotColorComposition(composition);
	QCOMPARE(compo_copy, spot_cyan_copy); // Equal! Order of compositions doesn't matter.
	
	// Test MapColor::equals() with cloned spot color compositions
	MapColor spot_yellow_copy(spot_yellow);
	composition.front().spot_color = &spot_yellow_copy;
	compo_copy.setSpotColorComposition(composition);
	QVERIFY(compo_copy.equals(spot_cyan_copy, false));
	QVERIFY(compo_copy.equals(spot_cyan_copy, true));
	spot_yellow_copy.setPriority(spot_yellow_copy.getPriority() + 1);
	QVERIFY(compo_copy.equals(spot_cyan_copy, false));
	QVERIFY(!compo_copy.equals(spot_cyan_copy, true));
	
	duplicate.reset(spot_cyan_copy.duplicate());
	QCOMPARE(*duplicate, spot_cyan_copy);
	spot_cyan_copy.setKnockout(!spot_cyan_copy.getKnockout());
	QVERIFY(spot_cyan_copy != *duplicate);
	duplicate.reset(spot_cyan_copy.duplicate());
	QCOMPARE(*duplicate, spot_cyan_copy);
	spot_cyan_copy.setKnockout(!spot_cyan_copy.getKnockout());
	QVERIFY(spot_cyan_copy != *duplicate);
	
	// Test MapColor::equals() with cloned spot color screens.
	{
		auto actual = spot_cyan;
		QCOMPARE(actual, spot_cyan);
		auto expected = spot_cyan;
		QCOMPARE(actual, expected);
		
		// Undefined screen must not affect equality.
		actual.setScreenAngle(45);
		QCOMPARE(actual, expected);
		actual.setScreenFrequency(150);
		QCOMPARE(actual, expected);
		expected.setScreenAngle(45);
		QCOMPARE(actual, expected);
		
		// If both screens are defined, equals() must take them into account.
		expected.setScreenFrequency(200);
		QVERIFY(actual != expected);
		actual.setScreenFrequency(200);
		QCOMPARE(actual, expected);
		actual.setScreenAngle(50);
		QVERIFY(actual != expected);
	}
}

void MapColorTest::miscTest()
{
	MapColor color;
	QVERIFY(color.isBlack());
	QVERIFY(!color.isWhite());
	color.setRgb({1.0, 1.0, 1.0});
	QVERIFY(!color.isBlack());
	QVERIFY(!color.isWhite());
	color.setCmyk({0, 0, 0, 0});
	QVERIFY(!color.isBlack());
	QVERIFY(color.isWhite());
}


QTEST_GUILESS_MAIN(MapColorTest)
