/*
 *    Copyright 2013-2015 Kai Pastor
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

#include "ocd_file_format.h"
#include "ocd_file_format_p.h"

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>

#include "ocd_types_v8.h"
#include "ocd_types_v9.h"
#include "ocd_types_v10.h"
#include "ocd_types_v11.h"
#include "../core/map_color.h"
#include "../core/map_view.h"
#include "../core/georeferencing.h"
#include "../file_format_ocad8.h"
#include "../file_format_ocad8_p.h"
#include "../map.h"
#include "../object_text.h"
#include "../settings.h"
#include "../symbol_area.h"
#include "../symbol_combined.h"
#include "../symbol_line.h"
#include "../symbol_point.h"
#include "../symbol_text.h"
#include "../template.h"
#include "../template_image.h"
#include "../template_map.h"
#include "../util.h"


// ### OcdFileFormat ###

OcdFileFormat::OcdFileFormat()
 : FileFormat(MapFile, "OCD", ImportExport::tr("OCAD"), "ocd",
     ImportSupported | ExportSupported | ExportLossy)
{
	// Nothing
}

bool OcdFileFormat::understands(const unsigned char* buffer, size_t sz) const
{
	// The first two bytes of the file must be 0x0cad in litte endian ordner.
	// This test will refuse to understand OCD files on big endian systems:
	// The importer's current implementation won't work there.
	return (sz >= 2 && *reinterpret_cast<const quint16*>(buffer) == 0x0cad);
}

Importer* OcdFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const
{
	return new OcdFileImport(stream, map, view);
}

Exporter* OcdFileFormat::createExporter(QIODevice* stream, Map* map, MapView* view) const
{
	return new OCAD8FileExport(stream, map, view);
}


// ### OcdFileImport ###

OcdFileImport::OcdFileImport(QIODevice* stream, Map* map, MapView* view)
 : Importer(stream, map, view)
 , delegate(nullptr)
{
    custom_8bit_encoding = QTextCodec::codecForName("Windows-1252");
}

OcdFileImport::~OcdFileImport()
{
	// nothing
}

void OcdFileImport::setCustom8BitEncoding(const char* encoding)
{
    custom_8bit_encoding = QTextCodec::codecForName(encoding);
}

void OcdFileImport::addSymbolWarning(LineSymbol* symbol, const QString& warning)
{
	addWarning( tr("In line symbol %1 '%2': %3").
	            arg(symbol->getNumberAsString()).
	            arg(symbol->getName()).
	            arg(warning) );
}

void OcdFileImport::addSymbolWarning(TextSymbol* symbol, const QString& warning)
{
	addWarning( tr("In text symbol %1 '%2': %3").
	            arg(symbol->getNumberAsString()).
	            arg(symbol->getName()).
	            arg(warning) );
}

#ifndef NDEBUG

// Heuristic detection of implementation errors
template< >
inline
qint64 OcdFileImport::convertLength< quint8 >(quint8 ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	if (ocd_length > 200)
		qDebug() << "quint8 has value" << ocd_length << ", might be qint8" << (qint8)ocd_length;
	return ((qint64)ocd_length) * 10;
}

template< >
inline
qint64 OcdFileImport::convertLength< quint16 >(quint16 ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	if (ocd_length > 50000)
		qDebug() << "quint16 has value" << ocd_length << ", might be qint16" << (qint16)ocd_length;
	return ((qint64)ocd_length) * 10;
}

template< >
inline
qint64 OcdFileImport::convertLength< quint32 >(quint32 ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	if (ocd_length > 3000000)
		qDebug() << "quint32 has value" << ocd_length << ", might be qint32" << (qint32)ocd_length;
	return ((qint64)ocd_length) * 10;
}
#endif // !NDEBUG

template< >
void OcdFileImport::importImplementation< Ocd::FormatLegacyImporter >(bool load_symbols_only)
{
	QBuffer new_stream(&buffer);
	new_stream.open(QIODevice::ReadOnly);
	delegate.reset(new OCAD8FileImport(&new_stream, map, view));
	
	delegate->import(load_symbols_only);
	
	for (auto&& w : delegate->warnings())
	{
		addWarning(w);
	}
	
	for (auto&& a : delegate->actions())
	{
		addAction(a);
	}
}

template< class F >
void OcdFileImport::importImplementation(bool load_symbols_only)
{
	OcdFile< F > file(buffer);
#if 0
	for (auto&& string : file.strings())
	{
		qDebug() << string.type << convertOcdString< typename F::Encoding >(file[string]);
	}
#endif
	
	importGeoreferencing< F >(file);
	importColors< F >(file);
	importSymbols< F >(file);
	if (!load_symbols_only)
	{
		importExtras< F >(file);
		importObjects< F >(file);
		importTemplates< F >(file);
		if (view)
		{
			importView< F >(file);
		}
	}
}

template< >
void OcdFileImport::importGeoreferencing< Ocd::FormatV8 >(const OcdFile< Ocd::FormatV8 >& file)
{
	const Ocd::FileHeaderV8* header = file.header();
	const Ocd::SetupV8* setup = reinterpret_cast< const Ocd::SetupV8* >(file.byteArray().data() + header->setup_pos);
	
	Georeferencing georef;
	georef.setScaleDenominator(setup->map_scale);
	georef.setProjectedRefPoint(QPointF(setup->real_offset_x, setup->real_offset_y));
	if (qAbs(setup->real_angle) >= 0.01) /* degrees */
	{
		georef.setGrivation(setup->real_angle);
	}
	map->setGeoreferencing(georef);
}

template< class F >
void OcdFileImport::importGeoreferencing(const OcdFile< F >& file)
{
	for (auto&& string : file.strings())
	{
		if (string.type == 1039)
		{
			importGeoreferencing(convertOcdString< typename F::Encoding >(file[string]));
			break;
		}
	}
}

void OcdFileImport::importGeoreferencing(const QString& param_string)
{
	const QChar* unicode = param_string.unicode();
	
	Georeferencing georef;
	QPointF proj_ref_point;
	bool x_ok = false, y_ok = false;
	
	int i = param_string.indexOf('\t', 0);
	; // skip first word for this entry type
	while (i >= 0)
	{
		bool ok;
		int next_i = param_string.indexOf('\t', i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
			case '\t':
				// empty item
				break;
			case 'm':
			{
				double scale = param_value.toDouble(&ok);
				if (ok && scale >= 0) georef.setScaleDenominator(qRound(scale));
				break;
			}
			case 'a':
			{
				double angle = param_value.toDouble(&ok);
				if (ok && qAbs(angle) >= 0.01) georef.setGrivation(angle);
				break;
			}
			case 'x':
			{
				proj_ref_point.setX(param_value.toDouble(&x_ok));
				break;
			}
			case 'y':
			{
				proj_ref_point.setY(param_value.toDouble(&y_ok));
				break;
			}
			default:
				; // nothing
		}
		i = next_i;
	}
	
	if (x_ok && y_ok)
	{
		georef.setProjectedRefPoint(proj_ref_point);
	}
	
	map->setGeoreferencing(georef);
}

template< >
void OcdFileImport::importColors< struct Ocd::FormatV8 >(const OcdFile< Ocd::FormatV8 >& file)
{
	const Ocd::SymbolHeaderV8 & symbol_header = file.header()->symbol_header;
	int num_colors = symbol_header.num_colors;
	
	for (int i = 0; i < num_colors && i < 256; i++)
	{
		const Ocd::ColorInfoV8& color_info = symbol_header.color_info[i];
		const QString name = convertOcdString(color_info.name);
		int color_pos = map->getNumColors();
		MapColor* color = new MapColor(name, color_pos);
		
		// OC*D stores CMYK values as integers from 0-200.
		MapColorCmyk cmyk;
		cmyk.c = 0.005f * color_info.cmyk.cyan;
		cmyk.m = 0.005f * color_info.cmyk.magenta;
		cmyk.y = 0.005f * color_info.cmyk.yellow;
		cmyk.k = 0.005f * color_info.cmyk.black;
		color->setCmyk(cmyk);
		color->setOpacity(1.0f);
		
		map->addColor(color, color_pos);
		color_index[color_info.number] = color;
	}
	
	addWarning(OcdFileImport::tr("Spot color information was ignored."));
}

