/*
 *    Copyright 2012, 2013 Pete Curtis
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

#include "ocad8_file_format.h"
#include "ocad8_file_format_p.h"

#include <QtMath>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QRgb>
#include <QTextCodec>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/symbol.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/xml_file_format.h"
#include "fileformats/file_import_export.h"
#include "fileformats/ocd_file_format.h"
#include "templates/template.h"
#include "templates/template_image.h"
#include "templates/template_map.h"
#include "util/encoding.h"
#include "util/util.h"


namespace OpenOrienteering {

// ### OCAD8FileFormat ###

OCAD8FileFormat::OCAD8FileFormat()
 : FileFormat(MapFile,
              "OCAD78",
              ::OpenOrienteering::ImportExport::tr("OCAD Versions 7, 8"),
              QString::fromLatin1("ocd"),
              Feature::FileOpen | Feature::FileImport | Feature::ReadingLossy |
              Feature::FileSave | Feature::FileSaveAs | Feature::WritingLossy )
{
	// Nothing
}


FileFormat::ImportSupportAssumption OCAD8FileFormat::understands(const char* buffer, int size) const
{
    // The first two bytes of the file must be AD 0C.
	if (size < 2)
		return Unknown;
	else if (quint8(buffer[0]) == 0xAD && buffer[1] == 0x0C)
		return FullySupported;
	else
		return NotSupported;
}


std::unique_ptr<Importer> OCAD8FileFormat::makeImporter(const QString& path, Map *map, MapView *view) const
{
	return std::make_unique<OCAD8FileImport>(path, map, view);
}

std::unique_ptr<Exporter> OCAD8FileFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	return std::make_unique<OCAD8FileExport>(path, map, view);
}



// ### OCAD8FileImport ###

OCAD8FileImport::OCAD8FileImport(const QString& path, Map* map, MapView* view) : Importer(path, map, view), file(nullptr)
{
    ocad_init();
    const QByteArray enc_name = Settings::getInstance().getSetting(Settings::General_Local8BitEncoding).toByteArray();
    encoding_1byte = Util::codecForName(enc_name);
    if (!encoding_1byte) encoding_1byte = QTextCodec::codecForLocale();
    encoding_2byte = QTextCodec::codecForName("UTF-16LE");
    offset_x = offset_y = 0;
}

OCAD8FileImport::~OCAD8FileImport()
{
    ocad_shutdown();
}

bool OCAD8FileImport::isRasterImageFile(const QString &filename)
{
	int dot_pos = filename.lastIndexOf(QLatin1Char('.'));
	if (dot_pos < 0)
		return false;
	QString extension = filename.right(filename.length() - dot_pos - 1).toLower();
	return QImageReader::supportedImageFormats().contains(extension.toLatin1());
}

bool OCAD8FileImport::importImplementation()
{
    //qint64 start = QDateTime::currentMSecsSinceEpoch();
	
	auto& device = *this->device();
	u32 size = device.bytesAvailable();
	u8* buffer = (u8*)malloc(size);
	if (!buffer)
		throw FileFormatException(tr("Could not allocate buffer."));
	if (device.read((char*)buffer, size) != size)
		throw FileFormatException(device.errorString());
	int err = ocad_file_open_memory(&file, buffer, size);
    if (err != 0) throw FileFormatException(tr("libocad returned %1").arg(err));
	
	if (file->header->major <= 5 || file->header->major >= 9)
		throw FileFormatException(tr("OCAD files of version %1 are not supported!").arg(file->header->major));

    //qDebug() << "file version is" << file->header->major << ", type is"
    //         << ((file->header->ftype == 2) ? "normal" : "other");
    //qDebug() << "map scale is" << file->setup->scale;

	map->setProperty(OcdFileFormat::versionProperty(), file->header->major);

	// Scale and georeferencing parameters
	Georeferencing georef;
	georef.setScaleDenominator(file->setup->scale);
	georef.setProjectedRefPoint(QPointF(file->setup->offsetx, file->setup->offsety));
	if (qAbs(file->setup->angle) >= 0.01) /* degrees */
	{
		georef.setGrivation(file->setup->angle);
	}
	map->setGeoreferencing(georef);
	
	map->setMapNotes(convertCString((const char*)file->buffer + file->header->infopos, file->header->infosize, false));

    // TODO: print parameters
	
	// Load the separations to a temporary stack
	std::vector< MapColor* > separations;
	int num_separations = ocad_separation_count(file);
#if 1
	addWarning(tr("%n color separation(s) were skipped, reason: Import disabled.", "", num_separations));
	num_separations = 0;
#endif
	if (num_separations < 0)
	{
		addWarning(tr("Could not load the spot color definitions, error: %1").arg(num_separations));
		num_separations = 0;
	}
	separations.reserve(num_separations);
	for (int i = 0; i < num_separations; i++)
	{
		const OCADColorSeparation *ocad_separation = ocad_separation_at(file, i);
		MapColor* color = new MapColor(convertPascalString(ocad_separation->sep_name), MapColor::Reserved);
		color->setSpotColorName(convertPascalString(ocad_separation->sep_name).toUpper());
		// OCD stores CMYK values as integers from 0-200.
		const MapColorCmyk cmyk(
		  0.005f * ocad_separation->cyan,
		  0.005f * ocad_separation->magenta,
		  0.005f * ocad_separation->yellow,
		  0.005f * ocad_separation->black );
		color->setCmyk(cmyk);
		color->setOpacity(1.0f);
		separations.push_back(color);
	}
	
	// Load colors
	int num_colors = ocad_color_count(file);
	for (int i = 0; i < num_colors; i++)
	{
		OCADColor *ocad_color = ocad_color_at(file, i);
		MapColor* color = new MapColor(convertPascalString(ocad_color->name), map->color_set->colors.size());
		// OCD stores CMYK values as integers from 0-200.
		MapColorCmyk cmyk(
		  0.005f * ocad_color->cyan,
		  0.005f * ocad_color->magenta,
		  0.005f * ocad_color->yellow,
		  0.005f * ocad_color->black );
		color->setCmyk(cmyk);
		color->setOpacity(1.0f);
		
		SpotColorComponents components;
		for (int j = 0; j < num_separations; ++j)
		{
			const u8& ocad_halftone = ocad_color->spot[j];
			if (ocad_halftone <= 200)
			{
				float halftone = 0.005f * ocad_halftone;
				components.reserve(std::size_t(num_separations));  // reserves only once for same capacity
				components.push_back(SpotColorComponent(separations[j], halftone));  // clazy:exclude=reserve-candidates
			}
		}
		if (!components.empty())
		{
			color->setSpotColorComposition(components);
			const MapColorCmyk cmyk(color->getCmyk());
			color->setCmykFromSpotColors();
			if (cmyk != color->getCmyk())
				// The color's CMYK was customized.
				color->setCmyk(cmyk);
		}
		
		if (i == 0 && color->isBlack() && color->getName() == QLatin1String("Registration black")
		           && XMLFileFormat::active_version >= 6 )
		{
			delete color; color = nullptr;
			color_index[ocad_color->number] = Map::getRegistrationColor();
			addWarning(tr("Color \"Registration black\" is imported as a special color."));
			// NOTE: This does not make a difference in output
			// as long as no spot colors are created,
			// but as a special color, it is protected from modification,
			// and it will be saved as number 0 in OCD export.
		}
		else
		{
			map->color_set->colors.push_back(color);
			color_index[ocad_color->number] = color;
		}
	}
	
	// Insert the spot colors into the map
	for (int i = 0; i < num_separations; ++i)
	{
		map->addColor(separations[i], map->color_set->colors.size());
	}
	
    // Load symbols
    for (OCADSymbolIndex *idx = ocad_symidx_first(file); idx; idx = ocad_symidx_next(file, idx))
    {
        for (int i = 0; i < 256; i++)
        {
            OCADSymbol *ocad_symbol = ocad_symbol_at(file, idx, i);
            if (ocad_symbol && ocad_symbol->number != 0)
            {
                Symbol *symbol = nullptr;
                if (ocad_symbol->type == OCAD_POINT_SYMBOL)
                {
                    symbol = importPointSymbol((OCADPointSymbol *)ocad_symbol);
                }
                else if (ocad_symbol->type == OCAD_LINE_SYMBOL)
                {
                    symbol = importLineSymbol((OCADLineSymbol *)ocad_symbol);
                }
                else if (ocad_symbol->type == OCAD_AREA_SYMBOL)
                {
                    symbol = importAreaSymbol((OCADAreaSymbol *)ocad_symbol);
                }
                else if (ocad_symbol->type == OCAD_TEXT_SYMBOL)
                {
                    symbol = importTextSymbol((OCADTextSymbol *)ocad_symbol);
                }
                else if (ocad_symbol->type == OCAD_RECT_SYMBOL)
                {
					RectangleInfo* rect = importRectSymbol((OCADRectSymbol *)ocad_symbol);
					map->symbols.push_back(rect->border_line);
					if (rect->has_grid)
					{
						map->symbols.push_back(rect->inner_line);
						map->symbols.push_back(rect->text);
					}
					continue;
                }
				

				if (symbol)
				{
					map->symbols.push_back(symbol);
					symbol_index[ocad_symbol->number] = symbol;
                }
                else
                {
                    addWarning(tr("Unable to import symbol \"%3\" (%1.%2)")
                               .arg(ocad_symbol->number / 10).arg(ocad_symbol->number % 10)
                               .arg(convertPascalString(ocad_symbol->name)));
                }
            }
        }
    }

    if (!loadSymbolsOnly())
	{
		// Load objects

		// Place all objects into a single OCAD import part
		MapPart* part = new MapPart(tr("OCAD import layer"), map);
		for (OCADObjectIndex *idx = ocad_objidx_first(file); idx; idx = ocad_objidx_next(file, idx))
		{
			for (int i = 0; i < 256; i++)
			{
				OCADObjectEntry *entry = ocad_object_entry_at(file, idx, i);
				OCADObject *ocad_obj = ocad_object(file, entry);
				if (ocad_obj)
				{
					Object *object = importObject(ocad_obj, part);
					if (object) {
						part->objects.push_back(object);
					}
				}
			}
		}
		delete map->parts[0];
		map->parts[0] = part;
		map->current_part_index = 0;

		// Load templates
		map->templates.clear();
		for (OCADStringIndex *idx = ocad_string_index_first(file); idx; idx = ocad_string_index_next(file, idx))
		{
			for (int i = 0; i < 256; i++)
			{
				OCADStringEntry *entry = ocad_string_entry_at(file, idx, i);
				if (entry->type != 0 && entry->size > 0)
					importString(entry);
			}
		}
		map->first_front_template = map->templates.size(); // Templates in front of the map are not supported by OCD

		// Fill view with relevant fields from OCD file
		if (view)
		{
			if (file->setup->zoom >= MapView::zoom_out_limit && file->setup->zoom <= MapView::zoom_in_limit)
				view->setZoom(file->setup->zoom);
			
			s32 buf[3];
			ocad_point(buf, &file->setup->center);
			MapCoord center_pos;
			convertPoint(center_pos, buf[0], buf[1]);
			view->setCenter(center_pos);
		}
		
		// TODO: read template visibilities
		/*
			int num_template_visibilities;
			file->read((char*)&num_template_visibilities, sizeof(int));

			for (int i = 0; i < num_template_visibilities; ++i)
			{
				int pos;
				file->read((char*)&pos, sizeof(int));

                TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
				file->read((char*)&vis->visible, sizeof(bool));
				file->read((char*)&vis->opacity, sizeof(float));
			}
		}
		*/

		// Undo steps are not supported in OCAD
    }

    ocad_file_close(file);

    //qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - start;
	//qDebug() << "OCAD map imported:"<<map->getNumSymbols()<<"symbols and"<<map->getNumObjects()<<"objects in"<<elapsed<<"milliseconds";

	emit map->currentMapPartIndexChanged(map->current_part_index);
	emit map->currentMapPartChanged(map->getPart(map->current_part_index));
	
	return true;
}

void OCAD8FileImport::setStringEncodings(const char *narrow, const char *wide) {
    encoding_1byte = QTextCodec::codecForName(narrow);
    encoding_2byte = QTextCodec::codecForName(wide);
}

Symbol *OCAD8FileImport::importPointSymbol(const OCADPointSymbol *ocad_symbol)
{
    PointSymbol *symbol = importPattern(ocad_symbol->ngrp, (OCADPoint *)ocad_symbol->pts);
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);
	symbol->setRotatable(ocad_symbol->base_flags & 1);

    return symbol;
}

