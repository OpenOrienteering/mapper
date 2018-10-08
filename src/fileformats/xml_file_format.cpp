/*
 *    Copyright 2012 Pete Curtis
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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

#include "xml_file_format.h"
#include "xml_file_format_p.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include <QtGlobal>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QExplicitlySharedDataPointer>
#include <QFileInfo>
#include <QFlags>
#include <QIODevice>
#include <QLatin1String>
#include <QObject>
#include <QRectF>
#include <QScopedValueRollback>
#include <QString>
#include <QStringRef>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_grid.h"
#include "core/map_part.h"
#include "core/map_printer.h"  // IWYU pragma: keep
#include "core/map_view.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_import_export.h"
#include "templates/template.h"
#include "undo/undo_manager.h"
#include "util/xml_stream_util.h"


namespace OpenOrienteering {

// ### XMLFileFormat definition ###

constexpr int XMLFileFormat::minimum_version = 2;
constexpr int XMLFileFormat::current_version = 8;

int XMLFileFormat::active_version = 5; // updated by XMLFileExporter::doExport()



namespace {

QString mapperNamespace()
{
	// Use a https URL with next major version
	return QStringLiteral("http://openorienteering.org/apps/mapper/xml/v2");
}


}  // namespace



XMLFileFormat::XMLFileFormat()
 : FileFormat(MapFile,
              "XML",
              ::OpenOrienteering::ImportExport::tr("OpenOrienteering Mapper"),
              QString::fromLatin1("omap"),
              Feature::FileOpen | Feature::FileImport |
              Feature::FileSave | Feature::FileSaveAs )
{
	addExtension(QString::fromLatin1("xmap"));
}


FileFormat::ImportSupportAssumption XMLFileFormat::understands(const char* buffer, int size) const
{
	const auto data = QByteArray::fromRawData(buffer, size);
	if (size >= 4 && qstrncmp(buffer, "OMAP", 4) == 0)
	    return FullySupported;  // Legacy binary format. Final error raised in doImport().
	
	if (size > 38)  // length of "<?xml ...>"
	{
		QXmlStreamReader xml(data);
		if (xml.readNextStartElement())
		{
			if (xml.name() != QLatin1String("map"))
				return NotSupported;
			else if (xml.namespaceUri() == mapperNamespace())
				return FullySupported;
			else if (xml.namespaceUri() == QLatin1String("http://oorienteering.sourceforge.net/mapper/xml/v2"))
			    return FullySupported;
		}
	}
	auto trimmed = data.trimmed();
	if (!trimmed.isEmpty() && !trimmed.startsWith('<'))
		return NotSupported;
		
	return Unknown;
}


std::unique_ptr<Importer> XMLFileFormat::makeImporter(const QString& path, Map* map, MapView* view) const
{
	return std::make_unique<XMLFileImporter>(path, map, view);
}

std::unique_ptr<Exporter> XMLFileFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	return std::make_unique<XMLFileExporter>(path, map, view);
}



// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String map("map");
	static const QLatin1String version("version");
	static const QLatin1String notes("notes");
	
	static const QLatin1String barrier("barrier");
	static const QLatin1String required("required");
	
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
	static const QLatin1String screen_angle("screen_angle");
	static const QLatin1String screen_frequency("screen_frequency");
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
	static const QLatin1String id("id");
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

XMLFileExporter::XMLFileExporter(const QString& path, const Map* map, const MapView* view)
: Exporter(path, map, view)
{
	// Determine auto-formatting default from filename, if possible.
	bool auto_formatting = path.endsWith(QLatin1String(".xmap"));
	setOption(QString::fromLatin1("autoFormatting"), auto_formatting);
}

bool XMLFileExporter::exportImplementation()
{
	xml.setDevice(device());
	
	if (option(QString::fromLatin1("autoFormatting")).toBool())
		xml.setAutoFormatting(true);
	
#ifdef MAPPER_ENABLE_COMPATIBILITY
	int current_version = XMLFileFormat::current_version;
	bool retain_compatibility = Settings::getInstance().getSetting(Settings::General_RetainCompatiblity).toBool();
	XMLFileFormat::active_version = retain_compatibility ? 5 : current_version;
	
	if (XMLFileFormat::active_version < 6 && map->getNumParts() != 1)
	{
		throw FileFormatException(tr("Older versions of Mapper do not support multiple map parts. To save the map in compatibility mode, you must first merge all map parts."));
	}
#else
	XMLFileFormat::active_version = XMLFileFormat::current_version;
#endif
	
	xml.writeDefaultNamespace(mapperNamespace());
	xml.writeStartDocument();
	writeLineBreak(xml);
	
	{
		XmlElementWriter map_element(xml, literal::map);
		map_element.writeAttribute(literal::version, XMLFileFormat::active_version);
		writeLineBreak(xml);
		
		xml.writeTextElement(literal::notes, map->getMapNotes());
		writeLineBreak(xml);
		
		exportGeoreferencing();
		exportColors();
		writeLineBreak(xml);

		XmlElementWriter* barrier = nullptr;
		if (XMLFileFormat::active_version >= 6)
		{
			// Prevent Mapper versions < 0.6.0 from crashing
			// when compatibilty mode is NOT activated
			// Incompatible feature: dense coordinates
			barrier = new XmlElementWriter(xml, literal::barrier);
			barrier->writeAttribute(literal::version, 6);
			barrier->writeAttribute(literal::required, "0.6.0");
			writeLineBreak(xml);
		}
		exportSymbols();
		writeLineBreak(xml);
		exportMapParts();
		writeLineBreak(xml);
		exportTemplates();
		writeLineBreak(xml);
		exportView();
		writeLineBreak(xml);
		exportPrint();
		delete barrier;
		writeLineBreak(xml);

		if (Settings::getInstance().getSetting(Settings::General_SaveUndoRedo).toBool())
		{
			{
				// Prevent Mapper versions < 0.6.0 from crashing
				// when compatibilty mode IS activated
				// Incompatible feature: new undo step types
				XmlElementWriter barrier(xml, literal::barrier);
				barrier.writeAttribute(literal::version, 6);
				barrier.writeAttribute(literal::required, "0.6.0");
				writeLineBreak(xml);
				exportUndo();
				exportRedo();
			}
			writeLineBreak(xml);
		}
	}
	
	xml.writeEndDocument();
	return true;
}

void XMLFileExporter::exportGeoreferencing()
{
	map->getGeoreferencing().save(xml);
	writeLineBreak(xml);
}

void XMLFileExporter::exportColors()
{
	XmlElementWriter all_colors_element(xml, literal::colors);
	std::size_t num_colors = map->color_set->colors.size();
	all_colors_element.writeAttribute(literal::count, num_colors);
	
	for (std::size_t i = 0; i < num_colors; ++i)
	{
		writeLineBreak(xml);
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
					{
						XmlElementWriter color_element(xml, literal::namedcolor);
						if (color->getScreenFrequency() > 0)
						{
							color_element.writeAttribute(literal::screen_angle, color->getScreenAngle(), 1);
							color_element.writeAttribute(literal::screen_frequency, color->getScreenFrequency(), 1);
						}
						xml.writeCharacters(color->getSpotColorName());
					}
					break;
				case MapColor::CustomColor:
					for (auto&& component : color->getComponents())
					{
						XmlElementWriter component_element(xml, literal::component);
						component_element.writeAttribute(literal::factor, component.factor);
						component_element.writeAttribute(literal::spotcolor, component.spot_color->getPriority());
					}
					break;
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
			switch(color->getRgbColorMethod())
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
	writeLineBreak(xml);
}

void XMLFileExporter::exportSymbols()
{
	XmlElementWriter symbols_element(xml, literal::symbols);
	auto id = map->symbolSetId();
	int num_symbols = map->getNumSymbols();
	symbols_element.writeAttribute(literal::count, num_symbols);
	if (!id.isEmpty())
		symbols_element.writeAttribute(literal::id, id);
	for (int i = 0; i < num_symbols; ++i)
	{
		writeLineBreak(xml);
		map->getSymbol(i)->save(xml, *map);
	}
	writeLineBreak(xml);
}

void XMLFileExporter::exportMapParts()
{
	XmlElementWriter parts_element(xml, literal::parts);
	
	auto num_parts = std::size_t(map->getNumParts());
	parts_element.writeAttribute(literal::count, num_parts);
	parts_element.writeAttribute(literal::current, map->current_part_index);
	for (auto i = 0u; i < num_parts; ++i)
	{
		writeLineBreak(xml);
		map->getPart(i)->save(xml);
	}
	writeLineBreak(xml);
}

void XMLFileExporter::exportTemplates()
{
	QDir map_dir;
	const QDir* map_dir_ptr = nullptr;
	if (!path.isEmpty())
	{
		map_dir = QFileInfo(path).absoluteDir();
		map_dir_ptr = &map_dir;
		
		/// \todo Update the relative paths in memory when saving to another directory.
		///       Otherwise opening templates in the reloaded saved map may
		///       behave different from the current map in memory.
	}
	
	XmlElementWriter templates_element(xml, literal::templates);
	
	int num_templates = map->getNumTemplates() + map->getNumClosedTemplates();
	templates_element.writeAttribute(literal::count, num_templates);
	templates_element.writeAttribute(literal::first_front_template, map->first_front_template);
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		writeLineBreak(xml);
		map->getTemplate(i)->saveTemplateConfiguration(xml, true, map_dir_ptr);
	}
	for (int i = 0; i < map->getNumClosedTemplates(); ++i)
	{
		writeLineBreak(xml);
		map->getClosedTemplate(i)->saveTemplateConfiguration(xml, false, map_dir_ptr);
	}
	
	{
		writeLineBreak(xml);
		XmlElementWriter defaults_element(xml, literal::defaults);
		defaults_element.writeAttribute(literal::use_meters_per_pixel, map->image_template_use_meters_per_pixel);
		defaults_element.writeAttribute(literal::meters_per_pixel, map->image_template_meters_per_pixel);
		defaults_element.writeAttribute(literal::dpi, map->image_template_dpi);
		defaults_element.writeAttribute(literal::scale, map->image_template_scale);
	}
	writeLineBreak(xml);
}

void XMLFileExporter::exportView()
{
	XmlElementWriter view_element(xml, literal::view);
	view_element.writeAttribute(literal::area_hatching_enabled, bool(map->renderable_options & Symbol::RenderAreasHatched));
	view_element.writeAttribute(literal::baseline_view_enabled, bool(map->renderable_options & Symbol::RenderBaselines));
	
	writeLineBreak(xml);
	map->getGrid().save(xml);
	
	if (view)
	{
		writeLineBreak(xml);
		view->save(xml, literal::map_view);
	}
	writeLineBreak(xml);
}

void XMLFileExporter::exportPrint()
{
	if (map->hasPrinterConfig())
	{
		map->printerConfig().save(xml, literal::print);
		writeLineBreak(xml);
	}
}

void XMLFileExporter::exportUndo()
{
	map->undoManager().saveUndo(xml);
	writeLineBreak(xml);
}

void XMLFileExporter::exportRedo()
{
	map->undoManager().saveRedo(xml);
	writeLineBreak(xml);
}



// ### XMLFileImporter definition ###

XMLFileImporter::XMLFileImporter(const QString& path, Map *map, MapView *view)
: Importer(path, map, view)
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

bool XMLFileImporter::importImplementation()
{
	xml.setDevice(device());
	if (!xml.readNextStartElement() || xml.name() != literal::map)
	{
		if (device()->seek(0))
		{
			char data[4] = {};
			device()->read(data, 4);
			if (qstrncmp(reinterpret_cast<const char*>(data), "OMAP", 4) == 0)
			{
				throw FileFormatException(::OpenOrienteering::Importer::tr(
				  "Unsupported obsolete file format version. "
				  "Please use program version v%1 or older "
				  "to load and update the file.").arg(QLatin1String("0.8")));
			}
		}
		throw FileFormatException(::OpenOrienteering::Importer::tr("Unsupported file format."));
	}
	
	XmlElementReader map_element(xml);
	const int version = map_element.attribute<int>(literal::version);
	if (version < 1)
		xml.raiseError(::OpenOrienteering::Importer::tr("Invalid file format version."));
	else if (version < XMLFileFormat::minimum_version)
		xml.raiseError(::OpenOrienteering::Importer::tr("Unsupported old file format version. Please use an older program version to load and update the file."));
	else if (version > XMLFileFormat::current_version)
		addWarning(::OpenOrienteering::Importer::tr("Unsupported new file format version. Some map features will not be loaded or saved by this version of the program."));
	
	QScopedValueRollback<MapCoord::BoundsOffset> rollback { MapCoord::boundsOffset() };
	MapCoord::boundsOffset().reset(true);
	georef_offset_adjusted = false;
	importElements();
	
	auto offset = MapCoord::boundsOffset();
	if (!loadSymbolsOnly() && !offset.isZero())
	{
		addWarning(tr("Some coordinates were out of bounds for printing. Map content was adjusted."));
		
		MapCoordF offset_f { offset.x / 1000.0, offset.y / 1000.0 };
		
		// Apply the offset
		auto printer_config = map->printerConfig();
		auto& print_area = printer_config.print_area;
		print_area.translate( -offset_f );
		
		// Verify the adjusted print area, and readjust if necessary
		if (print_area.top() <= -1000000.0 || print_area.bottom() > 1000000.0)
			print_area.moveTop(-print_area.width() / 2);
		if (print_area.left() <= -1000000.0 || print_area.right() > 1000000.0)
			print_area.moveLeft(-print_area.width() / 2);
		
		map->setPrinterConfig(printer_config);
		
		if (!georef_offset_adjusted)
		{
			// We need to adjust the georeferencing.
			auto georef = map->getGeoreferencing();
			auto ref_point = MapCoordF { georef.getMapRefPoint() };
			auto new_projected = georef.toProjectedCoords(ref_point + offset_f);
			georef.setProjectedRefPoint(new_projected, false);
			map->setGeoreferencing(georef);
		}
	}
	return true;
}

void XMLFileImporter::importElements()
{
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		
		if (name == literal::colors)
			importColors();
		else if (name == literal::symbols)
			importSymbols();
		else if (name == literal::georeferencing)
			importGeoreferencing();
		else if (name == literal::view)
			importView();
		else if (name == literal::barrier)
		{
			XmlElementReader barrier(xml);
			if (barrier.attribute<int>(literal::version) > XMLFileFormat::current_version)
			{
				QString required_version = barrier.attribute<QString>(literal::required);
				if (required_version.isEmpty())
					required_version = tr("unknown");
				addWarning(tr("Parts of this file cannot be read by this version of Mapper. Minimum required version: %1").arg(required_version));
				xml.skipCurrentElement();
			}
			else
			{
				importElements();
			}
		}
		else if (loadSymbolsOnly())
			xml.skipCurrentElement();
		/******************************************************
		* The remainder is skipped when loading a symbol set! *
		******************************************************/
		else if (name == literal::notes)
			importMapNotes();
		else if (name == literal::parts)
			importMapParts();
		else if (name == literal::templates)
			importTemplates();
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
		throw FileFormatException(
		        tr("Error at line %1 column %2: %3")
		        .arg(xml.lineNumber())
		        .arg(xml.columnNumber())
		        .arg(xml.errorString()) );
}

