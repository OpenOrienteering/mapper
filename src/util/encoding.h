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

#ifndef OPENORIENTEERING_UTIL_ENCODING_H
#define OPENORIENTEERING_UTIL_ENCODING_H

class QString;
class QTextCodec;

namespace OpenOrienteering {

namespace Util {

/**
 * Determines the name of the 8-bit legacy codepage for a language.
 * 
 * This function accepts language names as returned by QLocale::name().
 * Characters after the two letter language code are ignored.
 * 
 * If the language is unknown, it returns "Windows-1252".
 * 
 * @param language_name A lowercase, two-letter ISO 639 language code
 * @return A Windows codepage name.
 */
const char* codepageForLanguage(const QString& language_name);

/**
 * Determines the codec for a given name.
 * 
 * This function wraps QTextCodec::codecForName. Other than that function,
 * it will try to lookup the codepage name for the current locale if the
 * name is "Default" (case sensitive). It may return nullptr.
 */
QTextCodec* codecForName(const char* name);


}  // namespace Util

}  // namespace OpenOrienteering

#endif
