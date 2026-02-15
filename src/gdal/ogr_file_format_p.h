/*
 *    Copyright 2016-2020 Kai Pastor
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

#ifndef OPENORIENTEERING_OGR_FILE_FORMAT_P_H
#define OPENORIENTEERING_OGR_FILE_FORMAT_P_H

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include <QByteArray>
#include <QCoreApplication>
#include <QFlags>
#include <QHash>
#include <QString>
#include <QtGlobal>

// The GDAL/OGR C API is more stable than the C++ API.
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>
// IWYU pragma: no_include <ogr_core.h>

#include "core/map_coord.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_import_export.h"

class QPointF;

namespace OpenOrienteering {

class AreaSymbol;
class Georeferencing;
class LatLon;
class LineSymbol;
class Map;
class MapView;
class MapColor;
class MapPart;
class Object;
class KeyValueContainer;
class PathObject;
class PointSymbol;
class TextSymbol;


namespace ogr
{

	class OGRCoordinateTransformationHDeleter
	{
	public:
		void operator()(OGRCoordinateTransformationH ct) const
		{
			OCTDestroyCoordinateTransformation(ct);
		}
	};
	
	/**
	 * A convenience class for OGR C API coordinate transformation handles,
	 * similar to std::unique_ptr.
	 */
	using unique_transformation = std::unique_ptr<typename std::remove_pointer<OGRCoordinateTransformationH>::type, OGRCoordinateTransformationHDeleter>;
	

	class OGRDataSourceHDeleter
	{
	public:
		void operator()(OGRDataSourceH data_source) const
		{
			OGRReleaseDataSource(data_source);
		}
	};

	/** A convenience class for OGR C API datasource handles, similar to std::unique_ptr. */
	using unique_datasource = std::unique_ptr<typename std::remove_pointer<OGRDataSourceH>::type, OGRDataSourceHDeleter>;


	class OGRFeatureHDeleter
	{
	public:
		void operator()(OGRFeatureH feature) const
		{
			OGR_F_Destroy(feature);
		}
	};

	/** A convenience class for OGR C API feature handles, similar to std::unique_ptr. */
	using unique_feature = std::unique_ptr<typename std::remove_pointer<OGRFeatureH>::type, OGRFeatureHDeleter>;


	class OGRFieldDefnHDeleter
	{
	public:
		void operator()(OGRFieldDefnH field_defn) const
		{
			OGR_Fld_Destroy(field_defn);
		}
	};

	/** A convenience class for OGR C API field definition handles, similar to std::unique_ptr. */
	using unique_fielddefn = std::unique_ptr<typename std::remove_pointer<OGRFieldDefnH>::type, OGRFieldDefnHDeleter>;
	

	class OGRGeometryHDeleter
	{
	public:
		void operator()(OGRGeometryH geometry) const
		{
			OGR_G_DestroyGeometry(geometry);
		}
	};

	/** A convenience class for OGR C API geometry handles, similar to std::unique_ptr. */
	using unique_geometry = std::unique_ptr<typename std::remove_pointer<OGRGeometryH>::type, OGRGeometryHDeleter>;


	class OGRSpatialReferenceHDeleter
	{
	public:
		void operator()(OGRSpatialReferenceH srs) const
		{
			OSRDestroySpatialReference(srs);
		}
	};
	
	/**
	 * A convenience class for OGR C API SRS handles, similar to std::unique_ptr.
	 */
	using unique_srs = std::unique_ptr<typename std::remove_pointer<OGRSpatialReferenceH>::type, OGRSpatialReferenceHDeleter>;


	class OGRStyleMgrHDeleter
	{
	public:
		void operator()(OGRStyleMgrH manager) const
		{
			OGR_SM_Destroy(manager);
		}
	};
	
	/** A convenience class for OGR C API style manager handles, similar to std::unique_ptr. */
	using unique_stylemanager = std::unique_ptr<typename std::remove_pointer<OGRStyleMgrH>::type, OGRStyleMgrHDeleter>;


	class OGRStyleTableHDeleter
	{
	public:
		void operator()(OGRStyleTableH table) const
		{
			OGR_STBL_Destroy(table);
		}
	};

	/** A convenience class for OGR C API style manager handles, similar to std::unique_ptr. */
	using unique_styletable = std::unique_ptr<typename std::remove_pointer<OGRStyleTableH>::type, OGRStyleTableHDeleter>;
}



