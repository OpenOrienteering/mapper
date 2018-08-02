/*
 *    Copyright 2013-2018 Kai Pastor
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

#ifndef OPENORIENTEERING_XML_STREAM_UTIL_H
#define OPENORIENTEERING_XML_STREAM_UTIL_H

#include <QtGlobal>
#include <QChar>
#include <QHash>
#include <QLatin1Char>
#include <QLatin1String>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map_coord.h"

class QRectF;
class QSizeF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {


/**
 * Writes a line break to the XML stream unless auto formatting is active.
 */
void writeLineBreak(QXmlStreamWriter& xml);



/**
 * This class provides recovery from invalid characters in an XML stream.
 * 
 * Some characters are not allowed in well-formed XML 1.0 (cf.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char). While QXmlStreamWriter
 * will not complain when writing such characters, QXmlStreamReader will raise
 * a NotWellFormedError. This class will remove offending characters from the
 * input and reset the stream reader to the state it had when the helper object
 * was initialized.
 * 
 * In a single recovery attempt, the utility tries to handle all offending
 * characters from the element for wich the tool was constructed. For each
 * offending character, the whole XML data is parsed again from the start.
 * That's why multiple corrections may take a long time to run.
 * 
 * The XML stream must be based on a QIODevice which supports QIODevice::seek.
 * 
 * Synopsis:
 * 
 *     XmlRecoveryHelper recovery(xml);
 *     auto text = xml.readElementText();
 *     if (xml.hasError() && recovery())
 *     {
 *         addWarning(tr("Some invalid characters had to be removed.");
 *         text = xml.readElementText();
 *     }
 */
class XmlRecoveryHelper
{
public:
	/**
	 * Constructs a new recovery helper for the given xml stream.
	 * 
	 * Captures the current position in the XML stream (QXmlStreamReader::characterOffset()).
	 */
	XmlRecoveryHelper(QXmlStreamReader& xml) : xml (xml), recovery_start {xml.characterOffset()} {}
	
	/**
	 * Checks the stream for an error which this utility can handle,
	 * applies corrections, and resets the stream.
	 * 
	 * If this operator returns false if either there was a different type of
	 * error, or if recovery failed. If it returns true, the stream was modified
	 * in order to fix the errors which are handled by this utility, and a new
	 * attempt can be made to parse the remainder of the stream.
	 */
	bool operator() ();
	
private:
	QXmlStreamReader& xml;
	const qint64 recovery_start;
};



/**
 * The XmlElementWriter helps to construct a single element in an XML document.
 * 
 * It starts a new element on a QXmlStreamWriter when it is constructed,
 * and it writes the end tag when it is destructed. After construction, but
 * before (child) elements are created on the QXmlStreamWriter, it offers
 * convenient functions for writing named attributes of common types.
 * 
 * Typical use:
 * 
 * \code
   {
       // Construction, begins with start tag
       XmlElementWriter coord(xml_writer, QLatin1String("coord"));
       coord.writeAttribute(QLatin1String("x"), 34);
       coord.writeAttribute(QLatin1String("y"), 3.4);
       
       // Don't use coord once you wrote other data to the stream
       writeChildElements(xml_writer);
       
   }   // coord goes out of scope here, destructor called, end tag written
 * \endcode
 */
class XmlElementWriter
{
public:
	/**
	 * Begins a new element with the given name on the XML writer.
	 */
	XmlElementWriter(QXmlStreamWriter& xml, const QLatin1String& element_name);
	
	XmlElementWriter(const XmlElementWriter&) = delete;
	XmlElementWriter(XmlElementWriter&&) = delete;
	
