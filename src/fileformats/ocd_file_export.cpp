/*
 *    Copyright 2016-2018 Kai Pastor
 *
 *    Some parts taken from file_format_oc*d8{.h,_p.h,cpp} which are
 *    Copyright 2012 Pete Curtis
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

#include "ocd_file_export.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QColor>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFlags>
#include <QFontMetricsF>
#include <QIODevice>
#include <QImage>
#include <QLatin1Char>
#include <QLatin1String>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QRgb>
#include <QString>
#include <QTextCodec>
#include <QTextDecoder>
#include <QTextEncoder>
#include <QTextStream>
#include <QTransform>
#include <QVariant>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_grid.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/objects/object_operations.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "fileformats/ocd_file_format.h"
#include "fileformats/ocd_georef_fields.h"
#include "fileformats/ocd_types.h"
#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v9.h"
#include "fileformats/ocd_types_v11.h"
#include "fileformats/ocd_types_v12.h"
#include "templates/template.h"
#include "util/encoding.h"
#include "util/util.h"


namespace OpenOrienteering {

namespace {

/// \todo De-duplicate (ocd_file_import.cpp)
static QTextCodec* codecFromSettings()
{
	const auto& settings = Settings::getInstance();
	const auto name = settings.getSetting(Settings::General_Local8BitEncoding).toByteArray();
	return Util::codecForName(name);
}



/**
 * Returns the index of the first color which can be assumed to be a pure spot color.
 * 
 * During OCD import, spot colors are appended as regular colors at the end of
 * the color list. This function tries to recover the index where the spot colors
 * start, by looking at definition and usage of the colors at the end of the list.
 */
int beginOfSpotColors(const Map* map)
{
	const auto num_colors = map->getNumColors();
	auto first_pure_spot_color = num_colors;
	for (auto i = num_colors; i > 0; )
	{
		--i;
		const auto color = map->getColor(i);
		if (color->getSpotColorMethod() != MapColor::SpotColor
		    || map->isColorUsedByASymbol(color))
			break;
		--first_pure_spot_color;
	}
	return first_pure_spot_color;
}



quint32 makeSymbolNumber(const Symbol* symbol, quint32 symbol_number_factor)
{
	auto number = quint32(0);
	if (symbol->getNumberComponent(1) >= 0)
	{
		number = quint32(symbol->getNumberComponent(1));
		if (symbol->getNumberComponent(2) >= 0 && symbol_number_factor >= 1000)
			number = number * 100 + quint32(symbol->getNumberComponent(2)) % 100;
	}
	number = quint32(symbol->getNumberComponent(0)) * symbol_number_factor + number % symbol_number_factor;
	// Symbol number 0.0 is not valid
	return (number == 0) ? 1 : number;
}



void copySymbolHead(const Symbol& source, Symbol& symbol)
{
	for (auto i = 0u; i < Symbol::number_components; ++i)
		symbol.setNumberComponent(i, source.getNumberComponent(i));
	symbol.setName(source.getName());
	symbol.setHidden(source.isHidden());
	symbol.setProtected(source.isProtected());
	symbol.setHidden(source.isHidden());
	symbol.setProtected(source.isProtected());
}



/**
 * Test if a Mapper line symbol can be represented by the double line/filling aspect
 * of an OCD line symbol.
 * 
 * This function helps to merge elements of combined symbols into OCD line symbols.
 */
bool maybeDoubleFilling(const LineSymbol* line)
{
	return line
	        && line->hasBorder()
	        && line->getCapStyle() != LineSymbol::PointedCap
	        && (!line->getDashSymbol() || line->getDashSymbol()->isEmpty())
	        && (!line->getMidSymbol() || line->getMidSymbol()->isEmpty())
	        && (!line->getStartSymbol() || line->getStartSymbol()->isEmpty())
	        && (!line->getEndSymbol() || line->getEndSymbol()->isEmpty());
}


/**
 * Test if a Mapper line symbol can be represented by the framing aspect
 * of an OCD line symbol.
 * 
 * This function helps to merge elements of combined symbols into OCD line symbols.
 */
bool maybeFraming(const LineSymbol* line)
{
	return line
	        && !line->hasBorder()
	        && !line->isDashed()
	        && line->getCapStyle() != LineSymbol::PointedCap
	        && (!line->getDashSymbol() || line->getDashSymbol()->isEmpty())
	        && (!line->getMidSymbol() || line->getMidSymbol()->isEmpty())
	        && (!line->getStartSymbol() || line->getStartSymbol()->isEmpty())
	        && (!line->getEndSymbol() || line->getEndSymbol()->isEmpty());
}


/**
 * Test if a Mapper line symbol can be represented by the main line aspect
 * of an OCD line symbol.
 * 
 * This function helps to merge elements of combined symbols into OCD line symbols.
 */
bool maybeMainLine(const LineSymbol* line)
{
	return line
	        && !line->hasBorder();
}



/**
 * Efficiently convert a single coordinate value to the OCD format.
 * 
 * This function handles two responsibilites at the same time,
 * in a constexpr implementation:
 * 
 * - convert from 1/100 mm to 1/10 mm, rounding half up (for intervalls of equal size),
 * - shift by 8 bits (which are reserved for flags in OCD format).
 *
 * Neither rounding (the result of an integer division) half up 
 * nor shifting of signed integers ("implementation-defined")
 * come out of the box in C++.
 */
constexpr qint32 convertPointMember(qint32 value)
{
	return (value < -5) ? qint32(0x80000000u | ((0x7fffffu & quint32((value-4)/10)) << 8)) : qint32((0x7fffffu & quint32((value+5)/10)) << 8);
}

// convertPointMember() shall round half up.
Q_STATIC_ASSERT(convertPointMember(-16) == qint32(0xfffffe00u)); // __ down __
Q_STATIC_ASSERT(convertPointMember(-15) == qint32(0xffffff00u)); //     up
Q_STATIC_ASSERT(convertPointMember( -6) == qint32(0xffffff00u)); // __ down __
Q_STATIC_ASSERT(convertPointMember( -5) == qint32(0x00000000u)); //     up
Q_STATIC_ASSERT(convertPointMember( -1) == qint32(0x00000000u)); //     up
Q_STATIC_ASSERT(convertPointMember(  0) == qint32(0x00000000u)); //  unchanged
Q_STATIC_ASSERT(convertPointMember( +1) == qint32(0x00000000u)); //    down
Q_STATIC_ASSERT(convertPointMember( +4) == qint32(0x00000000u)); // __ down __
Q_STATIC_ASSERT(convertPointMember( +5) == qint32(0x00000100u)); //     up
Q_STATIC_ASSERT(convertPointMember(+14) == qint32(0x00000100u)); // __ down __
Q_STATIC_ASSERT(convertPointMember(+15) == qint32(0x00000200u)); //     up


/**
 * Convert a pair of coordinates to a point in OCD format.
 * 
 * \see convertPointMember()
 */
Ocd::OcdPoint32 convertPoint(qint32 x, qint32 y)
{
	return { convertPointMember(x), convertPointMember(-y) };
}


/**
 * Convert a MapCoord's coordinate values to a point in OCD format.
 * 
 * This function does not deal with flags.
 * 
 * \see convertPointMember()
 */
Ocd::OcdPoint32 convertPoint(const MapCoord& coord)
{
	return convertPoint(coord.nativeX(), coord.nativeY());
}


/**
 * Convert a size to the OCD format.
 * 
 * This function converts from 1/100 mm to 1/10 mm, rounding half up for positive values.
 */
constexpr qint16 convertSize(qint32 size)
{
	return qint16((size+5) / 10);
}

/**
 * Convert a size to the OCD format.
 * 
 * This function converts from 1/100 mm to 1/10 mm, rounding half up for positive values.
 */
constexpr qint32 convertSize(qint64 size)
{
	return qint32((size+5) / 10);
}


/**
 * Convert an angle to the OCD format.
 * 
 * This function converts from radians to 1/10 degrees, rounding to the nearest integer.
 */
int convertRotation(qreal angle)
{
	return qRound(10 * qRadiansToDegrees(angle));
}


int getPaletteColorV6(QRgb rgb)
{
	Q_ASSERT(qAlpha(rgb) == 255);
	
	// Quickly return for most frequent value
	if (rgb == qRgb(255, 255, 255))
		return 15;
	
	auto color = QColor(rgb).toHsv();
	if (color.hue() == -1 || color.saturation() < 32)
	{
		auto gray = qGray(rgb);  // qGray is used for dithering
		if (gray >= 192)
			return 8;
		if (gray >= 128)
			return 7;
		return 0;
	}
	
	struct PaletteColor
	{
		int hue;
		int saturation;
		int value;
	};
	static const PaletteColor palette[16] = {
	    {  -1,   0,   0 },
	    {   0, 255, 128 },
	    { 120, 255, 128 },
	    {  60, 255, 128 },
	    { 240, 255, 128 },
	    { 300, 255, 128 },
	    { 180, 255, 128 },
	    {  -1,   0, 128 },
	    {  -1,   0, 192 },
	    {   0, 255, 255 },
	    { 120, 255, 255 },
	    {  60, 255, 255 },
	    { 240, 255, 255 },
	    { 300, 255, 255 },
	    { 180, 255, 255 },
	    {  -1,   0, 255 },
	};
	
#if 0
	// This is how `palette` is generated.
	static auto generate = true;
	if (generate)
	{
		static const QColor original_palette[16] = {
			QColor(  0,   0,   0).toHsv(),
			QColor(128,   0,   0).toHsv(),
			QColor(0,   128,   0).toHsv(),
			QColor(128, 128,   0).toHsv(),
			QColor(  0,   0, 128).toHsv(),
			QColor(128,   0, 128).toHsv(),
			QColor(  0, 128, 128).toHsv(),
			QColor(128, 128, 128).toHsv(),
			QColor(192, 192, 192).toHsv(),
			QColor(255,   0,   0).toHsv(),
			QColor(  0, 255,   0).toHsv(),
			QColor(255, 255,   0).toHsv(),
			QColor(  0,   0, 255).toHsv(),
			QColor(255,   0, 255).toHsv(),
			QColor(  0, 255, 255).toHsv(),
			QColor(255, 255, 255).toHsv()
		};
		
		for (auto& c : original_palette)
		{
			qDebug("		{ %3d, %3d, %3d },", c.hue(), c.saturation(), c.value());
		}
		generate = false;
	}
#endif
	
	int best_index = 0;
	auto best_distance = 2100000;  // > 6 * (10*sq(180) + sq(128) + sq(64))
	auto sq = [](int n) { return n*n; };
	for (auto i : { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14 })
	{
		// True color
		const auto& palette_color = palette[i];
		auto hue_dist = std::abs(color.hue() - palette_color.hue);
		auto distance = 10 * sq(std::min(hue_dist, 360 - hue_dist))
		                + sq(color.saturation() - palette_color.saturation)
		                + sq(color.value() - palette_color.value);
		
		// (Too much) manual tweaking for orienteering colors
		if (i == 1)
			distance *= 3;	// Dark red
		else if (i == 3)
			distance *= 4;		// Olive
		else if (i == 11)
			distance *= 4;		// Yellow
		else if (i == 9)
			distance *= 6;		// Red is unlikely
		else
			distance *= 2;
		
		if (distance < best_distance)
		{
			best_distance = distance;
			best_index = i;
		}
	}
	return best_index;
}


