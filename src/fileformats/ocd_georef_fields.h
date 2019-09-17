/*
 *    Copyright 2018 Libor Pecháček
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

#ifndef OPENORIENTEERING_OCD_GEOREF_FIELDS_H
#define OPENORIENTEERING_OCD_GEOREF_FIELDS_H

#include <functional>

class QString;

namespace OpenOrienteering {

class Georeferencing;


/**
 * OCD type 1039 string field values packed in a struct for convenient
 * handling. Structure element name corresponds to field code and its value
 * is the field value.
 *
 * Fields are initialized with safe default values, commonly found in example
 * files shipped with the program.
 */
struct OcdGeorefFields
{
	double a { 0 };  ///< Real world angle
	int m { 15000 }, ///< Map scale
	    x { 0 },     ///< Real world offset easting
	    y { 0 },     ///< Real world offset northing
	    i { 1000 },  ///< Grid and zone
	    r { 0 };     ///< Real world coord (0 = paper, 1 = real world)

	/**
	 * Fill in provided georef with data extracted from type 1039 string.
	 * @param georef Reference to a Georeferencing object.
	 * @param warning_handler Function to handle import warnings.
	 */
	void setupGeoref(Georeferencing& georef,
	                 const std::function<void (const QString&)>& warning_handler) const;

	/**
	 * Translate from Mapper CRS representation into OCD one.
	 * @param georef Source Georeferencing.
	 * @param warning_handler Function to handle conversion warnings.
	 * @return Data for OCD type 1039 string compilation.
	 */
	static OcdGeorefFields fromGeoref(const Georeferencing& georef,
	                                  const std::function<void (const QString&)>& warning_handler);
};

/**
 * Equal-to operator comparing this structure with another. Field `a` gets
 * compared with 8-digit precision.
 */
bool operator==(const OcdGeorefFields& lhs, const OcdGeorefFields& rhs);

/**
 * Trivial non-equal-to operator.
 * @see operator==(const OcdGeorefFields&, const OcdGeorefFields&)
 */
inline
bool operator!=(const OcdGeorefFields& lhs, const OcdGeorefFields& rhs)
{
	return !operator==(lhs, rhs);
}

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_GEOREF_FIELDS_H