Symbol *OCAD8FileImport::importLineSymbol(const OCADLineSymbol *ocad_symbol)
{
	LineSymbol* line_for_borders = nullptr;
	
	// Import a main line?
	LineSymbol* main_line = nullptr;
	if (ocad_symbol->dmode == 0 || ocad_symbol->width > 0)
	{
		main_line = new LineSymbol();
		line_for_borders = main_line;
		fillCommonSymbolFields(main_line, (OCADSymbol *)ocad_symbol);

		// Basic line options
		main_line->line_width = convertSize(ocad_symbol->width);
		main_line->color = convertColor(ocad_symbol->color);
		
		// Cap and join styles
		if (ocad_symbol->ends == 0)
		{
			main_line->cap_style = LineSymbol::FlatCap;
			main_line->join_style = LineSymbol::BevelJoin;
		}
		else if (ocad_symbol->ends == 1)
		{
			main_line->cap_style = LineSymbol::RoundCap;
			main_line->join_style = LineSymbol::RoundJoin;
		}
		else if (ocad_symbol->ends == 2)
		{
			main_line->cap_style = LineSymbol::PointedCap;
			main_line->join_style = LineSymbol::BevelJoin;
		}
		else if (ocad_symbol->ends == 3)
		{
			main_line->cap_style = LineSymbol::PointedCap;
			main_line->join_style = LineSymbol::RoundJoin;
		}
		else if (ocad_symbol->ends == 4)
		{
			main_line->cap_style = LineSymbol::FlatCap;
			main_line->join_style = LineSymbol::MiterJoin;
		}
		else if (ocad_symbol->ends == 6)
		{
			main_line->cap_style = LineSymbol::PointedCap;
			main_line->join_style = LineSymbol::MiterJoin;
		}

		main_line->start_offset = convertSize(ocad_symbol->bdist);
		main_line->end_offset = convertSize(ocad_symbol->edist);
		if (main_line->cap_style == LineSymbol::PointedCap)
		{
			// Note: While the property in the file may be different
			// (cf. what is set in the first place), OC*D always
			// draws round joins if the line cap is pointed!
			main_line->join_style = LineSymbol::RoundJoin;
		}
		
		// Handle the dash pattern
		if( ocad_symbol->gap > 0 || ocad_symbol->gap2 > 0 )
		{
			main_line->dashed = true;
			
			// Detect special case
			if (ocad_symbol->gap2 > 0 && ocad_symbol->gap == 0)
			{
				main_line->dash_length = convertSize(ocad_symbol->len - ocad_symbol->gap2);
				main_line->break_length = convertSize(ocad_symbol->gap2);
				if (!(ocad_symbol->elen >= ocad_symbol->len / 2 - 1 && ocad_symbol->elen <= ocad_symbol->len / 2 + 1))
					addWarning(tr("In dashed line symbol %1, the end length cannot be imported correctly.").arg(0.1 * ocad_symbol->number));
				if (ocad_symbol->egap != 0)
					addWarning(tr("In dashed line symbol %1, the end gap cannot be imported correctly.").arg(0.1 * ocad_symbol->number));
			}
			else
			{
				if (ocad_symbol->len != ocad_symbol->elen)
				{
					if (ocad_symbol->elen >= ocad_symbol->len / 2 - 1 && ocad_symbol->elen <= ocad_symbol->len / 2 + 1)
						main_line->half_outer_dashes = true;
					else
						addWarning(tr("In dashed line symbol %1, main and end length are different (%2 and %3). Using %4.")
						.arg(0.1 * ocad_symbol->number).arg(ocad_symbol->len).arg(ocad_symbol->elen).arg(ocad_symbol->len));
				}
				
				main_line->dash_length = convertSize(ocad_symbol->len);
				main_line->break_length = convertSize(ocad_symbol->gap);
				if (ocad_symbol->gap2 > 0)
				{
					main_line->dashes_in_group = 2;
					if (ocad_symbol->gap2 != ocad_symbol->egap)
						addWarning(tr("In dashed line symbol %1, gaps D and E are different (%2 and %3). Using %4.")
						.arg(0.1 * ocad_symbol->number).arg(ocad_symbol->gap2).arg(ocad_symbol->egap).arg(ocad_symbol->gap2));
					main_line->in_group_break_length = convertSize(ocad_symbol->gap2);
					main_line->dash_length = (main_line->dash_length - main_line->in_group_break_length) / 2;
				}
			}
		} 
		else
		{
			main_line->segment_length = convertSize(ocad_symbol->len);
			main_line->end_length = convertSize(ocad_symbol->elen);
		}
	}
	
	// Import a 'framing' line?
	LineSymbol* framing_line = nullptr;
	if (ocad_symbol->fwidth > 0)
	{
		framing_line = new LineSymbol();
		if (!line_for_borders)
			line_for_borders = framing_line;
		fillCommonSymbolFields(framing_line, (OCADSymbol *)ocad_symbol);
		
		// Basic line options
		framing_line->line_width = convertSize(ocad_symbol->fwidth);
		framing_line->color = convertColor(ocad_symbol->fcolor);
		
		// Cap and join styles
		if (ocad_symbol->fstyle == 0)
		{
			framing_line->cap_style = LineSymbol::FlatCap;
			framing_line->join_style = LineSymbol::BevelJoin;
		}
		else if (ocad_symbol->fstyle == 1)
		{
			framing_line->cap_style = LineSymbol::RoundCap;
			framing_line->join_style = LineSymbol::RoundJoin;
		}
		else if (ocad_symbol->fstyle == 4)
		{
			framing_line->cap_style = LineSymbol::FlatCap;
			framing_line->join_style = LineSymbol::MiterJoin;
		}
	}
	
	// Import a 'double' line?
	bool has_border_line = ocad_symbol->lwidth > 0 || ocad_symbol->rwidth > 0;
	LineSymbol *double_line = nullptr;
	if (ocad_symbol->dmode != 0 && (ocad_symbol->dflags & 1 || (has_border_line && !line_for_borders)))
	{
		double_line = new LineSymbol();
		line_for_borders = double_line;
		fillCommonSymbolFields(double_line, (OCADSymbol *)ocad_symbol);
		
		double_line->line_width = convertSize(ocad_symbol->dwidth);
		if (ocad_symbol->dflags & 1)
			double_line->color = convertColor(ocad_symbol->dcolor);
		else
			double_line->color = nullptr;
		
		double_line->cap_style = LineSymbol::FlatCap;
		double_line->join_style = LineSymbol::MiterJoin;
		
		double_line->segment_length = convertSize(ocad_symbol->len);
		double_line->end_length = convertSize(ocad_symbol->elen);
	}
	
	// Border lines
	if (has_border_line)
	{
		Q_ASSERT(line_for_borders);
		line_for_borders->have_border_lines = true;
		LineSymbolBorder& border = line_for_borders->getBorder();
		LineSymbolBorder& right_border = line_for_borders->getRightBorder();
		
		// Border color and width
		border.color = convertColor(ocad_symbol->lcolor);
		border.width = convertSize(ocad_symbol->lwidth);
		border.shift = convertSize(ocad_symbol->lwidth) / 2 + (convertSize(ocad_symbol->dwidth) - line_for_borders->line_width) / 2;
		
		right_border.color = convertColor(ocad_symbol->rcolor);
		right_border.width = convertSize(ocad_symbol->rwidth);
		right_border.shift = convertSize(ocad_symbol->rwidth) / 2 + (convertSize(ocad_symbol->dwidth) - line_for_borders->line_width) / 2;
		
		// The borders may be dashed
		if (ocad_symbol->dgap > 0 && ocad_symbol->dmode > 1)
		{
			border.dashed = true;
			border.dash_length = convertSize(ocad_symbol->dlen);
			border.break_length = convertSize(ocad_symbol->dgap);
			
			// If ocad_symbol->dmode == 2, only the left border should be dashed
			if (ocad_symbol->dmode > 2)
			{
				right_border.dashed = border.dashed;
				right_border.dash_length = border.dash_length;
				right_border.break_length = border.break_length;
			}
		}
	}
	
    // Create point symbols along line; middle ("normal") dash, corners, start, and end.
    LineSymbol* symbol_line = main_line ? main_line : double_line;	// Find the line to attach the symbols to
    if (!symbol_line)
	{
		main_line = new LineSymbol();
		symbol_line = main_line;
		fillCommonSymbolFields(main_line, (OCADSymbol *)ocad_symbol);
		
		main_line->segment_length = convertSize(ocad_symbol->len);
		main_line->end_length = convertSize(ocad_symbol->elen);
	}
    OCADPoint * symbolptr = (OCADPoint *)ocad_symbol->pts;
	symbol_line->mid_symbol = importPattern( ocad_symbol->smnpts, symbolptr);
	symbol_line->mid_symbols_per_spot = ocad_symbol->snum;
	symbol_line->mid_symbol_distance = convertSize(ocad_symbol->sdist);
    symbolptr += ocad_symbol->smnpts;
    if( ocad_symbol->ssnpts > 0 )
    {
		//symbol_line->dash_symbol = importPattern( ocad_symbol->ssnpts, symbolptr);
        symbolptr += ocad_symbol->ssnpts;
    }
    if( ocad_symbol->scnpts > 0 )
    {
		symbol_line->dash_symbol = importPattern( ocad_symbol->scnpts, symbolptr);
        symbol_line->dash_symbol->setName(QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Dash symbol"));
        symbolptr += ocad_symbol->scnpts; 
    }
    if( ocad_symbol->sbnpts > 0 )
    {
		symbol_line->start_symbol = importPattern( ocad_symbol->sbnpts, symbolptr);
        symbol_line->start_symbol->setName(QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Start symbol"));
        symbolptr += ocad_symbol->sbnpts;
    }
    if( ocad_symbol->senpts > 0 )
    {
		symbol_line->end_symbol = importPattern( ocad_symbol->senpts, symbolptr);
    }
    // FIXME: not really sure how this translates... need test cases
    symbol_line->minimum_mid_symbol_count = 0; //1 + ocad_symbol->smin;
	symbol_line->minimum_mid_symbol_count_when_closed = 0; //1 + ocad_symbol->smin;
	symbol_line->show_at_least_one_symbol = false; // NOTE: this works in a different way than OCAD's 'at least X symbols' setting (per-segment instead of per-object)
	
	// Suppress dash symbol at line ends if both start symbol and end symbol exist,
	// but don't create a warning unless a dash symbol is actually defined
	// and the line symbol is not Mapper's 799 Simple orienteering course.
	if (symbol_line->start_symbol && symbol_line->end_symbol)
	{
		symbol_line->setSuppressDashSymbolAtLineEnds(true);
		if (symbol_line->dash_symbol && symbol_line->getNumberComponent(0) != 799)
			addWarning(tr("Line symbol %1: suppressing dash symbol at line ends.").arg(QString::number(0.1 * ocad_symbol->number) + QLatin1Char(' ') + symbol_line->getName()));
	}
	
    // TODO: taper fields (tmode and tlast)
	
    if (!main_line && !framing_line)
		return double_line;
	else if (!double_line && !framing_line)
		return main_line;
	else if (!main_line && !double_line)
		return framing_line;
	else
	{
		CombinedSymbol* full_line = new CombinedSymbol();
		fillCommonSymbolFields(full_line, (OCADSymbol *)ocad_symbol);
		full_line->setNumParts(3);
		int part = 0;
		if (main_line)
		{
			full_line->setPart(part++, main_line, true);
			main_line->setHidden(false);
			main_line->setProtected(false);
		}
		if (double_line)
		{
			full_line->setPart(part++, double_line, true);
			double_line->setHidden(false);
			double_line->setProtected(false);
		}
		if (framing_line)
		{
			full_line->setPart(part++, framing_line, true);
			framing_line->setHidden(false);
			framing_line->setProtected(false);
		}
		full_line->setNumParts(part);
		return full_line;
	}
}

Symbol *OCAD8FileImport::importAreaSymbol(const OCADAreaSymbol *ocad_symbol)
{
    AreaSymbol *symbol = new AreaSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    // Basic area symbol fields: minimum_area, color
    symbol->minimum_area = 0;
    symbol->color = ocad_symbol->fill ? convertColor(ocad_symbol->color) : nullptr;
    symbol->patterns.clear();
    AreaSymbol::FillPattern *pat = nullptr;

    // Hatching
    if (ocad_symbol->hmode > 0)
    {
        int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
        pat->type = AreaSymbol::FillPattern::LinePattern;
        pat->angle = convertRotation(ocad_symbol->hangle1);
        pat->flags = AreaSymbol::FillPattern::Option::Rotatable;
        pat->line_spacing = convertSize(ocad_symbol->hdist + ocad_symbol->hwidth);
        pat->line_offset = 0;
        pat->line_color = convertColor(ocad_symbol->hcolor);
        pat->line_width = convertSize(ocad_symbol->hwidth);
        if (ocad_symbol->hmode == 2)
        {
            // Second hatch, same as the first, just a different angle
            symbol->patterns.push_back(*pat);
            symbol->patterns.back().angle = convertRotation(ocad_symbol->hangle2);
        }
    }

    if (ocad_symbol->pmode > 0)
    {
        // OCAD 8 has a "staggered" pattern mode, where successive rows are shifted width/2 relative
        // to each other. We need to simulate this in Mapper with two overlapping patterns, each with
        // twice the height. The second is then offset by width/2, height/2.
        auto spacing = convertSize(ocad_symbol->pheight);
        if (ocad_symbol->pmode == 2) spacing *= 2;
        int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
        pat->type = AreaSymbol::FillPattern::PointPattern;
        pat->angle = convertRotation(ocad_symbol->pangle);
        pat->flags = AreaSymbol::FillPattern::Option::Rotatable;
        pat->point_distance = convertSize(ocad_symbol->pwidth);
        pat->line_spacing = spacing;
        pat->line_offset = 0;
        pat->offset_along_line = 0;
        // FIXME: somebody needs to own this symbol and be responsible for deleting it
        // Right now it looks like a potential memory leak
        pat->point = importPattern(ocad_symbol->npts, (OCADPoint *)ocad_symbol->pts);
        if (ocad_symbol->pmode == 2)
        {
            int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
            pat->type = AreaSymbol::FillPattern::PointPattern;
            pat->angle = convertRotation(ocad_symbol->pangle);
            pat->flags = AreaSymbol::FillPattern::Option::Rotatable;
            pat->point_distance = convertSize(ocad_symbol->pwidth);
            pat->line_spacing = spacing;
            pat->line_offset = pat->line_spacing / 2;
            pat->offset_along_line = pat->point_distance / 2;
            pat->point = importPattern(ocad_symbol->npts, (OCADPoint *)ocad_symbol->pts);
        }
    }

    return symbol;
}

Symbol *OCAD8FileImport::importTextSymbol(const OCADTextSymbol *ocad_symbol)
{
    TextSymbol *symbol = new TextSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    symbol->font_family = convertPascalString(ocad_symbol->font); // FIXME: font mapping?
    symbol->color = convertColor(ocad_symbol->color);
	double d_font_size = (0.1 * ocad_symbol->dpts) / 72.0 * 25.4;
	symbol->font_size = qRound(1000 * d_font_size);
    symbol->bold = (ocad_symbol->bold >= 550) ? true : false;
    symbol->italic = (ocad_symbol->italic) ? true : false;
    symbol->underline = false;
	symbol->paragraph_spacing = convertSize(ocad_symbol->pspace);
	symbol->character_spacing = ocad_symbol->cspace / 100.0;
	symbol->kerning = false;
	symbol->line_below = ocad_symbol->under;
	symbol->line_below_color = convertColor(ocad_symbol->ucolor);
	symbol->line_below_width = convertSize(ocad_symbol->uwidth);
	symbol->line_below_distance = convertSize(ocad_symbol->udist);
	symbol->custom_tabs.resize(ocad_symbol->ntabs);
	for (int i = 0; i < ocad_symbol->ntabs; ++i)
		symbol->custom_tabs[i] = convertSize(ocad_symbol->tab[i]);
	
	int halign = (int)TextObject::AlignHCenter;
	if (ocad_symbol->halign == 0)
		halign = (int)TextObject::AlignLeft;
	else if (ocad_symbol->halign == 1)
		halign = (int)TextObject::AlignHCenter;
	else if (ocad_symbol->halign == 2)
		halign = (int)TextObject::AlignRight;
	else if (ocad_symbol->halign == 3)
	{
		// TODO: implement justified alignment
		addWarning(tr("During import of text symbol %1: ignoring justified alignment").arg(0.1 * ocad_symbol->number));
	}
	text_halign_map[symbol] = halign;

    if (ocad_symbol->bold != 400 && ocad_symbol->bold != 700)
    {
        addWarning(tr("During import of text symbol %1: ignoring custom weight (%2)")
                       .arg(0.1 * ocad_symbol->number).arg(ocad_symbol->bold));
    }
    if (ocad_symbol->cspace != 0)
	{
		addWarning(tr("During import of text symbol %1: custom character spacing is set, its implementation does not match OCAD's behavior yet")
		.arg(0.1 * ocad_symbol->number));
	}
    if (ocad_symbol->wspace != 100)
    {
        addWarning(tr("During import of text symbol %1: ignoring custom word spacing (%2%)")
                       .arg(0.1 * ocad_symbol->number).arg(ocad_symbol->wspace));
    }
    if (ocad_symbol->indent1 != 0 || ocad_symbol->indent2 != 0)
    {
        addWarning(tr("During import of text symbol %1: ignoring custom indents (%2/%3)")
                       .arg(0.1 * ocad_symbol->number).arg(ocad_symbol->indent1).arg(ocad_symbol->indent2));
    }
	
	if (ocad_symbol->fmode > 0)
	{
		symbol->framing = true;
		symbol->framing_color = convertColor(ocad_symbol->fcolor);
		if (ocad_symbol->fmode == 1)
		{
			symbol->framing_mode = TextSymbol::ShadowFraming;
			symbol->framing_shadow_x_offset = convertSize(ocad_symbol->fdx);
			symbol->framing_shadow_y_offset = -1 * convertSize(ocad_symbol->fdy);
		}
		else if (ocad_symbol->fmode == 2)
		{
			symbol->framing_mode = TextSymbol::LineFraming;
			symbol->framing_line_half_width = convertSize(ocad_symbol->fdpts);
		}
		else
		{
			addWarning(tr("During import of text symbol %1: ignoring text framing (mode %2)")
			.arg(0.1 * ocad_symbol->number).arg(ocad_symbol->fmode));
		}
	}

    symbol->updateQFont();
	
	// Convert line spacing
	double absolute_line_spacing = d_font_size * 0.01 * ocad_symbol->lspace;
	symbol->line_spacing = absolute_line_spacing / (symbol->getFontMetrics().lineSpacing() / symbol->calculateInternalScaling());

    return symbol;
}
OCAD8FileImport::RectangleInfo* OCAD8FileImport::importRectSymbol(const OCADRectSymbol* ocad_symbol)
{
	RectangleInfo rect;
	rect.border_line = new LineSymbol();
	fillCommonSymbolFields(rect.border_line, (OCADSymbol *)ocad_symbol);
	rect.border_line->line_width = convertSize(ocad_symbol->width);
	rect.border_line->color = convertColor(ocad_symbol->color);
	rect.border_line->cap_style = LineSymbol::FlatCap;
	rect.border_line->join_style = LineSymbol::RoundJoin;
	rect.corner_radius = 0.001 * convertSize(ocad_symbol->corner);
	rect.has_grid = ocad_symbol->flags & 1;
	
	if (rect.has_grid)
	{
		rect.inner_line = new LineSymbol();
		fillCommonSymbolFields(rect.inner_line, (OCADSymbol *)ocad_symbol);
		rect.inner_line->setNumberComponent(2, 1);
		rect.inner_line->line_width = qRound(1000 * 0.15);
		rect.inner_line->color = rect.border_line->color;
		
		rect.text = new TextSymbol();
		fillCommonSymbolFields(rect.text, (OCADSymbol *)ocad_symbol);
		rect.text->setNumberComponent(2, 2);
		rect.text->font_family = QString::fromLatin1("Arial");
		rect.text->font_size = qRound(1000 * (15 / 72.0 * 25.4));
		rect.text->color = rect.border_line->color;
		rect.text->bold = true;
		rect.text->updateQFont();
		
		rect.number_from_bottom = ocad_symbol->flags & 2;
		rect.cell_width = 0.001 * convertSize(ocad_symbol->cwidth);
		rect.cell_height = 0.001 * convertSize(ocad_symbol->cheight);
		rect.unnumbered_cells = ocad_symbol->gcells;
		rect.unnumbered_text = convertPascalString(ocad_symbol->gtext);
	}
	
	return &rectangle_info.insert(ocad_symbol->number, rect).value();
}

PointSymbol *OCAD8FileImport::importPattern(s16 npts, OCADPoint *pts)
{
    PointSymbol *symbol = new PointSymbol();
    symbol->rotatable = true;
    OCADPoint *p = pts, *end = pts + npts;
    while (p < end) {
        OCADSymbolElement *elt = (OCADSymbolElement *)p;
        int element_index = symbol->getNumElements();
		bool multiple_elements = p + (2 + elt->npts) < end || p > pts;
        if (elt->type == OCAD_DOT_ELEMENT)
        {
			int inner_radius = (int)convertSize(elt->diameter) / 2;
			if (inner_radius > 0)
			{
				PointSymbol* element_symbol = multiple_elements ? (new PointSymbol()) : symbol;
				element_symbol->inner_color = convertColor(elt->color);
				element_symbol->inner_radius = inner_radius;
				element_symbol->outer_color = nullptr;
				element_symbol->outer_width = 0;
				if (multiple_elements)
				{
					element_symbol->rotatable = false;
					PointObject* element_object = new PointObject(element_symbol);
					element_object->coords.resize(1);
					symbol->addElement(element_index, element_object, element_symbol);
				}
			}
        }
        else if (elt->type == OCAD_CIRCLE_ELEMENT)
        {
			int inner_radius = (int)convertSize(elt->diameter) / 2 - (int)convertSize(elt->width);
			int outer_width = (int)convertSize(elt->width);
			if (outer_width > 0 && inner_radius > 0)
			{
				PointSymbol* element_symbol = (multiple_elements) ? (new PointSymbol()) : symbol;
				element_symbol->inner_color = nullptr;
				element_symbol->inner_radius = inner_radius;
				element_symbol->outer_color = convertColor(elt->color);
				element_symbol->outer_width = outer_width;
				if (multiple_elements)
				{
					element_symbol->rotatable = false;
					PointObject* element_object = new PointObject(element_symbol);
					element_object->coords.resize(1);
					symbol->addElement(element_index, element_object, element_symbol);
				}
			}
        }
        else if (elt->type == OCAD_LINE_ELEMENT)
        {
            LineSymbol* element_symbol = new LineSymbol();
            element_symbol->line_width = convertSize(elt->width);
            element_symbol->color = convertColor(elt->color);
            PathObject* element_object = new PathObject(element_symbol);
            fillPathCoords(element_object, false, elt->npts, elt->pts);
			element_object->recalculateParts();
            symbol->addElement(element_index, element_object, element_symbol);
        }
        else if (elt->type == OCAD_AREA_ELEMENT)
        {
            AreaSymbol* element_symbol = new AreaSymbol();
            element_symbol->color = convertColor(elt->color);
            PathObject* element_object = new PathObject(element_symbol);
            fillPathCoords(element_object, true, elt->npts, elt->pts);
			element_object->recalculateParts();
            symbol->addElement(element_index, element_object, element_symbol);
        }
        p += (2 + elt->npts);
    }
    return symbol;
}


void OCAD8FileImport::fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol)
{
    // common fields are name, number, description, helper_symbol, hidden/protected status
    symbol->setName(convertPascalString(ocad_symbol->name));
    symbol->setNumberComponent(0, ocad_symbol->number / 10);
    symbol->setNumberComponent(1, ocad_symbol->number % 10);
    symbol->setNumberComponent(2, -1);
    symbol->setIsHelperSymbol(false); // no such thing in OCAD
    if (ocad_symbol->status & 1)
		symbol->setProtected(true);
	if (ocad_symbol->status & 2)
		symbol->setHidden(true);
}

Object *OCAD8FileImport::importObject(const OCADObject* ocad_object, MapPart* part)
{
	Symbol* symbol;
    if (!symbol_index.contains(ocad_object->symbol))
    {
		if (!rectangle_info.contains(ocad_object->symbol))
		{
			if (ocad_object->type == 1)
				symbol = map->getUndefinedPoint();
			else if (ocad_object->type == 2 || ocad_object->type == 3)
				symbol = map->getUndefinedLine();
			else if (ocad_object->type == 4 || ocad_object->type == 5)
				symbol = map->getUndefinedText();
			else
			{
				addWarning(tr("Unable to load object"));
				return nullptr;
			}
		}
		else
		{
			if (!importRectangleObject(ocad_object, part, rectangle_info[ocad_object->symbol]))
				addWarning(tr("Unable to import rectangle object"));
			return nullptr;
		}
    }
    else
		symbol = symbol_index[ocad_object->symbol];

    if (symbol->getType() == Symbol::Point)
    {
        PointObject *p = new PointObject();
        p->symbol = symbol;

        // extra properties: rotation
		PointSymbol* point_symbol = reinterpret_cast<PointSymbol*>(symbol);
		if (point_symbol->isRotatable())
			p->setRotation(convertRotation(ocad_object->angle));
		else if (ocad_object->angle != 0)
		{
			if (!point_symbol->isSymmetrical())
			{
				point_symbol->setRotatable(true);
				p->setRotation(convertRotation(ocad_object->angle));
			}
		}

        // only 1 coordinate is allowed, enforce it even if the OCAD object claims more.
		fillPathCoords(p, false, 1, ocad_object->pts);
        p->setMap(map);
        return p;
    }
    else if (symbol->getType() == Symbol::Text)
    {
		TextObject *t = new TextObject(symbol);

        // extra properties: rotation, horizontalAlignment, verticalAlignment, text
        t->setRotation(convertRotation(ocad_object->angle));
		t->setHorizontalAlignment((TextObject::HorizontalAlignment)text_halign_map.value(symbol));
		t->setVerticalAlignment(TextObject::AlignBaseline);

        const char *text_ptr = (const char *)(ocad_object->pts + ocad_object->npts);
        std::size_t text_len = sizeof(OCADPoint) * ocad_object->ntext;
        if (ocad_object->unicode) t->setText(convertWideCString(text_ptr, text_len, true));
        else t->setText(convertCString(text_ptr, text_len, true));

        // Text objects need special path translation
        if (!fillTextPathCoords(t, reinterpret_cast<TextSymbol*>(symbol), ocad_object->npts, (OCADPoint *)ocad_object->pts))
        {
            addWarning(tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
                           .arg(ocad_object->npts).arg(t->getText()));
            delete t;
            return nullptr;
        }
        t->setMap(map);
        return t;
    }
    else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined) {
		PathObject *p = new PathObject(symbol);
		
		p->setPatternRotation(convertRotation(ocad_object->angle));

		// Normal path
		fillPathCoords(p, symbol->getType() == Symbol::Area, ocad_object->npts, ocad_object->pts);
		p->recalculateParts();
		p->setMap(map);
		return p;
    }

    return nullptr;
}