quint8 getPaletteColorV9(QRgb rgb)
{
	Q_ASSERT(qAlpha(rgb) == 255);
	
	// Quickly return most frequent value
	if (rgb == qRgb(255, 255, 255))
		return 124;
	
	struct PaletteColor
	{
		int r;
		int g;
		int b;
	};
	static const PaletteColor palette[125] = {
	    {   0,   0,   0 },
	    {   0,   0,  64 },
	    {   0,   0, 128 },
	    {   0,   0, 192 },
	    {   0,   0, 255 },
	    {   0,  64,   0 },
	    {   0,  64,  64 },
	    {   0,  64, 128 },
	    {   0,  64, 192 },
	    {   0,  64, 255 },
	    {   0, 128,   0 },
	    {   0, 128,  64 },
	    {   0, 128, 128 },
	    {   0, 128, 192 },
	    {   0, 128, 255 },
	    {   0, 192,   0 },
	    {   0, 192,  64 },
	    {   0, 192, 128 },
	    {   0, 192, 192 },
	    {   0, 192, 255 },
	    {   0, 255,   0 },
	    {   0, 255,  64 },
	    {   0, 255, 128 },
	    {   0, 255, 192 },
	    {   0, 255, 255 },
	    {  64,   0,   0 },
	    {  64,   0,  64 },
	    {  64,   0, 128 },
	    {  64,   0, 192 },
	    {  64,   0, 255 },
	    {  64,  64,   0 },
	    {  64,  64,  64 },
	    {  64,  64, 128 },
	    {  64,  64, 192 },
	    {  64,  64, 255 },
	    {  64, 128,   0 },
	    {  64, 128,  64 },
	    {  64, 128, 128 },
	    {  64, 128, 192 },
	    {  64, 128, 255 },
	    {  64, 192,   0 },
	    {  64, 192,  64 },
	    {  64, 192, 128 },
	    {  64, 192, 192 },
	    {  64, 192, 255 },
	    {  64, 255,   0 },
	    {  64, 255,  64 },
	    {  64, 255, 128 },
	    {  64, 255, 192 },
	    {  64, 255, 255 },
	    { 128,   0,   0 },
	    { 128,   0,  64 },
	    { 128,   0, 128 },
	    { 128,   0, 192 },
	    { 128,   0, 255 },
	    { 128,  64,   0 },
	    { 128,  64,  64 },
	    { 128,  64, 128 },
	    { 128,  64, 192 },
	    { 128,  64, 255 },
	    { 128, 128,   0 },
	    { 128, 128,  64 },
	    { 128, 128, 128 },
	    { 128, 128, 192 },
	    { 128, 128, 255 },
	    { 128, 192,   0 },
	    { 128, 192,  64 },
	    { 128, 192, 128 },
	    { 128, 192, 192 },
	    { 128, 192, 255 },
	    { 128, 255,   0 },
	    { 128, 255,  64 },
	    { 128, 255, 128 },
	    { 128, 255, 192 },
	    { 128, 255, 255 },
	    { 192,   0,   0 },
	    { 192,   0,  64 },
	    { 192,   0, 128 },
	    { 192,   0, 192 },
	    { 192,   0, 255 },
	    { 192,  64,   0 },
	    { 192,  64,  64 },
	    { 192,  64, 128 },
	    { 192,  64, 192 },
	    { 192,  64, 255 },
	    { 192, 128,   0 },
	    { 192, 128,  64 },
	    { 192, 128, 128 },
	    { 192, 128, 192 },
	    { 192, 128, 255 },
	    { 192, 192,   0 },
	    { 192, 192,  64 },
	    { 192, 192, 128 },
	    { 192, 192, 192 },
	    { 192, 192, 255 },
	    { 192, 255,   0 },
	    { 192, 255,  64 },
	    { 192, 255, 128 },
	    { 192, 255, 192 },
	    { 192, 255, 255 },
	    { 255,   0,   0 },
	    { 255,   0,  64 },
	    { 255,   0, 128 },
	    { 255,   0, 192 },
	    { 255,   0, 255 },
	    { 255,  64,   0 },
	    { 255,  64,  64 },
	    { 255,  64, 128 },
	    { 255,  64, 192 },
	    { 255,  64, 255 },
	    { 255, 128,   0 },
	    { 255, 128,  64 },
	    { 255, 128, 128 },
	    { 255, 128, 192 },
	    { 255, 128, 255 },
	    { 255, 192,   0 },
	    { 255, 192,  64 },
	    { 255, 192, 128 },
	    { 255, 192, 192 },
	    { 255, 192, 255 },
	    { 255, 255,   0 },
	    { 255, 255,  64 },
	    { 255, 255, 128 },
	    { 255, 255, 192 },
	    { 255, 255, 255 },
	};
	
#if 0
	// This is how `palette` is generated.
	static auto generate = true;
	if (generate)
	{
		static const int color_levels[5] = { 0x00, 0x40, 0x80, 0xc0, 0xff };
		for (auto r : color_levels)
		{
			for (auto g : color_levels)
			{
				for (auto b : color_levels)
				{
					qDebug("		{ %3d, %3d, %3d },", r, g, b);
				}
			}
		}
		generate = false;
	}
#endif
	
	auto r = qRed(rgb);
	auto g = qGreen(rgb);
	auto b = qBlue(rgb);
	auto sq = [](int n) { return n*n; };

	quint8 best_index = 0;
	auto best_distance = 10000; // > (2 + 3 + 4) * sq(32)
	for (quint8 i = 0; i < 125; ++i)
	{
		auto palette_color = palette[i];
		auto distance = 2 * sq(r - palette_color.r) + 4 * sq(g - palette_color.g) + 3 * sq(b - palette_color.b);
		if (distance < best_distance)
		{
			best_distance = distance;
			best_index = i;
		}
	}
	return best_index;
}



int getPatternSize(const PointSymbol* point)
{
	if (!point)
		return 0;
	
	int count = 0;
	for (int i = 0; i < point->getNumElements(); ++i)
	{
		int factor = 1;
		if (point->getElementSymbol(i)->getType() == Symbol::Point)
		{
			factor = 0;
			const PointSymbol* point_symbol = static_cast<const PointSymbol*>(point->getElementSymbol(i));
			if (point_symbol->getInnerRadius() > 0 && point_symbol->getInnerColor())
				++factor;
			if (point_symbol->getOuterWidth() > 0 && point_symbol->getOuterColor())
				++factor;
		}
		count += factor * int(2 + point->getElementObject(i)->getRawCoordinateVector().size());
	}
	if (point->getInnerRadius() > 0 && point->getInnerColor())
		count += 2 + 1;
	if (point->getOuterWidth() > 0 && point->getOuterColor())
		count += 2 + 1;
	
	return count * int(sizeof(Ocd::OcdPoint32));
}



/// String 9: color
QString stringForColor(int i, const MapColor& color)
{
	const auto& cmyk = color.getCmyk();
	QString string_9;
	QTextStream out(&string_9, QIODevice::Append);
	out << color.getName()
	    << "\tn" << i
	    << "\tc" << qRound(cmyk.c * 100)
	    << "\tm" << qRound(cmyk.m * 100)
	    << "\ty" << qRound(cmyk.y * 100)
	    << "\tk" << qRound(cmyk.k * 100)
	    << "\to" << (color.getKnockout() ? '0' : '1')
	    << "\tt" << qRound(color.getOpacity() * 100);
	if (color.getSpotColorMethod() == MapColor::CustomColor)
	{
		for (const auto& component : color.getComponents())
		{
			out << "\ts" << component.spot_color->getSpotColorName()
			    << "\tp" << qRound(component.factor * 100);
		}
	}
	else if (color.getSpotColorMethod() == MapColor::SpotColor)
	{
		out << "\ts" << color.getSpotColorName()
		    << "\tp" << 100;
	}
	return string_9;
}


/// String 10: spot color
QString stringForSpotColor(int i, const MapColor& color)
{
	const auto& cmyk = color.getCmyk();
	QString string_10;
	QTextStream out(&string_10, QIODevice::Append);
	out << color.getSpotColorName()
	    << "\tn" << i
	    << "\tv1"
	    << fixed << qSetRealNumberPrecision(1)
	    << "\tc" << qRound(cmyk.c * 200)/2.0
	    << "\tm" << qRound(cmyk.m * 200)/2.0
	    << "\ty" << qRound(cmyk.y * 200)/2.0
	    << "\tk" << qRound(cmyk.k * 200)/2.0
	    << "\tf" << 1500.0
	    << "\ta" << 45.0;
	return string_10;
}


/// String 8: background map (aka template)
/// \todo Unify implementation, or use specialization.
QString stringForTemplate(const Template& temp, const MapCoord& area_offset, quint16 version)
{
	
	auto template_path = temp.getTemplatePath();
	template_path.replace(QLatin1Char('/'), QLatin1Char('\\'));
	
	const auto x = (temp.getTemplateX() - area_offset.nativeX()) / 1000.0;
	const auto y = (temp.getTemplateY() - area_offset.nativeY()) / -1000.0;
	const auto ab = qRadiansToDegrees(temp.getTemplateRotation());
	
	QString string_8;
	QTextStream out(&string_8, QIODevice::Append);
	out << template_path
	    << "\ts" << 1;  // visible
	// The order of the following parameters may not matter,
	// but choosing the most frequent form may ease testing.
	if (version >= 11)
	{
		// Parameter 'r' (and 's') changed meaning in version 11
		out << "\tr1"	// visible in background favourites
		    << qSetRealNumberPrecision(10)
		    << "\tu" << temp.getTemplateScaleX()
		    << "\tv" << temp.getTemplateScaleY()
		    << qSetRealNumberPrecision(6)
		    << "\tx" << x
		    << "\ty" << y
		    << qSetRealNumberPrecision(8)
		    << "\ta" << ab
		    << "\tb" << ab
		    // Random order: d [ q t ]
		    << "\td0"
		    ;
	}
	else if (version == 10)
	{
		out << "\tr1"	// visible in background favourites
		    << qSetRealNumberPrecision(6)
		    << "\tx" << x
		    << "\ty" << y
		    << qSetRealNumberPrecision(8)
		    << "\ta" << ab
		    << "\tb" << ab
		    // Data may end here.
		    << qSetRealNumberPrecision(10)
		    << "\tu" << temp.getTemplateScaleX()
		    << "\tv" << temp.getTemplateScaleY()
		    // Data may end here.
		    // optional: t, q, d
		    ;
	}
	else if (version == 9)
	{
		// Parameters 'x'/'y', 'u'/'v' and 'p' changed domain in version 9
		out << qSetRealNumberPrecision(6)
		    << "\tx" << x
		    << "\ty" << y
		    << qSetRealNumberPrecision(8)
		    << "\ta" << ab
		    << qSetRealNumberPrecision(10)
		    << "\tu" << temp.getTemplateScaleX()
		    << "\tv" << temp.getTemplateScaleY()
		    << "\td0"
		    << "\tp"
		    << "\tt0"
		    << "\to0"
		    << qSetRealNumberPrecision(6) // less precision than 'a', indeed!
		    << "\tb" << ab
		    ;
	}
	else
	{
		// Data may end here (i.e. after 's'; spotted for OCD file)
		out << "\tx" << qRound(100 * x)
		    << "\ty" << qRound(100 * y)
		    << qSetRealNumberPrecision(8)
		    << "\ta" << ab
		    << qSetRealNumberPrecision(10)
		    << "\tu" << 100 * temp.getTemplateScaleX()
		    << "\tv" << 100 * temp.getTemplateScaleY()
		    << "\td0"
		    << "\tp-1"
		    << "\tt0"
		    << "\to0"
		    ;
		// Alternative observation: s, x, y, u, v, a
	}
	return string_8;
}


/// String 1030: view
QString stringForViewPar(const MapView& view, const MapCoord& area_offset, quint16 version)
{
	QString string_1030;
	QTextStream out(&string_1030, QIODevice::Append);
	if (version == 8)
	{
		out << "\ts0\tt1";
	}
	else
	{
		const auto center = view.center() - area_offset;
		out << qSetRealNumberPrecision(6)
		    << "\tx" << center.x()
		    << "\ty" << -center.y()
		    << "\tz" << view.getZoom()
		    << "\tv0"
		    << "\tm50"
		    << "\tt50"
		    << "\tb50"
		    << "\tc50"
		    << "\th0"
		    << "\td0";
	}
	return string_1030;
}

} // namespace



quint16 OcdFileExport::default_version = 9;



OcdFileExport::OcdFileExport(const QString& path, const Map* map, const MapView* view, quint16 version)
: Exporter { path, map, view }
, ocd_version { version }
{
#ifdef MAPPER_DEVELOPMENT_BUILD
	if (!ocd_version)
	{
		if (path.endsWith(QLatin1String("test-v8.ocd")))
			ocd_version = 8;
		else if (path.endsWith(QLatin1String("test-v9.ocd")))
			ocd_version = 9;
		else if (path.endsWith(QLatin1String("test-v10.ocd")))
			ocd_version = 10;
		else if (path.endsWith(QLatin1String("test-v11.ocd")))
			ocd_version = 11;
		else if (path.endsWith(QLatin1String("test-v12.ocd")))
			ocd_version = 12;
	}
#endif
	
	if (!ocd_version)
		ocd_version = decltype(ocd_version)(map->property(OcdFileFormat::versionProperty()).toInt());
	
	if (!ocd_version)
		ocd_version = default_version;
	
}


OcdFileExport::~OcdFileExport() = default;



template<class Encoding>
QTextCodec* OcdFileExport::determineEncoding()
{
	return nullptr;
}


template<>
QTextCodec* OcdFileExport::determineEncoding<Ocd::Custom8BitEncoding>()
{
	auto encoding = codecFromSettings();
	if (!encoding)
	{
		addWarning(tr("Encoding '%1' is not available. Check the settings.")
		           .arg(Settings::getInstance().getSetting(Settings::General_Local8BitEncoding).toString()));
		encoding = QTextCodec::codecForLocale();
	}
	return encoding;
}



bool OcdFileExport::exportImplementation()
{
	switch (ocd_version)
	{
	case 8:
		return exportImplementation<Ocd::FormatV8>();
		
	case 9:
	case 10:
		return exportImplementation<Ocd::FormatV9>();
		
	case 11:
		return exportImplementation<Ocd::FormatV11>();
		
	case 12:
		return exportImplementation<Ocd::FormatV12>();
		
	default:
		throw FileFormatException(
		            Exporter::tr("Could not write file: %1").
		            arg(tr("OCD files of version %1 are not supported!").arg(ocd_version))
		            );
	}
}



namespace {

void setupFileHeaderGeneric(quint16 actual_version, Ocd::FileHeaderGeneric& header)
{
	header.version = actual_version;
	header.file_type = Ocd::TypeMap;
	switch (actual_version)
	{
	case 8:
		header.file_type = Ocd::TypeMapV8;
		break;
	case 9:
		header.subversion = 4;
		break;
	case 10:
		header.subversion = 2;
		break;
	case 11:
		header.subversion = 3;
		break;
	default:
		; // nothing
	}
}

} // namespace


