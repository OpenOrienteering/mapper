/*
 *    Copyright 2012, 2013, 2014 Pete Curtis, Kai Pastor
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
#include <QStringBuilder>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifdef QT_PRINTSUPPORT_LIB
#  include <QPrinter>
#endif

#include "core/georeferencing.h"
#include "core/map_color.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "file_import_export.h"
#include "map.h"
#include "map_grid.h"
#include "object.h"
#include "object_text.h"
#include "settings.h"
#include "symbol_area.h"
#include "symbol_combined.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "symbol_text.h"
#include "template.h"
#include "util/xml_stream_util.h"

// ### XMLFileFormat definition ###

const int XMLFileFormat::minimum_version = 2;
const int XMLFileFormat::current_version = 6;

int XMLFileFormat::active_version = 5; // updated by XMLFileExporter::doExport()

const QString XMLFileFormat::magic_string = "<?xml ";
const QString XMLFileFormat::mapper_namespace = "http://oorienteering.sourceforge.net/mapper/xml/v2";

XMLFileFormat::XMLFileFormat()
 : FileFormat(MapFile, "XML", ImportExport::tr("OpenOrienteering Mapper"), "omap", 
              ImportSupported | ExportSupported) 
{
	addExtension("xmap");
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



// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String map("map");
	static const QLatin1String version("version");
	static const QLatin1String notes("notes");
	
	static const QLatin1String count("count");
	static const QLatin1String current("current");
	
	static const QLatin1String georeferencing("georeferencing");
	
	static const QLatin1String colors("colors");
	static const QLatin1String color("color");
	static const QLatin1String priority("priority");
	static const QLatin1String name("name");
	static const QLatin1String method("method");
	static const QLatin1String spotcolor("spotcolor");
	static const QLatin1String cmyk("cmyk");
	static const QLatin1String rgb("rgb");
	static const QLatin1String custom("custom");
	static const QLatin1String spotcolors("spotcolors");
	static const QLatin1String knockout("knockout");
	static const QLatin1String namedcolor("namedcolor");
	static const QLatin1String component("component");
	static const QLatin1String factor("factor");
	static const QLatin1String c("c");
	static const QLatin1String m("m");
	static const QLatin1String y("y");
	static const QLatin1String k("k");
	static const QLatin1String r("r");
	static const QLatin1String g("g");
	static const QLatin1String b("b");
	static const QLatin1String opacity("opacity");
	
	static const QLatin1String symbols("symbols");
	static const QLatin1String symbol("symbol");
	
	static const QLatin1String parts("parts");
	static const QLatin1String part("part");
	
	static const QLatin1String templates("templates");
	static const QLatin1String template_string("template");
	static const QLatin1String first_front_template("first_front_template");
	
	static const QLatin1String defaults("defaults");
	static const QLatin1String use_meters_per_pixel("use_meters_per_pixel");
	static const QLatin1String meters_per_pixel("meters_per_pixel");
	static const QLatin1String dpi("dpi");
	static const QLatin1String scale("scale");
	
	static const QLatin1String view("view");
	static const QLatin1String area_hatching_enabled("area_hatching_enabled");
	static const QLatin1String baseline_view_enabled("baseline_view_enabled");
	static const QLatin1String grid("grid");
	static const QLatin1String map_view("map_view");
	
	static const QLatin1String print("print");
	
	static const QLatin1String undo("undo");
	static const QLatin1String redo("redo");
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
	
	int current_version = XMLFileFormat::current_version;
	bool retain_compatibility = Settings::getInstance().getSetting(Settings::General_RetainCompatiblity).toBool();
	XMLFileFormat::active_version = retain_compatibility ? current_version-1 : current_version;
	
	xml.writeDefaultNamespace(XMLFileFormat::mapper_namespace);
	xml.writeStartDocument();
	
	{
		XmlElementWriter map_element(xml, literal::map);
		map_element.writeAttribute(literal::version, XMLFileFormat::active_version);
		
		xml.writeTextElement(literal::notes, map->getMapNotes());
		
		exportGeoreferencing();
		exportColors();
		exportSymbols();
		exportMapParts();
		exportTemplates();
		exportView();
		exportPrint();
		exportUndo();
		exportRedo();
	}
	
	xml.writeEndDocument();
}

void XMLFileExporter::exportGeoreferencing()
{
	map->getGeoreferencing().save(xml);
}

void XMLFileExporter::exportColors()
{
	XmlElementWriter all_colors_element(xml, literal::colors);
	std::size_t num_colors = map->color_set->colors.size();
	all_colors_element.writeAttribute(literal::count, num_colors);
	for (std::size_t i = 0; i < num_colors; ++i)
	{
		MapColor* color = map->color_set->colors[i];
		const MapColorCmyk &cmyk = color->getCmyk();
		XmlElementWriter color_element(xml, literal::color);
		color_element.writeAttribute(literal::priority, color->getPriority());
		color_element.writeAttribute(literal::name, color->getName());
		color_element.writeAttribute(literal::c, cmyk.c, 3);
		color_element.writeAttribute(literal::m, cmyk.m, 3);
		color_element.writeAttribute(literal::y, cmyk.y, 3);
		color_element.writeAttribute(literal::k, cmyk.k, 3);
		color_element.writeAttribute(literal::opacity, color->getOpacity(), 3);
		
		if (color->getSpotColorMethod() != MapColor::UndefinedMethod)
		{
			XmlElementWriter spotcolors_element(xml, literal::spotcolors);
			spotcolors_element.writeAttribute(literal::knockout, color->getKnockout());
			SpotColorComponent component;
			switch(color->getSpotColorMethod())
			{
				case MapColor::SpotColor:
					xml.writeTextElement(literal::namedcolor, color->getSpotColorName());
					break;
				case MapColor::CustomColor:
					Q_FOREACH(component, color->getComponents())
					{
						XmlElementWriter component_element(xml, literal::component);
						component_element.writeAttribute(literal::factor, component.factor);
						component_element.writeAttribute(literal::spotcolor, component.spot_color->getPriority());
					}
				default:
					; // nothing
			}
		}
		
		{
			XmlElementWriter cmyk_element(xml, literal::cmyk);
			switch(color->getCmykColorMethod())
			{
				case MapColor::SpotColor:
					cmyk_element.writeAttribute(literal::method, literal::spotcolor);
					break;
				case MapColor::RgbColor:
					cmyk_element.writeAttribute(literal::method, literal::rgb);
					break;
				default:
					cmyk_element.writeAttribute(literal::method, literal::custom);
			}
		}
		
		{
			XmlElementWriter rgb_element(xml, literal::rgb);
			switch(color->getCmykColorMethod())
			{
				case MapColor::SpotColor:
					rgb_element.writeAttribute(literal::method, literal::spotcolor);
					break;
				case MapColor::CmykColor:
					rgb_element.writeAttribute(literal::method, literal::cmyk);
					break;
				default:
					rgb_element.writeAttribute(literal::method, literal::custom);
			}
			const MapColorRgb &rgb = color->getRgb();
			rgb_element.writeAttribute(literal::r, rgb.r, 3);
			rgb_element.writeAttribute(literal::g, rgb.g, 3);
			rgb_element.writeAttribute(literal::b, rgb.b, 3);
		}
	}
}

void XMLFileExporter::exportSymbols()
{
	XmlElementWriter symbols_element(xml, literal::symbols);
	int num_symbols = map->getNumSymbols();
	symbols_element.writeAttribute(literal::count, num_symbols);
	for (int i = 0; i < num_symbols; ++i)
	{
		map->getSymbol(i)->save(xml, *map);
	}
}

void XMLFileExporter::exportMapParts()
{
	XmlElementWriter parts_element(xml, literal::parts);
	
	int num_parts = map->getNumParts();
	parts_element.writeAttribute(literal::count, num_parts);
	parts_element.writeAttribute(literal::current, map->current_part_index);
	for (int i = 0; i < num_parts; ++i)
		map->getPart(i)->save(xml, *map);
}

void XMLFileExporter::exportTemplates()
{
	XmlElementWriter templates_element(xml, literal::templates);
	
	int num_templates = map->getNumTemplates() + map->getNumClosedTemplates();
	templates_element.writeAttribute(literal::count, num_templates);
	templates_element.writeAttribute(literal::first_front_template, map->first_front_template);
	for (int i = 0; i < map->getNumTemplates(); ++i)
		map->getTemplate(i)->saveTemplateConfiguration(xml, true);
	for (int i = 0; i < map->getNumClosedTemplates(); ++i)
		map->getClosedTemplate(i)->saveTemplateConfiguration(xml, false);
	
	{
		XmlElementWriter defaults_element(xml, literal::defaults);
		defaults_element.writeAttribute(literal::use_meters_per_pixel, map->image_template_use_meters_per_pixel);
		defaults_element.writeAttribute(literal::meters_per_pixel, map->image_template_meters_per_pixel);
		defaults_element.writeAttribute(literal::dpi, map->image_template_dpi);
		defaults_element.writeAttribute(literal::scale, map->image_template_scale);
	}
}

void XMLFileExporter::exportView()
{
	XmlElementWriter view_element(xml, literal::view);
	
	view_element.writeAttribute(literal::area_hatching_enabled, map->area_hatching_enabled);
	view_element.writeAttribute(literal::baseline_view_enabled, map->baseline_view_enabled);
	
	map->getGrid().save(xml);
	
	if (view)
		view->save(xml, literal::map_view);
}

void XMLFileExporter::exportPrint()
{
	map->printerConfig().save(xml, literal::print);
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
	if (!xml.readNextStartElement() || xml.name() != literal::map)
	{
		xml.raiseError(Importer::tr("Unsupported file format."));
	}
	
	XmlElementReader map_element(xml);
	const int version = map_element.attribute<int>(literal::version);
	if (version < 1)
		xml.raiseError(Importer::tr("Invalid file format version."));
	else if (version < XMLFileFormat::minimum_version)
		xml.raiseError(Importer::tr("Unsupported old file format version. Please use an older program version to load and update the file."));
	else if (version > XMLFileFormat::current_version)
		addWarning(Importer::tr("Unsupported new file format version. Some map features will not be loaded or saved by this version of the program."));
	
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		
		if (name == literal::colors)
			importColors();
		else if (name == literal::symbols)
			importSymbols();
		else if (name == literal::georeferencing)
			importGeoreferencing(load_symbols_only);
		else if (load_symbols_only)
			xml.skipCurrentElement();
		else if (name == literal::notes)
			map->setMapNotes(xml.readElementText());
		else if (name == literal::parts)
			importMapParts();
		else if (name == literal::templates)
			importTemplates();
		else if (name == literal::view)
			importView();
		else if (name == literal::print)
			importPrint();
		else if (name == literal::undo)
			importUndo();
		else if (name == literal::redo)
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
	Q_ASSERT(xml.name() == literal::georeferencing);
	
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
	XmlElementReader all_colors_element(xml);
	int num_colors = all_colors_element.attribute<int>(literal::count);
	
	Map::ColorVector& colors(map->color_set->colors);
	colors.reserve(qMin(num_colors, 100)); // 100 is not a limit
	XMLFileImporterColorBacklog backlog;
	backlog.reserve(colors.size());
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::color)
		{
			XmlElementReader color_element(xml);
			MapColor* color = new MapColor(
			  color_element.attribute<QString>(literal::name),
			  color_element.attribute<int>(literal::priority) );
			if (color_element.hasAttribute(literal::opacity))
				color->setOpacity(color_element.attribute<float>(literal::opacity));
			
			MapColorCmyk cmyk;
			cmyk.c = color_element.attribute<float>(literal::c);
			cmyk.m = color_element.attribute<float>(literal::m);
			cmyk.y = color_element.attribute<float>(literal::y);
			cmyk.k = color_element.attribute<float>(literal::k);
			color->setCmyk(cmyk);
			
			SpotColorComponents components;
			QString cmyk_method;
			QString rgb_method;
			MapColorRgb rgb;
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::spotcolors)
				{
					XmlElementReader spotcolors_element(xml);
					color->setKnockout(spotcolors_element.attribute<bool>(literal::knockout));
					
					while(xml.readNextStartElement())
					{
						if (xml.name() == literal::namedcolor)
						{
							color->setSpotColorName(xml.readElementText());
						}
						else if (xml.name() == literal::component)
						{
							XmlElementReader component_element(xml);
							SpotColorComponent component;
							component.factor = component_element.attribute<float>(literal::factor);
							// We can't know if the spot color is already loaded. Create a temporary proxy.
							component.spot_color = new MapColor(component_element.attribute<int>(literal::spotcolor));
							components.push_back(component);
						}
						else
							xml.skipCurrentElement(); // unsupported
					}
				}
				else if (xml.name() == literal::cmyk)
				{
					XmlElementReader cmyk_element(xml);
					cmyk_method = cmyk_element.attribute<QString>(literal::method);
				}
				else if (xml.name() == literal::rgb)
				{
					XmlElementReader rgb_element(xml);
					rgb_method = rgb_element.attribute<QString>(literal::method);
					rgb.r = rgb_element.attribute<float>(literal::r);
					rgb.g = rgb_element.attribute<float>(literal::g);
					rgb.b = rgb_element.attribute<float>(literal::b);
					color->setRgbFromSpotColors();
				}
				else
					xml.skipCurrentElement(); // unsupported
			}
			
			if (!components.empty())
			{
				backlog.push_back(XMLFileImporterColorBacklogItem(color));
				XMLFileImporterColorBacklogItem& item = backlog.back();
				item.components = components;
				item.cmyk_from_spot = (cmyk_method == literal::spotcolor);
				item.rgb_from_spot = (rgb_method == literal::spotcolor);
			}
			
			if (cmyk_method == literal::rgb)
				color->setCmykFromRgb();
			
			if (rgb_method == literal::cmyk)
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
	XmlElementReader symbols_element(xml);
	int num_symbols = symbols_element.attribute<int>(literal::count);
	map->symbols.reserve(qMin(num_symbols, 1000)); // 1000 is not a limit
	
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedPoint()))] = map->getUndefinedPoint();
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedLine()))] = map->getUndefinedLine();
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::symbol)
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
	XmlElementReader mapparts_element(xml);
	int num_parts = mapparts_element.attribute<int>(literal::parts);
	int current_part_index = mapparts_element.attribute<int>(literal::current);
	map->parts.clear();
	map->parts.reserve(qMin(num_parts, 20)); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::part)
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
	Q_ASSERT(xml.name() == literal::templates);
	
	XmlElementReader templates_element(xml);
	int first_front_template = templates_element.attribute<int>(literal::first_front_template);
	
	int num_templates = templates_element.attribute<int>(literal::count);
	map->templates.reserve(qMin(num_templates, 20)); // 20 is not a limit
	map->closed_templates.reserve(qMin(num_templates, 20)); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::template_string)
		{
			bool opened = true;
			Template* temp = Template::loadTemplateConfiguration(xml, *map, opened);
			if (opened)
				map->templates.push_back(temp);
			else
				map->closed_templates.push_back(temp);
		}
		else if (xml.name() == literal::defaults)
		{
			XmlElementReader defaults_element(xml);
			map->image_template_use_meters_per_pixel = defaults_element.attribute<bool>(literal::use_meters_per_pixel);
			map->image_template_meters_per_pixel = defaults_element.attribute<double>(literal::meters_per_pixel);
			map->image_template_dpi = defaults_element.attribute<double>(literal::dpi);
			map->image_template_scale = defaults_element.attribute<double>(literal::scale);
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
	Q_ASSERT(xml.name() == literal::view);
	
	XmlElementReader view_element(xml);
	map->area_hatching_enabled = view_element.attribute<bool>(literal::area_hatching_enabled);
	map->baseline_view_enabled = view_element.attribute<bool>(literal::baseline_view_enabled);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::grid)
			map->getGrid().load(xml);
		else if (xml.name() == literal::map_view)
			view->load(xml);
		else
			xml.skipCurrentElement(); // unsupported
	}
}

void XMLFileImporter::importPrint()
{
	Q_ASSERT(xml.name() == literal::print);
	
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