bool OCAD8FileImport::importRectangleObject(const OCADObject* ocad_object, MapPart* part, const OCAD8FileImport::RectangleInfo& rect)
{
	if (ocad_object->npts != 4)
		return false;
	
	// Convert corner points
#ifdef Q_CC_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warray-bounds"
#endif
	s32 buf[3];
	ocad_point(buf, &(ocad_object->pts[3]));
	MapCoord top_left;
	convertPoint(top_left, buf[0], buf[1]);
	ocad_point(buf, &(ocad_object->pts[0]));
	MapCoord bottom_left;
	convertPoint(bottom_left, buf[0], buf[1]);
	ocad_point(buf, &(ocad_object->pts[2]));
	MapCoord top_right;
	convertPoint(top_right, buf[0], buf[1]);
	ocad_point(buf, &(ocad_object->pts[1]));
	MapCoord bottom_right;
	convertPoint(bottom_right, buf[0], buf[1]);
#ifdef Q_CC_CLANG
#pragma clang diagnostic pop
#endif
	
	MapCoordF top_left_f = MapCoordF(top_left);
	MapCoordF top_right_f = MapCoordF(top_right);
	MapCoordF bottom_left_f = MapCoordF(bottom_left);
	MapCoordF bottom_right_f = MapCoordF(bottom_right);
	MapCoordF right = MapCoordF(top_right.x() - top_left.x(), top_right.y() - top_left.y());
	double angle = right.angle();
	MapCoordF down = MapCoordF(bottom_left.x() - top_left.x(), bottom_left.y() - top_left.y());
	right.normalize();
	down.normalize();
	
	// Create border line
	MapCoordVector coords;
	if (rect.corner_radius == 0)
	{
		coords.emplace_back(top_left);
		coords.emplace_back(top_right);
		coords.emplace_back(bottom_right);
		coords.emplace_back(bottom_left);
	}
	else
	{
		double handle_radius = (1 - BEZIER_KAPPA) * rect.corner_radius;
		coords.emplace_back(top_right_f - right * rect.corner_radius, MapCoord::CurveStart);
		coords.emplace_back(top_right_f - right * handle_radius);
		coords.emplace_back(top_right_f + down * handle_radius);
		coords.emplace_back(top_right_f + down * rect.corner_radius);
		coords.emplace_back(bottom_right_f - down * rect.corner_radius, MapCoord::CurveStart);
		coords.emplace_back(bottom_right_f - down * handle_radius);
		coords.emplace_back(bottom_right_f - right * handle_radius);
		coords.emplace_back(bottom_right_f - right * rect.corner_radius);
		coords.emplace_back(bottom_left_f + right * rect.corner_radius, MapCoord::CurveStart);
		coords.emplace_back(bottom_left_f + right * handle_radius);
		coords.emplace_back(bottom_left_f - down * handle_radius);
		coords.emplace_back(bottom_left_f - down * rect.corner_radius);
		coords.emplace_back(top_left_f + down * rect.corner_radius, MapCoord::CurveStart);
		coords.emplace_back(top_left_f + down * handle_radius);
		coords.emplace_back(top_left_f + right * handle_radius);
		coords.emplace_back(top_left_f + right * rect.corner_radius);
	}
	PathObject *border_path = new PathObject(rect.border_line, coords, map);
	border_path->parts().front().setClosed(true, false);
	part->objects.push_back(border_path);
	
	if (rect.has_grid && rect.cell_width > 0 && rect.cell_height > 0)
	{
		// Calculate grid sizes
		double width = top_left.distanceTo(top_right);
		double height = top_left.distanceTo(bottom_left);
		int num_cells_x = qMax(1, qRound(width / rect.cell_width));
		int num_cells_y = qMax(1, qRound(height / rect.cell_height));
		
		float cell_width = width / num_cells_x;
		float cell_height = height / num_cells_y;
		
		// Create grid lines
		coords.resize(2);
		for (int x = 1; x < num_cells_x; ++x)
		{
			coords[0] = MapCoord(top_left_f + x * cell_width * right);
			coords[1] = MapCoord(bottom_left_f + x * cell_width * right);
			
			PathObject *path = new PathObject(rect.inner_line, coords, map);
			part->objects.push_back(path);
		}
		for (int y = 1; y < num_cells_y; ++y)
		{
			coords[0] = MapCoord(top_left_f + y * cell_height * down);
			coords[1] = MapCoord(top_right_f + y * cell_height * down);
			
			PathObject *path = new PathObject(rect.inner_line, coords, map);
			part->objects.push_back(path);
		}
		
		// Create grid text
		if (height >= rect.cell_height / 2)
		{
			for (int y = 0; y < num_cells_y; ++y) 
			{
				for (int x = 0; x < num_cells_x; ++x)
				{
					int cell_num;
					QString cell_text;
					
					if (rect.number_from_bottom)
						cell_num = y * num_cells_x + x + 1;
					else
						cell_num = (num_cells_y - 1 - y) * num_cells_x + x + 1;
					
					if (cell_num > num_cells_x * num_cells_y - rect.unnumbered_cells)
						cell_text = rect.unnumbered_text;
					else
						cell_text = QString::number(cell_num);
					
					TextObject* object = new TextObject(rect.text);
					object->setMap(map);
					object->setText(cell_text);
					object->setRotation(-angle);
					object->setHorizontalAlignment(TextObject::AlignLeft);
					object->setVerticalAlignment(TextObject::AlignTop);
					double position_x = (x + 0.07f) * cell_width;
					double position_y = (y + 0.04f) * cell_height + rect.text->getFontMetrics().ascent() / rect.text->calculateInternalScaling() - rect.text->getFontSize();
					object->setAnchorPosition(top_left_f + position_x * right + position_y * down);
					part->objects.push_back(object);
					
					//pts[0].Y -= rectinfo.gridText.FontAscent - rectinfo.gridText.FontEmHeight;
				}
			}
		}
	}
	
	return true;
}

