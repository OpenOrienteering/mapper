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

#ifndef OPENORIENTEERING_OCD_TYPES_V2018
#define OPENORIENTEERING_OCD_TYPES_V2018

#include "ocd_types.h"
#include "ocd_types_v12.h"

namespace Ocd
{
	/*
	 * There is no documentation for version 2018 of the OCD file format.
	 * It seems reasonable to assume only marginal changes from the version 12
	 * format:
	 * - Program features added in version 2018 do not seem to require mandatory
	 *   modifications to the file format.
	 * - Sample maps from the free version 2018 Viewer could be successfully
	 *   loaded by the free version 12 Viewer, after changing the file format
	 *   version fields in the file header.
	 */
	using FormatV2018 = FormatV12;
	
}

#endif // OPENORIENTEERING_OCD_TYPES_V2018_H
