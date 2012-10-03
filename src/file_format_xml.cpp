/*
 *    Copyright 2012 Pete Curtis, Kai Pastor
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

#include <QDebug>
#include <QFile>
#include <QStringBuilder>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "file_format_xml.h"
#include "georeferencing.h"

#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "symbol_combined.h"
#include "object.h"
#include "object_text.h"


// ### XMLFileExporter declaration ###

class XMLFileExporter : public Exporter
{
public:
	XMLFileExporter(QIODevice* stream, Map *map, MapView *view);
	virtual ~XMLFileExporter() {}
	
	virtual void doExport() throw (FormatException);

protected:
	QXmlStreamWriter xml;
};


// ### XMLFileImporter declaration ###

class XMLFileImporter : public Importer
{
public:
	XMLFileImporter(QIODevice* stream, Map *map, MapView *view);
	virtual ~XMLFileImporter() {}

protected:
	virtual void import(bool load_symbols_only) throw (FormatException);
	
	void addWarningUnsupportedElement();
	void importGeoreferencing();
	void importColors();
	void importSymbols();
	void importMapParts();
	
	QXmlStreamReader xml;
};



// ### XMLFileFormat definition ###

const int XMLFileFormat::minimum_version = 2;
const int XMLFileFormat::current_version = 2;
const QString XMLFileFormat::magic_string = "<?xml ";
const QString XMLFileFormat::mapper_namespace = "http://oorienteering.sourceforge.net/mapper/xml/v2";

XMLFileFormat::XMLFileFormat()
: Format("XML", QObject::tr("OpenOrienteering Mapper XML"), "xmap", true, true, true) 
{
	// NOP
}

bool XMLFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
	uint len = magic_string.length();
	if (sz >= len && memcmp(buffer, magic_string.toUtf8().data(), len) == 0) 
		return true;
	return false;
}

Importer *XMLFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException)
{
	return new XMLFileImporter(stream, map, view);
}

Exporter *XMLFileFormat::createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException)
{
	return new XMLFileExporter(stream, map, view);
}



// ### XMLFileExporter definition ###

XMLFileExporter::XMLFileExporter(QIODevice* stream, Map *map, MapView *view)
: Exporter(stream, map, view),
  xml(stream)
{
	xml.setAutoFormatting(true);
}

void XMLFileExporter::doExport() throw (FormatException)
{
	xml.writeDefaultNamespace(XMLFileFormat::mapper_namespace);
	xml.writeStartDocument();
	xml.writeStartElement("map");
	xml.writeAttribute("version", QString::number(XMLFileFormat::current_version));
	
	xml.writeTextElement("notes", map->getMapNotes());
	
	map->getGeoreferencing().save(xml);
	
	xml.writeStartElement("view");
	xml.writeEmptyElement("grid"); // TODO
	if (map->area_hatching_enabled)
		xml.writeEmptyElement("area_hatching_enabled");
	if (map->baseline_view_enabled)
		xml.writeEmptyElement("baseline_view_enabled");
	xml.writeEndElement(/*view*/); 
	
	xml.writeStartElement("print");
	// TODO
	xml.writeEndElement(/*print*/); 
	
	xml.writeStartElement("image_template");
	// TODO
	xml.writeEndElement(/*image_template*/); 
	
	xml.writeStartElement("colors");
	int num_colors = (int)map->color_set->colors.size();
	xml.writeAttribute("number", QString::number(num_colors));
	for (int i = 0; i < num_colors; ++i)
	{
		MapColor* color = map->color_set->colors[i];
		xml.writeEmptyElement("color");
		xml.writeAttribute("priority", QString::number(color->priority));
		xml.writeAttribute("c", QString::number(color->c));
		xml.writeAttribute("m", QString::number(color->m));
		xml.writeAttribute("y", QString::number(color->y));
		xml.writeAttribute("k", QString::number(color->k));
		xml.writeAttribute("opacity", QString::number(color->opacity));
		xml.writeAttribute("name", color->name);
	}
	xml.writeEndElement(/*colors*/); 
	
	xml.writeStartElement("symbols");
	int num_symbols = map->getNumSymbols();
	xml.writeAttribute("number", QString::number(num_symbols));
	for (int i = 0; i < num_symbols; ++i)
	{
		map->getSymbol(i)->save(xml, *map);
	}
	xml.writeEndElement(/*symbols*/); 
	
	xml.writeStartElement("undo");
	// TODO
	xml.writeEndElement(/*undo*/); 
	
	xml.writeStartElement("parts");
	int num_parts = map->getNumParts();
	xml.writeAttribute("number", QString::number(num_parts));
	xml.writeAttribute("current",QString::number( map->current_part_index));
	for (int i = 0; i < num_parts; ++i)
		map->getPart(i)->save(xml, *map);
	xml.writeEndElement(/*parts*/); 
	
	xml.writeEndElement(/*document*/);
	xml.writeEndDocument();
}