void OCAD8FileImport::importString(OCADStringEntry *entry)
{
	OCADCString *ocad_str = ocad_string(file, entry);
	if (entry->type == 8)
	{
		// Template
		importTemplate(ocad_str);
	}
	
	// TODO: parse more types of strings, maybe the print parameters?
}

Template *OCAD8FileImport::importTemplate(OCADCString* ocad_str)
{
	Template* templ = nullptr;
	QByteArray data(ocad_str->str); // copies the data.
	QString filename = encoding_1byte->toUnicode(data.left(data.indexOf('\t', 0)));
	QString clean_path = QDir::cleanPath(QString(filename).replace(QLatin1Char('\\'), QLatin1Char('/')));
	QString extension = QFileInfo(clean_path).suffix();
	if (extension.compare(QLatin1String("ocd"), Qt::CaseInsensitive) == 0)
	{
		templ = new TemplateMap(clean_path, map);
	}
	else if (QImageReader::supportedImageFormats().contains(extension.toLatin1()))
	{
		templ = new TemplateImage(clean_path, map);
	}
	else
	{
		addWarning(tr("Unable to import template: background \"%1\" doesn't seem to be a raster image").arg(filename));
		return nullptr;
	}
	
	OCADBackground background = importBackground(data);
	MapCoord c;
	convertPoint(c, background.trnx, background.trny);
	templ->setTemplatePosition(c);
	templ->setTemplateRotation(M_PI / 180 * background.angle);
	templ->setTemplateScaleX(convertTemplateScale(background.sclx));
	templ->setTemplateScaleY(convertTemplateScale(background.scly));
	
	map->templates.insert(map->templates.begin(), templ);
	
	if (view)
	{
		auto opacity = qMax(0.0, qMin(1.0, 0.01 * (100 - background.dimming)));
		view->setTemplateVisibility(templ, { float(opacity), bool(background.s) });
	}
	
	return templ;
}

// A more flexible reimplementation of libocad's ocad_to_background().
OCADBackground OCAD8FileImport::importBackground(const QByteArray& data)
{
	unsigned int num_angles = 0;
	OCADBackground background;
	background.filename = data.data(); // tab-terminated, not 0-terminated!
	background.trnx = 0;
	background.trny = 0;
	background.angle = 0.0;
	background.sclx = 100.0;
	background.scly = 100.0;
	background.dimming = 0;
	background.s = 1;
	
	int i = data.indexOf('\t', 0);
	while (i >= 0)
	{
		double value;
		bool ok;
		int next_i = data.indexOf('\t', i+1);
		int len = (next_i > 0 ? next_i : data.length()) - i - 2;
		switch (data[i+1])
		{
			case 'x':
				background.trnx = qRound(data.mid(i + 2, len).toDouble());
				break;
			case 'y':
				background.trny = qRound(data.mid(i + 2, len).toDouble());
				break;
			case 'a':
			case 'b':
				// TODO: use the distinct angles correctly, not just the average
				background.angle += data.mid(i + 2, len).toDouble(&ok);
				if (ok)
					++num_angles;
				break;
			case 'u':
				value = data.mid(i + 2, len).toDouble(&ok);
				if (ok && qAbs(value) >= 0.0001)
					background.sclx = value;
				break;
			case 'v':
				value = data.mid(i + 2, len).toDouble(&ok);
				if (ok && qAbs(value) >= 0.0001)
					background.scly = value;
				break;
			case 'd':
				background.dimming = data.mid(i + 2, len).toInt();
				break;
			case 's':
				background.s = data.mid(i + 2, len).toInt();
				break;
			default:
				; // nothing
		}
		i = next_i;
	}
	
	if (num_angles)
		background.angle = background.angle / num_angles;
	
	return background;
}

Template *OCAD8FileImport::importRasterTemplate(const OCADBackground &background)
{
	QString filename(encoding_1byte->toUnicode(background.filename));
	filename = QDir::cleanPath(filename.replace(QLatin1Char('\\'), QLatin1Char('/')));
	if (isRasterImageFile(filename))
    {
        TemplateImage* templ = new TemplateImage(filename, map);
        MapCoord c;
        convertPoint(c, background.trnx, background.trny);
        templ->setTemplateX(c.nativeX());
        templ->setTemplateY(c.nativeY());
        templ->setTemplateRotation(M_PI / 180 * background.angle);
        templ->setTemplateScaleX(convertTemplateScale(background.sclx));
        templ->setTemplateScaleY(convertTemplateScale(background.scly));
        // FIXME: import template view parameters: background.dimming and background.transparent
		// TODO: import template visibility
        return templ;
    }
    else
    {
        addWarning(tr("Unable to import template: background \"%1\" doesn't seem to be a raster image").arg(filename));
    }
    return nullptr;
}

void OCAD8FileImport::setPathHolePoint(Object *object, int i)
{
	// Look for curve start points before the current point and apply hole point only if no such point is there.
	// This prevents hole points in the middle of a curve caused by incorrect map objects.
	if (i >= 1 && object->coords[i].isCurveStart())
		; //object->coords[i-1].setHolePoint(true);
	else if (i >= 2 && object->coords[i-1].isCurveStart())
		; //object->coords[i-2].setHolePoint(true);
	else if (i >= 3 && object->coords[i-2].isCurveStart())
		; //object->coords[i-3].setHolePoint(true);
	else
		object->coords[i].setHolePoint(true);
}

/** Translates the OCAD path given in the last two arguments into an Object.
 */
void OCAD8FileImport::fillPathCoords(Object *object, bool is_area, u16 npts, const OCADPoint *pts)
{
    object->coords.resize(npts);
    s32 buf[3];
    for (int i = 0; i < npts; i++)
    {
        ocad_point(buf, &(pts[i]));
        MapCoord &coord = object->coords[i];
        convertPoint(coord, buf[0], buf[1]);
        // We can support CurveStart, HolePoint, DashPoint.
        // CurveStart needs to be applied to the main point though, not the control point, and
		// hole points need to bet set as the last point of a part of an area object instead of the first point of the next part
		if (buf[2] & PX_CTL1 && i > 0)
			object->coords[i-1].setCurveStart(true);
		if ((buf[2] & (PY_DASH << 8)) || (buf[2] & (PY_CORNER << 8)))
			coord.setDashPoint(true);
		if (buf[2] & (PY_HOLE << 8))
			setPathHolePoint(object, is_area ? (i - 1) : i);
    }
    
    // For path objects, create closed parts where the position of the last point is equal to that of the first point
    if (object->getType() == Object::Path)
	{
		int start = 0;
		for (int i = 0; i < (int)object->coords.size(); ++i)
		{
			if (!object->coords[i].isHolePoint() && i < (int)object->coords.size() - 1)
				continue;
			
			if (object->coords[i].isPositionEqualTo(object->coords[start]))
			{
				MapCoord coord = object->coords[start];
				coord.setCurveStart(false);
				coord.setHolePoint(true);
				coord.setClosePoint(true);
				object->coords[i] = coord;
			}
			
			start = i + 1;
		}
	}
}

/** Translates an OCAD text object path into a Mapper text object specifier, if possible.
 *  If successful, sets either 1 or 2 coordinates in the text object and returns true.
 *  If the OCAD path was not importable, leaves the TextObject alone and returns false.
 */
bool OCAD8FileImport::fillTextPathCoords(TextObject *object, TextSymbol *symbol, u16 npts, OCADPoint *pts)
{
    // text objects either have 1 point (free anchor) or 2 (midpoint/size)
    // OCAD appears to always have 5 or 4 points (possible single anchor, then 4 corner coordinates going clockwise from anchor).
    if (npts == 0) return false;
	
	if (npts == 4)
	{
		// Box text
		s32 buf[3];
		ocad_point(buf, &(pts[3]));
		MapCoord top_left;
		convertPoint(top_left, buf[0], buf[1]);
		ocad_point(buf, &(pts[0]));
		MapCoord bottom_left;
		convertPoint(bottom_left, buf[0], buf[1]);
		ocad_point(buf, &(pts[2]));
		MapCoord top_right;
		convertPoint(top_right, buf[0], buf[1]);
		
		// According to Purple Pen source code: OCAD adds an extra internal leading (incorrectly).
		QFontMetricsF metrics = symbol->getFontMetrics();
		double top_adjust = -symbol->getFontSize() + (metrics.ascent() + metrics.descent() + 0.5) / symbol->calculateInternalScaling();
		
		MapCoordF adjust_vector = MapCoordF(top_adjust * sin(object->getRotation()), top_adjust * cos(object->getRotation()));
		top_left = MapCoord(top_left.x() + adjust_vector.x(), top_left.y() + adjust_vector.y());
		top_right = MapCoord(top_right.x() + adjust_vector.x(), top_right.y() + adjust_vector.y());
		
		object->setBox((bottom_left.nativeX() + top_right.nativeX()) / 2, (bottom_left.nativeY() + top_right.nativeY()) / 2,
					   top_left.distanceTo(top_right), top_left.distanceTo(bottom_left));

		object->setVerticalAlignment(TextObject::AlignTop);
	}
	else
	{
		// Single anchor text
		if (npts != 5)
			addWarning(tr("Trying to import a text object with unknown coordinate format"));
		
		s32 buf[3];
		ocad_point(buf, &(pts[0])); // anchor point
		
		MapCoord coord;
		convertPoint(coord, buf[0], buf[1]);
		object->setAnchorPosition(coord.nativeX(), coord.nativeY());
		
		object->setVerticalAlignment(TextObject::AlignBaseline);
	}

    return true;
}

/** Converts a single-byte-per-character, length-payload string to a QString.
 *
 *  The byte sequence will be: LEN C0 C1 C2 C3...
 *
 *  Obviously this will only hold up to 255 characters. By default, we interpret the
 *  bytes using Windows-1252, as that's the most likely candidate for OCAD files in
 *  the wild.
 */
QString OCAD8FileImport::convertPascalString(const char *p) {
    int len = *((unsigned char *)p);
    return encoding_1byte->toUnicode((p + 1), len);
}

/** Converts a single-byte-per-character, zero-terminated string to a QString.
 *
 *  The byte sequence will be: C0 C1 C2 C3 ... 00. n describes the maximum
 *  length (in bytes) that will be scanned for a zero terminator; if none is found,
 *  the string will be truncated at that location.
 */
QString OCAD8FileImport::convertCString(const char *p, std::size_t n, bool ignore_first_newline) {
    size_t i = 0;
    for (; i < n; i++) {
        if (p[i] == 0) break;
    }
    if (ignore_first_newline && n >= 2 && p[0] == '\r' && p[1] == '\n')
	{
		// Remove "\r\n" at the beginning of texts, somehow OCAD seems to add this sometimes but ignores it
		p += 2;
		i -= 2;
	}
    return encoding_1byte->toUnicode(p, i);
}

/** Converts a two-byte-per-character, zero-terminated string to a QString. By default,
 *  we interpret the bytes using UTF-16LE, as that's the most likely candidate for
 *  OCAD files in the wild.
 *
 *  The byte sequence will be: L0 H0 L1 H1 L2 H2... 00 00. n describes the maximum
 *  length (in bytes) that will be scanned for a zero terminator; if none is found,
 *  the string will be truncated at that location.
 */
QString OCAD8FileImport::convertWideCString(const char *p, std::size_t n, bool ignore_first_newline) {
    const u16 *q = (const u16 *)p;
    size_t i = 0;
    for (; i < n; i++) {
        if (q[i] == 0) break;
    }
    if (ignore_first_newline && n >= 4 && p[0] == '\r' && p[2] == '\n')
	{
		// Remove "\r\n" at the beginning of texts, somehow OCAD seems to add this sometimes but ignores it
		p += 4;
		i -= 2;
	}
    return encoding_2byte->toUnicode(p, i * 2);
}

float OCAD8FileImport::convertRotation(int angle) {
    // OCAD uses tenths of a degree, counterclockwise
    // BUG: if sin(rotation) is < 0 for a hatched area pattern, the pattern's createRenderables() will go into an infinite loop.
    // So until that's fixed, we keep a between 0 and PI
    double a = (M_PI / 180) *  (0.1f * angle);
    while (a < 0) a += 2 * M_PI;
    //if (a < 0 || a > M_PI) qDebug() << "Found angle" << a;
    return (float)a;
}

