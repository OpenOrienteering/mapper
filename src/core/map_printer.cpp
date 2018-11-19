/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018  Kai Pastor
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


#include "map_printer.h"

#include <cmath>

#include <Qt>
#include <QtMath>
#include <QColor>
#include <QDebug>
#include <QHash>
#include <QImage>
#include <QLatin1String>
#include <QPagedPaintDevice>
#include <QPaintDevice>
#include <QPaintEngine> // IWYU pragma: keep
#include <QPainter>
#include <QPointF>
#include <QStringRef>
#include <QTransform>
#include <QXmlStreamReader>


#if defined(QT_PRINTSUPPORT_LIB)
#  include <QPrinter>
#  include <advanced_pdf_printer.h>
#  include <printer_properties.h>
#  if defined(Q_OS_WIN)
#    include <private/qprintengine_win_p.h>
#  endif
#endif

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_view.h"
#include "core/renderables/renderable.h"
#include "templates/template.h"
#include "util/xml_stream_util.h"


// ### A namespace which collects various string constants of type QLatin1String. ###

namespace literal
{
	static const QLatin1String scale("scale");
	static const QLatin1String resolution("resolution");
	static const QLatin1String templates_visible("templates_visible");
	static const QLatin1String grid_visible("grid_visible");
	static const QLatin1String simulate_overprinting("simulate_overprinting");
	static const QLatin1String mode("mode");
	static const QLatin1String vector("vector");
	static const QLatin1String raster("raster");
	static const QLatin1String separations("separations");
	static const QLatin1String color_mode("color_mode");
	static const QLatin1String default_color_mode("default");
	static const QLatin1String device_cmyk("DeviceCMYK");
	static const QLatin1String page_format("page_format");
	static const QLatin1String paper_size("paper_size");
	static const QLatin1String orientation("orientation");
	static const QLatin1String portrait("portrait");
	static const QLatin1String landscape("landscape");
	static const QLatin1String h_overlap("h_overlap");
	static const QLatin1String v_overlap("v_overlap");
	static const QLatin1String dimensions("dimensions");
	static const QLatin1String page_rect("page_rect");
	static const QLatin1String print_area("print_area");
	static const QLatin1String center_area("center_area");
	static const QLatin1String single_page("single_page");
}



namespace OpenOrienteering {

// #### MapPrinterPageFormat ###

MapPrinterPageFormat::MapPrinterPageFormat(QSizeF page_rect_size, qreal margin, qreal overlap) 
 :
#ifdef QT_PRINTSUPPORT_LIB
   paper_size(QPrinter::Custom)
#else
   paper_size(-1)
#endif
 , orientation(Portrait)
 , page_rect(QRectF(QPointF(margin, margin), page_rect_size))
 , paper_dimensions(page_rect.size() + 2.0* QSizeF(margin, margin))
 , h_overlap(overlap)
 , v_overlap(overlap)
{
	// nothing
}

#ifdef QT_PRINTSUPPORT_LIB

MapPrinterPageFormat::MapPrinterPageFormat(const QPrinter& printer, qreal overlap)
 : paper_size(printer.paperSize())
 , orientation((printer.orientation() == QPrinter::Portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape)
 , page_rect(printer.pageRect(QPrinter::Millimeter))
 , paper_dimensions(printer.paperSize(QPrinter::Millimeter))
 , h_overlap(overlap)
 , v_overlap(overlap)
{
	// nothing
}

#endif

MapPrinterPageFormat MapPrinterPageFormat::fromDefaultPrinter()
{
#ifdef QT_PRINTSUPPORT_LIB
	QPrinter default_printer;
	return MapPrinterPageFormat(default_printer);
#else
	return MapPrinterPageFormat();
#endif
}

bool operator==(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return  lhs.paper_size  == rhs.paper_size &&
	        lhs.orientation == rhs.orientation &&
	        fabs(lhs.h_overlap - rhs.h_overlap) < 0.05 &&
	        fabs(lhs.v_overlap - rhs.v_overlap) < 0.05 &&
	        fabs(lhs.page_rect.top() - rhs.page_rect.top()) < 0.05 &&
	        fabs(lhs.page_rect.left() - rhs.page_rect.left()) < 0.05 &&
	        fabs(lhs.page_rect.right() - rhs.page_rect.right()) < 0.05 &&
	        fabs(lhs.page_rect.bottom() - rhs.page_rect.bottom()) < 0.05 &&
	        fabs(lhs.paper_dimensions.width() - rhs.paper_dimensions.width()) < 0.05 &&
	        fabs(lhs.paper_dimensions.height() - rhs.paper_dimensions.height()) < 0.05;
}

// ### MapPrinterOptions ###

MapPrinterOptions::MapPrinterOptions(unsigned int scale, int resolution, MapPrinterMode mode)
 : scale(scale),
   resolution(resolution),
   mode(mode),
   color_mode(DefaultColorMode),
   show_templates(false),
   show_grid(false),
   simulate_overprinting(false)
{
	// nothing
}



// ### MapPrinterConfig ###

MapPrinterConfig::MapPrinterConfig(const Map& map)
 : printer_name(QString::fromLatin1("DEFAULT")),
   print_area(map.calculateExtent()),
   page_format(MapPrinterPageFormat::fromDefaultPrinter()),
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	if (print_area.isEmpty())
		print_area = page_format.page_rect;
}

MapPrinterConfig::MapPrinterConfig(const Map& map, QXmlStreamReader& xml)
 : printer_name(QString::fromLatin1("DEFAULT")),
   print_area(0.0, 0.0, 100.0, 100.0), // Avoid expensive calculation before loading.
   page_format(),
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	XmlElementReader printer_config_element(xml);
	