template< class F >
void OcdFileImport::importColors(const OcdFile< F >& file)
{
	for (auto&& string : file.strings())
	{
		if (string.type == 9)
		{
			importColor(convertOcdString< typename F::Encoding >(file[string]));
		}
	}
	addWarning(tr("Spot color information was ignored."));
}

MapColor* OcdFileImport::importColor(const QString& param_string)
{
	const QChar* unicode = param_string.unicode();
	
	int i = param_string.indexOf('\t', 0);
	const QString name = param_string.left(qMax(-1, i)); // copied
	
	int number;
	bool number_ok = false;
	MapColorCmyk cmyk;
	bool overprinting = false;
	float opacity = 1.0f;
	
	while (i >= 0)
	{
		float f_value;
		int i_value;
		bool ok;
		int next_i = param_string.indexOf('\t', i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
			case '\t':
				// empty item
				break;
			case 'n':
				number = param_value.toInt(&number_ok);
				break;
			case 'c':
				f_value = param_value.toFloat(&ok);
				if (ok && f_value >= 0 && f_value <= 100)
					cmyk.c = 0.01f * f_value;
				break;
			case 'm':
				f_value = param_value.toFloat(&ok);
				if (ok && f_value >= 0 && f_value <= 100)
					cmyk.m = 0.01f * f_value;
				break;
			case 'y':
				f_value = param_value.toFloat(&ok);
				if (ok && f_value >= 0 && f_value <= 100)
					cmyk.y = 0.01f * f_value;
				break;
			case 'k':
				f_value = param_value.toFloat(&ok);
				if (ok && f_value >= 0 && f_value <= 100)
					cmyk.k = 0.01f * f_value;
				break;
			case 'o':
				i_value = param_value.toInt(&ok);
				if (ok)
					overprinting = i_value;
				break;
			case 't':
				f_value = param_value.toFloat(&ok);
				if (ok && f_value >= 0.f && f_value <= 100.f)
					opacity = 0.01f * f_value;
				break;
			default:
				; // nothing
		}
		i = next_i;
	}
	
	if (!number_ok)
		return nullptr;
		
	int color_pos = map->getNumColors();
	MapColor* color = new MapColor(name, color_pos);
	color->setCmyk(cmyk);
	color->setKnockout(!overprinting);
	color->setOpacity(opacity);
	map->addColor(color, color_pos);
	color_index[number] = color;
	
	return color;
}

template< class F >
void OcdFileImport::importSymbols(const OcdFile< F >& file)
{
	for (typename OcdFile< F >::SymbolIndex::iterator it = file.symbols().begin(); it != file.symbols().end(); ++it)
	{
		// When extra symbols are created, we want to insert the main symbol
		// before them, i.e. at pos.
		int pos = map->getNumSymbols();
		Symbol* symbol = nullptr;
		
		// Don't use switch, because F::SymbolType may have duplicate values.
		if (it->type == F::TypePoint)
		{
			symbol = importPointSymbol((const typename F::PointSymbol&)*it);
		}
		else if (it->type == F::TypeLine)
		{
			symbol = importLineSymbol((const typename F::LineSymbol&)*it);
		}
		else if (it->type == F::TypeArea)
		{
			symbol = importAreaSymbol((const typename F::AreaSymbol&)*it, F::version());
		}
		else if (it->type == F::TypeText)
		{
			symbol = importTextSymbol((const typename F::TextSymbol&)*it);
		}
		else if (it->type == F::TypeRectangle)
		{
			symbol = importRectangleSymbol((const typename F::RectangleSymbol&)*it);
		}
		else if (it->type == F::TypeLineText)
		{
			symbol = importLineTextSymbol((const typename F::LineTextSymbol&)*it);
		}
		else if (it->type)
		{
			addWarning(tr("Unable to import symbol %1.%2 \"%3\": %4") .
			           arg(it->number / F::BaseSymbol::symbol_number_factor) .
			           arg(it->number % F::BaseSymbol::symbol_number_factor) .
			           arg(convertOcdString(it->description)).
			           arg(tr("Unsupported type \"%1\".").arg(it->type)) );
			continue;
		}
		
		map->addSymbol(symbol, pos);
		symbol_index[it->number] = symbol;
	}
}

template< >
void OcdFileImport::importObjects< struct Ocd::FormatV8 >(const OcdFile< Ocd::FormatV8 >& file)
{
	MapPart* part = map->getCurrentPart();
	Q_ASSERT(part);
	
	for (auto&& object_entry : file.objects())
	{
		if (object_entry.symbol)
		{
			auto object = importObject(file[object_entry], part);
			if (object)
			{
				part->addObject(object, part->getNumObjects());
			}
		}
	}
}

template< class F >
void OcdFileImport::importObjects(const OcdFile< F >& file)
{
	MapPart* part = map->getCurrentPart();
	Q_ASSERT(part);
	
	for (auto&& object_entry : file.objects())
	{
		if ( object_entry.symbol
		     && object_entry.status != OcdFile< F >::ObjectIndex::EntryType::StatusDeleted
		     && object_entry.status != OcdFile< F >::ObjectIndex::EntryType::StatusDeletedForUndo )
		{
			auto object = importObject(file[object_entry], part);
			if (object)
			{
				part->addObject(object, part->getNumObjects());
			}
		}
	}
}

template< class F >
void OcdFileImport::importTemplates(const OcdFile< F >& file)
{
	for (auto&& string : file.strings())
	{
		if (string.type == 8)
		{
			importTemplate(convertOcdString< typename F::Encoding >(file[string]), F::version());
		}
	}
}

Template* OcdFileImport::importTemplate(const QString& param_string, const int ocd_version)
{
	const QChar* unicode = param_string.unicode();
	
	int i = param_string.indexOf('\t', 0);
	const QString filename = QString::fromRawData(unicode, qMax(-1, i));
	const QString clean_path = QDir::cleanPath(QString(filename).replace('\\', '/'));
	const QString extension = QFileInfo(clean_path).suffix().toLower();
	
	Template* templ = nullptr;
	if (extension.compare("ocd") == 0)
	{
		templ = new TemplateMap(clean_path, map);
	}
	else if (QImageReader::supportedImageFormats().contains(extension.toLatin1()))
	{
		templ = new TemplateImage(clean_path, map);
	}
	else
	{
		addWarning(tr("Unable to import template: \"%1\" is not a supported template type.").arg(filename));
		return nullptr;
	}
	
	// 8 or 9 or 10 ? Only tested with 8 and 11
	double scale_factor = (ocd_version <= 8) ? 0.01 : 1.0;
	unsigned int num_rotation_params = 0;
	double rotation = 0.0;
	double scale_x = 1.0;
	double scale_y = 1.0;
	int dimming = 0;
	bool visible = false;
	
	while (i >= 0)
	{
		double value;
		bool ok;
		int next_i = param_string.indexOf('\t', i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
			case '\t':
				// empty item
				break;
			case 'x':
				value = param_value.toDouble(&ok);
				if (ok)
					templ->setTemplateX(qRound64(value*1000*scale_factor));
				break;
			case 'y':
				value = param_value.toDouble(&ok);
				if (ok)
					templ->setTemplateY(-qRound64(value*1000*scale_factor));
				break;
			case 'a':
			case 'b':
				// TODO: use the distinct angles correctly, not just the average
				rotation += param_value.toDouble(&ok);
				if (ok)
					++num_rotation_params;
				break;
			case 'u':
				value = param_value.toDouble(&ok);
				if (ok && qAbs(value) >= 0.0000000001)
					scale_x = value;
				break;
			case 'v':
				value = param_value.toDouble(&ok);
				if (ok && qAbs(value) >= 0.0000000001)
					scale_y = value;
				break;
			case 'd':
				dimming = param_value.toInt();
				break;
			case 's':
				visible = param_value.toInt();
				break;
			default:
				; // nothing
		}
		i = next_i;
	}
	
	if (num_rotation_params)
		templ->setTemplateRotation(Georeferencing::degToRad(rotation / num_rotation_params));
	
	templ->setTemplateScaleX(scale_x * scale_factor);
	templ->setTemplateScaleY(scale_y * scale_factor);
	
	int template_pos = map->getFirstFrontTemplate();
	map->addTemplate(templ, 0, view);
	map->setFirstFrontTemplate(template_pos+1);
	
	if (view)
	{
		TemplateVisibility* visibility = view->getTemplateVisibility(templ);
		visibility->opacity = qMax(0.0, qMin(1.0, 0.01 * (100 - dimming)));
		visibility->visible = visible;
	}
	
	return templ;
}