void OCAD8FileImport::convertPoint(MapCoord &coord, s32 ocad_x, s32 ocad_y)
{
    // Recover from broken coordinate export from Mapper 0.6.2 ... 0.6.4 (#749)
    // Cf. broken::convertPointMember below:
    // The values -4 ... -1 (-0.004 mm ... -0.001 mm) were converted to 0x80000000u instead of 0.
    // This is the maximum value. Thus it is okay to assume it won't occur in regular data,
    // and we can safely replace it with 0 here.
    // But the input parameter were already subject to right shift in ocad_point ...
    constexpr auto invalid_value = s32(0x80000000u) >> 8; // ... so we use this value here.
    if (ocad_x == invalid_value)
        ocad_x = 0;
    if (ocad_y == invalid_value)
        ocad_y = 0;
    
    // OCAD uses hundredths of a millimeter.
    // oo-mapper uses 1/1000 mm
    coord.setNativeX(offset_x + (qint32)ocad_x * 10);
    // Y-axis is flipped.
    coord.setNativeY(offset_y + (qint32)ocad_y * (-10));
}

qint32 OCAD8FileImport::convertSize(int ocad_size) {
    // OCAD uses hundredths of a millimeter.
    // oo-mapper uses 1/1000 mm
    return ((qint32)ocad_size) * 10;
}

const MapColor *OCAD8FileImport::convertColor(int color) {
	if (!color_index.contains(color))
	{
		addWarning(tr("Color id not found: %1, ignoring this color").arg(color));
		return nullptr;
	}
	else
		return color_index[color];
}

double OCAD8FileImport::convertTemplateScale(double ocad_scale)
{
	return ocad_scale * 0.01;	// millimeters(on map) per pixel
}


// ### OCAD8FileExport ###

OCAD8FileExport::OCAD8FileExport(const QString& path, const Map* map, const MapView* view)
 : Exporter(path, map, view),
   uses_registration_color(false),
   file(nullptr)
{
	ocad_init();
	encoding_1byte = QTextCodec::codecForName("Windows-1252");
	encoding_2byte = QTextCodec::codecForName("UTF-16LE");
	
	origin_point_object = new PointObject();
}

OCAD8FileExport::~OCAD8FileExport()
{
	delete origin_point_object;
}

bool OCAD8FileExport::exportImplementation()
{
	uses_registration_color = map->isColorUsedByASymbol(map->getRegistrationColor());
	if (map->getNumColors() > (uses_registration_color ? 255 : 256))
		throw FileFormatException(tr("The map contains more than 256 colors which is not supported by ocd version 8."));
	
	// Create struct in memory
	int err = ocad_file_new(&file);
	if (err != 0) throw FileFormatException(tr("libocad returned %1").arg(err));
	
	// Check for a neccessary offset (and add related warnings early).
	auto area_offset = calculateAreaOffset();
	
	// Fill header struct
	OCADFileHeader* header = file->header;
	*(((u8*)&header->magic) + 0) = 0xAD;
	*(((u8*)&header->magic) + 1) = 0x0C;
	header->ftype = 2;
	header->major = 8;
	header->minor = 0;
	if (map->getMapNotes().size() > 0)
	{
		header->infosize = map->getMapNotes().length() + 1;
		ocad_file_reserve(file, header->infosize);
		header->infopos = &file->buffer[file->size] - file->buffer;
		convertCString(map->getMapNotes(), &file->buffer[file->size], header->infosize);
		file->size += header->infosize;
	}
	
	// Fill setup struct
	OCADSetup* setup = file->setup;
	if (view)
	{
		setup->center = convertPoint(view->center() - area_offset);
		setup->zoom = view->getZoom();
	}
	else
		setup->zoom = 1;
	
	// Scale and georeferencing parameters
	const Georeferencing& georef = map->getGeoreferencing();
	setup->scale = georef.getScaleDenominator();
	const QPointF offset(georef.toProjectedCoords(area_offset));
	setup->offsetx = offset.x();
	setup->offsety = offset.y();
	setup->angle = georef.getGrivation();
	
	// TODO: print parameters
	
	// Colors
	int ocad_color_index = 0;
	if (uses_registration_color)
	{
		addWarning(tr("Registration black is exported as a regular color."));
		
		++file->header->ncolors;
		OCADColor *ocad_color = ocad_color_at(file, ocad_color_index);
		ocad_color->number = ocad_color_index;
		
		const MapColor* color = Map::getRegistrationColor();
		const MapColorCmyk& cmyk = color->getCmyk();
		ocad_color->cyan = qRound(1 / 0.005f * cmyk.c);
		ocad_color->magenta = qRound(1 / 0.005f * cmyk.m);
		ocad_color->yellow = qRound(1 / 0.005f * cmyk.y);
		ocad_color->black = qRound(1 / 0.005f * cmyk.k);
		convertPascalString(QString::fromLatin1("Registration black"), ocad_color->name, 32); // not translated
		
		++ocad_color_index;
	}
	for (int i = 0; i < map->getNumColors(); i++)
	{
		++file->header->ncolors;
		OCADColor *ocad_color = ocad_color_at(file, ocad_color_index);
		ocad_color->number = ocad_color_index;
		
		const MapColor* color = map->getColor(i);
		const MapColorCmyk& cmyk = color->getCmyk();
		ocad_color->cyan = qRound(1 / 0.005f * cmyk.c);
		ocad_color->magenta = qRound(1 / 0.005f * cmyk.m);
		ocad_color->yellow = qRound(1 / 0.005f * cmyk.y);
		ocad_color->black = qRound(1 / 0.005f * cmyk.k);
		convertPascalString(color->getName(), ocad_color->name, 32);
		
		++ocad_color_index;
	}
	
	// Symbols
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		const Symbol* symbol = map->getSymbol(i);
		
		s16 index = -1;
		if (symbol->getType() == Symbol::Point)
			index = exportPointSymbol(symbol->asPoint());
		else if (symbol->getType() == Symbol::Line)
			index = exportLineSymbol(symbol->asLine());
		else if (symbol->getType() == Symbol::Area)
			index = exportAreaSymbol(symbol->asArea());
		else if (symbol->getType() == Symbol::Text)
			index = exportTextSymbol(symbol->asText());
		else if (symbol->getType() == Symbol::Combined)
			; // This is done as a second pass to ensure that all dependencies are added to the symbol_index
		else
			Q_ASSERT(false);
		
		if (index >= 0)
		{
			std::set<s16> number;
			number.insert(index);
			symbol_index.insert(symbol, number);
		}
	}
	
	// Separate pass for combined symbols
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		const Symbol* symbol = map->getSymbol(i);
		if (symbol->getType() == Symbol::Combined)
			symbol_index.insert(symbol, exportCombinedSymbol(static_cast<const CombinedSymbol*>(symbol)));
	}
	
	// Objects
	OCADObject* ocad_object = ocad_object_alloc(nullptr);
	for (int l = 0; l < map->getNumParts(); ++l)
	{
		for (int o = 0; o < map->getPart(l)->getNumObjects(); ++o)
		{
			memset(ocad_object, 0, sizeof(OCADObject) - sizeof(OCADPoint) + 8 * (ocad_object->npts + ocad_object->ntext));
			Object* object = map->getPart(l)->getObject(o);
			std::unique_ptr<Object> duplicate;
			if (area_offset.nativeX() != 0 || area_offset.nativeY() != 0)
			{
				// Create a safely managed duplicate and move it as needed.
				duplicate.reset(object->duplicate());
				duplicate->move(-area_offset);
				object = duplicate.get();
			}
			object->update();
			
			// Fill some common entries of object struct
			OCADPoint* coord_buffer = ocad_object->pts;
			if (object->getType() != Object::Text)
				ocad_object->npts = exportCoordinates(object->getRawCoordinateVector(), &coord_buffer, object->getSymbol());
			else
				ocad_object->npts = exportTextCoordinates(static_cast<TextObject*>(object), &coord_buffer);
			
			if (object->getType() == Object::Point)
			{
				PointObject* point = static_cast<PointObject*>(object);
				ocad_object->angle = convertRotation(point->getRotation());
			}
			else if (object->getType() == Object::Path)
			{
				if (object->getSymbol()->getType() == Symbol::Area)
				{
					PathObject* path = static_cast<PathObject*>(object);
					// Known issue: In OCD format, pattern rotatability is all
					// or nothing. In Mapper, it is an option per pattern.
					if (path->getSymbol()->asArea()->hasRotatableFillPattern())
						ocad_object->angle = convertRotation(path->getPatternRotation());
					if (path->getPatternOrigin() != MapCoord(0, 0))
						addWarning(tr("Unable to export fill pattern shift for an area object"));
				}
			}
			else if (object->getType() == Object::Text)
			{
				TextObject* text = static_cast<TextObject*>(object);
				ocad_object->unicode = 1;
				ocad_object->angle = convertRotation(text->getRotation());
				int num_letters = convertWideCString(text->getText(), (unsigned char*)coord_buffer, 8 * (OCAD_MAX_OBJECT_PTS - ocad_object->npts));
				ocad_object->ntext = qCeil(num_letters / 4.0f);
			}
			
			// Insert an object into the map for every symbol contained in the symbol_index
			std::set<s16> index_set;
			if (symbol_index.contains(object->getSymbol()))
				index_set = symbol_index[object->getSymbol()];
			else
				index_set.insert(-1);	// export as undefined symbol
			
			for (const auto index : index_set)
			{
				s16 index_to_use = index;
				
				// For text objects, check if we have to change / create a new text symbol because of the formatting
				if (object->getType() == Object::Text && symbol_index.contains(object->getSymbol()))
				{
					TextObject* text_object = static_cast<TextObject*>(object);
					const TextSymbol* text_symbol = static_cast<const TextSymbol*>(object->getSymbol());
					if (!text_format_map.contains(text_symbol))
					{
						// Adjust the formatting in the first created symbol to this object
						OCADTextSymbol* ocad_text_symbol = (OCADTextSymbol*)ocad_symbol(file, index);
						setTextSymbolFormatting(ocad_text_symbol, text_object);
						
						TextFormatList new_list;
						new_list.push_back(std::make_pair(text_object->getHorizontalAlignment(), index));
						text_format_map.insert(text_symbol, new_list);
					}
					else
					{
						// Check if this formatting has already been created as symbol.
						// If yes, use this symbol, else create a new symbol
						TextFormatList& format_list = text_format_map[text_symbol];
						bool found = false;
						for (size_t i = 0, end = format_list.size(); i < end; ++i)
						{
							if (format_list[i].first == text_object->getHorizontalAlignment())
							{
								index_to_use = format_list[i].second;
								found = true;
								break;
							}
						}
						if (!found)
						{
							// Copy the symbol and adjust the formatting
							// TODO: insert these symbols directly after the original symbols
							OCADTextSymbol* ocad_text_symbol = (OCADTextSymbol*)ocad_symbol(file, index);
							OCADTextSymbol* new_symbol = (OCADTextSymbol*)ocad_symbol_new(file, ocad_text_symbol->size);
							// Get the pointer to the first symbol again as it might have changed during ocad_symbol_new()
							ocad_text_symbol = (OCADTextSymbol*)ocad_symbol(file, index);
							
							memcpy(new_symbol, ocad_text_symbol, ocad_text_symbol->size);
							setTextSymbolFormatting(new_symbol, text_object);
							
							// Give the new symbol a unique number
							while (symbol_numbers.find(new_symbol->number) != symbol_numbers.end())
								++new_symbol->number;
							symbol_numbers.insert(new_symbol->number);
							index_to_use = new_symbol->number;

							// Store packed new_symbol->number in separate variable first,
							// otherwise when compiling for Android this causes the error:
							// cannot bind packed field 'new_symbol->_OCADTextSymbol::number' to 'short int&'
							s16 new_symbol_number = new_symbol->number;
							format_list.push_back(std::make_pair(text_object->getHorizontalAlignment(), new_symbol_number));
						}
					}
				}
				
				ocad_object->symbol = index_to_use;
				if (object->getType() == Object::Point)
					ocad_object->type = 1;
				else if (object->getType() == Object::Path)
				{
					OCADSymbol* ocad_sym = ocad_symbol(file, index_to_use);
					if (!ocad_sym)
						ocad_object->type = 2;	// This case is for undefined lines; TODO: make another case for undefined areas, as soon as they are implemented
					else if (ocad_sym->type == 2)
						ocad_object->type = 2;	// Line
					else //if (ocad_symbol->type == 3)
						ocad_object->type = 3;	// Area
				}
				else if (object->getType() == Object::Text)
				{
					TextObject* text_object = static_cast<TextObject*>(object);
					if (text_object->hasSingleAnchor())
						ocad_object->type = 4;
					else
						ocad_object->type = 5;
				}
				
				OCADObjectEntry* entry;
				ocad_object_add(file, ocad_object, &entry);
				// This is done internally by libocad (in a slightly more imprecise way using the extent specified in the symbol)
				//entry->rect.min = convertPoint(MapCoord(object->getExtent().topLeft()));
				//entry->rect.max = convertPoint(MapCoord(object->getExtent().bottomRight()));
				entry->npts = ocad_object->npts + ocad_object->ntext;
				//entry->symbol = index_to_use;
			}
		}
	}
	
	// Templates
	for (int i = map->getNumTemplates() - 1; i >= 0; --i)
	{
		const Template* temp = map->getTemplate(i);
		
		QString template_path = temp->getTemplatePath();
		
		auto supported_by_ocd = false;
		if (qstrcmp(temp->getTemplateType(), "TemplateImage") == 0)
		{
			supported_by_ocd = true;
			
			if (temp->isTemplateGeoreferenced())
			{
				if (temp->getTemplateState() == Template::Unloaded)
				{
					// Try to load the template, so that the positioning gets set.
					const_cast<Template*>(temp)->loadTemplateFile(false);
				}
				
				if (temp->getTemplateState() != Template::Loaded)
				{
					addWarning(tr("Unable to save correct position of missing template: \"%1\"")
					           .arg(temp->getTemplateFilename()));
				}
			}
		}
		else if (QFileInfo(template_path).suffix().compare(QLatin1String("ocd"), Qt::CaseInsensitive) == 0)
		{
			supported_by_ocd = true;
		}
		
		if (supported_by_ocd)
		{
			// FIXME: export template view parameters
			
			double a = temp->getTemplateRotation() * 180 / M_PI;
			int d = 0;
			int o = 0;
			int p = 0;
			int s = 1;	// enabled
			int t = 0;
			OCADPoint pos = convertPoint(temp->getTemplateX()-area_offset.nativeX(), temp->getTemplateY()-area_offset.nativeY());
			int x = pos.x >> 8;
			int y = pos.y >> 8;
			double u = convertTemplateScale(temp->getTemplateScaleX());
			double v = convertTemplateScale(temp->getTemplateScaleY());
			
			template_path.replace(QLatin1Char('/'), QLatin1Char('\\'));
			
			QString string;
			string.sprintf("\ts%d\tx%d\ty%d\ta%f\tu%f\tv%f\td%d\tp%d\tt%d\to%d",
				s, x, y, a, u, v, d, p, t, o
			);
			string.prepend(template_path);
			
			OCADStringEntry* entry = ocad_string_entry_new(file, string.length() + 1);
			entry->type = 8;
			convertCString(string, file->buffer + entry->ptr, entry->size);
		}
		else
		{
			addWarning(tr("Unable to export template: file type of \"%1\" is not supported yet").arg(temp->getTemplateFilename()));
		}
	}
	
	device()->write((char*)file->buffer, file->size);
	
	ocad_file_close(file);
	return true;
}


