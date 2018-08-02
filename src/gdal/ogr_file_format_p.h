/*
 *    Copyright 2016-2017 Kai Pastor
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

#include <memory>

#include <QByteArray>
#include <QCoreApplication>
#include <QHash>

// The GDAL/OGR C API is more stable than the C++ API.
#include <ogr_api.h>
#include <ogr_srs_api.h>

#include "core/map_coord.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_import_export.h"

class QFile;

namespace OpenOrienteering {

class AreaSymbol;
class Georeferencing;
class LatLon;
class LineSymbol;
class MapColor;
class MapPart;
class Object;
class PathObject;
class PointObject;
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
	
	/** A convenience class for OGR C API feature handles, similar to std::unique_ptr. */
	using unique_stylemanager = std::unique_ptr<typename std::remove_pointer<OGRStyleMgrH>::type, OGRStyleMgrHDeleter>;
}



/**
 * An Importer for geospatial vector data supported by OGR.
 * 
 * OGR needs to know the filename. The filename can be either derived from
 * a QFile passed as stream to the constructor, or set directly through the
 * option "filename".
 * 
 * The option "separate_layers" will cause OGR layers to be imported as distinct
 * map parts if set to true.
 */
class OgrFileImport : public Importer
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::OgrFileImport)
	
public:
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
	
	bool importImplementation() override;
	
	ogr::unique_srs importGeoreferencing(OGRDataSourceH data_source);
	
	void importStyles(OGRDataSourceH data_source);
	
	void importLayer(MapPart* map_part, OGRLayerH layer);
	
	void importFeature(MapPart* map_part, OGRFeatureDefnH feature_definition, OGRFeatureH feature, OGRGeometryH geometry);
	
	using ObjectList = std::vector<Object*>;
	
	ObjectList importGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	ObjectList importGeometryCollection(OGRFeatureH feature, OGRGeometryH geometry);
	
	Object* importPointGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	PathObject* importLineStringGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	PathObject* importPolygonGeometry(OGRFeatureH feature, OGRGeometryH geometry);
	
	
	Symbol* getSymbol(Symbol::Type type, const char* raw_style_string);
	
	MapColor* makeColor(OGRStyleToolH tool, const char* color_string);
	
	void applyPenColor(OGRStyleToolH tool, LineSymbol* line_symbol);
	
	void applyBrushColor(OGRStyleToolH tool, AreaSymbol* area_symbol);
	
	
	MapCoord toMapCoord(double x, double y) const;
	
	/**
	 * A MapCoordConstructor which interpretes the given coordinates in millimeters on paper.
	 */
	MapCoord fromDrawing(double x, double y) const;
	
	/**
	 * A MapCoordConstructor which interpretes the given coordinates as projected.
	 */
	MapCoord fromProjected(double x, double y) const;
	
	
	static LatLon calcAverageLatLon(OGRDataSourceH data_source);
	
	
private:
	Symbol* getSymbolForPointGeometry(const QByteArray& style_string);
	LineSymbol* getLineSymbol(const QByteArray& style_string);
	AreaSymbol* getAreaSymbol(const QByteArray& style_string);
	
	PointSymbol* getSymbolForOgrSymbol(OGRStyleToolH tool, const QByteArray& style_string);
	TextSymbol* getSymbolForLabel(OGRStyleToolH tool, const QByteArray& style_string);
	LineSymbol* getSymbolForPen(OGRStyleToolH tool, const QByteArray& style_string);
	AreaSymbol* getSymbolForBrush(OGRStyleToolH tool, const QByteArray& style_string);
	
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
	
	OGRSpatialReferenceH data_srs;
	
	ogr::unique_transformation data_transform;
	
	ogr::unique_stylemanager manager;
	
	int empty_geometries;
	int no_transformation;
	int failed_transformation;
	int unsupported_geometry_type;
	int too_few_coordinates;
	
	UnitType unit_type;
	
	bool georeferencing_import_enabled;
};



// ### inline code ###

inline
MapCoord OgrFileImport::toMapCoord(double x, double y) const
{
	return (this->*to_map_coord)(x, y);
}


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OGR_FILE_FORMAT_P_H
