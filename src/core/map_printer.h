/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_PRINTER_H
#define OPENORIENTEERING_MAP_PRINTER_H

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QObject>
#include <QPageSize>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QTransform>

#ifdef QT_PRINTSUPPORT_LIB
#  include <QPrinterInfo>
#else
   class QPrinterInfo;
#endif

template <class Key, class T>
class QHash;
class QImage;
class QPainter;
class QPrinter;
class QRectF;
class QSizeF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class MapView;
class Template;


/** The MapPrinterPageFormat is a complete description of page properties. */
class MapPrinterPageFormat
{
public:
	/** Copy of QPrinter::Orientation because QPrinter is not available for Android */
	enum Orientation { Portrait, Landscape };
	
	/** Constructs a new page format.
	 * 
	 *  The format is initialized as a custom page format with a page rectangle
	 *  of the given size, surrounded by the given margin (in mm). */
	MapPrinterPageFormat(QSizeF page_rect_size = QSizeF(100.0, 100.0), qreal margin = 5.0, qreal overlap = 5.0);
	
#ifdef QT_PRINTSUPPORT_LIB
	/** Constructs a new page format, initialized from the given printer's settings. */
	MapPrinterPageFormat(const QPrinter& printer, qreal overlap = 5.0);
#endif
	
	/** Returns a page format initialized from the default printer's settings.
	 *  
	 *  When building without Qt Print Support, returns a default constructed
	 *  page format. */
	static MapPrinterPageFormat fromDefaultPrinter();
	
#ifdef QT_PRINTSUPPORT_LIB
	/** The nominal page size */
	QPageSize::PageSizeId page_size;
#else
	/** The page size as textual key for import/export */
	QString page_size;
#endif
	
	/** The orientation of the paper. */
	Orientation orientation;
	
	/** The printable page rectangle in mm. */
	QRectF page_rect;
	
	/** The total dimensions of the page in mm. */
	QSizeF paper_dimensions;
	
	/** The horizontal overlap in mm of pages when the output covers muliple pages. */
	qreal h_overlap;
	
	/** The vertival overlap in mm of pages when the output covers muliple pages. */
	qreal v_overlap;
};

/** Returns true iff the MapPrinterPageFormat values are equal. */
bool operator==(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs);

/** Returns true iff the MapPrinterPageFormat values are not equal. */
bool operator!=(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs);



/** MapPrinterOptions control how the map is rendered. */
class MapPrinterOptions
{
public:
	/** Basic modes of printing. */
	enum MapPrinterMode
	{
		Vector,        ///< Print in vector graphics mode
		Raster,        ///< Print in raster graphics mode
		Separations    ///< Print spot color separations (b/w vector)
	};
	
	/** Color modes.
	 * 
	 * At the moment, only PDF supports a different mode than the default.
	 */
	enum ColorMode
	{
		DefaultColorMode,  ///< Use the target engine's default color mode.
		DeviceCmyk         ///< Use device-dependent CMYK for vector data.
	};

	/** Constructs new printer options.
	 * 
	 *  The scale, the mode and the resolution are initialized to the given
	 *  values, all boolean properties are initialized to false.
	 */
	MapPrinterOptions(unsigned int scale, int resolution = 600, MapPrinterMode mode = Vector);
	
	/** The scale to be used for printing. */
	unsigned int scale;
	
	/** The nominal resolution to be used. */
	int resolution;
	
	/** The mode of printing.
	 * 
	 *  Note that other printing options may be available only in particular modes.
	 */
	MapPrinterMode mode;
	
	/** The color mode. */
	ColorMode color_mode;
	
	/** Controls if templates get printed.
	 * 
	 *  Not available in MapPrinterOptions::Separations mode.
	 */
	bool show_templates;
	
	/** Controls if the configured grid is printed.
	 * 
	 *  Not available in MapPrinterOptions::Separations mode.
	 */
	bool show_grid;
	
	/** Controls if the desktop printing is to simulate spot color printing.
	 * 
	 *  Only available in MapPrinterOptions::Raster mode.
	 */
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
	void save(QXmlStreamWriter& xml, const QLatin1String& element_name) const;
	
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
	
	/** Platform-dependent data. */
	std::shared_ptr<void> native_data;
};



