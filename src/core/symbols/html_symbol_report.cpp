/*
 *    Copyright 2024 Kai Pastor
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

#include "html_symbol_report.h"

#include <QBuffer>
#include <QByteArray>
#include <QChar>
#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QImageWriter>
#include <QLatin1String>
#include <QLocale>
#include <QString>
#include <QTextDocumentFragment>

#include "core/map.h"
#include "core/map_color.h"
#include "core/symbols/symbol.h"

namespace OpenOrienteering {

/**
 * Generates HTML reports for colors and symbols in a Map.
 * 
 * The public interface of this class is the free-standing function
 * makeHTMLSymbolReport(const Map& map).
 */
class HTMLSymbolReportGenerator
{
private:
	/**
	 * Efficiently produce PNG data from QImage.
	 * 
	 * An object of this class holds a buffer which is reused in multiple calls
	 * to write(). So the number of actual memory allocations can be kept small.
	 */
	class PNGImageWriter
	{
		QByteArray png {"PNG"};
		QByteArray png_data;
		
	public:
		/**
		 * Creates PNG data from an image.
		 * 
		 * @return A reference to a shared buffer. The buffer is invalidated
		 *         when the function is called again (for the same writer).
		 */
		const QByteArray& write(const QImage& image)
		{
			png_data.clear();
			QBuffer buffer(&png_data);
			QImageWriter(&buffer, png).write(image);
			return png_data;
		}
	};
	
	enum class ColorRowType {
		Basic,    ///< A color row with number, icon and name
		Extended  ///< A color row with all details
	};
	
	const Map& map;
	QImage icon_image;
	QLocale locale;
	PNGImageWriter image_writer;
	
	QString imgForColor(const QColor& c, const QString& alt)
	{
		icon_image.fill(c);
		return QString::fromLatin1("<img alt=\"%1\" src=\"data:image/png;base64,").arg(alt)
		       + QString::fromLatin1(image_writer.write(icon_image).toBase64())
		       + QString::fromLatin1("\">");
	}
	
	QString makeColorRow(const MapColor& c, ColorRowType row_type)
	{
		icon_image = QImage(16, 16, QImage::Format_ARGB32);
		
		auto details = QString {};
		if (row_type == ColorRowType::Extended)
		{
			auto cmyk = c.getCmyk();
			details = QString::fromLatin1(
			              "<td>%1</td>" // c
			              "<td>%2</td>" // m
			              "<td>%3</td>" // y
			              "<td>%4</td>" // k
			              "<td style=\"font-family:monospace\">%5</td>" // rgb
			              "<td>%6</td>" // spot color
			              "<td>%7</td>" // knockout
			              ).arg(
			                  locale.toString(100*cmyk.c, 'g', 3),
			                  locale.toString(100*cmyk.m, 'g', 3),
			                  locale.toString(100*cmyk.y, 'g', 3),
			                  locale.toString(100*cmyk.k, 'g', 3),
			                  QColor(c.getRgb()).name(),
			                  QString{c.getSpotColorName()}.replace(QString::fromLatin1(", "), QString::fromLatin1(",<br>")),
			                  c.getKnockout() ? QString::fromLatin1("[X]") : QString{} );
		}
		
		auto name = QTextDocumentFragment::fromHtml(c.getName()).toPlainText();
		return QString::fromLatin1(
		           "<tr>"
		           "<td>%1</td>" // level
		           "<td>%2</td>" // icon
		           "<td class=\"name\">%3</td>" // name
		           "%9"          // details
		           "</tr>\n"
		           ).arg(
		        QString::number(c.getPriority()),
		        imgForColor(c, name),
		        name,
		        details );
	}
	