template<class Format>
bool OcdFileExport::exportImplementation()
{
	addWarning(QLatin1String("OcdFileExport: WORK IN PROGRESS, FILE INCOMPLETE"));
	
	OcdFile<Format> file;
	
	custom_8bit_encoding = determineEncoding<typename Format::Encoding>();
	if (custom_8bit_encoding)
	{
		addParameterString = [&file, this](qint32 string_type, const QString& string) {
			file.strings().insert(string_type, custom_8bit_encoding->fromUnicode(string));
		};
	}
	else
	{
		addParameterString = [&file](qint32 string_type, const QString& string) {
			file.strings().insert(string_type, string.toUtf8());
		};
	}
	
	// Check for a neccessary offset (and add related warnings early).
	area_offset = calculateAreaOffset();
	uses_registration_color = map->isColorUsedByASymbol(map->getRegistrationColor());
	
	setupFileHeaderGeneric(ocd_version, *file.header());
	exportSetup(file);   // includes colors
	exportSymbols(file);
	exportObjects(file);
	exportTemplates(file);
	exportExtras(file);
	
	auto& data = file.constByteArray();
	return device()->write(data) == data.size();
}


MapCoord OcdFileExport::calculateAreaOffset()
{
	auto area_offset = QPointF{};
	
	// Attention: When changing ocd_bounds, update the warning messages, too.
	auto ocd_bounds = QRectF{QPointF{-2000, -2000}, QPointF{2000, 2000}};
	auto objects_extent = map->calculateExtent();
	if (objects_extent.isValid() && !ocd_bounds.contains(objects_extent))
	{
		if (objects_extent.width() < ocd_bounds.width()
		    && objects_extent.height() < ocd_bounds.height())
		{
			// The extent fits into the limited area.
			addWarning(tr("Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m)."));
			area_offset = objects_extent.center();
		}
		else
		{
			// The extent is too wide to fit.
			
			// Only move the objects if they are completely outside the drawing area.
			// This avoids repeated moves on open/save/close cycles.
			if (!objects_extent.intersects(ocd_bounds))
			{
				addWarning(tr("Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m)."));
				std::size_t count = 0;
				auto calculate_average_center = [&area_offset, &count](const Object* object) {
					area_offset *= qreal(count)/qreal(count+1);
					++count;
					area_offset += object->getExtent().center() / count;
				};
				map->applyOnAllObjects(calculate_average_center);
			}
			
			addWarning(tr("Some coordinates remain outside of the OCAD 8 drawing area."
			              " They might be unreachable in OCAD."));
		}
		
		if (area_offset.manhattanLength() > 0)
		{
			// Round offset to 100 m in projected coordinates, to avoid crude grid offset.
			constexpr auto unit = 100;
			auto projected_offset = map->getGeoreferencing().toProjectedCoords(MapCoordF(area_offset));
			projected_offset.rx() = qreal(qRound(projected_offset.x()/unit)) * unit;
			projected_offset.ry() = qreal(qRound(projected_offset.y()/unit)) * unit;
			area_offset = map->getGeoreferencing().toMapCoordF(projected_offset);
		}
	}
	
	return MapCoord{area_offset};
}



template<>
void OcdFileExport::exportSetup(OcdFile<Ocd::FormatV8>& file)
{
	{
		auto setup = reinterpret_cast<Ocd::SetupV8*>(file.byteArray().data() + file.header()->setup_pos);
		
		const auto& georef = map->getGeoreferencing();
		auto add_warning = [this](const QString& w){ addWarning(w); };
		auto fields = OcdGeorefFields::fromGeoref(georef, add_warning);
		setup->map_scale = fields.m;
		setup->real_offset_x = fields.x;
		setup->real_offset_y = fields.y;
		setup->real_angle = fields.a;
		if (fields.i)
			addWarning(tr("The georeferencing cannot be saved in OCD version 8."));
		
		if (view)
		{
			setup->center = convertPoint(view->center() - area_offset);
			setup->zoom = view->getZoom();
		}
		else
		{
			setup->zoom = 1;
		}
	}
	
	{	
		auto notes = custom_8bit_encoding->fromUnicode(map->getMapNotes());
		if (!notes.isEmpty())
		{
			auto size = notes.size() + 1;
			if (size > 32768)
			{
				/// \todo addWarning(...)
				size = 32768;
				notes.truncate(23767);
			}
			file.header()->info_pos = quint32(file.byteArray().size());
			file.header()->info_size = quint32(size);
			file.byteArray().append(notes).append('\0');
		}
	}
		
	{
		auto& symbol_header = file.header()->symbol_header;
		
		auto num_colors = map->getNumColors();
		
		auto spotColorNumber = [this, num_colors](const MapColor* color)->quint16 {
			quint16 number = 0;
			for (auto i = 0; i < num_colors; ++i)
			{
				const auto current = map->getColor(i);
				if (current == color)
					break;
				if (current->getSpotColorMethod() == MapColor::SpotColor)
					++number;
			}
			return number;
		};
		
		symbol_header.num_separations = spotColorNumber(nullptr);
		if (symbol_header.num_separations > 24)
			throw FileFormatException(tr("The map contains more than 24 spot colors which is not supported by OCD version 8."));
		
		auto begin_of_spot_colors = beginOfSpotColors(map);
		if (uses_registration_color)
			++begin_of_spot_colors;  // in ocd output (ocd_number below)
		if (begin_of_spot_colors > 256)
			throw FileFormatException(tr("The map contains more than 256 colors which is not supported by OCD version 8."));
		
		using std::begin; using std::end;
		auto separation_info = symbol_header.separation_info;
		auto color_info = symbol_header.color_info;
		quint16 ocd_number = 0;
		
		if (uses_registration_color)
		{
			color_info->number = 0;
			color_info->name = toOcdString(Map::getRegistrationColor()->getName());
			color_info->cmyk = { 200, 200, 200, 200 };  // OC*D stores CMYK values as integers from 0-200.
			std::fill(begin(color_info->separations), begin(color_info->separations) + symbol_header.num_separations, 200);
			std::fill(begin(color_info->separations) + symbol_header.num_separations, end(color_info->separations), 255);
			++color_info;
			++ocd_number;
		}
		
		for (int i = 0; i < num_colors; ++i)
		{
			const auto color = map->getColor(i);
			const auto& cmyk = color->getCmyk();
			// OC*D stores CMYK values as integers from 0-200.
			auto ocd_cmyk = Ocd::CmykV8 {
			                  quint8(qRound(200 * cmyk.c)),
			                  quint8(qRound(200 * cmyk.m)),
			                  quint8(qRound(200 * cmyk.y)),
			                  quint8(qRound(200 * cmyk.k))
			};
			
			if (color->getSpotColorMethod() == MapColor::SpotColor)
			{
				separation_info->name = toOcdString(color->getSpotColorName());
				separation_info->cmyk = ocd_cmyk;
				separation_info->raster_freq = 1500;  /// \todo Spot color raster frequency
				separation_info->raster_angle = 450;  /// \todo Spot color raster angle
				++separation_info;
				
				if (ocd_number >= begin_of_spot_colors)
					continue;  // Just a spot color, not a regular map color.
				
				std::fill(begin(color_info->separations), end(color_info->separations), 255);
				auto index = spotColorNumber(color);
				if (index >= symbol_header.num_separations)
					throw FileFormatException(tr("Invalid spot color."));
				color_info->separations[index] = 200;
			}
			else
			{
				std::fill(begin(color_info->separations), end(color_info->separations), 255);
				if (color->getSpotColorMethod() == MapColor::CustomColor)
				{
					for (const auto& component : color->getComponents())
					{
						auto index = spotColorNumber(component.spot_color);
						if (index >= symbol_header.num_separations)
							throw FileFormatException(tr("Invalid spot color."));
						color_info->separations[index] = quint8(qRound(component.factor * 200));
					}
				}
			}
			
			color_info->number = ocd_number;
			color_info->name = toOcdString(color->getName());
			color_info->cmyk = ocd_cmyk;
			++color_info;
			++ocd_number;
		}
		symbol_header.num_colors = ocd_number;
	}
}


template<class Format>
void OcdFileExport::exportSetup(OcdFile<Format>& /*file*/)
{
	exportGeoreferencing();
	exportSetup();
}


void OcdFileExport::exportGeoreferencing()
{
	const auto& georef = map->getGeoreferencing();
	auto add_warning = [this](const QString& w){ addWarning(w); };
	auto fields = OcdGeorefFields::fromGeoref(georef, add_warning);
	
	auto& grid = map->getGrid();
	auto grid_spacing_real = 500.0;
	auto grid_spacing_map  = 50.0;
	auto spacing = std::min(grid.getHorizontalSpacing(), grid.getVerticalSpacing());
	switch (grid.getUnit())
	{
	case MapGrid::MillimetersOnMap:
		grid_spacing_map = spacing;
		grid_spacing_real = spacing * georef.getScaleDenominator()  / 1000;
		break;
	case MapGrid::MetersInTerrain:
		grid_spacing_map = spacing * 1000 / georef.getScaleDenominator();
		grid_spacing_real = spacing;
		break;
	}
	
	QString string_1039;
	QTextStream out(&string_1039, QIODevice::Append);
	out << fixed
	    << "\tm" << fields.m
	    << qSetRealNumberPrecision(4)
	    << "\tg" << grid_spacing_map
	    << "\tr" << fields.r
	    << "\tx" << fields.x
	    << "\ty" << fields.y
	    << qSetRealNumberPrecision(8)
	    << "\ta" << fields.a
	    << qSetRealNumberPrecision(6)
	    << "\td" << grid_spacing_real
	    << "\ti" << fields.i;
	if (ocd_version > 9)
	{
		out << qSetRealNumberPrecision(2)
		    << "\tb" << 0.0
		    << "\tc" << 0.0;
	}
	
	addParameterString(1039, string_1039);
}


void OcdFileExport::exportSetup()
{
	// View
	if (view)
	{
		addParameterString(1030, stringForViewPar(*view, area_offset, ocd_version));
	}
	
	// Map notes
	if (ocd_version >= 9 && !map->getMapNotes().isEmpty())
	{
		auto param = 1061;
		auto notes = map->getMapNotes();
		if (ocd_version <= 10 && !notes.isEmpty())
		{
			param = 11;
			if (!notes.endsWith(QLatin1Char('\n')))
				notes.append(QLatin1Char('\n'));
		}
		notes.replace(QLatin1String("\n"), QLatin1String("\r\n"));
		notes.replace(QLatin1String("\r\r"), QLatin1String("\r"));  // just in case
		addParameterString(param, notes);
	}
	
	// Map colors
	auto num_colors = map->getNumColors();
	auto begin_of_spot_colors = beginOfSpotColors(map);
	auto ocd_number = 0;
	auto spot_number = 0;
	if (uses_registration_color)
	{
		SpotColorComponents all_spot_colors;
		for (int i = 0; i < num_colors; ++i)
		{
			const auto color = map->getColor(i);
			if (color->getSpotColorMethod() == MapColor::SpotColor)
			{
				all_spot_colors.push_back({color, 1});
			}
		}
		MapColorCmyk all_cmyk = { 1, 1, 1, 1 };
		MapColor registration_color{*Map::getRegistrationColor()};
		registration_color.setSpotColorComposition(all_spot_colors);
		registration_color.setCmyk(all_cmyk);
		addParameterString(9, stringForColor(ocd_number++, registration_color));
	}
	
	for (int i = 0; i < num_colors; ++i)
	{
		const auto color = map->getColor(i);
		if (color->getSpotColorMethod() == MapColor::SpotColor)
		{
			addParameterString(10, stringForSpotColor(spot_number++, *color));
			if (ocd_number >= begin_of_spot_colors)
				continue;  // Just a spot color, not a regular map color.
		}
		addParameterString(9, stringForColor(ocd_number++, *color));
	}
}



template<class Format>
void OcdFileExport::exportSymbols(OcdFile<Format>& file)
{
	symbol_numbers.clear();
	text_format_mapping.clear();
	breakdown_list.clear();
	const auto num_symbols = map->getNumSymbols();
	
	// First pass: Collect unique symbol numbers (i.e. skip duplicates)
	for (int i = 0; i < num_symbols; ++i)
	{
		const auto symbol = map->getSymbol(i);
		auto number = makeSymbolNumber(symbol, Format::BaseSymbol::symbol_number_factor);
		auto matches_symbol_number = [number](const auto& entry) { return number == entry.second; };
		if (!std::any_of(begin(symbol_numbers), end(symbol_numbers), matches_symbol_number))
			symbol_numbers[symbol] = number;
	}
	
	// Second pass: Turn duplicate symbol numbers into unique numbers
	for (int i = 0; i < num_symbols; ++i)
	{
		const auto symbol = map->getSymbol(i);
		if (symbol_numbers.find(symbol) != end(symbol_numbers))
			continue;
			
		auto number = makeSymbolNumber(symbol, Format::BaseSymbol::symbol_number_factor);
		symbol_numbers[symbol] = makeUniqueSymbolNumber(number);
	}
	
	// Third pass: Actual export
	for (int i = 0; i < num_symbols; ++i)
	{
		QByteArray ocd_symbol;
		
		const auto symbol = map->getSymbol(i);
		switch(symbol->getType())
		{
		case Symbol::Point:
			ocd_symbol = exportPointSymbol<typename Format::PointSymbol>(static_cast<const PointSymbol*>(symbol));
			break;
		
		case Symbol::Area:
			ocd_symbol = exportAreaSymbol<typename Format::AreaSymbol>(static_cast<const AreaSymbol*>(symbol));
			break;
			
		case Symbol::Line:
			ocd_symbol = exportLineSymbol<typename Format::LineSymbol>(static_cast<const LineSymbol*>(symbol));
			break;
			
		case Symbol::Text:
			exportTextSymbol<Format, typename Format::TextSymbol>(file, static_cast<const TextSymbol*>(symbol));
			continue;  // already added to file
			
		case Symbol::Combined:
			exportCombinedSymbol<Format>(file, static_cast<const CombinedSymbol*>(symbol));
			continue;  // already added to file
			
		case Symbol::NoSymbol:
		case Symbol::AllSymbols:
			Q_UNREACHABLE();
		}
		
		Q_ASSERT(!ocd_symbol.isEmpty());
		file.symbols().insert(ocd_symbol);
	}
}