template< >
void OcdFileImport::importExtras< struct Ocd::FormatV8 >(const OcdFile< Ocd::FormatV8 >& file)
{
	const Ocd::FileHeaderV8* header = file.header();
	map->setMapNotes(convertOcdString< Ocd::FormatV8::Encoding >(file.byteArray().data() + header->info_pos, header->info_size));
}

template< class F >
void OcdFileImport::importExtras(const OcdFile< F >& file)
{
	QString notes;
	
	for (auto&& string : file.strings())
	{
		switch (string.type)
		{
		case 11:
			// OCD 9, 10
			notes.append(convertOcdString< typename F::Encoding >(file[string]));
			notes.append("\n");
			break;
		case 1061:
			// OCD 11
			notes.append(convertOcdString< typename F::Encoding >(file[string]));
			break;
		default:
			; // nothing
		}
	}
	
	map->setMapNotes(notes);
}

template< >
void OcdFileImport::importView< struct Ocd::FormatV8 >(const OcdFile< Ocd::FormatV8 >& file)
{
	if (view)
	{
		const Ocd::FileHeaderV8* header = file.header();
		const Ocd::SetupV8* setup = reinterpret_cast< const Ocd::SetupV8* >(file.byteArray().data() + header->setup_pos);
		
		if (setup->zoom >= MapView::zoom_out_limit && setup->zoom <= MapView::zoom_in_limit)
			view->setZoom(setup->zoom);
		
		view->setCenter(convertOcdPoint(setup->center));
	}
}

template< class F >
void OcdFileImport::importView(const OcdFile< F >& file)
{
	for (auto&& string : file.strings())
	{
		if (string.type == 1030)
		{
			importView(convertOcdString< typename F::Encoding >(file[string]));
			break;
		}
	}
}

void OcdFileImport::importView(const QString& param_string)
{
	const QChar* unicode = param_string.unicode();
	
	bool zoom_ok = false;
	double zoom=1.0, offset_x=0.0, offset_y=0.0;
	
	int i = param_string.indexOf('\t', 0);
	; // skip first word for this entry type
	while (i >= 0)
	{
		int next_i = param_string.indexOf('\t', i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
			case '\t':
				// empty item
				break;
			case 'x':
			{
				offset_x = param_value.toDouble();
				break;
			}
			case 'y':
			{
				offset_y = param_value.toDouble();
				break;
			}
			case 'z':
			{
				zoom = param_value.toDouble(&zoom_ok);
				break;
			}
			default:
				; // nothing
		}
		i = next_i;
	}
	
	if (view)
	{
		view->setCenter(MapCoord(offset_x, -offset_y));
		if (zoom_ok)
		{
			view->setZoom(zoom);
		}
	}
}

template< class S >
void OcdFileImport::setupBaseSymbol(Symbol* symbol, const S& ocd_symbol)
{
	typedef typename S::BaseSymbol BaseSymbol;
	const BaseSymbol& base_symbol = ocd_symbol.base;
	// common fields are name, number, description, helper_symbol, hidden/protected status
	symbol->setName(convertOcdString(base_symbol.description));
	symbol->setNumberComponent(0, base_symbol.number / BaseSymbol::symbol_number_factor);
	symbol->setNumberComponent(1, base_symbol.number % BaseSymbol::symbol_number_factor);
	symbol->setNumberComponent(2, -1);
	symbol->setIsHelperSymbol(false);
	if (base_symbol.status & BaseSymbol::StatusProtected)
		symbol->setProtected(true);
	if (base_symbol.status & BaseSymbol::StatusHidden)
		symbol->setHidden(true);
}

template< class S >
PointSymbol* OcdFileImport::importPointSymbol(const S& ocd_symbol)
{
	OcdImportedPointSymbol* symbol = new OcdImportedPointSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setupPointSymbolPattern(symbol, ocd_symbol.data_size, ocd_symbol.begin_of_elements);
	symbol->setRotatable(ocd_symbol.base.flags & 1);
	return symbol;
}