void XMLFileImporter::importMapNotes()
{
	auto recovery = XmlRecoveryHelper(xml);
	map->setMapNotes(xml.readElementText());
	if (xml.hasError() && recovery())
	{
		addWarning(tr("Some invalid characters had to be removed."));
		map->setMapNotes(xml.readElementText());
	}
}

void XMLFileImporter::importGeoreferencing()
{
	Q_ASSERT(xml.name() == literal::georeferencing);
	
	bool check_for_offset = MapCoord::boundsOffset().check_for_offset;
	
	Georeferencing georef;
	georef.load(xml, loadSymbolsOnly());
	map->setGeoreferencing(georef);
	if (!georef.isValid())
	{
		QString error_text = georef.getErrorText();
		if (error_text.isEmpty())
			error_text = tr("Unknown error");
		addWarning(tr("Unsupported or invalid georeferencing specification '%1': %2").
		           arg(georef.getProjectedCRSSpec(), error_text));
	}
	
	if (MapCoord::boundsOffset().isZero())
		// Georeferencing was not adjusted on import.
		MapCoord::boundsOffset().reset(check_for_offset);
	else if (check_for_offset)
		// Georeferencing was adjusted on import, before other coordinates.
		georef_offset_adjusted = true;
}

/** Helper for delayed actions */
struct XMLFileImporterColorBacklogItem
{
	MapColor* color; // color which needs updating
	SpotColorComponents components; // components of the color
	bool knockout;
	bool cmyk_from_spot; // determine CMYK from spot
	bool rgb_from_spot; // determine RGB from spot
	