MapCoord OCAD8FileExport::calculateAreaOffset()
{
	auto area_offset = QPointF{};
	
	// Attention: When changing ocd_bounds, update the warning messages, too.
	auto ocd_bounds = QRectF{QPointF{-2000, -2000}, QPointF{2000, 2000}};
	auto objects_extent = map->calculateExtent();
	if (!ocd_bounds.contains(objects_extent))
	{
		if (objects_extent.width() < ocd_bounds.width()
		    && objects_extent.height() < ocd_bounds.height())
		{
			// The extent fits into the limited area.
			addWarning(tr("Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m)."));
			area_offset = objects_extent.center();
		}
		else
		{
			// The extent is too wide to fit.
			
			// Only move the objects if they are completely outside the drawing area.
			// This avoids repeated moves on open/save/close cycles.
			if (!objects_extent.intersects(ocd_bounds))
			{
				addWarning(tr("Coordinates are adjusted to fit into the OCAD 8 drawing area (-2 m ... 2 m)."));
				std::size_t count = 0;
				auto calculate_average_center = [&area_offset, &count](const Object* object)
				{
					area_offset *= qreal(count)/qreal(count+1);
					++count;
					area_offset += object->getExtent().center() / count;
				};
				map->applyOnAllObjects(calculate_average_center);
			}
			
			addWarning(tr("Some coordinates remain outside of the OCAD 8 drawing area."
			              " They might be unreachable in OCAD."));
		}
		
		if (area_offset.manhattanLength() > 0)
		{
			// Round offset to 100 m in projected coordinates, to avoid crude grid offset.
			constexpr auto unit = 100;
			auto projected_offset = map->getGeoreferencing().toProjectedCoords(MapCoordF(area_offset));
			projected_offset.rx() = qreal(qRound(projected_offset.x()/unit)) * unit;
			projected_offset.ry() = qreal(qRound(projected_offset.y()/unit)) * unit;
			area_offset = map->getGeoreferencing().toMapCoordF(projected_offset);
		}
	}
	
	return MapCoord{area_offset};
}


void OCAD8FileExport::exportCommonSymbolFields(const Symbol* symbol, OCADSymbol* ocad_symbol, int size)
{
	ocad_symbol->size = (s16)size;
	convertPascalString(symbol->getPlainTextName(), ocad_symbol->name, 32);
	ocad_symbol->number = symbol->getNumberComponent(0) * 10;
	if (symbol->getNumberComponent(1) >= 0)
		ocad_symbol->number += (symbol->getNumberComponent(1) % 10);
	// Symbol number 0.0 is not valid
	if (ocad_symbol->number == 0)
		ocad_symbol->number = 1;
	// Ensure uniqueness of the symbol number
	while (symbol_numbers.find(ocad_symbol->number) != symbol_numbers.end())
		++ocad_symbol->number;
	symbol_numbers.insert(ocad_symbol->number);
	
	if (symbol->isProtected())
		ocad_symbol->status |= 1;
	if (symbol->isHidden())
		ocad_symbol->status |= 2;
	
	// Set of used colors
	u8 bitmask = 1;
	u8* bitpos = ocad_symbol->colors;
	for (int c = 0; c < map->getNumColors(); ++c)
	{
		if (symbol->containsColor(map->getColor(c)))
			*bitpos |= bitmask;
		
		bitmask = bitmask << 1;
		if (bitmask == 0)
		{
			bitmask = 1;
			++bitpos;
		}
	}
	
	exportSymbolIcon(symbol, ocad_symbol->icon);
}

void OCAD8FileExport::exportSymbolIcon(const Symbol* symbol, u8 ocad_icon[])
{
	// Icon: 22x22 with 4 bit color code, origin at bottom left
	constexpr int icon_size = 22;
	QImage image = symbol->createIcon(*map, icon_size, false)
	               .convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
	auto process_pixel = [&image](int x, int y)->int
	{
		// Apply premultiplied pixel on white background
		auto premultiplied = image.pixel(x, y);
		auto alpha = qAlpha(premultiplied);
		auto r = 255 - alpha + qRed(premultiplied);
		auto g = 255 - alpha + qGreen(premultiplied);
		auto b = 255 - alpha + qBlue(premultiplied);
		auto pixel = qRgb(r, g, b);
		
		// Ordered dithering 2x2 threshold matrix, adjusted for o-map halftones
		static int threshold[4] = { 24, 192, 136, 80 };
		auto palette_color = getOcadColor(pixel);
		switch (palette_color)
		{
		case 0:
			// Black to gray (50%)
			return  qGray(pixel) < 128-threshold[(x%2 + 2*(y%2))]/2 ? 0 : 7;
			
		case 7:
			// Gray (50%) to light gray 
			return  qGray(pixel) < 192-threshold[(x%2 + 2*(y%2))]/4 ? 7 : 8;
			
		case 8:
			// Light gray to white
			return  qGray(pixel) < 256-threshold[(x%2 + 2*(y%2))]/4 ? 8 : 15;
			
		case 15:
			// Pure white
			return palette_color;
			
		default:
			// Color to white
			return  QColor(pixel).saturation() >= threshold[(x%2 + 2*(y%2))] ? palette_color : 15;
		}
	};
	
	for (int y = icon_size - 1; y >= 0; --y)
	{
		for (int x = 0; x < icon_size; x += 2)
		{
			auto first = process_pixel(x, y);
			auto second = process_pixel(x+1, y);
			*(ocad_icon++) = u8((first << 4) + second);
		}
		ocad_icon++;
	}
}

int OCAD8FileExport::getPatternSize(const PointSymbol* point)
{
	if (!point)
		return 0;
	
	int npts = 0;
	for (int i = 0; i < point->getNumElements(); ++i)
	{
		int factor = 1;
		if (point->getElementSymbol(i)->getType() == Symbol::Point)
		{
			factor = 0;
			const PointSymbol* point_symbol = static_cast<const PointSymbol*>(point->getElementSymbol(i));
			if (point_symbol->getInnerRadius() > 0 && point_symbol->getInnerColor())
				++factor;
			if (point_symbol->getOuterWidth() > 0 && point_symbol->getOuterColor())
				++factor;
		}
		npts += factor * (2 + point->getElementObject(i)->getRawCoordinateVector().size());
	}
	if (point->getInnerRadius() > 0 && point->getInnerColor())
		npts += 2 + 1;
	if (point->getOuterWidth() > 0 && point->getOuterColor())
		npts += 2 + 1;
	
	return npts * sizeof(OCADPoint);
}

s16 OCAD8FileExport::exportPattern(const PointSymbol* point, OCADPoint** buffer)
{
	if (!point)
		return 0;
	
	s16 num_coords = exportSubPattern(origin_point_object, point, buffer);
	for (int i = 0; i < point->getNumElements(); ++i)
	{
		num_coords += exportSubPattern(point->getElementObject(i), point->getElementSymbol(i), buffer);
	}
	return num_coords;
}

s16 OCAD8FileExport::exportSubPattern(const Object* object, const Symbol* symbol, OCADPoint** buffer)
{
	s16 num_coords = 0;
	OCADSymbolElement* element = (OCADSymbolElement*)*buffer;
	
	if (symbol->getType() == Symbol::Point)
	{
		const PointSymbol* point_symbol = static_cast<const PointSymbol*>(symbol);
		if (point_symbol->getInnerRadius() > 0 && point_symbol->getInnerColor())
		{
			element->type = 4;
			element->color = convertColor(point_symbol->getInnerColor());
			element->diameter = convertSize(2 * point_symbol->getInnerRadius());
			(*buffer) += 2;
			element->npts = exportCoordinates(object->getRawCoordinateVector(), buffer, point_symbol);
			num_coords += 2 + element->npts;
		}
		if (point_symbol->getOuterWidth() > 0 && point_symbol->getOuterColor())
		{
			element = (OCADSymbolElement*)*buffer;
			element->type = 3;
			element->color = convertColor(point_symbol->getOuterColor());
			element->width = convertSize(point_symbol->getOuterWidth());
			element->diameter = convertSize(2 * point_symbol->getInnerRadius() + 2 * point_symbol->getOuterWidth());
			(*buffer) += 2;
			element->npts = exportCoordinates(object->getRawCoordinateVector(), buffer, point_symbol);
			num_coords += 2 + element->npts;
		}
	}
	else if (symbol->getType() == Symbol::Line)
	{
		const LineSymbol* line_symbol = static_cast<const LineSymbol*>(symbol);
		element->type = 1;
		if (line_symbol->getCapStyle() == LineSymbol::RoundCap)
			element->flags |= 1;
		else if (line_symbol->getJoinStyle() == LineSymbol::MiterJoin)
			element->flags |= 4;
		element->color = convertColor(line_symbol->getColor());
		element->width = convertSize(line_symbol->getLineWidth());
		(*buffer) += 2;
		element->npts = exportCoordinates(object->getRawCoordinateVector(), buffer, line_symbol);
		num_coords += 2 + element->npts;
	}
	else if (symbol->getType() == Symbol::Area)
	{
		const AreaSymbol* area_symbol = static_cast<const AreaSymbol*>(symbol);
		element->type = 2;
		element->color = convertColor(area_symbol->getColor());
		(*buffer) += 2;
		element->npts = exportCoordinates(object->getRawCoordinateVector(), buffer, area_symbol);
		num_coords += 2 + element->npts;
	}
	else
		Q_ASSERT(false);
	return num_coords;
}

s16 OCAD8FileExport::exportPointSymbol(const PointSymbol* point)
{
	int data_size = (sizeof(OCADPointSymbol) - sizeof(OCADPoint)) + getPatternSize(point);
	OCADPointSymbol* ocad_symbol = (OCADPointSymbol*)ocad_symbol_new(file, data_size);
	exportCommonSymbolFields(point, (OCADSymbol*)ocad_symbol, data_size);
	
	ocad_symbol->type = OCAD_POINT_SYMBOL;
	ocad_symbol->extent = getPointSymbolExtent(point);
	if (ocad_symbol->extent <= 0)
		ocad_symbol->extent = 100;
	if (point->isRotatable())
		ocad_symbol->base_flags |= 1;
	ocad_symbol->ngrp = (data_size - (sizeof(OCADPointSymbol) - sizeof(OCADPoint))) / 8;
	
	OCADPoint* pattern_buffer = ocad_symbol->pts;
	exportPattern(point, &pattern_buffer);
	Q_ASSERT((u8*)ocad_symbol + data_size == (u8*)pattern_buffer);
	return ocad_symbol->number;
}

