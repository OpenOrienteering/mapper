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

#include "coord_xml_t.h"

#include <algorithm>

#include <QtTest>

#include "fileformats/xml_file_format.h"
#include "util/xml_stream_util.h"


namespace literal
{
	static const QLatin1String x("x");
	static const QLatin1String y("y");
	static const QLatin1String flags("flags");
}


namespace OpenOrienteering {

void CoordXmlTest::initTestCase()
{
	proto_coord = MapCoord::fromNative(12345, -6789, MapCoord::DashPoint);
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
	for (const auto& coord : coords)
	{
		xml.writeStartElement(XmlStreamLiteral::coord);
		xml.writeAttribute(literal::x, QString::number(coord.nativeX()));
		xml.writeAttribute(literal::y, QString::number(coord.nativeY()));
		xml.writeAttribute(literal::flags, QString::number(coord.flags()));
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
	for (const auto& coord : coords)
	{
		stream << coord.nativeX() << ' '
		       << coord.nativeY() << ' '
		       << coord.flags() << ';';
	}
	stream.flush();
	xml.writeCharacters(data);
}

void CoordXmlTest::writeHumanReadableStream()
{
	namespace literal = XmlStreamLiteral;
	
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
	QString data;
	data.reserve(coords.size() * 16);
	
	static char encoded[11] = "0123456789";
	
	const std::size_t buf_size = 32;
	char buffer[buf_size];
	
	for (const auto& coord : coords)
	{
		int j = buf_size - 1;
		
		buffer[j] = ';';
		--j;
		
		qint32 tmp = coord.flags();
		char sign = 0;
		if (tmp != 0)
		{
			if (tmp < 0)
			{
				sign = '-';
				tmp = -tmp; // TODO catch -MAX
			}
			do
			{
				buffer[j] = encoded[tmp % 10];
				tmp = tmp / 10;
				--j;
			}
			while (tmp != 0);
			if (sign)
			{
				buffer[j] = sign;
				--j;
				sign = 0;
			}
			
			buffer[j] = ' ';
			--j;
		}
		
		tmp = coord.nativeY();
		if (tmp < 0)
		{
			sign = '-';
			tmp = -tmp; // TODO catch -MAX
		}
		do
		{
			buffer[j] = encoded[tmp % 10];
			tmp = tmp / 10;
			--j;
		}
		while (tmp != 0);
		if (sign)
		{
			buffer[j] = sign;
			--j;
		}
		
		buffer[j] = ' ';
		--j;
		
		tmp = coord.nativeX();
		sign = 0;
		if (tmp < 0)
		{
			sign = '-';
			tmp = -tmp; // TODO catch -MAX
		}
		do
		{
			buffer[j] = encoded[tmp % 10];
			tmp = tmp / 10;
			--j;
		}
		while (tmp != 0);
		if (sign)
		{
			buffer[j] = sign;
			--j;
		}
		
		++j;
		data.append(QString::fromUtf8(buffer+j, buf_size-j));
	}
	
	xml.writeCharacters(data);
}

void CoordXmlTest::writeHumanReadableString()
{
	namespace literal = XmlStreamLiteral;
	
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
	for (const auto& coord : coords)
	{
		static const std::size_t buf_size = 3*sizeof(qint64);
		static char encoded[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		                          "abcdefghijklmnopqrstuvwxyz"
		                          "0123456789=/";
		QByteArray buffer((int)buf_size, 0);
		auto tmp = coord.xp;
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
		
		tmp = coord.yp;
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
		
		tmp = coord.fp;
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
	namespace literal = XmlStreamLiteral;
	
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


void CoordXmlTest::writeFastImplementation_data()
{
	common_data();
}

void CoordXmlTest::writeFastImplementation()
{
	buffer.open(QBuffer::ReadWrite);
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(false);
	xml.writeStartDocument();
	
	XMLFileFormat::active_version = 6; // Activate fast text format.
	XmlElementWriter element(xml, QLatin1String("root"));
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	QBENCHMARK
	{
		element.write(coords);
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
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement(QString::fromLatin1("root"));
		xml.writeCharacters(QString::fromLatin1(" ")); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement(QString::fromLatin1("coords"));
		writeXml_implementation(coords, xml);
		xml.writeEndElement(/* coords */);
		
		xml.setDevice(nullptr);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	xml.readNextStartElement();
	QCOMPARE(xml.name().toString(), QString::fromLatin1("root"));
	
	bool failed = false;
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		
		xml.readNextStartElement();
		if (xml.name() != QLatin1String("coords"))
		{
			failed = true;
			break;
		}
		
		while(xml.readNextStartElement())
		{
			if (xml.name() != XmlStreamLiteral::coord)
			{
				failed = true;
				break;
			}
			
			XmlElementReader element(xml);
			auto x = element.attribute<qint32>(literal::x);
			auto y = element.attribute<qint32>(literal::y);
			auto flags = MapCoord::Flags(element.attribute<unsigned int>(literal::flags));
			coords.push_back(MapCoord::fromNative(x, y, flags));
		}
	}
	
	QVERIFY(!failed);
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
	namespace literal = XmlStreamLiteral;
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement(QString::fromLatin1("root"));
		xml.writeCharacters(QString{}); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement(QString::fromLatin1("coords"));
		// Using the more efficient string implementation.
		writeHumanReadableString_implementation(coords, xml);
		xml.writeEndElement();
		
		xml.setDevice(nullptr);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	xml.readNextStartElement();
	QCOMPARE(xml.name().toString(), QString::fromLatin1("root"));
	
	bool failed = false;
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		
		xml.readNextStartElement();
		if (xml.name() != QLatin1String("coords"))
		{
			failed = true;
			break;
		}
		
		for( xml.readNext();
		     xml.tokenType() != QXmlStreamReader::EndElement;
		     xml.readNext() )
		{
			if (xml.error())
			{
				qDebug() << xml.errorString();
				failed = true;
				break;
			}
			
			const QXmlStreamReader::TokenType token = xml.tokenType();
			if (token == QXmlStreamReader::EndDocument)
			{
				failed = true;
				break;
			}
			
			if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				QString data = QString::fromRawData(text.constData(), text.length());
				QTextStream stream(&data, QIODevice::ReadOnly);
				stream.setIntegerBase(10);
				while (!stream.atEnd())
				{
					qint32 x, y;
					MapCoord::Flags::Int flags = 0;
					char separator;
					stream >> x >> y >> separator;
					if (separator != ';')
					{
						stream >> flags >> separator;
					}
					coords.push_back(MapCoord::fromNative(x, y, MapCoord::Flags{flags}));
				}
				if (stream.status() == QTextStream::ReadCorruptData)
				{
					failed = true;
					break;
				}
			}
			// otherwise: ignore element
		}
		
	}
	
	QVERIFY(!failed);
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}


void CoordXmlTest::readHumanReadableStringRef_data()
{
	common_data();
}

void CoordXmlTest::readHumanReadableStringRef()
{
	namespace literal = XmlStreamLiteral;
	
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement(QString::fromLatin1("root"));
		xml.writeCharacters(QString{}); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement(QString::fromLatin1("coords"));
		// Using the more efficient string implementation.
		writeHumanReadableString_implementation(coords, xml);
		xml.writeEndElement();
		
		xml.setDevice(nullptr);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	xml.readNextStartElement();
	QCOMPARE(xml.name().toString(), QString::fromLatin1("root"));
	
	bool failed = false;
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		
		xml.readNextStartElement();
		if (xml.name() != QLatin1String("coords"))
		{
			failed = true;
			break;
		}
		
		for( xml.readNext();
		     xml.tokenType() != QXmlStreamReader::EndElement;
		     xml.readNext() )
		{
			if (xml.error())
			{
				qDebug() << xml.errorString();
				failed = true;
				break;
			}
			
			const QXmlStreamReader::TokenType token = xml.tokenType();
			if (token == QXmlStreamReader::EndDocument)
			{
				failed = true;
				break;
			}
			
			if (token == QXmlStreamReader::Characters && !xml.isWhitespace())
			{
				QStringRef text = xml.text();
				auto data = text.constData();
				while (text.length() > 0)
				{
					int len = text.length();
					int i = 0;
					qint64 x64 = data[0].unicode();
					if (x64 == '-')
					{
						x64 = '0' - data[1].unicode();
						for (i = 2; i != len; ++i)
						{
							auto c = data[i].unicode();
							if (c < '0' || c > '9')
								break;
							else
								x64 = 10*x64 + '0' - c;
						}
					}
					else if (x64 >= '0' && x64 <= '9')
					{
						x64 -= '0';
						for (i = 1; i != len; ++i)
						{
							auto c = data[i].unicode();
							if (c < '0' || c > '9')
								break;
							else
								x64 = 10*x64 + c - '0';
						}
					}
					
					++i;
					if (Q_UNLIKELY(i+1 >= len))
					{
						failed = true;
						break;
					}
					
					qint64 y64 = data[i].unicode();
					if (y64 == '-')
					{
						++i;
						y64 = '0' - data[i].unicode();
						for (++i; i != len; ++i)
						{
							auto c = data[i].unicode();
							if (c < '0' || c > '9')
								break;
							else
								y64 = 10*y64 + '0' - c;
						}
					}
					else if (y64 >= '0' && y64 <= '9')
					{
						y64 -= '0';
						for (++i; i != len; ++i)
						{
							auto c = data[i].unicode();
							if (c < '0' || c > '9')
								break;
							else
								y64 = 10*y64 + c - '0';
						}
					}
					
					coords.push_back(MapCoord::fromNative64(x64, y64));
					
					if (i < len && data[i] == QChar::Space)
					{
						++i;
						if (Q_UNLIKELY(i == len))
						{
							failed = true;
							break;
						}
						
						// there are no negative flags
						int fp = data[i].unicode() - '0';
						for (++i; i < len; ++i)
						{
							auto c = data[i].unicode();
							if (c < '0' || c > '9')
								break;
							else
								fp = 10*fp + c - '0';
						}
						coords.back().setFlags(fp);
					}
					
					if (Q_UNLIKELY(i >= len || data[i] != QLatin1Char{';'}))
					{
						failed = true;
						break;
					}
					
					++i;
					text = text.mid(i, len-i);
				}
				if (failed)
					break;
			}
			// otherwise: ignore element
		}
		
	}
	
	QVERIFY(!failed);
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
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		xml.writeStartElement(QString::fromLatin1("root"));
		xml.writeCharacters(QString{}); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		xml.writeStartElement(QString::fromLatin1("coords"));
		writeCompressed_implementation(coords, xml);
		xml.writeEndElement();
		
		xml.setDevice(nullptr);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	
	xml.readNextStartElement();
	QCOMPARE(xml.name().toString(), QString::fromLatin1("root"));
	
	bool failed = false;
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		xml.readNextStartElement();
		if (xml.name() != QLatin1String("coords"))
		{
			failed = true;
			break;
		}
		
		for( xml.readNext();
		     xml.tokenType() != QXmlStreamReader::EndElement;
		     xml.readNext() )
		{
			if (xml.error())
			{
				qDebug() << xml.errorString();
				failed = true;
				break;
			}
			
			const QXmlStreamReader::TokenType token = xml.tokenType();
			if (token == QXmlStreamReader::EndDocument)
			{
				failed = true;
				break;
			}
			
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
					decltype(MapCoord::xp) x = 0;
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
					
					decltype(MapCoord::yp) y = 0;
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
					
					int f = 0;
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
							f = -f;
							break;
						}
						else
						{
							c = 0;
						}
						f = f | (c << i );
						i += 6;
					}
					
					coords.back().xp = x;
					coords.back().yp = y;
					coords.back().fp = decltype(MapCoord::fp)(f);
				}
			}
			// otherwise: ignore element
		}
	}
	
	QVERIFY(!failed);
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}