	/**
	 * Writes the end tag of the element.
	 */
	~XmlElementWriter();
	
	
	XmlElementWriter& operator=(const XmlElementWriter&) = delete;
	XmlElementWriter& operator=(XmlElementWriter&&) = delete;
	
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const char* value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const QString& value);
	
	/**
	 * Writes an attribute with the given name and value.
	 * This methods uses Qt's default QString::number(double) implementation.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const double value);
	
	/**
	 * Writes an attribute with the given name and value.
	 * The precision represents the number of digits after the decimal point.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const double value, int precision);
	
	/**
	 * Writes an attribute with the given name and value.
	 * This methods uses Qt's default QString::number(float) implementation.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const float value);
	
	/**
	 * Writes an attribute with the given name and value.
	 * The precision represents the number of digits after the decimal point.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const float value, int precision);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const qint64 value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const int value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const unsigned int value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const long unsigned int value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, const quint64 value);
	
	/**
	 * Writes an attribute with the given name and value.
	 */
	void writeAttribute(const QLatin1String& qualifiedName, bool value);
	
	/**
	 * Writes attributes named left, top, width and height,
	 * representing the given area.
	 * This methods uses Qt's default QString::number(qreal) implementation.
	 */
	void write(const QRectF& area);
	
	/**
	 * Writes attributes named left, top, width and height,
	 * representing the given area.
	 */
	void write(const QRectF& area, int precision);
	
	/**
	 * Writes attributes named width and height, representing the given size.
	 * This methods uses Qt's default QString::number(qreal) implementation.
	 */
	void write(const QSizeF& size);
	
	/**
	 * Writes attributes named width and height, representing the given size.
	 */
	void write(const QSizeF& size, int precision);
	
	/**
	 * Writes the coordinates vector as a simple text format.
	 * This is much more efficient than saving each coordinate as rich XML.
	 */
	void write(const MapCoordVector& coords);
	
	/**
	 * Writes tags.
	 */
	void write(const QHash<QString, QString>& tags);
	
private:
	QXmlStreamWriter& xml;
};


/**
 * The XmlElementReader helps to read a single element in an XML document.
 * 
 * It assumes to be on a QXmlStreamReader::StartElement when constructed,
 * and it reads until the end of the current element, skipping any child nodes,
 * when it is destructed. After construction, it offers convenient functions
 * for reading named attributes of common types.
 * 
 * Typical use:
 * 
 * \code
   while (xml_reader.readNextStartElement())
   {
       // Construction, begins with start tag
       XmlElementReader coord(xml_reader);
       int x = coord.attribute<int>(QLatin1String("x"));
       double y = coord.attribute<double>(QLatin1String("y"));
       FlagEnum flags = coord.attribute<FlagEnum>(QLatin1String("flags"));
       
       readChildData(xml_reader);
       
   }   // coord goes out of scope here, destructor called, reads until end of element
 * \endcode
 */
class XmlElementReader
{
public:
	/**
	 * Constructs a new element reader on the given XML reader.
	 * 
	 * It assumes to be on a QXmlStreamReader::StartElement.
	 */
	XmlElementReader(QXmlStreamReader& xml);
	
	XmlElementReader(const XmlElementReader&) = delete;
	XmlElementReader(XmlElementReader&&) = delete;
	
	/**
	 * Destructor.
	 * 
	 * Reads until the end of the current element, skipping any child nodes.
	 */
	~XmlElementReader();
	
	
	XmlElementReader& operator=(const XmlElementReader&) = delete;
	XmlElementReader& operator=(XmlElementReader&&) = delete;
	
	
	/**
	 * Tests whether the element has an attribute with the given name.
	 */
	bool hasAttribute(const QLatin1String&  qualifiedName) const;
	
	/**
	 * Tests whether the element has an attribute with the given name.
	 */
	bool hasAttribute(const QString& qualifiedName) const;
	
	/**
	 * Returns the value of an attribute of type T.
	 * 
	 * Apart from a number of specializations for common types,
	 * it has a general implementation which read the attribute as int
	 * and does static_cast to the actual type. This is useful for enumerations,
	 * but might also be a cause of buildtime or runtime errors.
	 */
	template< typename T >
	T attribute(const QLatin1String& qualifiedName) const;
	
	/**
	 * Reads attributes named left, top, width, height into the given area object.
	 * Counterpart for XmlElementWriter::write(const QRectF&, int).
	 */
	void read(QRectF& area);
	
	/**
	 * Reads attributes named width and height into the given size object.
	 * Counterpart for XmlElementWriter::write(const QSizeF&, int).
	 */
	void read(QSizeF& size);
	
	/**
	 * Reads the coordinates vector from a simple text format.
	 * This is much more efficient than loading each coordinate from rich XML.
	 */
	void read(MapCoordVector& coords);
	
	/**
	 * Reads the coordinates vector for a text object.
	 * 
	 * This is either a single anchor, or an anchor and a size, packed as a
	 * coordinates vector. Regular coordinate bounds checking is not applied
	 * to the size.
	 * 
	 * \todo Make box size explicit data.
	 * 
	 * \see read(MapCoordVector&)
	 */
	void readForText(MapCoordVector& coords);
	
	/**
	 * Read tags.
	 */
	void read(QHash<QString, QString>& tags);
	
private:
	QXmlStreamReader& xml;
	const QXmlStreamAttributes attributes; // implicitly shared QVector
};