	options.scale = printer_config_element.attribute<unsigned int>(literal::scale);
	options.resolution = printer_config_element.attribute<int>(literal::resolution);
	options.show_templates = printer_config_element.attribute<bool>(literal::templates_visible);
	options.show_grid = printer_config_element.attribute<bool>(literal::grid_visible);
	options.simulate_overprinting = printer_config_element.attribute<bool>(literal::simulate_overprinting);
	QStringRef mode = printer_config_element.attribute<QStringRef>(literal::mode);
	if (!mode.isEmpty())
	{
		if (mode == literal::vector)
			options.mode = MapPrinterOptions::Vector;
		else if (mode == literal::raster)
			options.mode = MapPrinterOptions::Raster;
		else if (mode == literal::separations)
			options.mode = MapPrinterOptions::Separations;
		else
			qDebug() << "Unsupported map printing mode:" << mode;
	}
	QStringRef color_mode = printer_config_element.attribute<QStringRef>(literal::color_mode);
	if (!color_mode.isEmpty())
	{
		if (color_mode == literal::default_color_mode)
			options.color_mode = MapPrinterOptions::DefaultColorMode;
		else if (color_mode == literal::device_cmyk)
			options.color_mode = MapPrinterOptions::DeviceCmyk;
		else
			qDebug() << "Unsupported map color mode:" << color_mode;
	}
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::page_format)
		{
			XmlElementReader page_format_element(xml);
			QString value;
			
#ifdef QT_PRINTSUPPORT_LIB
			value = page_format_element.attribute<QString>(literal::paper_size);
			const QHash< int, const char* >& paper_size_names = MapPrinter::paperSizeNames();
			for (int i = 0; i < paper_size_names.count(); ++i)
			{
				if (value == QLatin1String(paper_size_names[i]))
					page_format.paper_size = i;
			}
#endif
			
			value = page_format_element.attribute<QString>(literal::orientation);
			page_format.orientation =
			  (value == literal::portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape;
			if (page_format_element.hasAttribute(literal::h_overlap))
				page_format.h_overlap = page_format_element.attribute<qreal>(literal::h_overlap);
			if (page_format_element.hasAttribute(literal::v_overlap))
				page_format.v_overlap = page_format_element.attribute<qreal>(literal::v_overlap);
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::dimensions)
				{
					XmlElementReader(xml).read(page_format.paper_dimensions);
				}
				else if (xml.name() == literal::page_rect)
				{
					XmlElementReader(xml).read(page_format.page_rect);
				}
				else
					xml.skipCurrentElement();
			}
		}
		else if (xml.name() == literal::print_area)
		{
			XmlElementReader print_area_element(xml);
			print_area_element.read(print_area);
			center_print_area = print_area_element.attribute<bool>(literal::center_area);
			single_page_print_area= print_area_element.attribute<bool>(literal::single_page);
		}
		else
			xml.skipCurrentElement();
	}
	
	// Sanity checks
	if (options.scale <= 0)
		options.scale = map.getScaleDenominator();
	if (options.resolution <= 0)
		options.resolution = 600;
}

void MapPrinterConfig::save(QXmlStreamWriter& xml, const QLatin1String& element_name) const
{
	XmlElementWriter printer_config_element(xml, element_name);
	
	printer_config_element.writeAttribute(literal::scale, options.scale);
	printer_config_element.writeAttribute(literal::resolution, options.resolution);
	printer_config_element.writeAttribute(literal::templates_visible, options.show_templates);
	printer_config_element.writeAttribute(literal::grid_visible, options.show_grid);
	printer_config_element.writeAttribute(literal::simulate_overprinting, options.simulate_overprinting);
	switch (options.mode)
	{
	case MapPrinterOptions::Vector:
		printer_config_element.writeAttribute(literal::mode, literal::vector);
		break;
	case MapPrinterOptions::Raster:
		printer_config_element.writeAttribute(literal::mode, literal::raster);
		break;
	case MapPrinterOptions::Separations:
		printer_config_element.writeAttribute(literal::mode, literal::separations);
		break;
	default:
		// Do not fail on saving
		qDebug() << "Unsupported map printing mode:" << options.mode;
	}

	switch (options.color_mode)
	{
	case MapPrinterOptions::DefaultColorMode:
		// No need to write an attribute for default mode.
		// printer_config_element.writeAttribute(literal::color_mode, literal::default_color_mode);
		break;
	case MapPrinterOptions::DeviceCmyk:
		printer_config_element.writeAttribute(literal::color_mode, literal::device_cmyk);
		break;
	default:
		// Do not fail on saving
		qDebug() << "Unsupported map color mode:" << options.color_mode;
	}

	{
		XmlElementWriter page_format_element(xml, literal::page_format);
#ifdef QT_PRINTSUPPORT_LIB
		page_format_element.writeAttribute(literal::paper_size,
		  MapPrinter::paperSizeNames()[page_format.paper_size]);
#endif
		page_format_element.writeAttribute(literal::orientation,
		  (page_format.orientation == MapPrinterPageFormat::Portrait) ? literal::portrait : literal::landscape );
		page_format_element.writeAttribute(literal::h_overlap, page_format.h_overlap, 2);
		page_format_element.writeAttribute(literal::v_overlap, page_format.v_overlap, 2);
		{
			XmlElementWriter(xml, literal::dimensions).write(page_format.paper_dimensions, 3);
		}
		{
			XmlElementWriter(xml, literal::page_rect).write(page_format.page_rect, 3);
		}
	}
	{
		XmlElementWriter print_area_element(xml, literal::print_area);
		print_area_element.write(print_area, 3);
		print_area_element.writeAttribute(literal::center_area, center_print_area);
		print_area_element.writeAttribute(literal::single_page, single_page_print_area);
	}
}



