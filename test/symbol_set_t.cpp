/*
 *    Copyright 2014-2020 Kai Pastor
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

#include "symbol_set_t.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
// IWYU pragma: no_include <ext/alloc_traits.h>

#include <QtGlobal>
#include <QtTest>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QFlags>
#include <QIODevice>
#include <QLatin1Char>
#include <QLatin1String>
#include <QMetaObject>
#include <QPagedPaintDevice>
#include <QPrinter>
#include <QRectF>
#include <QStringList>
#include <QStringRef>
#include <QTextStream>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "test_config.h"

#include "global.h"
#include "settings.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "core/objects/symbol_rule_set.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/symbol.h"
#include "fileformats/xml_file_format_p.h"
#include "templates/template.h"
#include "undo/undo_manager.h"
#include "util/backports.h"  // IWYU pragma: keep


using namespace OpenOrienteering;

namespace
{

const auto legacy_symbol_sets =                              // clazy:exclude=non-pod-global-static
  QString::fromLatin1("ISOM2000;ISOM2017;ISSOM;ISSkiOM")
  .split(QLatin1Char(';'));

const auto Vanished = QString::fromLatin1("vanished");       // clazy:exclude=non-pod-global-static
const auto Obsolete = QString::fromLatin1("obsolete");       // clazy:exclude=non-pod-global-static
const auto NeedsReview = QString::fromLatin1("unfinished");  // clazy:exclude=non-pod-global-static

const auto map_symbol_translations = QByteArray(MAP_SYMBOL_TRANSLATIONS);  // clazy:exclude=non-pod-global-static

using TranslationEntries = std::vector<SymbolSetTool::TranslationEntry>;


void addSource(TranslationEntries& entries, const QString& context, const QString& source, const QString& comment)
{
	// Comment must be unique. It is used to match translations.
	auto found = std::find_if(begin(entries), end(entries), [&context, &comment](auto& entry) {
		return entry.context == context && entry.comment == comment;
	});
	QVERIFY(found == end(entries));
	entries.push_back({context, source, comment});
}

void processTranslation(const Map& map, TranslationEntries& translation_entries)
{
	auto const id = map.symbolSetId();
	
	auto num_colors = map.getNumColors();
	for (int i = 0; i < num_colors; ++i)
	{
		auto* color = map.getColor(i);
		auto const& source = color->getName();
		auto comment = QString{QLatin1String("Color ") + QString::number(color->getPriority())};
		addSource(translation_entries, id, source, comment);
	}
	
	auto num_symbols = map.getNumSymbols();
	for (int i = 0; i < num_symbols; ++i)
	{
		auto symbol = map.getSymbol(i);
		auto source = symbol->getName();
		auto comment = QString{QLatin1String("Name of symbol ") + symbol->getNumberAsString()};
		addSource(translation_entries, id, source, comment);
		
		source = symbol->getDescription();
		comment = QString{QLatin1String("Description of symbol ") + symbol->getNumberAsString()};
		if (source.isEmpty())
			qInfo("%s: empty", qPrintable(comment));
		else
			addSource(translation_entries, id, source, comment);
	}
}


/// \todo Review unused function
#ifdef SYMBOL_SET_T_UNUSED

void addTranslation(TranslationEntries& entries, const QString& context, const QString& comment, const QString& language, const QString& translation)
{
	// Match source by comment.
	auto found = std::find_if(begin(entries), end(entries), [&context, &comment](auto& entry) {
		return entry.context == context && entry.comment == comment;
	});
	QVERIFY(found != end(entries));
	// Translation must be unique.
	QVERIFY(!std::any_of(begin(found->translations), end(found->translations), [&language](auto& item) {
	    return item.language == language;
	}));
	found->translations.push_back({language, translation, {}});
	if (translation == found->source)
		found->translations.back().type = NeedsReview;
}


QString findSuggestion(TranslationEntries& translation_entries, const QString& source, const QString& language)
{
	for (auto& entry : translation_entries)
	{
		if (entry.source != source)
			continue;
		
		auto translation = std::find_if(begin(entry.translations), end(entry.translations), [&language](auto& current) {
			return current.language == language;
		});
		if (translation != end(entry.translations))
			return translation->translation;
	}
	return {};
}

#endif  // SYMBOL_SET_T_UNUSED


TranslationEntries readTsFile(QIODevice& device)
{
	auto result = TranslationEntries{};
	
	device.open(QIODevice::ReadOnly);
	QXmlStreamReader xml{&device};
	xml.readNextStartElement();
	if (!device.atEnd())
	{
		Q_ASSERT(xml.name() == QLatin1String("TS"));
		
		while (xml.readNextStartElement())
		{
			if (xml.name() == QLatin1String("context"))
			{
				xml.readNextStartElement();
				Q_ASSERT(xml.name() == QLatin1String("name"));
				
				auto context = xml.readElementText();
				
				while (xml.readNextStartElement())
				{
					if (xml.name() == QLatin1String("message"))
					{
						auto entry = SymbolSetTool::TranslationEntry{};
						entry.context = context;
						
						while (xml.readNextStartElement())
						{
							if (xml.name() == QLatin1String("source"))
							{
								entry.source = xml.readElementText();
							}
							else if (xml.name() == QLatin1String("comment"))
							{
								entry.comment = xml.readElementText();
							}
							else if (xml.name() == QLatin1String("translation"))
							{
								entry.type = xml.attributes().value(QLatin1String("type")).toString();
								entry.translation = xml.readElementText();
							}
							else
							{
								xml.skipCurrentElement();
							}
						}
						if (!entry.translation.isEmpty())
						{
							result.push_back(std::move(entry));
						}
					}
					else
					{
						xml.skipCurrentElement();
					}
				}
			}
			else
			{
				xml.skipCurrentElement();
			}
		}
	}
	
	device.close();
	return result;
}

void writeObsoleteEntries(QXmlStreamWriter& xml, TranslationEntries& entries, const QString& context)
{
	for (auto& entry: entries)
	{
		if (entry.type.isEmpty() && entry.translation.isEmpty() && legacy_symbol_sets.contains(context))
			continue;
		
		if (entry.obsolete && entry.context == context)
		{
			entry.type = Obsolete;
			entry.write(xml);
		}
	}
}


bool isMarked(const Symbol& symbol) noexcept
{
	return symbol.getName().startsWith(QStringLiteral("[MARK]"));
}

void markSymbol(Symbol& symbol)
{
	if (!isMarked(symbol))
		symbol.setName(QStringLiteral("[MARK]") + symbol.getName());
}

void markSymbolsByColor(Map& map, const MapColor* color)
{
	auto num_symbols = map.getNumSymbols();
	for (auto i = 0; i < num_symbols; ++i)
	{
		auto* symbol = map.getSymbol(i);
		if (symbol->containsColor(color))
			markSymbol(*symbol);
	}
}

void deleteMarkedSymbols(Map& map)
{
	auto num_symbols = map.getNumSymbols();
	for (auto i = num_symbols; i > 0; --i)
	{
		auto const index = i - 1;
		auto* symbol = map.getSymbol(index);
		if (isMarked(*symbol))
			map.deleteSymbol(index);
	}
}


bool validLineWidth(const LineSymbol& symbol)
{
	auto border_visible = [&]() -> bool {
		return symbol.hasBorder()
		       && (symbol.getBorder().isVisible() || symbol.getRightBorder().isVisible());
	};
	
	if (symbol.getColor())
		return symbol.getLineWidth() > 0;
	if (border_visible())
		return symbol.getLineWidth() >= 0;
	return symbol.getLineWidth() == 0;
}


bool validateAreaSymbolRotatability(const AreaSymbol& area_symbol)
{
	auto const num_fill_patterns = area_symbol.getNumFillPatterns();
	switch (num_fill_patterns)
	{
	case 0:
		return area_symbol.isRotatable() == false;
	default:
		return area_symbol.isRotatable() == area_symbol.hasRotatableFillPattern();
	}
}


void checkOsmCrtFile(const Map& target, const QDir& symbol_set_dir)
{
	auto const crt_filename = QString::fromLatin1("OSM-%1.crt").arg(target.symbolSetId());
	if (!symbol_set_dir.exists(crt_filename))
		return;
	
	QFile crt_file {symbol_set_dir.absoluteFilePath(crt_filename)};
	QVERIFY(crt_file.open(QIODevice::ReadOnly));
	QTextStream stream {&crt_file};
	
	auto rules = SymbolRuleSet::loadCrt(stream, target);
	QCOMPARE(stream.status(), QTextStream::Ok);
	
	auto test = [](auto const& item) ->bool {
		return item.type == SymbolRule::NoAssignment
		       || !item.symbol
		       || item.symbol->isHidden()
		       || !item.query.getOperator();
	};
	auto invalid_rule = std::find_if(begin(rules), end(rules), test);
	if (invalid_rule != end(rules))
	{
		auto message = QString::fromLatin1("Invalid rule in OSM-%1.crt:  %2  %3")
		               .arg(target.symbolSetId(),
		                    invalid_rule->symbol ? invalid_rule->symbol->getNumberAsString() : QStringLiteral("???"),
		                    invalid_rule->query.toString())
		               .toLatin1();
		QFAIL(message.constData());
	}
}


namespace ISOM_2017_2
{

void scale(Map& map, unsigned int source_scale, unsigned int target_scale)
{
	map.setScaleDenominator(target_scale);
	
	auto const factor = double(source_scale) / double(target_scale);
	map.scaleAllObjects(factor, MapCoord{});
	
	int symbols_changed = 0;
	for (int i = 0; i < map.getNumSymbols(); ++i)
	{
		auto* symbol = map.getSymbol(i);
		auto const code = symbol->getNumberComponent(0);
		switch (code)
		{
		case 602:  // Registration mark
		case 999:  // OpenOrienteering logo
			break;
		default:
			symbol->scale(factor);
			++symbols_changed;
		}
	}
	QCOMPARE(symbols_changed, 189);
}

}  // namespace ISOM_2017_2


namespace ISMTBOM
{

void scale(Map& map, unsigned int /*source_scale*/, unsigned int target_scale)
{
	map.setScaleDenominator(target_scale);
	
	auto const factor = (target_scale >= 15000u) ? 1.0 : 1.5;
	map.scaleAllObjects(factor, MapCoord());
	
	int symbols_changed = 0;
	for (int i = 0; i < map.getNumSymbols(); ++i)
	{
		auto* symbol = map.getSymbol(i);
		auto const code = symbol->getNumberComponent(0);
		switch (code)
		{
		case 602:  // Registration mark
		case 999:  // OpenOrienteering logo
			break;
		default:
			symbol->scale(factor);
			++symbols_changed;
		}
	}
	QCOMPARE(symbols_changed, 168);
}

}  // namespace ISMTBOM