/**
 * @namespace literal
 * @brief Namespace for \c QLatin1String constants
 * 
 * It's current main use is in connection with XMLFileFormat.
 * XMLFileFormat is built on \c QXmlStreamReader/\c QXmlStreamWriter which
 * expect \c QLatin1String arguments in many places.
 * In addition, a \c QLatin1String can be compared to a \c QStringRef without
 * implicit conversion.
 * 
 * The namespace \c literal cannot be used directly in header files because it
 * would easily lead to name conflicts in including files.
 * However, custom namespaces in header files can be aliased to \c literal
 * locally in method definitions:
 * 
 * \code
 * void someFuntion()
 * {
 *     namespace literal = XmlStreamLiteral;
 *     writeAttribute(literal::left, 37.0);
 * }
 * \endcode
 * 
 * @sa MapCoordLiteral, XmlStreamLiteral
 */


/**
 * @brief Local namespace for \c QLatin1String constants
 * 
 * This namespace collects various \c QLatin1String constants in xml_stream_util.h.
 * The namespace \link literal \endlink cannot be used directly in the
 * xml_stream_util.h header because it would easily lead to name conflicts
 * in including files. However, XmlStreamLiteral can be aliased to \c literal
 * locally in method definitions:
 * 
 * \code
 * void someFuntion()
 * {
 *     namespace literal = XmlStreamLiteral;
 *     writeAttribute(literal::left, 37.0);
 * }
 * \endcode
 * 
 * @sa literal
 */
namespace XmlStreamLiteral
{
	static const QLatin1String string_true("true");
	
	static const QLatin1String left("left");
	static const QLatin1String top("top");
	static const QLatin1String width("width");
	static const QLatin1String height("height");
	
	static const QLatin1String count("count");
	
	static const QLatin1String object("object");
	static const QLatin1String tags("tags");
	static const QLatin1String tag("tag"); ///< @deprecated
	static const QLatin1String key("key"); ///< @deprecated
	static const QLatin1String t("t");
	static const QLatin1String k("k");
	
	static const QLatin1String coord("coord");
}



//### XmlElementWriter inline implemenentation ###

inline
XmlElementWriter::XmlElementWriter(QXmlStreamWriter& xml, const QLatin1String& element_name)
 : xml(xml)
{
	xml.writeStartElement(element_name);
}

inline
XmlElementWriter::~XmlElementWriter()
{
	xml.writeEndElement();
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const char* value)
{
	xml.writeAttribute(qualifiedName, QString::fromUtf8(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const QString& value)
{
	xml.writeAttribute(qualifiedName, value);
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const double value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const double value, int precision)
{
	auto number = QString::number(value, 'f', precision);
	int i = number.length() - 1;
	if (number.contains(QLatin1Char('.')))
	{
		// Cut off trailing zeros
		while (i > 0 && number.at(i) == QLatin1Char('0'))
			--i;
		if (number.at(i) == QLatin1Char('.'))
		    --i;
		
	}
	xml.writeAttribute(qualifiedName, QString::fromRawData(number.data(), ++i));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const float value)
{
	writeAttribute(qualifiedName, double(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const float value, int precision)
{
	writeAttribute(qualifiedName, double(value), precision);
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const qint64 value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const int value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const unsigned int value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const long unsigned int value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, const quint64 value)
{
	xml.writeAttribute(qualifiedName, QString::number(value));
}

inline
void XmlElementWriter::writeAttribute(const QLatin1String& qualifiedName, bool value)
{
	namespace literal = XmlStreamLiteral;
	
	if (value)
		xml.writeAttribute(qualifiedName, literal::string_true);
}

inline
void XmlElementWriter::write(const QRectF& area)
{
	namespace literal = XmlStreamLiteral;
	
	writeAttribute( literal::left,   area.left());
	writeAttribute( literal::top,    area.top());
	writeAttribute( literal::width,  area.width());
	writeAttribute( literal::height, area.height());
}

inline
void XmlElementWriter::write(const QRectF& area, int precision)
{
	namespace literal = XmlStreamLiteral;
	
	writeAttribute( literal::left,   area.left(),   precision );
	writeAttribute( literal::top,    area.top(),    precision );
	writeAttribute( literal::width,  area.width(),  precision );
	writeAttribute( literal::height, area.height(), precision );
}

inline
void XmlElementWriter::write(const QSizeF& size)
{
	namespace literal = XmlStreamLiteral;
	
	writeAttribute( literal::width,  size.width());
	writeAttribute( literal::height, size.height());
}

inline
void XmlElementWriter::write(const QSizeF& size, int precision)
{
	namespace literal = XmlStreamLiteral;
	
	writeAttribute( literal::width,  size.width(),  precision );
	writeAttribute( literal::height, size.height(), precision );
}

inline
void XmlElementWriter::write(const QHash<QString, QString> &tags)
{
	namespace literal = XmlStreamLiteral;
	typedef QHash<QString, QString> Tags;
	
	for (Tags::const_iterator tag = tags.constBegin(), end = tags.constEnd(); tag != end; ++tag)
	{
		XmlElementWriter tag_element(xml, literal::t);
		tag_element.writeAttribute(literal::k, tag.key());
		xml.writeCharacters(tag.value());
	}
}

//### XmlElementReader inline implemenentation ###

inline
XmlElementReader::XmlElementReader(QXmlStreamReader& xml)
 : xml(xml),
   attributes(xml.attributes())
{
}

inline
XmlElementReader::~XmlElementReader()
{
	if (!xml.isEndElement())
		xml.skipCurrentElement();
}

inline
bool XmlElementReader::hasAttribute(const QLatin1String& qualifiedName) const
{
	return attributes.hasAttribute(qualifiedName);
}

inline
bool XmlElementReader::hasAttribute(const QString& qualifiedName) const
{
	return attributes.hasAttribute(qualifiedName);
}

template< >
inline
QString XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	return attributes.value(qualifiedName).toString();
}

template< >
inline
qint64 XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	qint64 value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toLongLong();
	return value;
}

template< >
inline
int XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	int value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toInt();
	return value;
}

template< >
inline
unsigned int XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	unsigned int value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toUInt();
	return value;
}

