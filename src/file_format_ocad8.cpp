/*
 *    Copyright 2012 Pete Curtis
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
#include <QDateTime>

#include "file_format_ocad8.h"
#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"
#include "symbol_text.h"
#include "symbol_combined.h"
#include "template_image.h"
#include "object_text.h"
#include "util.h"

#if (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))
#define currentMSecsSinceEpoch() currentDateTime().toTime_t() * 1000
#endif

bool OCAD8FileFormat::understands(const unsigned char *buffer, size_t sz) const
{
    // The first two bytes of the file must be AD 0C.
    if (sz >= 2 && buffer[0] == 0xAD && buffer[1] == 0x0C) return true;
    return false;
}

Importer *OCAD8FileFormat::createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    return new OCAD8FileImport(path, map, view);
}




OCAD8FileImport::OCAD8FileImport(const QString &path, Map *map, MapView *view) : Importer(path, map, view), file(NULL)
{
    ocad_init();
    encoding_1byte = QTextCodec::codecForName("Windows-1252");
    encoding_2byte = QTextCodec::codecForName("UTF-16LE");
    offset_x = offset_y = 0;
}

OCAD8FileImport::~OCAD8FileImport()
{
    ocad_shutdown();
}

void OCAD8FileImport::doImport(bool load_symbols_only) throw (FormatException)
{
    //qint64 start = QDateTime::currentMSecsSinceEpoch();

	QByteArray filename = path.toLocal8Bit().constData();
    int err = ocad_file_open(&file, filename);
    //qDebug() << "open ocad file" << err;
    if (err != 0) throw FormatException(QObject::tr("Could not open file: libocad returned %1").arg(err));
	
	if (file->header->major <= 5 || file->header->major >= 9)
		throw FormatException(QObject::tr("OCAD files of version %1 cannot be loaded!").arg(file->header->major));

    //qDebug() << "file version is" << file->header->major << ", type is"
    //         << ((file->header->ftype == 2) ? "normal" : "other");
    //qDebug() << "map scale is" << file->setup->scale;

    map->scale_denominator = file->setup->scale;


    // TODO: GPS projection parameters

    // TODO: print parameters

    // Load colors
    int num_colors = ocad_color_count(file);

    for (int i = 0; i < num_colors; i++)
    {
        OCADColor *ocad_color = ocad_color_at(file, i);

        MapColor* color = new MapColor();
        color->priority = i;

        // OCAD stores CMYK values as integers from 0-200.
        color->c = 0.005f * ocad_color->cyan;
        color->m = 0.005f * ocad_color->magenta;
        color->y = 0.005f * ocad_color->yellow;
        color->k = 0.005f * ocad_color->black;
        color->opacity = 1.0f;
		color->name = convertPascalString(ocad_color->name);
        color->updateFromCMYK();

        map->color_set->colors.push_back(color);
        color_index[ocad_color->number] = color;
    }

    // Load symbols
    for (OCADSymbolIndex *idx = ocad_symidx_first(file); idx != NULL; idx = ocad_symidx_next(file, idx))
    {
        for (int i = 0; i < 256; i++)
        {
            OCADSymbol *ocad_symbol = ocad_symbol_at(file, idx, i);
            if (ocad_symbol != NULL && ocad_symbol->number != 0)
            {
                Symbol *symbol = NULL;
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
					
					// For combined symbols, also add their parts
					// FIXME: implement private parts for combined symbols instead
					if (symbol->getType() == Symbol::Combined)
					{
						CombinedSymbol* combined_symbol = reinterpret_cast<CombinedSymbol*>(symbol);
						for (int i = 0; i < combined_symbol->getNumParts(); ++i)
						{
							Symbol* part = combined_symbol->getPart(i);
							part->setNumberComponent(2, i+1);
							map->symbols.push_back(part);
						}
					}
                }
                else
                {
                    addWarning(QObject::tr("Unable to import symbol \"%3\" (%1.%2)")
                               .arg(ocad_symbol->number / 10).arg(ocad_symbol->number % 10)
                               .arg(convertPascalString(ocad_symbol->name)));
                }
            }
        }
    }

    if (!load_symbols_only)
	{
		// Load objects

		// Place all objects into a single OCAD import layer
		MapLayer* layer = new MapLayer(QObject::tr("OCAD import layer"), map);
		for (OCADObjectIndex *idx = ocad_objidx_first(file); idx != NULL; idx = ocad_objidx_next(file, idx))
		{
			for (int i = 0; i < 256; i++)
			{
				OCADObjectEntry *entry = ocad_object_entry_at(file, idx, i);
				OCADObject *ocad_obj = ocad_object(file, entry);
				if (ocad_obj != NULL)
				{
					Object *object = importObject(ocad_obj, layer);
					if (object != NULL) {
						layer->objects.push_back(object);
					}
				}
			}
		}
		map->layers.resize(1);
		map->layers[0] = layer;
		map->current_layer_index = 0;

		// Load templates
		map->templates.clear();
		map->first_front_template = 0;
		for (OCADTemplateIndex *idx = ocad_template_index_first(file); idx != NULL; idx = ocad_template_index_next(file, idx))
		{
			for (int i = 0; i < 256; i++)
			{
				OCADTemplateEntry *entry = ocad_template_entry_at(file, idx, i);
				if (entry->type != 0 && entry->size > 0)
				{
					Template *templ = importTemplate(entry);
					if (templ) map->templates.push_back(templ);
				}
			}
		}

		// TODO: Fill this->view with relevant fields from OCAD file
		/*
			file->read((char*)&zoom, sizeof(double));
			file->read((char*)&rotation, sizeof(double));
			file->read((char*)&position_x, sizeof(qint64));
			file->read((char*)&position_y, sizeof(qint64));
			file->read((char*)&view_x, sizeof(int));
			file->read((char*)&view_y, sizeof(int));
			file->read((char*)&drag_offset, sizeof(QPoint));
			update();

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

			map_editor->main_view = view;
		}
		*/

		// Undo steps are not supported in OCAD
	}
	else
	{
		MapLayer* layer = new MapLayer(QObject::tr("default"), map);
		map->layers.resize(1);
		map->layers[0] = layer;
		map->current_layer_index = 0;
	}

    ocad_file_close(file);

    //qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - start;
	//qDebug() << "OCAD map imported:"<<map->getNumSymbols()<<"symbols and"<<map->getNumObjects()<<"objects in"<<elapsed<<"milliseconds";
}