namespace ISSkiOM_2019 {

auto const included_ISOM_codes = {
    101, 102, 103, 104, 105, 107, 109, 111,  // Land forms
    201, 202, 204, 205, 206, 207, 208, 209,  // Rock and boulders
    304, 305,  // Water and marsh
    401, 402, 403, 404, 405, 406, 413, 414, 415, 416, 419,  // Open land and vegetation
    501, 502, 503, 504, 508, 509, 510, 511, 512, 513, 515,
    516, 518, 519, 520, 521, 524, 525, 529, 530, 531,  // Man-made features
    601,  // Technical symbols
};

bool symbolOrder(const Symbol* s1, const Symbol* s2)
{
	if (s1->getNumberComponent(0) == 301
	    && s2->getNumberComponent(0) == 301)
	{
		// ISSkiOM 301.1 goes after 301.x
		if (s1->getNumberComponent(1) == 1)
			return false;
		if (s2->getNumberComponent(1) == 1)
			return true;
	}
	return Symbol::lessByNumber(s1, s2);
}

void mergeISOM(Map& target, const QDir& symbol_set_dir)
{
	// Load to-be-merged symbol set
	auto const target_scale = target.getScaleDenominator();
	auto const source_filename = QString::fromLatin1("src/ISOM 2017-2_%1.xmap").arg(target_scale);
	QVERIFY(symbol_set_dir.exists(source_filename));
	
	Map map;
	MapView view{ &map };
	map.loadFrom(symbol_set_dir.absoluteFilePath(source_filename), &view);
	QCOMPARE(map.getScaleDenominator(), target_scale);
	
	// Delete some colors, and mark the symbols which use these colors
	auto num_colors_deleted = 0;
	auto const num_colors = map.getNumColors();
	for (auto i = num_colors; i > 0; --i)
	{
		auto const index = i - 1;
		auto* const color = map.getColor(index);
		if (color->getSpotColorName() == QStringLiteral("GREEN 100, BLACK 50")
		    || color->getSpotColorName() == QStringLiteral("GREEN 60") )
		{
			markSymbolsByColor(map, color);
			map.deleteColor(index);
			++num_colors_deleted;
		}
	}
	QCOMPARE(num_colors_deleted, 3);
	
	// Mark all but explicitly included symbols
	auto const first_code = begin(included_ISOM_codes);
	auto const last_code = end(included_ISOM_codes);
	auto num_symbols = map.getNumSymbols();
	for (auto i = num_symbols; i > 0; --i)
	{
		auto const index = i - 1;
		auto* const symbol = map.getSymbol(index);
		auto const code = symbol->getNumberComponent(0);
		if (std::find(first_code, last_code, code) == last_code)
			markSymbol(*symbol);
	}
	
	// Actual merge
	auto first_imported_symbol = target.getNumSymbols();
	target.importMap(map, Map::CompleteImport, nullptr);
	
	auto const crt_filename = QString::fromLatin1("%1-%2.crt").arg(map.symbolSetId(), target.symbolSetId());
	QVERIFY(symbol_set_dir.exists(crt_filename));
	
	QFile crt_file {symbol_set_dir.absoluteFilePath(crt_filename)};
	QVERIFY(crt_file.open(QIODevice::ReadOnly));
	QTextStream stream {&crt_file};
	
	auto rules = SymbolRuleSet::loadCrt(stream, target);
	QCOMPARE(stream.status(), QTextStream::Ok);
	
	// Postprocess CRT: Find symbols by code number
	for (auto& item : rules)
	{
		QVERIFY(item.type != SymbolRule::NoAssignment);
		QVERIFY(item.symbol);
		QCOMPARE(item.query.getOperator(), ObjectQuery::OperatorSearch);
		
		auto operands = item.query.tagOperands();
		QVERIFY(operands);
		QVERIFY(!operands->value.isEmpty());
		
		// Find original symbol number matching the pattern
		for (int i = first_imported_symbol; i < target.getNumSymbols(); ++i)
		{
			auto* symbol = target.getSymbol(i);
			if (symbol->getNumberAsString() == operands->value
			    && Symbol::areTypesCompatible(symbol->getType(), item.symbol->getType()))
			{
				item.query = { symbol };
				break;
			}
		}
		if (item.query.getOperator() == ObjectQuery::OperatorSearch)
		{
			QFAIL(qPrintable(QString::fromLatin1("Invalid replacement: ISOM %1 -> ISSkiOM 2019 %2")
			                 .arg(operands->value, item.symbol->getNumberAsString())));
		}
		
		// Find inconsistencies
		if (item.query.getOperator() == ObjectQuery::OperatorSymbol)
		{
			auto has_conflict = [&item](const auto& other)->bool {
				return &other != &item
				       && other.type != SymbolRule::NoAssignment
				       && item.query == other.query
				       && item.symbol != other.symbol;
			};
			if (std::any_of(begin(rules), end(rules), has_conflict))
			{
				QFAIL(qPrintable(QString::fromLatin1("There are multiple replacements for symbol %1.")
				                 .arg(item.query.symbolOperand()->getNumberAsString())));
			}
		}
	}
	
	target.applyOnAllObjects(rules);
		
	deleteMarkedSymbols(target);
	
	target.sortSymbols(ISSkiOM_2019::symbolOrder);
}


struct ScaleFactors
{
	unsigned int scale;
	double track_symbol_factor;
	double general_factor;
};

auto const scale_factors = {
    ScaleFactors { 15000u, 1.0,  1.0 },
    ScaleFactors { 12500u, 1.0,  1.2 },
    ScaleFactors { 10000u, 1.0,  1.5 },
    ScaleFactors {  7500u, 1.33, 1.5 },
    ScaleFactors {  5000u, 1.33, 1.5 },
};

void scale(Map& map, unsigned int /*source_scale*/, unsigned int target_scale)
{
	map.setScaleDenominator(target_scale);
	
	auto scaling = std::find_if(begin(scale_factors), end(scale_factors), [target_scale](auto const& factors) {
		return factors.scale == target_scale;
	});
	QVERIFY(scaling != end(scale_factors));
	
	map.scaleAllObjects(scaling->general_factor, MapCoord());
	
	int track_symbols_changed = 0;
	int general_symbols_changed = 0;
	auto const num_symbols = map.getNumSymbols();
	for (int i = 0; i < num_symbols; ++i)
	{
		Symbol* symbol = map.getSymbol(i);
		const int code = symbol->getNumberComponent(0);
		if (code >= 800 && code < 900)
		{
			symbol->scale(scaling->track_symbol_factor);
			++track_symbols_changed;
		}
		else
		{
			symbol->scale(scaling->general_factor);
			++general_symbols_changed;
		}
	}
	QCOMPARE(track_symbols_changed, 15);
	QCOMPARE(general_symbols_changed, 116);
}

}  // namespace ISSkiOM_2019


