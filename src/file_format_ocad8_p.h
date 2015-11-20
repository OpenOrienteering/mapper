/*
 *    Copyright 2012, 2013 Pete Curtis
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

#ifndef _OPENORIENTEERING_FILE_FORMAT_OCAD_P_H
#define _OPENORIENTEERING_FILE_FORMAT_OCAD_P_H

#include "file_import_export.h"

#include <set>

#include <QRgb>

#include "libocad/libocad.h"

#include "map_coord.h"

class Map;
class MapColor;
class MapCoord;
class MapPart;
class Object;
class PointObject;
class TextObject;
class Symbol;
class AreaSymbol;
class CombinedSymbol;
class LineSymbol;
class PointSymbol;
class Template;
class TextSymbol;

// ### OCAD8FileImport declaration ###

class OCAD8FileImport : public Importer
{
Q_OBJECT
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
	OCAD8FileImport(QIODevice* stream, Map *map, MapView *view);
    ~OCAD8FileImport();

    void setStringEncodings(const char *narrow, const char *wide = "UTF-16LE");

    static const float ocad_pt_in_mm;

protected:
	void import(bool load_symbols_only) throw (FileFormatException);
	
    // Symbol import
    Symbol *importPointSymbol(const OCADPointSymbol *ocad_symbol);
    Symbol *importLineSymbol(const OCADLineSymbol *ocad_symbol);
    Symbol *importAreaSymbol(const OCADAreaSymbol *ocad_symbol);
    Symbol *importTextSymbol(const OCADTextSymbol *ocad_symbol);
    RectangleInfo *importRectSymbol(const OCADRectSymbol *ocad_symbol);

    // Object import
    Object *importObject(const OCADObject *ocad_object, MapPart* part);
	bool importRectangleObject(const OCADObject* ocad_object, MapPart* part, const RectangleInfo& rect);

    // String import
    void importString(OCADStringEntry *entry);
    Template *importRasterTemplate(const OCADBackground &background);

    // Some helper functions that are used in multiple places
    PointSymbol *importPattern(s16 npts, OCADPoint *pts);
    void fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol);
	void setPathHolePoint(Object *object, int i);
	void fillPathCoords(Object* object, bool is_area, u16 npts, const OCADPoint* pts);
	bool fillTextPathCoords(TextObject* object, TextSymbol* symbol, u16 npts, OCADPoint* pts);


    // Unit conversion functions
    QString convertPascalString(const char *p);
	QString convertCString(const char *p, size_t n, bool ignore_first_newline);
	QString convertWideCString(const char *p, size_t n, bool ignore_first_newline);
    float convertRotation(int angle);
    void convertPoint(MapCoord &c, int ocad_x, int ocad_y);
    qint64 convertSize(int ocad_size);
    MapColor *convertColor(int color);
	double convertTemplateScale(double ocad_scale);
	
	static bool isRasterImageFile(const QString &filename);

private:
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


// ### OCAD8FileExport declaration ###

class OCAD8FileExport : public Exporter
{
Q_OBJECT
public:
	OCAD8FileExport(QIODevice* stream, Map *map, MapView *view);
	~OCAD8FileExport();
	
	void doExport() throw (FileFormatException);
	
protected:
	
	// Symbol export
	void exportCommonSymbolFields(Symbol* symbol, OCADSymbol* ocad_symbol, int size);
	int getPatternSize(PointSymbol* point);
	s16 exportPattern(PointSymbol* point, OCADPoint** buffer);		// returns the number of written coordinates, including the headers
	s16 exportSubPattern(Object* object, Symbol* symbol, OCADPoint** buffer);
	
	s16 exportPointSymbol(PointSymbol* point);
	s16 exportLineSymbol(LineSymbol* line);
	s16 exportAreaSymbol(AreaSymbol* area);
	s16 exportTextSymbol(TextSymbol* text);
	void setTextSymbolFormatting(OCADTextSymbol* ocad_symbol, TextObject* formatting);
	std::set<s16> exportCombinedSymbol(CombinedSymbol* combination);
	
	// Helper functions
	/// Returns the number of exported coordinates. If not NULL, the given symbol is used to determine the meaning of dash points.
	u16 exportCoordinates(const MapCoordVector& coords, OCADPoint** buffer, Symbol* symbol);
	u16 exportTextCoordinates(TextObject* object, OCADPoint** buffer);
	int getOcadColor(QRgb rgb);
	s16 getPointSymbolExtent(PointSymbol* symbol);
	
	// Conversion functions
	void convertPascalString(const QString& text, char* buffer, int buffer_size);
	void convertCString(const QString& text, unsigned char* buffer, int buffer_size);
	/// Returns the number of bytes written into buffer
	int convertWideCString(const QString& text, unsigned char* buffer, int buffer_size);
	int convertRotation(float angle);
	OCADPoint convertPoint(qint64 x, qint64 y);
	/// Attention: this ignores the coordinate flags!
	OCADPoint convertPoint(const MapCoord& coord);
	s32 convertSize(qint64 size);
	s16 convertColor(const MapColor* color) const;
	double convertTemplateScale(double mapper_scale);
	
private:
	/// Handle to the open OCAD file
	OCADFile *file;
	
	/// Character encoding to use for 1-byte (narrow) strings
	QTextCodec *encoding_1byte;
	
	/// Character encoding to use for 2-byte (wide) strings
	QTextCodec *encoding_2byte;
	
	/// Set of used symbol numbers. Needed to ensure uniqueness of the symbol number as Mapper does not enforce it,
	/// but the indexing of symbols in OCAD depends on it.
	std::set<s16> symbol_numbers;
	
	/// Maps OO Mapper symbol pointer to a list of OCAD symbol numbers.
	/// Usually the list contains only one entry, except for combined symbols,
	/// for which it contains the indices of all basic parts
	QHash<Symbol*, std::set<s16> > symbol_index;
	
	/// In .ocd 8, text alignment needs to be specified in the text symbols instead of objects, so it is possible
	/// that multiple ocd text symbols have to be created for one native TextSymbol.
	/// This structure maps text symbols to lists containing information about the already created ocd symbols.
	/// The TextObject in each pair just gives information about the alignment option used for the symbol indexed by the
	/// second part of the pair.
	/// If there is no entry for a TextSymbol in this map yet, no object using this symbol has been encountered yet,
	/// no no specific formatting was set in the corresponding symbol (which has to be looked up using symbol_index).
	typedef std::vector< std::pair< TextObject*, s16 > > TextFormatList;
	QHash<TextSymbol*, TextFormatList > text_format_map;
	
	/// Helper object for pattern export
	PointObject* origin_point_object;
	
	void addStringTruncationWarning(const QString& text, int truncation_pos);
};

#endif