void OCAD8FileImport::setStringEncodings(const char *narrow, const char *wide) {
    encoding_1byte = QTextCodec::codecForName(narrow);
    encoding_2byte = QTextCodec::codecForName(wide);
}

Symbol *OCAD8FileImport::importPointSymbol(const OCADPointSymbol *ocad_symbol)
{
    PointSymbol *symbol = importPattern(ocad_symbol->ngrp, (OCADPoint *)ocad_symbol->pts);
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);
	symbol->setRotatable(ocad_symbol->subtype & 0x100);

    return symbol;
}

Symbol *OCAD8FileImport::importLineSymbol(const OCADLineSymbol *ocad_symbol)
{
	// Import a main line?
	LineSymbol *main_line = NULL;
	if (ocad_symbol->dmode == 0 || ocad_symbol->width > 0)
	{
		main_line = new LineSymbol();
		fillCommonSymbolFields(main_line, (OCADSymbol *)ocad_symbol);

		main_line->minimum_length = 0; // OCAD 8 does not store min length

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

		if (main_line->cap_style == LineSymbol::PointedCap)
		{
			if (ocad_symbol->bdist != ocad_symbol->edist)
				addWarning(QObject::tr("In dashed line symbol %1, pointed cap lengths for begin and end are different (%2 and %3). Using %4.")
				.arg(ocad_symbol->number).arg(ocad_symbol->bdist).arg(ocad_symbol->edist).arg((ocad_symbol->bdist + ocad_symbol->edist) / 2));
			main_line->pointed_cap_length = convertSize((ocad_symbol->bdist + ocad_symbol->edist) / 2); // FIXME: Different lengths for start and end length of pointed line ends are not supported yet, so take the average
			main_line->join_style = LineSymbol::RoundJoin;	// NOTE: while the setting may be different (see what is set in the first place), OCAD always draws round joins if the line cap is pointed!
		}
		
		// Handle the dash pattern
		if( ocad_symbol->gap > 0 || ocad_symbol->gap2 > 0 )
		{
			//dashed
			main_line->dashed = true;
			if (ocad_symbol->len != ocad_symbol->elen)
				addWarning(QObject::tr("In dashed line symbol %1, main and end length are different (%2 and %3). Using %4.")
				.arg(ocad_symbol->number).arg(ocad_symbol->len).arg(ocad_symbol->elen).arg(ocad_symbol->len));
			main_line->dash_length = convertSize(ocad_symbol->len);
			main_line->break_length = convertSize(ocad_symbol->gap);
			
			if (ocad_symbol->gap2 > 0)
			{
				main_line->dashes_in_group = 2;
				if (ocad_symbol->gap2 != ocad_symbol->egap)
					addWarning(QObject::tr("In dashed line symbol %1, gaps D and E are different (%2 and %3). Using %4.")
					.arg(ocad_symbol->number).arg(ocad_symbol->gap2).arg(ocad_symbol->egap).arg(ocad_symbol->gap2));
				main_line->in_group_break_length = convertSize(ocad_symbol->gap2);
				main_line->dash_length = (main_line->dash_length - main_line->in_group_break_length) / 2;
			}
		} 
		else
		{
			main_line->segment_length = convertSize(ocad_symbol->len);
			main_line->end_length = convertSize(ocad_symbol->elen);
		}
	}
	
	// Import a 'double' line?
	LineSymbol *double_line = NULL;
	if (ocad_symbol->dmode != 0)
	{
		double_line = new LineSymbol();
		fillCommonSymbolFields(double_line, (OCADSymbol *)ocad_symbol);
		
		double_line->line_width = convertSize(ocad_symbol->dwidth);
		if (ocad_symbol->dflags & 1)
			double_line->color = convertColor(ocad_symbol->dcolor);
		else
			double_line->color = NULL;
		
		double_line->cap_style = LineSymbol::FlatCap;
		double_line->join_style = LineSymbol::MiterJoin;
		
		// Border lines
		if (ocad_symbol->lwidth > 0 || ocad_symbol->rwidth > 0)
		{
			double_line->have_border_lines = true;
			
			// Border color and width - currently we don't support different values on left and right side,
			// although that seems easy enough to implement in the future. Import with a warning.
			s16 border_color = ocad_symbol->lcolor;
			if (border_color != ocad_symbol->rcolor)
			{
				addWarning(QObject::tr("In symbol %1, left and right borders are different colors (%2 and %3). Using %4.")
				.arg(ocad_symbol->number).arg(ocad_symbol->lcolor).arg(ocad_symbol->rcolor).arg(border_color));
			}
			double_line->border_color = convertColor(border_color);
			
			s16 border_width = ocad_symbol->lwidth;
			if (border_width != ocad_symbol->rwidth)
			{
				addWarning(QObject::tr("In symbol %1, left and right borders are different width (%2 and %3). Using %4.")
				.arg(ocad_symbol->number).arg(ocad_symbol->lwidth).arg(ocad_symbol->rwidth).arg(border_width));
			}
			double_line->border_width = convertSize(border_width);
			double_line->border_shift = double_line->border_width / 2;
			
			// And finally, the border may be dashed
			if (ocad_symbol->dgap > 0 && ocad_symbol->dmode > 1)
			{
				double_line->dashed_border = true;
				double_line->border_dash_length = convertSize(ocad_symbol->dlen);
				double_line->border_break_length = convertSize(ocad_symbol->dgap);
				
				if (ocad_symbol->dmode == 2)
					addWarning(QObject::tr("In line symbol %1, ignoring that only the left border line should be dashed").arg(ocad_symbol->number));
			}
		}
	}
    
    // Create point symbols along line; middle ("normal") dash, corners, start, and end.
    LineSymbol* symbol_line = main_line ? main_line : double_line;	// Find the line to attach the symbols to
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
        symbolptr += ocad_symbol->scnpts; 
    }
    if( ocad_symbol->sbnpts > 0 )
    {
		symbol_line->start_symbol = importPattern( ocad_symbol->sbnpts, symbolptr);
        symbolptr += ocad_symbol->sbnpts;
    }
    if( ocad_symbol->senpts > 0 )
    {
		symbol_line->end_symbol = importPattern( ocad_symbol->senpts, symbolptr);
    }
    // FIXME: not really sure how this translates... need test cases
    symbol_line->minimum_mid_symbol_count = 0; //1 + ocad_symbol->smin;
	symbol_line->minimum_mid_symbol_count_when_closed = 0; //1 + ocad_symbol->smin;
	symbol_line->show_at_least_one_symbol = false; // NOTE: this works in a different way than OCAD's 'at least X symbols' setting

    // TODO: taper fields (tmode and tlast)

    if (ocad_symbol->fwidth > 0)
    {
        addWarning(QObject::tr("In symbol %1, ignoring framing line.").arg(ocad_symbol->number));
    }
    
    if (main_line == NULL)
		return double_line;
	else if (double_line == NULL)
		return main_line;
	else
	{
		CombinedSymbol* full_line = new CombinedSymbol();
		fillCommonSymbolFields(full_line, (OCADSymbol *)ocad_symbol);
		full_line->setNumParts(2);
		full_line->setPart(0, main_line);
		full_line->setPart(1, double_line);
		
		// Don't let parts be affected by possible settings for the combined symbol
		main_line->setHidden(false);
		main_line->setProtected(false);
		double_line->setHidden(false);
		double_line->setProtected(false);
		
		return full_line;
	}
}