namespace Course_Design
{

MapView* makeView(Map& map, unsigned int target_scale)
{
	[&map]() { QCOMPARE(map.getNumTemplates(), 1); } ();
	auto* view = new MapView { &map };
	view->setGridVisible(true);
	if (target_scale == 10000)
		view->setTemplateVisibility(map.getTemplate(0), { 1, true });
	else
		map.deleteTemplate(0);
	return view;
}

void resetPrinterConfig(Map& map)
{
	auto printer_config = map.printerConfig();
	printer_config.options.show_templates = true;
	printer_config.single_page_print_area = true;
	printer_config.center_print_area = true;
	printer_config.page_format = { { 200.0, 287.0 }, 5.0 };
	printer_config.page_format.page_size = QPageSize::A4; 
	printer_config.print_area = printer_config.page_format.page_rect;
	map.setPrinterConfig(printer_config);
}

void scale(Map& map, unsigned int source_scale, unsigned int target_scale)
{
	map.setScaleDenominator(target_scale);
	
	auto const factor = double(source_scale) / double(target_scale);
	map.scaleAllObjects(factor, MapCoord());
}

}  // namespace Course_Design


}  // namespace



void SymbolSetTool::TranslationEntry::write(QXmlStreamWriter& xml) const
{
	if (source.isEmpty())
	{
		QVERIFY(!comment.isEmpty());
		return;
	}
	
	xml.writeStartElement(QLatin1String("message"));
	xml.writeTextElement(QLatin1String("source"), source);
	xml.writeTextElement(QLatin1String("comment"), comment);
	xml.writeStartElement(QLatin1String("translation"));
	if (!type.isEmpty())
		xml.writeAttribute(QLatin1String("type"), type);
	xml.writeCharacters(translation);		
	xml.writeEndElement();  //  translation
	xml.writeEndElement();  // message
}



