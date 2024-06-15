/*
 *    Copyright 2024 Kai Pastor
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

#ifndef OPENORIENTEERING_HTML_SYMBOL_REPORT_H
#define OPENORIENTEERING_HTML_SYMBOL_REPORT_H

class QString;

namespace OpenOrienteering {

class Map;

/**
 * Generates a symbol set report in HTML format.
 */
QString makeHTMLSymbolReport(const Map& map);

}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_HTML_SYMBOL_REPORT_H