Symbol *OCAD8FileImport::importAreaSymbol(const OCADAreaSymbol *ocad_symbol)
{
    AreaSymbol *symbol = new AreaSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    // Basic area symbol fields: minimum_area, color
    symbol->minimum_area = 0;
    symbol->color = ocad_symbol->fill ? convertColor(ocad_symbol->color) : NULL;
    symbol->patterns.clear();
    AreaSymbol::FillPattern *pat = NULL;

    // Hatching
    if (ocad_symbol->hmode > 0)
    {
        int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
        pat->type = AreaSymbol::FillPattern::LinePattern;
        pat->angle = convertRotation(ocad_symbol->hangle1);
        pat->rotatable = true;
        pat->line_spacing = convertSize(ocad_symbol->hdist + ocad_symbol->hwidth);
        pat->line_offset = 0;
        pat->line_color = convertColor(ocad_symbol->hcolor);
        pat->line_width = convertSize(ocad_symbol->hwidth);
        if (ocad_symbol->hmode == 2)
        {
            // Second hatch, same as the first, just a different angle
            int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
            pat->type = AreaSymbol::FillPattern::LinePattern;
            pat->angle = convertRotation(ocad_symbol->hangle2);
            pat->rotatable = true;
            pat->line_spacing = convertSize(ocad_symbol->hdist);
            pat->line_offset = 0;
            pat->line_color = convertColor(ocad_symbol->hcolor);
            pat->line_width = convertSize(ocad_symbol->hwidth);
        }
    }

    if (ocad_symbol->pmode > 0)
    {
        // OCAD 8 has a "staggered" pattern mode, where successive rows are shifted width/2 relative
        // to each other. We need to simulate this in Mapper with two overlapping patterns, each with
        // twice the height. The second is then offset by width/2, height/2.
        qint64 spacing = convertSize(ocad_symbol->pheight);
        if (ocad_symbol->pmode == 2) spacing *= 2;
        int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
        pat->type = AreaSymbol::FillPattern::PointPattern;
        pat->angle = convertRotation(ocad_symbol->pangle);
        pat->rotatable = true;
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
            pat->rotatable = true;
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
		addWarning(QObject::tr("During import of text symbol %1: ignoring justified alignment").arg(ocad_symbol->number));
	}
	text_halign_map[symbol] = halign;

    if (ocad_symbol->bold != 400 && ocad_symbol->bold != 700)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom weight (%2)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->bold));
    }
    if (ocad_symbol->cspace != 0)
	{
		addWarning(QObject::tr("During import of text symbol %1: custom character spacing is set, its implementation does not match OCAD's behavior yet")
		.arg(ocad_symbol->number));
	}
    if (ocad_symbol->wspace != 100)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom word spacing (%2%)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->wspace));
    }
    if (ocad_symbol->indent1 != 0 || ocad_symbol->indent2 != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom indents (%2/%3)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->indent1).arg(ocad_symbol->indent2));
    }
    if (ocad_symbol->fmode != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring text framing (mode %2)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->fmode));
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
		rect.text->font_family = "Arial";
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
				element_symbol->outer_color = NULL;
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
				element_symbol->inner_color = NULL;
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
    symbol->name = convertPascalString(ocad_symbol->name);
    symbol->number[0] = ocad_symbol->number / 10;
    symbol->number[1] = ocad_symbol->number % 10;
    symbol->number[2] = -1;
    symbol->description = symbol->name;
    symbol->is_helper_symbol = false; // no such thing in OCAD
    if (ocad_symbol->status & 1)
		symbol->setProtected(true);
	if (ocad_symbol->status & 2)
		symbol->setHidden(true);
}

