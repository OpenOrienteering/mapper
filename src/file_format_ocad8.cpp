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



// Mapper assumes approx. 100 dpi, but OCAD uses a different value.
const float OCAD8FileImport::ocad_pt_in_mm = 25.4f / 83;

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

void OCAD8FileImport::doImport() throw (FormatException)
{
    qint64 start = QDateTime::currentMSecsSinceEpoch();
    int symbol_count = 0, object_count = 0;

	QByteArray filename = path.toLocal8Bit().constData();
    int err = ocad_file_open(&file, filename);
    //qDebug() << "open ocad file" << err;
    if (err != 0) throw FormatException(QObject::tr("Could not open file: libocad returned %1").arg(err));

    qDebug() << "file version is" << file->header->major << ", type is"
             << ((file->header->ftype == 2) ? "normal" : "other");
    qDebug() << "map scale is" << file->setup->scale;

    map->scale_denominator = file->setup->scale;


    // TODO: GPS projection parameters

    // TODO: print parameters

    // Load colors
    int num_colors = ocad_color_count(file);

    for (int i = 0; i < num_colors; i++)
    {
        OCADColor *ocad_color = ocad_color_at(file, i);
        //qDebug() << clr->cyan << clr->magenta << clr->yellow << clr->black << clr->number << str1(clr->name);

        QString name = convertPascalString(ocad_color->name);
        if (name.isEmpty()) continue;

        MapColor* color = new MapColor();
        color->priority = i;

        // OCAD stores CMYK values as integers from 0-200.
        color->c = 0.005f * ocad_color->cyan;
        color->m = 0.005f * ocad_color->magenta;
        color->y = 0.005f * ocad_color->yellow;
        color->k = 0.005f * ocad_color->black;
        color->opacity = 1.0f;
        color->name = name;
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
                /*
                else if (ocad_symbol->type == OCAD_RECT_SYMBOL)
                {
                    symbol = importRectSymbol((OCADRectSymbol *)ocad_symbol);
                }
                */

                if (symbol)
                {
                    map->symbols.push_back(symbol);
                    symbol_index[ocad_symbol->number] = symbol;
                    symbol_count++;
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
                Object *object = importObject(ocad_obj);
                if (object != NULL) {
                    layer->objects.push_back(object);
                    object_count++;
                }
            }
        }
    }
    map->layers.resize(1);
    map->layers[0] = layer;
    map->current_layer_index = 0;
    map->current_layer = layer; // FIXME: should current_layer and current_layer_index be separate?

    // Load templates
    map->templates.clear();
    map->first_front_template = 0; // FIXME? Guessing this is the right value if there are no templates.
    for (OCADTemplateIndex *idx = ocad_template_index_first(file); idx != NULL; idx = ocad_template_index_next(file, idx))
    {
        for (int i = 0; i < 256; i++)
        {
            OCADTemplateEntry *entry = ocad_template_entry_at(file, idx, i);
            if (entry->size > 0)
            {
                OCADTemplate *templ = ocad_template(file, entry);
                qDebug() << "Template: "<<templ->str;
                OCADBackground background;
                if (ocad_to_background(&background, templ) == 0)
                {
                    qDebug() << "Found background" << background.filename;
                }
                else
                {
                    addWarning(QObject::tr("Unable to import template: %1").arg(templ->str));

                }
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


    ocad_file_close(file);

    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - start;
    qDebug() << "OCAD map imported:"<<symbol_count<<"symbols and"<<object_count<<"objects in"<<elapsed<<"milliseconds";
}

void OCAD8FileImport::setStringEncodings(const char *narrow, const char *wide) {
    encoding_1byte = QTextCodec::codecForName(narrow);
    encoding_2byte = QTextCodec::codecForName(wide);
}

Symbol *OCAD8FileImport::importPointSymbol(const OCADPointSymbol *ocad_symbol)
{
    PointSymbol *symbol = importPattern(ocad_symbol->ngrp, (OCADPoint *)ocad_symbol->pts);
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    return symbol;
}

Symbol *OCAD8FileImport::importLineSymbol(const OCADLineSymbol *ocad_symbol)
{
    LineSymbol *symbol = new LineSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    symbol->line_width = convertSize(ocad_symbol->width);
    symbol->color = color_index[ocad_symbol->color];
    symbol->minimum_length = 0; // FIXME
    // FIXME: are the cap mappings correct?
    // FIXME: where is "pointed ends" in OCAD?
    if (ocad_symbol->ends == 0) symbol->cap_style = LineSymbol::FlatCap;
    else if (ocad_symbol->ends == 1) symbol->cap_style = LineSymbol::RoundCap;
    else if (ocad_symbol->ends == 2) symbol->cap_style = LineSymbol::SquareCap;
    // FIXME: are the join mappings correct?

    // FIXME: Do everything else
    if( ocad_symbol->gap > 0 || ocad_symbol->gap2 > 0 )
    {
        //dashed
        symbol->dashed = true;
        symbol->dash_length = convertSize(ocad_symbol->len);
    }


    return symbol;
}

Symbol *OCAD8FileImport::importAreaSymbol(const OCADAreaSymbol *ocad_symbol)
{
    AreaSymbol *symbol = new AreaSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    // Basic area symbol fields: minimum_area, color
    symbol->minimum_area = 0;
    symbol->color = ocad_symbol->fill ? color_index[ocad_symbol->color] : NULL;
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
        pat->line_color = color_index[ocad_symbol->hcolor];
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
            pat->line_color = color_index[ocad_symbol->hcolor];
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
    symbol->color = color_index[ocad_symbol->color];
    symbol->ascent_size = (int)round((0.1 * ocad_symbol->dpts) * ocad_pt_in_mm * 1000);
    symbol->bold = (ocad_symbol->bold >= 700) ? true : false;
    symbol->italic = (ocad_symbol->italic) ? true : false;
    symbol->underline = (ocad_symbol->under) ? true : false;
    symbol->line_spacing = 0.01f * ocad_symbol->lspace;

    //qDebug() << "Convert"<<ocad_symbol->dpts<<"decipoints to"<<symbol->ascent_size<<"um";

    if (ocad_symbol->bold != 400 && ocad_symbol->bold != 700)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom weight (%2)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->bold));
    }
    if (ocad_symbol->under)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom underline color, width, and positioning")
                       .arg(ocad_symbol->number));
    }
    if (ocad_symbol->cspace != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom character spacing (%2%)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->cspace));
    }
    if (ocad_symbol->wspace != 100)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom word spacing (%2%)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->wspace));
    }
    if (ocad_symbol->pspace != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom paragraph spacing (%2%)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->pspace));
    }
    if (ocad_symbol->indent1 != 0 || ocad_symbol->indent2 != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom indents (%2/%3)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->indent1).arg(ocad_symbol->indent2));
    }
    if (ocad_symbol->ntabs > 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring custom tabs")
                       .arg(ocad_symbol->number));
    }
    if (ocad_symbol->fmode != 0)
    {
        addWarning(QObject::tr("During import of text symbol %1: ignoring text framing (mode %2)")
                       .arg(ocad_symbol->number).arg(ocad_symbol->fmode));
    }

    symbol->updateQFont();

    return symbol;
}

