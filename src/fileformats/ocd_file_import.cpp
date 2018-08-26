/*
 *    Copyright 2013-2017 Kai Pastor
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

#include "ocd_file_import.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include <QBuffer>
#include <QChar>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QIODevice>
#include <QImageReader>
#include <QLatin1Char>
#include <QLatin1String>
#include <QList>
#include <QPointF>
#include <QStringRef>
#include <QTextDecoder>
#include <QVariant>

#include "settings.h"
#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_grid.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_format.h"
#include "fileformats/ocad8_file_format_p.h"
#include "fileformats/ocd_types_v10.h"
#include "fileformats/ocd_types_v11.h"
#include "fileformats/ocd_types_v12.h"
#include "fileformats/ocd_types_v9.h"
#include "templates/template.h"
#include "templates/template_image.h"
#include "templates/template_map.h"
#include "util/encoding.h"
#include "util/util.h"


namespace OpenOrienteering {

namespace {

static QTextCodec* codecFromSettings()
{
	const auto& settings = Settings::getInstance();
	const auto name = settings.getSetting(Settings::General_Local8BitEncoding).toByteArray();
	return Util::codecForName(name);
}	


}  // namespace



OcdFileImport::OcdImportedPathObject::~OcdImportedPathObject()
{
	// nothing, not inlined
}

OcdFileImport::OcdFileImport(QIODevice* stream, Map* map, MapView* view)
 : Importer { stream, map, view }
 , delegate { nullptr }
 , custom_8bit_encoding { codecFromSettings() }
{
	if (!custom_8bit_encoding)
	{
		addWarning(tr("Encoding '%1' is not available. Check the settings."));
		custom_8bit_encoding = QTextCodec::codecForLocale();
	}
}

OcdFileImport::~OcdFileImport()
{
	// nothing
}


void OcdFileImport::setCustom8BitEncoding(QTextCodec* encoding)
{
	custom_8bit_encoding = encoding;
}


QString OcdFileImport::convertOcdString(const QChar* src, uint maxlen) const
{
	auto last = src;
	if (src)
	{
		while (maxlen && *last != 0)
		{
			last ++;
			--maxlen;
		}
	}
	QTextCodec* utf16 = QTextCodec::codecForName("UTF-16LE");
	Q_ASSERT(utf16);
	auto decoder = std::unique_ptr<QTextDecoder>(utf16->makeDecoder(QTextCodec::ConvertInvalidToNull));
	return decoder->toUnicode(reinterpret_cast<const char*>(src), 2*int(last - src));
}


void OcdFileImport::addSymbolWarning(const AreaSymbol* symbol, const QString& warning)
{
	addWarning( tr("In area symbol %1 '%2': %3").
	            arg(symbol->getNumberAsString(), symbol->getName(), warning) );
}

void OcdFileImport::addSymbolWarning(const LineSymbol* symbol, const QString& warning)
{
	addWarning( tr("In line symbol %1 '%2': %3").
	            arg(symbol->getNumberAsString(), symbol->getName(), warning) );
}

void OcdFileImport::addSymbolWarning(const TextSymbol* symbol, const QString& warning)
{
	addWarning( tr("In text symbol %1 '%2': %3").
	            arg(symbol->getNumberAsString(), symbol->getName(), warning) );
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
		qDebug("quint8 has value %d, might be qint8 %d", ocd_length, qint8(ocd_length));
	return qint64(ocd_length) * 10;
}

template< >
inline
qint64 OcdFileImport::convertLength< quint16 >(quint16 ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	if (ocd_length > 50000)
		qDebug("quint16 has value %d, might be qint16 %d", ocd_length, qint16(ocd_length));
	return qint64(ocd_length) * 10;
}

template< >
inline
qint64 OcdFileImport::convertLength< quint32 >(quint32 ocd_length) const
{
	// OC*D uses hundredths of a millimeter.
	// oo-mapper uses 1/1000 mm
	if (ocd_length > 3000000)
		qDebug("quint32 has value %d, might be qint32 %d", ocd_length, qint32(ocd_length));
	return qint64(ocd_length) * 10;
}

#endif // !NDEBUG


void OcdFileImport::importImplementationLegacy(bool load_symbols_only)
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
	const OcdFile<F> file(buffer);
#ifdef MAPPER_DEVELOPMENT_BUILD
	if (!qApp->applicationName().endsWith(QLatin1String("Test")))
	{
		qDebug("*** OcdFileImport: Importing a version %d.%d file", file.header()->version, file.header()->subversion);
		for (const auto& string : file.strings())
		{
			qDebug(" %d \t%s", string.type, qPrintable(convertOcdString< typename F::Encoding >(file[string])));
		}
	}
#endif
	
	map->setSymbolSetId(QStringLiteral("OCD"));
	importGeoreferencing(file);
	importColors(file);
	importSymbols(file);
	if (!load_symbols_only)
	{
		importExtras(file);
		importObjects(file);
		importTemplates(file);
		if (view)
		{
			importView(file);
		}
	}
}


void OcdFileImport::importGeoreferencing(const OcdFile<Ocd::FormatV8>& file)
{
	const Ocd::FileHeaderV8* header = file.header();
	const Ocd::SetupV8* setup = reinterpret_cast< const Ocd::SetupV8* >(file.byteArray().data() + header->setup_pos);
	
	Georeferencing georef;
	georef.setScaleDenominator(qRound(setup->map_scale));
	georef.setProjectedRefPoint(QPointF(setup->real_offset_x, setup->real_offset_y));
	if (std::abs(setup->real_angle) >= 0.01) /* degrees */
	{
		georef.setGrivation(setup->real_angle);
	}
	map->setGeoreferencing(georef);
}

template< class F >
void OcdFileImport::importGeoreferencing(const OcdFile< F >& file)
{
	handleStrings(file, { { 1039, &OcdFileImport::importGeoreferencing } });
}

void OcdFileImport::importGeoreferencing(const QString& param_string, int /*ocd_version*/)
{
	const QChar* unicode = param_string.unicode();
	
	Georeferencing georef;
	QString combined_grid_zone;
	QPointF proj_ref_point;
	bool x_ok = false, y_ok = false;
	
	int i = param_string.indexOf(QLatin1Char('\t'), 0);
	; // skip first word for this entry type
	while (i >= 0)
	{
		bool ok;
		int next_i = param_string.indexOf(QLatin1Char('\t'), i+1);
		int len = (next_i > 0 ? next_i : param_string.length()) - i - 2;
		const QString param_value = QString::fromRawData(unicode+i+2, len); // no copying!
		switch (param_string[i+1].toLatin1())
		{
		case 'm':
			{
				double scale = param_value.toDouble(&ok);
				if (ok && scale >= 0)
					georef.setScaleDenominator(qRound(scale));
			}
			break;
		case 'a':
			{
				double angle = param_value.toDouble(&ok);
				if (ok && qAbs(angle) >= 0.01)
					georef.setGrivation(angle);
			}
			break;
		case 'x':
			proj_ref_point.setX(param_value.toDouble(&x_ok));
			break;
		case 'y':
			proj_ref_point.setY(param_value.toDouble(&y_ok));
			break;
		case 'd':
			{
				auto spacing = param_value.toDouble(&ok);
				if (ok && spacing >= 0.001)
				{
					auto grid = map->getGrid();
					grid.setUnit(MapGrid::MetersInTerrain);
					grid.setHorizontalSpacing(spacing);
					grid.setVerticalSpacing(spacing);
					map->setGrid(grid);
				}
			}
			break;
		case 'i':
			combined_grid_zone = param_value;
			break;
		case '\t':
			// empty item, fall through
		default:
			; // nothing
		}
		i = next_i;
	}
	
	if (!combined_grid_zone.isEmpty())
	{
		applyGridAndZone(georef, combined_grid_zone);
	}
	
	if (x_ok && y_ok)
	{
		georef.setProjectedRefPoint(proj_ref_point, false);
	}
	
	map->setGeoreferencing(georef);
}

