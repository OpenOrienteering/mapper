/*
 *    Copyright 2013-2015 Kai Pastor
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

#include <QTextStream>

#include "../core/map_coord.h"
#include "../file_format_xml.h"
#include "../file_import_export.h"


void XmlElementWriter::write(const MapCoordVector& coords)
{
	namespace literal = XmlStreamLiteral;
	
	writeAttribute(literal::count, coords.size());
	
	if (XMLFileFormat::active_version < 6 || xml.autoFormatting())
	{
		// XMAP files and old format: syntactically rich output
		for (auto& coord : coords)
			coord.save(xml);
	}
	else
	{
		// Default: efficient plain text format
		//   Note that it is more efficient to concatenate the data
		// than to call writeCharacters() multiple times.
		QString data;
		data.reserve(coords.size() * 16);
		for (auto& coord : coords)
			data.append(coord.toString());
		xml.writeCharacters(data);
	}
}


void XmlElementReader::read(MapCoordVector& coords)
{
	namespace literal = XmlStreamLiteral;
	
	coords.clear();
	
	const auto num_coords = attribute<unsigned int>(literal::count);
	coords.reserve(std::min(num_coords, 500000u));
	
	try
	{
		for( xml.readNext(); xml.tokenType() != QXmlStreamReader::EndElement; xml.readNext() )
		{
			const QXmlStreamReader::TokenType token = xml.tokenType();
			if (xml.error() || token == QXmlStreamReader::EndDocument)
			{
				throw FileFormatException(ImportExport::tr("Could not parse the coordinates."));
			}
			else if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				try
				{
					while (text.length())
					{
						coords.emplace_back(text);
					}
				}
				catch (std::exception& e)
				{
					Q_UNUSED(e)
					qDebug("Could not parse the coordinates: %s", e.what());
					throw FileFormatException(ImportExport::tr("Could not parse the coordinates."));
				}
			}
			else if (token == QXmlStreamReader::StartElement)
			{
				if (xml.name() == literal::coord)
				{
					coords.emplace_back(MapCoord::load(xml));
				}
				else
				{
					xml.skipCurrentElement();
				}
			}
			// otherwise: ignore element
		}
	}
	catch (std::range_error &e)
	{
		throw FileFormatException(MapCoord::tr(e.what()));
	}
	
	if (coords.size() != num_coords)
	{
		throw FileFormatException(ImportExport::tr("Expected %1 coordinates, found %2."));
	}
}