Object *OCAD8FileImport::importObject(const OCADObject* ocad_object, MapLayer* layer)
{
    if (!symbol_index.contains(ocad_object->symbol))
    {
		if (!rectangle_info.contains(ocad_object->symbol))
		{
			addWarning(QObject::tr("Unable to load object"));
			return NULL;
		}
		else
		{
			if (!importRectangleObject(ocad_object, layer, rectangle_info[ocad_object->symbol]))
				addWarning(QObject::tr("Unable to import rectangle object"));
			return NULL;
		}
    }

    Symbol *symbol = symbol_index[ocad_object->symbol];
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
        fillPathCoords(p, false, 1, (OCADPoint *)ocad_object->pts);
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
        size_t text_len = sizeof(OCADPoint) * ocad_object->ntext;
        // (type |= 0x100) is used in OCAD 8 to indicate wide text (although this is not documented)
        if ((ocad_object->type) & 0x100) t->setText(convertWideCString(text_ptr, text_len));
        else t->setText(convertCString(text_ptr, text_len));

        // Text objects need special path translation
        if (!fillTextPathCoords(t, reinterpret_cast<TextSymbol*>(symbol), ocad_object->npts, (OCADPoint *)ocad_object->pts))
        {
            addWarning(QObject::tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
                           .arg(ocad_object->npts).arg(t->getText()));
            delete t;
            return NULL;
        }
        t->setMap(map);
        return t;
    }
    else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined) {
		PathObject *p = new PathObject(symbol);

		// Normal path
		fillPathCoords(p, symbol->getType() == Symbol::Area, ocad_object->npts, (OCADPoint *)ocad_object->pts);
		p->recalculateParts();
		p->setMap(map);
		return p;
    }

    return NULL;
}

