/*
 *    Copyright 2016-2018 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_FILE_EXPORT_H
#define OPENORIENTEERING_OCD_FILE_EXPORT_H

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <QtGlobal>
#include <QByteArray>
#include <QCoreApplication>
#include <QLocale>
#include <QString>
#include <QTextCodec>

#include "core/map_coord.h"
#include "fileformats/file_import_export.h"


template< class Format > class OcdFile;

namespace OpenOrienteering {

class AreaSymbol;
class CombinedSymbol;
class LineSymbol;
class Map;
class MapColor;
class MapView;
class Object;
class PathObject;
class PointObject;
class PointSymbol;
class Symbol;
class TextObject;
class TextSymbol;


/**
 * An exporter for OCD files.
 * 
 * The functions in this class are designed around the following pattern:
 * 
 * From `template<class Format> void OcdFileExport::exportImplementation()` we
 * call into template functions so that we get a template function instantiation
 * for the actual types of the format. This is what we can do:
 *
 * - If a format is significantly different, so that it needs a different algorithm:
 *   We specialize the template function (as done in 
 *   `template<> void OcdFileExport::exportSetup(OcdFile<Ocd::FormatV8>& file)`).
 * - If the same algorithm can be used for multiple formats:
 *   - If the differences are completely represented by the the exporter's state
 *     (which captures version and parameter string handler), then we may just
 *     call into a non-template function (as done in
 *     `template<class Format> void OcdFileExport::exportSetup(OcdFile<Format>&)`.
 *     The template function is likely to get inlined, thus reducing/avoiding
 *     template bloat.
 *   - Otherwise we still can use the same algorithm but need to specialize it
 *     for the actual types of the format. This specialization is solved by
 *     function template instantiation, i.e. the generic algorithm goes directly
 *     into the template function. This is the case for `exportSymbol()` where
 *     types and even the encoding of the symbol number vary over different
 *     versions of the format.
 *     This doesn't mean that there couldn't be room for reducing template bloat
 *     by factoring out universal parts. However, if the size gains are small,
 *     it might not be worth the increased complexity.
 */
class OcdFileExport : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OcdFileExport)
	
	/**
	 * A type for temporaries helping to convert strings to Ocd format.
	 */
	struct ExportableString
	{
		const QString& string;
		const QTextCodec* custom_8bit_encoding;
		
		operator QByteArray() const
		{
			return custom_8bit_encoding ? custom_8bit_encoding->fromUnicode(string) : string.toUtf8();
		}
		
		operator QString() const noexcept
		{
			return string;
		}
		
	};
	
	ExportableString toOcdString(const QString& string) const
	{
		return { string, custom_8bit_encoding };
	}
	
	
	using StringAppender = void (qint32, const QString&);
	
public:
	/// \todo Add proper API
	static quint16 default_version;
	
	
	OcdFileExport(const QString& path, const Map* map, const MapView* view, quint16 version);
	
	~OcdFileExport() override;
	
protected:
	/**
	 * Exports an OCD file.
	 * 
	 * For now, this simply uses the OCAD8FileExport class unless the file name
	 * ends with "test-vVERSION.ocd".
	 */
	bool exportImplementation() override;
	
