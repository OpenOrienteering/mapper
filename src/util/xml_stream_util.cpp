/*
 *    Copyright 2013-2017 Kai Pastor
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

#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>

#include <QtGlobal>
#include <QBuffer>
#include <QByteArray>
#include <QChar>
#include <QCoreApplication>
#include <QIODevice>
#include <QScopedValueRollback>
#include <QTextCodec>
// IWYU pragma: no_include <qxmlstream.h>

#include "core/map_coord.h"
#include "fileformats/file_format.h"
#include "fileformats/file_import_export.h"
#include "fileformats/xml_file_format.h"


namespace OpenOrienteering {

void writeLineBreak(QXmlStreamWriter& xml)
{
	if (!xml.autoFormatting())
	{
		static const QString linebreak = QLatin1String{"\n"};
		xml.writeCharacters(linebreak);
	}
}



//### XmlRecoveryHelper ###

bool XmlRecoveryHelper::operator() ()
{
	auto offending_index_64 = xml.characterOffset();
	auto offending_message = xml.errorString();
	auto stream = xml.device();
	
	if (xml.hasError()
	    && offending_index_64 <= std::numeric_limits<int>::max()
	    && offending_index_64 > recovery_start
	    && stream
	    && stream->seek(0))
	{
		// Get the whole input
		auto raw_data = stream->readAll();
		
		// Determine the encoding
		QTextCodec* codec = nullptr;
		for (QXmlStreamReader probe(raw_data); !probe.atEnd(); probe.readNext())
		{
			if (probe.tokenType() == QXmlStreamReader::StartDocument)
			{
				codec = QTextCodec::codecForName(probe.documentEncoding().toLatin1());
				break;
			}
		}
		if (Q_UNLIKELY(!codec))
		{
			return false;
		}
		
		// Convert to QString, and release the raw data
		auto data = codec->toUnicode(raw_data);
		raw_data = {};
		
		// Loop until the current element is fixed
		for (auto index = int(offending_index_64 - 1); xml.hasError(); index = int(xml.characterOffset() - 1))
		{
			// Cf. https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char
			// and QXmlStreamReaderPrivate::scanUntil,
			// http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/xml/qxmlstream.cpp?h=5.6.2#n961
			auto offending_char = static_cast<unsigned int>(data.at(index).unicode());
			if (xml.error() != QXmlStreamReader::NotWellFormedError
			    || offending_char == 0x9     // horizontal tab
			    || offending_char == 0xa  // line feed
			    || offending_char == 0xd  // carriage return
			    || (offending_char >= 0x20 && offending_char <= 0xfffd)
			    || offending_char > QChar::LastValidCodePoint)
			{
				// Another error than an invalid character
				break;
			}
			
			// Remove the offending character
			data.remove(index, 1);
			
			// Read again to the element start
			xml.clear();
			xml.addData(data);
			while (!xml.hasError() && xml.characterOffset() < recovery_start)
			{
				xml.readNext();
			}
			Q_ASSERT(!xml.hasError());
			
			// Read the current element completely
			xml.skipCurrentElement();
		}
		
		// Restore the XML stream state with the modified data.
		// We must use a QIODevice again, for later recovery attemts.
		auto buffer = new QBuffer(stream); // buffer will live as long as the original stream
		buffer->setData(codec->fromUnicode(data));
		buffer->open(QIODevice::ReadOnly);
		
		xml.clear();
		xml.setDevice(buffer);
		while (!xml.atEnd() && xml.characterOffset() < recovery_start)
		{
			xml.readNext();
		}
		
		if (xml.characterOffset() == recovery_start)
		{
			return true;
		}
		else if (!xml.hasError())
		{
			xml.raiseError(offending_message);
		}
	}
	return false;
}



//### XmlElementWriter ###

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



//### XmlElementReader ###

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
				throw FileFormatException(::OpenOrienteering::ImportExport::tr("Could not parse the coordinates."));
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
					throw FileFormatException(::OpenOrienteering::ImportExport::tr("Could not parse the coordinates."));
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
		throw FileFormatException(::OpenOrienteering::MapCoord::tr(e.what()));
	}
	
	if (coords.size() != num_coords)
	{
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("Expected %1 coordinates, found %2."));
	}
}


void XmlElementReader::readForText(MapCoordVector& coords)
{
	namespace literal = XmlStreamLiteral;
	
	coords.clear();
	coords.reserve(2);
	
	const auto num_coords = attribute<unsigned int>(literal::count);
	
	QScopedValueRollback<MapCoord::BoundsOffset> offset{MapCoord::boundsOffset()};
	
	try
	{
		for( xml.readNext(); xml.tokenType() != QXmlStreamReader::EndElement; xml.readNext() )
		{
			const QXmlStreamReader::TokenType token = xml.tokenType();
			if (xml.error() || token == QXmlStreamReader::EndDocument)
			{
				throw FileFormatException(::OpenOrienteering::ImportExport::tr("Could not parse the coordinates."));
			}
			else if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				try
				{
					while (text.length())
					{
						if (coords.size() == 1)
						{
							// Don't apply an offset to text box size.
							offset.commit();
							MapCoord::boundsOffset().reset(false);
						}
						coords.emplace_back(text);
					}
				}
				catch (std::exception& e)
				{
					Q_UNUSED(e)
					qDebug("Could not parse the coordinates: %s", e.what());
					throw FileFormatException(::OpenOrienteering::ImportExport::tr("Could not parse the coordinates."));
				}
			}
			else if (token == QXmlStreamReader::StartElement)
			{
				if (xml.name() == literal::coord)
				{
					if (coords.size() == 1)
					{
						// Don't apply an offset to text box size.
						offset.commit();
						MapCoord::boundsOffset().reset(false);
					}
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
		throw FileFormatException(::OpenOrienteering::MapCoord::tr(e.what()));
	}
	
	if (coords.size() != num_coords)
	{
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("Expected %1 coordinates, found %2."));
	}
}



}  // namespace OpenOrienteering
