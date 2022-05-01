/*
 *    Copyright 2022 Matthias Kühlewein
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

#include "ocd_parameter_stream_reader.h"

#include <QChar>
#include <QLatin1Char>


namespace OpenOrienteering {

OcdParameterStreamReader::OcdParameterStreamReader(const QString& param_string) noexcept
 : param_string(param_string)
 , pos(0)
{}

bool OcdParameterStreamReader::readNext()
{
	while (!atEnd())
	{
		pos = param_string.indexOf(QLatin1Char('\t'), pos);
		if (pos < 0 || pos + 1 >= param_string.length())	// accept \t at end only if there is space for at least the key
		{
			pos = param_string.length();
			return false;
		}
		++pos;
		if (param_string.at(pos).toLatin1() != '\t')	// is the next key \t ? if yes then skip over
			return true;
	}
	return false;
}

char OcdParameterStreamReader::key() const
{
	if (!pos || atEnd())
		return noKey();

	return param_string.at(pos).toLatin1();
}

QStringRef OcdParameterStreamReader::value() const
{
	QStringRef ref;
	
	if (!atEnd())
	{
		int start = pos ? pos + 1 : pos;
		if (start < param_string.length())
		{
			int end = param_string.indexOf(QLatin1Char('\t'), start);
			if (end < 0)
				end = param_string.length();
			if (end > start)
				ref = param_string.midRef(start, end - start);
		}
	}
	return ref;
}


}  // namespace OpenOrienteering