protected:
	
	template< class Encoding >
	QTextCodec* determineEncoding();
	
	
	template< class Format >
	bool exportImplementation();
	
	
	/**
	 * Calculates an offset to be applied to the coordinates so that the
	 * map data is moved into the limited OCD drawing area.
	 */
	MapCoord calculateAreaOffset();
	
	
	template< class Format >
	void exportSetup(OcdFile<Format>& file);
	
	void exportSetup();
	
	void exportGeoreferencing();
	
	
	template< class Format >
	void exportSymbols(OcdFile<Format>& file);
	
	template< class OcdBaseSymbol >
	void setupBaseSymbol(const Symbol* symbol, quint32 symbol_number, OcdBaseSymbol& ocd_base_symbol);
	
	template< class OcdBaseSymbol >
	void setupIcon(const Symbol* symbol, OcdBaseSymbol& ocd_base_symbol);
	
	template< class OcdPointSymbol >
	QByteArray exportPointSymbol(const PointSymbol* point_symbol);
	
	template< class Element >
	qint16 exportPattern(const PointSymbol* point, QByteArray& byte_array);		// returns the number of written coordinates, including the headers
	
	template< class Element >
	qint16 exportSubPattern(const MapCoordVector& coords, const Symbol* symbol, QByteArray& byte_array);
	
	template< class OcdAreaSymbol >
	QByteArray exportAreaSymbol(const AreaSymbol* area_symbol);
	
	template< class OcdAreaSymbol >
	QByteArray exportAreaSymbol(const AreaSymbol* area_symbol, quint32 symbol_number);
	
	template< class OcdAreaSymbolCommon >
	quint8 exportAreaSymbolCommon(const AreaSymbol* area_symbol, OcdAreaSymbolCommon& ocd_area_common, const PointSymbol*& pattern_symbol);
	
	template< class OcdAreaSymbol >
	void exportAreaSymbolSpecial(const AreaSymbol* area_symbol, OcdAreaSymbol& ocd_area_symbol);
	
	template< class OcdLineSymbol >
	QByteArray exportLineSymbol(const LineSymbol* line_symbol);
	
	template< class OcdLineSymbol >
	QByteArray exportLineSymbol(const LineSymbol* line_symbol, quint32 symbol_number);
	
	template< class OcdLineSymbolCommon >
	quint32 exportLineSymbolCommon(const LineSymbol* line_symbol, OcdLineSymbolCommon& ocd_line_common);
	
	template< class OcdLineSymbolCommon >
	void exportLineSymbolDoubleLine(const LineSymbol* line_symbol, quint32 fill_color, OcdLineSymbolCommon& ocd_line_common);
	
	template< class Format, class OcdTextSymbol >
	void exportTextSymbol(OcdFile<Format>& file, const TextSymbol* text_symbol);
	
	template< class OcdTextSymbol >
	QByteArray exportTextSymbol(const TextSymbol* text_symbol, quint32 symbol_number, int alignment);
	
	template< class OcdTextSymbol >
	void setupTextSymbolExtra(const TextSymbol* text_symbol, OcdTextSymbol& ocd_text_symbol);
	
	template< class OcdTextSymbolBasic >
	void setupTextSymbolBasic(const TextSymbol* text_symbol, int alignment, OcdTextSymbolBasic& ocd_text_basic);
	
	template< class OcdTextSymbolSpecial >
	void setupTextSymbolSpecial(const TextSymbol* text_symbol, OcdTextSymbolSpecial& ocd_text_special);
	
	template< class OcdTextSymbolFraming >
	void setupTextSymbolFraming(const TextSymbol* text_symbol, OcdTextSymbolFraming& ocd_text_framing);
	
	template< class Format >
	void exportCombinedSymbol(OcdFile<Format>& file, const CombinedSymbol* combined_symbol);
	
	template< class Format >
	void exportGenericCombinedSymbol(OcdFile<Format>& file, const CombinedSymbol* combined_symbol);
	
	template< class OcdAreaSymbol >
	QByteArray exportCombinedAreaSymbol(
	        quint32 symbol_number,
	        const CombinedSymbol* combined_symbol,
	        const AreaSymbol* area_symbol,
	        const LineSymbol* line_symbol );
	
	template< class OcdLineSymbol >
	QByteArray exportCombinedLineSymbol(
	        quint32 symbol_number,
	        const CombinedSymbol* combined_symbol,
	        const LineSymbol* main_line,
	        const LineSymbol* framing,
	        const LineSymbol* double_line );
	
	
	template< class Format >
	void exportObjects(OcdFile<Format>& file);
	
	template< class OcdObject >
	void handleObjectExtras(const Object* object, OcdObject& ocd_object, typename OcdObject::IndexEntryType& entry);
	
	template< class OcdObject >
	QByteArray exportPointObject(const PointObject* point, typename OcdObject::IndexEntryType& entry);
	
	template< class Format >
	void exportPathObject(OcdFile<Format>& file, const PathObject* path, bool lines_only = false);
	
	template< class OcdObject >
	QByteArray exportTextObject(const TextObject* text, typename OcdObject::IndexEntryType& entry);
	
	template< class OcdObject >
	QByteArray exportObjectCommon(const Object* object, OcdObject& ocd_object, typename OcdObject::IndexEntryType& entry);
	
	
	template< class Format >
	void exportTemplates(OcdFile<Format>& file);
	
	void exportTemplates();
	
	
	template< class Format >
	void exportExtras(OcdFile<Format>& file);
	
	void exportExtras();
	
	
	quint16 convertColor(const MapColor* color) const;
	
	quint16 getPointSymbolExtent(const PointSymbol* symbol) const;
	
	quint16 exportCoordinates(const MapCoordVector& coords, const Symbol* symbol, QByteArray& byte_array);
	
	quint16 exportCoordinates(const MapCoordVector& coords, const Symbol* symbol, QByteArray& byte_array, MapCoord& bottom_left, MapCoord& top_right);
	
	quint16 exportTextCoordinatesSingle(const TextObject* object, QByteArray& byte_array, MapCoord& bottom_left, MapCoord& top_right);
	
	quint16 exportTextCoordinatesBox(const TextObject* object, QByteArray& byte_array, MapCoord& bottom_left, MapCoord& top_right);
	
	QByteArray exportTextData(const TextObject* object, int chunk_size, int max_chunks);
	
	
	void addTextTruncationWarning(QString text, int pos);
	
	
	quint32 makeUniqueSymbolNumber(quint32 initial_number) const;
	
	
private:
	/// The locale is used for number formatting.
	QLocale locale;
	
	/// Character encoding to use for 1-byte (narrow) strings
	QTextCodec *custom_8bit_encoding = nullptr;
	
	MapCoord area_offset;
	
	std::function<StringAppender> addParameterString;
	
	std::unordered_map<const Symbol*, quint32> symbol_numbers;
	
	struct TextFormatMapping
	{
		const Symbol* symbol;
		int           alignment;
		int           count;
		quint32       symbol_number;
	};
	std::vector<TextFormatMapping> text_format_mapping;
	
	struct BreakdownEntry
	{
		quint32 number;
		quint8 type;
	};
	std::vector<BreakdownEntry> breakdown_list;
	std::unordered_map<quint32, std::size_t> breakdown_index;
	
	std::vector<std::unique_ptr<const Symbol>> number_owners;
	
	quint16 ocd_version;
	
	bool uses_registration_color = false;
};


}  // namespace OpenOrienteering

#endif