template< class OcdBaseSymbol >
void OcdFileExport::setupBaseSymbol(const Symbol* symbol, quint32 symbol_number, OcdBaseSymbol& ocd_base_symbol)
{
	ocd_base_symbol = {};
	ocd_base_symbol.description = toOcdString(symbol->getPlainTextName());
	ocd_base_symbol.number = decltype(ocd_base_symbol.number)(symbol_number);
	
	if (symbol->isProtected())
		ocd_base_symbol.status |= Ocd::SymbolProtected;
	if (symbol->isHidden())
		ocd_base_symbol.status |= Ocd::SymbolHidden;

	// Set of used colors
	using ColorBitmask = typename std::remove_extent<typename std::remove_pointer<decltype(ocd_base_symbol.colors)>::type>::type;
	Q_STATIC_ASSERT(std::is_unsigned<ColorBitmask>::value);
	ColorBitmask bitmask = 1;
	
	auto bitpos = std::begin(ocd_base_symbol.colors);
	auto last = std::end(ocd_base_symbol.colors);
	if (uses_registration_color && symbol->containsColor(map->getRegistrationColor()))
	{
		*bitpos |= bitmask;
		bitmask *= 2;
	}
	for (int c = 0; c < map->getNumColors(); ++c)
	{
		if (symbol->containsColor(map->getColor(c)))
			*bitpos |= bitmask;
		
		bitmask *= 2;
		if (bitmask == 0) 
		{
			bitmask = 1;
			++bitpos;
			if (++bitpos == last)
				break;
		}
	}
	
	/// \todo Switch to explicit types for icon data
	switch (std::extent<typename std::remove_pointer<decltype(ocd_base_symbol.icon_bits)>::type>::value)
	{
	case 264:
		exportSymbolIconV6(symbol, ocd_base_symbol.icon_bits);
		break;
		
	case 484:
		exportSymbolIconV9(symbol, ocd_base_symbol.icon_bits);
		break;
	}
}


template< class OcdPointSymbol >
QByteArray OcdFileExport::exportPointSymbol(const PointSymbol* point_symbol)
{
	auto symbol_number = symbol_numbers.at(point_symbol);
	
	OcdPointSymbol ocd_symbol = {};
	setupBaseSymbol<typename OcdPointSymbol::BaseSymbol>(point_symbol, symbol_number, ocd_symbol.base);
	ocd_symbol.base.type = Ocd::SymbolTypePoint;
	ocd_symbol.base.extent = decltype(ocd_symbol.base.extent)(getPointSymbolExtent(point_symbol));
	if (ocd_symbol.base.extent <= 0)
		ocd_symbol.base.extent = 100;
	if (point_symbol->isRotatable())
		ocd_symbol.base.flags |= 1;
	
	auto pattern_size = getPatternSize(point_symbol);
	auto header_size = int(sizeof(OcdPointSymbol) - sizeof(typename OcdPointSymbol::Element));
	ocd_symbol.base.size = decltype(ocd_symbol.base.size)(header_size + pattern_size);
	ocd_symbol.data_size = decltype(ocd_symbol.data_size)(pattern_size / 8);
	
	QByteArray data;
	data.reserve(header_size + pattern_size);
	data.append(reinterpret_cast<const char*>(&ocd_symbol), header_size);
	exportPattern<typename OcdPointSymbol::Element>(point_symbol, data);
	Q_ASSERT(data.size() == header_size + pattern_size);
	
	return data;
}


template< class Element >
qint16 OcdFileExport::exportPattern(const PointSymbol* point, QByteArray& byte_array)
{
	if (!point)
		return 0;
	
	auto num_coords = exportSubPattern<Element>({ {} }, point, byte_array);
	for (int i = 0; i < point->getNumElements(); ++i)
	{
		num_coords += exportSubPattern<Element>(point->getElementObject(i)->getRawCoordinateVector(), point->getElementSymbol(i), byte_array);
	}
	return num_coords;
}


template< class Element >
qint16 OcdFileExport::exportSubPattern(const MapCoordVector& coords, const Symbol* symbol, QByteArray& byte_array)
{
	auto makeElement = [](QByteArray& byte_array)->Element& {
		auto pos = byte_array.size();
		const auto proto_element = Element {};
		byte_array.append(reinterpret_cast<const char *>(&proto_element), sizeof(proto_element));
		return *reinterpret_cast<Element*>(byte_array.data() + pos);
	};
	
	qint16 num_coords = 0;
	
	switch (symbol->getType())
	{
	case Symbol::Point:
		{
			auto point_symbol = static_cast<const PointSymbol*>(symbol);
			if (point_symbol->getInnerRadius() > 0 && point_symbol->getInnerColor())
			{
				auto& element = makeElement(byte_array);
				element.type = Element::TypeDot;
				element.color = convertColor(point_symbol->getInnerColor());
				element.diameter = convertSize(2 * point_symbol->getInnerRadius());
				element.num_coords = exportCoordinates(coords, symbol, byte_array);
				num_coords += 2 + element.num_coords;
			}
			if (point_symbol->getOuterWidth() > 0 && point_symbol->getOuterColor())
			{
				auto& element = makeElement(byte_array);
				element.type = Element::TypeCircle;
				element.color = convertColor(point_symbol->getOuterColor());
				element.line_width = convertSize(point_symbol->getOuterWidth());
				if (ocd_version <= 8)
					element.diameter = convertSize(2 * point_symbol->getInnerRadius() + 2 * point_symbol->getOuterWidth());
				else
					element.diameter = convertSize(2 * point_symbol->getInnerRadius() + point_symbol->getOuterWidth());
				element.num_coords = exportCoordinates(coords, symbol, byte_array);
				num_coords += 2 + element.num_coords;
			}
		}
		break;
		
	case Symbol::Line:
		{
			const LineSymbol* line_symbol = static_cast<const LineSymbol*>(symbol);
			auto& element = makeElement(byte_array);
			element.type = Element::TypeLine;
			if (line_symbol->getCapStyle() == LineSymbol::RoundCap)
				element.flags |= 1;
			else if (line_symbol->getJoinStyle() == LineSymbol::MiterJoin)
				element.flags |= 4;
			element.color = convertColor(line_symbol->getColor());
			element.line_width = convertSize(line_symbol->getLineWidth());
			element.num_coords = exportCoordinates(coords, symbol, byte_array);
			num_coords += 2 + element.num_coords;
		}
		break;
		
	case Symbol::Area:
		{
			const AreaSymbol* area_symbol = static_cast<const AreaSymbol*>(symbol);
			auto& element = makeElement(byte_array);
			element.type = Element::TypeArea;
			element.color = convertColor(area_symbol->getColor());
			element.num_coords = exportCoordinates(coords, symbol, byte_array);
			num_coords += 2 + element.num_coords;
		}
		break;
		
	case Symbol::NoSymbol:
	case Symbol::AllSymbols:
	case Symbol::Combined:
	case Symbol::Text:
		Q_UNREACHABLE();
	}
	
	return num_coords;
}


template< class OcdAreaSymbol >
QByteArray OcdFileExport::exportAreaSymbol(const AreaSymbol* area_symbol)
{
	return exportAreaSymbol<OcdAreaSymbol>(area_symbol, symbol_numbers[area_symbol]);
}


template< class OcdAreaSymbol >
QByteArray OcdFileExport::exportAreaSymbol(const AreaSymbol* area_symbol, quint32 symbol_number)
{
	const PointSymbol* pattern_symbol = nullptr;
	
	OcdAreaSymbol ocd_symbol = {};
	setupBaseSymbol<typename OcdAreaSymbol::BaseSymbol>(area_symbol, symbol_number, ocd_symbol.base);
	ocd_symbol.base.type = Ocd::SymbolTypeArea;
	ocd_symbol.base.flags = exportAreaSymbolCommon(area_symbol, ocd_symbol.common, pattern_symbol);
	exportAreaSymbolSpecial<OcdAreaSymbol>(area_symbol, ocd_symbol);
	
	auto pattern_size = getPatternSize(pattern_symbol);
	auto header_size = int(sizeof(OcdAreaSymbol) - sizeof(typename OcdAreaSymbol::Element));
	ocd_symbol.base.size = decltype(ocd_symbol.base.size)(header_size + pattern_size);
	ocd_symbol.data_size = decltype(ocd_symbol.data_size)(pattern_size / 8);
	
	QByteArray data;
	data.reserve(header_size + pattern_size);
	data.append(reinterpret_cast<const char*>(&ocd_symbol), header_size);
	exportPattern<typename OcdAreaSymbol::Element>(pattern_symbol, data);
	Q_ASSERT(data.size() == header_size + pattern_size);
	
	return data;
}


template< class OcdAreaSymbolCommon >
quint8 OcdFileExport::exportAreaSymbolCommon(const AreaSymbol* area_symbol, OcdAreaSymbolCommon& ocd_area_common, const PointSymbol*& pattern_symbol)
{
	if (area_symbol->getColor())
	{
		ocd_area_common.fill_on_V9 = 1;
		ocd_area_common.fill_color = convertColor(area_symbol->getColor());
	}
	
	quint8 flags = 0;
	// Hatch
	// ocd_area_common.hatch_mode = Ocd::HatchNone; // 0
	for (int i = 0, end = area_symbol->getNumFillPatterns(); i < end; ++i)
	{
		const auto& pattern = area_symbol->getFillPattern(i);
		switch (pattern.type)
		{
		case AreaSymbol::FillPattern::LinePattern:
			switch (ocd_area_common.hatch_mode)
			{
			case Ocd::HatchNone:
				ocd_area_common.hatch_mode = Ocd::HatchSingle;
				ocd_area_common.hatch_color = convertColor(pattern.line_color);
				ocd_area_common.hatch_line_width = decltype(ocd_area_common.hatch_line_width)(convertSize(pattern.line_width));
				if (ocd_version <= 8)
					ocd_area_common.hatch_dist = decltype(ocd_area_common.hatch_dist)(convertSize(pattern.line_spacing - pattern.line_width));
				else
					ocd_area_common.hatch_dist = decltype(ocd_area_common.hatch_dist)(convertSize(pattern.line_spacing));
				ocd_area_common.hatch_angle_1 = decltype(ocd_area_common.hatch_angle_1)(convertRotation(pattern.angle));
				if (pattern.rotatable())
					flags |= 1;
				break;
			case Ocd::HatchSingle:
				if (ocd_area_common.hatch_color == convertColor(pattern.line_color))
				{
					ocd_area_common.hatch_mode = Ocd::HatchCross;
					ocd_area_common.hatch_line_width = decltype(ocd_area_common.hatch_line_width)(ocd_area_common.hatch_line_width + convertSize(pattern.line_width)) / 2;
					if (ocd_version <= 8)
						ocd_area_common.hatch_dist = decltype(ocd_area_common.hatch_dist)(ocd_area_common.hatch_dist + convertSize(pattern.line_spacing - pattern.line_width)) / 2;
					else
						ocd_area_common.hatch_dist = decltype(ocd_area_common.hatch_dist)(ocd_area_common.hatch_dist + convertSize(pattern.line_spacing)) / 2;
					ocd_area_common.hatch_angle_2 = decltype(ocd_area_common.hatch_angle_2)(convertRotation(pattern.angle));
					if (pattern.rotatable())
						flags |= 1;
					break;
				}
				// fall through
			default:
				addWarning(tr("In area symbol \"%1\", skipping a fill pattern.").arg(area_symbol->getPlainTextName()));
			}
			break;
			
		case AreaSymbol::FillPattern::PointPattern:
			switch (ocd_area_common.structure_mode)
			{
			case Ocd::StructureNone:
				ocd_area_common.structure_mode = Ocd::StructureAlignedRows;
				ocd_area_common.structure_width = decltype(ocd_area_common.structure_width)(convertSize(pattern.point_distance));
				ocd_area_common.structure_height = decltype(ocd_area_common.structure_height)(convertSize(pattern.line_spacing));
				ocd_area_common.structure_angle = decltype(ocd_area_common.structure_angle)(convertRotation(pattern.angle));
				pattern_symbol = pattern.point;
				if (pattern.rotatable())
					flags |= 1;
				break;
			case Ocd::StructureAlignedRows:
				ocd_area_common.structure_mode = Ocd::StructureShiftedRows;
				// NOTE: This is only a heuristic which works for the
				// orienteering symbol sets, not a general conversion.
				// (Conversion is not generally posssible.)
				// No further checks are done to find out if the conversion
				// is applicable because with these checks. Already a tiny
				// (not noticeable) error in the symbol definition would make
				// it take the wrong choice.
				addWarning(tr("In area symbol \"%1\", assuming a \"shifted rows\" point pattern. This might be correct as well as incorrect.")
				           .arg(area_symbol->getPlainTextName()));
				
				if (pattern.line_offset != 0)
					ocd_area_common.structure_height /= 2;
				else
					ocd_area_common.structure_width /= 2;
				
				break;
			default:
				addWarning(tr("In area symbol \"%1\", skipping a fill pattern.").arg(area_symbol->getPlainTextName()));
			}
		}
	}
	return flags;
}


