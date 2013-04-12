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


#ifndef _OPENORIENTEERING_MAP_PRINTER_H_
#define _OPENORIENTEERING_MAP_PRINTER_H_

#include <vector>

#include <QHash>
#include <QObject>
#include <QPrinterInfo>

QT_BEGIN_NAMESPACE
class QImage;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
class MapView;

/** The MapPrinterPageFormat is a complete description of page properties. */
class MapPrinterPageFormat
{
public:
	/** Constructs a new page format.
	 * 
	 *  It is initialized a custom page format with a page rectangle of the
	 *  given size, surrounded by the given margin (in mm).
	 *  The page overlap values are initialized to 5 (mm). */
	MapPrinterPageFormat(QSizeF page_rect_size = QSizeF(100.0, 100.0), qreal margin = 5.0);
	
	/** The nominal paper size. */
	QPrinter::PaperSize paper_size;
	
	/** The orientation of the paper. */
	QPrinter::Orientation orientation;
	
	/** The total dimensions of the page in mm. */
	QSizeF paper_dimensions;
	
	/** The printable page rectangle in mm. */
	QRectF page_rect;
	
	/** The horizontal overlap in mm of pages when the output covers muliple pages. */
	qreal h_overlap;
	
	/** The vertival overlap in mm of pages when the output covers muliple pages. */
	qreal v_overlap;
};



/** MapPrinterOptions control how the map is rendered. */
class MapPrinterOptions
{
public:
	/** Constructs new printer options.
	 * 
	 *  The scale and the resolution are initialized to the given values,
	 *  all boolean properties are initialized to false. */
	MapPrinterOptions(unsigned int scale, unsigned int resolution = 600);
	
	/** The scale to be used for printing. */
	unsigned int scale;
	
	/** The nominal resolution to be used. */
	unsigned int resolution;
	
	/** Controls if spot color separations are to be printed. */
	bool print_spot_color_separations;
	
	/** Controls if templates get printed. */
	bool show_templates;
	
	/** Controls if the configured grid is printed. */
	bool show_grid;
	
	/** Controls if the desktop printing is to simulate spot color printing. */
	bool simulate_overprinting;
};



/** MapPrintParameters contains all options that control printing 
 *  and provides methods for saving and loading. */
class MapPrinterConfig
{
public:
	/** Constructs a default config for the given map. */
	MapPrinterConfig(const Map& map);
	
	/** Constructs a config and loads the map print parameters from an XML stream. */
	MapPrinterConfig(const Map& map, QXmlStreamReader& xml);
	
	/** Saves the map print parameters to an XML stream. */
	void save(QXmlStreamWriter& xml, const QString& element_name) const;
	
	/** The name of the printer. */
	QString printer_name;
	
	/** The print area. */
	QRectF print_area;
	
	/** The page format. */
	MapPrinterPageFormat page_format;
	
	/** Printing options. */
	MapPrinterOptions options;
	
	/** A flag which indicates to the UI whether to maintain a centered 
	 *  print area when the size of the map or the page changes. */
	bool center_print_area;
	
	/** A flag which indicates to the UI whether to adjust the print area size
	 *  to the current page size. */
	bool single_page_print_area;
};



/** MapPrinter provides an interface to print a map (to a printer or file).
 * It may render a page on any QPaintDevice, such as an QImage. */
class MapPrinter : public QObject, protected MapPrinterConfig
{
Q_OBJECT
public:
	/** Returns a QPrinterInfo pointer which signals printing to a PDF file. */
	static const QPrinterInfo* pdfTarget();
	
	/** Returns a QPrinterInfo pointer which signals printing to a raster file. */
	static const QPrinterInfo* imageTarget();
	
	/** Returns a reference to a hash which maps paper sizes to names as C strings.
	 *  These strings are not translated. */
	static const QHash< int, const char* >& paperSizeNames();
	
	/** Constructs a new MapPrinter for the given map. */
	MapPrinter(Map& map, QObject* parent = NULL);
	
