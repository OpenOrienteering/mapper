/*
 *    Copyright 2016 Kai Pastor
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

#include "ogr_file_format.h"
#include "ogr_file_format_p.h"

#include <memory>

#include <cpl_error.h>
#include <ogr_srs_api.h>

#include <QFile>
#include <QFileInfo>

#include "../core/georeferencing.h"
#include "../map.h"
#include "../symbol_line.h"
#include "../symbol_point.h"


namespace ogr
{
	class OGRDataSourceHDeleter
	{
	public:
		void operator()(OGRDataSourceH data_source) const
		{
			OGRReleaseDataSource(data_source);
		}
	};
	
	class OGRFeatureHDeleter
	{
	public:
		void operator()(OGRFeatureH feature) const
		{
			OGR_F_Destroy(feature);
		}
	};
	
	/** A convenience class for OGR C API datasource handles, similar to std::unique_ptr. */
	using unique_datasource = std::unique_ptr<typename std::remove_pointer<OGRDataSourceH>::type, OGRDataSourceHDeleter>;
	
	/** A convenience class for OGR C API feature handles, similar to std::unique_ptr. */
	using unique_feature = std::unique_ptr<typename std::remove_pointer<OGRFeatureH>::type, OGRFeatureHDeleter>;
}



// ### OgrFileFormat ###

OgrFileFormat::OgrFileFormat()
 : FileFormat(OgrFile, "OGR", ImportExport::tr("Geospatial vector data"), QString::null, ImportSupported)
{
	static bool ogr_registered = false;
	if (!ogr_registered)
	{
		// GDAL 2.0: GDALAllRegister();
		OGRRegisterAll();
		ogr_registered = true;
	}
	
	/// \todo Query GDAL/OGR 2.0 for drivers and extensions
	addExtension(QLatin1String("dxf"));
	addExtension(QLatin1String("shp"));
}

bool OgrFileFormat::understands(const unsigned char*, size_t) const
{
	return true;
}

Importer* OgrFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const
{
	return new OgrFileImport(stream, map, view);
}



// ### OgrFileImport ###

OgrFileImport::OgrFileImport(QIODevice* stream, Map* map, MapView* view)
 : Importer(stream, map, view)
{
    // nothing
}

OgrFileImport::~OgrFileImport()
{
	// nothing
}

void OgrFileImport::import(bool load_symbols_only)
{
	auto file = qobject_cast<QFile*>(stream);
	if (!file)
		return;
	
	auto template_file = file->fileName();
	if (template_file.isEmpty())
		return;
	
	qDebug("OgrFileImport: File to import: %s", qPrintable(template_file));
	
	// GDAL 2.0: ... = GDALOpenEx(template_path.toLatin1(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
	OGRSFDriverH* driver = nullptr;
	auto data_source = ogr::unique_datasource(OGROpen(template_file.toLatin1(), 0, driver));
	if (data_source == nullptr)
	{
		throw FileFormatException(Importer::tr("Could not read file: %1").arg(CPLGetLastErrorMsg()));
	}
	
	importStyles(data_source.get());

	if (!load_symbols_only)
	{
		auto num_layers = OGR_DS_GetLayerCount(data_source.get());
		qDebug("OgrFileImport: Layers to import: %d", num_layers);
		for (int i = 0; i < num_layers; ++i)
		{
			if (auto layer = OGR_DS_GetLayer(data_source.get(), i))
			{
				importLayer(layer);
			}
		}
	}
}

