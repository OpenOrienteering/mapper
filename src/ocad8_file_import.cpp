#include <QDebug>

#include "ocad8_file_import.h"
#include "map_color.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_area.h"

OCAD8FileImport::OCAD8FileImport(Map *map, MapView *view) : map(map), view(view), file(NULL)
{
    ocad_init();
    encoding_1byte = QTextCodec::codecForName("Windows-1252");
    encoding_2byte = QTextCodec::codecForName("UTF-16LE");
}

OCAD8FileImport::~OCAD8FileImport()
{
}

void OCAD8FileImport::loadFrom(const char *filename) throw (std::exception)
{
    int err = ocad_file_open(&file, filename);
    qDebug() << "open ocad file" << err;
    if (err != 0) throw std::exception();

    qDebug() << "file version is " << file->header->major << ", type is "
             << ((file->header->ftype == 2) ? "normal" : "other");
    qDebug() << "map scale is" << file->setup->scale;

    map->scale_denominator = file->setup->scale;


    // TODO: GPS projection parameters

    // TODO: print parameters

    // Load colors
    int num_colors = ocad_color_count(file);

    map->color_set->colors.resize(num_colors);
    for (int i = 0; i < num_colors; i++)
    {
        OCADColor *ocad_color = ocad_color_at(file, i);
        //qDebug() << clr->cyan << clr->magenta << clr->yellow << clr->black << clr->number << str1(clr->name);

        MapColor* color = new MapColor();
        color->priority = i;

        // OCAD stores CMYK values as integers from 0-200.
        color->c = 0.005f * ocad_color->cyan;
        color->m = 0.005f * ocad_color->magenta;
        color->y = 0.005f * ocad_color->yellow;
        color->k = 0.005f * ocad_color->black;
        color->opacity = 1.0f;
        color->name = str1(ocad_color->name);
        color->updateFromCMYK();

        map->color_set->colors[i] = color;
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
                /*
                else if (ocad_symbol->type == OCAD_TEXT_SYMBOL)
                {
                    symbol = importTextSymbol((OCADTextSymbol *)ocad_symbol);
                }
                else if (ocad_symbol->type == OCAD_RECT_SYMBOL)
                {
                    symbol = importRectSymbol((OCADRectSymbol *)ocad_symbol);
                }
                */

                if (symbol)
                {
                    map->symbols.push_back(symbol);
                    symbol_index[ocad_symbol->number] = symbol;
                }
                else
                {
                    warn.push_back(QString(QObject::tr("Unable to import symbol \"%3\" (%1.%2)"))
                                   .arg(ocad_symbol->number / 10).arg(ocad_symbol->number % 10).arg(str1(ocad_symbol->name)));
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
                if (object != NULL) layer->objects.push_back(object);
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
                warn.push_back(QObject::tr("Skipping template"));
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
}

void OCAD8FileImport::setStringEncodings(const char *narrow, const char *wide) {
    encoding_1byte = QTextCodec::codecForName(narrow);
    encoding_2byte = QTextCodec::codecForName(wide);
}

Symbol *OCAD8FileImport::importPointSymbol(const OCADPointSymbol *ocad_symbol)
{
    return NULL;
}

Symbol *OCAD8FileImport::importLineSymbol(const OCADLineSymbol *ocad_symbol)
{
    LineSymbol *symbol = new LineSymbol();
    fillCommonSymbolFields(symbol, (OCADSymbol *)ocad_symbol);

    symbol->line_width = convertPosition(ocad_symbol->width);
    symbol->color = color_index[ocad_symbol->color];
    symbol->minimum_length = 0; // FIXME
    // FIXME: are the cap mappings correct?
    // FIXME: where is "pointed ends" in OCAD?
    if (ocad_symbol->ends == 0) symbol->cap_style = LineSymbol::FlatCap;
    else if (ocad_symbol->ends == 1) symbol->cap_style = LineSymbol::RoundCap;
    else if (ocad_symbol->ends == 2) symbol->cap_style = LineSymbol::SquareCap;
    // FIXME: are the join mappings correct?

    // FIXME: Do everything else

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
        pat->line_spacing = convertPosition(ocad_symbol->hdist);
        pat->line_offset = 0;
        pat->line_color = color_index[ocad_symbol->hcolor];
        pat->line_width = convertPosition(ocad_symbol->hwidth);
        if (ocad_symbol->hmode == 2)
        {
            // Second hatch, same as the first, just a different angle
            int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
            pat->type = AreaSymbol::FillPattern::LinePattern;
            pat->angle = convertRotation(ocad_symbol->hangle2);
            pat->rotatable = true;
            pat->line_spacing = convertPosition(ocad_symbol->hdist);
            pat->line_offset = 0;
            pat->line_color = color_index[ocad_symbol->hcolor];
            pat->line_width = convertPosition(ocad_symbol->hwidth);
        }
    }

    // Pattern
    /*
    if (ocad_symbol->pmode > 0)
    {
        int n = symbol->patterns.size(); symbol->patterns.resize(n + 1); pat = &(symbol->patterns[n]);
        pat->type = AreaSymbol::FillPattern::PointPattern;
        pat->angle = convertRotation(ocad_symbol->pangle);
        pat->rotatable = true;
        pat->line_spacing = convertPosition(ocad_symbol->pheight);
        pat->line_offset = 0;
        pat->point_distance = convertPosition(ocad_symbol->pwidth);
        pat->offset_along_line = (ocad_symbol->pmode == 2) ? 0.5 * pat->point_distance : 0; // staggered or aligned
        // FIXME: somebody needs to own this symbol and be responsible for deleting it
        // Right now it looks like a potential memory leak
        pat->point = importPointSymbolFromElements(ocad_symbol->npts, ocad_symbol->pts);
    }
    */

    return symbol;
}

PointSymbol *OCAD8FileImport::importPointSymbolFromElements(s16 npts, OCADPoint* pts)
{
    //ocad_symbol_element_iterate(npts, pts, callback, this);
    return NULL;
}


void OCAD8FileImport::fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol)
{
    // common fields are name, number, description, helper_symbol
    symbol->name = str1(ocad_symbol->name);
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
        warn.push_back(QObject::tr("Unable to load object"));
        return NULL;
    }

    Symbol *symbol = symbol_index[ocad_object->symbol];
    Object *object = NULL;
    if (symbol->getType() == Symbol::Point)
    {
        PointObject *p = new PointObject();
        // extra properties: rotation
        p->setRotation(convertRotation(ocad_object->angle));
        object = p;
    }
    else if (symbol->getType() == Symbol::Text)
    {
        TextObject *t = new TextObject();
        // extra properties: rotation, horizontalAlignment, verticalAlignment, text
        t->setRotation(convertRotation(ocad_object->angle));
        t->setHorizontalAlignment(TextObject::AlignLeft); // FIXME - correct?
        t->setVerticalAlignment(TextObject::AlignBaseline); // FIXME - correct?
        // (res1 == 1) is used in OCAD 8 to indicate wide text (although this is not documented)
        const char *text_ptr = (const char *)(ocad_object->pts + ocad_object->npts);
        t->setText((ocad_object->res1 != 0) ? str2(text_ptr) : str1(text_ptr));
        object = t;
    }
    else {
        PathObject *p = new PathObject();
        // nothing extra for these
        object = p;
    }

    if (object == NULL) return NULL;

    object->symbol = symbol;
    object->path_closed = false;
    object->coords.resize(ocad_object->npts);
    s32 buf[3];
    for (int i = 0; i < ocad_object->npts; i++) {
        ocad_point(buf, &(ocad_object->pts[i]));
        MapCoord &coord = object->coords[i];
        coord.setRawX(convertPosition(buf[0]));
        // Y-axis is flipped
        coord.setRawY(-convertPosition(buf[1]));
        // We can support CurveStart, HolePoint, DashPoint.
        // CurveStart needs to be applied to the main point though, not the control point
        if (buf[2] & PX_CTL1 && i > 0) object->coords[i-1].setCurveStart(true);
        if (buf[2] & (PY_DASH << 8)) coord.setDashPoint(true);
        //if (buf[2] & (PY_HOLE << 8)) coord.setHolePoint(true);
    }
    object->output_dirty = true;
    object->map = map;

    return object;

}

