/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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

#include "../util.h"
#include "../map.h"
#include "../settings.h"

//### MapPrinterPageFormat ###

/**
 * Returns true if the MapPrinterPageFormat objects are not equal.
 */
inline
bool operator!=(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return 
	  lhs.paper_size != rhs.paper_size ||
	  lhs.orientation != rhs.orientation ||
	  lhs.paper_dimensions != rhs.paper_dimensions ||
	  lhs.page_rect != rhs.page_rect;
}

/**
 * Returns true if the MapPrinterPageFormat objects are equal.
 */
inline
bool operator==(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return !(lhs != rhs);
}



//### MapPrinter ###

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

MapPrinter::MapPrinter(Map& map, QObject* parent)
: QObject(parent), 
  map(map),
  view(NULL),
  target(NULL)
{
	options.scale = map.getScaleDenominator();
	options.scale_adjustment = 1.0;
	if (map.arePrintParametersSet())
	{
		float left, top, width, height;
		bool different_scale_enabled;
		int different_scale;
		int int_orientation, int_paper_size;
		float resolution_f;
		map.getPrintParameters(
		  int_orientation, int_paper_size, resolution_f,
		  options.show_templates, options.show_grid, options.simulate_overprinting,
		  center, left, top, width, height,
		  different_scale_enabled, different_scale
		);
		page_format.orientation = (QPrinter::Orientation)int_orientation;
		page_format.paper_size = (QPrinter::PaperSize)int_paper_size;
		print_area = QRectF(left, top, width, height);
		options.resolution = qRound(resolution_f);
		if (different_scale_enabled)
		{
			options.scale = qMax(1, different_scale);
			options.scale_adjustment = map.getScaleDenominator() / (qreal) options.scale;
		}
	}
	else
	{
		page_format.paper_size = QPrinter::Custom;
		options.resolution = 600; // dpi
		options.show_templates = false;
		options.show_grid = false;
		options.simulate_overprinting = false;
		center = true;
		print_area = QRectF(map.calculateExtent());
		page_format.orientation = (print_area.width() > print_area.height()) ? QPrinter::Landscape : QPrinter::Portrait;
	}
	updatePaperDimensions();
}

MapPrinter::~MapPrinter()
{
	// Do not remove.
}

void MapPrinter::saveParameters() const
{
	map.setPrintParameters(
	  page_format.orientation, page_format.paper_size, (float)options.resolution,
	  options.show_templates, options.show_grid, options.simulate_overprinting,
	  center,
	  print_area.left(), print_area.top(), print_area.width(), print_area.height(),
	  (options.scale != map.getScaleDenominator()), options.scale
	);
}

// slot
void MapPrinter::setTarget(const QPrinterInfo* target)
{
	if (target == NULL)
		target = target;
	else if (target == pdfTarget())
		target = target;
	else if (target == imageTarget())
		target = target;
	else
	{
		// We don't own this target, so we need to make a copy.
		target_copy = *target;
		target = &target_copy;
	}
	emit targetChanged(target);
}

QPrinter* MapPrinter::makePrinter() const
{
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	
	printer->setDocName("MAP"/* FIXME QFileInfo(main_window->getCurrentFilePath()).fileName()*/);
	printer->setFullPage(true);
	if (page_format.paper_size == QPrinter::Custom)
	{	
		printer->setPaperSize(print_area.size(), QPrinter::Millimeter);
		printer->setOrientation(QPrinter::Portrait);
	}
	else
	{
		printer->setPaperSize(page_format.paper_size);
		printer->setOrientation(page_format.orientation);
	}
	printer->setColorMode(QPrinter::Color);
	printer->setResolution(options.resolution);
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
	if (print_area != area)
	{
		print_area = area;
		updatePageBreaks();
		emit printAreaChanged(print_area);
	}
}

