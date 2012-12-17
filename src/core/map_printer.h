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


#ifndef _OPENORIENTEERING_MAP_PRINTER_H_
#define _OPENORIENTEERING_MAP_PRINTER_H_

#include <vector>

#include <QObject>
#include <QPrinterInfo>

class Map;
class MapView;

/** The MapPrinterPageFormat is a complete description of page properties. */
class MapPrinterPageFormat
{
public:
	QPrinter::PaperSize paper_size;
	QPrinter::Orientation orientation;
	QSizeF paper_dimensions;
	QRectF page_rect;
};



/** MapPrinterOptions control how the map is rendered. */
class MapPrinterOptions
{
public:
	int resolution;
	int scale;
	qreal scale_adjustment;
	bool show_templates;
	bool show_grid;
	bool simulate_overprinting;
};



/** MapPrinter provides an interface to print a map (to a printer or file).
 * It may render a page on any QPaintDevice, such as an QImage. */
class MapPrinter : public QObject
{
Q_OBJECT
public:
	/** Returns a QPrinterInfo pointer which signals printing to a PDF file. */
	static const QPrinterInfo* pdfTarget();
	
	/** Returns a QPrinterInfo pointer which signals printing to a raster file. */
	static const QPrinterInfo* imageTarget();
	
	/** Constructs a new MapPrinter for the given map. */
	MapPrinter(Map& map, QObject* parent = NULL);
	
	/** Destructor. */
	virtual ~MapPrinter();
	
	/** Returns the configured target printer in terms of QPrinterInfo. */
	const QPrinterInfo* getTarget() const { return target; }
	
	/** Returns true if a real printer is configured. */
	bool isPrinter() const;
	
	/** Returns the page format specification. */
	const MapPrinterPageFormat& getPageFormat() const { return page_format; }
	
	/** Returns the map area to be printed. */
	const QRectF& getPrintArea() const { return print_area; }
	
	/** Returns the rendering options. */
	const MapPrinterOptions& getOptions() const { return options; }
	
	/** Returns true if the current print area and rendering options will 
	 *  result in empty pages. (The grid is not considered.) */
	bool isOutputEmpty() const;

	/** Returns the quotient of map scale denominator and print scale denominator. */
	qreal getScaleAdjustment() const { return options.scale_adjustment; }
	
	/** Returns a list of horizontal page positions on the map. */
	const std::vector<qreal>& horizontalPagePositions() const { return h_page_pos; }
	
	/** Returns a list of vertical page positions on the map. */
	const std::vector<qreal>& verticalPagePositions() const { return v_page_pos; }
	
	/** Creates a printer configured according to the current settings. */
	QPrinter* makePrinter() const;
	
	/** Takes the settings from the given printer, 
	 *  and generates signals for changing properties. */
	void takePrinterSettings(const QPrinter* printer);
	
	/** Draws a single page to the painter.	 */
	void drawPage(QPainter* device_painter, float dpi, const QRectF& page_extent, bool white_background) const;
	
public slots:
	/** Sets the target QPrinterInfo.
	 *  Ownership is not taken over! Target may even be NULL. */
	void setTarget(const QPrinterInfo* new_target);
	
	/** Sets the map area which is to be printed. */
	void setPrintArea(const QRectF& area);
	
	/** Sets the paper size to be used. */
	void setPaperSize(QPrinter::PaperSize size);
	
	/** Sets a custom paper size with the given dimensions. */
	void setCustomPaperSize(QSizeF dimensions);
	
	/** Sets the page orientation. */
	void setPageOrientation(QPrinter::Orientation orientation);
	
	/** Sets the desired printing resolution in dpi. 
	 *  The actual resolution will	be set by the printer. */
	void setResolution(float dpi);
	
	/** Sets the denominator of the map scale for printing. */
	void setScale(int value);
	
	/** Controls whether to print templates. 
	 *  If a MapView is given when enabling template printing, 
	 *  it will determine the visibility of map and templates. */
	void setPrintTemplates(bool visible, MapView* view = NULL);
	
	/** Controls whether to print the map grid. */
	void setPrintGrid(bool visible);
	
	/** Controls whether to print in overprinting simulation mode. */
	void setSimulateOverprinting(bool enabled);
	
	/** Saves the print parameter (to the map). */
	void saveParameters() const;
	
	/** Prints the map to the given printer. 
	 *  Multiple pages may be generated. */
	void printMap(QPrinter* printer);
	
signals:
	/** Indicates a new target printer. */
	void targetChanged(const QPrinterInfo* target) const;
	
	/** Indicates a change in the map area. */
	void printAreaChanged(const QRectF& area) const;
	
	/** Indicates a change in the page format. */
	void pageFormatChanged(const MapPrinterPageFormat& format) const;
	
	/** Indicates a change in the rendering options. */
	void optionsChanged(const MapPrinterOptions& options) const;
	
protected:
	/** Updates the paper dimensions from paper format and orientation. */
	void updatePaperDimensions();
	
	/** Updates the page breaks from map area and page format. */
	void updatePageBreaks();
	
	Map& map;
	MapView* view;
	const QPrinterInfo* target;
	QPrinterInfo target_copy;
	MapPrinterPageFormat page_format;
	QRectF print_area;
	bool center;
	MapPrinterOptions options;
	std::vector<qreal> h_page_pos;
	std::vector<qreal> v_page_pos;
};

#endif