template< class S >
Symbol* OcdFileImport::importLineSymbol(const S& ocd_symbol)
{
	OcdImportedLineSymbol* line_for_borders = nullptr;
	
	// Import a main line?
	OcdImportedLineSymbol* main_line = nullptr;
	if (ocd_symbol.double_mode == 0 || ocd_symbol.line_width > 0)
	{
		main_line = new OcdImportedLineSymbol();
		line_for_borders = main_line;
		setupBaseSymbol(main_line, ocd_symbol);
		
		// Basic line options
		main_line->line_width = convertLength(ocd_symbol.line_width);
		main_line->color = convertColor(ocd_symbol.line_color);
		
		// Cap and join styles
		switch (ocd_symbol.line_style)
		{
			case S::BevelJoin_FlatCap:
				main_line->join_style = LineSymbol::BevelJoin;
				main_line->cap_style = LineSymbol::FlatCap;
				break;
			case S::RoundJoin_RoundCap:
				main_line->join_style = LineSymbol::RoundJoin;
				main_line->cap_style = LineSymbol::RoundCap;
				break;
			case S::BevelJoin_PointedCap:
				main_line->join_style = LineSymbol::BevelJoin;
				main_line->cap_style = LineSymbol::PointedCap;
				break;
			case S::RoundJoin_PointedCap:
				main_line->join_style = LineSymbol::RoundJoin;
				main_line->cap_style = LineSymbol::PointedCap;
				break;
			case S::MiterJoin_FlatCap:
				main_line->join_style = LineSymbol::MiterJoin;
				main_line->cap_style = LineSymbol::FlatCap;
				break;
			case S::MiterJoin_PointedCap:
				main_line->join_style = LineSymbol::MiterJoin;
				main_line->cap_style = LineSymbol::PointedCap;
				break;
			default:
				addSymbolWarning( main_line,
				  tr("Unsupported line style '%1'.").
				  arg(ocd_symbol.line_style) );
		}
		
		if (main_line->cap_style == LineSymbol::PointedCap)
		{
			int ocd_length = ocd_symbol.dist_from_start;
			if (ocd_symbol.dist_from_start != ocd_symbol.dist_from_end)
			{
				// FIXME: Different lengths for start and end length of pointed line ends are not supported yet, so take the average
				ocd_length = (ocd_symbol.dist_from_start + ocd_symbol.dist_from_end) / 2;
				addSymbolWarning( main_line,
				  tr("Different lengths for pointed caps at begin (%1 mm) and end (%2 mm) are not supported. Using %3 mm.").
				  arg(locale.toString(0.001f * convertLength(ocd_symbol.dist_from_start))).
				  arg(locale.toString(0.001f * convertLength(ocd_symbol.dist_from_end))).
				  arg(locale.toString(0.001f * convertLength(ocd_length))) );
			}
			main_line->pointed_cap_length = convertLength(ocd_length);
			main_line->join_style = LineSymbol::RoundJoin;	// NOTE: while the setting may be different (see what is set in the first place), OC*D always draws round joins if the line cap is pointed!
		}
		
		// Handle the dash pattern
		if (ocd_symbol.main_gap || ocd_symbol.sec_gap)
		{
			if (!ocd_symbol.main_length)
			{
				// Invalid dash pattern
				addSymbolWarning( main_line,
				  tr("The dash pattern cannot be imported correctly.") );
			}
			else if (ocd_symbol.sec_gap && !ocd_symbol.main_gap)
			{
				// Special case main_gap == 0
				main_line->dashed = true;
				main_line->dash_length = convertLength(ocd_symbol.main_length - ocd_symbol.sec_gap);
				main_line->break_length = convertLength(ocd_symbol.sec_gap);
				
				if (ocd_symbol.end_length)
				{
					if (qAbs((qint32)ocd_symbol.main_length - 2*ocd_symbol.end_length) > 1)
					{
						// End length not equal to 0.5 * main length
						addSymbolWarning( main_line,
						  tr("The dash pattern's end length (%1 mm) cannot be imported correctly. Using %2 mm.").
						  arg(locale.toString(0.001f * convertLength(ocd_symbol.end_length))).
						  arg(locale.toString(0.001f * main_line->dash_length)) );
					}
					if (ocd_symbol.end_gap)
					{
						addSymbolWarning( main_line,
						  tr("The dash pattern's end gap (%1 mm) cannot be imported correctly. Using %2 mm.").
						  arg(locale.toString(0.001f * convertLength(ocd_symbol.end_gap))).
						  arg(locale.toString(0.001f * main_line->break_length)) );
					}
				}
			}
			else
			{
				// Standard case
				main_line->dashed = true;
				main_line->dash_length = convertLength(ocd_symbol.main_length);
				main_line->break_length = convertLength(ocd_symbol.main_gap);
				
				if (ocd_symbol.end_length && ocd_symbol.end_length != ocd_symbol.main_length)
				{
					if (ocd_symbol.main_length && 0.75 >= ocd_symbol.end_length / ocd_symbol.main_length)
					{
						// End length max. 75 % of main length
						main_line->half_outer_dashes = true;
					}
					
					if (qAbs((qint32)ocd_symbol.main_length - 2*ocd_symbol.end_length) > 1)
					{
						// End length not equal to 0.5 * main length
						addSymbolWarning( main_line,
						  tr("The dash pattern's end length (%1 mm) cannot be imported correctly. Using %2 mm.").
						  arg(locale.toString(0.001f * convertLength(ocd_symbol.end_length))).
						  arg(locale.toString(0.001f * (main_line->half_outer_dashes ? (main_line->dash_length/2) : main_line->dash_length))) );
					}
				}
				
				if (ocd_symbol.sec_gap)
				{
					main_line->dashes_in_group = 2;
					main_line->in_group_break_length = convertLength(ocd_symbol.sec_gap);
					main_line->dash_length = (main_line->dash_length - main_line->in_group_break_length) / 2;
					
					if (ocd_symbol.end_length && ocd_symbol.end_gap != ocd_symbol.sec_gap)
					{
						addSymbolWarning( main_line,
						  tr("The dash pattern's end gap (%1 mm) cannot be imported correctly. Using %2 mm.").
						  arg(locale.toString(0.001f * convertLength(ocd_symbol.end_gap))).
						  arg(locale.toString(0.001f * main_line->in_group_break_length)) );
					}
				}
			}
		} 
		else
		{
			main_line->segment_length = convertLength(ocd_symbol.main_length);
			main_line->end_length = convertLength(ocd_symbol.end_length);
		}
	}
	
	// Import a 'framing' line?
	OcdImportedLineSymbol* framing_line = nullptr;
	if (ocd_symbol.framing_width > 0)
	{
		framing_line = new OcdImportedLineSymbol();
		if (!line_for_borders)
			line_for_borders = framing_line;
		setupBaseSymbol(framing_line, ocd_symbol);
		
		// Basic line options
		framing_line->line_width = convertLength(ocd_symbol.framing_width);
		framing_line->color = convertColor(ocd_symbol.framing_color);
		
		// Cap and join styles
		switch (ocd_symbol.framing_style)
		{
			case S::BevelJoin_FlatCap:
				framing_line->join_style = LineSymbol::BevelJoin;
				framing_line->cap_style = LineSymbol::FlatCap;
				break;
			case S::RoundJoin_RoundCap:
				framing_line->join_style = LineSymbol::RoundJoin;
				framing_line->cap_style = LineSymbol::RoundCap;
				break;
			case S::MiterJoin_FlatCap:
				framing_line->join_style = LineSymbol::MiterJoin;
				framing_line->cap_style = LineSymbol::FlatCap;
				break;
			default:
				addSymbolWarning( main_line, 
				                  tr("Unsupported framing line style '%1'.").
				                  arg(ocd_symbol.line_style) );
		}
	}
	
	// Import a 'double' line?
	bool has_border_line =
	        (ocd_symbol.double_mode != 0) &&
	        (ocd_symbol.double_left_width > 0 || ocd_symbol.double_right_width > 0);
	OcdImportedLineSymbol *double_line = nullptr;
	if ( has_border_line &&
		(ocd_symbol.double_flags & S::DoubleFillColorOn || (has_border_line && !line_for_borders)))
	{
		double_line = new OcdImportedLineSymbol();
		line_for_borders = double_line;
		setupBaseSymbol(double_line, ocd_symbol);
		
		double_line->line_width = convertLength(ocd_symbol.double_width);
		if (ocd_symbol.double_flags & S::DoubleFillColorOn)
			double_line->color = convertColor(ocd_symbol.double_color);
		else
			double_line->color = nullptr;
		
		double_line->cap_style = LineSymbol::FlatCap;
		double_line->join_style = LineSymbol::MiterJoin;
		
		double_line->segment_length = convertLength(ocd_symbol.main_length);
		double_line->end_length = convertLength(ocd_symbol.end_length);
	}
	
	// Border lines
	if (has_border_line)
	{
		Q_ASSERT(line_for_borders);
		line_for_borders->have_border_lines = true;
		LineSymbolBorder& border = line_for_borders->getBorder();
		LineSymbolBorder& right_border = line_for_borders->getRightBorder();
		
		// Border color and width
		border.color = convertColor(ocd_symbol.double_left_color);
		border.width = convertLength(ocd_symbol.double_left_width);
		border.shift = convertLength(ocd_symbol.double_left_width) / 2 + (convertLength(ocd_symbol.double_width) - line_for_borders->line_width) / 2;
		
		right_border.color = convertColor(ocd_symbol.double_right_color);
		right_border.width = convertLength(ocd_symbol.double_right_width);
		right_border.shift = convertLength(ocd_symbol.double_right_width) / 2 + (convertLength(ocd_symbol.double_width) - line_for_borders->line_width) / 2;
		
		// The borders may be dashed
		if (ocd_symbol.double_gap > 0 && ocd_symbol.double_mode > 1)
		{
			border.dashed = true;
			border.dash_length = convertLength(ocd_symbol.double_length);
			border.break_length = convertLength(ocd_symbol.double_gap);
			
			// If ocd_symbol->dmode == 2, only the left border should be dashed
			if (ocd_symbol.double_mode > 2)
			{
				right_border.dashed = border.dashed;
				right_border.dash_length = border.dash_length;
				right_border.break_length = border.break_length;
			}
		}
	}
	
	// Create point symbols along line; middle ("normal") dash, corners, start, and end.
	OcdImportedLineSymbol* symbol_line = main_line ? main_line : double_line;	// Find the line to attach the symbols to
	if (symbol_line == nullptr)
	{
		main_line = new OcdImportedLineSymbol();
		symbol_line = main_line;
		setupBaseSymbol(main_line, ocd_symbol);
		
		main_line->segment_length = convertLength(ocd_symbol.main_length);
		main_line->end_length = convertLength(ocd_symbol.end_length);
	}
	
	const Ocd::OcdPoint32* coords = reinterpret_cast<const Ocd::OcdPoint32*>(ocd_symbol.begin_of_elements);
	
	symbol_line->mid_symbol = new OcdImportedPointSymbol();
	setupPointSymbolPattern(symbol_line->mid_symbol, ocd_symbol.primary_data_size, ocd_symbol.begin_of_elements);
	symbol_line->mid_symbols_per_spot = ocd_symbol.num_prim_sym;
	symbol_line->mid_symbol_distance = convertLength(ocd_symbol.prim_sym_dist);
	coords += ocd_symbol.primary_data_size;
	
	if (ocd_symbol.secondary_data_size > 0)
	{
		//symbol_line->dash_symbol = importPattern( ocd_symbol->ssnpts, symbolptr);
		coords += ocd_symbol.secondary_data_size;
		addSymbolWarning(symbol_line, tr("Skipped secondary point symbol."));
	}
	if (ocd_symbol.corner_data_size > 0)
	{
		symbol_line->dash_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(symbol_line->dash_symbol, ocd_symbol.corner_data_size, reinterpret_cast<const typename S::Element*>(coords));
		symbol_line->dash_symbol->setName(LineSymbolSettings::tr("Dash symbol"));
		coords += ocd_symbol.corner_data_size;
	}
	if (ocd_symbol.start_data_size > 0)
	{
		symbol_line->start_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(symbol_line->start_symbol, ocd_symbol.start_data_size, reinterpret_cast<const typename S::Element*>(coords));
		symbol_line->start_symbol->setName(LineSymbolSettings::tr("Start symbol"));
		coords += ocd_symbol.start_data_size;
	}
	if (ocd_symbol.end_data_size > 0)
	{
		symbol_line->end_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(symbol_line->end_symbol, ocd_symbol.end_data_size, reinterpret_cast<const typename S::Element*>(coords));
		symbol_line->end_symbol->setName(LineSymbolSettings::tr("End symbol"));
	}
	// FIXME: not really sure how this translates... need test cases
	symbol_line->minimum_mid_symbol_count = 0; //1 + ocd_symbol->smin;
	symbol_line->minimum_mid_symbol_count_when_closed = 0; //1 + ocd_symbol->smin;
	symbol_line->show_at_least_one_symbol = false; // NOTE: this works in a different way than OC*D's 'at least X symbols' setting (per-segment instead of per-object)
	
	// Suppress dash symbol at line ends if both start symbol and end symbol exist,
	// but don't create a warning unless a dash symbol is actually defined
	// and the line symbol is not Mapper's 799 Simple orienteering course.
	if (symbol_line->start_symbol != nullptr && symbol_line->end_symbol != nullptr)
	{
		symbol_line->setSuppressDashSymbolAtLineEnds(true);
		if (symbol_line->dash_symbol && symbol_line->number[0] != 799)
		{
			addSymbolWarning(symbol_line, tr("Suppressing dash symbol at line ends."));
		}
	}
	
	// TODO: taper fields (tmode and tlast)
	
	if (main_line == nullptr && framing_line == nullptr)
		return double_line;
	else if (double_line == nullptr && framing_line == nullptr)
		return main_line;
	else if (main_line == nullptr && double_line == nullptr)
		return framing_line;
	else
	{
		CombinedSymbol* full_line = new CombinedSymbol();
		setupBaseSymbol(full_line, ocd_symbol);
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
		addSymbolWarning(symbol_line, tr("This symbol cannot be saved as a proper OCD symbol again."));
		return full_line;
	}
}