// slot
void MapPrinter::setPaperSize(QPrinter::PaperSize size)
{
	if (page_format.paper_size != size)
	{
		page_format.paper_size = size;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setCustomPaperSize(QSizeF dimensions)
{
	if (page_format.paper_size != QPrinter::Custom && page_format.paper_dimensions != dimensions)
	{
		page_format.paper_size = QPrinter::Custom;
		page_format.orientation = QPrinter::Portrait;
		page_format.paper_dimensions = dimensions;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setPageOrientation(QPrinter::Orientation orientation)
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

void MapPrinter::updatePaperDimensions()
{
	QPrinter printer(QPrinter::HighResolution);
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer.setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		page_format.orientation = printer.orientation(); 
	}
	else
	{
		printer.setPaperSize(page_format.paper_size);
		printer.setOrientation(page_format.orientation);
	}
	
	page_format.page_rect = printer.paperRect(QPrinter::Millimeter); // temporary
	page_format.paper_dimensions = page_format.page_rect.size();
	
	qreal left, top, right, bottom;
	printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	page_format.page_rect.adjust(left, top, -right, -bottom);
	
	updatePageBreaks();
}

// slot
void MapPrinter::setResolution(float dpi)
{
	Q_ASSERT(dpi >= 0.05f);
	if (options.resolution != dpi)
	{
		options.resolution = dpi;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setScale(int value)
{
	if (options.scale != value)
	{
		Q_ASSERT(value > 0);
		options.scale = value;
		options.scale_adjustment = map.getScaleDenominator() / (qreal) value;
		updatePageBreaks();
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintTemplates(bool visible, MapView* view)
{
	if (options.show_templates != visible || view != view)
	{
		options.show_templates = visible;
		view = visible ? view : NULL;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintGrid(bool visible)
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
	if (options.simulate_overprinting != enabled)
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
	
	h_page_pos.clear();
	qreal page_width = page_format.page_rect.width() / options.scale_adjustment;
	for (qreal pos = print_area.left(); pos < print_area.right(); pos += page_width)
		h_page_pos.push_back(pos);
	
	v_page_pos.clear();
	qreal page_height = page_format.page_rect.height() / options.scale_adjustment;
	for (qreal pos = print_area.top(); pos < print_area.bottom(); pos += page_height)
		v_page_pos.push_back(pos);
}

void MapPrinter::takePrinterSettings(const QPrinter* printer)
{
	MapPrinterPageFormat f;
	f.paper_size  = printer->paperSize();
	f.orientation = printer->orientation();
	f.page_rect   = printer->paperRect(QPrinter::Millimeter); // temporary
	f.paper_dimensions = f.page_rect.size();
	
	qreal left, top, right, bottom;
	printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	f.page_rect.adjust(left, top, -right, -bottom);
	
	if (f != page_format)
	{
		page_format.paper_size = f.paper_size;
		page_format.orientation = f.orientation;
		page_format.paper_dimensions = f.paper_dimensions;
		page_format.page_rect = f.page_rect;
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
	
// 	setResolution((float)printer->resolution());
}

void MapPrinter::drawPage(QPainter* device_painter, float dpi, const QRectF& page_extent, bool white_background) const
{
	device_painter->save();
	device_painter->setRenderHint(QPainter::Antialiasing);
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform);
	
	// If there is anything transparent to draw,
	// use a temporary image which is drawn on the device printer as last step
	// because painting transparently seems to be unsupported when printing
	bool have_transparency = (view != NULL) && view->getMapVisibility()->visible && view->getMapVisibility()->opacity < 1;
	if (view != NULL)
	{
		for (int i = 0; i < map.getNumTemplates() && !have_transparency; ++i)
		{
			TemplateVisibility* visibility = view->getTemplateVisibility(map.getTemplate(i));
			have_transparency = visibility->visible && visibility->opacity < 1;
		}
	}
	
	bool print_rgb_image = have_transparency || options.simulate_overprinting;
	
	QPainter* painter = device_painter;
	QImage print_buffer;
	QPainter print_buffer_painter;
	if (print_rgb_image)
	{
		print_buffer = QImage(painter->device()->width(), painter->device()->height(), QImage::Format_RGB32);
		print_buffer_painter.begin(&print_buffer);
		painter = &print_buffer_painter;
		white_background = true;
	}
	
	if (white_background)
		painter->fillRect(QRect(0, 0, painter->device()->width(), painter->device()->height()), Qt::white);
	
	// Convert mm to dots
	float scale = dpi / 25.4f;
	
	// Translate for top left page margin 
	painter->scale(scale, scale);
	painter->translate(page_format.page_rect.left(), page_format.page_rect.top());
	
	// Convert native map scale to print scale
	if (options.scale_adjustment != (qreal)1.0)
	{
		scale *= options.scale_adjustment;
		painter->scale(options.scale_adjustment, options.scale_adjustment);
	}
	
	// Translate and clip for margins and print area
	painter->translate(-page_extent.left(), -page_extent.top());
	painter->setClipRect(page_extent, Qt::ReplaceClip);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, 0, map.getFirstFrontTemplate() - 1, view);
	
	if (view == NULL || view->getMapVisibility()->visible)
	{
		if (!print_rgb_image)
			map.draw(painter, page_extent, false, scale, false, false);
		else
		{
			// Draw map into a temporary buffer first which is printed with the map's opacity later.
			// This prevents artifacts with overlapping objects.
			QImage map_buffer(painter->device()->width(), painter->device()->height(), QImage::Format_ARGB32_Premultiplied);
			QPainter buffer_painter;
			buffer_painter.begin(&map_buffer);
			buffer_painter.setRenderHints(painter->renderHints());
			
			// Clear buffer
			buffer_painter.fillRect(map_buffer.rect(), Qt::transparent);
			
			// Draw map with full opacity
			buffer_painter.setTransform(painter->transform());
			if (options.simulate_overprinting)
				map.drawOverprintingSimulation(&buffer_painter, page_extent, false, scale, false, false);
			else
				map.draw(&buffer_painter, page_extent, false, scale, false, false);
			
			buffer_painter.end();
			
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
	if (options.show_grid)
		map.drawGrid(painter, print_area);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, map.getFirstFrontTemplate(), map.getNumTemplates() - 1, view);
	
	// If a temporary buffer has been used, paint it on the device printer
	if (print_buffer_painter.isActive())
	{
		print_buffer_painter.end();
		painter = device_painter;
		
		device_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
		device_painter->drawImage(0, 0, print_buffer);
	}
	device_painter->restore();
}

// slot
void MapPrinter::printMap(QPrinter* printer)
{
	// Printer settings may have been changed by preview or application.
	// We need to use them for printing.
	takePrinterSettings(printer);
	QSizeF extent_size = page_format.page_rect.size() / options.scale_adjustment;
	QPainter p;
	Q_FOREACH(qreal vpos, v_page_pos)
	{
		Q_FOREACH(qreal hpos, h_page_pos)
		{
			QRectF page_extent = QRectF(QPointF(hpos, vpos), extent_size).intersected(print_area);
			if (p.isActive())
				printer->newPage();
			else
				p.begin(printer);
			
			drawPage(&p, (float)options.resolution, page_extent, true); // white background required by QPrintPreviewDialog
		}
	}
	p.end();
}