PointSymbol *OCAD8FileImport::importPattern(s16 npts, OCADPoint *pts)
{
    PointSymbol *symbol = new PointSymbol();
    symbol->rotatable = true; // FIXME: by default for now, where is the "Align to north" checkbox in the OCAD file?
    OCADPoint *p = pts, *end = pts + npts;
    while (p < end) {
        OCADSymbolElement *elt = (OCADSymbolElement *)p;
        int element_index = symbol->getNumElements();
        if (elt->type == OCAD_DOT_ELEMENT)
        {
            PointSymbol* element_symbol = new PointSymbol();
            element_symbol->inner_color = color_index[elt->color];
            element_symbol->inner_radius = (int)convertSize(elt->diameter) / 2;
            element_symbol->outer_color = NULL;
            element_symbol->outer_width = 0;
            element_symbol->rotatable = false;
            PointObject* element_object = new PointObject(element_symbol);
            element_object->coords.resize(1);
            symbol->addElement(element_index, element_object, element_symbol);
        }
        else if (elt->type == OCAD_CIRCLE_ELEMENT)
        {
            PointSymbol* element_symbol = new PointSymbol();
            element_symbol->inner_color = NULL;
            element_symbol->inner_radius = (int)convertSize(elt->diameter) / 2;
            element_symbol->outer_color = color_index[elt->color];
            element_symbol->outer_width = (int)convertSize(elt->width);
            element_symbol->rotatable = false;
            PointObject* element_object = new PointObject(element_symbol);
            element_object->coords.resize(1);
            symbol->addElement(element_index, element_object, element_symbol);
        }
        else if (elt->type == OCAD_LINE_ELEMENT)
        {
            LineSymbol* element_symbol = new LineSymbol();
            element_symbol->line_width = convertSize(elt->width);
            element_symbol->color = color_index[elt->color];
            PathObject* element_object = new PathObject(element_symbol);
            fillPathCoords(element_object, elt->npts, elt->pts);
            symbol->addElement(element_index, element_object, element_symbol);
        }
        else if (elt->type == OCAD_AREA_ELEMENT)
        {
            AreaSymbol* element_symbol = new AreaSymbol();
            element_symbol->color = color_index[elt->color];
            PathObject* element_object = new PathObject(element_symbol);
            fillPathCoords(element_object, elt->npts, elt->pts);
            symbol->addElement(element_index, element_object, element_symbol);
        }
        p += (2 + elt->npts);
    }
    return symbol;
}


void OCAD8FileImport::fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol)
{
    // common fields are name, number, description, helper_symbol
    symbol->name = convertPascalString(ocad_symbol->name);
    symbol->number[0] = ocad_symbol->number / 10;
    symbol->number[1] = ocad_symbol->number % 10;
    symbol->number[2] = -1;
    symbol->description = symbol->name;
    symbol->is_helper_symbol = false; // no such thing in OCAD
    //symbol->map = map;
}