template< >
void OcdFileExport::exportAreaSymbolSpecial<Ocd::AreaSymbolV8>(const AreaSymbol* /*area_symbol*/, Ocd::AreaSymbolV8& ocd_area_symbol)
{
	ocd_area_symbol.fill_on = ocd_area_symbol.common.fill_on_V9;
	ocd_area_symbol.common.fill_on_V9 = 0;
}


template< class OcdAreaSymbol >
void OcdFileExport::exportAreaSymbolSpecial(const AreaSymbol* /*area_symbol*/, OcdAreaSymbol& /*ocd_area_symbol*/)
{
	// nothing
}



template< class OcdLineSymbol >
QByteArray OcdFileExport::exportLineSymbol(const LineSymbol* line_symbol)
{
	return  exportLineSymbol<OcdLineSymbol>(line_symbol, symbol_numbers[line_symbol]);
}


template< class OcdLineSymbol >
QByteArray OcdFileExport::exportLineSymbol(const LineSymbol* line_symbol, quint32 symbol_number)
{
	OcdLineSymbol ocd_symbol = {};
	setupBaseSymbol<typename OcdLineSymbol::BaseSymbol>(line_symbol, symbol_number, ocd_symbol.base);
	ocd_symbol.base.type = Ocd::SymbolTypeLine;
	
	auto extent = quint16(convertSize(line_symbol->getLineWidth()/2));
	if (line_symbol->hasBorder())
	{
		const auto& border = line_symbol->getBorder();
		extent += convertSize(std::max(0, border.shift + border.width / 2));
	}
	extent = std::max(extent, getPointSymbolExtent(line_symbol->getStartSymbol()));
	extent = std::max(extent, getPointSymbolExtent(line_symbol->getEndSymbol()));
	extent = std::max(extent, getPointSymbolExtent(line_symbol->getMidSymbol()));
	extent = std::max(extent, getPointSymbolExtent(line_symbol->getDashSymbol()));
	ocd_symbol.base.extent = decltype(ocd_symbol.base.extent)(extent);
	
	auto pattern_size = exportLineSymbolCommon(line_symbol, ocd_symbol.common);
	auto header_size = sizeof(OcdLineSymbol) - sizeof(typename OcdLineSymbol::Element);
	ocd_symbol.base.size = decltype(ocd_symbol.base.size)(header_size + pattern_size);
	if (ocd_version >= 11)
	{
		if (ocd_symbol.common.secondary_data_size)
			ocd_symbol.common.active_symbols_V11 |= 0x08;
		if (ocd_symbol.common.corner_data_size)
			ocd_symbol.common.active_symbols_V11 |= 0x04;
		if (ocd_symbol.common.start_data_size)
			ocd_symbol.common.active_symbols_V11 |= 0x02;
		if (ocd_symbol.common.end_data_size)
			ocd_symbol.common.active_symbols_V11 |= 0x01;
	}
	QByteArray data;
	data.reserve(int(header_size + pattern_size));
	data.append(reinterpret_cast<const char*>(&ocd_symbol), int(header_size));
	exportPattern<typename OcdLineSymbol::Element>(line_symbol->getMidSymbol(), data);
	exportPattern<typename OcdLineSymbol::Element>(line_symbol->getDashSymbol(), data);
	exportPattern<typename OcdLineSymbol::Element>(line_symbol->getStartSymbol(), data);
	exportPattern<typename OcdLineSymbol::Element>(line_symbol->getEndSymbol(), data);
	Q_ASSERT(data.size() == int(header_size + pattern_size));
	
	return data;
}


template< class OcdLineSymbolCommon >
quint32 OcdFileExport::exportLineSymbolCommon(const LineSymbol* line_symbol, OcdLineSymbolCommon& ocd_line_common)
{
	if (line_symbol->getColor())
	{
		ocd_line_common.line_color = convertColor(line_symbol->getColor());
		ocd_line_common.line_width = decltype(ocd_line_common.line_width)(convertSize(line_symbol->getLineWidth()));
	}
	
	// Cap and Join
	if (line_symbol->getCapStyle() == LineSymbol::FlatCap && line_symbol->getJoinStyle() == LineSymbol::BevelJoin)
		ocd_line_common.line_style = 0;
	else if (line_symbol->getCapStyle() == LineSymbol::RoundCap && line_symbol->getJoinStyle() == LineSymbol::RoundJoin)
		ocd_line_common.line_style = 1;
	else if (line_symbol->getCapStyle() == LineSymbol::PointedCap && line_symbol->getJoinStyle() == LineSymbol::BevelJoin)
		ocd_line_common.line_style = 2;
	else if (line_symbol->getCapStyle() == LineSymbol::PointedCap && line_symbol->getJoinStyle() == LineSymbol::RoundJoin)
		ocd_line_common.line_style = 3;
	else if (line_symbol->getCapStyle() == LineSymbol::FlatCap && line_symbol->getJoinStyle() == LineSymbol::MiterJoin)
		ocd_line_common.line_style = 4;
	else if (line_symbol->getCapStyle() == LineSymbol::PointedCap && line_symbol->getJoinStyle() == LineSymbol::MiterJoin)
		ocd_line_common.line_style = 6;
	else
	{
		addWarning(tr("In line symbol \"%1\", cannot represent cap/join combination.").arg(line_symbol->getPlainTextName()));
		// Decide based on the caps
		if (line_symbol->getCapStyle() == LineSymbol::FlatCap)
			ocd_line_common.line_style = 0;
		else if (line_symbol->getCapStyle() == LineSymbol::RoundCap)
			ocd_line_common.line_style = 1;
		else if (line_symbol->getCapStyle() == LineSymbol::PointedCap)
			ocd_line_common.line_style = 3;
		else if (line_symbol->getCapStyle() == LineSymbol::SquareCap)
			ocd_line_common.line_style = 0;
	}
	
	if (line_symbol->getCapStyle() == LineSymbol::PointedCap)
	{
		ocd_line_common.dist_from_start = convertSize(line_symbol->getPointedCapLength());
		ocd_line_common.dist_from_end = convertSize(line_symbol->getPointedCapLength());
	}
	
	// Dash pattern
	if (line_symbol->isDashed())
	{
		if (line_symbol->getMidSymbol() && !line_symbol->getMidSymbol()->isEmpty())
		{
			if (line_symbol->getDashesInGroup() > 1)
				addWarning(tr("In line symbol \"%1\", neglecting the dash grouping.").arg(line_symbol->getPlainTextName()));
			
			ocd_line_common.main_length = convertSize(line_symbol->getDashLength() + line_symbol->getBreakLength());
			ocd_line_common.end_length = ocd_line_common.main_length / 2;
			ocd_line_common.sec_gap = convertSize(line_symbol->getBreakLength());
		}
		else
		{
			if (line_symbol->getDashesInGroup() > 1)
			{
				if (line_symbol->getDashesInGroup() > 2)
					addWarning(tr("In line symbol \"%1\", the number of dashes in a group has been reduced to 2.").arg(line_symbol->getPlainTextName()));
				
				ocd_line_common.main_length = convertSize(2 * line_symbol->getDashLength() + line_symbol->getInGroupBreakLength());
				ocd_line_common.end_length = convertSize(2 * line_symbol->getDashLength() + line_symbol->getInGroupBreakLength());
				ocd_line_common.main_gap = convertSize(line_symbol->getBreakLength());
				ocd_line_common.sec_gap = convertSize(line_symbol->getInGroupBreakLength());
				ocd_line_common.end_gap = ocd_line_common.sec_gap;
			}
			else
			{
				ocd_line_common.main_length = convertSize(line_symbol->getDashLength());
				ocd_line_common.end_length = ocd_line_common.main_length / (line_symbol->getHalfOuterDashes() ? 2 : 1);
				ocd_line_common.main_gap = convertSize(line_symbol->getBreakLength());
			}
		}
	}
	else
	{
		ocd_line_common.main_length = convertSize(line_symbol->getSegmentLength());
		ocd_line_common.end_length = convertSize(line_symbol->getEndLength());
	}
	
	// Double line
	if (line_symbol->hasBorder() && (line_symbol->getBorder().isVisible() || line_symbol->getRightBorder().isVisible()))
	{
		exportLineSymbolDoubleLine(line_symbol, 0, ocd_line_common);
	}
	
	ocd_line_common.min_sym = line_symbol->getShowAtLeastOneSymbol() ? 0 : -1;
	ocd_line_common.num_prim_sym = decltype(ocd_line_common.num_prim_sym)(line_symbol->getMidSymbolsPerSpot());
	ocd_line_common.prim_sym_dist = convertSize(line_symbol->getMidSymbolDistance());
	
	ocd_line_common.primary_data_size = getPatternSize(line_symbol->getMidSymbol()) / 8;
	ocd_line_common.secondary_data_size = 0;
	ocd_line_common.corner_data_size = getPatternSize(line_symbol->getDashSymbol()) / 8;
	ocd_line_common.start_data_size = getPatternSize(line_symbol->getStartSymbol()) / 8;
	ocd_line_common.end_data_size = getPatternSize(line_symbol->getEndSymbol()) / 8;
	
	return 8 * (ocd_line_common.primary_data_size
	            + ocd_line_common.secondary_data_size
	            + ocd_line_common.corner_data_size
	            + ocd_line_common.start_data_size
	            + ocd_line_common.end_data_size);
}


template< class OcdLineSymbolCommon >
void OcdFileExport::exportLineSymbolDoubleLine(const LineSymbol* line_symbol, quint32 fill_color, OcdLineSymbolCommon& ocd_line_common)
{
	ocd_line_common.double_mode = OcdLineSymbolCommon::DoubleLineOff;
	if (!line_symbol->hasBorder())
		return;
	
	if (fill_color)
	{
		ocd_line_common.double_color = decltype(ocd_line_common.double_color)(fill_color);
		ocd_line_common.double_flags |= OcdLineSymbolCommon::DoubleFlagFillColorOn;
	}
	auto filling_dashed = fill_color && line_symbol->isDashed();
	
	LineSymbolBorder left_border = line_symbol->getBorder().isVisible() ? line_symbol->getBorder() : LineSymbolBorder{};
	if (left_border.isVisible() && line_symbol->isDashed() && !left_border.dashed)
	{
		left_border.dashed = true;
		left_border.dash_length = line_symbol->getDashLength();
		left_border.break_length = line_symbol->getBreakLength();
	}
	LineSymbolBorder right_border = line_symbol->getRightBorder().isVisible() ? line_symbol->getRightBorder() : LineSymbolBorder{};
	if (right_border.isVisible() && line_symbol->isDashed() && !right_border.dashed)
	{
		right_border.dashed = true;
		right_border.dash_length = line_symbol->getDashLength();
		right_border.break_length = line_symbol->getBreakLength();
	}
	
	if (left_border.isVisible() && right_border.isVisible())
	{
		auto dash_pattern_differs = left_border.dashed && right_border.dashed
		                            && (left_border.dash_length != right_border.dash_length
		                                || left_border.break_length != right_border.break_length);
		auto filling_pattern_differs = filling_dashed
		                               && (left_border.dashed || right_border.dashed)
		                               && (left_border.dashed != right_border.dashed
		                                   || dash_pattern_differs
		                                   || left_border.dash_length != line_symbol->getDashLength()
		                                   || left_border.break_length != line_symbol->getBreakLength()
		                                   || line_symbol->getHalfOuterDashes()
		                                   || line_symbol->getDashesInGroup() > 1);
		if (dash_pattern_differs
		    || filling_pattern_differs
		    || (!left_border.dashed && right_border.dashed)
		    || left_border.shift != right_border.shift
		    || left_border.width != right_border.width)
		{
			addWarning(tr("In line symbol \"%1\", cannot export the borders correctly.")
			           .arg(line_symbol->getPlainTextName()));
			
			if (filling_pattern_differs)
				filling_dashed = false;
			
			left_border.shift = right_border.shift = (left_border.shift + right_border.shift) / 2;
			// individual width left untouched
			
			if (!left_border.dashed)
				right_border.dashed = false;  // No left border solid, right border dashed in OCD
			
			if (dash_pattern_differs)
			{
				left_border.dash_length = right_border.dash_length = (left_border.dash_length + right_border.dash_length) / 2;
				left_border.break_length = right_border.break_length = (left_border.break_length + right_border.break_length) / 2;
			}
		}
		ocd_line_common.double_width = convertSize(line_symbol->getLineWidth()
		                                           - (left_border.width + right_border.width) / 2
		                                           + left_border.shift + right_border.shift);
	}
	else if (left_border.isVisible())
	{
		ocd_line_common.double_width = convertSize(line_symbol->getLineWidth()
		                                           - left_border.width
		                                           + left_border.shift * 2);
	}
	else if (right_border.isVisible())
	{
		ocd_line_common.double_width = convertSize(line_symbol->getLineWidth()
		                                           - right_border.width
		                                           + right_border.shift * 2);
	}
	else
	{
		ocd_line_common.double_width = convertSize(line_symbol->getLineWidth());
	}
	
	auto export_quirks = false;
	ocd_line_common.double_mode = OcdLineSymbolCommon::DoubleLineContinuous;
	if (left_border.isVisible())
	{
		ocd_line_common.double_left_width = convertSize(left_border.width);
		ocd_line_common.double_left_color = convertColor(left_border.color);
		if (left_border.dashed)
		{
			ocd_line_common.double_mode = OcdLineSymbolCommon::DoubleLineLeftBorderDashed;
			ocd_line_common.double_length = convertSize(left_border.dash_length);
			ocd_line_common.double_gap = convertSize(left_border.break_length);
		}
	}
	if (right_border.isVisible())
	{
		ocd_line_common.double_right_width = convertSize(right_border.width);
		ocd_line_common.double_right_color = convertColor(right_border.color);
		if (right_border.dashed)
		{
			export_quirks |= ocd_line_common.double_mode != OcdLineSymbolCommon::DoubleLineLeftBorderDashed;
			ocd_line_common.double_mode = OcdLineSymbolCommon::DoubleLineBordersDashed;
			ocd_line_common.double_length = convertSize(right_border.dash_length);
			ocd_line_common.double_gap = convertSize(right_border.break_length);
		}
	}
	if (filling_dashed)
	{
		export_quirks |= ocd_line_common.double_mode != OcdLineSymbolCommon::DoubleLineBordersDashed;
		ocd_line_common.double_mode = OcdLineSymbolCommon::DoubleLineAllDashed;
	}
	if (export_quirks)
	{
		qDebug("In line symbol \"%s %s\": Cannot export dashed border correctly.",
		       qUtf8Printable(line_symbol->getNumberAsString()),
		       qUtf8Printable(line_symbol->getDescription()));
	}
}