void OcdFileImport::applyGridAndZone(Georeferencing& georef, const QString& combined_grid_zone)
{
	bool zone_ok = false;
	const CRSTemplate* crs_template = nullptr;
	QString id;
	QString spec;
	std::vector<QString> values;
	
	if (combined_grid_zone.startsWith(QLatin1String("20")))
	{
		auto zone = combined_grid_zone.midRef(2).toUInt(&zone_ok);
		zone_ok &= (zone >= 1 && zone <= 60);
		if (zone_ok)
		{
			id = QLatin1String{"UTM"};
			crs_template = CRSTemplateRegistry().find(id);
			values.reserve(1);
			values.push_back(QString::number(zone));
		}
	}
	else if (combined_grid_zone.startsWith(QLatin1String("80")))
	{
		auto zone = combined_grid_zone.midRef(2).toUInt(&zone_ok);
		if (zone_ok)
		{
			id = QLatin1String{"Gauss-Krueger, datum: Potsdam"};
			crs_template = CRSTemplateRegistry().find(id);
			values.reserve(1);
			values.push_back(QString::number(zone));
		}
	}
	else if (combined_grid_zone == QLatin1String("6005"))
	{
		id = QLatin1String{"EPSG"};
		crs_template = CRSTemplateRegistry().find(id);
		values.reserve(1);
		values.push_back(QLatin1String{"3067"});
	}
	else if (combined_grid_zone == QLatin1String("14001"))
	{
		id = QLatin1String{"EPSG"};
		crs_template = CRSTemplateRegistry().find(id);
		values.reserve(1);
		values.push_back(QLatin1String{"21781"});
	}
	else if (combined_grid_zone == QLatin1String("1000"))
	{
		return;
	}
	
	if (crs_template)
	{
		spec = crs_template->specificationTemplate();
		auto param = crs_template->parameters().begin();
		for (const auto& value : values)
		{
			for (const auto& spec_value : (*param)->specValues(value))
			{
				spec = spec.arg(spec_value);
			}
			++param;
		}
	}
	
	if (spec.isEmpty())
	{
		addWarning(tr("Could not load the coordinate reference system '%1'.").arg(combined_grid_zone));
	}
	else
	{
		georef.setProjectedCRS(id, spec, std::move(values));
	}
}