/**
 * An Importer for geospatial vector data supported by OGR.
 * 
 * The option "separate_layers" will cause OGR layers to be imported as distinct
 * map parts if set to true.
 */
class OgrFileImport : public Importer
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OgrFileImport)
	
public:
	using ObjectList = std::vector<Object*>;
	
	/**
	 * An interface for clipping objects during import.
	 */
	class Clipping
	{
	public:
		virtual ~Clipping();
		
		/**
		 * Applies clipping to the input objects.
		 * 
		 * This function takes ownership of the input objects, and
		 * it passes ownership of the output objects to the caller.
		 */
		virtual ObjectList process(const ObjectList& objects) const = 0;
	};
	
	
	static bool canRead(const QString& path);
	
	/**
	 * The unit type indicates the coordinate system the data units refers to.
	 */
	enum UnitType
	{
		UnitOnGround,  ///< Data refers to real dimensions. Includes geograghic CS.
		UnitOnPaper    ///< Data refers to dimensions in the (printed) map.
	};
	
	/**
	 * A Pointer to a function which creates a MapCoordF from double coordinates.
	 */
	using MapCoordConstructor = MapCoord (OgrFileImport::*)(double, double) const;
	
	/**
	 * Constructs a new importer.
	 */
	OgrFileImport(const QString& path, Map *map, MapView *view, UnitType unit_type = UnitOnGround);
	
	~OgrFileImport() override;
	
	
	bool supportsQIODevice() const noexcept override;
	
	
	/**
	 * Enables the import of georeferencing from the geospatial data.
	 * 
	 * If this import is not enabled, the georeferencing of the Map given to
	 * the constructor will be used instead.
	 */
	void setGeoreferencingImportEnabled(bool enabled);
	
	
	/**
	 * Tests if the file's spatial references can be used with the given georeferencing.
	 * 
	 * This returns true only if all layers' spatial references can be
	 * transformed to the spatial reference systems represented by georef.
	 * It will always return false for a local or invalid Georeferencing.
	 */
	static bool checkGeoreferencing(const QString& path, const Georeferencing& georef);
	
	/**
	 * Calculates the average geographic coordinates (WGS84) of the file.
	 */
	static LatLon calcAverageLatLon(const QString& path);
	
	
	QByteArray driverName() const { return driver_name; }
	
	
protected:
	ogr::unique_srs srsFromMap();
	
	/**
	 * Tests if the file's spatial references can be used with the given georeferencing.
	 * 
	 * This returns true only if all layers' spatial references can be
	 * transformed to the spatial reference systems represented by georef.
	 * It will always return false for a local or invalid Georeferencing.
	 */
	static bool checkGeoreferencing(OGRDataSourceH data_source, const Georeferencing& georef);
	
	void prepare() override;
	
	bool importImplementation() override;
	
	ogr::unique_srs importGeoreferencing(OGRDataSourceH data_source);
	
	void importStyles(OGRDataSourceH data_source);
	
	void importLayer(MapPart* map_part, OGRLayerH layer);
	
	void importFeature(MapPart* map_part, OGRFeatureDefnH feature_definition, OGRFeatureH feature, OGRGeometryH geometry, const Clipping* clipping);
	
	
	KeyValueContainer importFields(OGRFeatureDefnH feature_definition, OGRFeatureH feature);
		
	ObjectList importGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	ObjectList importGeometryCollection(OGRFeatureH feature, OGRGeometryH geometry);
	
	Object* importPointGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	PathObject* importLineStringGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	PathObject* importPolygonGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	std::unique_ptr<Clipping> getLayerClipping(OGRLayerH layer);
	
	
	bool setSRS(OGRSpatialReferenceH srs);
	
	
	Symbol* getSymbol(Symbol::Type type, const char* raw_style_string);
	
	MapColor* makeColor(OGRStyleToolH tool, const char* color_string);
	
	void applyPenColor(OGRStyleToolH tool, LineSymbol* line_symbol);
	
	void applyBrushColor(OGRStyleToolH tool, AreaSymbol* area_symbol);
	
	
	MapCoord toMapCoord(double x, double y) const;
	
	/**
	 * A MapCoordConstructor which interprets the given coordinates in millimeters on paper.
	 */
	MapCoord fromDrawing(double x, double y) const;
	
	/**
	 * A MapCoordConstructor which interprets the given coordinates as projected.
	 */
	MapCoord fromProjected(double x, double y) const;
	
	
	static LatLon calcAverageLatLon(OGRDataSourceH data_source);
	
	static QPointF calcAverageCoords(OGRDataSourceH data_source, OGRSpatialReferenceH srs);
	
	
	void handleKmlOverlayIcon(OgrFileImport::ObjectList& objects, const KeyValueContainer& tags) const;
	
	