	/** Destructor. */
	virtual ~MapPrinter();
	
	/** Returns the configured target printer in terms of QPrinterInfo. */
	const QPrinterInfo* getTarget() const;
	
	/** Returns true if a real printer is configured. */
	bool isPrinter() const;
	
	/** Returns the page format specification. */
	const MapPrinterPageFormat& getPageFormat() const;
	
	/** Returns the map area to be printed. */
	const QRectF& getPrintArea() const;
	
	/** Returns the rendering options. */
	const MapPrinterOptions& getOptions() const;
	
	/** Returns true if the current print area and rendering options will 
	 *  result in empty pages. (The grid is not considered.) */
	bool isOutputEmpty() const;

	/** Returns the quotient of map scale denominator and print scale denominator. */
	qreal getScaleAdjustment() const;
	
	/** Returns the paper size which is required by the current print area and scale. */
	QSizeF getPrintAreaPaperSize() const;
	
	/** Returns the print area size which is possible with the current page rect size. */
	QSizeF getPageRectPrintAreaSize() const;
	
	/** Returns a list of horizontal page positions on the map. */
	const std::vector< qreal >& horizontalPagePositions() const;
	
	/** Returns a list of vertical page positions on the map. */
	const std::vector< qreal >& verticalPagePositions() const;
	
	/** Creates a printer configured according to the current settings. */
	QPrinter* makePrinter() const;
	
	/** Takes the settings from the given printer, 
	 *  and generates signals for changing properties. */
	void takePrinterSettings(const QPrinter* printer);
	
	/** Draws a single page to the painter.
	 *  When the actual paint device is a QImage, pass it as page_buffer.
	 *  This helps to determine the exact dimensions and to avoid the allocation
	 *  of another buffer. */
	void drawPage(QPainter* device_painter, float dpi, const QRectF& page_extent, bool white_background, QImage* page_buffer = NULL) const;
	
	/** Draws the separations as distinct pages to the printer. */
	void drawSeparationPages(QPrinter* printer, QPainter* device_painter, float dpi, const QRectF& page_extent) const;
	
	/** Returns the current configuration. */
	const MapPrinterConfig& config() const;
	
public slots:
	/** Sets the target QPrinterInfo.
	 *  Ownership is not taken over! Target may even be NULL. */
	void setTarget(const QPrinterInfo* new_target);
	
	/** Sets the map area which is to be printed. */
	void setPrintArea(const QRectF& area);
	
	/** Sets the paper size to be used. */
	void setPaperSize(const QPrinter::PaperSize size);
	
	/** Sets a custom paper size with the given dimensions. */
	void setCustomPaperSize(const QSizeF dimensions);
	
	/** Sets the page orientation. */
	void setPageOrientation(const QPrinter::Orientation orientation);
	
	/** Sets the overlapping of the pages at the margins. */
	void setOverlap(qreal h_overlap, qreal v_overlap);
	
	/** Sets the desired printing resolution in dpi. 
	 *  The actual resolution will	be set by the printer. */
	void setResolution(const unsigned int dpi);
	
	/** Sets the denominator of the map scale for printing. */
	void setScale(const unsigned int value);
	
	/** Enables or disables the printing of spot color separations.
	 *  Note that templates, grid, and overprinting simulation will be ignored
	 *  in spot color mode. */
	void setPrintSpotColorSeparations(bool enabled);
	
	/** Controls whether to print templates. 
	 *  If a MapView is given when enabling template printing, 
	 *  it will determine the visibility of map and templates. */
	void setPrintTemplates(const bool visible, MapView* view = NULL);
	
	/** Controls whether to print the map grid. */
	void setPrintGrid(const bool visible);
	
	/** Controls whether to print in overprinting simulation mode. */
	void setSimulateOverprinting(bool enabled);
	
	/** Saves the print parameter (to the map). */
	void saveConfig() const;
	
	/** Prints the map to the given printer. 
	 *  This will first update this object's properties from the printer's properties. */
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
	