void OcdFileImport::importColors(const OcdFile<Ocd::FormatV8>& file)
{
	const Ocd::SymbolHeaderV8 & symbol_header = file.header()->symbol_header;
	int num_colors = symbol_header.num_colors;
	
	for (int i = 0; i < num_colors && i < 256; i++)
	{
		const Ocd::ColorInfoV8& color_info = symbol_header.color_info[i];
		const QString name = convertOcdString(color_info.name);
		int color_pos = map->getNumColors();
		auto color = new MapColor(name, color_pos);
		
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
	handleStrings(file, { { 9, &OcdFileImport::importColor } });
	addWarning(OcdFileImport::tr("Spot color information was ignored."));
}

void OcdFileImport::importColor(const QString& param_string, int /*ocd_version*/)
{
	const QChar* unicode = param_string.unicode();
	
	int i = param_string.indexOf(QLatin1Char('\t'), 0);
	const QString name = param_string.left(qMax(-1, i)); // copied
	
	int number;
	bool number_ok = false;
	MapColorCmyk cmyk { 0.0, 0.0, 0.0, 0.0 };
	bool overprinting = false;
	float opacity = 1.0f;
	
	while (i >= 0)
	{
		float f_value;
		int i_value;
		bool ok;
		int next_i = param_string.indexOf(QLatin1Char('\t'), i+1);
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
		return;
		
	int color_pos = map->getNumColors();
	auto color = new MapColor(name, color_pos);
	color->setCmyk(cmyk);
	color->setKnockout(!overprinting);
	color->setOpacity(opacity);
	map->addColor(color, color_pos);
	color_index[number] = color;
}

namespace {
	quint16 symbolType(const Ocd::BaseSymbolV8& ocd_symbol)
	{
		if (ocd_symbol.type == Ocd::SymbolTypeLine && ocd_symbol.type2 == 1)
			return Ocd::SymbolTypeLineText;
		return ocd_symbol.type;
	}
	
	template< class T >
	quint8 symbolType(const T& ocd_symbol)
	{
		return ocd_symbol.type;
	}
}

template< class F >
void OcdFileImport::importSymbols(const OcdFile< F >& file)
{
	auto ocd_version = file.header()->version;
	for (const auto& ocd_symbol : file.symbols())
	{
		// When extra symbols are created, we want to insert the main symbol
		// before them. That is why pos needs to be determined first.
		auto pos = map->getNumSymbols();
		
		Symbol* symbol = nullptr;
		switch (symbolType(ocd_symbol))
		{
		case Ocd::SymbolTypePoint:
			symbol = importPointSymbol(reinterpret_cast<const typename F::PointSymbol&>(ocd_symbol), ocd_version);
			break;
		case Ocd::SymbolTypeLine:
			symbol = importLineSymbol(reinterpret_cast<const typename F::LineSymbol&>(ocd_symbol), ocd_version);
			break;
		case Ocd::SymbolTypeArea:
			symbol = importAreaSymbol(reinterpret_cast<const typename F::AreaSymbol&>(ocd_symbol), ocd_version);
			break;
		case Ocd::SymbolTypeText:
			symbol = importTextSymbol(reinterpret_cast<const typename F::TextSymbol&>(ocd_symbol), ocd_version);
			break;
		case Ocd::SymbolTypeRectangle_V8:
		case Ocd::SymbolTypeRectangle_V9:
			symbol = importRectangleSymbol(reinterpret_cast<const typename F::RectangleSymbol&>(ocd_symbol));
			break;
		case Ocd::SymbolTypeLineText:
			symbol = importLineTextSymbol(reinterpret_cast<const typename F::LineTextSymbol&>(ocd_symbol), ocd_version);
			break;
		default:
			addWarning(OcdFileImport::tr("Unable to import symbol %1.%2 \"%3\": %4") .
			           arg(ocd_symbol.number / F::BaseSymbol::symbol_number_factor) .
			           arg(ocd_symbol.number % F::BaseSymbol::symbol_number_factor) .
			           arg(convertOcdString(ocd_symbol.description)).
			           arg(OcdFileImport::tr("Unsupported type \"%1\".").arg(ocd_symbol.type)) );
			continue;
		}
		
		map->addSymbol(symbol, pos);
		symbol_index[ocd_symbol.number] = symbol;
	}
	resolveSubsymbols();
}

void OcdFileImport::resolveSubsymbols()
{
	for (auto i = 0; i < map->getNumSymbols(); ++i)
	{
		auto symbol = map->getSymbol(i);
		if (symbol->getType() == Symbol::Combined)
		{
			auto combined = symbol->asCombined();
			if (combined->getNumParts() == 2)
			{
				auto number = combined->getPart(1)->getNumberComponent(2);
				if (number >= 0 && symbol_index.contains(static_cast<unsigned int>(number)))
				{
					combined->setPart(1, symbol_index[static_cast<unsigned int>(number)], false);
				}
			}
		}
	}
}


void OcdFileImport::importObjects(const OcdFile<Ocd::FormatV8>& file)
{
	auto ocd_version = file.header()->version;
	MapPart* part = map->getCurrentPart();
	Q_ASSERT(part);
	
	for (const auto& object_entry : file.objects())
	{
		if (object_entry.symbol)
		{
			if (auto object = importObject(file[object_entry], part, ocd_version))
				part->addObject(object, part->getNumObjects());
		}
	}
}

template< class F >
void OcdFileImport::importObjects(const OcdFile< F >& file)
{
	auto ocd_version = file.header()->version;
	MapPart* part = map->getCurrentPart();
	Q_ASSERT(part);
	
	for (const auto& object_entry : file.objects())
	{
		if ( object_entry.symbol
		     && object_entry.status != Ocd::ObjectDeleted
		     && object_entry.status != Ocd::ObjectDeletedForUndo )
		{
			if (auto object = importObject(file[object_entry], part, ocd_version))
				part->addObject(object, part->getNumObjects());
		}
	}
}


template< class F >
void OcdFileImport::importTemplates(const OcdFile< F >& file)
{
	handleStrings(file, { { 8, &OcdFileImport::importTemplate } });
}

void OcdFileImport::importTemplate(const QString& param_string, int ocd_version)
{
	const QChar* unicode = param_string.unicode();
	
	int i = param_string.indexOf(QLatin1Char('\t'), 0);
	const QString filename = QString::fromRawData(unicode, qMax(-1, i));
	const QString clean_path = QDir::cleanPath(QString(filename).replace(QLatin1Char('\\'), QLatin1Char('/')));
	const QString extension = QFileInfo(clean_path).suffix().toLower();
	
	Template* templ = nullptr;
	if (extension.compare(QLatin1String("ocd")) == 0)
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
		return;
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
		int next_i = param_string.indexOf(QLatin1Char('\t'), i+1);
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
	map->addTemplate(templ, 0);
	map->setFirstFrontTemplate(template_pos+1);
	
	if (view)
	{
		auto opacity = qMax(0.0, qMin(1.0, 0.01 * (100 - dimming)));
		view->setTemplateVisibility(templ, { float(opacity), visible });
	}
}


void OcdFileImport::importExtras(const OcdFile<Ocd::FormatV8>& file)
{
	const Ocd::FileHeaderV8* header = file.header();
	map->setMapNotes(convertOcdString< Ocd::FormatV8::Encoding >(file.byteArray().data() + header->info_pos, header->info_size));
}

template< class F >
void OcdFileImport::importExtras(const OcdFile< F >& file)
{
	map->setMapNotes({ });
	handleStrings(file, extraStringHandlers);
}

const std::initializer_list<OcdFileImport::StringHandler> OcdFileImport::extraStringHandlers = {
    {   11, &OcdFileImport::appendNotes },
    { 1061, &OcdFileImport::appendNotes }
};

void OcdFileImport::appendNotes(const QString& param_string, int ocd_version)
{
	QString notes = map->getMapNotes();
	notes.append(param_string);
	if (ocd_version <= 10)
		notes.append(QLatin1Char('\n'));
	map->setMapNotes(notes);
}


void OcdFileImport::importView(const OcdFile<Ocd::FormatV8>& file)
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
	handleStrings(file, { { 1030, &OcdFileImport::importView } });
}

void OcdFileImport::importView(const QString& param_string, int /*ocd_version*/)
{
	const QChar* unicode = param_string.unicode();
	
	bool zoom_ok = false;
	double zoom=1.0, offset_x=0.0, offset_y=0.0;
	
	int i = param_string.indexOf(QLatin1Char('\t'), 0);
	; // skip first word for this entry type
	while (i >= 0)
	{
		int next_i = param_string.indexOf(QLatin1Char('\t'), i+1);
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
	symbol->setProtected(base_symbol.status & Ocd::SymbolProtected);
	symbol->setHidden(base_symbol.status & Ocd::SymbolHidden);
}

template< class S >
PointSymbol* OcdFileImport::importPointSymbol(const S& ocd_symbol, int ocd_version)
{
	auto symbol = new OcdImportedPointSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setupPointSymbolPattern(symbol, ocd_symbol.data_size, ocd_symbol.begin_of_elements, ocd_version);
	symbol->setRotatable(ocd_symbol.base.flags & 1);
	return symbol;
}

template< class S >
Symbol* OcdFileImport::importLineSymbol(const S& ocd_symbol, int ocd_version)
{
	using LineStyle = Ocd::LineSymbolCommonV8;
	
	OcdImportedLineSymbol* line_for_borders = nullptr;
	
	// Import a main line?
	OcdImportedLineSymbol* main_line = nullptr;
	if (ocd_symbol.common.double_mode == 0 || ocd_symbol.common.line_width > 0)
	{
		main_line = importLineSymbolBase(ocd_symbol.common);
		setupBaseSymbol(main_line, ocd_symbol);
		line_for_borders = main_line;
	}
	
	// Import a 'framing' line?
	OcdImportedLineSymbol* framing_line = nullptr;
	if (ocd_symbol.common.framing_width > 0 && ocd_version >= 7)
	{
		framing_line = importLineSymbolFraming(ocd_symbol.common, main_line);
		setupBaseSymbol(framing_line, ocd_symbol);
		if (!line_for_borders)
			line_for_borders = framing_line;
	}
	
	// Import a 'double' line?
	bool has_border_line =
	        (ocd_symbol.common.double_mode != 0) &&
	        (ocd_symbol.common.double_left_width > 0 || ocd_symbol.common.double_right_width > 0);
	OcdImportedLineSymbol *double_line = nullptr;
	if (ocd_symbol.common.double_flags & LineStyle::DoubleFillColorOn
	    || (has_border_line && !line_for_borders) )
	{
		double_line = importLineSymbolDoubleBorder(ocd_symbol.common);
		setupBaseSymbol(double_line, ocd_symbol);
		line_for_borders = double_line;
	}
	else if (ocd_symbol.common.double_flags & LineStyle::DoubleBackgroundColorOn)
	{
		auto symbol = std::unique_ptr<LineSymbol>(importLineSymbolDoubleBorder(ocd_symbol.common));
		addSymbolWarning(symbol.get(),
		  OcdFileImport::tr("Unsupported line style '%1'.").arg(QLatin1String("LineStyle::DoubleBackgroundColorOn")) );
	}
	
	// Border lines
	if (has_border_line)
	{
		Q_ASSERT(line_for_borders);
		setupLineSymbolForBorder(line_for_borders, ocd_symbol.common);
	}
	
	// Create point symbols along line; middle ("normal") dash, corners, start, and end.
	OcdImportedLineSymbol* symbol_line = main_line ? main_line : double_line;	// Find the line to attach the symbols to
	if (!symbol_line)
	{
		main_line = new OcdImportedLineSymbol();
		symbol_line = main_line;
		setupBaseSymbol(main_line, ocd_symbol);
		
		main_line->segment_length = convertLength(ocd_symbol.common.main_length);
		main_line->end_length = convertLength(ocd_symbol.common.end_length);
	}
	
	setupLineSymbolPointSymbol(symbol_line, ocd_symbol.common, ocd_symbol.begin_of_elements, ocd_version);
	
	// TODO: taper fields (tmode and tlast)
	
	if (!main_line && !framing_line)
	{
		return double_line;
	}
	else if (!double_line && !framing_line)
	{
		return main_line;
	}
	else if (!main_line && !double_line)
	{
		return framing_line;
	}
	else
	{
		auto full_line = new CombinedSymbol();
		setupBaseSymbol(full_line, ocd_symbol);
		mergeLineSymbol(full_line, main_line, framing_line, double_line);
		addSymbolWarning(symbol_line, OcdFileImport::tr("This symbol cannot be saved as a proper OCD symbol again."));
		return full_line;
	}
}

OcdFileImport::OcdImportedLineSymbol* OcdFileImport::importLineSymbolBase(const Ocd::LineSymbolCommonV8& attributes)
{
	using LineStyle = Ocd::LineSymbolCommonV8;
	
	// Basic line options
	auto symbol = new OcdImportedLineSymbol();
	symbol->line_width = convertLength(attributes.line_width);
	symbol->color = convertColor(attributes.line_color);
	
	// Cap and join styles
	switch (attributes.line_style)
	{
	default:
		addSymbolWarning( symbol,
		                  tr("Unsupported line style '%1'.").
		                  arg(attributes.line_style) );
		// fall through
	case LineStyle::BevelJoin_FlatCap:
		symbol->join_style = LineSymbol::BevelJoin;
		symbol->cap_style = LineSymbol::FlatCap;
		break;
	case LineStyle::RoundJoin_RoundCap:
		symbol->join_style = LineSymbol::RoundJoin;
		symbol->cap_style = LineSymbol::RoundCap;
		break;
	case LineStyle::BevelJoin_PointedCap:
		symbol->join_style = LineSymbol::BevelJoin;
		symbol->cap_style = LineSymbol::PointedCap;
		break;
	case LineStyle::RoundJoin_PointedCap:
		symbol->join_style = LineSymbol::RoundJoin;
		symbol->cap_style = LineSymbol::PointedCap;
		break;
	case LineStyle::MiterJoin_FlatCap:
		symbol->join_style = LineSymbol::MiterJoin;
		symbol->cap_style = LineSymbol::FlatCap;
		break;
	case LineStyle::MiterJoin_PointedCap:
		symbol->join_style = LineSymbol::MiterJoin;
		symbol->cap_style = LineSymbol::PointedCap;
		break;
	}
	
	if (symbol->cap_style == LineSymbol::PointedCap)
	{
		auto ocd_length = attributes.dist_from_start;
		if (attributes.dist_from_start != attributes.dist_from_end)
		{
			// FIXME: Different lengths for start and end length of pointed line ends are not supported yet, so take the average
			ocd_length = (attributes.dist_from_start + attributes.dist_from_end) / 2;
			addSymbolWarning( symbol,
			  tr("Different lengths for pointed caps at begin (%1 mm) and end (%2 mm) are not supported. Using %3 mm.").
			  arg(locale.toString(0.001f * convertLength(attributes.dist_from_start)),
			      locale.toString(0.001f * convertLength(attributes.dist_from_end)),
			      locale.toString(0.001f * convertLength(ocd_length))) );
		}
		symbol->pointed_cap_length = convertLength(ocd_length);
		symbol->join_style = LineSymbol::RoundJoin;	// NOTE: while the setting may be different (see what is set in the first place), OC*D always draws round joins if the line cap is pointed!
	}
	else if (attributes.dist_from_start > 0 || attributes.dist_from_end > 0)
	{
		addSymbolWarning(symbol,
		                 tr("Distances from start (%1 mm) or end (%2 mm) are not supported.")
		                 .arg(locale.toString(0.001f * convertLength(attributes.dist_from_start)),
		                      locale.toString(0.001f * convertLength(attributes.dist_from_end))) );
	}
	
	// Handle the dash pattern
	if (attributes.main_gap || attributes.sec_gap)
	{
		if (!attributes.main_length)
		{
			// Invalid dash pattern
			addSymbolWarning( symbol,
			  tr("The dash pattern cannot be imported correctly.") );
		}
		else if (attributes.sec_gap && !attributes.main_gap)
		{
			// Special case main_gap == 0
			symbol->dashed = true;
			symbol->dash_length = convertLength(attributes.main_length) - convertLength(attributes.sec_gap);
			symbol->break_length = convertLength(attributes.sec_gap);
			
			if (attributes.end_length)
			{
				if (qAbs(qint32(attributes.main_length) - 2*attributes.end_length) > 1)
				{
					// End length not equal to 0.5 * main length
					addSymbolWarning( symbol,
					  tr("The dash pattern's end length (%1 mm) cannot be imported correctly. Using %2 mm.").
					  arg(locale.toString(0.001f * convertLength(attributes.end_length)),
					      locale.toString(0.001f * symbol->dash_length)) );
				}
				if (attributes.end_gap)
				{
					addSymbolWarning( symbol,
					  tr("The dash pattern's end gap (%1 mm) cannot be imported correctly. Using %2 mm.").
					  arg(locale.toString(0.001f * convertLength(attributes.end_gap)),
					      locale.toString(0.001f * symbol->break_length)) );
				}
			}
		}
		else
		{
			// Standard case
			symbol->dashed = true;
			symbol->dash_length = convertLength(attributes.main_length);
			symbol->break_length = convertLength(attributes.main_gap);
			
			if (attributes.end_length && attributes.end_length != attributes.main_length)
			{
				if (attributes.main_length && 0.75 >= double(attributes.end_length) / double(attributes.main_length))
				{
					// End length max. 75 % of main length
					symbol->half_outer_dashes = true;
				}
				
				if (qAbs(qint32(attributes.main_length) - 2*attributes.end_length) > 1)
				{
					// End length not equal to 0.5 * main length
					addSymbolWarning( symbol,
					  tr("The dash pattern's end length (%1 mm) cannot be imported correctly. Using %2 mm.").
					  arg(locale.toString(0.001f * convertLength(attributes.end_length)),
					      locale.toString(0.001f * (symbol->half_outer_dashes ? (symbol->dash_length/2) : symbol->dash_length))) );
				}
			}
			
			if (attributes.sec_gap)
			{
				symbol->dashes_in_group = 2;
				symbol->in_group_break_length = convertLength(attributes.sec_gap);
				symbol->dash_length = (symbol->dash_length - symbol->in_group_break_length) / 2;
				
				if (attributes.end_length && attributes.end_gap != attributes.sec_gap)
				{
					addSymbolWarning( symbol,
					  tr("The dash pattern's end gap (%1 mm) cannot be imported correctly. Using %2 mm.").
					  arg(locale.toString(0.001f * convertLength(attributes.end_gap)),
					      locale.toString(0.001f * symbol->in_group_break_length)) );
				}
			}
		}
	} 
	else
	{
		symbol->segment_length = convertLength(attributes.main_length);
		symbol->end_length = convertLength(attributes.end_length);
	}
	
	return symbol;
}

OcdFileImport::OcdImportedLineSymbol* OcdFileImport::importLineSymbolFraming(const Ocd::LineSymbolCommonV8& attributes, const LineSymbol* main_line)
{
	using LineStyle = Ocd::LineSymbolCommonV8;
	
	// Basic line options
	auto framing_line = new OcdImportedLineSymbol();
	framing_line->line_width = convertLength(attributes.framing_width);
	framing_line->color = convertColor(attributes.framing_color);
	
	// Cap and join styles
	switch (attributes.framing_style)
	{
	case LineStyle::BevelJoin_FlatCap:
		framing_line->join_style = LineSymbol::BevelJoin;
		framing_line->cap_style = LineSymbol::FlatCap;
		break;
	case LineStyle::RoundJoin_RoundCap:
		framing_line->join_style = LineSymbol::RoundJoin;
		framing_line->cap_style = LineSymbol::RoundCap;
		break;
	case LineStyle::MiterJoin_FlatCap:
		framing_line->join_style = LineSymbol::MiterJoin;
		framing_line->cap_style = LineSymbol::FlatCap;
		break;
	default:
		addSymbolWarning( main_line, 
		                  tr("Unsupported framing line style '%1'.").
		                  arg(attributes.line_style) );
	}
	
	return framing_line;
}

OcdFileImport::OcdImportedLineSymbol* OcdFileImport::importLineSymbolDoubleBorder(const Ocd::LineSymbolCommonV8& attributes)
{
	using LineStyle = Ocd::LineSymbolCommonV8;
	
	auto double_line = new OcdImportedLineSymbol();
	double_line->line_width = convertLength(attributes.double_width);
	double_line->cap_style = LineSymbol::FlatCap;
	double_line->join_style = LineSymbol::MiterJoin;
	double_line->segment_length = convertLength(attributes.main_length);
	double_line->end_length = convertLength(attributes.end_length);
	
	if (attributes.double_flags & LineStyle::DoubleFillColorOn)
		double_line->color = convertColor(attributes.double_color);
	else
		double_line->color = nullptr;
	
	return double_line;
}

void OcdFileImport::setupLineSymbolForBorder(OcdFileImport::OcdImportedLineSymbol* line_for_borders, const Ocd::LineSymbolCommonV8& attributes)
{
	line_for_borders->have_border_lines = true;
	LineSymbolBorder& border = line_for_borders->getBorder();
	LineSymbolBorder& right_border = line_for_borders->getRightBorder();
	
	// Border color and width
	border.color = convertColor(attributes.double_left_color);
	border.width = convertLength(attributes.double_left_width);
	border.shift = convertLength(attributes.double_left_width) / 2 + (convertLength(attributes.double_width) - line_for_borders->line_width) / 2;
	
	right_border.color = convertColor(attributes.double_right_color);
	right_border.width = convertLength(attributes.double_right_width);
	right_border.shift = convertLength(attributes.double_right_width) / 2 + (convertLength(attributes.double_width) - line_for_borders->line_width) / 2;
	
	// The borders may be dashed
	if (attributes.double_gap > 0 && attributes.double_mode > 1)
	{
		border.dashed = true;
		border.dash_length = convertLength(attributes.double_length);
		border.break_length = convertLength(attributes.double_gap);
		
		// If ocd_symbol->dmode == 2, only the left border should be dashed
		if (attributes.double_mode > 2)
		{
			right_border.dashed = border.dashed;
			right_border.dash_length = border.dash_length;
			right_border.break_length = border.break_length;
		}
	}
}

void OcdFileImport::setupLineSymbolPointSymbol(OcdFileImport::OcdImportedLineSymbol* line_symbol, const Ocd::LineSymbolCommonV8& attributes, const Ocd::PointSymbolElementV8* elements, int ocd_version)
{
	const Ocd::OcdPoint32* coords = reinterpret_cast<const Ocd::OcdPoint32*>(elements);
	
	line_symbol->mid_symbols_per_spot = attributes.num_prim_sym;
	line_symbol->mid_symbol_distance = convertLength(attributes.prim_sym_dist);
	line_symbol->mid_symbol = new OcdImportedPointSymbol();
	setupPointSymbolPattern(line_symbol->mid_symbol, attributes.primary_data_size, elements, ocd_version);
	coords += attributes.primary_data_size;
	
	if (attributes.secondary_data_size > 0)
	{
		//symbol_line->dash_symbol = importPattern( ocd_symbol->ssnpts, symbolptr);
		coords += attributes.secondary_data_size;
		addSymbolWarning(line_symbol, tr("Skipped secondary point symbol."));
	}
	if (attributes.corner_data_size > 0)
	{
		line_symbol->dash_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(line_symbol->dash_symbol, attributes.corner_data_size, reinterpret_cast<const Ocd::PointSymbolElementV8*>(coords), ocd_version);
		line_symbol->dash_symbol->setName(QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Dash symbol"));
		coords += attributes.corner_data_size;
	}
	if (attributes.start_data_size > 0)
	{
		line_symbol->start_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(line_symbol->start_symbol, attributes.start_data_size, reinterpret_cast<const Ocd::PointSymbolElementV8*>(coords), ocd_version);
		line_symbol->start_symbol->setName(QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "Start symbol"));
		coords += attributes.start_data_size;
	}
	if (attributes.end_data_size > 0)
	{
		line_symbol->end_symbol = new OcdImportedPointSymbol();
		setupPointSymbolPattern(line_symbol->end_symbol, attributes.end_data_size, reinterpret_cast<const Ocd::PointSymbolElementV8*>(coords), ocd_version);
		line_symbol->end_symbol->setName(QCoreApplication::translate("OpenOrienteering::LineSymbolSettings", "End symbol"));
	}
	
	// FIXME: not really sure how this translates... need test cases
	line_symbol->minimum_mid_symbol_count = 0; //1 + ocd_symbol->smin;
	line_symbol->minimum_mid_symbol_count_when_closed = 0; //1 + ocd_symbol->smin;
	line_symbol->show_at_least_one_symbol = false; // NOTE: this works in a different way than OC*D's 'at least X symbols' setting (per-segment instead of per-object)
	
	// Suppress dash symbol at line ends if both start symbol and end symbol exist,
	// but don't create a warning unless a dash symbol is actually defined
	// and the line symbol is not Mapper's 799 Simple orienteering course.
	if (line_symbol->start_symbol && line_symbol->end_symbol)
	{
		line_symbol->setSuppressDashSymbolAtLineEnds(true);
		if (line_symbol->dash_symbol && line_symbol->getNumberComponent(0) != 799)
		{
			addSymbolWarning(line_symbol, tr("Suppressing dash symbol at line ends."));
		}
	}
}

void OcdFileImport::mergeLineSymbol(CombinedSymbol* full_line, LineSymbol* main_line, LineSymbol* framing_line, LineSymbol* double_line)
{
	full_line->setNumParts(3); // reserve
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
}

Symbol* OcdFileImport::importAreaSymbol(const Ocd::AreaSymbolV8& ocd_symbol, int ocd_version)
{
	Q_ASSERT(ocd_version <= 8);
	auto symbol = new OcdImportedAreaSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setupAreaSymbolCommon(
	            symbol,
	            ocd_symbol.fill_on,
	            ocd_symbol.common,
	            ocd_symbol.data_size,
	            ocd_symbol.begin_of_elements,
	            ocd_version);
	return symbol;
}

template< class S >
Symbol* OcdFileImport::importAreaSymbol(const S& ocd_symbol, int ocd_version)
{
	Q_ASSERT(ocd_version >= 9);
	auto symbol = new OcdImportedAreaSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setupAreaSymbolCommon(
	            symbol,
	            ocd_symbol.common.fill_on_V9,
	            ocd_symbol.common,
	            ocd_symbol.data_size,
	            ocd_symbol.begin_of_elements,
	            ocd_version);
	if (!ocd_symbol.common.border_on_V9)
	{
		return symbol;
	}
	
	if (ocd_symbol.border_symbol == ocd_symbol.base.number)
	{
		addSymbolWarning(symbol, OcdFileImport::tr("The border of this symbol could not be loaded."));
		return symbol;
	}
	
	auto combined = new CombinedSymbol();
	setupBaseSymbol(combined, ocd_symbol);
	combined->setNumParts(2);
	combined->setPart(0, symbol, true);
	auto border = map->getUndefinedLine()->duplicate();
	border->setNumberComponent(0, symbol->getNumberComponent(0));
	border->setNumberComponent(1, symbol->getNumberComponent(1));
	border->setNumberComponent(2, static_cast<int>(ocd_symbol.border_symbol));
	combined->setPart(1, border, true);
	addSymbolWarning(symbol, OcdFileImport::tr("This symbol cannot be saved as a proper OCD symbol again."));
	return combined;
}

void OcdFileImport::setupAreaSymbolCommon(OcdImportedAreaSymbol* symbol, bool fill_on, const Ocd::AreaSymbolCommonV8& ocd_symbol, std::size_t data_size, const Ocd::PointSymbolElementV8* elements, int ocd_version)
{
	// Basic area symbol fields: minimum_area, color
	symbol->minimum_area = 0;
	symbol->color = fill_on ? convertColor(ocd_symbol.fill_color) : nullptr;
	symbol->patterns.clear();
	symbol->patterns.reserve(4);
	
	// Hatching
	if (ocd_symbol.hatch_mode != Ocd::HatchNone)
	{
		AreaSymbol::FillPattern pattern;
		pattern.type = AreaSymbol::FillPattern::LinePattern;
		pattern.angle = convertAngle(ocd_symbol.hatch_angle_1);
		pattern.setRotatable(true);
		pattern.line_spacing = convertLength(ocd_symbol.hatch_dist);
		pattern.line_offset = 0;
		pattern.line_color = convertColor(ocd_symbol.hatch_color);
		pattern.line_width = convertLength(ocd_symbol.hatch_line_width);
		if (ocd_version <= 8)
		{
			pattern.line_spacing += pattern.line_width;
		}
		symbol->patterns.push_back(pattern);
		
		if (ocd_symbol.hatch_mode == Ocd::HatchCross)
		{
			// Second hatch, same as the first, just a different angle
			pattern.angle = convertAngle(ocd_symbol.hatch_angle_2);
			symbol->patterns.push_back(pattern);
		}
	}
	
	if (ocd_symbol.structure_mode != Ocd::StructureNone)
	{
		AreaSymbol::FillPattern pattern;
		pattern.type = AreaSymbol::FillPattern::PointPattern;
		pattern.angle = convertAngle(ocd_symbol.structure_angle);
		pattern.setRotatable(true);
		pattern.point_distance = convertLength(ocd_symbol.structure_width);
		pattern.line_spacing = convertLength(ocd_symbol.structure_height);
		pattern.line_offset = 0;
		pattern.offset_along_line = 0;
		// FIXME: somebody needs to own this symbol and be responsible for deleting it
		// Right now it looks like a potential memory leak
		pattern.point = new OcdImportedPointSymbol();
		setupPointSymbolPattern(pattern.point, data_size, elements, ocd_version);
		
		// OC*D 8 has a "staggered" pattern mode, where successive rows are shifted width/2 relative
		// to each other. We need to simulate this in Mapper with two overlapping patterns, each with
		// twice the height. The second is then offset by width/2, height/2.
		if (ocd_symbol.structure_mode == Ocd::StructureShiftedRows)
		{
			pattern.line_spacing *= 2;
			symbol->patterns.push_back(pattern);
			
			pattern.line_offset = pattern.line_spacing / 2;
			pattern.offset_along_line = pattern.point_distance / 2;
			pattern.point = pattern.point->duplicate()->asPoint();
		}
		symbol->patterns.push_back(pattern);
	}
}

template< class S >
TextSymbol* OcdFileImport::importTextSymbol(const S& ocd_symbol, int /*ocd_version*/)
{
	auto symbol = new OcdImportedTextSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setBasicAttributes(symbol, convertOcdString(ocd_symbol.font_name), ocd_symbol.basic);
	setSpecialAttributes(symbol, ocd_symbol.special);
	setFraming(symbol, ocd_symbol.framing);
	return symbol;
}

template< class S >
TextSymbol* OcdFileImport::importLineTextSymbol(const S& ocd_symbol, int /*ocd_version*/)
{
	auto symbol = new OcdImportedTextSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	setBasicAttributes(symbol, convertOcdString(ocd_symbol.font_name), ocd_symbol.basic);
	setFraming(symbol, ocd_symbol.framing);
	
	addSymbolWarning(symbol, OcdFileImport::tr("Line text symbols are not yet supported. Marking the symbol as hidden."));
	symbol->setHidden(true);
	return symbol;
}

template< class S >
LineSymbol* OcdFileImport::importRectangleSymbol(const S& ocd_symbol)
{
	auto symbol = new OcdImportedLineSymbol();
	setupBaseSymbol(symbol, ocd_symbol);
	
	symbol->line_width = convertLength(ocd_symbol.line_width);
	symbol->color = convertColor(ocd_symbol.line_color);
	symbol->cap_style = LineSymbol::RoundCap;
	symbol->join_style = LineSymbol::RoundJoin;
	
	auto rect = RectangleInfo();
	rect.border_line = symbol;
	rect.corner_radius = 0.001 * convertLength(ocd_symbol.corner_radius);
	rect.has_grid = ocd_symbol.grid_flags & 1;
	
	if (rect.has_grid)
	{
		auto inner_line = new OcdImportedLineSymbol();
		setupBaseSymbol(inner_line, ocd_symbol);
		inner_line->setNumberComponent(2, 1);  // TODO: Dynamic
		inner_line->line_width = qRound(1000 * 0.15);
		inner_line->color = symbol->color;
		map->addSymbol(inner_line, map->getNumSymbols());
		
		auto text = new OcdImportedTextSymbol();
		setupBaseSymbol(text, ocd_symbol);
		text->setNumberComponent(2, 2);  // TODO: Dynamic
		text->font_family = QString::fromLatin1("Arial");
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

void OcdFileImport::setupPointSymbolPattern(PointSymbol* symbol, std::size_t data_size, const Ocd::PointSymbolElementV8* elements, int version)
{
	Q_ASSERT(symbol);
	
	symbol->setRotatable(true);
	bool base_symbol_used = false;
	
	for (std::size_t i = 0; i < data_size; i += 2)
	{
		const Ocd::PointSymbolElementV8* element = reinterpret_cast<const Ocd::PointSymbolElementV8*>(&reinterpret_cast<const Ocd::OcdPoint32*>(elements)[i]);
		const Ocd::OcdPoint32* const coords = reinterpret_cast<const Ocd::OcdPoint32*>(elements) + i + 2;
		switch (element->type)
		{
		case Ocd::PointSymbolElementV8::TypeDot:
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
					auto element_object = new PointObject(working_symbol);
					if (element->num_coords)
					{
						const MapCoord coord = convertOcdPoint(coords[0]);
						element_object->setPosition(coord.nativeX(), coord.nativeY());
					}
					symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
				}
			}
			break;
		case Ocd::PointSymbolElementV8::TypeCircle:
			{
				decltype(element->diameter) element_radius =
				        (version <= 8) ? (element->diameter / 2 - element->line_width)
				                       : ((element->diameter - element->line_width) / 2);
				if (element_radius > 0 && element->line_width > 0)
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
						auto element_object = new PointObject(working_symbol);
						if (element->num_coords)
						{
							const MapCoord coord = convertOcdPoint(coords[0]);
							element_object->setPosition(coord.nativeX(), coord.nativeY());
						}
						symbol->addElement(symbol->getNumElements(), element_object, working_symbol);
					}
				}
				break;
			}
		case Ocd::PointSymbolElementV8::TypeLine:
			if (element->line_width > 0)
			{
				auto element_symbol = new OcdImportedLineSymbol();
				element_symbol->line_width = convertLength(element->line_width);
				element_symbol->color = convertColor(element->color);
				// The flags variable doesn't seem to contain individual flags.
				switch (element->flags)
				{
				default:
					qDebug("Ocd::PointSymbolElementV8: Unknown flags value %d", element->flags);
					// fall through
				case Ocd::PointSymbolElementV8::NoFlags:
					element_symbol->setCapStyle(LineSymbol::FlatCap);
					element_symbol->setJoinStyle(LineSymbol::BevelJoin);
					break;
				case Ocd::PointSymbolElementV8::RoundStyle:
					element_symbol->setCapStyle(LineSymbol::RoundCap);
					element_symbol->setJoinStyle(LineSymbol::RoundJoin);
					break;
				case Ocd::PointSymbolElementV8::FlatMiterStyle:
					element_symbol->setCapStyle(LineSymbol::FlatCap);
					element_symbol->setJoinStyle(LineSymbol::MiterJoin);
					break;
				}
				auto element_object = new OcdImportedPathObject(element_symbol);
				fillPathCoords(element_object, false, element->num_coords, coords);
				element_object->recalculateParts();
				symbol->addElement(symbol->getNumElements(), element_object, element_symbol);
			}
			break;
		case Ocd::PointSymbolElementV8::TypeArea:
			{
				auto element_symbol = new OcdImportedAreaSymbol();
				element_symbol->color = convertColor(element->color);
				auto element_object = new OcdImportedPathObject(element_symbol);
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
Object* OcdFileImport::importObject(const O& ocd_object, MapPart* part, int ocd_version)
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
			addWarning(OcdFileImport::tr("Unable to load object"));
			qDebug() << "Undefined object type" << ocd_object.type << " for object of symbol" << ocd_object.symbol;
			return nullptr;
		}
	}
		
	if (symbol->getType() == Symbol::Line && rectangle_info.contains(ocd_object.symbol))
	{
		Object* object = importRectangleObject(ocd_object, part, rectangle_info[ocd_object.symbol]);
		if (!object)
			addWarning(OcdFileImport::tr("Unable to import rectangle object"));
		return object;
	}
	
	if (symbol->getType() == Symbol::Point)
	{
		auto p = new PointObject();
		p->setSymbol(symbol, true);
		
		// extra properties: rotation
		auto point_symbol = reinterpret_cast<PointSymbol*>(symbol);
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
		p->setPosition(pos.nativeX(), pos.nativeY());
		
		p->setMap(map);
		return p;
	}
	else if (symbol->getType() == Symbol::Text)
	{
		auto t = new TextObject(symbol);
		t->setText(getObjectText(ocd_object, ocd_version));
		t->setRotation(convertAngle(ocd_object.angle));
		t->setHorizontalAlignment(text_halign_map.value(symbol));
		// Vertical alignment is set in fillTextPathCoords().
		
		// Text objects need special path translation
		if (!fillTextPathCoords(t, reinterpret_cast<TextSymbol*>(symbol), ocd_object.num_items, reinterpret_cast<const Ocd::OcdPoint32 *>(ocd_object.coords)))
		{
			addWarning(OcdFileImport::tr("Not importing text symbol, couldn't figure out path' (npts=%1): %2")
			           .arg(ocd_object.num_items).arg(t->getText()));
			delete t;
			return nullptr;
		}
		t->setMap(map);
		return t;
	}
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined)
	{
		auto p = new OcdImportedPathObject(symbol);
		p->setPatternRotation(convertAngle(ocd_object.angle));
		
		// Normal path
		fillPathCoords(p, symbol->getType() == Symbol::Area, ocd_object.num_items, reinterpret_cast<const Ocd::OcdPoint32 *>(ocd_object.coords));
		p->recalculateParts();
		p->setMap(map);
		return p;
	}
	
	return nullptr;
}

