/*
 *    Copyright 2013-2018 Kai Pastor
 *
 *    Some parts taken from file_format_oc*d8{.h,_p.h,cpp} which are
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

#ifndef OPENORIENTEERING_OCD_FILE_IMPORT
#define OPENORIENTEERING_OCD_FILE_IMPORT

#include <cstddef>
#include <initializer_list>
#include <vector>

#include <QtGlobal>
#include <QByteArray>
#include <QCoreApplication>
#include <QHash>
#include <QLocale>
#include <QString>

#include "core/map_coord.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_import_export.h"
#include "fileformats/ocd_types.h"
#include "fileformats/ocd_types_v8.h" // IWYU pragma: keep

class QChar;
class QTextCodec;

namespace OpenOrienteering {

class CombinedSymbol;
class Map;
class MapColor;
class MapPart;
class MapView;
class Symbol;


/**
 * An map file importer for OC*D files.
 */
class OcdFileImport : public Importer
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OcdFileImport)
	
protected:
	/// Information about an OC*D rectangle symbol
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
	
	// Helper classes that provide to core classes' protected members
	
	class OcdImportedAreaSymbol : public AreaSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedLineSymbol : public LineSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedPointSymbol : public PointSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedTextSymbol : public TextSymbol
	{
		friend class OcdFileImport;
	};
	
	class OcdImportedPathObject : public PathObject
	{
		friend class OcdFileImport;
		
	public:
		OcdImportedPathObject(Symbol* symbol = nullptr) : PathObject(symbol) { }
		OcdImportedPathObject(const OcdImportedPathObject&) = delete;
		OcdImportedPathObject(OcdImportedPathObject&&) = delete;
		OcdImportedPathObject& operator=(const OcdImportedPathObject&) = delete;
		OcdImportedPathObject& operator=(OcdImportedPathObject&&) = delete;
		~OcdImportedPathObject() override;
	};
	
public:
	OcdFileImport(const QString& path, Map *map, MapView *view);
	
	~OcdFileImport() override;
	
	
	void setCustom8BitEncoding(QTextCodec* encoding);
	
	
	template< unsigned char N >
	QString convertOcdString(const Ocd::PascalString<N>& src) const;
	
	template< unsigned char N >
	QString convertOcdString(const Ocd::Utf8PascalString<N>& src) const;
	
	template< std::size_t N >
	QString convertOcdString(const Ocd::Utf16PascalString<N>& src) const;
	
	template< class E >
	QString convertOcdString(const char* src, uint len) const;
	
	template< class E >
	QString convertOcdString(const QByteArray& data) const;
	
	QString convertOcdString(const QChar* src, uint maxlen) const;
	
	MapCoord convertOcdPoint(const Ocd::OcdPoint32& ocd_point) const;
	
	qreal convertAngle(int ocd_angle) const;
	
	int convertLength(qint16 ocd_length) const;
	
	int convertLength(quint16 ocd_length) const;
	
	template< class T, class R = qint64 >
	R convertLength(T ocd_length) const;
	
	const MapColor* convertColor(int ocd_color);
	
	void addSymbolWarning(const AreaSymbol* symbol, const QString& warning);
	
	void addSymbolWarning(const LineSymbol* symbol, const QString& warning);
	
	void addSymbolWarning(const TextSymbol* symbol, const QString& warning);
	
	