/**
 * Saves the map to the given path iff this changes the file's content.
 */
void saveIfDifferent(const QString& path, Map* map, MapView* view = nullptr)
{
	QByteArray new_data;
	QByteArray existing_data;
	QFile file(path);
	if (file.exists())
	{
		QVERIFY(file.open(QIODevice::ReadOnly));
		existing_data.reserve(int(file.size()+1));
		existing_data = file.readAll();
		QCOMPARE(file.error(), QFileDevice::NoError);
		file.close();
		
		new_data.reserve(existing_data.size()*2);
	}
	
	XMLFileExporter exporter({}, map, view);
	auto is_src_format = path.contains(QLatin1String(".xmap"));
	exporter.setOption(QString::fromLatin1("autoFormatting"), is_src_format);
	auto retain_compatibility = is_src_format && map->getNumParts() == 1
	                            && !path.contains(QLatin1String("ISOM2017"));
	Settings::getInstance().setSetting(Settings::General_RetainCompatiblity, retain_compatibility);
	
	QBuffer buffer(&new_data);
	exporter.setDevice(&buffer);
	exporter.doExport();
	buffer.close();
	QVERIFY(exporter.warnings().empty());
	
	if (new_data != existing_data)
	{
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(new_data);
		QVERIFY(file.flush());
		file.close();
		QCOMPARE(file.error(), QFileDevice::NoError);
	}
	
	QVERIFY(file.exists());
}

