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

#include "encoding.h"

#include <QLocale>
#include <QString>
#include <QTextCodec>

namespace
{

struct LanguageMapping
{
	const char* languages;
	const char* codepage;
};

// Languages are listed as two letters, separated by single char
const LanguageMapping mappings[] = {
	{ "cs hu pl", "Windows-1250" }, // Central European
	{ "bg ru uk", "Windows-1251" }, // Cyrillic
	{ "et lt lv", "Windows-1257" }, // Baltic
	{ "el", "Windows-1253" }, // Greek
	{ "he", "Windows-1255" }, // Hebrew
#ifdef ENABLE_ALL_LANGUAGE_MAPPINGS
	{ "da de en es fi fr id it nb nl pt sv", "Windows-1252" }, // Western European (default)
	{ "tr", "Windows-1254" }, // Turkish
	{ "ar", "Windows-1256" }, // Arabic
	{ "vi", "Windows-1258" }, // Vietnamese
#endif
};

} //namespace

const char* Util::codepageForLanguage(const QString& language_name)
{
	const auto language = language_name.leftRef(2).toLatin1();
	for (const auto& mapping : mappings)
	{
		auto len = qstrlen(mapping.languages);
		for (decltype(len) i = 0; i < len; i += 3)
		{
			if (language == QByteArray::fromRawData(mapping.languages+i, 2))
				return mapping.codepage;
		}
	}
	return "Windows-1252";
}


QTextCodec* Util::codecForName(const char* name)
{
	if  (qstrcmp(name, "Default") == 0)
		return QTextCodec::codecForName(Util::codepageForLanguage(QLocale().name()));
	else
		return QTextCodec::codecForName(name);
}