protected:
	bool importImplementation() override;
	
	template< class F >
	void importImplementation();
	
	
	struct StringHandler
	{
		using Callback = void (OcdFileImport::*)(const QString&);
		qint32   type;
		Callback callback;
	};
	
	template< class F >
	void handleStrings(const OcdFile< F >& file, std::initializer_list<StringHandler> handlers);
	
	
	void importGeoreferencing(const OcdFile<Ocd::FormatV8>& file);
	
	template< class F >
	void importGeoreferencing(const OcdFile< F >& file);
	
	/// Imports string 1039.
	void importGeoreferencing(const QString& param_string);
	
	
	void importColors(const OcdFile<struct Ocd::FormatV8>& file);
	
	template< class F >
	void importColors(const OcdFile< F >& file);
	
	void importSpotColor(const QString& param_string);
	
	void importColor(const QString& param_string);
	
	
	template< class F >
	void importSymbols(const OcdFile< F >& file);
	
	void resolveSubsymbols();
	
	
	void importObjects(const OcdFile<Ocd::FormatV8>& file);
	
	template< class F >
	void importObjects(const OcdFile< F >& file);
	
	
	template< class F >
	void importTemplates(const OcdFile< F >& file);
	
	void importTemplate(const QString& param_string);
	
	
	void importExtras(const OcdFile<Ocd::FormatV8>& file);
	
	template< class F >
	void importExtras(const OcdFile< F >& file);
	
	static const std::initializer_list<StringHandler> extraStringHandlers;
	
	void appendNotes(const QString& param_string);
	
	
	void importView(const OcdFile<Ocd::FormatV8>& file);
	
	template< class F >
	void importView(const OcdFile< F >& file);
	
	void importView(const QString& param_string);
	
	
	// Symbol import
	
	template< class S >
	PointSymbol* importPointSymbol(const S& ocd_symbol);
	
	template< class S >
	Symbol* importLineSymbol(const S& ocd_symbol);
	
	OcdImportedLineSymbol* importLineSymbolBase(const Ocd::LineSymbolCommonV8& attributes);
	
	void setupLineSymbolFraming(OcdImportedLineSymbol* framing_line, const Ocd::LineSymbolCommonV8& attributes, const LineSymbol* main_line);
	
	void setupLineSymbolDoubleBorder(OcdImportedLineSymbol* double_line, const Ocd::LineSymbolCommonV8& attributes);
	
	void setupLineSymbolForBorder(OcdImportedLineSymbol* line_for_borders, const Ocd::LineSymbolCommonV8& attributes);
	
	void setupLineSymbolPointSymbols(OcdImportedLineSymbol* line_symbol, const Ocd::LineSymbolCommonV8& attributes, const Ocd::PointSymbolElementV8* elements);
	
	void mergeLineSymbol(CombinedSymbol* full_line, LineSymbol* main_line, LineSymbol* framing_line, LineSymbol* double_line);
	
	Symbol* importAreaSymbol(const Ocd::AreaSymbolV8& ocd_symbol);
	
	template< class S >
	Symbol* importAreaSymbol(const S& ocd_symbol);
	
	void setupAreaSymbolCommon(
	        OcdImportedAreaSymbol* symbol,
	        bool fill_on,
	        const Ocd::AreaSymbolCommonV8& ocd_symbol,
	        std::size_t data_size,
	        const Ocd::PointSymbolElementV8* elements
	);
	
	template< class S >
	TextSymbol* importTextSymbol(const S& ocd_symbol);
	
	template< class S >
	TextSymbol* importLineTextSymbol(const S& ocd_symbol);
	
	template< class S >
	LineSymbol* importRectangleSymbol(const S& ocd_symbol);
	
	template< class OcdBaseSymbol >
	void setupBaseSymbol(Symbol* symbol, const OcdBaseSymbol& ocd_base_symbol);
	
	void setupPointSymbolPattern(PointSymbol* symbol, std::size_t data_size, const Ocd::PointSymbolElementV8* elements);
	
	
	// Object import
	
	template< class O >
	Object* importObject(const O& ocd_object, MapPart* part);
	
	QString getObjectText(const Ocd::ObjectV8& ocd_object) const;
	
	template< class O >
	QString getObjectText(const O& ocd_object) const;
	
	template< class O >
	Object* importRectangleObject(const O& ocd_object, MapPart* part, const OcdFileImport::RectangleInfo& rect);
	
	Object* importRectangleObject(const Ocd::OcdPoint32* ocd_points, MapPart* part, const OcdFileImport::RectangleInfo& rect);
	
	// Some helper functions that are used in multiple places
	
	void setPointFlags(OcdImportedPathObject* object, quint32 pos, bool is_area, const Ocd::OcdPoint32& ocd_point);
	
	void setPathHolePoint(OcdFileImport::OcdImportedPathObject* object, quint32 pos);
	
	void fillPathCoords(OcdFileImport::OcdImportedPathObject* object, bool is_area, quint32 num_points, const Ocd::OcdPoint32* ocd_points);
	
	bool fillTextPathCoords(TextObject* object, TextSymbol* symbol, quint32 npts, const Ocd::OcdPoint32* ocd_points);
	
	void setBasicAttributes(OcdImportedTextSymbol* symbol, const QString& font_name, const Ocd::BasicTextAttributesV8& attributes);
	
	void setSpecialAttributes(OcdImportedTextSymbol* symbol, const Ocd::SpecialTextAttributesV8& attributes);
	
	void setFraming(OcdImportedTextSymbol* symbol, const Ocd::FramingAttributesV8& framing);
	
protected:
	/// The locale is used for number formatting.
	QLocale locale;
	
	QByteArray buffer;
	
	/// Character encoding to use for 1-byte (narrow) strings
	QTextCodec *custom_8bit_encoding;
	
	/// Temporarily stores spot colors
	std::vector<MapColor*> spot_colors;
	
	/// maps OCD color number to oo-mapper color object
	QHash<int, const MapColor *> color_index;
	
	/// maps OCD symbol number to oo-mapper symbol object
	QHash<unsigned int, Symbol *> symbol_index;
	
	/// maps OO Mapper text symbol pointer to OCD defined horizontal alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, TextObject::HorizontalAlignment> text_halign_map;
	
	/// maps OO Mapper text symbol pointer to OCD defined vertical alignment (stored in objects instead of symbols in OO Mapper)
	QHash<Symbol*, TextObject::VerticalAlignment> text_valign_map;
	
	/// maps OCD symbol number to rectangle information struct
	QHash<int, RectangleInfo> rectangle_info;
	
	/// The actual format version of the imported file
	int ocd_version;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_FILE_IMPORT
