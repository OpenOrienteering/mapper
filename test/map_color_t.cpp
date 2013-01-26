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
			default:                        ba += QString("unknown %1").arg(method);
		}
		return qstrdup(ba.data());
	}
	
	template<>
	char* toString(const MapColor& c)
	{
		QByteArray ba = "MapColor(" + QString::number(c.getPriority()).toLocal8Bit();
		ba += " " + c.getName().toLocal8Bit();
		ba += " (SPOT: " + QString(toString(c.getSpotColorMethod())) + " " + c.getSpotColorName();
		ba += (c.getKnockout() ? " k.o." : " ---");
		ba += QString(" [%1%]").arg(c.getOpacity()*100.0,0,'f',0);
		ba += " CMYK: " + QString(toString(c.getCmykColorMethod()));
		const MapColorCmyk& cmyk = c.getCmyk();
		ba += QString(" %1/%2/%3/%4").arg(cmyk.c*100.0,0,'f',1).arg(cmyk.m*100.0,0,'f',1).arg(cmyk.y*100.0,0,'f',1).arg(cmyk.k*100.0,0,'f',1);
		ba += " RGB: " + QString(toString(c.getRgbColorMethod()));
		const MapColorRgb& rgb = c.getRgb();
		ba += QString(" %1/%2/%3").arg(rgb.r*255.0,0,'f',0).arg(rgb.g*255.0,0,'f',0).arg(rgb.b*255.0,0,'f',0);
		ba += ")";
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
	
	MapColor named_color("Name of the color", 7);
	QVERIFY(named_color.isBlack());
	QVERIFY(named_color.getCmyk().isBlack());
	QVERIFY(named_color.getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(named_color), QColor(Qt::black));
	QCOMPARE(named_color.getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(named_color.getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(named_color.getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(named_color.getPriority(),        7);
	QCOMPARE(named_color.getName(),            QString("Name of the color"));
	
	MapColor* duplicate_color = named_color.duplicate();
	QVERIFY(duplicate_color != NULL);
	QVERIFY(duplicate_color->isBlack());
	QVERIFY(duplicate_color->getCmyk().isBlack());
	QVERIFY(duplicate_color->getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(*duplicate_color), QColor(Qt::black));
	QCOMPARE(duplicate_color->getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(duplicate_color->getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(duplicate_color->getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(duplicate_color->getPriority(),        7);
	QCOMPARE(duplicate_color->getName(),            QString("Name of the color"));
	QVERIFY(duplicate_color->equals(named_color, true));
	delete duplicate_color;
	
	// Test default copy constructor.
	MapColor copy_constructed_color(named_color);
	duplicate_color = &copy_constructed_color;
	QVERIFY(duplicate_color != NULL);
	QVERIFY(duplicate_color->isBlack());
	QVERIFY(duplicate_color->getCmyk().isBlack());
	QVERIFY(duplicate_color->getRgb().isBlack());
	QCOMPARE(static_cast<const QColor&>(*duplicate_color), QColor(Qt::black));
	QCOMPARE(duplicate_color->getSpotColorMethod(), MapColor::UndefinedMethod);
	QCOMPARE(duplicate_color->getCmykColorMethod(), MapColor::CustomColor);
	QCOMPARE(duplicate_color->getRgbColorMethod(),  MapColor::CmykColor);
	QCOMPARE(duplicate_color->getPriority(),        7);
	QCOMPARE(duplicate_color->getName(),            QString("Name of the color"));
	QVERIFY(duplicate_color->equals(named_color, true));
	duplicate_color = NULL;
}

void MapColorTest::equalsTest()
{
	MapColor black("Black", 0);
	
	// Difference in priority: equals' result depends on second attribute. 
	MapColor black_1("Black", 1);
	QVERIFY(black_1.equals(black, false));
	QVERIFY(black.equals(black_1, false));
	QVERIFY(!black_1.equals(black, true));
	QVERIFY(!black.equals(black_1, true));
	
	// Difference in case of name: equals operates case-insensitive.
	black_1.setName("BLACK");
	QVERIFY(black_1.equals(black, false));
	QVERIFY(!black_1.equals(black, true));
	
	// Difference in knockout attribute.
	black_1.setKnockout(!black.getKnockout());
	QVERIFY(!black_1.equals(black, false));
	QVERIFY(!black_1.equals(black, true));
	
	// Difference in name.
	MapColor blue("Blue", 0);
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
	// Initalizing a spot color.
	MapColor spot_cyan("Cyan", 0);
	spot_cyan.setSpotColorName("CYAN");
	QCOMPARE(spot_cyan.getSpotColorMethod(), MapColor::SpotColor);
	QCOMPARE(spot_cyan.getSpotColorName(), QString("CYAN"));
	
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
	
	// Renaming the clone's spot color name.
	spot_cyan_copy.setSpotColorName("CYAN2");
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::SpotColor);
	QCOMPARE(spot_cyan_copy.getSpotColorName(), QString("CYAN2"));        // new
	QVERIFY(!spot_cyan_copy.equals(spot_cyan, true));
	QCOMPARE(spot_cyan_copy.getCmykColorMethod(), MapColor::CustomColor); // unchanged
	QCOMPARE(spot_cyan_copy.getCmyk(), spot_cyan.getCmyk());              // unchanged
	
	// Setting a composition
	SpotColorComponents composition;
	composition.push_back(SpotColorComponent(&spot_cyan, 1.0));
	spot_cyan_copy.setSpotColorComposition(composition);
	QCOMPARE(spot_cyan_copy.getSpotColorMethod(), MapColor::CustomColor); // new
	QVERIFY(spot_cyan_copy.getSpotColorName() != "CYAN2");                // new
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
}


QTEST_MAIN(MapColorTest)