/** Converts a single-byte-per-character, length-payload string to a QString.
 *
 *  The byte sequence will be: LEN C0 C1 C2 C3...
 *
 *  Obviously this will only hold up to 255 characters. By default, we interpret the
 *  bytes using Windows-1252, as that's the most likely candidate for OCAD files in
 *  the wild.
 */
QString OCAD8FileImport::str1(const char *p) {
    int len = *((unsigned char *)p);
    return encoding_1byte->toUnicode((p + 1), len);
}

/** Converts a two-byte-per-character, length-payload string to a QString.
 *
 *  The byte sequence will be: LEN L0 H0 L1 H1 L2 H2...
 *
 *  Obviously this will only hold up to 255 characters. By default, we interpret the
 *  bytes using UTF-16LE, as that's the most likely candidate for OCAD files in the
 *  wild.
 */
QString OCAD8FileImport::str2(const char *p) {
    int len = *((unsigned char *)p);
    return encoding_1byte->toUnicode((p + 1), len);
}

float OCAD8FileImport::convertRotation(int angle) {
    // OCAD uses tenths of a degree, counterclockwise
    // FIXME: oo-mapper uses a real number of degrees, counterclockwise
    // BUG: if sin(rotation) is < 0 for a hatched area pattern, the pattern's createRenderables() will go into an infinite loop.
    // So until that's fixed, we keep a between 0 and PI
    float a = (M_PI / 180) *  (0.1f * angle);
    while (a < 0) a += 2 * M_PI;
    if (a < 0 || a > M_PI) qDebug() << "Found angle" << a;
    return a;
}

qint64 OCAD8FileImport::convertPosition(int position) {
    // OCAD uses tenths of a millimeter.
    // oo-mapper uses 1/1000 mm
    return ((qint64)position) * 100;
}