// ### XMLFileImporter definition ###

XMLFileImporter::XMLFileImporter(QIODevice* stream, Map *map, MapView *view)
: Importer(stream, map, view),
  xml(stream)
{
	//NOP
}

void XMLFileImporter::addWarningUnsupportedElement()
{
	addWarning(QObject::tr("Unsupported element: %1 (line %2 column %3)").
	  arg(xml.name().toString()).
	  arg(xml.lineNumber()).
	  arg(xml.columnNumber())
	);
}

void XMLFileImporter::import(bool load_symbols_only) throw (FormatException)
{
	if (xml.readNextStartElement())
	{
		if (xml.name() != "map")
			xml.raiseError(QObject::tr("Unsupport file format."));
		else
		{
			int version = xml.attributes().value("version").toString().toInt();
			if (version < 1)
				xml.raiseError(QObject::tr("Invalid file format version."));
			else if (version < XMLFileFormat::minimum_version)
				xml.raiseError(QObject::tr("Unsupported file format version. Please use an older program version to load and update the file."));
			else if (version > XMLFileFormat::current_version)
				addWarning(QObject::tr("New file format version detected. Some features will be not be supported by this version of the program."));
		}
	}
	
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		
		if (name == "georeferencing")
			importGeoreferencing();
		else if (name == "colors")
			importColors();
		else if (name == "symbols")
			importSymbols();
		else if (load_symbols_only)
			xml.skipCurrentElement();
		else if (name == "notes")
			map->setMapNotes(xml.readElementText());
/*
		else if (name == "view")
			xml.skipCurrentElement();
		else if (name == "print")
			xml.skipCurrentElement();
		else if (name == "image_template")
			xml.skipCurrentElement();
		else if (name == "undo")
			xml.skipCurrentElement();
*/
		else if (name == "parts")
			importMapParts();
#ifdef Mapper_XML_OBSOLETE_ELEMENTS
		else if (name == "layers")
			importMapParts();
#endif
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (xml.error())
		throw FormatException(xml.errorString());
}

void XMLFileImporter::importGeoreferencing()
{
	Q_ASSERT(xml.name() == "georeferencing");
	Georeferencing georef;
	georef.load(xml);
	map->setGeoreferencing(georef);
}

void XMLFileImporter::importColors()
{
	int num_colors = xml.attributes().value("number").toString().toInt();
	Map::ColorVector& colors(map->color_set->colors);
	colors.reserve(num_colors % 100); // 100 is not a limit
	while (xml.readNextStartElement())
	{
		if (xml.name() == "color")
		{
			QXmlStreamAttributes attributes = xml.attributes();
			MapColor* color = new MapColor();
			color->name = attributes.value("name").toString();
			color->priority = attributes.value("priority").toString().toInt();
			color->c = attributes.value("c").toString().toFloat();
			color->m = attributes.value("m").toString().toFloat();
			color->y = attributes.value("y").toString().toFloat();
			color->k = attributes.value("k").toString().toFloat();
			color->opacity = attributes.value("opacity").toString().toFloat();
			color->updateFromCMYK();
			colors.push_back(color);
		}
		else
			addWarningUnsupportedElement();
		xml.skipCurrentElement();
	}
	
	if (num_colors > 0 && num_colors != (int)colors.size())
		addWarning(QObject::tr("Expected %1 colors, found %2.").
		  arg(num_colors).
		  arg(colors.size())
		);
}

void XMLFileImporter::importSymbols()
{
	int num_symbols = xml.attributes().value("number").toString().toInt();
	map->symbols.reserve(num_symbols % 1000); // 1000 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "symbol")
		{
			map->symbols.push_back(Symbol::load(xml, *map));
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (num_symbols > 0 && num_symbols != map->getNumSymbols())
		addWarning(QObject::tr("Expected %1 symbols, found %2.").
		  arg(num_symbols).
		  arg(map->getNumSymbols())
		);
}

void XMLFileImporter::importMapParts()
{
	int num_parts = xml.attributes().value("number").toString().toInt();
	int current_part_index = xml.attributes().value("current").toString().toInt();
	map->parts.clear();
	map->parts.reserve(num_parts % 20); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "part")
		{
			map->parts.push_back(MapPart::load(xml, *map));
		}
#ifndef Mapper_XML_DROP_OBSOLETE_ELEMENTS
		else if (xml.name() == "layer")
		{
			map->parts.push_back(MapPart::load(xml, *map));
		}
#endif
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (0 <= current_part_index && current_part_index < map->getNumParts())
		map->current_part_index = current_part_index;
	
	if (num_parts > 0 && num_parts != map->getNumParts())
		addWarning(QObject::tr("Expected %1 map parts, found %2.").
		  arg(num_parts).
		  arg(map->getNumParts())
		);
}