	XMLFileImporterColorBacklogItem(MapColor* color)
	: color(color), knockout(false), cmyk_from_spot(false), rgb_from_spot(false)
	{}
};
typedef std::vector<XMLFileImporterColorBacklogItem> XMLFileImporterColorBacklog;
	

void XMLFileImporter::importColors()
{
	XmlElementReader all_colors_element(xml);
	auto num_colors = all_colors_element.attribute<std::size_t>(literal::count);
	
	Map::ColorVector& colors(map->color_set->colors);
	colors.reserve(qMin(num_colors, std::size_t(100))); // 100 is not a limit
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
			
			bool knockout = false;
			SpotColorComponents components;
			QString cmyk_method;
			QString rgb_method;
			MapColorRgb rgb;
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::spotcolors)
				{
					XmlElementReader spotcolors_element(xml);
					knockout = spotcolors_element.attribute<bool>(literal::knockout);
					
					while(xml.readNextStartElement())
					{
						if (xml.name() == literal::namedcolor)
						{
							XmlElementReader color_element(xml);
							if (color_element.hasAttribute(literal::screen_frequency))
							{
								color->setScreenAngle(color_element.attribute<double>(literal::screen_angle));
								color->setScreenFrequency(std::max(0.0, color_element.attribute<double>(literal::screen_frequency)));
							}
							color->setSpotColorName(xml.readElementText());
							color->setKnockout(knockout);
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
				}
				else
				{
					xml.skipCurrentElement(); // unsupported
				}
			}
			
			if (cmyk_method == literal::custom)
			{
				color->setCmyk(cmyk);
				if (rgb_method == literal::cmyk)
					color->setRgbFromCmyk();
			}
			
			if (rgb_method == literal::custom)
			{
				color->setRgb(rgb);
				if (cmyk_method == literal::rgb)
					color->setCmykFromRgb();
			}
			
			if (!components.empty())
			{
				backlog.push_back(XMLFileImporterColorBacklogItem(color));
				XMLFileImporterColorBacklogItem& item = backlog.back();
				item.components = components;
				item.knockout = knockout;
				item.cmyk_from_spot = (cmyk_method == literal::spotcolor);
				item.rgb_from_spot = (rgb_method == literal::spotcolor);
			}
			else if (knockout && !color->getKnockout())
			{
				addWarning(tr("Could not set knockout property of color '%1'.").arg(color->getName()));
			}
			
			colors.push_back(color);
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (num_colors > 0 && num_colors != colors.size())
		addWarning(tr("Expected %1 colors, found %2.").
		  arg(num_colors).
		  arg(colors.size())
		);
	
	// All spot colors are loaded at this point.
	// Now deal with depending color compositions from the backlog.
	for (auto&& item : backlog)
	{
		// Process the list of spot color components.
		SpotColorComponents out_components;
		for (auto&& in_component : item.components)
		{
			const MapColor* out_color = map->getColor(in_component.spot_color->getPriority());
			if (!out_color || out_color->getSpotColorMethod() != MapColor::SpotColor)
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
		item.color->setKnockout(item.knockout);
		if (item.cmyk_from_spot)
			item.color->setCmykFromSpotColors();
		if (item.rgb_from_spot)
			item.color->setRgbFromSpotColors();
		
		if (item.knockout && !item.color->getKnockout())
		{
			addWarning(tr("Could not set knockout property of color '%1'.").arg(item.color->getName()));
		}
	}
}

void XMLFileImporter::importSymbols()
{
	QScopedValueRollback<MapCoord::BoundsOffset> offset { MapCoord::boundsOffset() };
	MapCoord::boundsOffset().reset(false);
	
	XmlElementReader symbols_element(xml);
	map->setSymbolSetId(symbols_element.attribute<QString>(literal::id));
	auto num_symbols = symbols_element.attribute<std::size_t>(literal::count);
	map->symbols.reserve(qMin(num_symbols, std::size_t(1000))); // 1000 is not a limit
	
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedPoint()))] = map->getUndefinedPoint();
	symbol_dict[QString::number(map->findSymbolIndex(map->getUndefinedLine()))] = map->getUndefinedLine();
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::symbol)
		{
			map->symbols.push_back(Symbol::load(xml, *map, symbol_dict).release());
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (num_symbols > 0 && num_symbols != map->symbols.size())
		addWarning(tr("Expected %1 symbols, found %2.").
		  arg(num_symbols).
		  arg(map->symbols.size())
		);
}

