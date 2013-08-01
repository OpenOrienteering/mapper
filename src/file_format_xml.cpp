/*
 *    Copyright 2012, 2013 Pete Curtis, Kai Pastor
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

#include "file_format_xml.h"
#include "file_format_xml_p.h"

#include <QDebug>
#include <QFile>
#include <QPrinter>
#include <QStringBuilder>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map_color.h"
#include "core/map_printer.h"
#include "file_import_export.h"
#include "georeferencing.h"
#include "map.h"
#include "map_grid.h"
#include "object.h"
#include "object_text.h"
#include "symbol_area.h"
#include "symbol_combined.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_text.h"
#include "template.h"

// ### XMLFileFormat definition ###

const int XMLFileFormat::minimum_version = 2;
const int XMLFileFormat::current_version = 5;
const QString XMLFileFormat::magic_string = "<?xml ";
const QString XMLFileFormat::mapper_namespace = "http://oorienteering.sourceforge.net/mapper/xml/v2";

XMLFileFormat::XMLFileFormat()
 : FileFormat(MapFile, "XML", ImportExport::tr("OpenOrienteering Mapper"), "omap", 
              ImportSupported | ExportSupported) 
{
	addExtension("xmap"); // Legacy, TODO: Remove .xmap in release 1.0.
}

bool XMLFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
	uint len = magic_string.length();
	if (sz >= len && memcmp(buffer, magic_string.toUtf8().data(), len) == 0) 
		return true;
	return false;
}

Importer *XMLFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException)
{
	return new XMLFileImporter(stream, map, view);
}

Exporter *XMLFileFormat::createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException)
{
	return new XMLFileExporter(stream, map, view);
}



// ### XMLFileExporter definition ###

XMLFileExporter::XMLFileExporter(QIODevice* stream, Map *map, MapView *view)
: Exporter(stream, map, view),
  xml(stream)
{
	// Determine auto-formatting default from filename, if possible.
	const QFile* file = qobject_cast< const QFile* >(stream);
	bool auto_formatting = (file && file->fileName().contains(".xmap"));
	setOption("autoFormatting", auto_formatting);
}

void XMLFileExporter::doExport() throw (FileFormatException)
{
	if (option("autoFormatting").toBool() == true)
		xml.setAutoFormatting(true);
	
	xml.writeDefaultNamespace(XMLFileFormat::mapper_namespace);
	xml.writeStartDocument();
	xml.writeStartElement("map");
	xml.writeAttribute("version", QString::number(XMLFileFormat::current_version));
	
	xml.writeTextElement("notes", map->getMapNotes());
	
	exportGeoreferencing();
	exportColors();
	exportSymbols();
	exportMapParts();
	exportTemplates();
	exportView();
	exportPrint();
	exportUndo();
	exportRedo();
	
	xml.writeEndElement(/*document*/);
	xml.writeEndDocument();
}

void XMLFileExporter::exportGeoreferencing()
{
	map->getGeoreferencing().save(xml);
}