void CoordXmlTest::readFastImplementation_data()
{
	common_data();
}

void CoordXmlTest::readFastImplementation()
{
	QFETCH(int, num_coords);
	MapCoordVector coords(num_coords, proto_coord);
	
	buffer.buffer().truncate(0);
	QBuffer header;
	{
		QXmlStreamWriter xml(&header);
		
		header.open(QBuffer::ReadWrite);
		xml.setAutoFormatting(false);
		xml.writeStartDocument();
		
		XMLFileFormat::active_version = 6; // Activate fast text format.
		
		xml.writeStartElement(QString::fromLatin1("root"));
		xml.writeCharacters(QString{}); // flush root start element
		
		buffer.open(QBuffer::ReadWrite);
		xml.setDevice(&buffer);
		{
			XmlElementWriter element(xml, QLatin1String("coords"));
			element.write(coords);
		}
		
		xml.setDevice(nullptr);
		
		buffer.close();
		header.close();
	}
	
	header.open(QBuffer::ReadOnly);
	buffer.open(QBuffer::ReadOnly);
	QXmlStreamReader xml;
	xml.addData(header.buffer());
	xml.readNextStartElement();
	QCOMPARE(xml.name().toString(), QString::fromLatin1("root"));
	
	bool failed = false;
	QBENCHMARK
	{
		// benchmark iteration overhead
		coords.clear();
		xml.addData(buffer.data());
		
		xml.readNextStartElement();
		if (xml.name() != QLatin1String("coords"))
		{
			failed = true;
			break;
		}
		
		XmlElementReader element(xml);
		element.read(coords);
	}
		
	QVERIFY(!failed);
	QCOMPARE((int)coords.size(), num_coords);
	QVERIFY(compare_all(coords, proto_coord));
	
	header.close();
	buffer.close();
}


bool CoordXmlTest::compare_all(MapCoordVector& coords, MapCoord& expected) const
{
	return std::all_of(begin(coords), end(coords), [expected](const MapCoord& coord){ return coord == expected; });
}


}  // namespace OpenOrienteering

QTEST_GUILESS_MAIN(OpenOrienteering::CoordXmlTest)