bool OCAD8FileImport::importRectangleObject(const OCADObject* ocad_object, MapLayer* layer, const OCAD8FileImport::RectangleInfo& rect)
{
	if (ocad_object->npts != 4)
		return false;
	
	// Convert corner points
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
	
	MapCoordF top_left_f = MapCoordF(top_left);
	MapCoordF top_right_f = MapCoordF(top_right);
	MapCoordF bottom_left_f = MapCoordF(bottom_left);
	MapCoordF bottom_right_f = MapCoordF(bottom_right);
	MapCoordF right = MapCoordF(top_right.xd() - top_left.xd(), top_right.yd() - top_left.yd());
	double angle = right.getAngle();
	MapCoordF down = MapCoordF(bottom_left.xd() - top_left.xd(), bottom_left.yd() - top_left.yd());
	right.normalize();
	down.normalize();
	
	// Create border line
	MapCoordVector coords;
	if (rect.corner_radius == 0)
	{
		coords.push_back(top_left);
		coords.push_back(top_right);
		coords.push_back(bottom_right);
		coords.push_back(bottom_left);
	}
	else
	{
		double handle_radius = (1 - BEZIER_KAPPA) * rect.corner_radius;
		coords.push_back((top_right_f - right * rect.corner_radius).toCurveStartMapCoord());
		coords.push_back((top_right_f - right * handle_radius).toMapCoord());
		coords.push_back((top_right_f + down * handle_radius).toMapCoord());
		coords.push_back((top_right_f + down * rect.corner_radius).toMapCoord());
		coords.push_back((bottom_right_f - down * rect.corner_radius).toCurveStartMapCoord());
		coords.push_back((bottom_right_f - down * handle_radius).toMapCoord());
		coords.push_back((bottom_right_f - right * handle_radius).toMapCoord());
		coords.push_back((bottom_right_f - right * rect.corner_radius).toMapCoord());
		coords.push_back((bottom_left_f + right * rect.corner_radius).toCurveStartMapCoord());
		coords.push_back((bottom_left_f + right * handle_radius).toMapCoord());
		coords.push_back((bottom_left_f - down * handle_radius).toMapCoord());
		coords.push_back((bottom_left_f - down * rect.corner_radius).toMapCoord());
		coords.push_back((top_left_f + down * rect.corner_radius).toCurveStartMapCoord());
		coords.push_back((top_left_f + down * handle_radius).toMapCoord());
		coords.push_back((top_left_f + right * handle_radius).toMapCoord());
		coords.push_back((top_left_f + right * rect.corner_radius).toMapCoord());
	}
	PathObject *border_path = new PathObject(rect.border_line, coords, map);
	border_path->getPart(0).setClosed(true);
	layer->objects.push_back(border_path);
	
	if (rect.has_grid && rect.cell_width > 0 && rect.cell_height > 0)
	{
		// Calculate grid sizes
		double width = top_left.lengthTo(top_right);
		double height = top_left.lengthTo(bottom_left);
		int num_cells_x = qMax(1, qRound(width / rect.cell_width));
		int num_cells_y = qMax(1, qRound(height / rect.cell_height));
		
		float cell_width = width / num_cells_x;
		float cell_height = height / num_cells_y;
		
		// Create grid lines
		coords.resize(2);
		for (int x = 1; x < num_cells_x; ++x)
		{
			coords[0] = (top_left_f + x * cell_width * right).toMapCoord();
			coords[1] = (bottom_left_f + x * cell_width * right).toMapCoord();
			
			PathObject *path = new PathObject(rect.inner_line, coords, map);
			layer->objects.push_back(path);
		}
		for (int y = 1; y < num_cells_y; ++y)
		{
			coords[0] = (top_left_f + y * cell_height * down).toMapCoord();
			coords[1] = (top_right_f + y * cell_height * down).toMapCoord();
			
			PathObject *path = new PathObject(rect.inner_line, coords, map);
			layer->objects.push_back(path);
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
					layer->objects.push_back(object);
					
					//pts[0].Y -= rectinfo.gridText.FontAscent - rectinfo.gridText.FontEmHeight;
				}
			}
		}
	}
	
	return true;
}