#ifdef QT_PRINTSUPPORT_LIB

// ### MapPrinter ###

const QPrinterInfo* MapPrinter::pdfTarget()
{
	static QPrinterInfo pdf_target; // TODO: set name and features?
	return &pdf_target;
}

const QPrinterInfo* MapPrinter::imageTarget()
{
	static QPrinterInfo image_target; // TODO: set name and features?
	return &image_target;
}

const QHash< int, const char* >& MapPrinter::paperSizeNames()
{
	static QHash< int, const char* > names;
	if (names.empty())
	{
		names[QPrinter::A0]        = "A0";
		names[QPrinter::A1]        = "A1";
		names[QPrinter::A2]        = "A2";
		names[QPrinter::A3]        = "A3";
		names[QPrinter::A4]        = "A4";
		names[QPrinter::A5]        = "A5";
		names[QPrinter::A6]        = "A6";
		names[QPrinter::A7]        = "A7";
		names[QPrinter::A8]        = "A8";
		names[QPrinter::A9]        = "A9";
		names[QPrinter::B0]        = "B0";
		names[QPrinter::B1]        = "B1";
		names[QPrinter::B2]        = "B2";
		names[QPrinter::B3]        = "B3";
		names[QPrinter::B4]        = "B4";
		names[QPrinter::B5]        = "B5";
		names[QPrinter::B6]        = "B6";
		names[QPrinter::B7]        = "B7";
		names[QPrinter::B8]        = "B8";
		names[QPrinter::B9]        = "B9";
		names[QPrinter::B10]       = "B10";
		names[QPrinter::C5E]       = "C5E";
		names[QPrinter::DLE]       = "DLE";
		names[QPrinter::Executive] = "Executive";
		names[QPrinter::Folio]     = "Folio";
		names[QPrinter::Ledger]    = "Ledger";
		names[QPrinter::Legal]     = "Legal";
		names[QPrinter::Letter]    = "Letter";
		names[QPrinter::Tabloid]   = "Tabloid";
		names[QPrinter::Comm10E]   = "US Common #10 Envelope";
		names[QPrinter::Custom]    = "Custom";
	}
	return names;
}


MapPrinter::MapPrinter(Map& map, const MapView* view, QObject* parent)
: QObject(parent),
  MapPrinterConfig(map.printerConfig()),
  map(map),
  view(view),
  target(nullptr)
{
	scale_adjustment = map.getScaleDenominator() / qreal(options.scale);
	updatePaperDimensions();
	connect(&map.getGeoreferencing(), &Georeferencing::transformationChanged, this, &MapPrinter::mapScaleChanged);
}

MapPrinter::~MapPrinter()
{
	// Do not remove.
}

void MapPrinter::saveConfig() const
{
	map.setPrinterConfig(*this);
}

// slot
void MapPrinter::setTarget(const QPrinterInfo* new_target)
{
	if (new_target != target)
	{
		const QPrinterInfo* old_target = target;
		if (!new_target)
			target = new_target;
		else if (new_target == pdfTarget())
			target = new_target;
		else if (new_target == imageTarget())
			target = new_target;
		else
		{
			// We don't own this target, so we need to make a copy.
			target_copy = *new_target;
			target = &target_copy;
		}
		
		if (old_target == imageTarget() || new_target == imageTarget())
		{
			// No page margins. Will emit pageFormatChanged( ).
			setCustomPaperSize(page_format.page_rect.size());
		}
		else if (page_format.paper_size != QPrinter::Custom)
		{
			updatePaperDimensions();
			emit pageFormatChanged(page_format);
		}
		
		emit targetChanged(target);
	}
}