QString OcdFileImport::getObjectText(const Ocd::ObjectV8& ocd_object, int ocd_version) const
{
	auto input  = ocd_object.coords + ocd_object.num_items;
	auto maxlen = uint(sizeof(Ocd::OcdPoint32) * ocd_object.num_text);
	QString object_text;
	if (ocd_object.unicode && ocd_version >= 8)
	{
		object_text = convertOcdString(reinterpret_cast<const QChar*>(input), maxlen/2);
	}
	else
	{
		object_text = convertOcdString<Ocd::Custom8BitEncoding>(reinterpret_cast<const char*>(input), maxlen);
	}
	
	// Remove leading "\r\n"
	if (object_text.startsWith(QLatin1String("\r\n")))
	{
		object_text.remove(0, 2);
	}
	
	return object_text;
}

template< class O >
inline
QString OcdFileImport::getObjectText(const O& ocd_object, int /*ocd_version*/) const
{
	auto data = reinterpret_cast<const QChar *>(ocd_object.coords + ocd_object.num_items);
	if (data[0] == QLatin1Char{'\r'} && data[1] == QLatin1Char{'\n'})
		data += 2;
	return QString(data);
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
	return importRectangleObject(ocd_object.coords, part, rect);
}

Object* OcdFileImport::importRectangleObject(const Ocd::OcdPoint32* ocd_points, MapPart* part, const OcdFileImport::RectangleInfo& rect)
{
	// Convert corner points
	MapCoord bottom_left = convertOcdPoint(ocd_points[0]);
	MapCoord bottom_right = convertOcdPoint(ocd_points[1]);
	MapCoord top_right = convertOcdPoint(ocd_points[2]);
	MapCoord top_left = convertOcdPoint(ocd_points[3]);
	
	MapCoordF top_left_f = MapCoordF(top_left);
	MapCoordF top_right_f = MapCoordF(top_right);
	MapCoordF bottom_left_f = MapCoordF(bottom_left);
	MapCoordF bottom_right_f = MapCoordF(bottom_right);
	MapCoordF right = MapCoordF(top_right.x() - top_left.x(), top_right.y() - top_left.y());
	auto angle = right.angle();
	MapCoordF down = MapCoordF(bottom_left.x() - top_left.x(), bottom_left.y() - top_left.y());
	right.normalize();
	down.normalize();
	
	// Create border line
	MapCoordVector coords;
	if (qIsNull(rect.corner_radius))
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
	auto border_path = new PathObject(rect.border_line, coords, map);
	border_path->parts().front().setClosed(true, false);
	
	if (rect.has_grid && rect.cell_width > 0 && rect.cell_height > 0)
	{
		// Calculate grid sizes
		auto width = top_left.distanceTo(top_right);
		auto height = top_left.distanceTo(bottom_left);
		int num_cells_x = qMax(1, qRound(width / rect.cell_width));
		int num_cells_y = qMax(1, qRound(height / rect.cell_height));
		
		auto cell_width = width / num_cells_x;
		auto cell_height = height / num_cells_y;
		
		// Create grid lines
		coords.resize(2);
		for (int x = 1; x < num_cells_x; ++x)
		{
			coords[0] = MapCoord(top_left_f + x * cell_width * right);
			coords[1] = MapCoord(bottom_left_f + x * cell_width * right);
			
			auto path = new PathObject(rect.inner_line, coords, map);
			part->addObject(path, part->getNumObjects());
		}
		for (int y = 1; y < num_cells_y; ++y)
		{
			coords[0] = MapCoord(top_left_f + y * cell_height * down);
			coords[1] = MapCoord(top_right_f + y * cell_height * down);
			
			auto path = new PathObject(rect.inner_line, coords, map);
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
					
					auto object = new TextObject(rect.text);
					object->setMap(map);
					object->setText(cell_text);
					object->setRotation(-angle);
					object->setHorizontalAlignment(TextObject::AlignLeft);
					object->setVerticalAlignment(TextObject::AlignTop);
					auto position_x = (x + qreal(0.07)) * cell_width;
					auto position_y = (y + qreal(0.04)) * cell_height + rect.text->getFontMetrics().ascent() / rect.text->calculateInternalScaling() - rect.text->getFontSize();
					object->setAnchorPosition(top_left_f + position_x * right + position_y * down);
					part->addObject(object, part->getNumObjects());
					
					//pts[0].Y -= rectinfo.gridText.FontAscent - rectinfo.gridText.FontEmHeight;
				}
			}
		}
	}
	
	return border_path;
}