	/** Emitted during printMap() to indicate progress. You can expect this
	 *  signal to be emitted at the start of the print process (value = 1),
	 *  after each page printed, and at the end of the process (value = 100).
	 * 
	 *  @param value Reflects the progress in the range from 1 (just started)
	 *               to 100 (finished). */
	void printMapProgress(int value) const;
	
protected:
	/** Updates the paper dimensions from paper format and orientation. */
	void updatePaperDimensions();
	
	/** Updates the page breaks from map area and page format. */
	void updatePageBreaks();
	
	Map& map;
	MapView* view;
	const QPrinterInfo* target;
	QPrinterInfo target_copy;
	qreal scale_adjustment;
	std::vector<qreal> h_page_pos;
	std::vector<qreal> v_page_pos;
};



//### MapPrinterPageFormat inline code ###

/** Returns true iff the MapPrinterPageFormat values are not equal. */
inline
bool operator!=(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return
	  lhs.paper_size       != rhs.paper_size       ||
	  lhs.orientation      != rhs.orientation      ||
	  lhs.paper_dimensions != rhs.paper_dimensions ||
	  lhs.page_rect        != rhs.page_rect        ||
	  lhs.h_overlap        != rhs.h_overlap        ||
	  lhs.v_overlap        != rhs.v_overlap;
}

/** Returns true iff the MapPrinterPageFormat values are equal. */
inline
bool operator==(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return !(lhs != rhs);
}



//### MapPrinterOptions inline code ###

/** Returns true iff the MapPrinterOptions values are not equal. */
inline
bool operator!=(const MapPrinterOptions& lhs, const MapPrinterOptions& rhs)
{
	return
	  lhs.resolution            != rhs.resolution            ||
	  lhs.scale                 != rhs.scale                 ||
	  lhs.show_templates        != rhs.show_templates        ||
	  lhs.show_grid             != rhs.show_grid             ||
	  lhs.simulate_overprinting != rhs.simulate_overprinting;
}

/** Returns true iff the MapPrinterOptions values are equal. */
inline
bool operator==(const MapPrinterOptions& lhs, const MapPrinterOptions& rhs)
{
	return !(lhs != rhs);
}



// ### MapPrinterConfig ###

/** Returns true iff the MapPrinterConfig values are not equal. */
inline
bool operator!=(const MapPrinterConfig& lhs, const MapPrinterConfig& rhs)
{
	return
	  lhs.printer_name           != rhs.printer_name            ||
	  lhs.page_format            != rhs.page_format             ||
	  lhs.options                != rhs.options                 ||
	  lhs.center_print_area      != rhs.center_print_area       ||
	  lhs.single_page_print_area != rhs.single_page_print_area;
}

/** Returns true iff the MapPrinterConfig values are equal. */
inline
bool operator==(const MapPrinterConfig& lhs, const MapPrinterConfig& rhs)
{
	return !(lhs != rhs);
}



// ### MapPrinter inline code ###

inline
const MapPrinterConfig& MapPrinter::config() const
{
	return *this;
}

inline
const QPrinterInfo* MapPrinter::getTarget() const
{
	return target;
}

inline
const MapPrinterPageFormat& MapPrinter::getPageFormat() const
{
	return page_format;
}

inline
const QRectF& MapPrinter::getPrintArea() const
{
	return print_area;
}

inline
const MapPrinterOptions& MapPrinter::getOptions() const
{
	return options;
}

inline
qreal MapPrinter::getScaleAdjustment() const
{
	return scale_adjustment;
}

inline
QSizeF MapPrinter::getPrintAreaPaperSize() const
{
	return getPrintArea().size() * scale_adjustment;
}

inline
QSizeF MapPrinter::getPageRectPrintAreaSize() const
{
	return page_format.page_rect.size() / scale_adjustment;
}

inline
const std::vector< qreal >& MapPrinter::horizontalPagePositions() const
{
	return h_page_pos;
}

inline
const std::vector< qreal >& MapPrinter::verticalPagePositions() const
{
	return v_page_pos;
}

#endif