s16 OCAD8FileExport::exportLineSymbol(const LineSymbol* line)
{
	int data_size = (sizeof(OCADLineSymbol) - sizeof(OCADPoint)) +
					getPatternSize(line->getStartSymbol()) +
					getPatternSize(line->getEndSymbol()) +
					getPatternSize(line->getMidSymbol()) +
					getPatternSize(line->getDashSymbol());
	OCADLineSymbol* ocad_symbol = (OCADLineSymbol*)ocad_symbol_new(file, data_size);
	exportCommonSymbolFields(line, (OCADSymbol*)ocad_symbol, data_size);
	
	// Basic settings
	ocad_symbol->type = OCAD_LINE_SYMBOL;
	s16 extent = convertSize(0.5f * line->getLineWidth());
	if (line->hasBorder())
		extent = qMax(extent, (s16)convertSize(0.5f * line->getLineWidth() + line->getBorder().shift + 0.5f * line->getBorder().width));
	extent = qMax(extent, getPointSymbolExtent(line->getStartSymbol()));
	extent = qMax(extent, getPointSymbolExtent(line->getEndSymbol()));
	extent = qMax(extent, getPointSymbolExtent(line->getMidSymbol()));
	extent = qMax(extent, getPointSymbolExtent(line->getDashSymbol()));
	ocad_symbol->extent = extent;
	ocad_symbol->color = convertColor(line->getColor());
	if (line->getColor())
		ocad_symbol->width = convertSize(line->getLineWidth());
	
	// Cap and Join
	if (line->getCapStyle() == LineSymbol::FlatCap && line->getJoinStyle() == LineSymbol::BevelJoin)
		ocad_symbol->ends = 0;
	else if (line->getCapStyle() == LineSymbol::RoundCap && line->getJoinStyle() == LineSymbol::RoundJoin)
		ocad_symbol->ends = 1;
	else if (line->getCapStyle() == LineSymbol::PointedCap && line->getJoinStyle() == LineSymbol::BevelJoin)
		ocad_symbol->ends = 2;
	else if (line->getCapStyle() == LineSymbol::PointedCap && line->getJoinStyle() == LineSymbol::RoundJoin)
		ocad_symbol->ends = 3;
	else if (line->getCapStyle() == LineSymbol::FlatCap && line->getJoinStyle() == LineSymbol::MiterJoin)
		ocad_symbol->ends = 4;
	else if (line->getCapStyle() == LineSymbol::PointedCap && line->getJoinStyle() == LineSymbol::MiterJoin)
		ocad_symbol->ends = 6;
	else
	{
		addWarning(tr("In line symbol \"%1\", cannot represent cap/join combination.").arg(line->getPlainTextName()));
		// Decide based on the caps
		if (line->getCapStyle() == LineSymbol::FlatCap)
			ocad_symbol->ends = 0;
		else if (line->getCapStyle() == LineSymbol::RoundCap)
			ocad_symbol->ends = 1;
		else if (line->getCapStyle() == LineSymbol::PointedCap)
			ocad_symbol->ends = 3;
		else if (line->getCapStyle() == LineSymbol::SquareCap)
			ocad_symbol->ends = 0;
	}
	
	ocad_symbol->bdist = convertSize(line->startOffset());
	ocad_symbol->edist = convertSize(line->endOffset());
	
	// Dash pattern
	if (line->isDashed())
	{
		if (line->getMidSymbol() && !line->getMidSymbol()->isEmpty())
		{
			if (line->getDashesInGroup() > 1)
				addWarning(tr("In line symbol \"%1\", neglecting the dash grouping.").arg(line->getPlainTextName()));
			
			ocad_symbol->len = convertSize(line->getDashLength() + line->getBreakLength());
			ocad_symbol->elen = ocad_symbol->len / 2;
			ocad_symbol->gap2 = convertSize(line->getBreakLength());
		}
		else
		{
			if (line->getDashesInGroup() > 1)
			{
				if (line->getDashesInGroup() > 2)
					addWarning(tr("In line symbol \"%1\", the number of dashes in a group has been reduced to 2.").arg(line->getPlainTextName()));
				
				ocad_symbol->len = convertSize(2 * line->getDashLength() + line->getInGroupBreakLength());
				ocad_symbol->elen = convertSize(2 * line->getDashLength() + line->getInGroupBreakLength());
				ocad_symbol->gap = convertSize(line->getBreakLength());
				ocad_symbol->gap2 = convertSize(line->getInGroupBreakLength());
				ocad_symbol->egap = ocad_symbol->gap2;
			}
			else
			{
				ocad_symbol->len = convertSize(line->getDashLength());
				ocad_symbol->elen = ocad_symbol->len / (line->getHalfOuterDashes() ? 2 : 1);
				ocad_symbol->gap = convertSize(line->getBreakLength());
			}
		}
	}
	else
	{
		ocad_symbol->len = convertSize(line->getSegmentLength());
		ocad_symbol->elen = convertSize(line->getEndLength());
	}
	
	ocad_symbol->smin = line->getShowAtLeastOneSymbol() ? 0 : -1;
	
	// Double line
	if (line->hasBorder() && (line->getBorder().isVisible() || line->getRightBorder().isVisible()))
	{
		ocad_symbol->dwidth = convertSize(line->getLineWidth() - line->getBorder().width + 2 * line->getBorder().shift);
		if (line->getBorder().dashed && !line->getRightBorder().dashed)
			ocad_symbol->dmode = 2;
		else
			ocad_symbol->dmode = line->getBorder().dashed ? 3 : 1;
		// ocad_symbol->dflags
		
		ocad_symbol->lwidth = convertSize(line->getBorder().width);
		ocad_symbol->rwidth = convertSize(line->getRightBorder().width);
		
		ocad_symbol->lcolor = convertColor(line->getBorder().color);
		ocad_symbol->rcolor = convertColor(line->getRightBorder().color);
		
		if (line->getBorder().dashed)
		{
			ocad_symbol->dlen = convertSize(line->getBorder().dash_length);
			ocad_symbol->dgap = convertSize(line->getBorder().break_length);
		}
		else if (line->getRightBorder().dashed)
		{
			ocad_symbol->dlen = convertSize(line->getRightBorder().dash_length);
			ocad_symbol->dgap = convertSize(line->getRightBorder().break_length);
		}
		
		if (((line->getBorder().dashed && line->getRightBorder().dashed) &&
				(line->getBorder().dash_length != line->getRightBorder().dash_length ||
				line->getBorder().break_length != line->getRightBorder().break_length)) ||
			(!line->getBorder().dashed && line->getRightBorder().dashed))
		{
			addWarning(tr("In line symbol \"%1\", cannot export the borders correctly.").arg(line->getPlainTextName()));
		}
	}
	
	// Mid symbol
	OCADPoint* pattern_buffer = ocad_symbol->pts;
	ocad_symbol->smnpts = exportPattern(line->getMidSymbol(), &pattern_buffer);
	ocad_symbol->snum = line->getMidSymbolsPerSpot();
	ocad_symbol->sdist = convertSize(line->getMidSymbolDistance());
	
	// No secondary symbol
	ocad_symbol->ssnpts = 0;
	
	// Export dash symbol as corner symbol
	ocad_symbol->scnpts = exportPattern(line->getDashSymbol(), &pattern_buffer);
	
	// Start symbol
	ocad_symbol->sbnpts = exportPattern(line->getStartSymbol(), &pattern_buffer);
	
	// End symbol
	ocad_symbol->senpts = exportPattern(line->getEndSymbol(), &pattern_buffer);
	
	Q_ASSERT((u8*)ocad_symbol + data_size == (u8*)pattern_buffer);
	return ocad_symbol->number;
}