std::unique_ptr<QPrinter> MapPrinter::makePrinter() const
{
	std::unique_ptr<QPrinter> printer;
	if (!target)
	{
		printer.reset(new QPrinter(QPrinter::HighResolution));
	}
	else if (isPrinter())
	{
		printer.reset(new QPrinter(*target, QPrinter::HighResolution));
	}
	else if (options.color_mode == MapPrinterOptions::DeviceCmyk)
	{
		printer.reset(new AdvancedPdfPrinter(*target, QPrinter::HighResolution));
	}
	else
	{
		printer.reset(new QPrinter(*target, QPrinter::HighResolution));
		printer->setOutputFormat(QPrinter::PdfFormat);
	}
	
	if (!printer->isValid())
	{
		printer.reset();
		return printer;
	}
	
	// Color can only be changed in (native) properties dialogs. This is the default.
	printer->setColorMode(separationsModeSelected() ? QPrinter::GrayScale : QPrinter::Color);
	if (printer->outputFormat() == QPrinter::NativeFormat)
	{
		PlatformPrinterProperties::restore(printer.get(), native_data);
	}
	
	printer->setDocName(::OpenOrienteering::MapPrinter::tr("- Map -"));
	printer->setFullPage(true);
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		printer->setOrientation(QPrinter::Portrait);
	}
	else
	{
		printer->setPaperSize(QPrinter::PaperSize(page_format.paper_size));
		printer->setOrientation((page_format.orientation == MapPrinterPageFormat::Portrait) ? QPrinter::Portrait : QPrinter::Landscape);
	}
	printer->setResolution(options.resolution);
	
	if (page_format.paper_size == QPrinter::Custom || !isPrinter())
	{
		printer->setPageMargins(0.0, 0.0, 0.0, 0.0, QPrinter::Millimeter);
	}
	
	return printer;
}

bool MapPrinter::isPrinter() const
{
	bool is_printer = target
	                  && target != imageTarget()
	                  && target != pdfTarget();
	return is_printer;
}

// slot
void MapPrinter::setPrintArea(const QRectF& area)
{
	if (print_area != area && area.left() < area.right() && area.top() < area.bottom())
	{
		print_area = area;
		
		if (target == imageTarget() && print_area.size() != page_format.paper_dimensions)
			setCustomPaperSize(print_area.size() * scale_adjustment);
		
		updatePageBreaks();
		
		emit printAreaChanged(print_area);
	}
}

