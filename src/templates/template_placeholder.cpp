/*
 *    Copyright 2020 Kai Pastor
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


#include "template_placeholder.h"

#include <utility>

#include <QByteArray>
#include <QRectF>
#include <QStringRef>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// IWYU pragma: no_include <qxmlstream.h>

namespace OpenOrienteering {

namespace {

bool copyXml(QXmlStreamReader& xml, QXmlStreamWriter& writer)
{
	auto token = xml.tokenType();
	if (token != QXmlStreamReader::StartElement)
	{
		xml.skipCurrentElement();
		return false;
	}
	
	writer.writeStartElement(xml.name().toString());
	writer.writeAttributes(xml.attributes());
	for (auto depth = 1; depth > 0; )
	{
		if (xml.error() != QXmlStreamReader::NoError)
			return false;
		
		token = xml.readNext();
		switch (token)
		{
		case QXmlStreamReader::NoToken:
		case QXmlStreamReader::Invalid:
		case QXmlStreamReader::DTD:
		case QXmlStreamReader::StartDocument:
		case QXmlStreamReader::EndDocument:
			return false;
		case QXmlStreamReader::StartElement:
			writer.writeStartElement(xml.name().toString());
			writer.writeAttributes(xml.attributes());
			++depth;
			break;
		case QXmlStreamReader::EndElement:
			--depth;
			writer.writeEndElement();
			break;
		case QXmlStreamReader::Characters:
			writer.writeCharacters(xml.text().toString());
			break;
		case QXmlStreamReader::EntityReference:
			writer.writeEntityReference(xml.name().toString());
			break;
		case QXmlStreamReader::Comment:
		case QXmlStreamReader::ProcessingInstruction:
			// skip
			break;
		}
	}
	return true;
}

}


TemplatePlaceholder::TemplatePlaceholder(QByteArray type, const QString& path, not_null<Map*> map)
: Template(path, map)
, original_type(std::move(type))
{}

TemplatePlaceholder::TemplatePlaceholder(const TemplatePlaceholder& proto) = default;

TemplatePlaceholder::~TemplatePlaceholder() = default;

TemplatePlaceholder* TemplatePlaceholder::duplicate() const
{
	return new TemplatePlaceholder(*this);
}

const char* TemplatePlaceholder::getTemplateType() const
{
	return original_type;
}

std::unique_ptr<Template> TemplatePlaceholder::makeActualTemplate() const
{
	auto maybe_open = true;
	QString buffer;
	{
		QXmlStreamWriter writer(&buffer);
		saveTemplateConfiguration(writer, maybe_open);
	}
	QXmlStreamReader reader(buffer);
	reader.readNextStartElement();  // <template ...>
	return Template::loadTemplateConfiguration(reader, *map, maybe_open);
}

bool TemplatePlaceholder::isRasterGraphics() const
{
	return true;
}

void TemplatePlaceholder::drawTemplate(QPainter* /*painter*/, const QRectF& /*clip_rect*/, double /*scale*/, bool /*on_screen*/, qreal /*opacity*/) const
{}

void TemplatePlaceholder::saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const
{
	if (config.isEmpty())
		return;
	
	QXmlStreamReader reader(config);
	reader.readNextStartElement();  // Enter root element
	while (reader.readNextStartElement())
		copyXml(reader, xml);
}

bool TemplatePlaceholder::loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml)
{
	QXmlStreamWriter writer(&config);
	if (config.isEmpty())
	{
		writer.writeStartDocument();
		// We need an enclosing root element.
		writer.writeStartElement(QString::fromLatin1("root"));
	}
	auto result = copyXml(xml, writer);
	//writer.writeEndDocument(); // No, we may need to append.
	return result;
}

bool TemplatePlaceholder::loadTemplateFileImpl()
{
	setErrorString(tr("Unknown file format"));
	return false;
}

void TemplatePlaceholder::unloadTemplateFileImpl()
{}


}  // namespace OpenOrienteering
