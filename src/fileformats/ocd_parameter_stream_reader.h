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

#ifndef OPENORIENTEERING_OCD_PARAMETER_STREAM_READER_H
#define OPENORIENTEERING_OCD_PARAMETER_STREAM_READER_H

#include <QString>
#include <QStringRef>

namespace OpenOrienteering {


/**
 * A class for processing OCD parameter string,
 * loosely modeled after QXmlStreamReader.
 */
class OcdParameterStreamReader
{
public:
	explicit OcdParameterStreamReader(const QString& param_string) noexcept;
	bool readNext();
	char key() const;
	QStringRef value() const;
	bool atEnd() const { return pos >= param_string.length(); }
	static constexpr char noKey() { return 0; }
	
private:
	const QString& param_string;
	int pos;
};

}  // namespace Openorienteering

#endif // OPENORIENTEERING_OCD_PARAMETER_STREAM_READER