void XMLFileExporter::exportColors()
{
	xml.writeStartElement("colors");
	int num_colors = (int)map->color_set->colors.size();
	xml.writeAttribute("count", QString::number(num_colors));
	for (int i = 0; i < num_colors; ++i)
	{
		MapColor* color = map->color_set->colors[i];
		const MapColorCmyk &cmyk = color->getCmyk();
		xml.writeStartElement("color");
		xml.writeAttribute("priority", QString::number(color->getPriority()));
		xml.writeAttribute("name", color->getName());
		xml.writeAttribute("c", QString::number(cmyk.c, 'f', 3));
		xml.writeAttribute("m", QString::number(cmyk.m, 'f', 3));
		xml.writeAttribute("y", QString::number(cmyk.y, 'f', 3));
		xml.writeAttribute("k", QString::number(cmyk.k, 'f', 3));
		xml.writeAttribute("opacity", QString::number(color->getOpacity(), 'f', 3));
		
		if (color->getSpotColorMethod() != MapColor::UndefinedMethod)
		{
			xml.writeStartElement("spotcolors");
			if (color->getKnockout())
				xml.writeAttribute("knockout", "true");
			SpotColorComponent component;
			switch(color->getSpotColorMethod())
			{
				case MapColor::SpotColor:
					xml.writeTextElement("namedcolor", color->getSpotColorName());
					break;
				case MapColor::CustomColor:
					Q_FOREACH(component, color->getComponents())
					{
						xml.writeStartElement("component");
						xml.writeAttribute("factor", QString::number(component.factor));
						xml.writeAttribute("spotcolor", QString::number(component.spot_color->getPriority()));
						xml.writeEndElement(/*component*/);
					}
				default:
					; // nothing
			}
			xml.writeEndElement(/*spotcolors*/);
		}
		
		xml.writeStartElement("cmyk");
		switch(color->getCmykColorMethod())
		{
			case MapColor::SpotColor:
				xml.writeAttribute("method", "spotcolor");
				break;
			case MapColor::RgbColor:
				xml.writeAttribute("method", "rgb");
				break;
			default:
				xml.writeAttribute("method", "custom");
		}
		xml.writeEndElement(/*cmyk*/);
		
		xml.writeStartElement("rgb");
		switch(color->getCmykColorMethod())
		{
			case MapColor::SpotColor:
				xml.writeAttribute("method", "spotcolor");
				break;
			case MapColor::CmykColor:
				xml.writeAttribute("method", "cmyk");
				break;
			default:
				xml.writeAttribute("method", "custom");
		}
		const MapColorRgb &rgb = color->getRgb();
		xml.writeAttribute("r", QString::number(rgb.r, 'f', 3));
		xml.writeAttribute("g", QString::number(rgb.g, 'f', 3));
		xml.writeAttribute("b", QString::number(rgb.b, 'f', 3));
		xml.writeEndElement(/*rgb*/);
		
		xml.writeEndElement(/*color*/);
	}
	xml.writeEndElement(/*colors*/); 
}

void XMLFileExporter::exportSymbols()
{
	xml.writeStartElement("symbols");
	int num_symbols = map->getNumSymbols();
	xml.writeAttribute("count", QString::number(num_symbols));
	for (int i = 0; i < num_symbols; ++i)
	{
		map->getSymbol(i)->save(xml, *map);
	}
	xml.writeEndElement(/*symbols*/); 
}

void XMLFileExporter::exportMapParts()
{
	xml.writeStartElement("parts");
	int num_parts = map->getNumParts();
	xml.writeAttribute("count", QString::number(num_parts));
	xml.writeAttribute("current", QString::number(map->current_part_index));
	for (int i = 0; i < num_parts; ++i)
		map->getPart(i)->save(xml, *map);
	xml.writeEndElement(/*parts*/); 
}

void XMLFileExporter::exportTemplates()
{
	xml.writeStartElement("templates");
	
	int num_templates = map->getNumTemplates() + map->getNumClosedTemplates();
	xml.writeAttribute("count", QString::number(num_templates));
	xml.writeAttribute("first_front_template", QString::number(map->first_front_template));
	for (int i = 0; i < map->getNumTemplates(); ++i)
		map->getTemplate(i)->saveTemplateConfiguration(xml, true);
	for (int i = 0; i < map->getNumClosedTemplates(); ++i)
		map->getClosedTemplate(i)->saveTemplateConfiguration(xml, false);
	
	xml.writeEmptyElement("defaults");
	xml.writeAttribute("use_meters_per_pixel", map->image_template_use_meters_per_pixel ? "true" : "false");
	xml.writeAttribute("meters_per_pixel", QString::number(map->image_template_meters_per_pixel));
	xml.writeAttribute("dpi", QString::number(map->image_template_dpi));
	xml.writeAttribute("scale", QString::number(map->image_template_scale));
	
	xml.writeEndElement(/*templates*/); 
}