void XMLFileImporter::importMapParts()
{
	XmlElementReader mapparts_element(xml);
	auto num_parts = mapparts_element.attribute<std::size_t>(literal::count);
	auto current_part_index = mapparts_element.attribute<std::size_t>(literal::current);
	map->parts.clear();
	map->parts.reserve(qMin(num_parts, std::size_t(20))); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::part)
		{
			auto recovery = XmlRecoveryHelper(xml);
			auto part = MapPart::load(xml, *map, symbol_dict);
			if (xml.hasError() && recovery())
			{
				addWarning(tr("Some invalid characters had to be removed."));
				delete part;
				part = MapPart::load(xml, *map, symbol_dict);
			}
			map->parts.push_back(part);
		}
		else
		{
			addWarningUnsupportedElement();
			xml.skipCurrentElement();
		}
	}
	
	if (current_part_index < map->parts.size())
		map->current_part_index = current_part_index;
	
	if (num_parts > 0 && num_parts != map->parts.size())
		addWarning(tr("Expected %1 map parts, found %2.").
		  arg(num_parts).
		  arg(map->parts.size())
		);
	
	emit map->currentMapPartIndexChanged(map->current_part_index);
	emit map->currentMapPartChanged(map->getPart(map->current_part_index));
}

void XMLFileImporter::importTemplates()
{
	Q_ASSERT(xml.name() == literal::templates);
	
	XmlElementReader templates_element(xml);
	int first_front_template = templates_element.attribute<int>(literal::first_front_template);
	
	auto num_templates = templates_element.attribute<std::size_t>(literal::count);
	map->templates.reserve(qMin(num_templates, std::size_t(20))); // 20 is not a limit
	map->closed_templates.reserve(qMin(num_templates, std::size_t(20))); // 20 is not a limit
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::template_string)
		{
			bool opened = true;
			auto temp = Template::loadTemplateConfiguration(xml, *map, opened);
			if (opened)
				map->templates.push_back(temp.release());
			else
				map->closed_templates.push_back(temp.release());
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
			qDebug("Unsupported element: %s", qPrintable(xml.qualifiedName().toString()));
			xml.skipCurrentElement();
		}
	}
	
	map->first_front_template = qMax(0, qMin(map->getNumTemplates(), first_front_template));
}

