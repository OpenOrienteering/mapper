/*
 *    Copyright 2013, 2014 Kai Pastor
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


#include "xml_stream_util.h"

#include "../map_coord.h"
#include "../file_format_xml.h"


void XmlElementWriter::write(const MapCoordVector& coords)
{
	namespace literal = XmlStreamLiteral;
	
	int num_coords = (int)coords.size();
	writeAttribute(literal::count, num_coords);
	
	if (XMLFileFormat::current_version < 6 || xml.autoFormatting())
	{
		// XMAP files and old format: syntactically rich output
		for (MapCoordVector::const_iterator coord = coords.begin(), end = coords.end(); coord != end; ++coord)
			coord->save(xml);
	}
	else
	{
		// Default: efficient plain text format
		QString data;
		data.reserve(coords.size() * 16);
		for (MapCoordVector::const_iterator coord = coords.begin(), end = coords.end(); coord != end; ++coord)
		{
			data.append(coord->toString());
		}
		xml.writeCharacters(data);
	}
}


void XmlElementReader::read(MapCoordVector& coords) throw (FileFormatException)
{
	namespace literal = XmlStreamLiteral;
	
	const unsigned int num_coords = attribute<unsigned int>(literal::count);
	coords.clear();
	coords.reserve(qMin(num_coords, 500000u));
	
	MapCoord coord;
	
	for( xml.readNext(); xml.tokenType() != QXmlStreamReader::EndElement; xml.readNext() )
	{
		const QXmlStreamReader::TokenType token = xml.tokenType();
		if (xml.error() || token == QXmlStreamReader::EndDocument)
		{
			throw FileFormatException(ImportExport::tr("Could not parse the data."));
		}
		else if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
		{
			QStringRef text = xml.text();
			QString data = QString::fromRawData(text.constData(), text.length());
			
			QTextStream stream(&data, QIODevice::ReadOnly);
			stream.setIntegerBase(10);
			while (!stream.atEnd())
			{
				stream >> coord;
				coords.push_back(coord);
			}
			if (stream.status() == QTextStream::ReadCorruptData)
			{
				throw FileFormatException(ImportExport::tr("Could not parse the coordinates."));
			}
		}
		else if (token == QXmlStreamReader::StartElement)
		{
			if (xml.name() == MapCoordLiteral::coord)
			{
				coords.push_back(MapCoord::load(xml));
			}
			else
			{
				xml.skipCurrentElement();
			}
		}
		// otherwise: ignore element
	}
	
	if (coords.size() != num_coords)
	{
		throw FileFormatException(ImportExport::tr("Expected %1 coordinates, found %2."));
	}
}
