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
		if (page_format.paper_size == QPrinter::Custom);
		{
			// TODO: Save/load custom paper dimensions.
			page_format.paper_dimensions = print_area.size() * options.scale_adjustment;
			page_format.page_rect = QRectF(QPointF(0.0, 0.0), page_format.paper_dimensions);
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
		emit targetChanged(target);
		
		if (old_target == imageTarget() || new_target == imageTarget())
		{
			// Page margins change
			updatePaperDimensions();
			emit pageFormatChanged(page_format);
		}
	}
}

QPrinter* MapPrinter::makePrinter() const
{
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	
	printer->setDocName("MAP"/* FIXME QFileInfo(main_window->getCurrentFilePath()).fileName()*/);
	printer->setFullPage(true);
	if (page_format.paper_size == QPrinter::Custom)
	{	
		// TODO: Proper handling of custom paper dimensions.
		printer->setPaperSize(print_area.size() * options.scale_adjustment, QPrinter::Millimeter);
		printer->setOrientation(QPrinter::Portrait);
	}
	else
	{
		printer->setPaperSize(page_format.paper_size);
		printer->setOrientation(page_format.orientation);
	}
	printer->setColorMode(QPrinter::Color);
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
		updatePageBreaks();
		emit printAreaChanged(print_area);
	}
}

// slot
void MapPrinter::setPaperSize(QPrinter::PaperSize size)
{
	if (page_format.paper_size == QPrinter::Custom)
		setCustomPaperSize(print_area.size() * options.scale_adjustment);
	else if (page_format.paper_size != size)
	{
		page_format.paper_size = size;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setCustomPaperSize(QSizeF dimensions)
{
	if ((page_format.paper_size != QPrinter::Custom || 
	     page_format.paper_dimensions != dimensions) &&
	     ! dimensions.isEmpty())
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
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	  
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		page_format.orientation = printer->orientation(); 
	}
	else
	{
		printer->setPaperSize(page_format.paper_size);
		printer->setOrientation(page_format.orientation);
	}
	
	page_format.page_rect = printer->paperRect(QPrinter::Millimeter);
	page_format.paper_dimensions = page_format.page_rect.size();
	
	if (target != imageTarget() && page_format.paper_size != QPrinter::Custom)
	{
		qreal left, top, right, bottom;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
		page_format.page_rect.adjust(left, top, -right, -bottom);
	}
	
	delete printer;
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
	qreal hpos = print_area.left();
	h_page_pos.push_back(hpos);
	qreal page_width = page_format.page_rect.width() / options.scale_adjustment;
	if (page_width >= 0.01)
	{
		for (hpos += page_width; hpos < print_area.right(); hpos += page_width)
			h_page_pos.push_back(hpos);
		// Don' pre-calculate offset to avoid FP precision problems
		qreal hoffset = 0.5 * (hpos - print_area.right());
		for (std::vector<qreal>::iterator it=h_page_pos.begin(); it != h_page_pos.end(); ++it)
			*it -= hoffset;
	}
	
	v_page_pos.clear();
	qreal vpos = print_area.top();
	v_page_pos.push_back(vpos);
	qreal page_height = page_format.page_rect.height() / options.scale_adjustment;
	if (page_height >= 0.01)
	{
		for (vpos += page_height; vpos < print_area.bottom(); vpos += page_height)
			v_page_pos.push_back(vpos);
		// Don' pre-calculate offset to avoid FP precision problems
		qreal voffset = 0.5 * (vpos - print_area.bottom());
		for (std::vector<qreal>::iterator it=v_page_pos.begin(); it != v_page_pos.end(); ++it)
			*it -= voffset;
	}
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
}

void MapPrinter::drawPage(QPainter* device_painter, float dpi, const QRectF& page_extent, bool white_background) const
{
	device_painter->save();
	
	device_painter->setRenderHint(QPainter::Antialiasing);
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform);
	
	// Painting transparently is not supported for print devices.
	// If there are any transparent features, use a temporary image 
	// which is drawn on the device printer as last step.
	// If the target is an image, use the temporary image to enforce
	// the given resolution during preview.
	bool have_transparency = options.simulate_overprinting || target == imageTarget();
	if (view != NULL)
	{
		have_transparency = view->getMapVisibility()->visible && view->getMapVisibility()->opacity < 1.0f;
		for (int i = 0; i < map.getNumTemplates() && !have_transparency; ++i)
		{
			TemplateVisibility* visibility = view->getTemplateVisibility(map.getTemplate(i));
			have_transparency = visibility->visible && visibility->opacity < 1.0f;
		}
	}
	
	// Dots per mm
	qreal scale = dpi / 25.4;
	
	QPainter* painter = device_painter;
	QImage print_buffer;
	if (have_transparency)
	{
		int w = qCeil(device_painter->device()->widthMM() * scale);
		int h = qCeil(device_painter->device()->heightMM() * scale);
		print_buffer = QImage(w, h, QImage::Format_RGB32);
		print_buffer.fill(QColor(Qt::white));
		painter = new QPainter(&print_buffer);
		painter->setRenderHints(device_painter->renderHints());
	}
	else if (white_background)
		painter->fillRect(QRect(0, 0, painter->device()->width(), painter->device()->height()), Qt::white);
	
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
	painter->setClipRect(page_extent.intersected(print_area), Qt::ReplaceClip);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, 0, map.getFirstFrontTemplate() - 1, view);
	
	if (view == NULL || view->getMapVisibility()->visible)
	{
		QImage map_buffer;
		QPainter* map_painter = painter;
		if (have_transparency)
		{
			// Draw map into a temporary buffer first which is printed with the map's opacity later.
			// This prevents artifacts with overlapping objects.
			map_buffer = QImage(print_buffer.size(), QImage::Format_ARGB32_Premultiplied);
			map_buffer.fill(QColor(Qt::transparent));
			
			map_painter = new QPainter(&map_buffer);
			map_painter->setRenderHints(painter->renderHints());
			map_painter->setTransform(painter->transform());
		}
		
		if (options.simulate_overprinting)
			map.drawOverprintingSimulation(map_painter, page_extent, false, scale, false, false);
		else
			map.draw(map_painter, page_extent, false, scale, false, false);
			
		if (map_painter != painter)
		{
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
	
	if (options.show_grid)
		map.drawGrid(painter, print_area);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, map.getFirstFrontTemplate(), map.getNumTemplates() - 1, view);
	
	// If a temporary buffer has been used, paint it on the device painter
	if (painter != device_painter)
	{
		delete painter; 
		painter = NULL;
		
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
	QPainter p(printer);
	bool need_new_page = false;
	Q_FOREACH(qreal vpos, v_page_pos)
	{
		Q_FOREACH(qreal hpos, h_page_pos)
		{
			if (p.isActive())
			{
				if (need_new_page)
					printer->newPage();
				
				QRectF page_extent = QRectF(QPointF(hpos, vpos), extent_size);
				drawPage(&p, (float)options.resolution, page_extent, false);
				need_new_page = true;
			}
		}
	}
}