Template *OCAD8FileImport::importTemplate(OCADTemplateEntry *entry)
{
    OCADTemplate *ocad_templ = ocad_template(file, entry);
    if (entry->type == 8)
    {
        OCADBackground background;
        if (ocad_to_background(&background, ocad_templ) == 0)
        {
            return importRasterTemplate(background);
        }
        else
        {
            addWarning(QObject::tr("Unable to import template: %1").arg(ocad_templ->str));
        }
    }
    else
    {
        addWarning(QObject::tr("Ignoring template of type: %1 (%2)").arg(entry->type).arg(ocad_templ->str));
    }
    return NULL;
}

Template *OCAD8FileImport::importRasterTemplate(const OCADBackground &background)
{
    QString filename(background.filename); // FIXME: use platform char encoding?
    if (isRasterImageFile(filename))
    {
        TemplateImage *templ = new TemplateImage(filename, map);
        MapCoord c;
        convertPoint(c, background.trnx, background.trny);
        templ->setTemplateX(c.rawX());
        templ->setTemplateY(c.rawY());
        // This seems to be measured in degrees. Plus there's wacky values like -359.7.
        templ->setTemplateRotation(M_PI / 180 * background.angle);
        templ->setTemplateScaleX(background.sclx);
        templ->setTemplateScaleY(background.scly);
        // FIXME: import template view parameters: background.dimming and background.transparent
        return templ;
    }
    else
    {
        addWarning(QObject::tr("Unable to import template: background \"%1\" doesn't seem to be a raster image").arg(filename));
    }
    return NULL;
}

bool OCAD8FileImport::isRasterImageFile(const QString &filename) const
{
    // FIXME: see if we can drive this from the format list support by QImageReader instead.
    // That way it automatically adapts to the system on which it's run.
    if (filename.endsWith(".jpg", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".jpeg", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".png", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".gif", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".tif", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".tiff", Qt::CaseInsensitive)) return true;
    if (filename.endsWith(".bmp", Qt::CaseInsensitive)) return true;
    return false;
}

/** Translates the OCAD path given in the last two arguments into an Object.
 */