void OgrFileImport::importStyles(OGRDataSourceH data_source)
{
	auto style_table = OGR_DS_GetStyleTable(data_source);
	Q_UNUSED(style_table) /// \todo Read style from style_table
	
	auto color = new MapColor("Default", 0);
	color->setCmyk({0, 1, 0, 0});
	map->addColor(color, 0);
	
	auto point_symbol = new PointSymbol();
	point_symbol->setName(tr("Point"));
	point_symbol->setNumberComponent(0, 1);
	point_symbol->setInnerColor(color);
	map->addSymbol(point_symbol, 0);
	symbol_table.insert(QLatin1String("POINT"), point_symbol);
	
	auto line_symbol = new LineSymbol();
	line_symbol->setName(tr("Line"));
	line_symbol->setNumberComponent(0, 2);
	line_symbol->setColor(color);
	line_symbol->setLineWidth(0.2); // mm
	map->addSymbol(line_symbol, 1);
	symbol_table.insert(QLatin1String("LINE"), line_symbol);
}

void OgrFileImport::importLayer(OGRLayerH layer)
{
	MapPart* map_part = map->getCurrentPart();
	Q_ASSERT(map_part);
	
	OGR_L_ResetReading(layer);
	while (auto feature = ogr::unique_feature(OGR_L_GetNextFeature(layer)))
	{
		if (auto geometry = OGR_F_GetGeometryRef(feature.get()))
		{
			auto srs = OGR_G_GetSpatialReference(geometry);
			if (srs)
				qDebug("OgrFileImport: Linear unit: %f", OSRGetLinearUnits(srs, 0));
			
			importGeometry(map_part, feature.get(), geometry);
		}
	}
}

void OgrFileImport::importGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry)
{
	auto geometry_type = wkbFlatten(OGR_G_GetGeometryType(geometry));
	switch (geometry_type)
	{
	case OGRwkbGeometryType::wkbPoint:
		importPointGeometry(map_part, feature, geometry);
		break;
		
	case OGRwkbGeometryType::wkbLineString:
	case OGRwkbGeometryType::wkbPolygon:
		importLineStringGeometry(map_part, feature, geometry);
		break;
		
	case OGRwkbGeometryType::wkbGeometryCollection:
		importGeometryCollection(map_part, feature, geometry);
		break;
		
	default:
		qDebug("OgrFileImport: Unknown or unsupported geometry: %d", geometry_type);
	}
}

void OgrFileImport::importGeometryCollection(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry)
{
	auto num_geometries = OGR_G_GetGeometryCount(geometry);
	//qDebug("OgrFileImport: Members in geometry collection: %d", num_geometries);
	/// \todo: Check whether the parts can be joined in some way.
	for (int i = 0; i < num_geometries; ++i)
		importGeometry(map_part, feature, OGR_G_GetGeometryRef(geometry, i));
}

void OgrFileImport::importPointGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry)
{
	Q_UNUSED(feature)
	
	auto object = new PointObject(getSymbol(QLatin1String("POINT")));
	auto point = QPointF{ OGR_G_GetX(geometry, 0), OGR_G_GetY(geometry, 0) };
	object->setPosition(map->getGeoreferencing().toMapCoordF(point));
	map_part->addObject(object);
}

void OgrFileImport::importLineStringGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry)
{
	Q_UNUSED(feature)
	
	OGR_G_FlattenTo2D(geometry);
	geometry = OGR_G_ForceToLineString(geometry);
	
	auto num_points = OGR_G_GetPointCount(geometry);
	if (num_points >= 2)
	{
		auto object = new PathObject(getSymbol(QLatin1String("LINE")));
		for (int p = 0; p < num_points; ++p)
		{
			auto point = QPointF{ OGR_G_GetX(geometry, p), OGR_G_GetY(geometry, p) };
			object->addCoordinate(MapCoord{ map->getGeoreferencing().toMapCoordF(point) });
		}
		map_part->addObject(object);
	}
	else
	{
		qWarning("OgrFileImport: Short or empty line string, points: %d", num_points);
	}
}

Symbol* OgrFileImport::getSymbol(const QString& identifier)
{
	// TODO: Ad-hoc creation and registration of new Symbols
	auto symbol = symbol_table[identifier];
	Q_ASSERT(symbol);
	return symbol;
}