template< class Format, class OcdTextSymbol >
void OcdFileExport::exportTextSymbol(OcdFile<Format>& file, const TextSymbol* text_symbol)
{
	auto symbol_number = symbol_numbers.at(text_symbol);
	
	text_format_mapping.push_back({ text_symbol, TextObject::AlignLeft, 0, symbol_number });
	text_format_mapping.push_back({ text_symbol, TextObject::AlignHCenter, 0, symbol_number });
	text_format_mapping.push_back({ text_symbol, TextObject::AlignRight, 0, symbol_number });
	auto text_format = text_format_mapping.end() - 3;
	auto count = [text_format](const auto* object) {
		auto alignment = static_cast<const TextObject*>(object)->getHorizontalAlignment();
		switch (alignment)
		{
		case TextObject::AlignLeft:
			text_format->count++;
			break;
		case TextObject::AlignHCenter:
			(text_format+1)->count++;
			break;
		case TextObject::AlignRight:
			(text_format+2)->count++;
			break;
		}
	};
	map->applyOnMatchingObjects(count, ObjectOp::HasSymbol{text_symbol});
	
	// The most frequent usage is to get the regular number.
	std::sort(text_format, end(text_format_mapping), [](const auto& a, const auto& b) { return a.count > b.count; });
	
	text_format = text_format_mapping.end() - 3;
	if (text_format->count == 0)
		text_format->alignment = TextObject::AlignHCenter;  // default if unused: centered
	auto ocd_symbol = exportTextSymbol<typename Format::TextSymbol>(text_symbol, text_format->symbol_number, text_format->alignment);
	Q_ASSERT(!ocd_symbol.isEmpty());
	file.symbols().insert(ocd_symbol);
	
	++text_format;
	if (text_format->count > 0)
	{
		text_format->symbol_number = makeUniqueSymbolNumber(symbol_number);
		number_owners.emplace_back(new PointSymbol());
		symbol_numbers[number_owners.back().get()] = text_format->symbol_number;
		ocd_symbol = exportTextSymbol<typename Format::TextSymbol>(text_symbol, text_format->symbol_number, text_format->alignment);
		Q_ASSERT(!ocd_symbol.isEmpty());
		file.symbols().insert(ocd_symbol);
		
		++text_format;
		if (text_format->count > 0)
		{
			text_format->symbol_number = makeUniqueSymbolNumber(symbol_number);
			number_owners.emplace_back(new PointSymbol());
			symbol_numbers[number_owners.back().get()] = text_format->symbol_number;
			ocd_symbol = exportTextSymbol<typename Format::TextSymbol>(text_symbol, text_format->symbol_number, text_format->alignment);
			Q_ASSERT(!ocd_symbol.isEmpty());
			file.symbols().insert(ocd_symbol);
			
			++text_format;
		}
	}
	text_format_mapping.erase(text_format, end(text_format_mapping));
}


template< class OcdTextSymbol >
QByteArray OcdFileExport::exportTextSymbol(const TextSymbol* text_symbol, quint32 symbol_number, int alignment)
{
	OcdTextSymbol ocd_symbol = {};
	setupBaseSymbol<typename OcdTextSymbol::BaseSymbol>(text_symbol, symbol_number, ocd_symbol.base);
	ocd_symbol.base.type = Ocd::SymbolTypeText;
	
	ocd_symbol.font_name = toOcdString(text_symbol->getFontFamily());
	setupTextSymbolExtra(text_symbol, ocd_symbol);
	setupTextSymbolBasic(text_symbol, alignment, ocd_symbol.basic);
	setupTextSymbolSpecial(text_symbol, ocd_symbol.special);
	
	auto header_size = int(sizeof(OcdTextSymbol));
	ocd_symbol.base.size = decltype(ocd_symbol.base.size)(header_size);
	
	QByteArray data;
	data.reserve(header_size);
	data.append(reinterpret_cast<const char*>(&ocd_symbol), header_size);
	Q_ASSERT(data.size() == header_size);
	
	return data;
}


template< >
void OcdFileExport::setupTextSymbolExtra<Ocd::TextSymbolV8>(const TextSymbol* /*text_symbol*/, Ocd::TextSymbolV8& ocd_text_symbol)
{
	ocd_text_symbol.base.type2 = 1;
}


template< class OcdTextSymbol >
void OcdFileExport::setupTextSymbolExtra(const TextSymbol* /*text_symbol*/, OcdTextSymbol& /*ocd_text_symbol*/)
{
	// nothing
}


template< class OcdTextSymbolBasic >
void OcdFileExport::setupTextSymbolBasic(const TextSymbol* text_symbol, int alignment, OcdTextSymbolBasic& ocd_text_basic)
{
	ocd_text_basic.color = convertColor(text_symbol->getColor());
	ocd_text_basic.font_size = decltype(ocd_text_basic.font_size)(qRound(10 * text_symbol->getFontSize() / 25.4 * 72.0));
	ocd_text_basic.font_weight = text_symbol->isBold() ? 700 : 400;
	ocd_text_basic.font_italic = text_symbol->isItalic() ? 1 : 0;
	ocd_text_basic.char_spacing = decltype(ocd_text_basic.char_spacing)(convertSize(qRound(1000 * text_symbol->getCharacterSpacing())));
	if (ocd_text_basic.char_spacing != 0)
		addWarning(tr("In text symbol %1: custom character spacing is set,"
		              "its implementation does not match OCAD's behavior yet")
		           .arg(text_symbol->getPlainTextName()));
	ocd_text_basic.word_spacing = 100;
	ocd_text_basic.alignment = alignment;	// Default value, we might have to change this or even create copies of this symbol with other alignments later
}


template< class OcdTextSymbolSpecial >
void OcdFileExport::setupTextSymbolSpecial(const TextSymbol* text_symbol, OcdTextSymbolSpecial& ocd_text_special)
{
	auto absolute_line_spacing = text_symbol->getLineSpacing()
	                             * (text_symbol->getFontMetrics().lineSpacing() / text_symbol->calculateInternalScaling());
	ocd_text_special.line_spacing = decltype(ocd_text_special.line_spacing)(qRound(absolute_line_spacing / (text_symbol->getFontSize() * 0.01)));
	ocd_text_special.para_spacing = convertSize(qRound(1000 * text_symbol->getParagraphSpacing()));
	if (text_symbol->isUnderlined())
		addWarning(tr("In text symbol %1: ignoring underlining").arg(text_symbol->getPlainTextName()));
	if (text_symbol->usesKerning())
		addWarning(tr("In text symbol %1: ignoring kerning").arg(text_symbol->getPlainTextName()));
	
	ocd_text_special.line_below_on = text_symbol->hasLineBelow() ? 1 : 0;
	ocd_text_special.line_below_color = convertColor(text_symbol->getLineBelowColor());
	ocd_text_special.line_below_width = decltype(ocd_text_special.line_below_width)(convertSize(qRound(1000 * text_symbol->getLineBelowWidth())));
	ocd_text_special.line_below_offset = decltype(ocd_text_special.line_below_offset)(convertSize(qRound(1000 * text_symbol->getLineBelowDistance())));
	
	ocd_text_special.num_tabs = text_symbol->getNumCustomTabs();
	auto last_tab = std::min(ocd_text_special.num_tabs, decltype(ocd_text_special.num_tabs)(std::extent<typename std::remove_pointer<decltype(ocd_text_special.tab_pos)>::type>::value));
	for (auto i = 0u; i < last_tab; ++i)
		ocd_text_special.tab_pos[i] = convertSize(text_symbol->getCustomTab(i));
}


template< class OcdTextSymbolFraming >
void OcdFileExport::setupTextSymbolFraming(const TextSymbol* text_symbol, OcdTextSymbolFraming& ocd_text_framing)
{
	if (text_symbol->getFramingColor())
	{
		ocd_text_framing.color = convertColor(text_symbol->getFramingColor());
		switch (text_symbol->getFramingMode())
		{
		case TextSymbol::NoFraming:
			ocd_text_framing.mode = 0;
			ocd_text_framing.color = 0;
			break;
		case TextSymbol::ShadowFraming:
			ocd_text_framing.mode = 1;
			ocd_text_framing.offset_x = convertSize(text_symbol->getFramingShadowXOffset());
			ocd_text_framing.offset_y = -convertSize(text_symbol->getFramingShadowYOffset());
			break;
		case TextSymbol::LineFraming:
			ocd_text_framing.mode = 2;
			ocd_text_framing.line_width = convertSize(text_symbol->getFramingLineHalfWidth());
			break;
		}
	}
}



template< class Format >
void OcdFileExport::exportCombinedSymbol(OcdFile<Format>& file, const CombinedSymbol* combined_symbol)
{
	auto num_parts = 0;  // The count of non-null parts.
	const Symbol* parts[3] = {};  // A random access list without holes
	for (auto i = 0; i < combined_symbol->getNumParts(); ++i)
	{
		if (auto part = combined_symbol->getPart(i))
		{
			if (num_parts < 3)
				parts[num_parts] = part;
			++num_parts;
		}
	}
	
	const auto symbol_number = symbol_numbers.at(combined_symbol);
	switch (num_parts)
	{
	case 1:
		// Single subsymbol: Output just this subsymbol, if sufficient.
		switch (combined_symbol->getType())
		{
		case Symbol::Area:
			{
				auto copy = duplicate(static_cast<const AreaSymbol&>(*parts[0]));
				copySymbolHead(*combined_symbol, *copy);
				auto ocd_subsymbol = exportAreaSymbol<typename Format::AreaSymbol>(copy.get(), symbol_number);
				file.symbols().insert(ocd_subsymbol);
			}
			return;
		case Symbol::Line:
			{
				auto copy = duplicate(static_cast<const LineSymbol&>(*parts[0]));
				copySymbolHead(*combined_symbol, *copy);
				auto ocd_subsymbol = exportLineSymbol<typename Format::LineSymbol>(copy.get(), symbol_number);
				file.symbols().insert(ocd_subsymbol);
			}
			return;
		case Symbol::Combined:
			break;
		case Symbol::Point:
		case Symbol::Text:
		case Symbol::NoSymbol:
		case Symbol::AllSymbols:
			Q_UNREACHABLE();
		}
		break;
		
	case 2:
		// Two subsymbols: Area with border, or line with framing, if sufficient.
	case 3:
		// Three subsymbols: Line with framing line and filled double line, if sufficient.
		if (parts[0]->getType() != Symbol::Line && parts[1]->getType() != Symbol::Line)
		{
			break;
		}
		
		if (parts[1]->getType() == Symbol::Area)
		{
			std::swap(parts[0], parts[1]);
		}
		
		if (parts[0]->getType() == Symbol::Area)
		{
			if (ocd_version < 9 || num_parts != 2)
				break;
			
			// Area symbol with border, since OCD V9
			auto border_symbol = static_cast<const LineSymbol*>(parts[1]);
			if (symbol_numbers.find(border_symbol) == end(symbol_numbers))
			{
				// An unknown border symbol must be a private one
				auto border_duplicate = duplicate(static_cast<const LineSymbol&>(*border_symbol));
				copySymbolHead(*combined_symbol, *border_duplicate);
				border_duplicate->setName(QLatin1String("Border of ") + border_symbol->getName());
				border_symbol = border_duplicate.get();
				number_owners.emplace_back(std::move(border_duplicate));
				auto border_symbol_number = makeUniqueSymbolNumber(symbol_number);
				symbol_numbers[border_symbol] = border_symbol_number;
				file.symbols().insert(exportLineSymbol<typename Format::LineSymbol>(border_symbol, border_symbol_number));
			}
			
			auto copy = duplicate(static_cast<const AreaSymbol&>(*parts[0]));
			copySymbolHead(*combined_symbol, *copy);
			file.symbols().insert(exportCombinedAreaSymbol<typename Format::AreaSymbol>(symbol_number, copy.get(), border_symbol));
			return;
		}
		
		if (parts[0]->getType() == Symbol::Line && parts[1]->getType() == Symbol::Line
		    && (num_parts == 2 || parts[2]->getType() == Symbol::Line))
		{
			// Complex line symbol
			// Desired assignment, after rearrangement
			auto main_line   = static_cast<const LineSymbol*>(parts[0]);
			auto framing     = static_cast<const LineSymbol*>(parts[1]);
			auto double_line = static_cast<const LineSymbol*>(parts[2]);
			if (!maybeDoubleFilling(double_line))
			{
				// Select candidate double line/filling
				if (maybeDoubleFilling(main_line))
					std::swap(main_line, double_line);
				else if (maybeDoubleFilling(framing))
					std::swap(framing, double_line);
				else if (double_line)
					break;
			}
			if (!maybeFraming(framing))
			{
				// Select candidate framing
				if (!main_line || maybeFraming(main_line))
					std::swap(main_line, framing);
				else if (framing)
					break;
			}
			if (!maybeMainLine(main_line))
			{
				if (main_line)
					break;
				std::swap(main_line, framing);
			}
			
			// Line symbol with framing and/or double line
			auto copy = duplicate(static_cast<const LineSymbol&>(*main_line));
			copySymbolHead(*combined_symbol, *copy);
			file.symbols().insert(exportCombinedLineSymbol<typename Format::LineSymbol>(symbol_number, copy.get(), framing, double_line));
			return;
		}
		break;
	
	default:
		break;
	}
	
	// Fallback
	exportGenericCombinedSymbol<Format>(file, combined_symbol);
}


