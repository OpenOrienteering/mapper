/*
 *    Copyright 2018 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_ICON_H
#define OPENORIENTEERING_OCD_ICON_H


class QImage;


namespace Ocd {

struct IconV8;
struct IconV9;

}  // namespace Ocd 


namespace OpenOrienteering {

class Map;
class Symbol;


/**
 * A utility for converting OCD icons.
 * 
 * Synopsis:
 * 
 * ocd_base_symbol.icon = OcdIcon{map, symbol};
 * 
 * symbol->setCustomIcon(OcdIcon::toQImage(ocd_base_symbol.icon));
 */
struct OcdIcon
{
	const Map& map;
	const Symbol& symbol;
	
	// Default special member functions are fine.
	
	operator Ocd::IconV8() const;
	operator Ocd::IconV9() const;
	
	/**
	 * Creates a QImage for the given uncompressed icon data.
	 * 
	 * Note: This function does not support OCD version 8 compressed icons.
	 */
	static QImage toQImage(const Ocd::IconV8& icon);
	
	/**
	 * Creates a QImage for the given icon data.
	 */
	static QImage toQImage(const Ocd::IconV9& icon);
};


}  // namespace OpenOrienteering

#endif
