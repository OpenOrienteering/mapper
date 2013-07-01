/*
 *    Copyright 2013 Kai Pastor
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

#include <QDebug>
#include <QBuffer>

#include "ocd_types_v9.h"
#include "ocd_types_v11.h"
#include "../map.h"
#include "../georeferencing.h"
#include "../core/map_color.h"
#include "../symbol_area.h"
#include "../symbol_combined.h"
#include "../symbol_line.h"
#include "../symbol_point.h"
#include "../symbol_text.h"
#include "../object_text.h"
#include "../file_format_ocad8.h"
#include "../util.h"


// ### OcdFileFormat ###

OcdFileFormat::OcdFileFormat()
 : FileFormat(MapFile, "OCD", ImportExport::tr("OCAD"), "ocd", ImportSupported)
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

Importer* OcdFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException)
{
	return new OcdFileImport(stream, map, view);
}



// ### OcdFileImport ###

OcdFileImport::OcdFileImport(QIODevice* stream, Map* map, MapView* view) : Importer(stream, map, view)
{
    local_8bit = QTextCodec::codecForName("Windows-1252");
}

OcdFileImport::~OcdFileImport()
{
}

void OcdFileImport::setLocal8BitEncoding(const char *encoding) {
    local_8bit = QTextCodec::codecForName(encoding);
}

void OcdFileImport::addSymbolWarning(LineSymbol* symbol, const QString& warning)
{
	addWarning( tr("In line symbol %1 '%2': %3").
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
void OcdFileImport::importImplementation< Ocd::FormatV8 >(bool load_symbols_only) throw (FileFormatException)
{
	QBuffer new_stream(&buffer);
	new_stream.open(QIODevice::ReadOnly);
	QScopedPointer<Importer> importer(OCAD8FileFormat().createImporter(&new_stream, map, view));
	importer->doImport(load_symbols_only);
}

template< class T >
void OcdFileImport::importImplementation(bool load_symbols_only) throw (FileFormatException)
{
	OcdFile< T > file(buffer);
	importGeoreferencing< T >(file);
	importColors< T >(file);
	importSymbols< T >(file);
	if (!load_symbols_only)
	{
		importObjects< T >(file);
	}
}

template< class T >
void OcdFileImport::importGeoreferencing(const OcdFile< T >& file) throw (FileFormatException)
{
	for (typename OcdFile<T>::StringIndex::iterator it = file.strings().begin(); it != file.strings().end(); ++it)
	{
		if (it->type != 1039)
			continue;
		
		Georeferencing georef;
		QPointF proj_ref_point;
		bool x_ok = false, y_ok = false;
		
		QByteArray data = file[it];
		int i = data.indexOf('\t', 0);
		; // skip first word for this entry type
		while (i >= 0)
		{
			int next_i = data.indexOf('\t', i+1);
			int len = (next_i > 0 ? next_i : data.length()) - i - 2;
			bool ok;
			switch (data[i+1])
			{
				case 'm':
				{
					int scale = data.mid(i + 2, len).toInt(&ok);
					if (ok) georef.setScaleDenominator(scale);
					break;
				}
				case 'a':
				{
					double angle = data.mid(i + 2, len).toDouble(&ok);
					if (ok && qAbs(angle) >= 0.01) georef.setGrivation(angle);
					break;
				}
				case 'x':
				{
					proj_ref_point.setX(data.mid(i + 2, len).toDouble(&x_ok));
					break;
				}
				case 'y':
				{
					proj_ref_point.setY(data.mid(i + 2, len).toDouble(&y_ok));
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
		return; // for-loop
	}
}

template< class T >
void OcdFileImport::importColors(const OcdFile< T >& file) throw (FileFormatException)
{
	int current_color = 0;
	for (typename OcdFile<T>::StringIndex::iterator it = file.strings().begin(); it != file.strings().end(); ++it)
	{
		if (it->type != 9)
			continue;
		
		int number;
		bool number_ok = false;
		MapColorCmyk cmyk;
		bool overprinting = false;
		float opacity = 1.0f;
		
		QByteArray data = file[it];
		int i = data.indexOf('\t', 0);
		QByteArray name = data.mid(0, qMax(-1, i));
		while (i >= 0)
		{
			float f_value;
			int i_value;
			bool ok;
			int next_i = data.indexOf('\t', i+1);
			int len = (next_i > 0 ? next_i : data.length()) - i - 2;
			switch (data[i+1])
			{
				case 'n':
					number = data.mid(i + 2, len).toInt(&number_ok);
					break;
				case 'c':
					f_value = data.mid(i + 2, len).toFloat(&ok);
					if (ok && f_value >= 0 && f_value <= 100)
						cmyk.c = 0.01f * f_value;
					break;
				case 'm':
					f_value = data.mid(i + 2, len).toFloat(&ok);
					if (ok && f_value >= 0 && f_value <= 100)
						cmyk.m = 0.01f * f_value;
					break;
				case 'y':
					f_value = data.mid(i + 2, len).toFloat(&ok);
					if (ok && f_value >= 0 && f_value <= 100)
						cmyk.y = 0.01f * f_value;
					break;
				case 'k':
					f_value = data.mid(i + 2, len).toFloat(&ok);
					if (ok && f_value >= 0 && f_value <= 100)
						cmyk.k = 0.01f * f_value;
					break;
				case 'o':
					i_value = data.mid(i + 2, len).toInt(&ok);
					if (ok)
						overprinting = i_value;
					break;
				case 't':
					f_value = data.mid(i + 2, len).toFloat(&ok);
					if (ok && f_value >= 0.f && f_value <= 100.f)
						opacity = 0.01f * f_value;
					break;
				default:
					; // nothing
			}
			i = next_i;
		}
		if (number_ok)
		{
			MapColor* color = new MapColor(current_color++);
			color->setName(name);
			color->setCmyk(cmyk);
			color->setKnockout(!overprinting);
			color->setOpacity(opacity);
			map->addColor(color, map->getNumColors());
			color_index[number] = color;
		}
	}
	addWarning(tr("Spot color information was ignored."));
}

template< class T >
void OcdFileImport::importSymbols(const OcdFile< T >& file) throw (FileFormatException)
{
	for (typename OcdFile<T>::SymbolIndex::iterator it = file.symbols().begin(); it != file.symbols().end(); ++it)
	{
		// When extra symbols are created, we want to insert the main symbol
		// before them, i.e. at pos.
		int pos = map->getNumSymbols();
		Symbol* symbol = NULL;
		switch (it->type)
		{
			case T::TypePoint:
				symbol = importPointSymbol((const typename T::PointSymbol&)*it);
				break;
			case T::TypeLine:
				symbol = importLineSymbol((const typename T::LineSymbol&)*it);
				break;
			case T::TypeArea:
				symbol = importAreaSymbol((const typename T::AreaSymbol&)*it);
				break;
			case T::TypeText:
				symbol = importTextSymbol((const typename T::TextSymbol&)*it);
				break;
			case T::TypeRectangle:
				symbol = importRectangleSymbol((const typename T::RectangleSymbol&)*it);
				break;
			case T::TypeLineText:
// 				symbol = importLineTextSymbol((const typename T::LineTextSymbol&)*it);
// 				break;
				; // fall through
			default:
				addWarning(tr("Unable to import symbol %1.%2 \"%3\": %4") .
				           arg(it->number / T::BaseSymbol::symbol_number_factor) .
				           arg(it->number % T::BaseSymbol::symbol_number_factor) .
				           arg(convertOcdString(it->description)).
				           arg(tr("Unsupported type \"%1\".").arg(it->type)) );
				continue;
		}
		
		map->addSymbol(symbol, pos);
		symbol_index[it->number] = symbol;
	}
}

template< class T >
void OcdFileImport::importObjects(const OcdFile< T >& file) throw (FileFormatException)
{
	MapPart* part = map->getCurrentPart();
	Q_ASSERT(part);
	
	for (typename OcdFile<T>::ObjectIndex::iterator it = file.objects().begin(); it != file.objects().end(); ++it)
	{
		Object* object = importObject(file[it], part);
		if (object != NULL)
		{
			part->addObject(object, part->getNumObjects());
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
	return symbol;
}

template< class S >
Symbol* OcdFileImport::importLineSymbol(const S& ocd_symbol)
{
	OcdImportedLineSymbol* line_for_borders = NULL;
	
	// Import a main line?
	OcdImportedLineSymbol* main_line = NULL;
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
	OcdImportedLineSymbol* framing_line = NULL;
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
	bool has_border_line = ocd_symbol.double_left_width > 0 || ocd_symbol.double_right_width > 0;
	OcdImportedLineSymbol *double_line = NULL;
	if ( ocd_symbol.double_mode != 0 &&
		(ocd_symbol.double_flags & S::DoubleFillColorOn || (has_border_line && !line_for_borders)))
	{
		double_line = new OcdImportedLineSymbol();
		line_for_borders = double_line;
		setupBaseSymbol(double_line, ocd_symbol);
		
		double_line->line_width = convertLength(ocd_symbol.double_width);
		if (ocd_symbol.double_flags & S::DoubleFillColorOn)
			double_line->color = convertColor(ocd_symbol.double_color);
		else
			double_line->color = NULL;
		
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
	if (symbol_line == NULL)
	{
		main_line = new OcdImportedLineSymbol();
		symbol_line = main_line;
		setupBaseSymbol(main_line, ocd_symbol);
		
		main_line->segment_length = convertLength(ocd_symbol.main_length);
		main_line->end_length = convertLength(ocd_symbol.end_length);
	}
	
	typedef typename S::Element::PointCoord PointCoord;
	const PointCoord* coords = reinterpret_cast<const PointCoord*>(ocd_symbol.begin_of_elements);
	
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
	if (symbol_line->start_symbol != NULL && symbol_line->end_symbol != NULL)
	{
		symbol_line->setSuppressDashSymbolAtLineEnds(true);
		if (symbol_line->dash_symbol && symbol_line->number[0] != 799)
		{
			addSymbolWarning(symbol_line, tr("Suppressing dash symbol at line ends."));
		}
	}
	
	// TODO: taper fields (tmode and tlast)
	
	if (main_line == NULL && framing_line == NULL)
		return double_line;
	else if (double_line == NULL && framing_line == NULL)
		return main_line;
	else if (main_line == NULL && double_line == NULL)
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
		return full_line;
	}
}

template< class S >
AreaSymbol* OcdFileImport::importAreaSymbol(const S& ocd_symbol)
{
	OcdImportedAreaSymbol* symbol = new OcdImportedAreaSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	// Basic area symbol fields: minimum_area, color
	symbol->minimum_area = 0;
	symbol->color = ocd_symbol.fill_on ? convertColor(ocd_symbol.fill_color) : NULL;
	
	symbol->patterns.clear();
	symbol->patterns.reserve(4);
	
	// Hatching
	if (ocd_symbol.hatch_mode != S::HatchNone)
	{
		AreaSymbol::FillPattern pattern;
		pattern.type = AreaSymbol::FillPattern::LinePattern;
		pattern.angle = convertAngle(ocd_symbol.hatch_angle_1);
		pattern.rotatable = true;
		pattern.line_spacing = convertLength(ocd_symbol.hatch_dist + ocd_symbol.hatch_line_width);
		pattern.line_offset = 0;
		pattern.line_color = convertColor(ocd_symbol.hatch_color);
		pattern.line_width = convertLength(ocd_symbol.hatch_line_width);
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
	
	int halign = (int)TextObject::AlignHCenter;
	if (ocd_symbol.alignment == 0) // TODO: Named identifiers, all values
		halign = (int)TextObject::AlignLeft;
	else if (ocd_symbol.alignment == 1)
		halign = (int)TextObject::AlignHCenter;
	else if (ocd_symbol.alignment == 2)
		halign = (int)TextObject::AlignRight;
	else if (ocd_symbol.alignment == 3)
	{
		// TODO: implement justified alignment
		addWarning(tr("During import of text symbol %1: ignoring justified alignment").arg(0.1 * ocd_symbol.base.number));
	}
	text_halign_map[symbol] = halign;

	if (ocd_symbol.font_weight != 400 && ocd_symbol.font_weight != 700)
	{
		addWarning(tr("During import of text symbol %1: ignoring custom weight (%2)")
		               .arg(0.1 * ocd_symbol.base.number).arg(ocd_symbol.font_weight));
	}
	if (ocd_symbol.char_spacing != 0)
	{
		addWarning(tr("During import of text symbol %1: custom character spacing is set, its implementation does not match OCAD's behavior yet")
		.arg(0.1 * ocd_symbol.base.number));
	}
	if (ocd_symbol.word_spacing != 100)
	{
		addWarning(tr("During import of text symbol %1: ignoring custom word spacing (%2%)")
		               .arg(0.1 * ocd_symbol.base.number).arg(ocd_symbol.word_spacing));
	}
	if (ocd_symbol.indent_first_line != 0 || ocd_symbol.indent_other_lines != 0)
	{
		addWarning(tr("During import of text symbol %1: ignoring custom indents (%2/%3)")
		               .arg(0.1 * ocd_symbol.base.number).arg(ocd_symbol.indent_first_line).arg(ocd_symbol.indent_other_lines));
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
			addWarning(tr("During import of text symbol %1: ignoring text framing (mode %2)")
			.arg(0.1 * ocd_symbol.base.number).arg(ocd_symbol.framing_mode));
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
	// TODO
	TextSymbol* symbol = new TextSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
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

template< class E >
void OcdFileImport::setupPointSymbolPattern(PointSymbol* symbol, std::size_t data_size, const E* elements)
{
	Q_ASSERT(symbol != NULL);
	
	symbol->setRotatable(true);
	bool base_symbol_used = false;
	
	const typename E::PointCoord* const coords = reinterpret_cast<const typename E::PointCoord*>(elements);
	for (std::size_t i = 0; i < data_size; i += 2)
	{
		const E* element = reinterpret_cast<const E*>(&coords[i]);
		switch (element->type)
		{
			case E::TypeDot:
				if (element->diameter > 0)
				{
					PointSymbol* working_symbol = base_symbol_used ? new PointSymbol() : symbol;
					working_symbol->setInnerColor(convertColor(element->color));
					working_symbol->setInnerRadius(convertLength(element->diameter) / 2);
					working_symbol->setOuterColor(NULL);
					working_symbol->setOuterWidth(0);
					if (!base_symbol_used)
					{
						base_symbol_used = true;
					}
					else
					{
						working_symbol->setRotatable(false);
						PointObject* element_object = new PointObject(working_symbol);
// 						element_object->coords.resize(1);
						if (element->num_coords)
// 							element_object->coords[0] = MapCoord(coords[2].x, coords[2].y);
							element_object->setPosition(coords[2].x, coords[2].y);
						symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
					}
				}
				break;
			case E::TypeCircle:
				if (element->diameter > 0 && element->line_width > 0)
				{
					PointSymbol* working_symbol = base_symbol_used ? new PointSymbol() : symbol;
					working_symbol->setInnerColor(NULL);
					working_symbol->setInnerRadius(convertLength(element->diameter) / 2 - element->line_width);
					working_symbol->setOuterColor(convertColor(element->color));
					working_symbol->setOuterWidth(convertLength(element->line_width));
					if (!base_symbol_used)
					{
						base_symbol_used = true;
					}
					else
					{
						working_symbol->setRotatable(false);
						PointObject* element_object = new PointObject(working_symbol);
// 						element_object->coords.resize(1);
						if (element->num_coords)
// 							element_object->coords[0] = MapCoord(coords[2].x, coords[2].y);
							element_object->setPosition(coords[2].x, coords[2].y);
						symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
					}
				}
				break;
			case E::TypeLine:
				{
					OcdImportedLineSymbol* element_symbol = new OcdImportedLineSymbol();
					element_symbol->line_width = convertLength(element->line_width);
					element_symbol->color = convertColor(element->color);
					OcdImportedPathObject* element_object = new OcdImportedPathObject(element_symbol);
					fillPathCoords(element_object, false, element->num_coords, reinterpret_cast<const Ocd::OcdPoint32*>(coords+i+2));
					element_object->recalculateParts();
					symbol->addElement(symbol->getNumElements(), element_object, element_symbol);
				}
				break;
			case E::TypeArea:
				{
					OcdImportedAreaSymbol* element_symbol = new OcdImportedAreaSymbol();
					element_symbol->color = convertColor(element->color);
					OcdImportedPathObject* element_object = new OcdImportedPathObject(element_symbol);
					fillPathCoords(element_object, true, element->num_coords, reinterpret_cast<const Ocd::OcdPoint32*>(coords+i+2));
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
	if (ocd_object.symbol < 0)
		return NULL;
	
	Symbol* symbol = symbol_index[ocd_object.symbol];
	if (!symbol)
	{
		if (ocd_object.type == 1)
			symbol = map->getUndefinedPoint();
		else if (ocd_object.type == 2 || ocd_object.type == 3)
			symbol = map->getUndefinedLine();
		else
		{
			addWarning(tr("Unable to load object"));
			qDebug() << "Undefined object type" << ocd_object.type << " for object of symbol" << ocd_object.symbol;
			return NULL;
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
		
		MapCoord pos = convertOcdPoint(ocd_object.coords[0]);
		p->setPosition(pos.rawX(), pos.rawY());
		
		p->setMap(map);
		return p;
	}
	else if (symbol->getType() == Symbol::Text)
	{
		TextObject *t = new TextObject(symbol);
		
		// extra properties: rotation, horizontalAlignment, verticalAlignment, text
		t->setRotation(convertAngle(ocd_object.angle));
		t->setHorizontalAlignment((TextObject::HorizontalAlignment)text_halign_map.value(symbol));
		t->setVerticalAlignment(TextObject::AlignBaseline);
		
		const QChar* text_ptr = (const QChar *)(ocd_object.coords + ocd_object.num_items);
		t->setText(convertOcdString(text_ptr));
		
		// Text objects need special path translation
		if (!fillTextPathCoords(t, reinterpret_cast<TextSymbol*>(symbol), ocd_object.num_items, (Ocd::OcdPoint32 *)ocd_object.coords))
		{
			addWarning(tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
			           .arg(ocd_object.num_items).arg(t->getText()));
			delete t;
			return NULL;
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
	
	return NULL;
}

template< class O >
Object* OcdFileImport::importRectangleObject(const O& ocd_object, MapPart* part, const OcdFileImport::RectangleInfo& rect)
{
	if (ocd_object.num_items != 4)
	{
		qDebug() << "importRectangleObject called with num_items =" << ocd_object.num_items << "for object of symbol" << ocd_object.symbol;
		return NULL;
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
	border_path->getPart(0).setClosed(true, false);
	
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
			part->addObject(path, part->getNumObjects());
		}
		for (int y = 1; y < num_cells_y; ++y)
		{
			coords[0] = (top_left_f + y * cell_height * down).toMapCoord();
			coords[1] = (top_right_f + y * cell_height * down).toMapCoord();
			
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
	else if (pos >= 0)
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
bool OcdFileImport::fillTextPathCoords(TextObject *object, TextSymbol *symbol, quint16 npts, Ocd::OcdPoint32 *pts)
{
    // text objects either have 1 point (free anchor) or 2 (midpoint/size)
    // OCAD appears to always have 5 or 4 points (possible single anchor, then 4 corner coordinates going clockwise from anchor).
    if (npts == 0) return false;
	
	if (npts == 4)
	{
		// Box text
		MapCoord bottom_left = convertOcdPoint(pts[0]);
		MapCoord top_right = convertOcdPoint(pts[2]);
		MapCoord top_left = convertOcdPoint(pts[3]);
		
		// According to Purple Pen source code: OC*D adds an extra internal leading (incorrectly).
		QFontMetricsF metrics = symbol->getFontMetrics();
		double top_adjust = -symbol->getFontSize() + (metrics.ascent() + metrics.descent() + 0.5) / symbol->calculateInternalScaling();
		
		MapCoordF adjust_vector = MapCoordF(top_adjust * sin(object->getRotation()), top_adjust * cos(object->getRotation()));
		top_left = MapCoord(top_left.xd() + adjust_vector.getX(), top_left.yd() + adjust_vector.getY());
		top_right = MapCoord(top_right.xd() + adjust_vector.getX(), top_right.yd() + adjust_vector.getY());
		
		object->setBox((bottom_left.rawX() + top_right.rawX()) / 2, (bottom_left.rawY() + top_right.rawY()) / 2,
					   top_left.lengthTo(top_right), top_left.lengthTo(bottom_left));
		object->setVerticalAlignment(TextObject::AlignTop);
	}
	else
	{
		// Single anchor text
		if (npts != 5)
			addWarning(tr("Trying to import a text object with unknown coordinate format"));
		
		// anchor point
		MapCoord coord = convertOcdPoint(pts[0]); 
		object->setAnchorPosition(coord.rawX(), coord.rawY());
		object->setVerticalAlignment(TextObject::AlignBaseline);
	}
	
	return true;
}

void OcdFileImport::import(bool load_symbols_only) throw (FileFormatException)
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
		case 8:
			importImplementation< Ocd::FormatV8 >(load_symbols_only);
			break;
		case 9:
			addWarning("Untested file format: OCD 9");
			importImplementation< Ocd::FormatV9 >(load_symbols_only);
			break;
		case 11:
			addWarning("Untested file format: OCD 11");
			importImplementation< Ocd::FormatV11 >(load_symbols_only);
			break;
		default:
			throw FileFormatException(
			  Importer::tr("Could not read file: %1").
			  arg(tr("OCD files of version %1 are not supported!").arg(version))
			);
	}
}