void OCAD8FileImport::fillPathCoords(Object *object, bool is_area, s16 npts, OCADPoint *pts)
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
        if (buf[2] & PX_CTL1 && i > 0) object->coords[i-1].setCurveStart(true);
		if ((buf[2] & (PY_DASH << 8)) || (buf[2] & (PY_CORNER << 8))) coord.setDashPoint(true);
        if (buf[2] & (PY_HOLE << 8))
		{
			if (is_area)
				object->coords[i-1].setHolePoint(true);
			else
				coord.setHolePoint(true);
		}
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
				object->coords[i].setClosePoint(true);
			
			start = i + 1;
		}
	}
}

/** Translates an OCAD text object path into a Mapper text object specifier, if possible.
 *  If successful, sets either 1 or 2 coordinates in the text object and returns true.
 *  If the OCAD path was not importable, leaves the TextObject alone and returns false.
 */
bool OCAD8FileImport::fillTextPathCoords(TextObject *object, TextSymbol *symbol, s16 npts, OCADPoint *pts)
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
		top_left = MapCoord(top_left.xd() + adjust_vector.getX(), top_left.yd() + adjust_vector.getY());
		bottom_left = MapCoord(bottom_left.xd() + adjust_vector.getX(), bottom_left.yd() + adjust_vector.getY());
		top_right = MapCoord(top_right.xd() + adjust_vector.getX(), top_right.yd() + adjust_vector.getY());
		
		object->setBox((bottom_left.rawX() + top_right.rawX()) / 2, (bottom_left.rawY() + top_right.rawY()) / 2,
					   top_left.lengthTo(top_right), top_left.lengthTo(bottom_left));

		object->setVerticalAlignment(TextObject::AlignTop);
	}
	else
	{
		// Single anchor text
		if (npts != 5)
			addWarning(QObject::tr("Trying to import a text object with unknown coordinate format"));
		
		s32 buf[3];
		ocad_point(buf, &(pts[0])); // anchor point
		
		MapCoord coord;
		convertPoint(coord, buf[0], buf[1]);
		object->setAnchorPosition(coord.rawX(), coord.rawY());
		
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
QString OCAD8FileImport::convertCString(const char *p, size_t n) {
    size_t i = 0;
    for (; i < n; i++) {
        if (p[i] == 0) break;
    }
    if (n >= 2 && p[0] == '\r' && p[1] == '\n')
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
QString OCAD8FileImport::convertWideCString(const char *p, size_t n) {
    const u16 *q = (const u16 *)p;
    size_t i = 0;
    for (; i < n; i++) {
        if (q[i] == 0) break;
    }
    if (n >= 4 && p[0] == '\r' && p[2] == '\n')
	{
		// Remove "\r\n" at the beginning of texts, somehow OCAD seems to add this sometimes but ignores it
		p += 4;
		i -= 2;
	}
    return encoding_2byte->toUnicode(p, i * 2);
}

float OCAD8FileImport::convertRotation(int angle) {
    // OCAD uses tenths of a degree, counterclockwise
    // FIXME: oo-mapper uses a real number of degrees, counterclockwise
    // BUG: if sin(rotation) is < 0 for a hatched area pattern, the pattern's createRenderables() will go into an infinite loop.
    // So until that's fixed, we keep a between 0 and PI
    float a = (M_PI / 180) *  (0.1f * angle);
    while (a < 0) a += 2 * M_PI;
    //if (a < 0 || a > M_PI) qDebug() << "Found angle" << a;
    return a;
}

void OCAD8FileImport::convertPoint(MapCoord &coord, int ocad_x, int ocad_y)
{
    // OCAD uses hundredths of a millimeter.
    // oo-mapper uses 1/1000 mm
    coord.setRawX(offset_x + (qint64)ocad_x * 10);
    // Y-axis is flipped.
    coord.setRawY(offset_y + (qint64)ocad_y * (-10));
}

qint64 OCAD8FileImport::convertSize(int ocad_size) {
    // OCAD uses hundredths of a millimeter.
    // oo-mapper uses 1/1000 mm
    return ((qint64)ocad_size) * 10;
}

MapColor *OCAD8FileImport::convertColor(int color) {
	if (!color_index.contains(color))
	{
		addWarning(QObject::tr("Color id not found: %1, ignoring this color").arg(color));
		return NULL;
	}
	else
		return color_index[color];
}