template< class S >
AreaSymbol* OcdFileImport::importAreaSymbol(const S& ocd_symbol, int ocd_version)
{
	OcdImportedAreaSymbol* symbol = new OcdImportedAreaSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	// Basic area symbol fields: minimum_area, color
	symbol->minimum_area = 0;
	symbol->color = ocd_symbol.fill_on ? convertColor(ocd_symbol.fill_color) : nullptr;
	
	symbol->patterns.clear();
	symbol->patterns.reserve(4);
	
	// Hatching
	if (ocd_symbol.hatch_mode != S::HatchNone)
	{
		AreaSymbol::FillPattern pattern;
		pattern.type = AreaSymbol::FillPattern::LinePattern;
		pattern.angle = convertAngle(ocd_symbol.hatch_angle_1);
		pattern.rotatable = true;
		pattern.line_spacing = convertLength(ocd_symbol.hatch_dist);
		pattern.line_offset = 0;
		pattern.line_color = convertColor(ocd_symbol.hatch_color);
		pattern.line_width = convertLength(ocd_symbol.hatch_line_width);
		if (ocd_version <= 8)
		{
			pattern.line_spacing += pattern.line_width;
		}
		symbol->patterns.push_back(pattern);
		
		if (ocd_symbol.hatch_mode == S::HatchCross)
		{
			// Second hatch, same as the first, just a different angle
			pattern.angle = convertAngle(ocd_symbol.hatch_angle_2);
			symbol->patterns.push_back(pattern);
		}
	}
	
	if (ocd_symbol.structure_mode != S::StructureNone)
	{
		AreaSymbol::FillPattern pattern;
		pattern.type = AreaSymbol::FillPattern::PointPattern;
		pattern.angle = convertAngle(ocd_symbol.structure_angle);
		pattern.rotatable = true;
		pattern.point_distance = convertLength(ocd_symbol.structure_width);
		pattern.line_spacing = convertLength(ocd_symbol.structure_height);
		pattern.line_offset = 0;
		pattern.offset_along_line = 0;
		// FIXME: somebody needs to own this symbol and be responsible for deleting it
		// Right now it looks like a potential memory leak
		pattern.point = new OcdImportedPointSymbol();
		setupPointSymbolPattern(pattern.point, ocd_symbol.data_size, ocd_symbol.begin_of_elements);
		
		// OC*D 8 has a "staggered" pattern mode, where successive rows are shifted width/2 relative
		// to each other. We need to simulate this in Mapper with two overlapping patterns, each with
		// twice the height. The second is then offset by width/2, height/2.
		if (ocd_symbol.structure_mode == S::StructureShiftedRows)
		{
			pattern.line_spacing *= 2;
			symbol->patterns.push_back(pattern);
			
			pattern.line_offset = pattern.line_spacing / 2;
			pattern.offset_along_line = pattern.point_distance / 2;
			pattern.point = pattern.point->duplicate()->asPoint();
		}
		symbol->patterns.push_back(pattern);
	}
	
	return symbol;
}

template< class S >
TextSymbol* OcdFileImport::importTextSymbol(const S& ocd_symbol)
{
	OcdImportedTextSymbol* symbol = new OcdImportedTextSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	symbol->font_family = convertOcdString(ocd_symbol.font_name); // FIXME: font mapping?
	symbol->color = convertColor(ocd_symbol.font_color);
	double d_font_size = (0.1 * ocd_symbol.font_size) / 72.0 * 25.4;
	symbol->font_size = qRound(1000 * d_font_size);
	symbol->bold = (ocd_symbol.font_weight>= 550) ? true : false;
	symbol->italic = (ocd_symbol.font_italic) ? true : false;
	symbol->underline = false;
	symbol->paragraph_spacing = convertLength(ocd_symbol.para_spacing);
	symbol->character_spacing = ocd_symbol.char_spacing / 100.0;
	symbol->kerning = false;
	symbol->line_below = ocd_symbol.line_below_on;
	symbol->line_below_color = convertColor(ocd_symbol.line_below_color);
	symbol->line_below_width = convertLength(ocd_symbol.line_below_width);
	symbol->line_below_distance = convertLength(ocd_symbol.line_below_offset);
	symbol->custom_tabs.resize(ocd_symbol.num_tabs);
	for (int i = 0; i < ocd_symbol.num_tabs; ++i)
		symbol->custom_tabs[i] = convertLength(ocd_symbol.tab_pos[i]);
	
	TextObject::HorizontalAlignment halign = TextObject::AlignHCenter;
	int ocd_halign = ocd_symbol.alignment & 3;
	if (ocd_halign == 0) // TODO: Named identifiers, all values
		halign = TextObject::AlignLeft;
	else if (ocd_halign == 1)
		halign = TextObject::AlignHCenter;
	else if (ocd_halign == 2)
		halign = TextObject::AlignRight;
	else if (ocd_halign == 3)
		addSymbolWarning(symbol, tr("Justified alignment is not supported.")); // TODO
	
	text_halign_map[symbol] = halign;
	
	TextObject::VerticalAlignment valign = TextObject::AlignBaseline;
	int ocd_valign = ocd_symbol.alignment & 12;
	if (ocd_valign == 4) // TODO: Named identifiers, all values
		valign = TextObject::AlignVCenter;
	else if (ocd_valign == 8)
		valign = TextObject::AlignTop;
	else if (ocd_valign == 12)
		addSymbolWarning(symbol, tr("Vertical alignment '%1' is not supported.").arg(ocd_symbol.alignment)); // TODO
	
	text_valign_map[symbol] = valign;
	
	if (ocd_symbol.font_weight != 400 && ocd_symbol.font_weight != 700)
	{
		addSymbolWarning(symbol, tr("Ignoring custom weight (%1).").arg(ocd_symbol.font_weight));
	}
	if (ocd_symbol.char_spacing != 0)
	{
		addSymbolWarning(symbol, tr("Custom character spacing may be incorrect."));
	}
	if (ocd_symbol.word_spacing != 100)
	{
		addSymbolWarning(symbol, tr("Ignoring custom word spacing (%1 %).").arg(ocd_symbol.word_spacing));
	}
	if (ocd_symbol.indent_first_line != 0 || ocd_symbol.indent_other_lines != 0)
	{
		addSymbolWarning(symbol, tr("Ignoring custom indents (%1/%2).").arg(ocd_symbol.indent_first_line).arg(ocd_symbol.indent_other_lines));
	}
	
	if (ocd_symbol.framing_mode > 0) // TODO: Identifiers
	{
		symbol->framing = true;
		symbol->framing_color = convertColor(ocd_symbol.framing_color);
		if (ocd_symbol.framing_mode == 1)
		{
			symbol->framing_mode = TextSymbol::ShadowFraming;
			symbol->framing_shadow_x_offset = convertLength(ocd_symbol.framing_offset_x);
			symbol->framing_shadow_y_offset = -1 * convertLength(ocd_symbol.framing_offset_y);
		}
		else if (ocd_symbol.framing_mode == 2)
		{
			symbol->framing_mode = TextSymbol::LineFraming;
			symbol->framing_line_half_width = convertLength(ocd_symbol.framing_line_width);
		}
		else
		{
			addSymbolWarning(symbol, tr("Ignoring text framing (mode %1).").arg(ocd_symbol.framing_mode));
		}
	}
	
	symbol->updateQFont();
	
	// Convert line spacing
	double absolute_line_spacing = d_font_size * 0.01 * ocd_symbol.line_spacing;
	symbol->line_spacing = absolute_line_spacing / (symbol->getFontMetrics().lineSpacing() / symbol->calculateInternalScaling());
	
	return symbol;
}