#ifdef QT_PRINTSUPPORT_LIB

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
	
	/** Returns a QPrinterInfo pointer which signals printing to a KML ground overlay. */
	static const QPrinterInfo* kmzTarget();
	
	/** Returns a reference to a hash which maps paper sizes to names as C strings.
	 *  These strings are not translated.
	 *
	 * @deprecated Use QPageSize::key() instead.
	 */
	static const QHash< int, const char* >& paperSizeNames();
	
	/** Constructs a new MapPrinter for the given map and (optional) view. */
	MapPrinter(Map& map, const MapView* view, QObject* parent = nullptr);
	
	MapPrinter(const MapPrinter&) = delete;
	MapPrinter(MapPrinter&&) = delete;
	MapPrinter& operator=(const MapPrinter&) = delete;
	MapPrinter& operator=(MapPrinter&&) = delete;
	
	/** Destructor. */
	~MapPrinter() override;
	
	/** Returns the configured target printer in terms of QPrinterInfo. */
	const QPrinterInfo* getTarget() const
	{
		return target;
	}
	
	/** Returns true if a real printer is configured. */
	bool isPrinter() const noexcept;
	
	/** Returns true if the target is not representing a virtual printer. */
	static bool isPrinter(const QPrinterInfo* target) noexcept;
	
	/** Returns the page format specification. */
	const MapPrinterPageFormat& getPageFormat() const
	{
		return page_format;
	}
	
	/** Returns the map area to be printed. */
	const QRectF& getPrintArea() const
	{
		return print_area;
	}
	
	/** Returns the rendering options. */
	const MapPrinterOptions& getOptions() const
	{
		return options;
	}
	
	/** Returns true if the current print area and rendering options will 
	 *  result in empty pages. (The grid is not considered.) */
	bool isOutputEmpty() const;

	/** Returns the quotient of map scale denominator and print scale denominator. */
	qreal getScaleAdjustment() const
	{
		return scale_adjustment;
	}
	
	/** Returns the paper size which is required by the current print area and scale. */
	QSizeF getPrintAreaPaperSize() const;
	
	/** Returns the print area size which is possible with the current page rect size. */
	QSizeF getPageRectPrintAreaSize() const;
	
	/** Returns a list of horizontal page positions on the map. */
	const std::vector< qreal >& horizontalPagePositions() const
	{
		return h_page_pos;
	}
	
	/** Returns a list of vertical page positions on the map. */
	const std::vector< qreal >& verticalPagePositions() const
	{
		return v_page_pos;
	}
	
	
	/**
	 * Returns true when the Qt print engine may rasterize non-opaque data.
	 * 
	 * Drawing non-opaque data might cause rasterization with some Qt print
	 * engines. This function is used to detect configurations where such
	 * rasterization may happen. The result can be comined with tests for the
	 * data actually having alpha.
	 */
	bool engineMayRasterize() const;
	
	/**
	 * Returns true when the Qt print engine will have to rasterize the map.
	 * 
	 * Drawing a map with non-opaque colors will cause rasterization with some
	 * Qt print engines. This functions signals when this rasterization must
	 * take place (i.e. even when we try to avoid rasterization for templates
	 * and grid).
	 */
	bool engineWillRasterize() const;
	
	/**
	 * Returns true when the template will be printed non-opaquely.
	 * 
	 * Drawing non-opaque templates might cause rasterization with some Qt
	 * print engines. This function is used to detect this case. It considers
	 * the template's state and hasAlpha() property as well the view's
	 * visibility configuration for the given template.
	 */
	bool hasAlpha(const Template* temp) const;
	
	
	/** Creates a printer configured according to the current settings. */
	std::unique_ptr<QPrinter> makePrinter() const;
	
	/** Takes the settings from the given printer, 
	 *  and generates signals for changing properties. */
	void takePrinterSettings(const QPrinter* printer);
	
	/** Prints the map to the given printer.
	 * 
	 *  This will first update this object's properties from the printer's properties.
	 *
	 *  @return true on success, false on error. */
	bool printMap(QPrinter* printer);
	
	/** Draws a single page to the painter.
	 * 
	 *  In case of an error, the painter will be inactive when returning from
	 *  this function.
	 * 
	 *  When the actual paint device is a QImage, pass it as page_buffer.
	 *  This helps to determine the exact dimensions and to avoid the allocation
	 *  of another buffer.
	 *  Otherwise, drawPage() may allocate a buffer with this map printer's
	 *  resolution and size. Parameter units_per_inch has no influence on this
	 *  buffer but refers to the logical coordinates of device_painter. */
	void drawPage(QPainter* device_painter, const QRectF& page_extent, QImage* page_buffer = nullptr) const;
	
	void drawPage(QPainter* device_painter, const QRectF& page_extent, const QTransform& page_extent_transform, QImage* page_buffer = nullptr) const;
	
	/** Draws the separations as distinct pages to the printer. */
	void drawSeparationPages(QPrinter* printer, QPainter* device_painter, const QRectF& page_extent) const;
	
	/** Returns the current configuration. */
	const MapPrinterConfig& config() const
	{
		return *this;
	}
	
