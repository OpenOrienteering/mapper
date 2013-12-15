/*
 *    Copyright 2013 Kai Pastor
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

#include "coord_xml_t.h"


void CoordXmlTest::initTestCase()
{
	proto_coord = MapCoord::fromRaw(12345, -6789);
	proto_coord.setFlags(3);
	
	buffer.buffer().reserve(5000000);
}

void CoordXmlTest::common_data()
{
	QTest::addColumn<int>("num_coords");
	QTest::newRow("num_coords = 1") << 1;
	QTest::newRow("num_coords = 5") << 5;
	QTest::newRow("num_coords = 50") << 50;
	QTest::newRow("num_coords = 500") << 500;
	QTest::newRow("num_coords = 5000") << 5000;
	QTest::newRow("num_coords = 50000") << 50000;
}

void CoordXmlTest::writeXml_data()
{
	common_data();
}

void CoordXmlTest::writeXml_implementation(MapCoordVector& coords, QXmlStreamWriter& xml)
{
	namespace literal = MapCoordLiteral;
	
	for (MapCoordVector::iterator coord = coords.begin(), end = coords.end();
	     coord != end;
	     ++coord)
	{
		xml.writeStartElement(literal::coord);
		xml.writeAttribute(literal::x, QString::number(coord->rawX()));
		xml.writeAttribute(literal::y, QString::number(coord->rawY()));
		xml.writeAttribute(literal::flags, QString::number(coord->getFlags()));
		xml.writeEndElement();
	}
}

void CoordXmlTest::writeXml()
{
	buffer.open(QBuffer::ReadWrite);
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(false);
	xml.writeStartDocument();
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	QBENCHMARK
	{
		writeXml_implementation(coords, xml);
	} 
	
	xml.writeEndDocument();
	buffer.close();
}


void CoordXmlTest::writeHumanReadableStream_data()
{
	common_data();
}

void CoordXmlTest::writeHumanReadableStream_implementation(MapCoordVector& coords, QXmlStreamWriter& xml)
{
	QString data;
	data.reserve(coords.size() * 14);
	QTextStream stream(&data);
	for (MapCoordVector::iterator coord = coords.begin(), end = coords.end();
	     coord != end;
	     ++coord)
	{
		stream << coord->rawX() << ' '
		       << coord->rawY() << ' '
		       << coord->getFlags() << ';';
	}
	stream.flush();
	xml.writeCharacters(data);
}

void CoordXmlTest::writeHumanReadableStream()
{
	namespace literal = MapCoordLiteral;
	
	buffer.open(QBuffer::ReadWrite);
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(false);
	xml.writeStartDocument();
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	QBENCHMARK
	{
		writeHumanReadableStream_implementation(coords, xml);
	}
	
	xml.writeEndDocument();
	buffer.close();
}


void CoordXmlTest::writeHumanReadableString_data()
{
	common_data();
}

void CoordXmlTest::writeHumanReadableString_implementation(MapCoordVector& coords, QXmlStreamWriter& xml)
{
	for (MapCoordVector::iterator coord = coords.begin(), end = coords.end();
	     coord != end;
	     ++coord)
	{
		QString s = QString::number(coord->rawX());
		s.append(' ');
		s.append(QString::number(coord->rawY()));
		s.append(' ');
		s.append(QString::number(coord->getFlags()));
		xml.writeCharacters(s);
	}
}

void CoordXmlTest::writeHumanReadableString()
{
	namespace literal = MapCoordLiteral;
	
	buffer.open(QBuffer::ReadWrite);
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(false);
	xml.writeStartDocument();
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	QBENCHMARK
	{
		writeHumanReadableString_implementation(coords, xml);
	}
	
	xml.writeEndDocument();
	buffer.close();
}


void CoordXmlTest::writeCompressed_data()
{
	common_data();
}

void CoordXmlTest::writeCompressed_implementation(MapCoordVector& coords, QXmlStreamWriter& xml)
{
	QString data;
	data.reserve(coords.size() * 14);
	QTextStream stream(&data);
	for (MapCoordVector::iterator coord = coords.begin(), end = coords.end();
	     coord != end;
	     ++coord)
	{
		static const std::size_t buf_size = 3*sizeof(qint64);
		static char encoded[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		                          "abcdefghijklmnopqrstuvwxyz"
		                          "0123456789=/";
		QByteArray buffer((int)buf_size, 0);
		qint64 tmp = coord->internalX();
		char sign = '+';
		if (tmp < 0)
		{
			sign = '-';
			tmp = -tmp; // TODO catch -MAX
		}
		int j = 0;
		while (tmp != 0)
		{
			buffer[j] = encoded[tmp & 63];
			tmp = tmp >> 6;
			++j;
		}
		buffer[j] = sign;
		++j;
		
		tmp = coord->internalY();
		sign = '+';
		if (tmp < 0)
		{
			sign = '-';
			tmp = -tmp; // TODO catch -MAX
		}
		while (tmp != 0)
		{
			buffer[j] = encoded[tmp & 63];
			tmp = tmp >> 6;
			++j;
		}
		buffer[j] = sign;
		++j;
		
		stream << QString::fromUtf8(buffer, j);
	}
	stream.flush();
	xml.writeCharacters(data);
}

void CoordXmlTest::writeCompressed()
{
	namespace literal = MapCoordLiteral;
	
	buffer.open(QBuffer::ReadWrite);
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(false);
	xml.writeStartDocument();
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	QBENCHMARK
	{
		writeCompressed_implementation(coords, xml);
	}
	
	xml.writeEndDocument();
	buffer.close();
}


void CoordXmlTest::readXml_data()
{
	common_data();
}

void CoordXmlTest::readXml()
{
	namespace literal = MapCoordLiteral;
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement("root");
		xml.writeStartElement("coords");
		xml.writeCharacters(""); // flush coords start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		writeXml_implementation(coords, xml);
		xml.writeCharacters(" "); // flush coords start element
		
		xml.setDevice(NULL);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	Q_ASSERT(xml.readNextStartElement() && xml.name() == "root");
	Q_ASSERT(xml.readNextStartElement() && xml.name() == "coords");
	
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		
		while(xml.readNextStartElement())
		{
			Q_ASSERT(xml.name() == literal::coord);
			
			XmlElementReader element(xml);
			qint64 x = element.attribute<qint64>(literal::x);
			qint64 y = element.attribute<qint64>(literal::y);
			MapCoord coord = MapCoord::fromRaw(x, y);
			
			int flags = element.attribute<int>(literal::flags);
			if (flags)
			{
				coord.setFlags(flags);
			}
			
			coords.push_back(coord);
		}
	}
		
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}


void CoordXmlTest::readHumanReadableStream_data()
{
	common_data();
}

void CoordXmlTest::readHumanReadableStream()
{
	namespace literal = MapCoordLiteral;
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement("root");
		xml.writeCharacters(""); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement("coords");
		writeHumanReadableStream_implementation(coords, xml);
		xml.writeEndElement();
		
		xml.setDevice(NULL);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	Q_ASSERT(xml.readNextStartElement() && xml.name() == "root");
	
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		Q_ASSERT(xml.readNextStartElement() && xml.name() == "coords");
		
		for( xml.readNext();
		     xml.tokenType() != QXmlStreamReader::EndElement;
		     xml.readNext() )
		{
			if (xml.error()) qDebug() << xml.errorString();
			Q_ASSERT(!xml.error());
			
			const QXmlStreamReader::TokenType token = xml.tokenType();
			Q_ASSERT(token != QXmlStreamReader::EndDocument);
			
			if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				QString data = QString::fromRawData(text.constData(), text.length());
				QTextStream stream(&data, QIODevice::ReadOnly);
				stream.setIntegerBase(10);
				char separator;
				while (!stream.atEnd())
				{
					coords.resize(coords.size() + 1);
					stream >> coords.back() >> separator;
				}
				Q_ASSERT (stream.status() != QTextStream::ReadCorruptData);
			}
			// otherwise: ignore element
		}
		
	}
	
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}

void CoordXmlTest::readCompressed_data()
{
	common_data();
}

void CoordXmlTest::readCompressed()
{
	namespace literal = MapCoordLiteral;
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement("root");
		xml.writeCharacters(""); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement("coords");
		writeCompressed_implementation(coords, xml);
		xml.writeEndElement();
		
		xml.setDevice(NULL);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	Q_ASSERT(xml.readNextStartElement() && xml.name() == "root");
	
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		Q_ASSERT(xml.readNextStartElement() && xml.name() == "coords");
		
		for( xml.readNext();
		     xml.tokenType() != QXmlStreamReader::EndElement;
		     xml.readNext() )
		{
			if (xml.error()) qDebug() << xml.errorString();
			Q_ASSERT(!xml.error());
			
			const QXmlStreamReader::TokenType token = xml.tokenType();
			Q_ASSERT(token != QXmlStreamReader::EndDocument);
			
			if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				QByteArray data = QString::fromRawData(text.constData(), text.length()).toLatin1();
				int pos = 0;
				int len = data.length();
				while (pos < len)
				{
					coords.resize(coords.size() + 1);
					quint8 c;
					qint64 x = 0;
					int i = 0;
					
					const int len = data.length();
					while (pos < len)
					{
						c = data[pos];
						pos++;
						
						if (c >= 'A' && c <= 'Z')
						{
						c =  c - 'A';
						}
						else if (c >= 'a' && c <= 'z')
						{
							c = (c - 'a') + 26;
						}
						else if (c >= '0' && c <= '9')
						{
							c = (c - '0') + 52;
						}
						else if (c == '/')
						{
							c = 63;
						}
						else if (c == '=')
						{
							c = 62;
						}
						else if (c == '+')
						{
							break;
						}
						else if (c == '-')
						{
							x = -x;
							break;
						}
						else
						{
							c = 0;
						}
						x = x | (c << i );
						i += 6;
					}
					
					qint64 y = 0;
					i = 0;
					while (pos < len)
					{
						c = data[pos];
						pos++;
						
						if (c >= 'A' && c <= 'Z')
						{
						c =  c - 'A';
						}
						else if (c >= 'a' && c <= 'z')
						{
							c = (c - 'a') + 26;
						}
						else if (c >= '0' && c <= '9')
						{
							c = (c - '0') + 52;
						}
						else if (c == '/')
						{
							c = 63;
						}
						else if (c == '=')
						{
							c = 62;
						}
						else if (c == '+')
						{
							break;
						}
						else if (c == '-')
						{
							y = -y;
							break;
						}
						else
						{
							c = 0;
						}
						y = y | (c << i );
						i += 6;
					}
					coords.back().x = x;
					coords.back().y = y;
				}
			}
			// otherwise: ignore element
		}
	}
	
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}


bool CoordXmlTest::compare_all(MapCoordVector& coords, MapCoord& expected) const
{
	for (MapCoordVector::iterator coord = coords.begin(), end = coords.end();
	     coord != end;
	     ++coord)
	{
		if (*coord != expected)
			return false;
	}
	return true;
}


QTEST_MAIN(CoordXmlTest)