	QString makeColorSection()
	{
		QString color_data;
		map.applyOnAllColors([this, &color_data](const MapColor* c) {
			color_data.append(makeColorRow(*c, ColorRowType::Extended));
		});
		return QString::fromLatin1(
		           "<h2>%1</h2>\n"
		           "<table class=\"colors\">\n"
		           "<thead>\n"
		           "<tr>"
		           "<th colspan=\"2\">%2</th>"
		           "<th class=\"name\">%3</th>"
		           "<th>C</th>"
		           "<th>M</th>"
		           "<th>Y</th>"
		           "<th>K</th>"
		           "<th>%4</th>"
		           "<th>%5</th>"
		           "<th>%6</th>"
		           "</tr>\n"
		           "</thead>\n"
		           "<tbody>\n"
		           "%9"
		           "</tbody>\n"
		           "</table>\n"
		           ).arg(
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Map Colors"),
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Color"),
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Name"),
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "RGB"),
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Spot colors"),
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Knockout"),
		        color_data );
	}
	
	QString imgForSymbol(const Symbol& s, const QString& alt)
	{
		// For better quality and compression,
		// using multiple of display size but no antialiasing
		auto const display_size = 48;
		icon_image = s.createIcon(map, 4 * display_size, false);
		return QString::fromLatin1("<img alt=\"%1\" width=\"%2\" src=\"data:image/png;base64,").arg(alt).arg(display_size)
		       + QString::fromLatin1(image_writer.write(icon_image).toBase64())
		       + QString::fromLatin1("\">");
	}
	
	QString colorsForSymbol(const Symbol& s)
	{
		QString color_data;
		map.applyOnAllColors([this, &s, &color_data](const MapColor* c){
			if (s.containsColor(c))
				color_data.append(makeColorRow(*c, ColorRowType::Basic));
		});
		return QString::fromLatin1(
		           "<table>\n"
		           "<tbody>\n"
		           "%1"
		           "</tbody>\n"
		           "</table>\n"
		           ).arg(
		        color_data );
	}
	
	QString makeSymbolRow(const Symbol& s)
	{
		auto label = s.getNumberAndPlainTextName();
		auto extra_text = QString{};
		if (s.isRotatable())
			extra_text += QCoreApplication::translate("OpenOrienteering::SymbolReport", "[X] %1")
			              .arg(QCoreApplication::translate("OpenOrienteering::SymbolReport", "Symbol orientation can be changed."))
			              + QLatin1String("<br>\n");
		if (s.hasRotatableFillPattern())
			extra_text += QCoreApplication::translate("OpenOrienteering::SymbolReport", "[X] %1")
			              .arg(QCoreApplication::translate("OpenOrienteering::SymbolReport", "Pattern orientation can be changed."))
			              + QLatin1String("<br>\n");
		if (s.isHelperSymbol())
			extra_text += QCoreApplication::translate("OpenOrienteering::SymbolReport", "[X] %1")
			              .arg(QCoreApplication::translate("OpenOrienteering::SymbolReport", "Helper symbol"))
			              + QLatin1String("<br>\n");
		return QString::fromLatin1(
		           "<tr>"
		           "<td style=\"vertical-align:top;\">%1</td>\n"          // icon
		           "<td style=\"vertical-align:middle;\"><b>%2</b></td>"  // label
		           "</tr>\n"
		           "<tr>"
		           "<td>&nbsp;</td>\n"
		           "<td style=\"padding-bottom:18px;\">\n"
		           "<div>\n%3</div>\n"  // description
		           "<p>%4</p>\n"        // configuration properties
		           "%5</td>"            // colors
		           "</tr>\n"
		           ).arg(
		        imgForSymbol(s, label),
		        label,
		        QString(s.getDescription()).replace(QChar::LineFeed, QLatin1String("<br>\n")),
		        extra_text,
		        colorsForSymbol(s) );
	}
	
	QString makeSymbolSection()
	{
		QString symbol_data;
		map.applyOnAllSymbols([this, &symbol_data](const Symbol* s){
			symbol_data.append(makeSymbolRow(*s));
		});
		return QString::fromLatin1(
		           "<h2>%1</h2>\n"
		           "<table class=\"symbols\">\n"
		           "<tbody>\n"
		           "%2"
		           "</tbody>\n"
		           "</table>\n"
		           ).arg(
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Symbols"),
		        symbol_data );
	}
	
public:
	explicit HTMLSymbolReportGenerator(const Map& map)
	: map(map)
	{}
	
	QString generate()
	{
		return QString::fromLatin1(
		           "<!DOCTYPE html>\n"
		           "<html>\n"
		           "<head>\n"
		           "<meta charset=\"utf-8\">"
		           "<title>%0</title>\n"
		           "<meta name=\"generator\" content=\"OpenOrienteering Mapper\">\n"
		           "<style>\n"
		           "th { font-size: 120%; text-align: center; }\n"
		           "th, td { padding: 4px; }\n"
		           "table.colors { text-align: center; }\n"
		           "table.colors td:first-child { text-align: right; }\n"
		           "table.colors td.name { text-align: left; }\n"
		           "table.symbols { max-width: 60em; }\n"
		           "</style>\n"
		           "</head>\n"
		           "<body>\n"
		           "<h1>%0</h1>\n"
		           "%1"
		           "%2"
		           "</body>\n"
		           "</html>"
		           ).arg(
		        QCoreApplication::translate("OpenOrienteering::SymbolReport", "Symbol Set Report on '%0'").arg(map.symbolSetId()),
		        makeColorSection(),
		        makeSymbolSection() );
	}
};


QString makeHTMLSymbolReport(const Map& map)
{
	return HTMLSymbolReportGenerator(map).generate();
}

}  // namespace OpenOrienteering