// slot
void MapPrinter::setPaperSize(const int size)
{
	if (page_format.paper_size != size)
	{
		if (size == QPrinter::Custom)
		{
			setCustomPaperSize(page_format.paper_dimensions);
		}
		else
		{
			if ( page_format.paper_size == QPrinter::Custom &&
			     page_format.paper_dimensions.width() > page_format.paper_dimensions.height() )
			{
				// After QPrinter::Custom, determine orientation from actual dimensions.
				page_format.orientation = MapPrinterPageFormat::Landscape;
			}
			page_format.paper_size = size;
			updatePaperDimensions();
		}
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setCustomPaperSize(const QSizeF dimensions)
{
	if ((page_format.paper_size != QPrinter::Custom || 
	     page_format.paper_dimensions != dimensions) &&
	     ! dimensions.isEmpty())
	{
		page_format.paper_size = QPrinter::Custom;
		page_format.orientation = MapPrinterPageFormat::Portrait;
		page_format.paper_dimensions = dimensions;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setPageOrientation(const MapPrinterPageFormat::Orientation orientation)
{
	if (page_format.paper_size == QPrinter::Custom)
	{
		// do nothing
		emit pageFormatChanged(page_format);
	}
	else if (page_format.orientation != orientation)
	{
		page_format.orientation = orientation;
		page_format.paper_dimensions.transpose();
		updatePaperDimensions();
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setOverlap(qreal h_overlap, qreal v_overlap)
{
	if (page_format.h_overlap != h_overlap || page_format.v_overlap != v_overlap)
	{
		page_format.h_overlap = qMax(qreal(0), qMin(h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax(qreal(0), qMin(v_overlap, page_format.page_rect.height()));
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
}

void MapPrinter::updatePaperDimensions()
{
	if (target == imageTarget() && page_format.paper_size == QPrinter::Custom)
	{
		// No margins, no need to query QPrinter.
		page_format.page_rect = QRectF(QPointF(0.0, 0.0), page_format.paper_dimensions);
		page_format.h_overlap = qMax(qreal(0), qMin(page_format.h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax(qreal(0), qMin(page_format.v_overlap, page_format.page_rect.height()));
		updatePageBreaks();
		return;
	}
	
	QPrinter* printer = target ? new QPrinter(*target, QPrinter::HighResolution)
	                           : new QPrinter(QPrinter::HighResolution);
	if (!printer->isValid() || target == imageTarget() || target == pdfTarget())
		printer->setOutputFormat(QPrinter::PdfFormat);
	  
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		page_format.orientation = (printer->orientation() == QPrinter::Portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape; 
	}
	else
	{
		printer->setPaperSize(QPrinter::PaperSize(page_format.paper_size));
		printer->setOrientation((page_format.orientation == MapPrinterPageFormat::Portrait) ? QPrinter::Portrait : QPrinter::Landscape);
	}
	
	page_format.page_rect = printer->paperRect(QPrinter::Millimeter);
	page_format.paper_dimensions = page_format.page_rect.size();
	
	if ( target != imageTarget() && target != pdfTarget() &&
		 page_format.paper_size != QPrinter::Custom )
	{
		qreal left, top, right, bottom;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
		page_format.page_rect.adjust(left, top, -right, -bottom);
	}
	page_format.h_overlap = qMax(qreal(0), qMin(page_format.h_overlap, page_format.page_rect.width()));
	page_format.v_overlap = qMax(qreal(0), qMin(page_format.v_overlap, page_format.page_rect.height()));
	
	delete printer;
	updatePageBreaks();
}

// slot
void MapPrinter::setResolution(int dpi)
{
	if (dpi > 0 && options.resolution != dpi)
	{
		options.resolution = dpi;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setScale(const unsigned int value)
{
	if (options.scale != value)
	{
		Q_ASSERT(value > 0);
		options.scale = value;
		scale_adjustment = map.getScaleDenominator() / qreal(value);
		updatePageBreaks();
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setMode(const MapPrinterOptions::MapPrinterMode mode)
{
	if (options.mode != mode)
	{
		options.mode = mode;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintTemplates(const bool visible)
{
	if (options.show_templates != visible)
	{
		options.show_templates = visible;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintGrid(const bool visible)
{
	if (options.show_grid != visible)
	{
		options.show_grid = visible;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setSimulateOverprinting(bool enabled)
{
	if (enabled && !map.hasSpotColors())
	{
		options.simulate_overprinting = false;
		emit optionsChanged(options);
	}
	else if (options.simulate_overprinting != enabled)
	{
		options.simulate_overprinting = enabled;
		emit optionsChanged(options);
	}
}

void MapPrinter::setColorMode(MapPrinterOptions::ColorMode color_mode)
{
	if (options.color_mode != color_mode)
	{
		options.color_mode = color_mode;
		emit optionsChanged(options);
	}
}

bool MapPrinter::isOutputEmpty() const
{
	return (
	  (map.getNumObjects() == 0 || (view && !view->effectiveMapVisibility().visible)) &&
	  (!options.show_templates || map.getNumTemplates() == 0) &&
	  !options.show_grid
	);
}

void MapPrinter::updatePageBreaks()
{
	Q_ASSERT(print_area.left() <= print_area.right());
	Q_ASSERT(print_area.top() <= print_area.bottom());
	
	// This whole implementation needs to deal with FP precision issues
	
	h_page_pos.clear();
	qreal h_pos = print_area.left();
	h_page_pos.push_back(h_pos);
	const qreal h_overlap = page_format.h_overlap / scale_adjustment;
	const qreal page_width = page_format.page_rect.width() / scale_adjustment - h_overlap;
	
	const qreal right_bound = print_area.right() - h_overlap - 0.05;
	if (page_width >= 0.01)
	{
		for (h_pos += page_width; h_pos < right_bound; h_pos += page_width)
			h_page_pos.push_back(h_pos);
		
		// Center the print area on the pages total area.
		// Don't pre-calculate this offset to avoid FP precision problems
		const qreal h_offset = 0.5 * (h_pos + h_overlap - print_area.right());
		for (auto& pos : h_page_pos)
			pos -= h_offset;
	}
	
	v_page_pos.clear();
	qreal v_pos = print_area.top();
	v_page_pos.push_back(v_pos);
	const qreal v_overlap = page_format.v_overlap / scale_adjustment;
	const qreal page_height = page_format.page_rect.height() / scale_adjustment - v_overlap;
	const qreal bottom_bound = print_area.bottom() - v_overlap - 0.05;
	if (page_height >= 0.01)
	{
		for (v_pos += page_height; v_pos < bottom_bound; v_pos += page_height)
			v_page_pos.push_back(v_pos);
		
		// Don't pre-calculate offset to avoid FP precision problems
		const qreal v_offset = 0.5 * (v_pos + v_overlap - print_area.bottom());
		for (auto& pos : v_page_pos)
			pos -= v_offset;
	}
}

void MapPrinter::mapScaleChanged()
{
	auto value = qreal(map.getScaleDenominator()) / options.scale;
	if (!qFuzzyCompare(scale_adjustment, value))
	{
		scale_adjustment = value;
		updatePageBreaks();
		emit optionsChanged(options);
	}
}

void MapPrinter::takePrinterSettings(const QPrinter* printer)
{
	if (!printer) return;

	MapPrinterPageFormat f(*printer);
	if (target == pdfTarget() || target == imageTarget())
	{
		f.page_rect = QRectF(QPointF(0.0, 0.0), f.paper_dimensions);
	}
	
	if (f != page_format)
	{
		page_format.paper_size = f.paper_size;
		page_format.orientation = f.orientation;
		page_format.paper_dimensions = f.paper_dimensions;
		page_format.page_rect = f.page_rect;
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}

	setResolution(printer->resolution());
	
	if (printer->outputFormat() == QPrinter::NativeFormat)
	{
		PlatformPrinterProperties::save(printer, native_data);
	}
}

// local
void drawBuffer(QPainter* device_painter, const QImage* page_buffer)
{
	Q_ASSERT(page_buffer);
	
	device_painter->save();
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	device_painter->drawImage(0, 0, *page_buffer);
	device_painter->restore();
}

void MapPrinter::drawPage(QPainter* device_painter, const QRectF& page_extent, bool white_background, QImage* page_buffer) const
{
	device_painter->save();
	
	QPainter* painter = device_painter;
	
	// Logical units per mm
	const qreal units_per_mm = options.resolution / 25.4;
	
	const auto render_hints = device_painter->renderHints()
	                          | QPainter::Antialiasing
	                          | QPainter::SmoothPixmapTransform;
	
	// Determine transformation and clipping for page extent and region
	const auto page_extent_transform = [this, units_per_mm, page_extent]() {
		// Translate for top left page margin 
		auto transform = QTransform::fromScale(units_per_mm, units_per_mm);
		transform.translate(page_format.page_rect.left(), page_format.page_rect.top());
		// Convert native map scale to print scale
		transform.scale(scale_adjustment, scale_adjustment);
		// Translate and clip for margins and print area
		transform.translate(-page_extent.left(), -page_extent.top());
		return transform;
	}();
	
	const auto page_region_used = page_extent.intersected(print_area);
	
	
	/*
	 * Analyse need for page buffer
	 * 
	 * Painting raster images with opacity is not supported on print devices.
	 * This can only be solved by merging the images before sending them to
	 * the printer. For the map itself, this may result in loss of sharpness
	 * and increase in data volume.
	 * 
	 * In vector mode, a page buffer (raster image) is used to collect background
	 * templates and raster foreground templates. After this buffer is sent to
	 * to the printer, map, grid, and
	 * non-raster foreground templates are drawn on top of it.
	 * 
	 * In raster mode, all map features are drawn to in regular order to
	 * temporary images first.
	 * 
	 * When the target is an image, use the temporary image to enforce the given
	 * resolution.
	 */
	bool use_buffer_for_map = (options.mode == MapPrinterOptions::Raster || target == imageTarget());
	bool use_buffer_for_background = use_buffer_for_map && options.show_templates;
	bool use_buffer_for_foreground = use_buffer_for_map && options.show_templates;
	if (view && options.show_templates)
	{
		if (!use_buffer_for_background)
		{
			for (int i = 0; i < map.getFirstFrontTemplate() && !use_buffer_for_background; ++i)
			{
				if (map.getTemplate(i)->isRasterGraphics())
				{
					auto visibility = view->getTemplateVisibility(map.getTemplate(i));
					use_buffer_for_background = visibility.visible && visibility.opacity < 1.0f;
				}
			}
		}
		if (!use_buffer_for_foreground)
		{
			for (int i = map.getFirstFrontTemplate(); i < map.getNumTemplates() && !use_buffer_for_foreground; ++i)
			{
				if (map.getTemplate(i)->isRasterGraphics())
				{
					auto visibility = view->getTemplateVisibility(map.getTemplate(i));
					use_buffer_for_foreground = visibility.visible && visibility.opacity < 1.0f;
				}
			}
		}
	}
	
	// Prepare buffer if required
	bool use_page_buffer = use_buffer_for_map ||
	                       use_buffer_for_background ||
	                       use_buffer_for_foreground;
	QImage scoped_buffer;
	if (use_page_buffer && !page_buffer)
	{
		int w = qCeil(page_format.paper_dimensions.width() * units_per_mm);
		int h = qCeil(page_format.paper_dimensions.height() * units_per_mm);
#if defined (Q_OS_MACOS)
		if (device_painter->device()->physicalDpiX() == 0)
		{
			// Possible Qt bug, since according to QPaintDevice documentation,
			// "if the physicalDpiX() doesn't equal the logicalDpiX(),
			// the corresponding QPaintEngine must handle the resolution mapping"
			// which doesn't seem to happen here.
			qreal corr = device_painter->device()->logicalDpiX() / 72.0;
			w = qCeil(page_format.paper_dimensions.width() * units_per_mm * corr);
			h = qCeil(page_format.paper_dimensions.height() * units_per_mm * corr);
		}
#endif
		
		scoped_buffer = QImage(w, h, QImage::Format_RGB32);
		if (scoped_buffer.isNull())
		{
			// Allocation failed
			device_painter->restore();
			device_painter->end(); // Signal error
			return;
		}
		
		page_buffer = &scoped_buffer;
		painter = new QPainter(page_buffer);
	}
	
	/*
	 * Prepare the common background
	 */
	if (use_page_buffer)
	{
		page_buffer->fill(QColor(Qt::white));
	}
	else if (white_background)
	{
		painter->fillRect(QRect(0, 0, painter->device()->width(), painter->device()->height()), Qt::white);
	}
	
	painter->setRenderHints(render_hints);
	painter->setTransform(page_extent_transform);
	painter->setClipRect(page_region_used, Qt::ReplaceClip);
	
	/*
	 * Draw templates in the background
	 */
	if (options.show_templates)
	{
		map.drawTemplates(painter, page_region_used, 0, map.getFirstFrontTemplate() - 1, view, false);
		if (vectorModeSelected() && use_buffer_for_foreground)
		{
			for (int i = map.getFirstFrontTemplate(); i < map.getNumTemplates(); ++i)
			{
				if (map.getTemplate(i)->isRasterGraphics())
					map.drawTemplates(painter, page_region_used, i, i, view, false);
			}
		}
	}
	
	if (use_buffer_for_background && !use_buffer_for_map)
	{
		// Flush the buffer, reset painter
		delete painter;
		drawBuffer(device_painter, page_buffer);
		
		painter = device_painter;
		painter->setRenderHints(render_hints);
		painter->setTransform(page_extent_transform);
		painter->setClipRect(page_region_used, Qt::ReplaceClip);
	}
	
	/*
	 * Draw the map
	 */
	if (!view || view->effectiveMapVisibility().visible)
	{
		QImage map_buffer;
		QPainter* map_painter = painter;
		if (use_buffer_for_map)
		{
			// Draw map into a temporary buffer first which is printed with the map's opacity later.
			// This prevents artifacts with overlapping objects.
			if (painter == device_painter)
			{
				map_buffer = *page_buffer; // Use existing buffer
			}
			else
			{
				map_buffer = QImage(page_buffer->size(), QImage::Format_ARGB32_Premultiplied);
			}
			map_buffer.fill(QColor(Qt::transparent));
			
			map_painter = new QPainter(&map_buffer);
			map_painter->setRenderHints(painter->renderHints());
			map_painter->setTransform(painter->transform());
		}
		
		RenderConfig config = { map, page_region_used, units_per_mm * scale_adjustment, RenderConfig::NoOptions, 1.0 };
		
		if (rasterModeSelected() && options.simulate_overprinting)
		{
			map.drawOverprintingSimulation(map_painter, config);
		}
		else
		{
			if (vectorModeSelected() && view)
				config.opacity = view->effectiveMapVisibility().opacity;
		
			map.draw(map_painter, config);
		}
			
		if (map_painter != painter)
		{
			// Flush the buffer
			delete map_painter;
			map_painter = nullptr;
			
			// Print buffer with map opacity
			painter->save();
			painter->resetTransform();
			if (view)
				painter->setOpacity(view->effectiveMapVisibility().opacity);
			painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
			painter->drawImage(0, 0, map_buffer);
			painter->restore();
		}
	}
	
	/*
	 * Draw the grid
	 */
	if (options.show_grid)
		map.drawGrid(painter, print_area, false); // Maybe replace by page_region_used?
	
	/*
	 * Draw the foreground templates
	 */
	if (options.show_templates)
	{
		if (vectorModeSelected() && use_buffer_for_foreground)
		{
			for (int i = map.getFirstFrontTemplate(); i < map.getNumTemplates(); ++i)
			{
				if (!map.getTemplate(i)->isRasterGraphics())
					map.drawTemplates(painter, page_region_used, i, i, view, false);
			}
		}
		else
		{
			if (use_buffer_for_foreground && !use_buffer_for_map)
			{
				page_buffer->fill(QColor(Qt::transparent));
				painter = new QPainter(page_buffer);
				painter->setRenderHints(device_painter->renderHints());
				painter->setTransform(page_extent_transform);
				painter->setClipRect(page_region_used, Qt::ReplaceClip);
			}
			map.drawTemplates(painter, page_region_used, map.getFirstFrontTemplate(), map.getNumTemplates() - 1, view, false);
		}
	}

	/*
	 * Cleanup: If a temporary buffer has been used, paint it on the device painter
	 */
	if (painter != device_painter)
	{
		delete painter;
		painter = nullptr;
		device_painter->resetTransform();
		drawBuffer(device_painter, page_buffer);
	}
	device_painter->restore();
}

void MapPrinter::drawSeparationPages(QPrinter* printer, QPainter* device_painter, const QRectF& page_extent) const
{
	Q_ASSERT(printer->colorMode() == QPrinter::GrayScale);
	
	device_painter->save();
	
	device_painter->setRenderHint(QPainter::Antialiasing);
	
	// Translate for top left page margin 
	qreal scale = options.resolution / 25.4; // Dots per mm
	device_painter->scale(scale, scale);
	device_painter->translate(page_format.page_rect.left(), page_format.page_rect.top());
	
	// Convert native map scale to print scale
	if (scale_adjustment != 1.0)
	{
		scale *= scale_adjustment;
		device_painter->scale(scale_adjustment, scale_adjustment);
	}
	
	// Translate and clip for margins and print area
	device_painter->translate(-page_extent.left(), -page_extent.top());
	device_painter->setClipRect(page_extent.intersected(print_area), Qt::ReplaceClip);
	
	bool need_new_page = false;
	for (int i = map.getNumColors() - 1; i >= 0; --i)
	{
		const MapColor* color = map.getColor(i);
		if (color->getSpotColorMethod() == MapColor::SpotColor)
		{
			if (need_new_page)
			{
				printer->newPage();
			}
			
			RenderConfig config = { map, page_extent, scale, RenderConfig::NoOptions, 1.0 };
			map.drawColorSeparation(device_painter, config, color);
			need_new_page = true;
		}
	}
	
	device_painter->restore();
}

bool MapPrinter::printMap(QPrinter* printer)
{
	// Printer settings may have been changed by preview or application.
	// We need to use them for printing.
	printer->setFullPage(true);
	takePrinterSettings(printer);
	
	QSizeF extent_size = page_format.page_rect.size() / scale_adjustment;
	QPainter painter(printer);
	
#if defined(Q_OS_WIN)
	// Workaround for Wine
	if (printer->resolution() == 0)
	{
		return false;
	}
	
	/* Non-opaque drawing of vector data to the raw printer will trigger
	 * internal rasterization in Qt.
	 */
	auto engine_will_rasterize = [](const Map& map, const MapView* view, const MapPrinterOptions& options)->bool {
		if (!view)
			return false;
		
		if (options.mode != MapPrinterOptions::Raster)
		{
			auto visibility = view->effectiveMapVisibility();
			if (visibility.visible && visibility.opacity < 1.0)
				return true;
		}
			
		for (int i = 0; i < map.getNumTemplates(); ++i)
		{
			auto temp = map.getTemplate(i);
			if (temp->isRasterGraphics())
				continue;
			auto visibility = view->getTemplateVisibility(temp);
			if (visibility.visible && visibility.opacity < 1.0)
				return true;
		}
		
		return false;
	};

	if (printer->paintEngine()->type() == QPaintEngine::Windows
	    && !engine_will_rasterize(map, view, options) )
	{
		/* QWin32PrintEngine will (have to) do rounding when passing coordinates
		 * to GDI, using the device's reported logical resolution.
		 * We establish an MM_ISOTROPIC transformation from a higher resolution
		 * to avoid the loss of precision due to this rounding.
		 * However, output of rasterization produced by Qt fails when the
		 * MM_ISOTROPIC transformation is active.
		 */
		
		QWin32PrintEngine* engine = static_cast<QWin32PrintEngine*>(printer->printEngine());
		HDC dc = engine->getDC();
		
		// The high resolution in units per millimeter
		const int hires_ppmm = 1000;
		
		// The paper dimensions in high resolution units
		const int hires_width  = qRound(page_format.page_rect.width() * hires_ppmm);
		const int hires_height = qRound(page_format.page_rect.height() * hires_ppmm);
		
		// The physical paper dimensions in device units
		const int phys_width   = GetDeviceCaps(dc, PHYSICALWIDTH);
		const int phys_height  = GetDeviceCaps(dc, PHYSICALHEIGHT);
		
		// The physical printing offset in device units
		const int phys_off_x   = GetDeviceCaps(dc, PHYSICALOFFSETX);
		const int phys_off_y   = GetDeviceCaps(dc, PHYSICALOFFSETY);
		// (Needed to work around an unexpected offset, maybe related to QTBUG-5363)
		
		if (phys_width > 0)
		{
			// Establish the transformation
			SetMapMode (dc, MM_ISOTROPIC);
			SetWindowExtEx(dc, hires_width, hires_height, nullptr);
			SetViewportExtEx(dc, phys_width, phys_height, nullptr);
			SetViewportOrgEx(dc, -phys_off_x, -phys_off_y, nullptr);
			const auto hires_scale = static_cast<qreal>(hires_width) / phys_width;
			painter.scale(hires_scale, hires_scale);
		}
	}
	else if (printer->paintEngine()->type() == QPaintEngine::Picture)
	{
		// Preview: work around for offset, maybe related to QTBUG-5363
		painter.translate(
		    -page_format.page_rect.left() * options.resolution / 25.4,
		    -page_format.page_rect.top() * options.resolution / 25.4   );
	}
#endif
	
	cancel_print_map = false;
	int step = 0;
	auto num_steps = v_page_pos.size() * h_page_pos.size();
	const QString message_template( (options.mode == MapPrinterOptions::Separations) ?
	  ::OpenOrienteering::MapPrinter::tr("Processing separations of page %1...") :
	  ::OpenOrienteering::MapPrinter::tr("Processing page %1...") );
	auto message = message_template.arg(1);
	emit printProgress(0, message);
	
	bool need_new_page = false;
	for (auto vpos : v_page_pos)
	{
		if (!painter.isActive())
		{
			break;
		}
		
		for (auto hpos : h_page_pos)
		{
			if (!painter.isActive())
			{
				break;
			}
			
			++step;
			auto progress = qMin(99, qMax(1, int((100 * static_cast<decltype(num_steps)>(step) - 50) / num_steps)));
			emit printProgress(progress, message_template.arg(step));
			
			if (cancel_print_map) /* during printProgress handling */
			{
				painter.end();
				break;
			}
				
			if (need_new_page)
			{
				printer->newPage();
			}
			
			QRectF page_extent = QRectF(QPointF(hpos, vpos), extent_size);
			if (separationsModeSelected())
			{
				drawSeparationPages(printer, &painter, page_extent);
			}
			else
			{
				drawPage(&painter, page_extent, false);
			}
			
			need_new_page = true;
		}
	}
	
	if (cancel_print_map)
	{
		emit printProgress(100, ::OpenOrienteering::MapPrinter::tr("Canceled"));
	}
	else if (!painter.isActive())
	{
		emit printProgress(100, ::OpenOrienteering::MapPrinter::tr("Error"));
		return false;
	}
	else
	{
		emit printProgress(100, ::OpenOrienteering::MapPrinter::tr("Finished"));
	}
	return true;
}

void MapPrinter::cancelPrintMap()
{
	cancel_print_map = true;
}

#endif  // QT_PRINTSUPPORT_LIB


}  // namespace OpenOrienteering