template< class S >
TextSymbol* OcdFileImport::importLineTextSymbol(const S& ocd_symbol)
{
	OcdImportedTextSymbol* symbol = new OcdImportedTextSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	symbol->font_family = convertOcdString(ocd_symbol.font_name); // FIXME: font mapping?
	symbol->color = convertColor(ocd_symbol.font_color);
	double d_font_size = (0.1 * ocd_symbol.font_size) / 72.0 * 25.4;
	symbol->font_size = qRound(1000 * d_font_size);
	symbol->bold = (ocd_symbol.font_weight>= 550) ? true : false;
	symbol->italic = (ocd_symbol.font_italic) ? true : false;
	symbol->underline = false;
	symbol->character_spacing = ocd_symbol.char_spacing / 100.0;
	symbol->kerning = false;
	symbol->line_below = false;
	symbol->custom_tabs.resize(0);
	
	TextObject::HorizontalAlignment halign = TextObject::AlignHCenter;
	int ocd_halign = ocd_symbol.alignment & 3;
	if (ocd_halign == 0) // TODO: Named identifiers, all values
		halign = TextObject::AlignLeft;
	else if (ocd_halign == 1)
		halign = TextObject::AlignHCenter;
	else if (ocd_halign == 2)
		halign = TextObject::AlignRight;
	else if (ocd_halign == 3)
		addSymbolWarning(symbol, tr("Justified alignment is not supported.")); // TODO
	
	text_halign_map[symbol] = halign;
	
	TextObject::VerticalAlignment valign = TextObject::AlignBaseline;
	int ocd_valign = ocd_symbol.alignment & 12;
	if (ocd_valign == 4) // TODO: Named identifiers, all values
		valign = TextObject::AlignVCenter;
	else if (ocd_valign == 8)
		valign = TextObject::AlignTop;
	else if (ocd_valign == 12)
		addSymbolWarning(symbol, tr("Vertical alignment '%1' is not supported.").arg(ocd_symbol.alignment)); // TODO
	
	text_valign_map[symbol] = valign;
	
	if (ocd_symbol.font_weight != 400 && ocd_symbol.font_weight != 700)
	{
		addSymbolWarning(symbol, tr("Ignoring custom weight (%1).").arg(ocd_symbol.font_weight));
	}
	if (ocd_symbol.char_spacing != 0)
	{
		addSymbolWarning(symbol, tr("Custom character spacing may be incorrect."));
	}
	if (ocd_symbol.word_spacing != 100)
	{
		addSymbolWarning(symbol, tr("Ignoring custom word spacing (%1 %).").arg(ocd_symbol.word_spacing));
	}
	
	if (ocd_symbol.framing_mode > 0) // TODO: Identifiers
	{
		symbol->framing = true;
		symbol->framing_color = convertColor(ocd_symbol.framing_color);
		if (ocd_symbol.framing_mode == 1)
		{
			symbol->framing_mode = TextSymbol::ShadowFraming;
			symbol->framing_shadow_x_offset = convertLength(ocd_symbol.framing_offset_x);
			symbol->framing_shadow_y_offset = -1 * convertLength(ocd_symbol.framing_offset_y);
		}
		else if (ocd_symbol.framing_mode == 2)
		{
			symbol->framing_mode = TextSymbol::LineFraming;
			symbol->framing_line_half_width = convertLength(ocd_symbol.framing_line_width);
		}
		else
		{
			addSymbolWarning(symbol, tr("Ignoring text framing (mode %1).").arg(ocd_symbol.framing_mode));
		}
	}
	
	symbol->updateQFont();
	
	addSymbolWarning(symbol, tr("Line text symbols are not yet supported. Marking the symbol as hidden."));
	symbol->setHidden(true);
	
	return symbol;
}

template< class S >
LineSymbol* OcdFileImport::importRectangleSymbol(const S& ocd_symbol)
{
	OcdImportedLineSymbol* symbol = new OcdImportedLineSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	symbol->line_width = convertLength(ocd_symbol.line_width);
	symbol->color = convertColor(ocd_symbol.line_color);
	symbol->cap_style = LineSymbol::FlatCap;
	symbol->join_style = LineSymbol::RoundJoin;
	
	RectangleInfo rect;
	rect.border_line = symbol;
	rect.corner_radius = 0.001 * convertLength(ocd_symbol.corner_radius);
	rect.has_grid = ocd_symbol.grid_flags & 1;
	
	if (rect.has_grid)
	{
		OcdImportedLineSymbol* inner_line = new OcdImportedLineSymbol();
		setupBaseSymbol(inner_line, ocd_symbol);
		inner_line->setNumberComponent(2, 1);  // TODO: Dynamic
		inner_line->line_width = qRound(1000 * 0.15);
		inner_line->color = symbol->color;
		map->addSymbol(inner_line, map->getNumSymbols());
		
		OcdImportedTextSymbol* text = new OcdImportedTextSymbol();
		setupBaseSymbol(text, ocd_symbol);
		text->setNumberComponent(2, 2);  // TODO: Dynamic
		text->font_family = "Arial";
		text->font_size = qRound(1000 * (15 / 72.0 * 25.4));
		text->color = symbol->color;
		text->bold = true;
		text->updateQFont();
		map->addSymbol(text, map->getNumSymbols());
		
		rect.inner_line = inner_line;
		rect.text = text;
		rect.number_from_bottom = ocd_symbol.grid_flags & 2;
		rect.cell_width = 0.001 * convertLength(ocd_symbol.cell_width);
		rect.cell_height = 0.001 * convertLength(ocd_symbol.cell_height);
		rect.unnumbered_cells = ocd_symbol.unnumbered_cells;
		rect.unnumbered_text = convertOcdString(ocd_symbol.unnumbered_text);
	}
	rectangle_info.insert(ocd_symbol.base.number, rect);
	
	return symbol;
}

template< > // OCD::FormatV8
int OcdFileImport::circleRadius(const Ocd::PointSymbolElementV8* element) const
{
	return element->diameter / 2 - element->line_width;
}