void saveIfDifferent(QFile& file, QByteArray& new_data, const QByteArray& existing_data)
{
	auto find_eol = [](auto first, auto last) {
		return std::find(first, last, '\n');
	};
	auto find_printing = [](auto first, auto last) {
		return std::find_if(first, last, [](auto c) {
			return c != ' ' && c != '\t' && c != '\n';
		});
	};
	using std::begin; using std::end;
	auto last1 = end(new_data);
	auto first1 = find_printing(begin(new_data), last1);
	auto last2 = end(existing_data);
	auto first2 = find_printing(begin(existing_data), last2);
	auto equal = true;
	while (equal && first1 != last1)
	{
		auto eol1 = find_eol(first1, last1);
		auto eol2 = find_eol(first2, last2);
		equal = std::equal(first1, eol1, first2, eol2);
		first1 = find_printing(eol1, last1);
		first2 = find_printing(eol2, last2);
	}
	
	if (!equal)
	{
		new_data.replace("'", "&apos;");
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(new_data);
		QVERIFY(file.flush());
		file.close();
		QCOMPARE(file.error(), QFileDevice::NoError);
	}
}



void SymbolSetTool::initTestCase()
{
	// Use distinct QSettings
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1(metaObject()->className()));
	QVERIFY2(QDir::home().exists(), "The home dir must be writable in order to use QSettings.");
	
	doStaticInitializations();
	
	symbol_set_dir.cd(QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("../symbol sets")));
	QVERIFY(symbol_set_dir.exists());
	
	examples_dir.cd(QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("../examples")));
	QVERIFY(examples_dir.exists());
	
	test_data_dir.cd(QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("../test/data")));
	QVERIFY(test_data_dir.exists());
	
	translations_dir.cd(QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("../translations")));
	QVERIFY(translations_dir.exists());
	
	Template::suppressAbsolutePaths = true;
	
	translations_complete = false;
}