template< class Format >
void OcdFileExport::exportGenericCombinedSymbol(OcdFile<Format>& file, const CombinedSymbol* combined_symbol)
{
	// Generic handling: breadown into indidual symbols (and later objects)
	auto symbol_number = symbol_numbers[combined_symbol];
	auto number_owner = std::unique_ptr<const Symbol>();
	breakdown_index[symbol_number] = breakdown_list.size();
	
	for (int i = 0; i < combined_symbol->getNumParts(); ++i)
	{
		const Symbol* part = combined_symbol->getPart(i);
		QByteArray ocd_data;
		quint8 type = 0;
		switch (part->getType())
		{
		case Symbol::Area:
			type = 3;
			if (combined_symbol->isPartPrivate(i))
				ocd_data = exportAreaSymbol<typename Format::AreaSymbol>(static_cast<const AreaSymbol*>(part), symbol_number);
			break;
		case Symbol::Line:
			type = 2;
			if (combined_symbol->isPartPrivate(i))
				ocd_data = exportLineSymbol<typename Format::LineSymbol>(static_cast<const LineSymbol*>(part), symbol_number);
			break;
		case Symbol::Combined:
			type = 99;
			break;
		case Symbol::Point:
		case Symbol::Text:
			qWarning("In combined symbol %s: Unsupported subsymbol at index %d.",
			        qPrintable(combined_symbol->getPlainTextName()), i);
			break;
		case Symbol::NoSymbol:
		case Symbol::AllSymbols:
			Q_UNREACHABLE();
		}
		if (type == 0)
		{
			addWarning(tr("In combined symbol %1: Unsupported subsymbol at index %2.")
			           .arg(combined_symbol->getPlainTextName(), QString::number(i)));
		}
		else if (!combined_symbol->isPartPrivate(i))
		{
			breakdown_list.push_back({symbol_numbers.at(part), type});
		}
		else
		{
			Q_ASSERT(!ocd_data.isEmpty());
			breakdown_list.push_back({symbol_number, type});
			if (number_owner)
			{
				number_owners.emplace_back(std::move(number_owner));
				symbol_numbers[number_owner.get()] = symbol_number;
			}
			file.symbols().insert(ocd_data);
			// Prepare for next private part
			symbol_number = makeUniqueSymbolNumber(symbol_number);
			number_owner.reset(new PointSymbol());
		}
	}
	breakdown_list.push_back({0, 0});  // end of breakdown
	return;
}


template< >
QByteArray OcdFileExport::exportCombinedAreaSymbol<Ocd::AreaSymbolV8>(quint32 /*symbol_number*/, const AreaSymbol* /*area_symbol*/, const LineSymbol* /*line_symbol*/)
{
	Q_UNREACHABLE();
}


template< class OcdAreaSymbol >
QByteArray OcdFileExport::exportCombinedAreaSymbol(quint32 symbol_number, const AreaSymbol* area_symbol, const LineSymbol* line_symbol)
{
	auto ocd_symbol = exportAreaSymbol<OcdAreaSymbol>(area_symbol, symbol_number);
	auto ocd_subsymbol_data = reinterpret_cast<OcdAreaSymbol*>(ocd_symbol.data());
	ocd_subsymbol_data->common.border_on_V9 = 1;
	ocd_subsymbol_data->border_symbol = symbol_numbers[line_symbol];
	return ocd_symbol;
}


template< class OcdLineSymbol >
QByteArray OcdFileExport::exportCombinedLineSymbol(quint32 symbol_number, const LineSymbol* main_line, const LineSymbol* framing, const LineSymbol* double_line)
{
	auto ocd_symbol = exportLineSymbol<OcdLineSymbol>(main_line, symbol_number);
	auto& ocd_line_common = reinterpret_cast<OcdLineSymbol*>(ocd_symbol.data())->common;
	
	if (framing)
	{
		ocd_line_common.framing_color = convertColor(framing->getColor());
		ocd_line_common.framing_width = convertSize(framing->getLineWidth());
		// Cap and Join
		if (framing->getCapStyle() == LineSymbol::FlatCap && framing->getJoinStyle() == LineSymbol::BevelJoin)
			ocd_line_common.framing_style = 0;
		else if (framing->getCapStyle() == LineSymbol::RoundCap && framing->getJoinStyle() == LineSymbol::RoundJoin)
			ocd_line_common.framing_style = 1;
		else if (framing->getCapStyle() == LineSymbol::FlatCap && framing->getJoinStyle() == LineSymbol::MiterJoin)
			ocd_line_common.framing_style = 4;
		else
		{
			addWarning(tr("In line symbol \"%1\", cannot represent cap/join combination.").arg(main_line->getPlainTextName()));
			// Decide based on the caps
			if (framing->getCapStyle() == LineSymbol::RoundCap)
				ocd_line_common.framing_style = 1;
			else
				ocd_line_common.framing_style = 0;
		}
	}
	
	if (double_line)
	{
		exportLineSymbolDoubleLine(double_line, convertColor(double_line->getColor()), ocd_line_common);
	}
	
	return ocd_symbol;
}



void OcdFileExport::exportSymbolIconV6(const Symbol* symbol, quint8 icon_bits[])
{
	// Icon: 22x22 with 4 bit palette color, origin at bottom left
	constexpr int icon_size = 22;
	QImage image = symbol->createIcon(*map, icon_size, false)
	               .convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
	auto process_pixel = [&image](int x, int y)->int {
		// Apply premultiplied pixel on white background
		auto premultiplied = image.pixel(x, y);
		auto alpha = qAlpha(premultiplied);
		auto r = 255 - alpha + qRed(premultiplied);
		auto g = 255 - alpha + qGreen(premultiplied);
		auto b = 255 - alpha + qBlue(premultiplied);
		auto pixel = qRgb(r, g, b);
		
		// Ordered dithering 2x2 threshold matrix, adjusted for o-map halftones
		static int threshold[4] = { 24, 192, 136, 80 };
		auto palette_color = getPaletteColorV6(pixel);
		switch (palette_color)
		{
		case 0:
			// Black to gray (50%)
			return  qGray(pixel) < 128-threshold[(x%2 + 2*(y%2))]/2 ? 0 : 7;
			
		case 7:
			// Gray (50%) to light gray 
			return  qGray(pixel) < 192-threshold[(x%2 + 2*(y%2))]/4 ? 7 : 8;
			
		case 8:
			// Light gray to white
			return  qGray(pixel) < 256-threshold[(x%2 + 2*(y%2))]/4 ? 8 : 15;
			
		case 15:
			// Pure white
			return palette_color;
			
		default:
			// Color to white
			return  QColor(pixel).saturation() >= threshold[(x%2 + 2*(y%2))] ? palette_color : 15;
		}
	};
	
	for (int y = icon_size - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon_size; x += 2)
		{
			auto first = process_pixel(x, y);
			auto second = process_pixel(x+1, y);
			*(icon_bits++) = quint8((first << 4) + second);
		}
		icon_bits++;
	}
}

void OcdFileExport::exportSymbolIconV9(const Symbol* symbol, quint8 icon_bits[])
{
	// Icon: 22x22 with 8 bit palette color code, origin at bottom left
	constexpr int icon_size = 22;
	QImage image = symbol->createIcon(*map, icon_size, true)
	               .convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
	auto process_pixel = [&image](int x, int y)->quint8 {
		// Apply premultiplied pixel on white background
		auto premultiplied = image.pixel(x, y);
		auto alpha = qAlpha(premultiplied);
		auto r = 255 - alpha + qRed(premultiplied);
		auto g = 255 - alpha + qGreen(premultiplied);
		auto b = 255 - alpha + qBlue(premultiplied);
		return getPaletteColorV9(qRgb(r, g, b));
	};
	
	for (int y = icon_size - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon_size; ++x)
		{
			*(icon_bits++) = process_pixel(x, y);
		}
	}
}



template<class Format>
void OcdFileExport::exportObjects(OcdFile<Format>& file)
{
	for (int l = 0; l < map->getNumParts(); ++l)
	{
		auto part = map->getPart(std::size_t(l));
		for (int o = 0; o < part->getNumObjects(); ++o)
		{
			const auto* object = part->getObject(o);
			
			std::unique_ptr<Object> duplicate;
			if (area_offset.nativeX() != 0 || area_offset.nativeY() != 0)
			{
				// Create a safely managed duplicate and move it as needed.
				duplicate.reset(object->duplicate());
				duplicate->move(-area_offset);  /// \todo move pattern origin etc.
				object = duplicate.get();
			}
			object->update();
			
			QByteArray ocd_object;
			auto entry = typename Format::Object::IndexEntryType {};
			
			switch (object->getType())
			{
			case Object::Point:
				ocd_object = exportPointObject<typename Format::Object>(static_cast<const PointObject*>(object), entry);
				Q_ASSERT(!ocd_object.isEmpty());
				file.objects().insert(ocd_object, entry);
				break;
				
			case Object::Path:
				exportPathObject<Format>(file, static_cast<const PathObject*>(object));
				break;
				
			case Object::Text:
				ocd_object = exportTextObject<typename Format::Object>(static_cast<const TextObject*>(object), entry);
				Q_ASSERT(!ocd_object.isEmpty());
				file.objects().insert(ocd_object, entry);
				break;
			}
			
		}
	}
}


/**
 * Object setup which depends on the type features, not on minor type variations of members.
 */
template< class OcdObject >
void handleObjectExtras(OcdObject& ocd_object, typename OcdObject::IndexEntryType& entry)
{
	// Extra entry members since V9
	entry.type = ocd_object.type;
	entry.status = Ocd::ObjectNormal;
}


template< >
void handleObjectExtras<Ocd::ObjectV8>(Ocd::ObjectV8& ocd_object, typename Ocd::ObjectV8::IndexEntryType& /*entry*/)
{
	switch (ocd_object.type)
	{
	case 4:
	case 5:
		ocd_object.unicode = 1;
		break;
	default:
		;  // nothing
	}
}


template< class OcdObject >
QByteArray OcdFileExport::exportPointObject(const PointObject* point, typename OcdObject::IndexEntryType& entry)
{
	OcdObject ocd_object = {};
	ocd_object.type = 1;
	ocd_object.symbol = entry.symbol = decltype(entry.symbol)(symbol_numbers[point->getSymbol()]);
	ocd_object.angle = decltype(ocd_object.angle)(convertRotation(point->getRotation()));
	return exportObjectCommon(point, ocd_object, entry);
}


template< class Format >
void OcdFileExport::exportPathObject(OcdFile<Format>& file, const PathObject* path, bool lines_only)
{
	typename Format::Object ocd_object = {};
	typename Format::Object::IndexEntryType entry = {};
	
	bool need_split_lines = false;
	auto symbol = path->getSymbol();
	if (symbol &&  symbol->getContainedTypes() & Symbol::Area)
	{
		ocd_object.type = 3;  // Area symbol
		// We we may get away with an object with holes,
		// but need to check the breakdown.
	}
	else
	{
		ocd_object.type = 2;  // Line symbol
		// We need to split multiple path parts into multiple Ocd objects.
		need_split_lines = path->parts().size() > 1;
	}
	
	if (!need_split_lines)
	{
		ocd_object.symbol = entry.symbol = decltype(entry.symbol)(symbol_numbers[symbol]);
		auto data = exportObjectCommon(path, ocd_object, entry);
		Q_ASSERT(!data.isEmpty());
		
		auto breakdown_index_entry = breakdown_index.find(quint32(entry.symbol));
		if (breakdown_index_entry == end(breakdown_index))
		{
			// Regular symbol which does not need to be split
			file.objects().insert(data, entry);
			return;
		}
		
		// Combined symbol
		auto backlog = std::vector<quint32>();
		auto offset = quint32(breakdown_index_entry->second);
		auto& exported_ocd_object = reinterpret_cast<typename Format::Object&>(*data.data());
		do
		{
			auto breakdown = begin(breakdown_list) + offset;
			for ( ; breakdown->number != 0; ++breakdown)
			{
				if (Q_UNLIKELY(breakdown->type == 99))
				{
					auto child_index_entry = breakdown_index.find(breakdown->number);
					Q_ASSERT(child_index_entry != end(breakdown_index));
					backlog.push_back(quint32(child_index_entry->second));
					continue;
				}
				if (breakdown->type != 2 && lines_only)
				{
					continue;
				}
				if (Q_UNLIKELY(breakdown->type == 2 && path->parts().size() > 1))
				{
					// Subsymbol of type line.
					// We need to split multiple path parts into multiple Ocd objects.
					need_split_lines = true;
					continue;
				}
				
				exported_ocd_object.symbol = entry.symbol = decltype(entry.symbol)(breakdown->number);
				exported_ocd_object.type = decltype(exported_ocd_object.type)(breakdown->type);
				handleObjectExtras(exported_ocd_object, entry);  // update entry.type if it exists
				file.objects().insert(data, entry);
			}
			
			if (backlog.empty())
				break;
			offset = backlog.back();
			backlog.pop_back();
		}
		while (offset);
	}
		
	if (need_split_lines)
	{
		for (auto& part : path->parts())
		{
			PathObject split_line{part};
			split_line.setSymbol(path->getSymbol(), true);
			split_line.update();
			exportPathObject(file, &split_line, true);
		}
	}
	
}