void OcdFileImport::setPathHolePoint(OcdImportedPathObject *object, quint32 pos)
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

void OcdFileImport::setPointFlags(OcdImportedPathObject* object, quint32 pos, bool is_area, const Ocd::OcdPoint32& ocd_point)
{
	// We can support CurveStart, HolePoint, DashPoint.
	// CurveStart needs to be applied to the main point though, not the control point, and
	// hole points need to bet set as the last point of a part of an area object instead of the first point of the next part
	if (ocd_point.x & Ocd::OcdPoint32::FlagCtl1 && pos > 0)
		object->coords[pos-1].setCurveStart(true);
	if ((ocd_point.y & Ocd::OcdPoint32::FlagDash) || (ocd_point.y & Ocd::OcdPoint32::FlagCorner))
		object->coords[pos].setDashPoint(true);
	if (ocd_point.y & Ocd::OcdPoint32::FlagHole)
	{
		if (!is_area)
			setPathHolePoint(object, pos);
		else if (pos > 0)
			setPathHolePoint(object, pos - 1);
	}
}

/** Translates the OC*D path given in the last two arguments into an Object.
 */
void OcdFileImport::fillPathCoords(OcdImportedPathObject *object, bool is_area, quint32 num_points, const Ocd::OcdPoint32* ocd_points)
{
	object->coords.resize(num_points);
	for (auto i = 0u; i < num_points; i++)
	{
		object->coords[i] = convertOcdPoint(ocd_points[i]);
		setPointFlags(object, i, is_area, ocd_points[i]);
    }
    
    // For path objects, create closed parts where the position of the last point is equal to that of the first point
    if (object->getType() == Object::Path)
	{
		size_t start = 0;
		for (size_t i = 0; i < object->coords.size(); ++i)
		{
			if (!object->coords[i].isHolePoint() && i < object->coords.size() - 1)
				continue;
			
			if (object->coords[i].isPositionEqualTo(object->coords[start]))
			{
				MapCoord coord = object->coords[start];
				coord.setCurveStart(false);
				coord.setHolePoint(true);
				coord.setClosePoint(true);
				object->coords[i] = coord;
			}
			
			switch (i - start)
			{
			default:
				object->coords[i-2].setCurveStart(false);
				// fall through
			case 1:
				object->coords[i-1].setCurveStart(false);
				// fall through
			case 0:
				; // nothing
			}
			
			start = i + 1;
		}
	}
}