void XMLFileImporter::importView()
{
	Q_ASSERT(xml.name() == literal::view);
	
	XmlElementReader view_element(xml);
	if (view_element.attribute<bool>(literal::area_hatching_enabled))
		map->renderable_options |= Symbol::RenderAreasHatched;
	if (view_element.attribute<bool>(literal::baseline_view_enabled))
		map->renderable_options |= Symbol::RenderBaselines;
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::grid)
		{
			map->setGrid(MapGrid().load(xml));
		}
		else if (xml.name() == literal::map_view)
		{
			if (view)
				view->load(xml);
		}
		else
		{
			xml.skipCurrentElement(); // unsupported
		}
	}
}

void XMLFileImporter::importPrint()
{
	Q_ASSERT(xml.name() == literal::print);
	
	try
	{
		map->setPrinterConfig(MapPrinterConfig(*map, xml));
	}
	catch (FileFormatException& e)
	{
		addWarning(::OpenOrienteering::ImportExport::tr("Error while loading the printing configuration at %1:%2: %3")
		           .arg(xml.lineNumber()).arg(xml.columnNumber()).arg(e.message()));
	}
}

void XMLFileImporter::importUndo()
{
	if (!Settings::getInstance().getSetting(Settings::General_SaveUndoRedo).toBool())
	{
		xml.skipCurrentElement();
		return;
	}
	
	try
	{
		map->undoManager().loadUndo(xml, symbol_dict);
	}
	catch (FileFormatException& e)
	{
		addWarning(::OpenOrienteering::ImportExport::tr("Error while loading the undo/redo steps at %1:%2: %3")
		           .arg(xml.lineNumber()).arg(xml.columnNumber()).arg(e.message()));
		map->undoManager().clear();
	}
}

void XMLFileImporter::importRedo()
{
	if (!Settings::getInstance().getSetting(Settings::General_SaveUndoRedo).toBool())
	{
		xml.skipCurrentElement();
		return;
	}
	
	try
	{
		map->undoManager().loadRedo(xml, symbol_dict);
	}
	catch (FileFormatException& e)
	{
		addWarning(::OpenOrienteering::ImportExport::tr("Error while loading the undo/redo steps at %1:%2: %3")
		           .arg(xml.lineNumber()).arg(xml.columnNumber()).arg(e.message()));
		map->undoManager().clear();
	}
}


}  // namespace OpenOrienteering
