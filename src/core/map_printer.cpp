/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#include <limits>

#include <QDebug>
#include <QPainter>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#if defined(QT_PRINTSUPPORT_LIB) && defined(Q_OS_WIN)
#include <private/qprintengine_win_p.h>
#endif

#include "../core/map_color.h"
#include "../core/map_view.h"
#include "../map.h"
#include "../settings.h"
#include "../template.h"
#include "../util.h"
#include "../util/xml_stream_util.h"


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



// #### MapPrinterPageFormat ###

MapPrinterPageFormat::MapPrinterPageFormat(QSizeF page_rect_size, qreal margin) 
 :
#ifdef QT_PRINTSUPPORT_LIB
   paper_size(QPrinter::Custom),
#else
   paper_size(-1),
#endif
   orientation(Portrait),
   page_rect(QRectF(QPointF(margin, margin), page_rect_size)),
   h_overlap(5.0), // mm
   v_overlap(5.0)  // mm
{
	const qreal double_margin = 2 * margin;
	paper_dimensions = page_rect.size() + QSizeF(double_margin, double_margin);
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

MapPrinterOptions::MapPrinterOptions(unsigned int scale, unsigned int resolution, MapPrinterMode mode)
 : scale(scale),
   resolution(resolution),
   mode(mode),
   show_templates(false),
   show_grid(false),
   simulate_overprinting(false)
{
	// nothing
}



// ### MapPrinterConfig ###

MapPrinterConfig::MapPrinterConfig(const Map& map)
 : printer_name("DEFAULT"),
   print_area(map.calculateExtent()),
   page_format(),                      // Use defaults.
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	options.resolution = 600.0;
	options.scale = map.getScaleDenominator();
	options.show_grid = false;
	options.show_templates = false;
	options.simulate_overprinting = false;
}

MapPrinterConfig::MapPrinterConfig(const Map& map, QXmlStreamReader& xml)
 : printer_name("DEFAULT"),
   print_area(0.0, 0.0, 100.0, 100.0), // Avoid expensive calculation before loading.
   page_format(),                      // Use defaults.
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	XmlElementReader printer_config_element(xml);
	
	options.scale = printer_config_element.attribute<unsigned int>(literal::scale);
	options.resolution = printer_config_element.attribute<unsigned int>(literal::resolution);
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
				if (value == paper_size_names[i])
					page_format.paper_size = i;
			}
