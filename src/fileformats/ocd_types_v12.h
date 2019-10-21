/*
 *    Copyright 2013, 2015, 2016 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_TYPES_V12_H
#define OPENORIENTEERING_OCD_TYPES_V12_H

#include "ocd_types.h"
#include "ocd_types_v11.h"

namespace Ocd
{
	
#pragma pack(push, 1)
	
	struct FileHeaderV12 : public FormatV11::FileHeader
	{
		quint32 RESERVED_MEMBER[2];
		quint32 first_multi_rep_block;
	};
	
	struct AreaSymbolV12
	{
		using BaseSymbol = FormatV11::BaseSymbol;
		using Element    = FormatV11::PointSymbol::Element;
		
		BaseSymbol base;
		
		quint32 border_symbol;
		AreaSymbolCommonV8 common;
		quint8  structure_variation_x;
		quint8  structure_variation_y;
		quint16 structure_minimum_dist;
		quint16 RESERVED_MEMBER;
		quint16 data_size;
		
		Element begin_of_elements[1];
	};
	
	struct ObjectV12
	{
		using IndexEntryType = FormatV11::Object::IndexEntryType;
		
		qint32  symbol;
		quint8  type;
		quint8  customer;
		qint16  angle;
		qint32  color;
		quint16 line_width;
		quint16 diam_flags;
		quint32 server_object_id;
		quint32 height;
		quint64 creation_date;
		quint32 multi_rep_id;
		quint64 modification_date;
		quint32 num_items;
		quint16 num_text;
		quint16 object_string_length;
		quint16 db_link_length;
		quint8  object_string_type;
		quint8  RESERVED_MEMBER;
		
		OcdPoint32 coords[1];
	};
	
#pragma pack(pop)
	
	/** OCD file format version 11 trait. */
	struct FormatV12
	{
		constexpr static quint16 version = 12;
		using FileHeader      = FileHeaderV12;
		using BaseSymbol      = FormatV11::BaseSymbol;
		using PointSymbol     = FormatV11::PointSymbol;
		using LineSymbol      = FormatV11::LineSymbol;
		using AreaSymbol      = AreaSymbolV12;
		using TextSymbol      = FormatV11::TextSymbol;
		using LineTextSymbol  = FormatV11::LineTextSymbol;
		using RectangleSymbol = FormatV11::RectangleSymbol;
		using Object          = ObjectV12;
		using Encoding        = Utf8Encoding;
	};
}


OCD_EXPLICIT_INSTANTIATION(extern template, Ocd::FormatV12)


#endif // OPENORIENTEERING_OCD_TYPES_V12_H