/** Translates an OCAD text object path into a Mapper text object specifier, if possible.
 *  If successful, sets either 1 or 2 coordinates in the text object and returns true.
 *  If the OCAD path was not importable, leaves the TextObject alone and returns false.
 */
bool OcdFileImport::fillTextPathCoords(TextObject *object, TextSymbol *symbol, quint32 npts, const Ocd::OcdPoint32 *ocd_points)
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
		top_left = MapCoord(top_left.x() + adjust_vector.x(), top_left.y() + adjust_vector.y());
		top_right = MapCoord(top_right.x() + adjust_vector.x(), top_right.y() + adjust_vector.y());
		
		object->setBox((bottom_left.nativeX() + top_right.nativeX()) / 2, (bottom_left.nativeY() + top_right.nativeY()) / 2,
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
		object->setAnchorPosition(coord.nativeX(), coord.nativeY());
		object->setVerticalAlignment(text_valign_map.value(symbol));
	}
	
	return true;
}

void OcdFileImport::setBasicAttributes(OcdFileImport::OcdImportedTextSymbol* symbol, const QString& font_name, const Ocd::BasicTextAttributesV8& attributes)
{
	symbol->font_family = font_name;
	symbol->color = convertColor(attributes.color);
	symbol->font_size = qRound(100.0 * attributes.font_size / 72.0 * 25.4);
	symbol->bold = (attributes.font_weight>= 550);
	symbol->italic = attributes.font_italic;
	symbol->underline = false;
	symbol->kerning = false;
	symbol->line_below = false;
	symbol->custom_tabs.resize(0);

	if (attributes.font_weight != 400 && attributes.font_weight != 700)
	{
		addSymbolWarning(symbol, tr("Ignoring custom weight (%1).").arg(attributes.font_weight));
	}
	
	switch (attributes.alignment & Ocd::HAlignMask)
	{
	case Ocd::HAlignLeft:
		text_halign_map[symbol] = TextObject::AlignLeft;
		break;
	case Ocd::HAlignRight:
		text_halign_map[symbol] = TextObject::AlignRight;
		break;
	case Ocd::HAlignJustified:
		/// \todo Implement justified alignment
		addSymbolWarning(symbol, tr("Justified alignment is not supported."));
		// fall through
	default:
		text_halign_map[symbol] = TextObject::AlignHCenter;
	}
	
	switch (attributes.alignment & Ocd::VAlignMask)
	{
	case Ocd::VAlignTop:
		text_valign_map[symbol] = TextObject::AlignTop;
		break;
	case Ocd::VAlignMiddle:
		text_valign_map[symbol] = TextObject::AlignVCenter;
		break;
	default:
		addSymbolWarning(symbol, tr("Vertical alignment '%1' is not supported.").arg(attributes.alignment & Ocd::VAlignMask));
		// fall through
	case Ocd::VAlignBottom:
		text_valign_map[symbol] = TextObject::AlignBaseline;
	}
	
	if (attributes.char_spacing != 0)
	{
		symbol->character_spacing = attributes.char_spacing / 100.0f;
		addSymbolWarning(symbol, tr("Custom character spacing may be incorrect."));
	}
	
	if (attributes.word_spacing != 100)
	{
		addSymbolWarning(symbol, tr("Ignoring custom word spacing (%1 %).").arg(attributes.word_spacing));
	}
	
	symbol->updateQFont();
}