void XMLFileExporter::exportView()
{
	xml.writeStartElement("view");
	if (map->area_hatching_enabled)
		xml.writeAttribute("area_hatching_enabled", "true");
	if (map->baseline_view_enabled)
		xml.writeAttribute("baseline_view_enabled", "true");
	
	map->getGrid().save(xml);
	
	if (view)
		view->save(xml, QLatin1String("map_view"));
	
	xml.writeEndElement(/*view*/);
}

void XMLFileExporter::exportPrint()
{
	map->printerConfig().save(xml, "print");
}

void XMLFileExporter::exportUndo()
{
	map->object_undo_manager.saveUndo(xml);
}

void XMLFileExporter::exportRedo()
{
	map->object_undo_manager.saveRedo(xml);
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
	addWarning(tr("Unsupported element: %1 (line %2 column %3)").
	  arg(xml.name().toString()).
	  arg(xml.lineNumber()).
	  arg(xml.columnNumber())
	);
}

void XMLFileImporter::import(bool load_symbols_only) throw (FileFormatException)
{
	/*while (!xml.atEnd())
	{
		if (xml.readNextStartElement())
			qDebug() << xml.qualifiedName();
		else
			qDebug() << "FALSE";
	}*/
	
	if (xml.readNextStartElement())
	{
		if (xml.name() != "map")
			xml.raiseError(Importer::tr("Unsupported file format."));
		else
		{
			int version = xml.attributes().value("version").toString().toInt();
			if (version < 1)
				xml.raiseError(Importer::tr("Invalid file format version."));
			else if (version < XMLFileFormat::minimum_version)
				xml.raiseError(Importer::tr("Unsupported old file format version. Please use an older program version to load and update the file."));
			else if (version > XMLFileFormat::current_version)
				addWarning(Importer::tr("Unsupported new file format version. Some map features will not be loaded or saved by this version of the program."));
		}
	}
	
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		
		if (name == "colors")
			importColors();
		else if (name == "symbols")
			importSymbols();
		else if (name == "georeferencing")
			importGeoreferencing(load_symbols_only);
		else if (load_symbols_only)
			xml.skipCurrentElement();
		else if (name == "notes")
			map->setMapNotes(xml.readElementText());
		else if (name == "parts")
			importMapParts();
		else if (name == "templates")
			importTemplates();
		else if (name == "view")
			importView();
		else if (name == "print")
			importPrint();
		else if (name == "undo")
			importUndo();
		else if (name == "redo")
			importRedo();
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (xml.error())
		throw FileFormatException(xml.errorString());
}

void XMLFileImporter::importGeoreferencing(bool load_symbols_only)
{
	Q_ASSERT(xml.name() == "georeferencing");
	Georeferencing georef;
	georef.load(xml, load_symbols_only);
	map->setGeoreferencing(georef);
}

/** Helper for delayed actions */
struct XMLFileImporterColorBacklogItem
{
	MapColor* color; // color which needs updating
	SpotColorComponents components; // components of the color
	bool cmyk_from_spot; // determine CMYK from spot
	bool rgb_from_spot; // determine RGB from spot
	
	XMLFileImporterColorBacklogItem(MapColor* color)
	: color(color), cmyk_from_spot(false), rgb_from_spot(false)
	{}
};
typedef std::vector<XMLFileImporterColorBacklogItem> XMLFileImporterColorBacklog;
	

