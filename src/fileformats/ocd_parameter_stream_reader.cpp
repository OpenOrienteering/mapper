/*
 *    Copyright 2022 Matthias Kühlewein
 *    Copyright 2022 Kai Pastor
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

#include <cmath>

#include <QtGlobal>
#include <QChar>
#include <QLatin1Char>


namespace OpenOrienteering {

OcdParameterStreamReader::OcdParameterStreamReader(const QString& param_string) noexcept
 : param_string(param_string)
 , pos(0)
 , next(param_string.indexOf(QLatin1Char('\t')))
 , current_key(noKey())
{}

bool OcdParameterStreamReader::readNext()
{
	while (!atEnd())
	{
		pos = next + 1;
		next = param_string.indexOf(QLatin1Char('\t'), pos);
		if (pos < param_string.length())
		{
			current_key = param_string.at(pos).toLatin1();
			if (Q_LIKELY(current_key != '\t'))
			{
				++pos;
				return true;
			}
		}
	}
	
	current_key = noKey();
	pos = param_string.length();
	return false;
}

QStringRef OcdParameterStreamReader::value() const
{
	return param_string.midRef(pos, std::max(-1, next - pos));
}


}  // namespace OpenOrienteering