template< >
inline
long unsigned int XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	unsigned int value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toUInt();
	return value;
}

template< >
inline
double XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	double value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toDouble();
	return value;
}

template< >
inline
float XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	float value = 0;
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = QString::fromRawData(ref.data(), ref.size()).toFloat();
	return value;
}

template< >
inline
bool XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	namespace literal = XmlStreamLiteral;
	
	bool value = (attributes.value(qualifiedName) == literal::string_true);
	return value;
}

template<  >
inline
QStringRef XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	return attributes.value(qualifiedName);
}

template< typename T >
inline
T XmlElementReader::attribute(const QLatin1String& qualifiedName) const
{
	T value = static_cast<T>(0);
	const QStringRef ref = attributes.value(qualifiedName);
	if (ref.size())
		value = static_cast<T>(QString::fromRawData(ref.data(), ref.size()).toInt());
	return value;
}

inline
void XmlElementReader::read(QRectF& area)
{
	namespace literal = XmlStreamLiteral;
	
	QStringRef ref = attributes.value(literal::left);
	area.setLeft(QString::fromRawData(ref.data(), ref.size()).toDouble());
	ref = attributes.value(literal::top);
	area.setTop(QString::fromRawData(ref.data(), ref.size()).toDouble());
	ref = attributes.value(literal::width);
	area.setWidth(QString::fromRawData(ref.data(), ref.size()).toDouble());
	ref = attributes.value(literal::height);
	area.setHeight(QString::fromRawData(ref.data(), ref.size()).toDouble());
}

inline
void XmlElementReader::read(QSizeF& size)
{
	namespace literal = XmlStreamLiteral;
	
	QStringRef ref = attributes.value(literal::width);
	size.setWidth(QString::fromRawData(ref.data(), ref.size()).toDouble());
	ref = attributes.value(literal::height);
	size.setHeight(QString::fromRawData(ref.data(), ref.size()).toDouble());
}

inline
void XmlElementReader::read(QHash<QString, QString> &tags)
{
	namespace literal = XmlStreamLiteral;
	
	tags.clear();
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::t)
		{
			const QString key(xml.attributes().value(literal::k).toString());
			tags.insert(key, xml.readElementText());
		}
		else if (xml.name() == literal::tag)
		{
			// Full keywords were used in pre-0.6.0 master branch
			// TODO Remove after Mapper 0.6.x releases
			const QString key(xml.attributes().value(literal::key).toString());
			tags.insert(key, xml.readElementText());
		}
		else if (xml.name() == literal::tags)
		{
			// Fix for broken Object::save in pre-0.6.0 master branch
			// TODO Remove after Mapper 0.6.x releases
			const QString key(xml.attributes().value(literal::key).toString());
			tags.insert(key, xml.readElementText());
		}
		else
			xml.skipCurrentElement();
	}
}


}  // namespace OpenOrienteering

#endif