void XMLFileImporter::importColors()
{
	int num_colors = xml.attributes().value("count").toString().toInt();
	Map::ColorVector& colors(map->color_set->colors);
	colors.reserve(qMin(num_colors, 100)); // 100 is not a limit
	XMLFileImporterColorBacklog backlog;
	backlog.reserve(colors.size());
	while (xml.readNextStartElement())
	{
		if (xml.name() == "color")
		{
			QXmlStreamAttributes attributes = xml.attributes();
			MapColor* color = new MapColor(attributes.value("name").toString(), attributes.value("priority").toString().toInt());
			if (attributes.hasAttribute("opacity"))
				color->setOpacity(attributes.value("opacity").toString().toFloat());
			
			MapColorCmyk cmyk;
			cmyk.c = attributes.value("c").toString().toFloat();
			cmyk.m = attributes.value("m").toString().toFloat();
			cmyk.y = attributes.value("y").toString().toFloat();
			cmyk.k = attributes.value("k").toString().toFloat();
			color->setCmyk(cmyk);
			
			SpotColorComponents components;
			QString cmyk_method;
			QString rgb_method;
			MapColorRgb rgb;
			
			while(xml.readNextStartElement())
			{
				attributes = xml.attributes();
				if (xml.name() == "spotcolors")
				{
					color->setKnockout(attributes.value("knockout").toString() == "true");
					while(xml.readNextStartElement())
					{
						if (xml.name() == "namedcolor")
						{
							color->setSpotColorName(xml.readElementText());
						}
						else if (xml.name() == "component")
						{
							SpotColorComponent component;
							component.factor = xml.attributes().value("factor").toString().toFloat();
							// We can't know if the spot color is already loaded. Create a temporary proxy.
							component.spot_color = new MapColor(xml.attributes().value("spotcolor").toString().toInt());
							components.push_back(component);
							xml.skipCurrentElement();
						}
						else
							xml.skipCurrentElement(); // unsupported
					}
				}
				else if (xml.name() == "cmyk")
				{
					cmyk_method = attributes.value("method").toString();
					xml.skipCurrentElement();
				}
				else if (xml.name() == "rgb")
				{
					rgb_method = attributes.value("method").toString();
					rgb.r = attributes.value("r").toString().toFloat();
					rgb.g = attributes.value("g").toString().toFloat();
					rgb.b = attributes.value("b").toString().toFloat();
					color->setRgbFromSpotColors();
					xml.skipCurrentElement();
				}
				else
					xml.skipCurrentElement(); // unsupported
			}
			
			if (!components.empty())
			{
				backlog.push_back(XMLFileImporterColorBacklogItem(color));
				XMLFileImporterColorBacklogItem& item = backlog.back();
				item.components = components;
				item.cmyk_from_spot = (cmyk_method == "spotcolor");
				item.rgb_from_spot = (rgb_method == "spotcolor");
			}
			
			if (cmyk_method == "rgb")
				color->setCmykFromRgb();
			
			if (rgb_method == "cmyk")
				color->setRgbFromCmyk();
			
			colors.push_back(color);
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (num_colors > 0 && num_colors != (int)colors.size())
		addWarning(tr("Expected %1 colors, found %2.").
		  arg(num_colors).
		  arg(colors.size())
		);
	
	// All spot colors are loaded at this point.
	// Now deal with depending color compositions from the backlog.
	Q_FOREACH(XMLFileImporterColorBacklogItem item, backlog)
	{
		// Process the list of spot color components.
		SpotColorComponents out_components;
		Q_FOREACH(SpotColorComponent in_component, item.components)
		{
			MapColor* out_color = map->getColor(in_component.spot_color->getPriority());
			if (out_color == NULL || out_color->getSpotColorMethod() != MapColor::SpotColor)
			{
				addWarning(tr("Spot color %1 not found while processing %2 (%3).").
				  arg(in_component.spot_color->getPriority()).
				  arg(item.color->getPriority()).
				  arg(item.color->getName())
				);
				continue; // Drop this color, invalid reference
			}
			
			out_components.push_back(in_component);
			SpotColorComponent& out_component = out_components.back();
			out_component.spot_color = out_color; // That is the major point!
			delete in_component.spot_color; // Delete the temporary proxy.
		}
		
		// Update the current color
		item.color->setSpotColorComposition(out_components);
		if (item.cmyk_from_spot)
			item.color->setCmykFromSpotColors();
		if (item.rgb_from_spot)
			item.color->setRgbFromSpotColors();
	}
}

void XMLFileImporter::importSymbols()
{
	int num_symbols = xml.attributes().value("count").toString().toInt();
	map->symbols.reserve(qMin(num_symbols, 1000)); // 1000 is not a limit
	
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedPoint()))] = map->getUndefinedPoint();
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedLine()))] = map->getUndefinedLine();
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "symbol")
		{
			map->symbols.push_back(Symbol::load(xml, *map, symbol_dict));
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (num_symbols > 0 && num_symbols != map->getNumSymbols())
		addWarning(tr("Expected %1 symbols, found %2.").
		  arg(num_symbols).
		  arg(map->getNumSymbols())
		);
}

