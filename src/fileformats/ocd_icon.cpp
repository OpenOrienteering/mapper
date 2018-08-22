/*
 *    Copyright 2018 Kai Pastor
 *
 *    Some parts taken from ocad8_file_format{.h,_p.h,cpp} which are
 *    Copyright 2012, 2013 Pete Curtis
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

#include "ocd_icon.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>

#include <QtGlobal>
#include <QColor>
#include <QImage>
#include <QRgb>

#include "core/symbols/symbol.h"
#include "fileformats/ocd_types_v8.h"
#include "fileformats/ocd_types_v9.h"


namespace OpenOrienteering {

namespace {

QImage iconForExport(const Map& map, const Symbol& symbol, int width, int height)
{
	auto image = symbol.createIcon(map, std::max(width, height), true);
	if (image.format() != QImage::Format_ARGB32_Premultiplied)
		image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	return image;
}


quint8 getPaletteColorV8(QRgb rgb)
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
		static PaletteColor fromQColor(const QColor& color)
		{
			return { color.hue(), color.saturation(), color.value() };
		}
	};
	static const auto palette = []() {
		std::array<PaletteColor, 16> palette_hsv;
		const auto palette_rgb = Ocd::IconV8::palette<QColor>();
		std::transform(begin(palette_rgb), end(palette_rgb), begin(palette_hsv), [](auto& rgb){
			return PaletteColor::fromQColor(rgb.toHsv());
		});
		return palette_hsv;
	}();
	
	quint8 best_index = 0;
	auto best_distance = 2100000;  // > 6 * (10*sq(180) + sq(128) + sq(64))
	auto sq = [](int n) { return n*n; };
	for (auto i : { 1u, 2u, 3u, 4u, 5u, 6u, 9u, 10u, 11u, 12u, 13u, 14u })
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
			best_index = quint8(i);
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
	static const auto palette = Ocd::IconV9::palette<PaletteColor>();
	
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

} // namespace



OcdIcon::operator Ocd::IconV8() const
{
	Ocd::IconV8 icon;
	auto image = iconForExport(map, symbol, icon.width(), icon.height());
	auto process_pixel = [&image](int x, int y)->quint8 {
		// Apply premultiplied pixel on white background
		auto premultiplied = image.pixel(x, y);
		auto alpha = qAlpha(premultiplied);
		auto r = 255 - alpha + qRed(premultiplied);
		auto g = 255 - alpha + qGreen(premultiplied);
		auto b = 255 - alpha + qBlue(premultiplied);
		auto pixel = qRgb(r, g, b);
		
		// Ordered dithering 2x2 threshold matrix, adjusted for o-map halftones
		static int threshold[4] = { 24, 192, 136, 80 };
		auto palette_color = getPaletteColorV8(pixel);
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
	
	auto icon_bits = icon.bits;
	for (int y = icon.height() - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon.width(); x += 2)
		{
			auto first = process_pixel(x, y);
			auto second = process_pixel(x+1, y);
			*(icon_bits++) = quint8(first << 4) + second;
		}
		icon_bits++;
	}
	return icon;
}


OcdIcon::operator Ocd::IconV9() const
{
	Ocd::IconV9 icon;
	auto image = iconForExport(map, symbol, icon.width(), icon.height());
	auto process_pixel = [&image](int x, int y)->quint8 {
		// Apply premultiplied pixel on white background
		auto premultiplied = image.pixel(x, y);
		auto alpha = qAlpha(premultiplied);
		auto r = 255 - alpha + qRed(premultiplied);
		auto g = 255 - alpha + qGreen(premultiplied);
		auto b = 255 - alpha + qBlue(premultiplied);
		return getPaletteColorV9(qRgb(r, g, b));
	};
	
	auto icon_bits = icon.bits;
	for (int y = icon.height() - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon.width(); ++x)
		{
			*(icon_bits++) = process_pixel(x, y);
		}
	}
	return icon;
}


}  // namespace OpenOrienteering
