/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015 Kai Pastor
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


#include "map_color.h"

#include <algorithm>
#include <iterator>

#include <Qt>
#include <QCoreApplication>
#include <QLatin1Char>
#include <QLatin1String>


namespace OpenOrienteering {

MapColor::MapColor()
: name(QCoreApplication::translate("OpenOrienteering::Map", "New color")),
  priority(Undefined),
  opacity(1.0f),
  q_color(Qt::black),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name()
{
	Q_ASSERT(isBlack());
}

MapColor::MapColor(int priority)
: name(QCoreApplication::translate("OpenOrienteering::Map", "New color")),
  priority(priority),
  opacity(1.0f),
  q_color(Qt::black),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name()
{
	Q_ASSERT(isBlack());
	
	switch (priority)
	{
		case CoveringWhite:
			setCmyk(QColor(Qt::white));
			Q_ASSERT(isWhite());
			opacity = 1000.0f;	// HACK: (almost) always opaque, even if multiplied by opacity factors
			break;
		case CoveringRed:
			setRgb(QColor(Qt::red));
			setCmykFromRgb();
			opacity = 1000.0f;	// HACK: (almost) always opaque, even if multiplied by opacity factors
			break;
		case Undefined:
			setCmyk(QColor(Qt::darkGray));
			break;
		case Registration:
			Q_ASSERT(isBlack());
			name = QCoreApplication::translate("OpenOrienteering::MapColor", "Registration black (all printed colors)");
			break;
		default:
			; // no change
	}	
}

MapColor::MapColor(const QString& name, int priority)
: name(name),
  priority(priority),
  opacity(1.0),
  q_color(Qt::black),
  spot_color_method(MapColor::UndefinedMethod),
  cmyk_color_method(MapColor::CustomColor),
  rgb_color_method(MapColor::CmykColor),
  flags(0),
  spot_color_name()
{
	Q_ASSERT(isBlack());
}


MapColor* MapColor::duplicate() const
{
	MapColor* copy = new MapColor(name, priority);
	copy->cmyk = cmyk;
	copy->rgb = rgb;
	copy->opacity = opacity;
	copy->q_color = q_color;
	copy->spot_color_method = spot_color_method;
	copy->cmyk_color_method = cmyk_color_method;
	copy->rgb_color_method  = rgb_color_method;
	copy->flags = flags;
	copy->spot_color_name = spot_color_name;
	copy->screen_angle = screen_angle;
	copy->screen_frequency = screen_frequency;
	copy->components = components;
	return copy;
}

bool MapColor::isBlack() const
{
	return rgb.isBlack() && cmyk.isBlack();
}

bool MapColor::isWhite() const
{
	return rgb.isWhite() && cmyk.isWhite();
}


bool operator==(const SpotColorComponents& lhs, const SpotColorComponents& rhs)
{
	return lhs.size() == rhs.size()
	       && std::is_permutation(begin(lhs), end(lhs), begin(rhs), [](const auto& left, const auto& right) {
		return *left.spot_color == *right.spot_color
		       && qAbs(left.factor - right.factor) < 1e-03;
	});
}

bool MapColor::componentsEqual(const MapColor& other, bool compare_priority) const
{
	const SpotColorComponents& lhs(components);
	const SpotColorComponents& rhs(other.components);
	return lhs.size() == rhs.size()
	       && std::is_permutation(begin(lhs), end(lhs), begin(rhs), [compare_priority](const auto& left, const auto& right) {
		return left.spot_color->equals(*right.spot_color, compare_priority)
		       && qAbs(left.factor - right.factor) < 1e-03;
	});
}

bool MapColor::equals(const MapColor& other, bool compare_priority) const
{
	return (!compare_priority || (priority == other.priority)) &&
	       (name.compare(other.name, Qt::CaseInsensitive) == 0) &&
	       (spot_color_method == other.spot_color_method) &&
	       (cmyk_color_method == other.cmyk_color_method) &&
	       (rgb_color_method == other.rgb_color_method) &&
	       (flags == other.flags) &&
	       (cmyk_color_method != CustomColor || cmyk == other.cmyk) &&
	       (rgb_color_method != CustomColor || rgb == other.rgb) &&
	       (  spot_color_method == UndefinedMethod || 
	         (spot_color_method == SpotColor
	          && spot_color_name.compare(other.spot_color_name, Qt::CaseInsensitive) == 0
	          && (screen_frequency <= 0 || other.screen_frequency <= 0
	              || (std::abs(screen_angle - other.screen_angle) < 0.05
	                  && std::abs(screen_frequency - other.screen_frequency) < 0.05))) ||
	         (spot_color_method == CustomColor && componentsEqual(other, compare_priority)) ) &&
	       (qAbs(opacity - other.opacity) < 1e-03);
}


void MapColor::setSpotColorName(const QString& spot_color_name) 
{ 
	spot_color_method = MapColor::SpotColor;
	this->spot_color_name = spot_color_name;
	components.clear();
	updateCalculatedColors();
}

void MapColor::setScreenFrequency(double value)
{
	if (spot_color_method == SpotColor)
		screen_frequency = value;
}

void MapColor::setScreenAngle(double value)
{
	if (spot_color_method == SpotColor)
		screen_angle = value;
}

void MapColor::setSpotColorComposition(const SpotColorComponents& components)
{
	this->components = components;
	if (components.empty())
		spot_color_method = UndefinedMethod;
	else 
		spot_color_method = CustomColor;
	
	removeSpotColorComponent(this);
	updateCompositionName();
	updateCalculatedColors();
}

bool MapColor::removeSpotColorComponent(const MapColor* color)
{
	auto size_before = components.size();
	auto match = [this, color](const SpotColorComponent& scc) { return scc.spot_color == color; };
	components.erase(std::remove_if(begin(components), end(components), match), end(components));
	bool changed = components.size() != size_before;
	if (changed)
	{
		if (components.empty())
			spot_color_method = UndefinedMethod;
		
		updateCompositionName();
		updateCalculatedColors();
	}
	return changed;
}

void MapColor::setKnockout(bool flag)
{
	if (spot_color_method != MapColor::UndefinedMethod)
	{
		if (flag)
		{
			if (!getKnockout())
				flags += MapColor::Knockout;
		}
		else if (getKnockout())
			flags -= MapColor::Knockout;
		
		Q_ASSERT(getKnockout() == flag);
	}
}

bool MapColor::getKnockout() const
{
	return (flags & MapColor::Knockout) > 0;
}


void MapColor::setCmyk(const MapColorCmyk& new_cmyk)
{
	cmyk_color_method = MapColor::CustomColor;
	cmyk = new_cmyk;
	updateCalculatedColors();
}

void MapColor::setCmykFromSpotColors()
{
	if (spot_color_method == MapColor::CustomColor)
	{
		cmyk_color_method = MapColor::SpotColor;
		updateCalculatedColors();
	}
}

void MapColor::setCmykFromRgb()
{
	if (rgb_color_method == MapColor::CmykColor)
		rgb_color_method = MapColor::CustomColor;
	
	cmyk_color_method = MapColor::RgbColor;
	updateCalculatedColors();
}


void MapColor::setRgb(const MapColorRgb& new_rgb)
{
	rgb_color_method = MapColor::CustomColor;
	rgb = new_rgb;
	updateCalculatedColors();
}

void MapColor::setRgbFromSpotColors()
{
	if (spot_color_method == MapColor::CustomColor)
	{
		rgb_color_method = MapColor::SpotColor;
		updateCalculatedColors();
	}
}

void MapColor::setRgbFromCmyk()
{
	if (cmyk_color_method == MapColor::RgbColor)
		cmyk_color_method = MapColor::CustomColor;
	
	rgb_color_method = MapColor::CmykColor;
	updateCalculatedColors();
}

void MapColor::updateCompositionName()
{
	if (spot_color_method != MapColor::SpotColor)
	{
		spot_color_name.clear();
		for (auto& component : components)
		{
			if (!spot_color_name.isEmpty())
				spot_color_name += QLatin1String(", ");
			spot_color_name += component.spot_color->getSpotColorName() + QLatin1Char(' ')
			                   + QString::number(component.factor * 100) /* % */;
		}
	}
}

void MapColor::updateCalculatedColors()
{
	Q_ASSERT(components.size() == 0 || spot_color_method == CustomColor);
	Q_ASSERT(components.size() >  0 || spot_color_method != CustomColor);
	Q_ASSERT(!((cmyk_color_method == RgbColor) && (rgb_color_method == CmykColor)));
	
	if (spot_color_method != CustomColor)
	{
		// No composition, thus cannot determine CMYK or RGB from spot colors.
		if (cmyk_color_method == MapColor::SpotColor)
			cmyk_color_method = MapColor::CustomColor;
		
		if (rgb_color_method == MapColor::SpotColor)
			rgb_color_method = MapColor::CustomColor;
	}
	else
	{
		if (cmyk_color_method == MapColor::SpotColor)
			cmyk = cmykFromSpotColors();
			
		if (rgb_color_method == MapColor::SpotColor)
			rgb = rgbFromSpotColors();
	}
	
	if (cmyk_color_method == MapColor::RgbColor)
		cmyk = MapColorCmyk(rgb);
	
	if (rgb_color_method == MapColor::CmykColor)
		rgb = MapColorRgb(cmyk);
	
	if (cmyk_color_method != RgbColor)
		q_color = cmyk;
	else
		q_color = rgb;
	
	Q_ASSERT(components.size() >  0 || cmyk_color_method != MapColor::SpotColor);
	Q_ASSERT(components.size() >  0 || rgb_color_method  != MapColor::SpotColor);
}

MapColorCmyk MapColor::cmykFromSpotColors() const
{
	Q_ASSERT(components.size() > 0);
	
	MapColorCmyk cmyk(Qt::white);
	Q_ASSERT(cmyk.isWhite());
	for (auto&& component : components)
	{
		const MapColorCmyk& other = component.spot_color->cmyk;
		cmyk.c = cmyk.c + component.factor * other.c * (1.0f - cmyk.c);
		cmyk.m = cmyk.m + component.factor * other.m * (1.0f - cmyk.m);
		cmyk.y = cmyk.y + component.factor * other.y * (1.0f - cmyk.y);
		cmyk.k = cmyk.k + component.factor * other.k * (1.0f - cmyk.k);
	}
	return cmyk;
}

MapColorRgb MapColor::rgbFromSpotColors() const
{
	Q_ASSERT(components.size() > 0);
	
	MapColorRgb rgb = QColor(Qt::white);
	Q_ASSERT(rgb.isWhite());
	for (auto&& component : components)
	{
		const MapColorRgb& other = component.spot_color->rgb;
		rgb.r = rgb.r - component.factor * (1.0f - other.r) * rgb.r;
		rgb.g = rgb.g - component.factor * (1.0f - other.g) * rgb.g;
		rgb.b = rgb.b - component.factor * (1.0f - other.b) * rgb.b;
	}
	return rgb;
}


}  // namespace OpenOrienteering