void SymbolSetTool::processSymbolSet_data()
{
	translations_complete = true;
	
	QTest::addColumn<QString>("name");
	QTest::addColumn<unsigned int>("source_scale");
	QTest::addColumn<unsigned int>("target_scale");

	QTest::newRow("ISOM 2017-2 1:15000") << QString::fromLatin1("ISOM 2017-2")  << 15000u << 15000u;
	QTest::newRow("ISOM 2017-2 1:10000") << QString::fromLatin1("ISOM 2017-2")  << 15000u << 10000u;
	
	QTest::newRow("ISOM2017 translation-only") << QString::fromLatin1("ISOM2017") << 15000u << 15000u;
	
	QTest::newRow("ISOM2000 translation-only") << QString::fromLatin1("ISOM2000") << 15000u << 15000u;
	
	QTest::newRow("ISSprOM 2019 1:4000") << QString::fromLatin1("ISSprOM 2019") <<  4000u <<  4000u;
	
	QTest::newRow("ISSOM translation-only") << QString::fromLatin1("ISSOM") <<  5000u <<  5000u;
	
	QTest::newRow("ISMTBOM 1:20000") << QString::fromLatin1("ISMTBOM") << 15000u << 20000u;
	QTest::newRow("ISMTBOM 1:15000") << QString::fromLatin1("ISMTBOM") << 15000u << 15000u;
	QTest::newRow("ISMTBOM 1:10000") << QString::fromLatin1("ISMTBOM") << 15000u << 10000u;
	QTest::newRow("ISMTBOM 1:7500")  << QString::fromLatin1("ISMTBOM") << 15000u <<  7500u;
	QTest::newRow("ISMTBOM 1:5000")  << QString::fromLatin1("ISMTBOM") << 15000u <<  5000u;
	
	QTest::newRow("ISSkiOM translation-only") << QString::fromLatin1("ISSkiOM") << 15000u << 15000u;
	
	QTest::newRow("ISSkiOM 2019 1:15000") << QString::fromLatin1("ISSkiOM 2019") << 15000u << 15000u;
	QTest::newRow("ISSkiOM 2019 1:12500") << QString::fromLatin1("ISSkiOM 2019") << 15000u << 12500u;
	QTest::newRow("ISSkiOM 2019 1:10000") << QString::fromLatin1("ISSkiOM 2019") << 15000u << 10000u;
	QTest::newRow("ISSkiOM 2019 1:7500")  << QString::fromLatin1("ISSkiOM 2019") << 15000u <<  7500u;
	QTest::newRow("ISSkiOM 2019 1:5000")  << QString::fromLatin1("ISSkiOM 2019") << 15000u <<  5000u;
	
	QTest::newRow("Course Design 1:15000") << QString::fromLatin1("Course_Design") << 10000u << 15000u;
	QTest::newRow("Course Design 1:10000") << QString::fromLatin1("Course_Design") << 10000u << 10000u;
	QTest::newRow("Course Design 1:5000")  << QString::fromLatin1("Course_Design") << 10000u <<  5000u;
	QTest::newRow("Course Design 1:4000")  << QString::fromLatin1("Course_Design") << 10000u <<  4000u;
}

