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

#ifndef _OPENORIENTEERING_XML_STREAM_UTIL_H_
#define _OPENORIENTEERING_XML_STREAM_UTIL_H_

#include <QDebug>
#include <QRectF>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


class ScopedXmlWriterElement
{
public:
	ScopedXmlWriterElement(QXmlStreamWriter& xml, const QString& element_name);
	~ScopedXmlWriterElement();
	void writeAttribute(const QString& qualifiedName, const char* value);
	void writeAttribute(const QString& qualifiedName, const QString& value);
	void writeAttribute(const QString& qualifiedName, const qreal value, int precision = 6);
	void writeAttribute(const QString& qualifiedName, const int value);
	void writeAttribute(const QString& qualifiedName, const unsigned int value);
	void writeAttribute(const QString& qualifiedName, bool value);
	void write(const QRectF& area, int precision = 6);
	void write(const QSizeF& size, int precision = 6);
	
private:
	QXmlStreamWriter& xml;
};


class ScopedXmlReaderElement
{
public:
	ScopedXmlReaderElement(QXmlStreamReader& xml);
	~ScopedXmlReaderElement();
	void readAttribute(const QString& qualifiedName, QString& value);
	void readAttribute(const QString& qualifiedName, float& value);
	void readAttribute(const QString& qualifiedName, int& value);
	void readAttribute(const QString& qualifiedName, unsigned int& value);
	void readAttribute(const QString& qualifiedName, bool& value);
	void read(QRectF& area);
	void read(QSizeF& size);
	
private:
	QXmlStreamReader& xml;
};


inline
ScopedXmlWriterElement::ScopedXmlWriterElement(QXmlStreamWriter& xml, const QString& element_name)
 : xml(xml)
{
	xml.writeStartElement(element_name);
}

inline
ScopedXmlWriterElement::~ScopedXmlWriterElement()
{
	xml.writeEndElement();
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, const char* value)
{
	xml.writeAttribute(qualifiedName, QString(value));
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, const QString& value)
{
	xml.writeAttribute(qualifiedName, value);
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, const qreal value, int precision)
{
	xml.writeAttribute(qualifiedName, QString::number(value, 'f', precision));
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, const int value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, const unsigned int value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void ScopedXmlWriterElement::writeAttribute(const QString& qualifiedName, bool value)
{
	if (value)
		xml.writeAttribute(qualifiedName, "true");
}

inline
void ScopedXmlWriterElement::write(const QRectF& area, int precision)
{
	writeAttribute( "left",   area.left(),   precision );
	writeAttribute( "top",    area.top(),    precision );
	writeAttribute( "width",  area.width(),  precision );
	writeAttribute( "height", area.height(), precision );
}

inline
void ScopedXmlWriterElement::write(const QSizeF& size, int precision)
{
	writeAttribute( "width",  size.width(),  precision );
	writeAttribute( "height", size.height(), precision );
}


inline
ScopedXmlReaderElement::ScopedXmlReaderElement(QXmlStreamReader& xml)
 : xml(xml)
{
}

inline
ScopedXmlReaderElement::~ScopedXmlReaderElement()
{
	if (!xml.isEndElement())
		xml.skipCurrentElement();
}

inline
void ScopedXmlReaderElement::readAttribute(const QString& qualifiedName, QString& value)
{
	if (xml.attributes().hasAttribute(qualifiedName))
		value = xml.attributes().value(qualifiedName).toString();
}

inline
void ScopedXmlReaderElement::readAttribute(const QString& qualifiedName, float& value)
{
	if (xml.attributes().hasAttribute(qualifiedName))
		value = xml.attributes().value(qualifiedName).toString().toFloat();
}

inline
void ScopedXmlReaderElement::readAttribute(const QString& qualifiedName, int& value)
{
	if (xml.attributes().hasAttribute(qualifiedName))
		value = xml.attributes().value(qualifiedName).toString().toInt();
}

inline
void ScopedXmlReaderElement::readAttribute(const QString& qualifiedName, unsigned int& value)
{
	if (xml.attributes().hasAttribute(qualifiedName))
		value = xml.attributes().value(qualifiedName).toString().toUInt();
}

inline
void ScopedXmlReaderElement::readAttribute(const QString& qualifiedName, bool& value)
{
		value = xml.attributes().value(qualifiedName) == "true";
}

inline
void ScopedXmlReaderElement::read(QRectF& area)
{
	QXmlStreamAttributes attributes = xml.attributes();
	area.setLeft(attributes.value("left").toString().toDouble());
	area.setTop(attributes.value("top").toString().toDouble());
	area.setWidth(attributes.value("width").toString().toDouble());
	area.setHeight(attributes.value("height").toString().toDouble());
}

inline
void ScopedXmlReaderElement::read(QSizeF& size)
{
	QXmlStreamAttributes attributes = xml.attributes();
	size.setWidth(attributes.value("width").toString().toDouble());
	size.setHeight(attributes.value("height").toString().toDouble());
}


#endif