void OcdFileImport::setSpecialAttributes(OcdFileImport::OcdImportedTextSymbol* symbol, const Ocd::SpecialTextAttributesV8& attributes)
{
	// Convert line spacing
	double absolute_line_spacing = 0.00001 * symbol->font_size * attributes.line_spacing;
	symbol->line_spacing = absolute_line_spacing / (symbol->getFontMetrics().lineSpacing() / symbol->calculateInternalScaling());
	symbol->paragraph_spacing = convertLength(attributes.para_spacing);
	
	symbol->line_below = attributes.line_below_on;
	symbol->line_below_color = convertColor(attributes.line_below_color);
	symbol->line_below_width = convertLength(attributes.line_below_width);
	symbol->line_below_distance = convertLength(attributes.line_below_offset);
	
	symbol->custom_tabs.resize(attributes.num_tabs);
	for (auto i = 0u; i < attributes.num_tabs; ++i)
		symbol->custom_tabs[i] = convertLength<quint32, int>(attributes.tab_pos[i]);
	
	if (attributes.indent_first_line != 0 || attributes.indent_other_lines != 0)
	{
		addSymbolWarning(symbol, tr("Ignoring custom indents (%1/%2).").arg(attributes.indent_first_line).arg(attributes.indent_other_lines));
	}
}