void SymbolSetTool::processSymbolSet()
{
	// On failure, we exit with translations_complete == false.
	auto completeness  = translations_complete;
	translations_complete = false;
	
	QFETCH(QString, name);
	QFETCH(unsigned int, source_scale);
	QFETCH(unsigned int, target_scale);
	
	QString source_filename = QString::fromLatin1("src/%1_%2.xmap").arg(name, QString::number(source_scale));
	QVERIFY(symbol_set_dir.exists(source_filename));
	
	QString source_path = symbol_set_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, &view);
	QCOMPARE(map.getScaleDenominator(), source_scale);
	QCOMPARE(map.getNumClosedTemplates(), 0);
	
	auto id = map.symbolSetId();
	QCOMPARE(id, name);
	
	map.resetPrinterConfig();
	map.undoManager().clear();
	for (int i = 0; i < map.getNumColors(); ++i)
	{
		auto color = map.getMapColor(i);
		if (color->getSpotColorMethod() == MapColor::CustomColor)
		{
			color->setCmykFromSpotColors();
			color->setRgbFromSpotColors();
		}
		else
		{
			color->setRgbFromCmyk();
		}
	}
	saveIfDifferent(source_path, &map, &view);
	
	if (name == QLatin1String("ISSkiOM 2019"))
	{
		ISSkiOM_2019::mergeISOM(map, symbol_set_dir);
	}
	
	auto const is_legacy = legacy_symbol_sets.contains(id);
	if (!is_legacy)
	{
		const int num_symbols = map.getNumSymbols();
		QStringList previous_numbers;
		for (int i = 0; i < num_symbols; ++i)
		{
			const Symbol* symbol = map.getSymbol(i);
			QString number = symbol->getNumberAsString();
			QString number_and_name = number + QLatin1Char(' ') % symbol->getPlainTextName();
			QVERIFY2(!symbol->getName().isEmpty(), qPrintable(number_and_name + QLatin1String(": Name is empty")));
			QVERIFY2(!previous_numbers.contains(number), qPrintable(number_and_name + QLatin1String(": Number is not unique")));
			previous_numbers.append(number);
			QVERIFY2(symbol->validate(), qPrintable(number_and_name + QLatin1String(": Symbol validation failed")));
			switch(symbol->getType())
			{
			case Symbol::Area:
				QVERIFY2(validateAreaSymbolRotatability(static_cast<const AreaSymbol&>(*symbol)), qPrintable(number_and_name + QLatin1String(": Unexpected area symbol rotatability")));
				break;
			case Symbol::Line:
				QVERIFY2(validLineWidth(static_cast<const LineSymbol&>(*symbol)), qPrintable(number_and_name + QLatin1String(": Invalid line width")));
				break;
			default:
			    ; // nothing
			}
		}
	}
		
	if (std::none_of(begin(translation_entries), end(translation_entries),
	                 [&id](auto const& entry) { return entry.context == id; }) )
	{
		processTranslation(map, translation_entries);
	}
	
	if (source_scale == target_scale)
	{
		checkOsmCrtFile(map, symbol_set_dir);
	}
	else
	{
		if (name == QStringLiteral("ISOM 2017-2"))
		{
			ISOM_2017_2::scale(map, source_scale, target_scale);
		}
		else if (name.startsWith(QLatin1String("ISMTBOM")))
		{
			ISMTBOM::scale(map, source_scale, target_scale);
		}
		else if (name == QLatin1String("ISSkiOM 2019"))
		{
			ISSkiOM_2019::scale(map, source_scale, target_scale);
		}
		else if (name.startsWith(QLatin1String("Course_Design")))
		{
			Course_Design::scale(map, source_scale, target_scale);
		}
		else
		{
			QFAIL("Symbol set not recognized");
		}
	}
	QCOMPARE(map.getScaleDenominator(), target_scale);
	
	if (is_legacy)
		return;
	
	MapView* new_view = nullptr;
	if (name.startsWith(QLatin1String("Course_Design")))
	{
		new_view = Course_Design::makeView(map, target_scale);
		Course_Design::resetPrinterConfig(map);
	}
	else
	{
		QCOMPARE(map.getNumTemplates(), 0);
	}
	
	QString target_filename = QString::fromLatin1("%2/%1_%2.omap").arg(name, QString::number(target_scale));
	saveIfDifferent(symbol_set_dir.absoluteFilePath(target_filename), &map, new_view);
	
	translations_complete = completeness;
}


void SymbolSetTool::processSymbolSetTranslations_data()
{
	QTest::addColumn<int>("unused");
	QTest::newRow("map_symbols_template.ts") << 0;
#ifdef MAP_SYMBOL_TRANSLATIONS
	const auto files = map_symbol_translations.split(';');
	for (const auto& file : files)
		QTest::newRow(file) << 0;
#endif
}
	