public slots:
	/** Sets the target QPrinterInfo.
	 *  Ownership is not taken over! Target may even be nullptr. */
	void setTarget(const QPrinterInfo* new_target);
	
	/** Sets the map area which is to be printed. */
	void setPrintArea(const QRectF& area);
	
	/** Sets the page size ID to be used. */
	void setPageSize(QPageSize::PageSizeId size);
	
	/** Sets a custom paper size with the given dimensions. */
	void setCustomPageSize(const QSizeF& dimensions);
	
	/** Sets the page orientation. */
	void setPageOrientation(MapPrinterPageFormat::Orientation orientation);
	
	/** Sets the overlapping of the pages at the margins. */
	void setOverlap(qreal h_overlap, qreal v_overlap);
	
	/** Sets the desired printing resolution in dpi.
	 *  The actual resolution will	be set by the printer.
	 * 
	 * Does nothing if dpi is 0 or less.
	 */
	void setResolution(int dpi);
	
	/** Sets the denominator of the map scale for printing. */
	void setScale(unsigned int value);
	
	/** Sets the printing mode. */
	void setMode(MapPrinterOptions::MapPrinterMode mode);
	
	/** Controls whether to print templates. 
	 *  If a MapView is given when enabling template printing, 
	 *  it will determine the visibility of map and templates. */
	void setPrintTemplates(bool visible);
	
	/** Controls whether to print the map grid. */
	void setPrintGrid(bool visible);
	
	/** Controls whether to print in overprinting simulation mode. */
	void setSimulateOverprinting(bool enabled);
	
	/** Controls the color mode. */
	void setColorMode(MapPrinterOptions::ColorMode color_mode);
	
	/** Saves the print parameter (to the map). */
	void saveConfig() const;
	
	/** Cancels a running printMap(), if possible.
	 *  This can only be used during handlers of the printMapProgress() signal. */
	void cancelPrintMap();
	
signals:
	/** Indicates a new target printer. */
	void targetChanged(const QPrinterInfo* target) const;  // clazy:exclude=const-signal-or-slot
	
	/** Indicates a change in the map area. */
	void printAreaChanged(const QRectF& area) const;  // clazy:exclude=const-signal-or-slot
	
	/** Indicates a change in the page format. */
	void pageFormatChanged(const OpenOrienteering::MapPrinterPageFormat& format) const;  // clazy:exclude=const-signal-or-slot
	
	/** Indicates a change in the rendering options. */
	void optionsChanged(const OpenOrienteering::MapPrinterOptions& options) const;  // clazy:exclude=const-signal-or-slot
	
	/** Emitted during printMap() to indicate progress. You can expect this
	 *  signal to be emitted at the start of the print process (value = 1),
	 *  after each page printed, and at the end of the process (value = 100).
	 * 
	 *  @param value Reflects the progress in the range from 1 (just started)
	 *               to 100 (finished).
	 *  @param status A verbal representation of what printMap() is doing. */
	void printProgress(int value, const QString& status) const;  // clazy:exclude=const-signal-or-slot
	
protected:
	/** Returns true if vector mode is set. */
	bool vectorModeSelected() const
	{
		return options.mode == MapPrinterOptions::Vector;
	}
	
	/** Returns true if raster mode is set. */
	bool rasterModeSelected() const
	{
		return options.mode == MapPrinterOptions::Raster;
	}
	
	/** Returns true if separations mode is set. */
	bool separationsModeSelected() const
	{
		return options.mode == MapPrinterOptions::Separations;
	}
	
	/** Updates the paper dimensions from paper format and orientation. */
	void updatePaperDimensions();
	
	/** Updates the page breaks from map area and page format. */
	void updatePageBreaks();
	
	/** Updates the scale adjustment and page breaks. */
	void mapScaleChanged();
	
	Map& map;
	const MapView* view;
	const QPrinterInfo* target = nullptr;
	QPrinterInfo target_copy;
	qreal scale_adjustment;
	std::vector<qreal> h_page_pos;
	std::vector<qreal> v_page_pos;
	bool cancel_print_map = false;
};

#endif



//### MapPrinterPageFormat inline code ###

inline
bool operator!=(const MapPrinterPageFormat& lhs, const MapPrinterPageFormat& rhs)
{
	return !(lhs == rhs);
}



//### Free functions for MapPrinterOptions ###

/** Returns true iff the MapPrinterOptions values are equal. */
bool operator==(const MapPrinterOptions& lhs, const MapPrinterOptions& rhs);

/** Returns true iff the MapPrinterOptions values are not equal. */
inline
bool operator!=(const MapPrinterOptions& lhs, const MapPrinterOptions& rhs)
{
	return !(lhs == rhs);
}



// ### Free functions for MapPrinterConfig ###

/** Returns true iff the MapPrinterConfig values are equal. */
bool operator==(const MapPrinterConfig& lhs, const MapPrinterConfig& rhs);

/** Returns true iff the MapPrinterConfig values are not equal. */
inline
bool operator!=(const MapPrinterConfig& lhs, const MapPrinterConfig& rhs)
{
	return !(lhs == rhs);
}



}  // namespace OpenOrienteering

#endif