void OcdFileImport::setFraming(OcdFileImport::OcdImportedTextSymbol* symbol, const Ocd::FramingAttributesV8& framing)
{
	switch (framing.mode)
	{
	case Ocd::FramingShadow:
		symbol->framing = true;
		symbol->framing_mode = TextSymbol::ShadowFraming;
		symbol->framing_color = convertColor(framing.color);
		symbol->framing_shadow_x_offset = convertLength(framing.offset_x);
		symbol->framing_shadow_y_offset = -1 * convertLength(framing.offset_y);
		break;
	case Ocd::FramingLine: // since V7
		symbol->framing = true;
		symbol->framing_mode = TextSymbol::LineFraming;
		symbol->framing_line_half_width = convertLength(framing.line_width);
		break;
	case Ocd::FramingRectangle:
	default:
		addSymbolWarning(symbol, tr("Ignoring text framing (mode %1).").arg(framing.mode));
		// fall through
	case Ocd::FramingNone:
		symbol->framing = false;
	}
}

void OcdFileImport::import(bool load_symbols_only)
{
	Q_ASSERT(buffer.isEmpty());
	
	buffer.clear();
	buffer.append(stream->readAll());
	if (buffer.isEmpty())
		throw FileFormatException(::OpenOrienteering::Importer::tr("Could not read file: %1").arg(stream->errorString()));
	
	if (size_t(buffer.size()) < sizeof(Ocd::FormatGeneric::FileHeader))
		throw FileFormatException(::OpenOrienteering::Importer::tr("Could not read file: %1").arg(tr("Invalid data.")));
	
	const OcdFile<Ocd::FormatGeneric> generic_file(buffer);
	if (generic_file.header()->vendor_mark != 0x0cad) // This also tests correct endianess...
		throw FileFormatException(::OpenOrienteering::Importer::tr("Could not read file: %1").arg(tr("Invalid data.")));
	
	int version = generic_file.header()->version;
	switch (version)
	{
	case 6:
	case 7:
	case 8:
		// Note: Version 6 and 7 do have some differences, which will need to be
		//       handled in the version 8 implementation by looking up the
		//       actual format version in the file header.
		if (Settings::getInstance().getSetting(Settings::General_NewOcd8Implementation).toBool())
			importImplementation< Ocd::FormatV8 >(load_symbols_only);
		else
			importImplementationLegacy(load_symbols_only);
		break;
	case 9:
	case 10:
		using FormatV10Assumption = std::is_same<Ocd::FormatV10, Ocd::FormatV9>;
		Q_STATIC_ASSERT(FormatV10Assumption::value);
		importImplementation< Ocd::FormatV9 >(load_symbols_only);
		break;
	case 11:
		importImplementation< Ocd::FormatV11 >(load_symbols_only);
		break;
	case 12:
		importImplementation< Ocd::FormatV12 >(load_symbols_only);
		break;
	default:
		throw FileFormatException(
		            ::OpenOrienteering::Importer::tr("Could not read file: %1").
		            arg(tr("OCD files of version %1 are not supported!").arg(version))
		            );
	}
}

void OcdFileImport::finishImport()
{
	if (delegate)
	{
		// The current warnings and actions are already propagated.
		auto warnings_size = ptrdiff_t(delegate->warnings().size());
		auto actions_size = ptrdiff_t(delegate->actions().size());
		
		delegate->finishImport();
		
		// Propagate new warnings and actions from the delegate to this importer.
		std::for_each(begin(delegate->warnings()) + warnings_size, end(delegate->warnings()), [this](const QString& w) { addWarning(w); });
		std::for_each(begin(delegate->actions()) + actions_size, end(delegate->actions()), [this](const ImportAction& a) { addAction(a); });
	}
}

template< class F >
void OcdFileImport::handleStrings(const OcdFile<F>& file, std::initializer_list<StringHandler> handlers)
{
	for (const auto& string : file.strings())
	{
		for (const auto& handler : handlers)
		{
			if (string.type == handler.type)
			{
				(this->*handler.callback)(convertOcdString<typename F::Encoding>(file[string]), file.header()->version);
			}
		}
	}
}


}  // namespace OpenOrienteering
