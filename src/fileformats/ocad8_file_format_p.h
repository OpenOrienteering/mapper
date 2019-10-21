/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2014, 2015 Kai Pastor
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

#ifndef OPENORIENTEERING_FILE_FORMAT_OCAD_P_H
#define OPENORIENTEERING_FILE_FORMAT_OCAD_P_H

#include <set>

#include <QCoreApplication>
#include <QRgb>

#include "core/map_coord.h"
#include "fileformats/file_import_export.h"
#include "libocad/libocad.h"

namespace OpenOrienteering {

class Map;
class MapColor;
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


/** Importer for OCD version 8 files. */
class OCAD8FileImport : public Importer
{
	friend class OcdFileImport;
	
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OCAD8FileImport)
	
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
	OCAD8FileImport(const QString& path, Map *map, MapView *view);
	~OCAD8FileImport() override;

	void setStringEncodings(const char *narrow, const char *wide = "UTF-16LE");

protected:
	bool importImplementation() override;
	
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
	virtual void importString(OCADStringEntry *entry);
	
	Template *importTemplate(OCADCString* ocad_str);
	OCADBackground importBackground(const QByteArray& data);
	/// @deprecated Replaced by Template *importTemplate(OCADCString* string).
	Template *importRasterTemplate(const OCADBackground &background);

	// Some helper functions that are used in multiple places
	PointSymbol *importPattern(s16 npts, OCADPoint *pts);
	void fillCommonSymbolFields(Symbol *symbol, const OCADSymbol *ocad_symbol);
	void setPathHolePoint(Object *object, int i);
	void fillPathCoords(Object* object, bool is_area, u16 npts, const OCADPoint* pts);
	bool fillTextPathCoords(TextObject* object, TextSymbol* symbol, u16 npts, OCADPoint* pts);


	// Unit conversion functions
	QString convertPascalString(const char *p);
	QString convertCString(const char *p, std::size_t n, bool ignore_first_newline);
	QString convertWideCString(const char *p, std::size_t n, bool ignore_first_newline);
	float convertRotation(int angle);
	void convertPoint(MapCoord &c, s32 ocad_x, s32 ocad_y);
	qint32 convertSize(int ocad_size);
	const MapColor *convertColor(int color);
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
	QHash<int, const MapColor *> color_index;

	/// maps OCAD symbol number to oo-mapper symbol object
	QHash<int, Symbol *> symbol_index;
	
	/// maps OO Mapper text symbol pointer to OCAD text symbol horizontal alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, int> text_halign_map;
	
	/// maps OCAD symbol number to rectangle information struct
	QHash<int, RectangleInfo> rectangle_info;

	/// Offset between OCAD map origin and Mapper map origin (in Mapper coordinates)
	qint64 offset_x, offset_y;
};


/** Exporter for OCD version 8 files. */
class OCAD8FileExport : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OCAD8FileExport)
	
public:
	OCAD8FileExport(const QString& path, const Map* map, const MapView* view);
	~OCAD8FileExport() override;
	
protected:
	bool exportImplementation() override;
	
	// Determines an offset for moving objects to the OCD drawing area.
	MapCoord calculateAreaOffset();
	
	// Symbol export
	void exportCommonSymbolFields(const Symbol* symbol, OCADSymbol* ocad_symbol, int size);
	void exportSymbolIcon(const Symbol* symbol, u8 ocad_icon[]);
	int getPatternSize(const PointSymbol* point);
	s16 exportPattern(const PointSymbol* point, OCADPoint** buffer);		// returns the number of written coordinates, including the headers
	s16 exportSubPattern(const Object* object, const Symbol* symbol, OCADPoint** buffer);
	
	s16 exportPointSymbol(const PointSymbol* point);
	s16 exportLineSymbol(const LineSymbol* line);
	s16 exportAreaSymbol(const AreaSymbol* area);
	s16 exportTextSymbol(const TextSymbol* text);
	void setTextSymbolFormatting(OCADTextSymbol* ocad_symbol, TextObject* formatting);
	std::set<s16> exportCombinedSymbol(const CombinedSymbol* combination);
	
	// Helper functions
	/// Returns the number of exported coordinates. If not nullptr, the given symbol is used to determine the meaning of dash points.
	u16 exportCoordinates(const MapCoordVector& coords, OCADPoint** buffer, const Symbol* symbol);
	u16 exportTextCoordinates(TextObject* object, OCADPoint** buffer);
	static int getOcadColor(QRgb rgb);
	s16 getPointSymbolExtent(const PointSymbol* symbol);
	
	// Conversion functions
	void convertPascalString(const QString& text, char* buffer, int buffer_size);
	void convertCString(const QString& text, unsigned char* buffer, int buffer_size);
	/// Returns the number of bytes written into buffer
	int convertWideCString(const QString& text, unsigned char* buffer, int buffer_size);
	int convertRotation(float angle);
	OCADPoint convertPoint(qint32 x, qint32 y);
	/// Attention: this ignores the coordinate flags!
	OCADPoint convertPoint(const MapCoord& coord);
	s32 convertSize(qint32 size);
	s16 convertColor(const MapColor* color) const;
	double convertTemplateScale(double mapper_scale);
	
private:
	/** Indicates that the map uses the special registration color. */
	bool uses_registration_color;
	
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
	QHash<const Symbol*, std::set<s16> > symbol_index;
	
	/// In .ocd 8, text alignment needs to be specified in the text symbols instead of objects, so it is possible
	/// that multiple ocd text symbols have to be created for one native TextSymbol.
	/// This structure maps text symbols to lists containing information about the already created ocd symbols.
	/// The first member in each pair just gives information about the alignment option used for the symbol indexed by the
	/// second part of the pair.
	/// If there is no entry for a TextSymbol in this map yet, no object using this symbol has been encountered yet,
	/// no no specific formatting was set in the corresponding symbol (which has to be looked up using symbol_index).
	typedef std::vector< std::pair< int, s16 > > TextFormatList;
	QHash<const TextSymbol*, TextFormatList > text_format_map;
	
	/// Helper object for pattern export
	PointObject* origin_point_object;
	
	void addStringTruncationWarning(const QString& text, int truncation_pos);
};


}  // namespace OpenOrienteering

#endif