template< class E >
int OcdFileImport::circleRadius(const E* element) const
{
	return (element->diameter - element->line_width) / 2;
}

template< class E >
void OcdFileImport::setupPointSymbolPattern(PointSymbol* symbol, std::size_t data_size, const E* elements)
{
	Q_ASSERT(symbol != nullptr);
	
	symbol->setRotatable(true);
	bool base_symbol_used = false;
	
	for (std::size_t i = 0; i < data_size; i += 2)
	{
		const E* element = reinterpret_cast<const E*>(&reinterpret_cast<const Ocd::OcdPoint32*>(elements)[i]);
		const Ocd::OcdPoint32* const coords = reinterpret_cast<const Ocd::OcdPoint32*>(elements) + i + 2;
		switch (element->type)
		{
			case E::TypeDot:
				if (element->diameter > 0)
				{
					bool can_use_base_symbol = (!base_symbol_used && (!element->num_coords || (!coords[0].x && !coords[0].y)));
					PointSymbol* working_symbol = can_use_base_symbol ? symbol : new PointSymbol();
					working_symbol->setInnerColor(convertColor(element->color));
					working_symbol->setInnerRadius(convertLength(element->diameter) / 2);
					working_symbol->setOuterColor(nullptr);
					working_symbol->setOuterWidth(0);
					if (can_use_base_symbol)
					{
						base_symbol_used = true;
					}
					else
					{
						working_symbol->setRotatable(false);
						PointObject* element_object = new PointObject(working_symbol);
						if (element->num_coords)
						{
							const MapCoord coord = convertOcdPoint(coords[0]);
							element_object->setPosition(coord.rawX(), coord.rawY());
						}
						symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
					}
				}
				break;
			case E::TypeCircle:
			{
				int element_radius = circleRadius(element);
				if (element_radius > 0)
				{
					bool can_use_base_symbol = (!base_symbol_used && (!element->num_coords || (!coords[0].x && !coords[0].y)));
					PointSymbol* working_symbol = can_use_base_symbol ? symbol : new PointSymbol();
					working_symbol->setInnerColor(nullptr);
					working_symbol->setInnerRadius(convertLength(element_radius));
					working_symbol->setOuterColor(convertColor(element->color));
					working_symbol->setOuterWidth(convertLength(element->line_width));
					if (can_use_base_symbol)
					{
						base_symbol_used = true;
					}
					else
					{
						working_symbol->setRotatable(false);
						PointObject* element_object = new PointObject(working_symbol);
						if (element->num_coords)
						{
							const MapCoord coord = convertOcdPoint(coords[0]);
							element_object->setPosition(coord.rawX(), coord.rawY());
						}
						symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
					}
				}
				break;
			}
			case E::TypeLine:
				{
					OcdImportedLineSymbol* element_symbol = new OcdImportedLineSymbol();
					element_symbol->line_width = convertLength(element->line_width);
					element_symbol->color = convertColor(element->color);
					OcdImportedPathObject* element_object = new OcdImportedPathObject(element_symbol);
					fillPathCoords(element_object, false, element->num_coords, coords);
					element_object->recalculateParts();
					symbol->addElement(symbol->getNumElements(), element_object, element_symbol);
				}
				break;
			case E::TypeArea:
				{
					OcdImportedAreaSymbol* element_symbol = new OcdImportedAreaSymbol();
					element_symbol->color = convertColor(element->color);
					OcdImportedPathObject* element_object = new OcdImportedPathObject(element_symbol);
					fillPathCoords(element_object, true, element->num_coords, coords);
					element_object->recalculateParts();
					symbol->addElement(symbol->getNumElements(), element_object, element_symbol);
				}
				break;
			default:
				; // TODO: not-supported warning
		}
		i += element->num_coords;
	}
}

template< class O >
Object* OcdFileImport::importObject(const O& ocd_object, MapPart* part)
{
	Symbol* symbol = nullptr;
	if (ocd_object.symbol >= 0)
	{
		symbol = symbol_index[ocd_object.symbol];
	}
	
	if (!symbol)
	{
		switch (ocd_object.type)
		{
		case 1:
			symbol = map->getUndefinedPoint();
			break;
		case 2:
		case 3:
			symbol = map->getUndefinedLine();
			break;
		case 4:
		case 5:
			symbol = map->getUndefinedText();
			break;
		default:
			addWarning(tr("Unable to load object"));
			qDebug() << "Undefined object type" << ocd_object.type << " for object of symbol" << ocd_object.symbol;
			return nullptr;
		}
	}
		
	if (symbol->getType() == Symbol::Line && rectangle_info.contains(ocd_object.symbol))
	{
		Object* object = importRectangleObject(ocd_object, part, rectangle_info[ocd_object.symbol]);
		if (!object)
			addWarning(tr("Unable to import rectangle object"));
		return object;
	}
	
	if (symbol->getType() == Symbol::Point)
	{
		PointObject* p = new PointObject();
		p->setSymbol(symbol, true);
		
		// extra properties: rotation
		PointSymbol* point_symbol = reinterpret_cast<PointSymbol*>(symbol);
		if (point_symbol->isRotatable())
		{
			p->setRotation(convertAngle(ocd_object.angle));
		}
		else if (ocd_object.angle != 0)
		{
			if (!point_symbol->isSymmetrical())
			{
				point_symbol->setRotatable(true);
				p->setRotation(convertAngle(ocd_object.angle));
			}
		}
		
		const MapCoord pos = convertOcdPoint(ocd_object.coords[0]);
		p->setPosition(pos.rawX(), pos.rawY());
		
		p->setMap(map);
		return p;
	}
	else if (symbol->getType() == Symbol::Text)
	{
		TextObject *t = new TextObject(symbol);
		t->setText(getObjectText(ocd_object));
		t->setRotation(convertAngle(ocd_object.angle));
		t->setHorizontalAlignment(text_halign_map.value(symbol));
		// Vertical alignment is set in fillTextPathCoords().
		
		// Text objects need special path translation
		if (!fillTextPathCoords(t, reinterpret_cast<TextSymbol*>(symbol), ocd_object.num_items, (Ocd::OcdPoint32 *)ocd_object.coords))
		{
			addWarning(tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
			           .arg(ocd_object.num_items).arg(t->getText()));
			delete t;
			return nullptr;
		}
		t->setMap(map);
		return t;
	}
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined)
	{
		OcdImportedPathObject *p = new OcdImportedPathObject(symbol);
		p->setPatternRotation(convertAngle(ocd_object.angle));
		
		// Normal path
		fillPathCoords(p, symbol->getType() == Symbol::Area, ocd_object.num_items, (Ocd::OcdPoint32*)ocd_object.coords);
		p->recalculateParts();
		p->setMap(map);
		return p;
	}
	
	return nullptr;
}

template< >
inline
QString OcdFileImport::getObjectText< struct Ocd::ObjectV8 >(const Ocd::ObjectV8& ocd_object) const
{
	QString object_text;
	if (ocd_object.unicode)
	{
		object_text = convertOcdString((const QChar*)(ocd_object.coords + ocd_object.num_items));
	}
	else
	{
		const size_t len = sizeof(Ocd::OcdPoint32) * ocd_object.num_text;
		object_text = convertOcdString<Ocd::Custom8BitEncoding>((const char*)(ocd_object.coords + ocd_object.num_items), len);
	}
	
	// Remove leading "\r\n"
	if (object_text.startsWith("\r\n"))
	{
		object_text.remove(0, 2);
	}
	
	return object_text;
}

template< class O >
inline
QString OcdFileImport::getObjectText(const O& ocd_object) const
{
	return QString((const QChar *)(ocd_object.coords + ocd_object.num_items));
}