s16 OCAD8FileExport::exportAreaSymbol(const AreaSymbol* area)
{
	int data_size = (sizeof(OCADAreaSymbol) - sizeof(OCADPoint));
	for (int i = 0, end = area->getNumFillPatterns(); i < end; ++i)
	{
		if (area->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
		{
			data_size += getPatternSize(area->getFillPattern(i).point);
			break;
		}
	}
	OCADAreaSymbol* ocad_symbol = (OCADAreaSymbol*)ocad_symbol_new(file, data_size);
	exportCommonSymbolFields(area, (OCADSymbol*)ocad_symbol, data_size);
	
	// Basic settings
	ocad_symbol->type = OCAD_AREA_SYMBOL;
	ocad_symbol->extent = 0;
	if (area->getColor())
	{
		ocad_symbol->fill = 1;
		ocad_symbol->color = convertColor(area->getColor());
	}
	
	// Hatch
	ocad_symbol->hmode = 0;
	for (int i = 0, end = area->getNumFillPatterns(); i < end; ++i)
	{
		const AreaSymbol::FillPattern& pattern = area->getFillPattern(i);
		if (pattern.type == AreaSymbol::FillPattern::LinePattern)
		{
			if ( (ocad_symbol->hmode == 1 && ocad_symbol->hcolor != convertColor(pattern.line_color)) ||
			     ocad_symbol->hmode == 2 )
			{
				addWarning(tr("In area symbol \"%1\", skipping a fill pattern.").arg(area->getPlainTextName()));
				continue;
			}
			
			if (pattern.rotatable())
				ocad_symbol->base_flags |= 1;
			
			++ocad_symbol->hmode;
			if (ocad_symbol->hmode == 1)
			{
				ocad_symbol->hcolor = convertColor(pattern.line_color);
				ocad_symbol->hwidth = convertSize(pattern.line_width);
				ocad_symbol->hdist = convertSize(pattern.line_spacing - pattern.line_width);
				ocad_symbol->hangle1 = convertRotation(pattern.angle);
			}
			else if (ocad_symbol->hmode == 2)
			{
				ocad_symbol->hwidth = (ocad_symbol->hwidth + convertSize(pattern.line_width)) / 2;
				ocad_symbol->hdist = (ocad_symbol->hdist + convertSize(pattern.line_spacing - pattern.line_width)) / 2;
				ocad_symbol->hangle2 = convertRotation(pattern.angle);
			}
		}
	}
	
	// Struct
	PointSymbol* point_pattern = nullptr;
	for (int i = 0, end = area->getNumFillPatterns(); i < end; ++i)
	{
		const AreaSymbol::FillPattern& pattern = area->getFillPattern(i);
		if (pattern.type == AreaSymbol::FillPattern::PointPattern)
		{
			if (pattern.rotatable())
				ocad_symbol->base_flags |= 1;
			
			++ocad_symbol->pmode;
			if (ocad_symbol->pmode == 1)
			{
				ocad_symbol->pwidth = convertSize(pattern.point_distance);
				ocad_symbol->pheight = convertSize(pattern.line_spacing);
				ocad_symbol->pangle = convertRotation(pattern.angle);
				point_pattern = pattern.point;
			}
			else if (ocad_symbol->pmode == 2)
			{
				// NOTE: This is only a heuristic which works for the orienteering symbol sets, not a real conversion, which would be impossible in most cases.
				//       There are no further checks done to find out if the conversion is applicable because with these checks, already a tiny (not noticeable) error
				//       in the symbol definition would make it take the wrong choice.
				addWarning(tr("In area symbol \"%1\", assuming a \"shifted rows\" point pattern. This might be correct as well as incorrect.").arg(area->getPlainTextName()));
				
				if (pattern.line_offset != 0)
					ocad_symbol->pheight /= 2;
				else
					ocad_symbol->pwidth /= 2;
				
				break;
			}
		}
	}
	
	if (point_pattern)
	{
		OCADPoint* pattern_buffer = ocad_symbol->pts;
		ocad_symbol->npts = exportPattern(point_pattern, &pattern_buffer);
		Q_ASSERT((u8*)ocad_symbol + data_size == (u8*)pattern_buffer);
	}
	return ocad_symbol->number;
}

s16 OCAD8FileExport::exportTextSymbol(const TextSymbol* text)
{
	int data_size = sizeof(OCADTextSymbol);
	OCADTextSymbol* ocad_symbol = (OCADTextSymbol*)ocad_symbol_new(file, data_size);
	exportCommonSymbolFields(text, (OCADSymbol*)ocad_symbol, data_size);
	
	ocad_symbol->type = OCAD_TEXT_SYMBOL;
	ocad_symbol->subtype = 1;
	ocad_symbol->extent = 0;
	
	convertPascalString(text->getFontFamily(), ocad_symbol->font, 32);
	ocad_symbol->color = convertColor(text->getColor());
	ocad_symbol->dpts = qRound(10 * text->getFontSize() / 25.4 * 72.0);
	ocad_symbol->bold = text->isBold() ? 700 : 400;
	ocad_symbol->italic = text->isItalic() ? 1 : 0;
	//ocad_symbol->charset
	ocad_symbol->cspace = convertSize(1000 * text->getCharacterSpacing());
	if (ocad_symbol->cspace != 0)
		addWarning(tr("In text symbol %1: custom character spacing is set, its implementation does not match OCAD's behavior yet").arg(text->getPlainTextName()));
	ocad_symbol->wspace = 100;
	ocad_symbol->halign = 0;	// Default value, we might have to change this or even create copies of this symbol with other alignments later
	double absolute_line_spacing = text->getLineSpacing() * (text->getFontMetrics().lineSpacing() / text->calculateInternalScaling());
	ocad_symbol->lspace = qRound(absolute_line_spacing / (text->getFontSize() * 0.01));
	ocad_symbol->pspace = convertSize(1000 * text->getParagraphSpacing());
	if (text->isUnderlined())
		addWarning(tr("In text symbol %1: ignoring underlining").arg(text->getPlainTextName()));
	if (text->usesKerning())
		addWarning(tr("In text symbol %1: ignoring kerning").arg(text->getPlainTextName()));
	
	ocad_symbol->under = text->hasLineBelow() ? 1 : 0;
	ocad_symbol->ucolor = convertColor(text->getLineBelowColor());
	ocad_symbol->uwidth = convertSize(1000 * text->getLineBelowWidth());
	ocad_symbol->udist = convertSize(1000 * text->getLineBelowDistance());
	
	ocad_symbol->ntabs = text->getNumCustomTabs();
	for (int i = 0; i < qMin((s16)32, ocad_symbol->ntabs); ++i)
		ocad_symbol->tab[i] = convertSize(text->getCustomTab(i));
	
	if (text->getFramingMode() != TextSymbol::NoFraming && text->getFramingColor())
	{
		ocad_symbol->fcolor = convertColor(text->getFramingColor());
		if (text->getFramingMode() == TextSymbol::ShadowFraming)
		{
			ocad_symbol->fmode = 1;
			ocad_symbol->fdx = convertSize(text->getFramingShadowXOffset());
			ocad_symbol->fdy = -1 * convertSize(text->getFramingShadowYOffset());
		}
		else if (text->getFramingMode() == TextSymbol::LineFraming)
		{
			ocad_symbol->fmode = 2;
			ocad_symbol->fdpts = convertSize(text->getFramingLineHalfWidth());
		}
		else
			Q_ASSERT(false);
	}
	
	return ocad_symbol->number;
}

void OCAD8FileExport::setTextSymbolFormatting(OCADTextSymbol* ocad_symbol, TextObject* formatting)
{
	if (formatting->getHorizontalAlignment() == TextObject::AlignLeft)
		ocad_symbol->halign = 0;
	else if (formatting->getHorizontalAlignment() == TextObject::AlignHCenter)
		ocad_symbol->halign = 1;
	else if (formatting->getHorizontalAlignment() == TextObject::AlignRight)
		ocad_symbol->halign = 2;
}

std::set< s16 > OCAD8FileExport::exportCombinedSymbol(const CombinedSymbol* combination)
{
	// Insert public parts
	std::vector<bool> map_bitfield;
	map_bitfield.assign(map->getNumSymbols(), false);
	map_bitfield[map->findSymbolIndex(combination)] = true;
	map->determineSymbolUseClosure(map_bitfield);
	
	std::set<s16> result;
	for (size_t i = 0, end = map_bitfield.size(); i < end; ++i)
	{
		if (map_bitfield[i] && symbol_index.contains(map->getSymbol(i)))
		{
			result.insert(symbol_index[map->getSymbol(i)].begin(),
			              symbol_index[map->getSymbol(i)].end());
		}
	}
	
	// Insert private parts
	for (int i = 0; i < combination->getNumParts(); ++i)
	{
		if (combination->isPartPrivate(i))
		{
			const Symbol* part = combination->getPart(i);
			int index = 0;
			if (part->getType() == Symbol::Line)
				index = exportLineSymbol(part->asLine());
			else if (part->getType() == Symbol::Area)
				index = exportAreaSymbol(part->asArea());
			else
				Q_ASSERT(false);
			result.insert(index);
		}
	}
	
	return result;
}

u16 OCAD8FileExport::exportCoordinates(const MapCoordVector& coords, OCADPoint** buffer, const Symbol* symbol)
{
	s16 num_points = 0;
	bool curve_start = false;
	bool hole_point = false;
	bool curve_continue = false;
	for (size_t i = 0, end = coords.size(); i < end; ++i)
	{
		const MapCoord& point = coords[i];
		OCADPoint p = convertPoint(point);
		if (point.isDashPoint())
		{
			if (!symbol || symbol->getType() != Symbol::Line)
				p.y |= PY_CORNER;
			else
			{
				const LineSymbol* line_symbol = static_cast<const LineSymbol*>(symbol);
				if ((line_symbol->getDashSymbol() == nullptr || line_symbol->getDashSymbol()->isEmpty()) && line_symbol->isDashed())
					p.y |= PY_DASH;
				else
					p.y |= PY_CORNER;
			}
		}
		if (curve_start)
			p.x |= PX_CTL1;
		if (hole_point)
			p.y |= PY_HOLE;
		if (curve_continue)
			p.x |= PX_CTL2;
		
		curve_continue = curve_start;
		curve_start = point.isCurveStart();
		hole_point = point.isHolePoint();
		
		**buffer = p;
		++(*buffer);
		++num_points;
	}
	return num_points;
}

u16 OCAD8FileExport::exportTextCoordinates(TextObject* object, OCADPoint** buffer)
{
	if (object->getNumLines() == 0)
		return 0;
	
	QTransform text_to_map = object->calcTextToMapTransform();
	QTransform map_to_text = object->calcMapToTextTransform();
	
	if (object->hasSingleAnchor())
	{
		// Create 5 coordinates:
		// 0 - baseline anchor point
		// 1 - bottom left
		// 2 - bottom right
		// 3 - top right
		// 4 - top left
		
		QPointF anchor = QPointF(object->getAnchorCoordF());
		QPointF anchor_text = map_to_text.map(anchor);
		
		TextObjectLineInfo* line0 = object->getLineInfo(0);
		**buffer = convertPoint(MapCoord(text_to_map.map(QPointF(anchor_text.x(), line0->line_y))));
		++(*buffer);
		
		QRectF bounding_box_text;
		for (int i = 0; i < object->getNumLines(); ++i)
		{
			TextObjectLineInfo* info = object->getLineInfo(i);
			rectIncludeSafe(bounding_box_text, QPointF(info->line_x, info->line_y - info->ascent));
			rectIncludeSafe(bounding_box_text, QPointF(info->line_x + info->width, info->line_y + info->descent));
		}
		
		**buffer = convertPoint(MapCoord(text_to_map.map(bounding_box_text.bottomLeft())));
		++(*buffer);
		**buffer = convertPoint(MapCoord(text_to_map.map(bounding_box_text.bottomRight())));
		++(*buffer);
		**buffer = convertPoint(MapCoord(text_to_map.map(bounding_box_text.topRight())));
		++(*buffer);
		**buffer = convertPoint(MapCoord(text_to_map.map(bounding_box_text.topLeft())));
		++(*buffer);
		
		return 5;
	}
	else
	{
		// As OCD 8 only supports Top alignment, we have to replace the top box coordinates by the top coordinates of the first line
		const TextSymbol* text_symbol = static_cast<const TextSymbol*>(object->getSymbol());
		QFontMetricsF metrics = text_symbol->getFontMetrics();
		double internal_scaling = text_symbol->calculateInternalScaling();
		TextObjectLineInfo* line0 = object->getLineInfo(0);
		
		double new_top = (object->getVerticalAlignment() == TextObject::AlignTop) ? (-object->getBoxHeight() / 2) : ((line0->line_y - line0->ascent) / internal_scaling);
		// Account for extra internal leading
		double top_adjust = -text_symbol->getFontSize() + (metrics.ascent() + metrics.descent() + 0.5) / internal_scaling;
		new_top = new_top - top_adjust;
		
		QTransform transform;
		transform.rotate(-object->getRotation() * 180 / M_PI);
		**buffer = convertPoint(MapCoord(transform.map(QPointF(-object->getBoxWidth() / 2, object->getBoxHeight() / 2)) + object->getAnchorCoordF()));
		++(*buffer);
		**buffer = convertPoint(MapCoord(transform.map(QPointF(object->getBoxWidth() / 2, object->getBoxHeight() / 2)) + object->getAnchorCoordF()));
		++(*buffer);
		**buffer = convertPoint(MapCoord(transform.map(QPointF(object->getBoxWidth() / 2, new_top)) + object->getAnchorCoordF()));
		++(*buffer);
		**buffer = convertPoint(MapCoord(transform.map(QPointF(-object->getBoxWidth() / 2, new_top)) + object->getAnchorCoordF()));
		++(*buffer);
		
		return 4;
	}
}

// static
int OCAD8FileExport::getOcadColor(QRgb rgb)
{
	static const QColor ocad_colors[16] = {
		QColor(  0,   0,   0).toHsv(),
		QColor(128,   0,   0).toHsv(),
		QColor(0,   128,   0).toHsv(),
		QColor(128, 128,   0).toHsv(),
		QColor(  0,   0, 128).toHsv(),
		QColor(128,   0, 128).toHsv(),
		QColor(  0, 128, 128).toHsv(),
		QColor(128, 128, 128).toHsv(),
		QColor(192, 192, 192).toHsv(),
		QColor(255,   0,   0).toHsv(),
		QColor(  0, 255,   0).toHsv(),
		QColor(255, 255,   0).toHsv(),
		QColor(  0,   0, 255).toHsv(),
		QColor(255,   0, 255).toHsv(),
		QColor(  0, 255, 255).toHsv(),
		QColor(255, 255, 255).toHsv()
	};
	
	Q_ASSERT(qAlpha(rgb) == 255);
	
	// Quick return for frequent values
	if (rgb == qRgb(255, 255, 255))
		return 15;
	else if (rgb == qRgb(0, 0, 0))
		return 0;
	
	QColor color = QColor(rgb).toHsv();
	if (color.hue() == -1 || color.saturation() < 32)
	{
		auto gray = qGray(rgb);  // qGray is used for dithering
		if (gray >= 192)
			return 8;
		if (gray >= 128)
			return 7;
		return 0;
	}
	
	int best_index = 0;
	auto best_distance = std::numeric_limits<qreal>::max();
	for (auto i : { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14 })
	{
		// True color
		int hue_dist = qAbs(color.hue() - ocad_colors[i].hue());
		hue_dist = qMin(hue_dist, 360 - hue_dist);
		auto distance = qPow(hue_dist, 2)
		                + 0.1 * qPow(color.saturation() - ocad_colors[i].saturation(), 2)
		                + 0.1 * qPow(color.value() - ocad_colors[i].value(), 2);
		
		// (Too much) manual tweaking for orienteering colors
		if (i == 1)
			distance *= 1.5;	// Dark red
		else if (i == 3)
			distance *= 2;		// Olive
		else if (i == 11)
			distance *= 2;		// Yellow
		else if (i == 9)
			distance *= 3;		// Red is unlikely
		
		if (distance < best_distance)
		{
			best_distance = distance;
			best_index = i;
		}
	}
	return best_index;
}

s16 OCAD8FileExport::getPointSymbolExtent(const PointSymbol* symbol)
{
	if (!symbol)
		return 0;
	QRectF extent;
	for (int i = 0; i < symbol->getNumElements(); ++i)
	{
		QScopedPointer<Object> object(symbol->getElementObject(i)->duplicate());
		object->setSymbol(symbol->getElementSymbol(i), true);
		object->update();
		rectIncludeSafe(extent, object->getExtent());
		object->clearRenderables();
	}
	float float_extent = 0.5f * qMax(extent.width(), extent.height());
	if (symbol->getInnerColor())
		float_extent = qMax(float_extent, 0.001f * symbol->getInnerRadius());
	if (symbol->getOuterColor())
		float_extent = qMax(float_extent, 0.001f * (symbol->getInnerRadius() + symbol->getOuterWidth()));
	return convertSize(1000 * float_extent);
}

void OCAD8FileExport::convertPascalString(const QString& text, char* buffer, int buffer_size)
{
	Q_ASSERT(buffer_size <= 256);		// not possible to store a bigger length in the first byte
	int max_size = buffer_size - 1;
	
	if (text.length() > max_size)
		addStringTruncationWarning(text, max_size);
	
	QByteArray data = encoding_1byte->fromUnicode(text);
	int min_size = qMin(text.length(), max_size);
	*((unsigned char *)buffer) = min_size;
	memcpy(buffer + 1, data.data(), min_size);
}

void OCAD8FileExport::convertCString(const QString& text, unsigned char* buffer, int buffer_size)
{
	if (text.length() + 1 > buffer_size)
		addStringTruncationWarning(text, buffer_size - 1);
	
	QByteArray data = encoding_1byte->fromUnicode(text);
	int min_size = qMin(buffer_size - 1, data.length());
	memcpy(buffer, data.data(), min_size);
	buffer[min_size] = 0;
}

int OCAD8FileExport::convertWideCString(const QString& text, unsigned char* buffer, int buffer_size)
{
	// Convert text to Windows-OCAD format:
	// - if it starts with a newline, add another
	// - convert \n to \r\n
	QString exported_text;
	if (text.startsWith(QLatin1Char('\n')))
		exported_text = QLatin1Char('\n') + text;
	else
		exported_text = text;
	exported_text.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
	
	if (2 * (exported_text.length() + 1) > buffer_size)
		addStringTruncationWarning(exported_text, buffer_size - 1);
	
	// Do not add a byte order mark by using QTextCodec::IgnoreHeader
	QTextEncoder* encoder = encoding_2byte->makeEncoder(QTextCodec::IgnoreHeader);
	QByteArray data = encoder->fromUnicode(exported_text);
	delete encoder;
	
	int min_size = qMin(buffer_size - 2, data.length());
	memcpy(buffer, data.data(), min_size);
	buffer[min_size] = 0;
	buffer[min_size + 1] = 0;
	return min_size + 2;
}

int OCAD8FileExport::convertRotation(float angle)
{
	return qRound(10 * (angle * 180 / M_PI));
}

namespace
{
	constexpr s32 convertPointMember(s32 value)
	{
		return (value < -5) ? s32(0x80000000u | ((0x7fffffu & u32((value-4)/10)) << 8)) : s32((0x7fffffu & u32((value+5)/10)) << 8);
	}
	
	// convertPointMember() shall round half up.
	Q_STATIC_ASSERT(convertPointMember(-16) == s32(0xfffffe00u)); // __ down __
	Q_STATIC_ASSERT(convertPointMember(-15) == s32(0xffffff00u)); //     up
	Q_STATIC_ASSERT(convertPointMember( -6) == s32(0xffffff00u)); // __ down __
	Q_STATIC_ASSERT(convertPointMember( -5) == s32(0x00000000u)); //     up
	Q_STATIC_ASSERT(convertPointMember( -1) == s32(0x00000000u)); //     up
	Q_STATIC_ASSERT(convertPointMember(  0) == s32(0x00000000u)); //  unchanged
	Q_STATIC_ASSERT(convertPointMember( +1) == s32(0x00000000u)); //    down
	Q_STATIC_ASSERT(convertPointMember( +4) == s32(0x00000000u)); // __ down __
	Q_STATIC_ASSERT(convertPointMember( +5) == s32(0x00000100u)); //     up
	Q_STATIC_ASSERT(convertPointMember(+14) == s32(0x00000100u)); // __ down __
	Q_STATIC_ASSERT(convertPointMember(+15) == s32(0x00000200u)); //     up
	
#ifdef MAPPER_DEVELOPMENT_BUILD
	namespace broken
	{
		// Previous, broken implementation (#749)
		// Left here for reference.
		constexpr s32 convertPointMember(s32 value)
		{
			return (value < 0) ? (0x80000000 | ((0x7fffffu & ((value-5)/10)) << 8)) : ((0x7fffffu & ((value+5)/10)) << 8);
		}
	}
	// Actual behaviour of the broken implementation
	Q_STATIC_ASSERT(broken::convertPointMember(-16) == s32(0xfffffe00u)); //    down
	Q_STATIC_ASSERT(broken::convertPointMember(-15) == s32(0xfffffe00u)); // __ down __ (should be up)
	Q_STATIC_ASSERT(broken::convertPointMember(-14) == s32(0xffffff00u)); //     up
	Q_STATIC_ASSERT(broken::convertPointMember( -6) == s32(0xffffff00u)); //    down
	Q_STATIC_ASSERT(broken::convertPointMember( -5) == s32(0xffffff00u)); // __ down __ (should be up)
	Q_STATIC_ASSERT(broken::convertPointMember( -4) == s32(0x80000000u)); //   wrong    (should be 0x00000000u)
	Q_STATIC_ASSERT(broken::convertPointMember( -3) == s32(0x80000000u)); //   wrong    (should be 0x00000000u)
	Q_STATIC_ASSERT(broken::convertPointMember( -2) == s32(0x80000000u)); //   wrong    (should be 0x00000000u)
	Q_STATIC_ASSERT(broken::convertPointMember( -1) == s32(0x80000000u)); //   wrong    (should be 0x00000000u)
	Q_STATIC_ASSERT(broken::convertPointMember(  0) == s32(0x00000000u)); //  unchanged
	Q_STATIC_ASSERT(broken::convertPointMember( +1) == s32(0x00000000u)); //    down
	Q_STATIC_ASSERT(broken::convertPointMember( +4) == s32(0x00000000u)); // __ down __
	Q_STATIC_ASSERT(broken::convertPointMember( +5) == s32(0x00000100u)); //     up
	Q_STATIC_ASSERT(broken::convertPointMember(+14) == s32(0x00000100u)); // __ down __
	Q_STATIC_ASSERT(broken::convertPointMember(+15) == s32(0x00000200u)); //     up
#endif
}

OCADPoint OCAD8FileExport::convertPoint(qint32 x, qint32 y)
{
	return { convertPointMember(x), convertPointMember(-y) };
}

OCADPoint OCAD8FileExport::convertPoint(const MapCoord& coord)
{
	return convertPoint(coord.nativeX(), coord.nativeY());
}

s32 OCAD8FileExport::convertSize(qint32 size)
{
	return (s32)((size+5) / 10);
}

s16 OCAD8FileExport::convertColor(const MapColor* color) const
{
	int index = map->findColorIndex(color);
	if (index >= 0)
	{
		return uses_registration_color ? (index + 1) : index;
	}
	return 0;
}

double OCAD8FileExport::convertTemplateScale(double mapper_scale)
{
	return mapper_scale * (1 / 0.01);
}

void OCAD8FileExport::addStringTruncationWarning(const QString& text, int truncation_pos)
{
	QString temp = text;
	temp.insert(truncation_pos, QLatin1String("|||"));
	addWarning(tr("String truncated (truncation marked with three '|'): %1").arg(temp));
}


}  // namespace OpenOrienteering