Object *OCAD8FileImport::importObject(const OCADObject *ocad_object)
{
    if (!symbol_index.contains(ocad_object->symbol))
    {
        addWarning(QObject::tr("Unable to load object"));
        return NULL;
    }

    Symbol *symbol = symbol_index[ocad_object->symbol];
    if (symbol->getType() == Symbol::Point)
    {
        PointObject *p = new PointObject();
        p->symbol = symbol;

        // extra properties: rotation
        p->setRotation(convertRotation(ocad_object->angle));

        // only 1 coordinate is allowed, enforce it even if the OCAD object claims more.
        fillPathCoords(p, 1, (OCADPoint *)ocad_object->pts);
        p->map = map;
        p->output_dirty = true;
        return p;
    }
    else if (symbol->getType() == Symbol::Text)
    {
        TextObject *t = new TextObject();
        t->symbol = symbol;

        // extra properties: rotation, horizontalAlignment, verticalAlignment, text
        t->setRotation(convertRotation(ocad_object->angle));

        const char *text_ptr = (const char *)(ocad_object->pts + ocad_object->npts);
        size_t text_len = sizeof(OCADPoint) * ocad_object->ntext;
        // (type |= 0x100) is used in OCAD 8 to indicate wide text (although this is not documented)
        if ((ocad_object->type) & 0x100) t->setText(convertWideCString(text_ptr, text_len));
        else t->setText(convertCString(text_ptr, text_len));

        // Text objects need special path translation
        if (!fillTextPathCoords(t, ocad_object->npts, (OCADPoint *)ocad_object->pts))
        {
            addWarning(QObject::tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
                           .arg(ocad_object->npts).arg(t->getText()));
            delete t;
            return NULL;
        }
        t->path_closed = false;
        t->map = map;
        t->output_dirty = true;
        return t;
    }
    else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area) {
        PathObject *p = new PathObject();
        p->symbol = symbol;

        // Normal path
        fillPathCoords(p, ocad_object->npts, (OCADPoint *)ocad_object->pts);
        p->path_closed = false;
        p->map = map;
        p->output_dirty = true;
        return p;
    }

    return NULL;
}

/** Translates the OCAD path given in the last two arguments into an Object.
 */
void OCAD8FileImport::fillPathCoords(Object *object, s16 npts, OCADPoint *pts)
{
    object->coords.resize(npts);
    s32 buf[3];
    for (int i = 0; i < npts; i++)
    {
        ocad_point(buf, &(pts[i]));
        MapCoord &coord = object->coords[i];
        convertPoint(coord, buf[0], buf[1]);
        // We can support CurveStart, HolePoint, DashPoint.
        // CurveStart needs to be applied to the main point though, not the control point
        if (buf[2] & PX_CTL1 && i > 0) object->coords[i-1].setCurveStart(true);
        if (buf[2] & (PY_DASH << 8)) coord.setDashPoint(true);
        if (buf[2] & (PY_HOLE << 8)) coord.setHolePoint(true);
    }
}

/** Translates an OCAD text object path into a Mapper text object specifier, if possible.
 *  If successful, sets either 1 or 2 coordinates in the text object and returns true.
 *  If the OCAD path was not importable, leaves the TextObject alone and returns false.
 */
bool OCAD8FileImport::fillTextPathCoords(TextObject *object, s16 npts, OCADPoint *pts)
{
    // text objects either have 1 point (free anchor) or 2 (midpoint/size)
    // OCAD appears to always have 5 points (anchor on left edge, then 4 corner coordinates going clockwise from anchor).
    if (npts != 5) return false;
    s32 buf[3];
    ocad_point(buf, &(pts[0])); // anchor point
    s32 x0 = buf[0], y0 = buf[1];
    /*ocad_point(buf, &(pts[1])); // bottom left point
    s32 x1 = buf[0], y1 = buf[1]; */
    ocad_point(buf, &(pts[2])); // bottom right point
    s32 x2 = buf[0], y2 = buf[1];
    /*ocad_point(buf, &(pts[3])); // top right point
    s32 x3 = buf[0], y3 = buf[1];
    ocad_point(buf, &(pts[4])); // top left point
    s32 x4 = buf[0], y4 = buf[1];*/
    //qDebug() << "text path"<<x0<<y0<<x1<<y1<<x2<<y2<<x3<<y3<<x4<<y4;
    if (x2 <= x0 || y0 <= y2)
    {
        return false;
    }

    object->setHorizontalAlignment(TextObject::AlignLeft);
    object->setVerticalAlignment(TextObject::AlignBaseline);
    object->coords.resize(2);
    convertPoint(object->coords[0], (x0 + x2) / 2, (y0 + y2) / 2);
    object->coords[1].setRawX(convertSize(x2 - x0));
    object->coords[1].setRawY(convertSize(y0 - y2));

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