template< class O >
Object* OcdFileImport::importRectangleObject(const O& ocd_object, MapPart* part, const OcdFileImport::RectangleInfo& rect)
{
	if (ocd_object.num_items != 4)
	{
		qDebug() << "importRectangleObject called with num_items =" << ocd_object.num_items << "for object of symbol" << ocd_object.symbol;
		if (ocd_object.num_items != 5)  // 5 coords are handled like 4 coords now
			return nullptr;
	}
	
	// Convert corner points
	MapCoord bottom_left = convertOcdPoint(ocd_object.coords[0]);
	MapCoord bottom_right = convertOcdPoint(ocd_object.coords[1]);
	MapCoord top_right = convertOcdPoint(ocd_object.coords[2]);
	MapCoord top_left = convertOcdPoint(ocd_object.coords[3]);
	
	MapCoordF top_left_f = MapCoordF(top_left);
	MapCoordF top_right_f = MapCoordF(top_right);
	MapCoordF bottom_left_f = MapCoordF(bottom_left);
	MapCoordF bottom_right_f = MapCoordF(bottom_right);
	MapCoordF right = MapCoordF(top_right.xd() - top_left.xd(), top_right.yd() - top_left.yd());
	double angle = right.angle();
	MapCoordF down = MapCoordF(bottom_left.xd() - top_left.xd(), bottom_left.yd() - top_left.yd());
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
			part->addObject(path, part->getNumObjects());
		}
		for (int y = 1; y < num_cells_y; ++y)
		{
			coords[0] = MapCoord(top_left_f + y * cell_height * down);
			coords[1] = MapCoord(top_right_f + y * cell_height * down);
			
			PathObject *path = new PathObject(rect.inner_line, coords, map);
			part->addObject(path, part->getNumObjects());
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
					part->addObject(object, part->getNumObjects());
					
					//pts[0].Y -= rectinfo.gridText.FontAscent - rectinfo.gridText.FontEmHeight;
				}
			}
		}
	}
	
	return border_path;
}

void OcdFileImport::setPathHolePoint(OcdImportedPathObject *object, int pos)
{
	// Look for curve start points before the current point and apply hole point only if no such point is there.
	// This prevents hole points in the middle of a curve caused by incorrect map objects.
	if (pos >= 1 && object->coords[pos].isCurveStart())
		; //object->coords[i-1].setHolePoint(true);
	else if (pos >= 2 && object->coords[pos-1].isCurveStart())
		; //object->coords[i-2].setHolePoint(true);
	else if (pos >= 3 && object->coords[pos-2].isCurveStart())
		; //object->coords[i-3].setHolePoint(true);
	else if (pos > 0) // Don't start with hole point.
		object->coords[pos].setHolePoint(true);
}

void OcdFileImport::setPointFlags(OcdImportedPathObject* object, quint16 pos, bool is_area, const Ocd::OcdPoint32& ocd_point)
{
	// We can support CurveStart, HolePoint, DashPoint.
	// CurveStart needs to be applied to the main point though, not the control point, and
	// hole points need to bet set as the last point of a part of an area object instead of the first point of the next part
	if (ocd_point.x & Ocd::OcdPoint32::FlagCtl1 && pos > 0)
		object->coords[pos-1].setCurveStart(true);
	if ((ocd_point.y & Ocd::OcdPoint32::FlagDash) || (ocd_point.y & Ocd::OcdPoint32::FlagCorner))
		object->coords[pos].setDashPoint(true);
	if (ocd_point.y & Ocd::OcdPoint32::FlagHole)
		setPathHolePoint(object, is_area ? (pos - 1) : pos);
}

/** Translates the OC*D path given in the last two arguments into an Object.
 */
void OcdFileImport::fillPathCoords(OcdImportedPathObject *object, bool is_area, quint16 num_points, const Ocd::OcdPoint32* ocd_points)
{
	object->coords.resize(num_points);
	for (int i = 0; i < num_points; i++)
	{
		object->coords[i] = convertOcdPoint(ocd_points[i]);
		setPointFlags(object, i, is_area, ocd_points[i]);
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
bool OcdFileImport::fillTextPathCoords(TextObject *object, TextSymbol *symbol, quint16 npts, const Ocd::OcdPoint32 *ocd_points)
{
    // text objects either have 1 point (free anchor) or 2 (midpoint/size)
    // OCAD appears to always have 5 or 4 points (possible single anchor, then 4 corner coordinates going clockwise from anchor).
    if (npts == 0) return false;
	
	if (npts == 4)
	{
		// Box text
		MapCoord bottom_left = convertOcdPoint(ocd_points[0]);
		MapCoord top_right = convertOcdPoint(ocd_points[2]);
		MapCoord top_left = convertOcdPoint(ocd_points[3]);
		
		// According to Purple Pen source code: OC*D adds an extra internal leading (incorrectly).
		QFontMetricsF metrics = symbol->getFontMetrics();
		double top_adjust = -symbol->getFontSize() + (metrics.ascent() + metrics.descent() + 0.5) / symbol->calculateInternalScaling();
		
		MapCoordF adjust_vector = MapCoordF(top_adjust * sin(object->getRotation()), top_adjust * cos(object->getRotation()));
		top_left = MapCoord(top_left.xd() + adjust_vector.x(), top_left.yd() + adjust_vector.y());
		top_right = MapCoord(top_right.xd() + adjust_vector.x(), top_right.yd() + adjust_vector.y());
		
		object->setBox((bottom_left.rawX() + top_right.rawX()) / 2, (bottom_left.rawY() + top_right.rawY()) / 2,
					   top_left.distanceTo(top_right), top_left.distanceTo(bottom_left));
		object->setVerticalAlignment(TextObject::AlignTop);
	}
	else
	{
		// Single anchor text
		if (npts != 5)
			addWarning(tr("Trying to import a text object with unknown coordinate format"));
		
		// anchor point
		MapCoord coord = convertOcdPoint(ocd_points[0]);
		object->setAnchorPosition(coord.rawX(), coord.rawY());
		object->setVerticalAlignment(text_valign_map.value(symbol));
	}
	
	return true;
}

void OcdFileImport::import(bool load_symbols_only)
{
	Q_ASSERT(buffer.isEmpty());
	
	buffer.clear();
	buffer.append(stream->readAll());
	if (buffer.isEmpty())
		throw FileFormatException(Importer::tr("Could not read file: %1").arg(stream->errorString()));
	
	if (buffer.size() < (int)sizeof(Ocd::FormatGeneric::FileHeader))
		throw FileFormatException(Importer::tr("Could not read file: %1").arg(tr("Invalid data.")));
	
	OcdFile< Ocd::FormatGeneric > generic_file(buffer);
	if (generic_file.header()->vendor_mark != 0x0cad) // This also tests correct endianess...
		throw FileFormatException(Importer::tr("Could not read file: %1").arg(tr("Invalid data.")));
	
	int version = generic_file.header()->version;
	switch (version)
	{
		case 6:
		case 7:
			// We keep the following existing translation just in case
			// we add support for version 6/7 to the new importer.
			/* addWarning( */
				(void)tr("Untested file importer for format: OCD %1").arg(version)
			/* ) */;
			importImplementation< Ocd::FormatLegacyImporter >(load_symbols_only);
			break;
		case 8:
			if (Settings::getInstance().getSetting(Settings::General_NewOcd8Implementation).toBool())
				importImplementation< Ocd::FormatV8 >(load_symbols_only);
			else
				importImplementation< Ocd::FormatLegacyImporter >(load_symbols_only);
			break;
		case 9:
			importImplementation< Ocd::FormatV9 >(load_symbols_only);
			break;
		case 10:
			importImplementation< Ocd::FormatV10 >(load_symbols_only);
			break;
		case 11:
			importImplementation< Ocd::FormatV11 >(load_symbols_only);
			break;
		default:
			throw FileFormatException(
			  Importer::tr("Could not read file: %1").
			  arg(tr("OCD files of version %1 are not supported!").arg(version))
			);
	}
}

void OcdFileImport::finishImport()
{
	if (delegate)
	{
		// The current warnings and actions are already propagated.
		std::size_t warnings_size = delegate->warnings().size();
		std::size_t actions_size = delegate->actions().size();
		
		delegate->finishImport();
		
		// Propagate new warnings and actions from the delegate to this importer.
		std::for_each(begin(delegate->warnings()) + warnings_size, end(delegate->warnings()), [this](const QString& w) { addWarning(w); });
		std::for_each(begin(delegate->actions()) + actions_size, end(delegate->actions()), [this](const ImportAction& a) { addAction(a); });
	}
}
