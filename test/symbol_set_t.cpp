/*
 *    Copyright 2014-2017 Kai Pastor
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
#include <QPagedPaintDevice>
#include <QPrinter>
#include <QRectF>
#include <QStringList>
#include <QStringRef>
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
#include "core/symbols/area_symbol.h"
#include "core/symbols/symbol.h"
#include "fileformats/xml_file_format_p.h"
#include "templates/template.h"
#include "undo/undo_manager.h"
#include "util/backports.h"


using namespace OpenOrienteering;

namespace
{

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
	entries.push_back({context, source, comment, {} });
	// n occurences of ';' means n+1 translations, plus translation template -> n+2
	entries.back().translations.reserve(std::size_t(map_symbol_translations.count(';')) + 2);
}


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


TranslationEntries readTsFile(QIODevice& device, const QString& language)
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
								auto type = xml.attributes().value(QLatin1String("type")).toString();
								auto translation = xml.readElementText();
								if (!translation.isEmpty())
									entry.translations.resize(1, { language, translation, type });
							}
							else
							{
								xml.skipCurrentElement();
							}
						}
						if (!entry.translations.empty())
							result.push_back(std::move(entry));
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

}  // namespace



void SymbolSetTool::TranslationEntry::write(QXmlStreamWriter& xml, const QString& language)
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
	for (const auto& translation : translations)
	{
		if (translation.language == language)
		{
			if (!translation.type.isEmpty())
			{
				xml.writeAttribute(QLatin1String("type"), translation.type);
			}
			xml.writeCharacters(translation.translation);		
			break;
		}
	}
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

void SymbolSetTool::initTestCase()
{
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1("SymbolSetTool"));
	
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

	QTest::newRow("ISOM2017 1:15000") << QString::fromLatin1("ISOM2017")  << 15000u << 15000u;
	QTest::newRow("ISOM2017 1:10000") << QString::fromLatin1("ISOM2017")  << 15000u << 10000u;
	
	QTest::newRow("ISOM2000 1:15000") << QString::fromLatin1("ISOM2000")  << 15000u << 15000u;
	QTest::newRow("ISOM2000 1:10000") << QString::fromLatin1("ISOM2000")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000") << QString::fromLatin1("ISSOM") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000") << QString::fromLatin1("ISSOM") <<  5000u <<  4000u;
	
	QTest::newRow("ISMTBOM 1:20000") << QString::fromLatin1("ISMTBOM") << 15000u << 20000u;
	QTest::newRow("ISMTBOM 1:15000") << QString::fromLatin1("ISMTBOM") << 15000u << 15000u;
	QTest::newRow("ISMTBOM 1:10000") << QString::fromLatin1("ISMTBOM") << 15000u << 10000u;
	QTest::newRow("ISMTBOM 1:7500")  << QString::fromLatin1("ISMTBOM") << 15000u <<  7500u;
	QTest::newRow("ISMTBOM 1:5000")  << QString::fromLatin1("ISMTBOM") << 15000u <<  5000u;
	
	QTest::newRow("ISSkiOM 1:15000") << QString::fromLatin1("ISSkiOM") << 15000u << 15000u;
	QTest::newRow("ISSkiOM 1:10000") << QString::fromLatin1("ISSkiOM") << 15000u << 10000u;
	QTest::newRow("ISSkiOM 1:5000")  << QString::fromLatin1("ISSkiOM") << 15000u <<  5000u;
	
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
	
	auto raw_tag = QTest::currentDataTag();
	auto tag = QByteArray::fromRawData(raw_tag, int(qstrlen(raw_tag)));
	
	QFETCH(QString, name);
	QFETCH(unsigned int, source_scale);
	QFETCH(unsigned int, target_scale);
	
	auto id = name;
	auto language = QString{};
	if (!tag.endsWith('0'))
	{
		auto suffix_index = name.lastIndexOf(QLatin1Char('_'));
		id = name.left(suffix_index);
		language = name.mid(suffix_index + 1);
		Q_ASSERT(language.length() == 2);
	}
	
	QString source_filename = QString::fromLatin1("src/%1_%2.xmap").arg(name, QString::number(source_scale));
	QVERIFY(symbol_set_dir.exists(source_filename));
	
	QString source_path = symbol_set_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, &view);
	QCOMPARE(map.getScaleDenominator(), source_scale);
	QCOMPARE(map.getNumClosedTemplates(), 0);
	
	map.setSymbolSetId(id);
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
	
	auto purple = QColor::fromCmykF(0, 1, 0, 0).hueF();
	if (source_scale != target_scale)
	{
		map.setScaleDenominator(target_scale);
		
		if (name.startsWith(QLatin1String("ISOM2000")))
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				const QColor& color = *symbol->guessDominantColor();
				if (qAbs(purple - color.hueF()) > 0.1
				    && code != 602
				    && code != 999)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
				
				if (code == 601 && symbol->getType() == Symbol::Area)
				{
					AreaSymbol::FillPattern& pattern0 = symbol->asArea()->getFillPattern(0);
					if (pattern0.type == AreaSymbol::FillPattern::LinePattern)
					{
						switch (target_scale)
						{
						case 10000u:
							pattern0.line_spacing = 40000;
							break;
						default:
							QFAIL("Undefined north line spacing for this scale");
						}
						++north_lines_changed;
					}
				}
			}
			QCOMPARE(symbols_changed, 139);
			QCOMPARE(north_lines_changed, 2);
		}
		else if (name.startsWith(QLatin1String("ISOM2017")))
		{
			const auto factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord{});
			
			int symbols_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code != 602
				    && code != 999)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
			}
			QCOMPARE(symbols_changed, 184);
		}
		else if (name.startsWith(QLatin1String("ISSOM")))
		{
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code == 601 && symbol->getType() == Symbol::Area)
				{
					AreaSymbol::FillPattern& pattern0 = symbol->asArea()->getFillPattern(0);
					if (pattern0.type == AreaSymbol::FillPattern::LinePattern)
					{
						switch (target_scale)
						{
						case 4000u:
							pattern0.line_spacing = 37500;
							break;
						default:
							QFAIL("Undefined north line spacing for this scale");
						}
						++north_lines_changed;
					}
				}
			}
			QCOMPARE(north_lines_changed, 2);
		}
		else if (name.startsWith(QLatin1String("ISMTBOM")))
		{
			QCOMPARE(source_scale, 15000u);
			const double factor = (target_scale >= 15000u) ? 1.0 : 1.5;
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code != 602
				    && code != 999)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
			}
			QCOMPARE(symbols_changed, 168);
		}
		else if (name.startsWith(QLatin1String("ISSkiOM")))
		{
			QCOMPARE(source_scale, 15000u);
			const double factor = (target_scale >= 15000u) ? 1.0 : 1.5;
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				const QColor& color = *symbol->guessDominantColor();
				if (qAbs(purple - color.hueF()) > 0.1
				    && code != 602
				    && code != 999)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
				
				if (code == 601 && symbol->getType() == Symbol::Area)
				{
					AreaSymbol::FillPattern& pattern0 = symbol->asArea()->getFillPattern(0);
					if (pattern0.type == AreaSymbol::FillPattern::LinePattern)
					{
						switch (target_scale)
						{
						case 5000u:
						case 10000u:
							pattern0.line_spacing = 40000;
							break;
						default:
							QFAIL("Undefined north line spacing for this scale");
						}
						++north_lines_changed;
					}
				}
			}
			QCOMPARE(symbols_changed, 152);
			QCOMPARE(north_lines_changed, 2);
		}
		else if (name.startsWith(QLatin1String("Course_Design")))
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
		}
		else
		{
			QFAIL("Symbol set not recognized");
		}
	}
	else
	{
		// Not scaled: Collect translation source strings.
		auto num_colors = map.getNumColors();
		for (int i = 0; i < num_colors; ++i)
		{
			auto color = map.getColor(i);
			auto source = color->getName();
			auto comment = QString{QLatin1String("Color ") + QString::number(color->getPriority())};
			addSource(translation_entries, id, source, comment);
		}
		
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
	
	MapView* new_view = nullptr;
	if (name.startsWith(QLatin1String("Course_Design")))
	{
		QCOMPARE(map.getNumTemplates(), 1);
		new_view = new MapView { &map };
		new_view->setGridVisible(true);
		if (target_scale == 10000)
			new_view->setTemplateVisibility(map.getTemplate(0), { 1, true });
		else
			map.deleteTemplate(0);
		
		auto printer_config = map.printerConfig();
		printer_config.options.show_templates = true;
		printer_config.single_page_print_area = true;
		printer_config.center_print_area = true;
		printer_config.page_format = { { 200.0, 287.0 }, 5.0 };
		printer_config.page_format.paper_size = QPrinter::A4; 
		printer_config.print_area = printer_config.page_format.page_rect;
		map.setPrinterConfig(printer_config);
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
	
void SymbolSetTool::processSymbolSetTranslations()
{
	auto translation_filename = QString::fromLatin1(QTest::currentDataTag());
	auto language = translation_filename.mid(int(qstrlen("map_symbols_")));
	language.chop(int(qstrlen(".ts")));
	if (language.length() > 2)
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
	auto translations_from_ts = readTsFile(buffer, language);
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
	for (auto& entry : translation_entries)
	{
		if (context != entry.context)
		{
			if (!context.isEmpty())
				xml.writeEndElement(); // context
			xml.writeStartElement(QLatin1String("context"));
			xml.writeTextElement(QLatin1String("name"), entry.context);
			context = entry.context;
		}
		
		auto item = std::find_if(begin(entry.translations), end(entry.translations), [&language](auto& translation) {
			return translation.language == language;
		});
		if (item == end(entry.translations))
		{
			entry.translations.push_back({language, {}, NeedsReview});
			item = --entry.translations.end();
		}
		auto& translation = item->translation;
		auto& type = item->type;
		
		
		// First attempt: exact context + source + comment (symbol number!) match.
		auto found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
			return entry.context == current.context && entry.source == current.source && entry.comment == current.comment;
		});
		if (found == end(translations_from_ts))
		{
			// Second attempt: exact context + source match.
			found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return entry.context == current.context && entry.source == current.source;
			});
		}
		if (found == end(translations_from_ts))
		{
			// Third attempt: exact source match.
			found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return entry.source == current.source;
			});
		}
		if (found != end(translations_from_ts))
		{
			// If any of the previous attempts to find a translation succeeded,
			// the translation is chosen and marked as not being obsolete.
			auto match = found->translations.front();
			translation = match.translation;
			if (match.type != Obsolete)
				type = match.type;
		}
		else
		{
			// Find an existing translation even with changed source,
			// using the comment (symbol number!) instead of source.
			auto found = std::find_if(begin(translations_from_ts), end(translations_from_ts), [&entry](auto& current) {
				return entry.context == current.context && entry.comment == current.comment;
			});
			if (found != end(translations_from_ts))
			{
				auto match = found->translations.front();
				translation = match.translation;
			}
			// Anyway, this translation needs review.
			type = NeedsReview;
		}
		entry.write(xml, language);
	}
	if (!context.isEmpty())
	{
		xml.writeEndElement();  // context
	}
	
	xml.writeEndElement(); // TS
	xml.writeEndDocument();
	buffer.close();
	
	// Weblate uses lower-case "utf-8".
	static auto lower_case_xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	if (!new_data.startsWith(lower_case_xml))
	{
		auto pos = new_data.indexOf('>');
		new_data.replace(0, pos+1, lower_case_xml);
	}
	
	if (new_data != existing_data)
	{
		new_data.replace("'", "&apos;");
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(new_data);
		QVERIFY(file.flush());
		file.close();
		QCOMPARE(file.error(), QFileDevice::NoError);
	}
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
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(SymbolSetTool)