void XMLFileImporter::importMapParts()
{
	int num_parts = xml.attributes().value("count").toString().toInt();
	int current_part_index = xml.attributes().value("current").toString().toInt();
	map->parts.clear();
	map->parts.reserve(qMin(num_parts, 20)); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "part")
		{
			map->parts.push_back(MapPart::load(xml, *map, symbol_dict));
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (0 <= current_part_index && current_part_index < map->getNumParts())
		map->current_part_index = current_part_index;
	
	if (num_parts > 0 && num_parts != map->getNumParts())
		addWarning(tr("Expected %1 map parts, found %2.").
		  arg(num_parts).
		  arg(map->getNumParts())
		);
	
	emit map->currentMapPartChanged(map->current_part_index);
}

void XMLFileImporter::importTemplates()
{
	Q_ASSERT(xml.name() == "templates");
	
	int first_front_template = xml.attributes().value("first_front_template").toString().toInt();
	
	int num_templates = xml.attributes().value("count").toString().toInt();
	map->templates.reserve(qMin(num_templates, 20)); // 20 is not a limit
	map->closed_templates.reserve(num_templates % 20);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "template")
		{
			bool opened = true;
			Template* temp = Template::loadTemplateConfiguration(xml, *map, opened);
			if (opened)
				map->templates.push_back(temp);
			else
				map->closed_templates.push_back(temp);
		}
		else if (xml.name() == "defaults")
		{
			QXmlStreamAttributes attributes = xml.attributes();
			map->image_template_use_meters_per_pixel = (attributes.value("use_meters_per_pixel") == "true");
			map->image_template_meters_per_pixel = attributes.value("meters_per_pixel").toString().toDouble();
			map->image_template_dpi = attributes.value("dpi").toString().toDouble();
			map->image_template_scale = attributes.value("scale").toString().toDouble();
			xml.skipCurrentElement();
		}
		else
		{
			qDebug() << "Unsupported element: " << xml.qualifiedName();
			xml.skipCurrentElement();
		}
	}
	
	map->first_front_template = qMax(0, qMin(map->getNumTemplates(), first_front_template));
}

void XMLFileImporter::importView()
{
	Q_ASSERT(xml.name() == "view");
	
	map->area_hatching_enabled = (xml.attributes().value("area_hatching_enabled") == "true");
	map->baseline_view_enabled = (xml.attributes().value("baseline_view_enabled") == "true");
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "grid")
			map->getGrid().load(xml);
		else if (xml.name() == "map_view")
			view->load(xml);
		else
			xml.skipCurrentElement(); // unsupported
	}
}

void XMLFileImporter::importPrint()
{
	Q_ASSERT(xml.name() == "print");
	
	map->setPrinterConfig(MapPrinterConfig(*map, xml));
}

void XMLFileImporter::importUndo()
{
	map->object_undo_manager.loadUndo(xml, symbol_dict);
}

void XMLFileImporter::importRedo()
{
	map->object_undo_manager.loadRedo(xml, symbol_dict);
}