void SymbolSetTool::processSymbolSetTranslations() const
{
	auto translation_filename = QString::fromLatin1(QTest::currentDataTag());
	auto language = translation_filename.mid(int(qstrlen("map_symbols_")));
	language.chop(int(qstrlen(".ts")));
	if (language == QLatin1String("template"))
		language.clear();
	
	QByteArray new_data;
	QByteArray existing_data;
	QFile file(translations_dir.absoluteFilePath(translation_filename));
	if (file.exists())
	{
		QVERIFY(file.open(QIODevice::ReadOnly));
		existing_data.reserve(int(file.size()+1));
		existing_data = file.readAll();
		QCOMPARE(file.error(), QFileDevice::NoError);
		file.close();
		
		// Workaround Weblate quirk
		existing_data.replace("\n    </message>", "\n        </message>");
		existing_data.replace("&apos;", "'");
		
		new_data.reserve(existing_data.size()*2);
	}
	
	QBuffer buffer(&existing_data);
	auto translations_from_ts = readTsFile(buffer);
	QVERIFY(!buffer.isOpen());
	
	buffer.setBuffer(&new_data);
	QVERIFY(buffer.open(QIODevice::WriteOnly | QIODevice::Truncate));
	QXmlStreamWriter xml(&buffer);
	xml.setAutoFormatting(true);
	xml.writeStartDocument();
	xml.writeDTD(QLatin1String("<!DOCTYPE TS>"));
	xml.writeStartElement(QLatin1String("TS"));
	xml.writeAttribute(QLatin1String("version"), QLatin1String("2.1"));
	if (!language.isEmpty())
		xml.writeAttribute(QLatin1String("language"), language);
	
	auto context = QString{};
	for (auto it = begin(translation_entries), last = end(translation_entries); it != last; ++it)
	{
		auto entry = TranslationEntry{*it};  // copy, don't touch original entry.
		entry.type = NeedsReview;
		entry.obsolete = false;
		
		if (context != entry.context)
		{
			if (!context.isEmpty())
			{
				writeObsoleteEntries(xml, translations_from_ts, context);
				xml.writeEndElement(); // context
			}
			xml.writeStartElement(QLatin1String("context"));
			xml.writeTextElement(QLatin1String("name"), entry.context);
			context = entry.context;
		}
		
		// First attempt: exact context + source + comment (symbol number!) match.
		auto found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
			return !current.translation.isEmpty()
			       && entry.context == current.context
			       && entry.source == current.source
			       && entry.comment == current.comment;
		});
		if (found != end(translations_from_ts))
		{
			found->obsolete = false;
		}
		else
		{
			// Second attempt: exact context + source match.
			found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return !current.translation.isEmpty()
				       && entry.context == current.context
				       && entry.source == current.source;
			});
		}
		if (found == end(translations_from_ts))
		{
			// Third attempt: exact source match.
			found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return !current.translation.isEmpty()
				       && entry.source == current.source;
			});
		}
		if (found != end(translations_from_ts))
		{
			// If any of the previous attempts to find a translation succeeded,
			// the translation is chosen and marked as not being obsolete.
			entry.translation = found->translation;
			if (found->type != Obsolete && found->type != Vanished)
				entry.type = found->type;
		}
		else
		{
			// Find an existing translation even with changed source,
			// using the comment (symbol number!) instead of source.
			auto found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return !current.translation.isEmpty()
				       && entry.context == current.context
				       && entry.comment == current.comment;
			});
			if (found != end(translations_from_ts))
			{
				entry.translation = found->translation;
			}
		}
		
		if (!entry.translation.isEmpty()
		    || !legacy_symbol_sets.contains(context))
		{
			entry.write(xml);
		}
	}
	if (!context.isEmpty())
	{
		writeObsoleteEntries(xml, translations_from_ts, context);
		xml.writeEndElement();  // context
	}
	
	xml.writeEndElement(); // TS
	xml.writeEndDocument();
	buffer.close();
	
	// Weblate uses lower-case "utf-8".
	static auto lower_case_xml = R"(<?xml version="1.0" encoding="utf-8"?>)";
	if (!new_data.startsWith(lower_case_xml))
	{
		auto pos = new_data.indexOf('>');
		new_data.replace(0, pos+1, lower_case_xml);
	}
	
	// Weblate uses different formatting
	new_data.replace("    <", "<");
	
	saveIfDifferent(file, new_data, existing_data);
}



void SymbolSetTool::processExamples_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<QString>("id");

	QTest::newRow("complete map")  << QString::fromLatin1("complete map") << QString::fromLatin1("ISSOM");
	QTest::newRow("forest sample") << QString::fromLatin1("forest sample") << QString::fromLatin1("ISOM2000");
	QTest::newRow("overprinting")  << QString::fromLatin1("overprinting") << QString::fromLatin1("ISOM2000");
}

void SymbolSetTool::processExamples()
{
	QFETCH(QString, name);
	QFETCH(QString, id);
	
	QString source_filename = QString::fromLatin1("src/%1.xmap").arg(name);
	QVERIFY(examples_dir.exists(source_filename));
	
	QString source_path = examples_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, &view);
	
	const int num_symbols = map.getNumSymbols();
	QStringList previous_numbers;
	for (int i = 0; i < num_symbols; ++i)
	{
		const Symbol* symbol = map.getSymbol(i);
		QString number = symbol->getNumberAsString();
		QString number_and_name = number + QLatin1Char(' ') % symbol->getPlainTextName();
		QVERIFY2(!symbol->getName().isEmpty(), qPrintable(number_and_name + QLatin1String(": Name is empty")));
		QVERIFY2(!previous_numbers.contains(number), qPrintable(number_and_name + QLatin1String(": Number is not unique")));
		previous_numbers.append(number);
		QVERIFY2(symbol->validate(), qPrintable(number_and_name + QLatin1String(": Symbol validation failed")));
	}
	
	map.setSymbolSetId(id);
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
	
	QString target_filename = QString::fromLatin1("%1.omap").arg(name);
	saveIfDifferent(examples_dir.absoluteFilePath(target_filename), &map, &view);
}


void SymbolSetTool::processTestData_data()
{
	QTest::addColumn<QString>("name");

	QTest::newRow("world-file")  << QString::fromLatin1("templates/world-file");
}

void SymbolSetTool::processTestData()
{
	QFETCH(QString, name);
	
	QString source_filename = QString::fromLatin1("%1.xmap").arg(name);
	QVERIFY(test_data_dir.exists(source_filename));
	
	QString source_path = test_data_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, &view);
	
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
}


/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace {
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(SymbolSetTool)