template< class OcdObject >
QByteArray OcdFileExport::exportTextObject(const TextObject* text, typename OcdObject::IndexEntryType& entry)
{
	auto symbol = static_cast<const TextSymbol*>(text->getSymbol());
	auto alignment = text->getHorizontalAlignment();
	auto text_format = std::find_if(begin(text_format_mapping), end(text_format_mapping), [symbol, alignment](const auto& m) {
		return m.symbol == symbol && m.alignment == alignment;
	});
	Q_ASSERT(text_format != end(text_format_mapping));
	
	OcdObject ocd_object = {};
	ocd_object.type = text->hasSingleAnchor() ? 4 : 5;
	ocd_object.symbol = entry.symbol = decltype(entry.symbol)(text_format->symbol_number);
	ocd_object.angle = decltype(ocd_object.angle)(convertRotation(text->getRotation()));
	return exportObjectCommon(text, ocd_object, entry);
}


template< class OcdObject >
QByteArray OcdFileExport::exportObjectCommon(const Object* object, OcdObject& ocd_object, typename OcdObject::IndexEntryType& entry)
{
	
	auto& coords = object->getRawCoordinateVector();
	QByteArray text_data;
	switch(ocd_object.type)
	{
	case 4:
		ocd_object.num_items = (static_cast<const TextObject*>(object)->getNumLines() == 0) ? 0 : 5;
		if (ocd_object.num_items > 0)
		{
			text_data = exportTextData(static_cast<const TextObject*>(object), sizeof(Ocd::OcdPoint32) * 8, 1024 / 8);
			ocd_object.num_text = decltype(ocd_object.num_text)(text_data.size() / int(sizeof(Ocd::OcdPoint32)));
		}
		break;
	case 5:
		ocd_object.num_items = (static_cast<const TextObject*>(object)->getNumLines() == 0) ? 0 : 4;
		if (ocd_object.num_items > 0)
		{
			text_data = exportTextData(static_cast<const TextObject*>(object), sizeof(Ocd::OcdPoint32) * 8, 1024 / 8);
			ocd_object.num_text = decltype(ocd_object.num_text)(text_data.size() / int(sizeof(Ocd::OcdPoint32)));
		}
		break;
	default:
		ocd_object.num_items = decltype(ocd_object.num_items)(coords.size());
	}
	
	entry.bottom_left_bound = convertPoint(MapCoord(object->getExtent().bottomLeft()));
	entry.top_right_bound = convertPoint(MapCoord(object->getExtent().topRight()));
	handleObjectExtras(ocd_object, entry);

	auto header_size = int(sizeof(OcdObject) - sizeof(Ocd::OcdPoint32));
	auto items_size = int((ocd_object.num_items + ocd_object.num_text) * sizeof(Ocd::OcdPoint32));
	
	QByteArray data;
	data.reserve(header_size + items_size);
	data.append(reinterpret_cast<const char*>(&ocd_object), header_size);
	if (ocd_object.num_items > 0)
	{
		switch(ocd_object.type)
		{
		case 4:
			exportTextCoordinatesSingle(static_cast<const TextObject*>(object), data);
			data.append(text_data);
			break;
		case 5:
			exportTextCoordinatesBox(static_cast<const TextObject*>(object), data);
			data.append(text_data);
			break;
		default:
			exportCoordinates(coords, object->getSymbol(), data);
		}
	}
	Q_ASSERT(data.size() == header_size + items_size);
	
	entry.size = decltype(entry.size)((Ocd::addPadding(data).size()));
	if (ocd_version < 11)
		entry.size = (entry.size - decltype(entry.size)(header_size)) / sizeof(Ocd::OcdPoint32);
	
	return data;
}



template<class Format>
void OcdFileExport::exportTemplates(OcdFile<Format>& /*file*/)
{
	exportTemplates();
}


void OcdFileExport::exportTemplates()
{
	for (int i = map->getNumTemplates() - 1; i >= 0; --i)
	{
		const auto temp = map->getTemplate(i);
		if (qstrcmp(temp->getTemplateType(), "TemplateImage") == 0
		    || QFileInfo(temp->getTemplatePath()).suffix().compare(QLatin1String("ocd"), Qt::CaseInsensitive) == 0)
		{
			addParameterString(8, stringForTemplate(*temp, area_offset, ocd_version));
		}
		else
		{
			addWarning(tr("Unable to export template: file type of \"%1\" is not supported yet").arg(temp->getTemplateFilename()));
		}
	}
}



template<class Format>
void OcdFileExport::exportExtras(OcdFile<Format>& /*file*/)
{
	exportExtras();
}


void OcdFileExport::exportExtras()
{
	/// \todo
}


quint16 OcdFileExport::convertColor(const MapColor* color) const
{
	auto index = map->findColorIndex(color);
	if (index >= 0)
	{
		return quint16(uses_registration_color ? (index + 1) : index);
	}
	return 0;
}


quint16 OcdFileExport::getPointSymbolExtent(const PointSymbol* symbol) const
{
	if (!symbol)
		return 0;
	
	QRectF extent;
	for (int i = 0; i < symbol->getNumElements(); ++i)
	{
		std::unique_ptr<Object> object { symbol->getElementObject(i)->duplicate() };
		object->setSymbol(symbol->getElementSymbol(i), true);
		object->update();
		rectIncludeSafe(extent, object->getExtent());
		object->clearRenderables();
	}
	auto extent_f = 0.5 * std::max(extent.width(), extent.height());
	if (symbol->getInnerColor())
		extent_f = std::max(extent_f, 0.001 * symbol->getInnerRadius());
	if (symbol->getOuterColor())
		extent_f = std::max(extent_f, 0.001 * (symbol->getInnerRadius() + symbol->getOuterWidth()));
	return quint16(convertSize(qRound(std::max(0.0, 1000 * extent_f))));
}


quint16 OcdFileExport::exportCoordinates(const MapCoordVector& coords, const Symbol* symbol, QByteArray& byte_array)
{
	quint16 num_points = 0;
	bool curve_start = false;
	bool hole_point = false;
	bool curve_continue = false;
	for (const auto& point : coords)
	{
		auto p = convertPoint(point);
		if (point.isDashPoint())
		{
			if (!symbol || symbol->getType() != Symbol::Line)
				p.y |= Ocd::OcdPoint32::FlagCorner;
			else
			{
				const LineSymbol* line_symbol = static_cast<const LineSymbol*>(symbol);
				if ((line_symbol->getDashSymbol() == nullptr || line_symbol->getDashSymbol()->isEmpty()) && line_symbol->isDashed())
					p.y |= Ocd::OcdPoint32::FlagDash;
				else
					p.y |= Ocd::OcdPoint32::FlagCorner;
			}
		}
		if (curve_start)
			p.x |= Ocd::OcdPoint32::FlagCtl1;
		if (hole_point)
			p.y |= Ocd::OcdPoint32::FlagHole;
		if (curve_continue)
			p.x |= Ocd::OcdPoint32::FlagCtl2;
		
		curve_continue = curve_start;
		curve_start = point.isCurveStart();
		hole_point = point.isHolePoint();
		
		byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
		++num_points;
	}
	return num_points;
}


quint16 OcdFileExport::exportTextCoordinatesSingle(const TextObject* object, QByteArray& byte_array)
{
	if (object->getNumLines() == 0)
		return 0;
	
	auto text_to_map = object->calcTextToMapTransform();
	auto map_to_text = object->calcMapToTextTransform();
	
	// Create 5 coordinates:
	// 0 - baseline anchor point
	// 1 - bottom left
	// 2 - bottom right
	// 3 - top right
	// 4 - top left
	
	auto anchor = QPointF(object->getAnchorCoordF());
	auto anchor_text = map_to_text.map(anchor);
	
	auto line0 = object->getLineInfo(0);
	auto p = convertPoint(MapCoord(text_to_map.map(QPointF(anchor_text.x(), line0->line_y))));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	
	QRectF bounding_box_text;
	for (int i = 0; i < object->getNumLines(); ++i)
	{
		auto info = object->getLineInfo(i);
		rectIncludeSafe(bounding_box_text, QPointF(info->line_x, info->line_y - info->ascent));
		rectIncludeSafe(bounding_box_text, QPointF(info->line_x + info->width, info->line_y + info->descent));
	}
	
	p = convertPoint(MapCoord(text_to_map.map(bounding_box_text.bottomLeft())));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(text_to_map.map(bounding_box_text.bottomRight())));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(text_to_map.map(bounding_box_text.topRight())));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(text_to_map.map(bounding_box_text.topLeft())));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	
	return 5;
}


quint16 OcdFileExport::exportTextCoordinatesBox(const TextObject* object, QByteArray& byte_array)
{
	if (object->getNumLines() == 0)
		return 0;
	
	// As OCD 8 only supports Top alignment, we have to replace the top box coordinates by the top coordinates of the first line
	auto text_symbol = static_cast<const TextSymbol*>(object->getSymbol());
	auto metrics = text_symbol->getFontMetrics();
	auto internal_scaling = text_symbol->calculateInternalScaling();
	auto line0 = object->getLineInfo(0);
	
	auto new_top = (object->getVerticalAlignment() == TextObject::AlignTop) ? (-object->getBoxHeight() / 2) : ((line0->line_y - line0->ascent) / internal_scaling);
	// Account for extra internal leading
	auto top_adjust = -text_symbol->getFontSize() + (metrics.ascent() + metrics.descent() + 0.5) / internal_scaling;
	new_top = new_top - top_adjust;
	
	QTransform transform;
	transform.rotate(-qRadiansToDegrees(object->getRotation()));
	auto p = convertPoint(MapCoord(transform.map(QPointF(-object->getBoxWidth() / 2, object->getBoxHeight() / 2)) + object->getAnchorCoordF()));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(transform.map(QPointF(object->getBoxWidth() / 2, object->getBoxHeight() / 2)) + object->getAnchorCoordF()));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(transform.map(QPointF(object->getBoxWidth() / 2, new_top)) + object->getAnchorCoordF()));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	p = convertPoint(MapCoord(transform.map(QPointF(-object->getBoxWidth() / 2, new_top)) + object->getAnchorCoordF()));
	byte_array.append(reinterpret_cast<const char*>(&p), sizeof(p));
	
	return 4;
}


QByteArray OcdFileExport::exportTextData(const TextObject* object, int chunk_size, int max_chunks)
{
	auto max_size = chunk_size * max_chunks;
	Q_ASSERT(max_size > 0);
	
	// Convert text to OCD format:
	// - If it starts with a newline, add another.
	// - Convert '\n' to '\r\n'
	QString text = object->getText();
	if (text.startsWith(QLatin1Char('\n')))
		text.prepend(QLatin1Char('\n'));
	text.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
	
	// Suppress byte order mark by using QTextCodec::IgnoreHeader.
	static auto encoding_2byte = QTextCodec::codecForName("UTF-16LE");
	auto encoder = encoding_2byte->makeEncoder(QTextCodec::IgnoreHeader);
	auto encoded_text = encoder->fromUnicode(text);
	if (encoded_text.size() >= max_size)
	{
		// Truncate safely by decoding truncated encoded data.
		auto decoder = encoding_2byte->makeDecoder();
		auto truncated_text = decoder->toUnicode(encoded_text.constData(), max_size - 1);
		delete decoder;
		addTextTruncationWarning(text, truncated_text.length());
		encoded_text = encoder->fromUnicode(truncated_text);
	}
	delete encoder;
	
	auto text_size = encoded_text.size();
	Q_ASSERT(text_size < max_size);
	
	// Resize to multiple of chunk size, appending trailing zeros.
	encoded_text.resize(text_size + (max_size - text_size) % chunk_size);
	Q_ASSERT(encoded_text.size() <= max_size);
	Q_ASSERT(encoded_text.size() % chunk_size == 0);
	std::fill(encoded_text.begin() + text_size, encoded_text.end(), 0);
	return encoded_text;
}



void OcdFileExport::addTextTruncationWarning(QString text, int pos)
{
	text.insert(pos, QLatin1Char('|'));
	addWarning(tr("Text truncated at '|'): %1").arg(text));
}



quint32 OcdFileExport::makeUniqueSymbolNumber(quint32 initial_number) const
{
	auto matches_symbol_number = [&initial_number](const auto& entry) { return initial_number == entry.second; };
	while (std::any_of(begin(symbol_numbers), end(symbol_numbers), matches_symbol_number))
		++initial_number;
	return initial_number;
}


}  // namespace OpenOrienteering
