/*
 *    Copyright 2026 Michael Behrens
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

#include "cli.h"

#include <cstdio>

#include <QCommandLineParser>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QLatin1String>
#include <QPainter>
#include <QStringList>

#include "core/map.h"
#include "core/map_printer.h"
#include "fileformats/file_format.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"

#ifdef QT_PRINTSUPPORT_LIB
#include <QPageSize>
#include <QPrinter>
#include <QPrinterInfo>
#include "core/map_printer.h"
#endif


namespace OpenOrienteering {

namespace {


bool exportViaFileFormat(const QString& output_path, const Map& map, const QString& format_id)
{
	const FileFormat* format = nullptr;
	if (!format_id.isEmpty())
		format = FileFormats.findFormat(format_id.toLatin1().constData());
	else
		format = FileFormats.findFormatForFilename(output_path, &FileFormat::supportsWriting);

	if (!format)
	{
		return false;
	}

	auto exporter = format->makeExporter(output_path, &map, nullptr);
	if (!exporter)
		return false;

	return exporter->doExport();
}


#ifdef QT_PRINTSUPPORT_LIB

bool exportViaPdf(const QString& output_path, Map& map, const QRectF& print_area, int dpi = 300)
{
	if (print_area.isEmpty())
	{
		fprintf(stderr, "Error: map has no extent\n");
		return false;
	}

	MapPrinter printer(map, nullptr);
	printer.setTarget(MapPrinter::pdfTarget());
	printer.setPrintArea(print_area);
	printer.setCustomPageSize(print_area.size());
	printer.setResolution(dpi);

	auto qprinter = printer.makePrinter();
	if (!qprinter)
	{
		fprintf(stderr, "Error: failed to create PDF printer.\n");
		return false;
	}

	qprinter->setOutputFileName(output_path);

	// setOutputFileName may reset the page size, re-apply it
	qprinter->setFullPage(true);
	qprinter->setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);
	qprinter->setPaperSize(printer.getPrintAreaPaperSize(), QPrinter::Millimeter);

	QPainter painter(qprinter.get());
	printer.drawPage(&painter, printer.getPrintArea());
	painter.end();
	return true;
}


bool exportViaImage(const QString& output_path, Map& map, const QRectF& print_area, int dpi = 300, const char* image_format = nullptr)
{
	if (print_area.isEmpty())
	{
		fprintf(stderr, "Error: map has no extent\n");
		return false;
	}

	MapPrinter printer(map, nullptr);
	printer.setTarget(MapPrinter::imageTarget());
	printer.setPrintArea(print_area);
	printer.setResolution(dpi);

	auto const& options = printer.getOptions();
	qreal pixel_per_mm = options.resolution / 25.4;
	QSizeF mm_size = printer.getPrintAreaPaperSize();
	int w = qMax(1, qRound(mm_size.width() * pixel_per_mm));
	int h = qMax(1, qRound(mm_size.height() * pixel_per_mm));

	QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
	if (image.isNull())
	{
		fprintf(stderr, "Error: not enough memory for image of size %dx%d\n", w, h);
		return false;
	}
	image.fill(Qt::white);
	image.setDotsPerMeterX(qRound(pixel_per_mm * 1000));
	image.setDotsPerMeterY(qRound(pixel_per_mm * 1000));

	QPainter p(&image);
	printer.drawPage(&p, printer.getPrintArea(), &image);
	p.end();

	if (!image.save(output_path, image_format))
	{
		fprintf(stderr, "Error: failed to save image to '%s'\n", qPrintable(output_path));
		return false;
	}
	return true;
}

#endif


int runExport(const QStringList& sub_args)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(QStringLiteral("Export map to printable and GIS formats"));

	QCommandLineOption input_option(
		QStringList{QStringLiteral("i"), QStringLiteral("input")},
		QStringLiteral("Input map file."),
		QStringLiteral("path"));
	parser.addOption(input_option);

	QCommandLineOption output_option(
		QStringList{QStringLiteral("o"), QStringLiteral("output")},
		QStringLiteral("Output file path."),
		QStringLiteral("path"));
	parser.addOption(output_option);

	QCommandLineOption format_option(
		QStringLiteral("output-format"),
		QStringLiteral("Output format ID (e.g. pdf, png, jpg)."),
		QStringLiteral("id"));
	parser.addOption(format_option);

	QCommandLineOption full_map_option(QStringLiteral("full-map"), QStringLiteral("Export the full map extent instead of the saved print area."));
	parser.addOption(full_map_option);

	QCommandLineOption dpi_option(
		QStringLiteral("dpi"),
		QStringLiteral("Output resolution in DPI (default: 300)."),
		QStringLiteral("dpi"),
		QStringLiteral("300"));
	parser.addOption(dpi_option);

	parser.addHelpOption();

	// QCommandLineParser requires the program name as first argument
	QStringList cli_args = {QStringLiteral("mapper")};
	cli_args.append(sub_args);

	if (!parser.parse(cli_args))
	{
		fprintf(stderr, "Error: %s\n", qPrintable(parser.errorText()));
		return 1;
	}

	if (parser.isSet(QStringLiteral("help")))
	{
		fprintf(stderr, "%s", qPrintable(parser.helpText()));
		return 0;
	}

	if (!parser.isSet(input_option))
	{
		fprintf(stderr, "Error: --input / -i is required.\n");
		return 1;
	}
	QString input_path = parser.value(input_option);

	if (!parser.isSet(output_option))
	{
		fprintf(stderr, "Error: --output / -o is required.\n");
		return 1;
	}
	QString output_path = parser.value(output_option);

	QString format_id = parser.value(format_option);

	int dpi = parser.value(dpi_option).toInt();

	Map map;
	if (!map.loadFrom(input_path))
	{
		fprintf(stderr, "Error: failed to load map from '%s'\n", qPrintable(input_path));
		return 1;
	}

	QRectF print_area = parser.isSet(full_map_option)
		? map.calculateExtent()
		: map.printerConfig().print_area;
	if (print_area.isEmpty())
	{
		fprintf(stderr, "Error: map has no content in the selected print area.\n");
		return 1;
	}

	// Determine the format key: use format_id if set, otherwise the file extension
	QString format_key = format_id.isEmpty() ? QFileInfo(output_path).suffix().toLower() : format_id.toLower();

	// Try MapPrinter for PDF and image formats (always, regardless of format_id)
#ifdef QT_PRINTSUPPORT_LIB
	if (format_key == QStringLiteral("pdf"))
	{
		if (exportViaPdf(output_path, map, print_area, dpi))
			return 0;
		fprintf(stderr, "Error: PDF export failed.\n");
		return 1;
	}

	if (QImageWriter::supportedImageFormats().contains(format_key.toLatin1()))
	{
		// Pass explicit format when format_id is set, so QImage::save
		// uses the requested format regardless of file extension.
		QByteArray format_key_latin;
		const char* img_fmt = nullptr;
		if (!format_id.isEmpty())
		{
			format_key_latin = format_key.toLatin1();
			img_fmt = format_key_latin.constData();
		}
		if (exportViaImage(output_path, map, print_area, dpi, img_fmt))
			return 0;
		fprintf(stderr, "Error: image export failed.\n");
		return 1;
	}
#endif

	fprintf(stderr, "Error: no exporter found for '%s'.\n", qPrintable(output_path));
	return 1;
}

int runConvert(const QStringList& sub_args)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(QStringLiteral("Convert between orienteering map formats"));

	QCommandLineOption input_option(
		QStringList{QStringLiteral("i"), QStringLiteral("input")},
		QStringLiteral("Input map file."),
		QStringLiteral("path"));
	parser.addOption(input_option);

	QCommandLineOption output_option(
		QStringList{QStringLiteral("o"), QStringLiteral("output")},
		QStringLiteral("Output file path."),
		QStringLiteral("path"));
	parser.addOption(output_option);

	QCommandLineOption format_option(
		QStringLiteral("output-format"),
		QStringLiteral("Output format ID (e.g. XML, OCD, OCD12)."),
		QStringLiteral("id"),
		QString{});
	parser.addOption(format_option);

	parser.addHelpOption();

	QStringList cli_args = {QStringLiteral("mapper")};
	cli_args.append(sub_args);

	if (!parser.parse(cli_args))
	{
		fprintf(stderr, "Error: %s\n", qPrintable(parser.errorText()));
		return 1;
	}

	if (parser.isSet(QStringLiteral("help")))
	{
		fprintf(stderr, "%s", qPrintable(parser.helpText()));
		return 0;
	}

	if (!parser.isSet(input_option))
	{
		fprintf(stderr, "Error: --input / -i is required.\n");
		return 1;
	}
	QString input_path = parser.value(input_option);

	if (!parser.isSet(output_option))
	{
		fprintf(stderr, "Error: --output / -o is required.\n");
		return 1;
	}
	QString output_path = parser.value(output_option);

	QString format_id = parser.value(format_option);

	Map map;
	if (!map.loadFrom(input_path))
	{
		fprintf(stderr, "Error: failed to load map from '%s'\n", qPrintable(input_path));
		return 1;
	}

	if (exportViaFileFormat(output_path, map, format_id))
		return 0;

	fprintf(stderr, "Error: no exporter found for '%s'.\n", qPrintable(output_path));
	return 1;
}

int runListFileFormats(const QStringList& sub_args)
{
	fprintf(stdout, "The following types are avaliable:\n");
	for (auto format : FileFormats.formats()) {
		fprintf(stdout, "  '%s'\t(%s)\n", format->id(), format->description().toLocal8Bit().data());
	}
	return 0;
}


}  // namespace


int execCli(int argc, char** argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Error: no subcommand specified.\n");
		return 1;
	}

	QString subcommand = QString::fromLocal8Bit(argv[2]);

	QStringList sub_args;
	for (int i = 3; i < argc; ++i)
		sub_args << QString::fromLocal8Bit(argv[i]);

	if (subcommand == QLatin1String("export"))
		return runExport(sub_args);

	if (subcommand == QLatin1String("convert"))
		return runConvert(sub_args);

	if (subcommand == QLatin1String("listFileFormats"))
		return runListFileFormats(sub_args);

	if (subcommand == QLatin1String("help") || subcommand == QLatin1String("--help"))
	{
	fprintf(stderr,
		"Usage: %s --cli <subcommand> [options]\n\n"
		"Available subcommands:\n"
		"  export           Export map to printable and image formats\n"
		"  convert          Convert between orienteering map formats\n"
		"  listFileFormats  List all available file formats\n"
		"  help							Show this help\n",
		argv[0]);
	return 0;
	}

	fprintf(stderr, "Error: unknown subcommand '%s'. Available: export, convert, listFileFormats\n", qPrintable(subcommand));
	return 1;
}


}  // namespace OpenOrienteering