#endif
			
			value = page_format_element.attribute<QString>(literal::orientation);
			page_format.orientation =
			  (value == literal::portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape;
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
		qDebug() << "Unsupported map printig mode:" << options.mode;
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


MapPrinter::MapPrinter(Map& map, MapView* view, QObject* parent)
: QObject(parent),
  MapPrinterConfig(map.printerConfig()),
  map(map),
  view(view),
  target(NULL)
{
	scale_adjustment = map.getScaleDenominator() / (qreal) options.scale;
	updatePaperDimensions();
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
		if (new_target == NULL)
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

QPrinter* MapPrinter::makePrinter() const
{
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	if (!printer->isValid())
		printer->setOutputFormat(QPrinter::PdfFormat);
	
	printer->setDocName("MAP"/* FIXME QFileInfo(main_window->getCurrentFilePath()).fileName()*/);
	printer->setFullPage(true);
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		printer->setOrientation(QPrinter::Portrait);
	}
	else
	{
		printer->setPaperSize((QPrinter::PaperSize)page_format.paper_size);
		printer->setOrientation((page_format.orientation == MapPrinterPageFormat::Portrait) ? QPrinter::Portrait : QPrinter::Landscape);
	}
	printer->setColorMode(separationsModeSelected() ? QPrinter::GrayScale : QPrinter::Color);
	printer->setResolution(options.resolution);
	
	if (target == imageTarget() || page_format.paper_size == QPrinter::Custom)
		printer->setPageMargins(0.0, 0.0, 0.0, 0.0, QPrinter::Millimeter);
	
	return printer;
}

bool MapPrinter::isPrinter() const
{
	if (target == NULL)
		return false;
	else if (target == pdfTarget())
		return false;
	else if (target == imageTarget())
		return false;
	
	return true;
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
		page_format.h_overlap = qMax((qreal)0.0, qMin(h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax((qreal)0.0, qMin(v_overlap, page_format.page_rect.height()));
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
		page_format.h_overlap = qMax((qreal)0.0, qMin(page_format.h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax((qreal)0.0, qMin(page_format.v_overlap, page_format.page_rect.height()));
		updatePageBreaks();
		return;
	}
	
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	if (!printer->isValid())
		printer->setOutputFormat(QPrinter::PdfFormat);
	  
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		page_format.orientation = (printer->orientation() == QPrinter::Portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape; 
	}
	else
	{
		printer->setPaperSize((QPrinter::PaperSize)page_format.paper_size);
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
	page_format.h_overlap = qMax((qreal)0.0, qMin(page_format.h_overlap, page_format.page_rect.width()));
	page_format.v_overlap = qMax((qreal)0.0, qMin(page_format.v_overlap, page_format.page_rect.height()));
	
	delete printer;
	updatePageBreaks();
}

// slot
void MapPrinter::setResolution(const unsigned int dpi)
{
	Q_ASSERT(dpi >= 0.05f);
	if (options.resolution != dpi)
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
		scale_adjustment = map.getScaleDenominator() / (qreal) value;
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
void MapPrinter::setPrintTemplates(const bool visible, MapView* view)
{
	if (options.show_templates != visible || this->view != view)
	{
		options.show_templates = visible;
		this->view = visible ? view : NULL;
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

bool MapPrinter::isOutputEmpty() const
{
	return (
	  (map.getNumObjects() == 0 || (view != NULL && (view->getMapVisibility()->visible == false || view->getMapVisibility()->opacity < 0.0005f))) &&
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
		for (std::vector<qreal>::iterator it=h_page_pos.begin(); it != h_page_pos.end(); ++it)
			*it -= h_offset;
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
		for (std::vector<qreal>::iterator it=v_page_pos.begin(); it != v_page_pos.end(); ++it)
			*it -= v_offset;
	}
}

void MapPrinter::takePrinterSettings(const QPrinter* printer)
{
	MapPrinterPageFormat f;
	f.paper_size  = printer->paperSize();
	f.orientation = (printer->orientation() == QPrinter::Portrait) ? MapPrinterPageFormat::Portrait : MapPrinterPageFormat::Landscape;
	f.page_rect   = printer->paperRect(QPrinter::Millimeter); // temporary
	f.paper_dimensions = f.page_rect.size();
	
	if (target != pdfTarget() && target != imageTarget())
	{
		qreal left, top, right, bottom;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
		f.page_rect.adjust(left, top, -right, -bottom);
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
}

// local
void drawBuffer(QPainter* device_painter, const QImage* page_buffer, qreal pixel2units)
{
	Q_ASSERT(page_buffer);
	
	device_painter->save();
	
#if defined(Q_OS_MAC)
	// Workaround for miss-scaled image export output
	int logical_dpi = device_painter->device()->logicalDpiX();
	if (logical_dpi != 0)
	{
		int physical_dpi = device_painter->device()->physicalDpiX();
		if (physical_dpi != 0 && logical_dpi != physical_dpi)
		{
			qreal s = (qreal)logical_dpi / (qreal)physical_dpi;
			//qreal s = physical_dpi / logical_dpi;
			device_painter->scale(s, s);
		}
	}
#endif
	
	device_painter->scale(pixel2units, pixel2units);
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	device_painter->drawImage(0, 0, *page_buffer);
	device_painter->restore();
}

void MapPrinter::drawPage(QPainter* device_painter, float units_per_inch, const QRectF& page_extent, bool white_background, QImage* page_buffer) const
{
	device_painter->save();
	
	device_painter->setRenderHint(QPainter::Antialiasing);
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform);
	
	QPainter* painter = device_painter;
	
	// Logical units per mm
	qreal units_per_mm = units_per_inch / 25.4;
	// Image pixels per mm
	qreal pixel_per_mm = options.resolution / 25.4;
	// Scaling from pixels to logical units
	qreal pixel2units = units_per_inch / options.resolution;
	// The current painter's resolution
	qreal scale = units_per_mm;
	
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
	if (view != NULL && options.show_templates)
	{
		if (!use_buffer_for_background)
		{
			for (int i = 0; i < map.getFirstFrontTemplate() && !use_buffer_for_background; ++i)
			{
				if (map.getTemplate(i)->isRasterGraphics())
				{
					TemplateVisibility* visibility = view->getTemplateVisibility(map.getTemplate(i));
					use_buffer_for_background = visibility->visible && visibility->opacity < 1.0f;
				}
			}
		}
		if (!use_buffer_for_foreground)
		{
			for (int i = map.getFirstFrontTemplate(); i < map.getNumTemplates() && !use_buffer_for_foreground; ++i)
			{
				if (map.getTemplate(i)->isRasterGraphics())
				{
					TemplateVisibility* visibility = view->getTemplateVisibility(map.getTemplate(i));
					use_buffer_for_foreground = visibility->visible && visibility->opacity < 1.0f;
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
		scale = pixel_per_mm;
		int w = qCeil(page_format.paper_dimensions.width() * scale);
		int h = qCeil(page_format.paper_dimensions.height() * scale);
#if defined (Q_OS_MAC)
		if (device_painter->device()->physicalDpiX() == 0)
		{
			// Possible Qt bug, since according to QPaintDevice documentation,
			// "if the physicalDpiX() doesn't equal the logicalDpiX(),
			// the corresponding QPaintEngine must handle the resolution mapping"
			// which doesn't seem to happen here.
			qreal corr = device_painter->device()->logicalDpiX() / 72.0;
			w = qCeil(page_format.paper_dimensions.width() * scale * corr);
			h = qCeil(page_format.paper_dimensions.height() * scale * corr);
		}
#endif
		scoped_buffer = QImage(w, h, QImage::Format_RGB32);
		page_buffer = &scoped_buffer;
		painter = new QPainter(page_buffer);
		painter->setRenderHints(device_painter->renderHints());
	}
	
	/*
	 * Prepare the common background
	 */
	if (use_buffer_for_background)
	{
		page_buffer->fill(QColor(Qt::white));
	}
	else if (white_background)
	{
		painter->fillRect(QRect(0, 0, painter->device()->width(), painter->device()->height()), Qt::white);
	}
	
	/*
	 * One-time setup of transformation and clipping
	 */
	// Translate for top left page margin 
	painter->scale(scale, scale);
	painter->translate(page_format.page_rect.left(), page_format.page_rect.top());
	
	// Convert native map scale to print scale
	if (scale_adjustment != 1.0)
	{
		scale *= scale_adjustment;
		painter->scale(scale_adjustment, scale_adjustment);
	}
	
	// Translate and clip for margins and print area
	painter->translate(-page_extent.left(), -page_extent.top());
	
	QTransform transform = painter->transform();
	
	QRectF page_region_used(page_extent.intersected(print_area));
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
		drawBuffer(device_painter, page_buffer, pixel2units);
		
		painter = device_painter;
		painter->setTransform(transform);
		painter->setClipRect(page_region_used, Qt::ReplaceClip);
	}
	
	/*
	 * Draw the map
	 */
	if (view == NULL || view->getMapVisibility()->visible)
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
		
		if (rasterModeSelected() && options.simulate_overprinting)
			map.drawOverprintingSimulation(map_painter, page_region_used, false, scale, false, false);
		else if (vectorModeSelected() && view)
			map.draw(map_painter, page_region_used, false, scale, false, false, view->getMapVisibility()->opacity);
		else
			map.draw(map_painter, page_region_used, false, scale, false, false);
			
		if (map_painter != painter)
		{
			// Flush the buffer
			delete map_painter;
			map_painter = NULL;
			
			// Print buffer with map opacity
			painter->save();
			painter->resetTransform();
			if (view != NULL)
				painter->setOpacity(view->getMapVisibility()->opacity);
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
				painter->setTransform(transform);
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
		painter = NULL;
		device_painter->resetTransform();
		drawBuffer(device_painter, page_buffer, pixel2units);
	}
	device_painter->restore();
}

void MapPrinter::drawSeparationPages(QPrinter* printer, QPainter* device_painter, float dpi, const QRectF& page_extent) const
{
	Q_ASSERT(printer->colorMode() == QPrinter::GrayScale);
	
	device_painter->save();
	
	device_painter->setRenderHint(QPainter::Antialiasing);
	
	// Translate for top left page margin 
	qreal scale = dpi / 25.4; // Dots per mm
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
			map.drawColorSeparation(device_painter, page_extent, false, scale, false, false, color);
			need_new_page = true;
		}
	}
	
	device_painter->restore();
}

// slot
void MapPrinter::printMap(QPrinter* printer)
{
	int num_steps = v_page_pos.size() * h_page_pos.size();
	int step = 0;
	cancel_print_map = false;
	const QString message_template( (options.mode == MapPrinterOptions::Separations) ?
	  tr("Processing separations of page %1...") :
	  tr("Processing page %1...") );
	
	// Printer settings may have been changed by preview or application.
	// We need to use them for printing.
	printer->setFullPage(true);
	takePrinterSettings(printer);
	
	QSizeF extent_size = page_format.page_rect.size() / scale_adjustment;
	QPainter painter(printer);
	
#if defined(Q_OS_WIN)
	float resolution = (float)options.resolution;
	
	if (printer->paintEngine()->type() == QPaintEngine::Windows)
	{
		/* QWin32PrintEngine will (have to) do rounding when passing coordinates
		 * to GDI, using the device's reported logical resolution.
		 * We establish an MM_ISOTROPIC transformation from a higher resolution
		 * to avoid the loss of precision due to this rounding.
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
			SetWindowExtEx(dc, hires_width, hires_height, NULL);
			SetViewportExtEx(dc, phys_width, phys_height, NULL);
			SetViewportOrgEx(dc, -phys_off_x, -phys_off_y, NULL);
			resolution *= ((double)hires_width / phys_width);
		}
	}
	else if (printer->paintEngine()->type() == QPaintEngine::Picture)
	{
		// Preview: work around for offset, maybe related to QTBUG-5363
		painter.translate(
		    -page_format.page_rect.left()*resolution / 25.4,
		    -page_format.page_rect.top()*resolution / 25.4   );
	}
#endif
	
	bool need_new_page = false;
	Q_FOREACH(qreal vpos, v_page_pos)
	{
		if (!painter.isActive())
		{
			break;
		}
		
		Q_FOREACH(qreal hpos, h_page_pos)
		{
			if (!painter.isActive())
			{
				break;
			}
			
			++step;
			int progress = qMin(99, qMax(1, (100*step-50)/num_steps));
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
				drawSeparationPages(printer, &painter, (float)options.resolution, page_extent);
			}
			else
			{
				drawPage(&painter, (float)options.resolution, page_extent, false);
			}
			
			need_new_page = true;
		}
	}
	
	if (cancel_print_map)
	{
		emit printProgress(100, tr("Canceled"));
		return;
	}
	else if (!painter.isActive())
	{
		emit printProgress(100, tr("Error"));
		return;
	}
	
	emit printProgress(100, tr("Finished"));
}

void MapPrinter::cancelPrintMap()
{
	cancel_print_map = true;
}

#endif