private:
	Symbol* getSymbolForPointGeometry(const QByteArray& style_string);
	LineSymbol* getLineSymbol(const QByteArray& style_string);
	AreaSymbol* getAreaSymbol(const QByteArray& style_string);
	
	PointSymbol* getSymbolForOgrSymbol(OGRStyleToolH tool, const QByteArray& style_string);
	TextSymbol* getSymbolForLabel(OGRStyleToolH tool, const QByteArray& style_string);
	LineSymbol* getSymbolForPen(OGRStyleToolH tool, const QByteArray& style_string);
	AreaSymbol* getSymbolForBrush(OGRStyleToolH tool, const QByteArray& style_string);
	
	QByteArray driver_name;
	
	QHash<QByteArray, Symbol*> point_symbols;
	PointSymbol* default_point_symbol;
	QHash<QByteArray, Symbol*> text_symbols;
	TextSymbol* default_text_symbol;
	QHash<QByteArray, Symbol*> line_symbols;
	LineSymbol* default_line_symbol;
	QHash<QByteArray, Symbol*> area_symbols;
	AreaSymbol* default_area_symbol;
	QHash<QByteArray, MapColor*> colors;
	MapColor* default_pen_color;
	
	MapCoordConstructor to_map_coord;
	
	ogr::unique_srs map_srs;
	
	OGRSpatialReferenceH data_srs = {};
	
	ogr::unique_transformation data_transform;
	
	ogr::unique_stylemanager manager;
	
	int empty_geometries = 0;
	int no_transformation = 0;
	int failed_transformation = 0;
	int unsupported_geometry_type = 0;
	int too_few_coordinates = 0;
	
	UnitType unit_type;
	
	bool georeferencing_import_enabled = true;
	bool clip_layers;
};



// ### inline code ###

inline
MapCoord OgrFileImport::toMapCoord(double x, double y) const
{
	return (this->*to_map_coord)(x, y);
}

/**
 * An Exporter to geospatial vector data supported by OGR.
 *
 * OGR needs to know the filename. As well, it doesn't use
 * a QIODevice, but rather uses the filename directly.
 */
class OgrFileExport : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OgrFileExport)

public:

	/**
	 * Quirks for OGR drivers
	 *
	 * Each quirk shall use a distinct bit so that they may be OR-combined.
	 */
	enum OgrQuirk
	{
		GeorefOptional = 0x01,   ///< The driver does not need or use georeferencing.
		NeedsWgs84     = 0x02,   ///< The driver needs WGS84 geographic coordinates.
		SingleLayer    = 0x04,   ///< The driver supports just a single layer.
		UseLayerField  = 0x08,   ///< Write the symbol names to the layer field.
	};

	/**
	 * A type which handles OR-combinations of OGR driver quirks.
	 */
	Q_DECLARE_FLAGS(OgrQuirks, OgrQuirk)

	OgrFileExport(const QString& path, const Map *map, const MapView *view, const char* id);
	~OgrFileExport() override;

	bool supportsQIODevice() const noexcept override;

	bool exportImplementation() override;

protected:
	std::vector<const Symbol*> symbolsForExport() const;
	
	void addPointsToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition);
	void addTextToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition);
	void addLinesToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition);
	void addAreasToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition);

	OGRLayerH createLayer(const char* layer_name, OGRwkbGeometryType type);

	static QByteArray symbolId(const Symbol* symbol) { return QByteArray::number(quint64(symbol), 16); }
	
	void populateStyleTable(const std::vector<const Symbol*>& symbols);

	void setupGeoreferencing(GDALDriverH po_driver);
	void setupQuirks(GDALDriverH po_driver);

private:
	const char* id;
	ogr::unique_datasource po_ds;
	ogr::unique_fielddefn o_name_field;
	ogr::unique_srs map_srs;
	ogr::unique_styletable table;
	ogr::unique_transformation transformation;
	
	const char* symbol_field;

	OgrQuirks quirks;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OGR_FILE_FORMAT_P_H
