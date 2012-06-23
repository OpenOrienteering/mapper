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

#ifndef OCAD8_FILE_IMPORT_H
#define OCAD8_FILE_IMPORT_H

#include <set>

#include <QTextCodec>

#include "../libocad/libocad.h"

#include "file_format.h"
#include "symbol_combined.h"
#include "template.h"

class Object;
class Symbol;
class TextObject;
class TextSymbol;

class OCAD8FileFormat : public Format
{
public:
    OCAD8FileFormat() : Format("OCAD78", QObject::tr("OCAD Versions 7, 8"), "ocd", true, false, true) {}

    bool understands(const unsigned char *buffer, size_t sz) const;
	Importer *createImporter(QIODevice* stream, const QString &path, Map *map, MapView *view) const throw (FormatException);
};


class OCAD8FileImport : public Importer
{
private:
	/// Information about an OCAD rectangle symbol
	struct RectangleInfo
	{
		LineSymbol* border_line;
		double corner_radius;
		bool has_grid;
		
		// Only valid if has_grid is true
		LineSymbol* inner_line;
		TextSymbol* text;
		bool number_from_bottom;
		double cell_width;
		double cell_height;
		int unnumbered_cells;
		QString unnumbered_text;
	};
	
public:
	OCAD8FileImport(QIODevice* stream, const QString &path, Map *map, MapView *view);
    ~OCAD8FileImport();

	void doImport(bool load_symbols_only) throw (FormatException);

    void setStringEncodings(const char *narrow, const char *wide = "UTF-16LE");

    static const float ocad_pt_in_mm;

protected:
    // Symbol import
    Symbol *importPointSymbol(const OCADPointSymbol *ocad_symbol);
    Symbol *importLineSymbol(const OCADLineSymbol *ocad_symbol);
    Symbol *importDoubleLineSymbol(const OCADLineSymbol *ocad_symbol);
    Symbol *importAreaSymbol(const OCADAreaSymbol *ocad_symbol);
    Symbol *importTextSymbol(const OCADTextSymbol *ocad_symbol);
    RectangleInfo *importRectSymbol(const OCADRectSymbol *ocad_symbol);

    // Object import
    Object *importObject(const OCADObject *ocad_object, MapLayer* layer);
	bool importRectangleObject(const OCADObject* ocad_object, MapLayer* layer, const RectangleInfo& rect);

    // String import
    void importString(OCADStringEntry *entry);
    Template *importRasterTemplate(const OCADBackground &background);

    // Some helper functions that are used in multiple places
    PointSymbol *importPattern(s16 npts, OCADPoint *pts);
    void fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol);
    void fillCombinedSymbol(CombinedSymbol *symbol, const std::vector<Symbol *> &symbols);
    void fillPathCoords(Object *object, bool is_area, s16 npts, OCADPoint *pts);
	bool fillTextPathCoords(TextObject *object, TextSymbol *symbol, s16 npts, OCADPoint *pts);
    bool isRasterImageFile(const QString &filename) const;
    bool isMainLineTrivial(const LineSymbol *symbol);


    // Unit conversion functions
    QString convertPascalString(const char *p);
	QString convertCString(const char *p, size_t n, bool ignore_first_newline);
	QString convertWideCString(const char *p, size_t n, bool ignore_first_newline);
    float convertRotation(int angle);
    void convertPoint(MapCoord &c, int ocad_x, int ocad_y);
    qint64 convertSize(int ocad_size);
    MapColor *convertColor(int color);
	double convertTemplateScale(double ocad_scale);

private:
	/// File path
	QString path;
	
    /// Handle to the open OCAD file
    OCADFile *file;

    /// Character encoding to use for 1-byte (narrow) strings
    QTextCodec *encoding_1byte;

    /// Character encoding to use for 2-byte (wide) strings
    QTextCodec *encoding_2byte;

    /// maps OCAD color number to oo-mapper color object
    QHash<int, MapColor *> color_index;

    /// maps OCAD symbol number to oo-mapper symbol object
    QHash<int, Symbol *> symbol_index;
	
	/// maps OO Mapper text symbol pointer to OCAD text symbol horizontal alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, int> text_halign_map;
	
	/// maps OCAD symbol number to rectangle information struct
	QHash<int, RectangleInfo> rectangle_info;

    /// Offset between OCAD map origin and Mapper map origin (in Mapper coordinates)
    qint64 offset_x, offset_y;

};

#endif // OCAD8_FILE_IMPORT_H
