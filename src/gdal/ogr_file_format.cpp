/*
 *    Copyright 2016-2018 Kai Pastor
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

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>
#include <type_traits>

#include <cpl_error.h>
#include <cpl_conv.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>

#include <QtGlobal>
#include <QtMath>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QHash>
#include <QLatin1Char>
#include <QLatin1String>
#include <QPointF>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScopedValueRollback>
#include <QString>
#include <QStringRef>
#include <QVariant>

#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_coord.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_import_export.h"
#include "gdal/gdal_manager.h"

// IWYU pragma: no_forward_declare QFile


namespace OpenOrienteering {

namespace ogr {
	
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
	
}  // namespace ogr



namespace {
	
	void applyPenWidth(OGRStyleToolH tool, LineSymbol* line_symbol)
	{
		int is_null;
		auto pen_width = OGR_ST_GetParamDbl(tool, OGRSTPenWidth, &is_null);
		if (!is_null)
		{
			Q_ASSERT(OGR_ST_GetUnit(tool) == OGRSTUMM);
	
			if (pen_width <= 0.01)
				pen_width = 0.1;
			
			line_symbol->setLineWidth(pen_width);
		}
	}
	
	void applyPenCap(OGRStyleToolH tool, LineSymbol* line_symbol)
	{
		int is_null;
		auto pen_cap = OGR_ST_GetParamStr(tool, OGRSTPenCap, &is_null);
		if (!is_null)
		{
			switch (pen_cap[0])
			{
			case 'p':
				line_symbol->setCapStyle(LineSymbol::SquareCap);
				break;
			case 'r':
				line_symbol->setCapStyle(LineSymbol::RoundCap);
				break;
			default:
				;
			}
		}
	}

	void applyPenJoin(OGRStyleToolH tool, LineSymbol* line_symbol)
	{
		int is_null;
		auto pen_join = OGR_ST_GetParamStr(tool, OGRSTPenJoin, &is_null);
		if (!is_null)
		{
			switch (pen_join[0])
			{
			case 'b':
				line_symbol->setJoinStyle(LineSymbol::BevelJoin);
				break;
			case 'r':
				line_symbol->setJoinStyle(LineSymbol::RoundJoin);
				break;
			default:
				;
			}
		}
	}
	
	void applyPenPattern(OGRStyleToolH tool, LineSymbol* line_symbol)
	{
		int is_null;
		auto raw_pattern = OGR_ST_GetParamStr(tool, OGRSTPenPattern, &is_null);
		if (!is_null)
		{
			auto pattern = QString::fromLatin1(raw_pattern);
			auto sub_pattern_re = QRegularExpression(QString::fromLatin1("([0-9.]+)([a-z]*) *([0-9.]+)([a-z]*)"));
			auto match = sub_pattern_re.match(pattern);
			double length_0{}, length_1{};
			bool ok = match.hasMatch();
			if (ok)
				length_0 = match.capturedRef(1).toDouble(&ok);
			if (ok)
				length_1 = match.capturedRef(3).toDouble(&ok);
			if (ok)
			{
				/// \todo Apply units from capture 2 and 4
				line_symbol->setDashed(true);
				line_symbol->setDashLength(qMax(100, qRound(length_0 * 1000)));
				line_symbol->setBreakLength(qMax(100, qRound(length_1 * 1000)));
			}
			else
			{
				qDebug("OgrFileFormat: Failed to parse dash pattern '%s'", raw_pattern);
			}
		}
	}
	
#if 0
	int getFontSize(const char* font_size_string)
	{
		auto pattern = QString::fromLatin1(font_size_string);
		auto sub_pattern_re = QRegularExpression(QString::fromLatin1("([0-9.]+)([a-z]*)"));
		auto match = sub_pattern_re.match(pattern);
		double font_size;
		bool ok = match.hasMatch();
		if (ok)
			font_size = match.capturedRef(1).toDouble(&ok);
		if (ok)
		{
			auto unit = match.capturedRef(2).toUtf8();
			if (!unit.isEmpty())
			{
				if (unit == "pt")
				{
					
				}
				else if (unit == "px")
				{
					
				}
				else
				{
					qDebug("OgrFileFormat: Unsupported font size unit '%s'", unit.constData());
				}
			}
		}
		else
		{
			qDebug("OgrFileFormat: Failed to parse font size '%s'", font_size_string);
			font_size = 0;
		}
		return font_size;
	}
#endif
	
	void applyLabelAnchor(int anchor, TextObject* text_object)
	{
		auto v_align = (anchor - 1) / 3;
		switch (v_align)
		{
		case 0:
			text_object->setVerticalAlignment(TextObject::AlignBaseline);
			break;
		case 1:
			text_object->setVerticalAlignment(TextObject::AlignVCenter);
			break;
		case 2:
			text_object->setVerticalAlignment(TextObject::AlignTop);
			break;
		case 3:
			text_object->setVerticalAlignment(TextObject::AlignBottom);
			break;
		default:
			Q_UNREACHABLE();
		}
		auto h_align = (anchor - 1) % 3;
		switch (h_align)
		{
		case 0:
			text_object->setHorizontalAlignment(TextObject::AlignLeft);
			break;
		case 1:
			text_object->setHorizontalAlignment(TextObject::AlignHCenter);
			break;
		case 2:
			text_object->setHorizontalAlignment(TextObject::AlignRight);
			break;
		default:
			Q_UNREACHABLE();
		}
	}

	QString toPrettyWkt(OGRSpatialReferenceH spatial_reference)
	{
		char* srs_wkt_raw = nullptr;
		OSRExportToPrettyWkt(spatial_reference, &srs_wkt_raw, 0);
		auto srs_wkt = QString::fromLocal8Bit(srs_wkt_raw);
		CPLFree(srs_wkt_raw);
		return srs_wkt;
	}
	
}  // namespace



// ### OgrFileFormat ###

OgrFileFormat::OgrFileFormat()
 : FileFormat(OgrFile, "OGR", ::OpenOrienteering::ImportExport::tr("Geospatial vector data"), QString{}, ImportSupported)
{
	for (const auto& extension : GdalManager().supportedVectorExtensions())
		addExtension(QString::fromLatin1(extension));
}


std::unique_ptr<Importer> OgrFileFormat::makeImporter(const QString& path, Map* map, MapView* view) const
{
	return std::make_unique<OgrFileImport>(path, map, view);
}



// ### OgrFileImport ###

OgrFileImport::OgrFileImport(const QString& path, Map* map, MapView* view, UnitType unit_type)
 : Importer(path, map, view)
 , manager{ OGR_SM_Create(nullptr) }
 , unit_type{ unit_type }
 , georeferencing_import_enabled{ true }
{
	GdalManager().configure();
	
	setOption(QLatin1String{ "Separate layers" }, QVariant{ false });
	
	// OGR feature style defaults
	default_pen_color = new MapColor(tr("Purple"), 0); 
	default_pen_color->setSpotColorName(QLatin1String{"PURPLE"});
	default_pen_color->setCmyk({0.2f, 1.0, 0.0, 0.0});
	default_pen_color->setRgbFromCmyk();
	map->addColor(default_pen_color, 0);
	
	auto default_brush_color = new MapColor(default_pen_color->getName() + QLatin1String(" 50%"), 0);
	default_brush_color->setSpotColorComposition({ {default_pen_color, 0.5f} });
	default_brush_color->setCmykFromSpotColors();
	default_brush_color->setRgbFromSpotColors();
	map->addColor(default_brush_color, 1);
	
	default_point_symbol = new PointSymbol();
	default_point_symbol->setName(tr("Point"));
	default_point_symbol->setNumberComponent(0, 1);
	default_point_symbol->setInnerColor(default_pen_color);
	default_point_symbol->setInnerRadius(500); // (um)
	map->addSymbol(default_point_symbol, 0);
	
	default_line_symbol = new LineSymbol();
	default_line_symbol->setName(tr("Line"));
	default_line_symbol->setNumberComponent(0, 2);
	default_line_symbol->setColor(default_pen_color);
	default_line_symbol->setLineWidth(0.1); // (0.1 mm, nearly cosmetic)
	default_line_symbol->setCapStyle(LineSymbol::FlatCap);
	default_line_symbol->setJoinStyle(LineSymbol::MiterJoin);
	map->addSymbol(default_line_symbol, 1);
	
	default_area_symbol = new AreaSymbol();
	default_area_symbol->setName(tr("Area"));
	default_area_symbol->setNumberComponent(0, 3);
	default_area_symbol->setColor(default_brush_color);
	map->addSymbol(default_area_symbol, 2);
	
	default_text_symbol = new TextSymbol();
	default_text_symbol->setName(tr("Text"));
	default_text_symbol->setNumberComponent(0, 4);
	default_text_symbol->setColor(default_pen_color);
	map->addSymbol(default_text_symbol, 3);
}


OgrFileImport::~OgrFileImport() = default;  // not inlined



bool OgrFileImport::supportsQIODevice() const noexcept
{
	return false;
}



void OgrFileImport::setGeoreferencingImportEnabled(bool enabled)
{
	georeferencing_import_enabled = enabled;
}



ogr::unique_srs OgrFileImport::srsFromMap()
{
	auto srs = ogr::unique_srs(OSRNewSpatialReference(nullptr));
	auto& georef = map->getGeoreferencing();
	if (georef.isValid() && !georef.isLocal())
	{
		OSRSetProjCS(srs.get(), "Projected map SRS");
		OSRSetWellKnownGeogCS(srs.get(), "WGS84");
		auto spec = QByteArray(georef.getProjectedCRSSpec().toLatin1() + " +wktext");
		auto error = OSRImportFromProj4(srs.get(), spec);
		if (!error)
			return srs;
		
		addWarning(tr("Unable to setup \"%1\" SRS for GDAL: %2")
		           .arg(QString::fromLatin1(spec), QString::number(error)));
		srs.reset(OSRNewSpatialReference(nullptr));
	}
	
	OSRSetLocalCS(srs.get(), "Local SRS");
	return srs;
}



bool OgrFileImport::importImplementation()
{
	// GDAL 2.0: ... = GDALOpenEx(template_path.toLatin1(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
	auto data_source = ogr::unique_datasource(OGROpen(path.toUtf8().constData(), 0, nullptr));
	if (!data_source)
	{
		addWarning(Importer::tr("Cannot open file\n%1:\n%2").arg(path, QString::fromLatin1(CPLGetLastErrorMsg())));
		return false;
	}
	
	if (auto driver = OGR_DS_GetDriver(data_source.get()))
	{
		if (auto driver_name = OGR_Dr_GetName(driver))
		{
			map->setSymbolSetId(QString::fromLatin1(driver_name));
		}
	}
	
	empty_geometries = 0;
	no_transformation = 0;
	failed_transformation = 0;
	unsupported_geometry_type = 0;
	too_few_coordinates = 0;
	
	if (georeferencing_import_enabled)
		map_srs = importGeoreferencing(data_source.get());
	else
		map_srs = srsFromMap();
	
	importStyles(data_source.get());

	if (!loadSymbolsOnly())
	{
		QScopedValueRollback<MapCoord::BoundsOffset> rollback { MapCoord::boundsOffset() };
		MapCoord::boundsOffset().reset(true);
		
		auto num_layers = OGR_DS_GetLayerCount(data_source.get());
		for (int i = 0; i < num_layers; ++i)
		{
			auto layer = OGR_DS_GetLayer(data_source.get(), i);
			if (!layer)
			{
				addWarning(tr("Unable to load layer %1.").arg(i));
				continue;
			}
			
			if (qstrcmp(OGR_L_GetName(layer), "track_points") == 0)
			{
				// Skip GPX track points as points. Track line is separate.
				/// \todo Use hooks and delegates per file format
				continue;
			}
			
			auto part = map->getCurrentPart();
			if (option(QLatin1String("Separate layers")).toBool())
			{
				if (num_layers > 0)
				{
					if (part->getNumObjects() == 0)
					{
						part->setName(QString::fromUtf8(OGR_L_GetName(layer)));
					}
					else
					{
						part = new MapPart(QString::fromUtf8(OGR_L_GetName(layer)), map);
						auto index = std::size_t(map->getNumParts());
						map->addPart(part, index);
						map->setCurrentPartIndex(index);
					}
				}
			}
				
			importLayer(part, layer);
		}
		
		const auto& offset = MapCoord::boundsOffset();
		if (!offset.isZero())
		{
			// We need to adjust the georeferencing.
			auto offset_f  = MapCoordF { offset.x / 1000.0, offset.y / 1000.0 };
			auto georef = map->getGeoreferencing();
			auto ref_point = MapCoordF { georef.getMapRefPoint() };
			auto new_projected = georef.toProjectedCoords(ref_point + offset_f);
			georef.setProjectedRefPoint(new_projected, false);
			map->setGeoreferencing(georef);
		}
	}
	
	if (empty_geometries)
	{
		addWarning(tr("Unable to load %n objects, reason: %1", nullptr, empty_geometries)
		           .arg(tr("Empty geometry.")));
	}
	if (no_transformation)
	{
		addWarning(tr("Unable to load %n objects, reason: %1", nullptr, no_transformation)
		           .arg(tr("Can't determine the coordinate transformation: %1").arg(QString::fromUtf8(CPLGetLastErrorMsg()))));
	}
	if (failed_transformation)
	{
		addWarning(tr("Unable to load %n objects, reason: %1", nullptr, failed_transformation)
		           .arg(tr("Failed to transform the coordinates.")));
	}
	if (unsupported_geometry_type)
	{
		addWarning(tr("Unable to load %n objects, reason: %1", nullptr, unsupported_geometry_type)
		           .arg(tr("Unknown or unsupported geometry type.")));
	}
	if (too_few_coordinates)
	{
		addWarning(tr("Unable to load %n objects, reason: %1", nullptr, too_few_coordinates)
		           .arg(tr("Not enough coordinates.")));
	}
	
	return true;
}



ogr::unique_srs OgrFileImport::importGeoreferencing(OGRDataSourceH data_source)
{
	auto no_srs = true;
	auto local_srs = ogr::unique_srs { nullptr };
	auto suitable_srs = ogr::unique_srs { nullptr };
	char* projected_srs_spec =  { nullptr };
	
	auto orthographic = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
	OSRSetProjCS(orthographic.get(), "Orthographic SRS");
	OSRSetWellKnownGeogCS(orthographic.get(), "WGS84");
	OSRSetOrthographic(orthographic.get(), 0.0, 0.0, 0.0, 0.0);
	
	// Find any SRS which can be transformed to our orthographic SRS,
	// but prefer a projected SRS.
	auto num_layers = OGR_DS_GetLayerCount(data_source);
	for (int i = 0; i < num_layers; ++i)
	{
		auto layer = OGR_DS_GetLayer(data_source, i);
		if (!layer)
		    continue;
		             
		auto spatial_reference = OGR_L_GetSpatialRef(layer);
		if (!spatial_reference)
			continue;
		
		no_srs = false;
		
		if (OSRIsLocal(spatial_reference))
		{
			if (!local_srs)
				local_srs.reset(OSRClone(spatial_reference));
			continue;
		}
		
		auto transformation = OCTNewCoordinateTransformation(spatial_reference, orthographic.get());
		if (!transformation)
		{
			addWarning(tr("Cannot use this spatial reference:\n%1").arg(toPrettyWkt(spatial_reference)));
			continue;
		}
		OCTDestroyCoordinateTransformation(transformation);
		
		if (OSRIsProjected(spatial_reference))
		{
			char *srs_spec = nullptr;
			auto error = OSRExportToProj4(spatial_reference, &srs_spec);
			if (!error)
			{
				projected_srs_spec = srs_spec;  // transfer ownership
				suitable_srs.reset(OSRClone(spatial_reference));
				break;
			}
			CPLFree(srs_spec);
		}
		
		if (!suitable_srs)
			suitable_srs.reset(OSRClone(spatial_reference));
	}
	
	if (projected_srs_spec)
	{
		// Found a suitable projected SRS
		auto georef = map->getGeoreferencing();  // copy
		georef.setProjectedCRS(QStringLiteral("PROJ.4"), QString::fromLatin1(projected_srs_spec));
		map->setGeoreferencing(georef);
		CPLFree(projected_srs_spec);
		return suitable_srs;
	}
	else if (suitable_srs)
	{
		// Found a suitable SRS but it is not projected.
		// Setting up a local orthographic projection.
		auto center = calcAverageLatLon(data_source);
		auto latitude = 0.001 * qRound(1000 * center.latitude());
		auto longitude = 0.001 * qRound(1000 * center.longitude());
		auto ortho_georef = Georeferencing();
		ortho_georef.setScaleDenominator(int(map->getScaleDenominator()));
		ortho_georef.setProjectedCRS(QString{},
		                             QString::fromLatin1("+proj=ortho +datum=WGS84 +ellps=WGS84 +units=m +lat_0=%1 +lon_0=%2 +no_defs")
		                             .arg(latitude, 0, 'f')
		                             .arg(longitude, 0, 'f') );
		ortho_georef.setProjectedRefPoint({}, false);
		ortho_georef.setDeclination(map->getGeoreferencing().getDeclination());
		map->setGeoreferencing(ortho_georef);
		return srsFromMap();
	}
	else if (local_srs || no_srs)
	{
		auto georef = Georeferencing();
		georef.setScaleDenominator(int(map->getScaleDenominator()));
		georef.setDeclination(map->getGeoreferencing().getDeclination());
		map->setGeoreferencing(georef);
		return local_srs ? std::move(local_srs) : srsFromMap();
	}
	else
	{
		throw FileFormatException(tr("The geospatial data has no suitable spatial reference."));
	}
}



void OgrFileImport::importStyles(OGRDataSourceH data_source)
{
	//auto style_table = OGR_DS_GetStyleTable(data_source);
	Q_UNUSED(data_source)
}

void OgrFileImport::importLayer(MapPart* map_part, OGRLayerH layer)
{
	Q_ASSERT(map_part);
	
	auto feature_definition = OGR_L_GetLayerDefn(layer);
	
	OGR_L_ResetReading(layer);
	while (auto feature = ogr::unique_feature(OGR_L_GetNextFeature(layer)))
	{
		auto geometry = OGR_F_GetGeometryRef(feature.get());
		if (!geometry || OGR_G_IsEmpty(geometry))
		{
			++empty_geometries;
			continue;
		}
		
		importFeature(map_part, feature_definition, feature.get(), geometry);
	}
}

void OgrFileImport::importFeature(MapPart* map_part, OGRFeatureDefnH feature_definition, OGRFeatureH feature, OGRGeometryH geometry)
{
	to_map_coord = &OgrFileImport::fromProjected;
	auto new_srs = OGR_G_GetSpatialReference(geometry);
	if (new_srs && data_srs != new_srs)
	{
		// New SRS, indeed.
		auto transformation = ogr::unique_transformation{ OCTNewCoordinateTransformation(new_srs, map_srs.get()) };
		if (!transformation)
		{
			++no_transformation;
			return;
		}
		
		// Commit change to data srs and coordinate transformation
		data_srs = new_srs;
		data_transform = std::move(transformation);
	}
	
	if (new_srs)
	{
		auto error = OGR_G_Transform(geometry, data_transform.get());
		if (error)
		{
			++failed_transformation;
			return;
		}
	}
	else if (unit_type == UnitOnPaper)
	{
		to_map_coord = &OgrFileImport::fromDrawing;
	}
	
	auto objects = importGeometry(feature, geometry);
	for (auto object : objects)
	{
		map_part->addObject(object);
		if (!feature_definition)
			continue;
		
		auto num_fields = OGR_FD_GetFieldCount(feature_definition);
		for (int i = 0; i < num_fields; ++i)
		{
			auto value = OGR_F_GetFieldAsString(feature, i);
			if (value && qstrlen(value) > 0)
			{
				auto field_definition = OGR_FD_GetFieldDefn(feature_definition, i);
				object->setTag(QString::fromUtf8(OGR_Fld_GetNameRef(field_definition)), QString::fromUtf8(value));
			}
		}
	}
}

OgrFileImport::ObjectList OgrFileImport::importGeometry(OGRFeatureH feature, OGRGeometryH geometry)
{
	ObjectList result;
	auto geometry_type = wkbFlatten(OGR_G_GetGeometryType(geometry));
	switch (geometry_type)
	{
	case OGRwkbGeometryType::wkbPoint:
		if (auto object = importPointGeometry(feature, geometry))
			result = { object };
		break;
		
	case OGRwkbGeometryType::wkbLineString:
		if (auto object = importLineStringGeometry(feature, geometry))
			result = { object };
		break;
		
	case OGRwkbGeometryType::wkbPolygon:
		if (auto object = importPolygonGeometry(feature, geometry))
			result = { object };
		break;
		
	case OGRwkbGeometryType::wkbGeometryCollection:
	case OGRwkbGeometryType::wkbMultiLineString:
	case OGRwkbGeometryType::wkbMultiPoint:
	case OGRwkbGeometryType::wkbMultiPolygon:
		result = importGeometryCollection(feature, geometry);
		break;
		
	default:
		qDebug("OgrFileImport: Unknown or unsupported geometry type: %d", geometry_type);
		++unsupported_geometry_type;
	}
	return result;
}

OgrFileImport::ObjectList OgrFileImport::importGeometryCollection(OGRFeatureH feature, OGRGeometryH geometry)
{
	ObjectList result;
	auto num_geometries = OGR_G_GetGeometryCount(geometry);
	result.reserve(std::size_t(num_geometries));
	for (int i = 0; i < num_geometries; ++i)
	{
		auto tmp = importGeometry(feature, OGR_G_GetGeometryRef(geometry, i));
		result.insert(result.end(), begin(tmp), end(tmp));
	}
	return result;
}

Object* OgrFileImport::importPointGeometry(OGRFeatureH feature, OGRGeometryH geometry)
{
	auto style = OGR_F_GetStyleString(feature);
	auto symbol = getSymbol(Symbol::Point, style);
	if (symbol->getType() == Symbol::Point)
	{
		auto object = new PointObject(symbol);
		object->setPosition(toMapCoord(OGR_G_GetX(geometry, 0), OGR_G_GetY(geometry, 0)));
		return object;
	}
	else if (symbol->getType() == Symbol::Text)
	{
		const auto& description = symbol->getDescription();
		auto length = description.length();
		auto split = description.indexOf(QLatin1Char(' '));
		Q_ASSERT(split > 0);
		Q_ASSERT(split < length);
		
		auto label = description.right(length - split - 1);
		if (label.startsWith(QLatin1Char{'{'}) && label.endsWith(QLatin1Char{'}'}))
		{
			label.remove(0, 1);
			label.chop(1);
			int index = OGR_F_GetFieldIndex(feature, label.toLatin1().constData());
			if (index >= 0)
			{
				label = QString::fromUtf8(OGR_F_GetFieldAsString(feature, index));
			}
		}
		if (!label.isEmpty())
		{
			auto object = new TextObject(symbol);
			object->setAnchorPosition(toMapCoord(OGR_G_GetX(geometry, 0), OGR_G_GetY(geometry, 0)));
			// DXF observation
			label.replace(QRegularExpression(QString::fromLatin1("(\\\\[^;]*;)*"), QRegularExpression::MultilineOption), QString{});
			label.replace(QLatin1String("^I"), QLatin1String("\t"));
			object->setText(label);
			
			bool ok;
			auto anchor = QStringRef(&description, 1, 2).toInt(&ok);
			if (ok)
			{
				applyLabelAnchor(anchor, object);
			}
				
			auto angle = QStringRef(&description, 3, split-3).toFloat(&ok);
			if (ok)
			{
				object->setRotation(qDegreesToRadians(angle));
			}
			
			return object;
		}
	}
	
	return nullptr;
}

PathObject* OgrFileImport::importLineStringGeometry(OGRFeatureH feature, OGRGeometryH geometry)
{
	auto managed_geometry = ogr::unique_geometry(nullptr);
	if (OGR_G_GetGeometryType(geometry) != wkbLineString)
	{
		geometry = OGR_G_ForceToLineString(OGR_G_Clone(geometry));
		managed_geometry.reset(geometry);
	}
	
	auto num_points = OGR_G_GetPointCount(geometry);
	if (num_points < 2)
	{
		++too_few_coordinates;
		return nullptr;
	}
	
	auto style = OGR_F_GetStyleString(feature);
	auto object = new PathObject(getSymbol(Symbol::Line, style));
	for (int i = 0; i < num_points; ++i)
	{
		object->addCoordinate(toMapCoord(OGR_G_GetX(geometry, i), OGR_G_GetY(geometry, i)));
	}
	return object;
}

PathObject* OgrFileImport::importPolygonGeometry(OGRFeatureH feature, OGRGeometryH geometry)
{
	auto num_geometries = OGR_G_GetGeometryCount(geometry);
	if (num_geometries < 1)
	{
		++too_few_coordinates;
		return nullptr;
	}
	
	auto outline = OGR_G_GetGeometryRef(geometry, 0);
	auto managed_outline = ogr::unique_geometry(nullptr);
	if (OGR_G_GetGeometryType(outline) != wkbLineString)
	{
		outline = OGR_G_ForceToLineString(OGR_G_Clone(outline));
		managed_outline.reset(outline);
	}
	auto num_points = OGR_G_GetPointCount(outline);
	if (num_points < 3)
	{
		++too_few_coordinates;
		return nullptr;
	}
	
	auto style = OGR_F_GetStyleString(feature);
	auto object = new PathObject(getSymbol(Symbol::Area, style));
	for (int i = 0; i < num_points; ++i)
	{
		object->addCoordinate(toMapCoord(OGR_G_GetX(outline, i), OGR_G_GetY(outline, i)));
	}
	
	for (int g = 1; g < num_geometries; ++g)
	{
		bool start_new_part = true;
		auto hole = /*OGR_G_ForceToLineString*/(OGR_G_GetGeometryRef(geometry, g));
		auto num_points = OGR_G_GetPointCount(hole);
		for (int i = 0; i < num_points; ++i)
		{
			object->addCoordinate(toMapCoord(OGR_G_GetX(hole, i), OGR_G_GetY(hole, i)), start_new_part);
			start_new_part = false;
		}
	}
	
	object->closeAllParts();
	return object;
}

Symbol* OgrFileImport::getSymbol(Symbol::Type type, const char* raw_style_string)
{
	auto style_string = QByteArray::fromRawData(raw_style_string, qstrlen(raw_style_string));
	Symbol* symbol = nullptr;
	switch (type)
	{
	case Symbol::Point:
	case Symbol::Text:
		symbol = point_symbols.value(style_string);
		if (!symbol)
			symbol = getSymbolForPointGeometry(style_string);
		if (!symbol)
			symbol = default_point_symbol;
		break;
		
	case Symbol::Combined:
		/// \todo
		//  fall through
	case Symbol::Line:
		symbol = line_symbols.value(style_string);
		if (!symbol)
			symbol = getLineSymbol(style_string);
		if (!symbol)
			symbol = default_line_symbol;
		break;
		
	case Symbol::Area:
		symbol = area_symbols.value(style_string);
		if (!symbol)
			symbol = getAreaSymbol(style_string);
		if (!symbol)
			symbol = default_area_symbol;
		break;
		
	case Symbol::NoSymbol:
	case Symbol::AllSymbols:
		Q_UNREACHABLE();
	}
	
	Q_ASSERT(symbol);
	return symbol;
}

MapColor* OgrFileImport::makeColor(OGRStyleToolH tool, const char* color_string)
{
	auto key = QByteArray::fromRawData(color_string, qstrlen(color_string));
	auto color = colors.value(key);
	if (!color)
	{	
		int r, g, b, a;
		auto success = OGR_ST_GetRGBFromString(tool, color_string, &r, &g, &b, &a);
		if (!success)
		{
			color = default_pen_color;
		}
		else if (a > 0)
		{
			color = new MapColor(QString::fromUtf8(color_string), map->getNumColors());
			color->setRgb(QColor{ r, g, b });
			color->setCmykFromRgb();
			map->addColor(color, map->getNumColors());
		}
		
		key.detach();
		colors.insert(key, color);
	}
	
	return color;
}

void OgrFileImport::applyPenColor(OGRStyleToolH tool, LineSymbol* line_symbol)
{
	int is_null;
	auto color_string = OGR_ST_GetParamStr(tool, OGRSTPenColor, &is_null);
	if (!is_null)
	{
		auto color = makeColor(tool, color_string);
		if (color)
			line_symbol->setColor(color);
		else
			line_symbol->setHidden(true);
	}
}

void OgrFileImport::applyBrushColor(OGRStyleToolH tool, AreaSymbol* area_symbol)
{
	int is_null;
	auto color_string = OGR_ST_GetParamStr(tool, OGRSTBrushFColor, &is_null);
	if (!is_null)
	{
		auto color = makeColor(tool, color_string);
		if (color)
			area_symbol->setColor(color);
		else
			area_symbol->setHidden(true);
	}
}


Symbol* OgrFileImport::getSymbolForPointGeometry(const QByteArray& style_string)
{
	if (style_string.isEmpty())
		return nullptr;
	
	auto manager = this->manager.get();
	
	auto data = style_string.constData();
	if (!OGR_SM_InitStyleString(manager, data))
		return nullptr;
	
	auto num_parts = OGR_SM_GetPartCount(manager, data);
	if (!num_parts)
		return nullptr;
	
	Symbol* symbol = nullptr;
	for (int i = 0; !symbol && i < num_parts; ++i)
	{
		auto tool = OGR_SM_GetPart(manager, i, nullptr);
		if (!tool)
			continue;
		
		OGR_ST_SetUnit(tool, OGRSTUMM, map->getScaleDenominator());
		
		auto type = OGR_ST_GetType(tool);
		switch (type)
		{
		case OGRSTCBrush:
		case OGRSTCPen:
		case OGRSTCSymbol:
			symbol = getSymbolForOgrSymbol(tool, style_string);
			break;
			
		case OGRSTCLabel:
			symbol = getSymbolForLabel(tool, style_string);
			break;
			
		default:
			;
		}
		
		OGR_ST_Destroy(tool);
	}
	
	return symbol;
}

LineSymbol* OgrFileImport::getLineSymbol(const QByteArray& style_string)
{
	if (style_string.isEmpty())
		return nullptr;
	
	auto manager = this->manager.get();
	
	auto data = style_string.constData();
	if (!OGR_SM_InitStyleString(manager, data))
		return nullptr;
	
	auto num_parts = OGR_SM_GetPartCount(manager, data);
	if (!num_parts)
		return nullptr;
	
	LineSymbol* symbol = nullptr;
	for (int i = 0; !symbol && i < num_parts; ++i)
	{
		auto tool = OGR_SM_GetPart(manager, i, nullptr);
		if (!tool)
			continue;
		
		OGR_ST_SetUnit(tool, OGRSTUMM, map->getScaleDenominator());
		
		auto type = OGR_ST_GetType(tool);
		switch (type)
		{
		case OGRSTCPen:
			symbol = getSymbolForPen(tool, style_string);
			break;
			
		default:
			;
		}
		
		OGR_ST_Destroy(tool);
	}
	
	return symbol;
}

AreaSymbol* OgrFileImport::getAreaSymbol(const QByteArray& style_string)
{
	if (style_string.isEmpty())
		return nullptr;
	
	auto manager = this->manager.get();
	
	auto data = style_string.constData();
	if (!OGR_SM_InitStyleString(manager, data))
		return nullptr;
	
	auto num_parts = OGR_SM_GetPartCount(manager, data);
	if (!num_parts)
		return nullptr;
	
	AreaSymbol* symbol = nullptr;
	for (int i = 0; !symbol && i < num_parts; ++i)
	{
		auto tool = OGR_SM_GetPart(manager, i, nullptr);
		if (!tool)
			continue;
		
		OGR_ST_SetUnit(tool, OGRSTUMM, map->getScaleDenominator());
		
		auto type = OGR_ST_GetType(tool);
		switch (type)
		{
		case OGRSTCBrush:
			symbol = getSymbolForBrush(tool, style_string);
			break;
			
		default:
			;
		}
		
		OGR_ST_Destroy(tool);
	}
	
	return symbol;
}

PointSymbol* OgrFileImport::getSymbolForOgrSymbol(OGRStyleToolH tool, const QByteArray& style_string)
{
	auto raw_tool_key = OGR_ST_GetStyleString(tool);
	auto tool_key = QByteArray::fromRawData(raw_tool_key, qstrlen(raw_tool_key));
	auto symbol = point_symbols.value(tool_key);
	if (symbol && symbol->getType() == Symbol::Point)
		return static_cast<PointSymbol*>(symbol);
	
	int color_key;
	switch (OGR_ST_GetType(tool))
	{
	case OGRSTCBrush:
		color_key = OGRSTBrushFColor;
		break;
	case OGRSTCPen:
		color_key = OGRSTPenColor;
		break;
	case OGRSTCSymbol:
		color_key = OGRSTSymbolColor;
		break;
	default:
		return nullptr;
	};
	
	int is_null;
	auto color_string = OGR_ST_GetParamStr(tool, color_key, &is_null);
	if (is_null)
		return nullptr;
	
	auto point_symbol = duplicate<PointSymbol>(*default_point_symbol);
	auto color = makeColor(tool, color_string);
	if (color)
		point_symbol->setInnerColor(color);
	else
		point_symbol->setHidden(true);
	
	auto key = style_string;
	key.detach();
	point_symbols.insert(key, point_symbol.get());
	
	if (key != tool_key)
	{
		tool_key.detach();
		point_symbols.insert(tool_key, point_symbol.get());
	}
	
	auto ret = point_symbol.get();
	map->addSymbol(point_symbol.release(), map->getNumSymbols());
	return ret;
}

TextSymbol* OgrFileImport::getSymbolForLabel(OGRStyleToolH tool, const QByteArray& /*style_string*/)
{
	Q_ASSERT(OGR_ST_GetType(tool) == OGRSTCLabel);
	
	int is_null;
	auto label_string = OGR_ST_GetParamStr(tool, OGRSTLabelTextString, &is_null);
	if (is_null)
		return nullptr;
	
	auto color_string = OGR_ST_GetParamStr(tool, OGRSTLabelFColor, &is_null);
	auto font_size_string = OGR_ST_GetParamStr(tool, OGRSTLabelSize, &is_null);
	
	// Don't use the style string as a key: The style contains the label.
	QByteArray key;
	key.reserve(qstrlen(color_string) + qstrlen(font_size_string) + 1);
	key.append(color_string);
	key.append(font_size_string);
	auto text_symbol = static_cast<TextSymbol*>(text_symbols.value(key));
	if (!text_symbol)
	{
		auto copy = duplicate<TextSymbol>(*default_text_symbol);
		text_symbol = copy.get();
		
		auto color = makeColor(tool, color_string);
		if (color)
			text_symbol->setColor(color);
		else
			text_symbol->setHidden(true);
		
		auto font_size = OGR_ST_GetParamDbl(tool, OGRSTLabelSize, &is_null);
		if (!is_null && font_size > 0.0)
			text_symbol->scale(font_size / text_symbol->getFontSize());
		
		key.detach();
		text_symbols.insert(key, text_symbol);
		
		map->addSymbol(copy.release(), map->getNumSymbols());
	}
	
	auto anchor = qBound(1, OGR_ST_GetParamNum(tool, OGRSTLabelAnchor, &is_null), 12);
	if (is_null)
		anchor = 1;
	
	auto angle = OGR_ST_GetParamDbl(tool, OGRSTLabelAngle, &is_null);
	if (is_null)
		angle = 0.0;
	
	QString description;
	description.reserve(qstrlen(label_string) + 100);
	description.append(QString::number(100 + anchor));
	description.append(QString::number(angle, 'g', 1));
	description.append(QLatin1Char(' '));
	description.append(QString::fromUtf8(label_string));
	text_symbol->setDescription(description);
	
	return text_symbol;
}

LineSymbol* OgrFileImport::getSymbolForPen(OGRStyleToolH tool, const QByteArray& style_string)
{
	Q_ASSERT(OGR_ST_GetType(tool) == OGRSTCPen);
	
	auto raw_tool_key = OGR_ST_GetStyleString(tool);
	auto tool_key = QByteArray::fromRawData(raw_tool_key, qstrlen(raw_tool_key));
	auto symbol = line_symbols.value(tool_key);
	if (symbol && symbol->getType() == Symbol::Line)
		return static_cast<LineSymbol*>(symbol);
	
	auto line_symbol = duplicate<LineSymbol>(*default_line_symbol);
	applyPenColor(tool, line_symbol.get());
	applyPenWidth(tool, line_symbol.get());
	applyPenCap(tool, line_symbol.get());
	applyPenJoin(tool, line_symbol.get());
	applyPenPattern(tool, line_symbol.get());
	
	auto key = style_string;
	key.detach();
	line_symbols.insert(key, line_symbol.get());
	
	if (key != tool_key)
	{
		tool_key.detach();
		line_symbols.insert(tool_key, line_symbol.get());
	}
	
	auto ret = line_symbol.get();
	map->addSymbol(line_symbol.release(), map->getNumSymbols());
	return ret;
}

AreaSymbol* OgrFileImport::getSymbolForBrush(OGRStyleToolH tool, const QByteArray& style_string)
{
	Q_ASSERT(OGR_ST_GetType(tool) == OGRSTCBrush);
	
	auto raw_tool_key = OGR_ST_GetStyleString(tool);
	auto tool_key = QByteArray::fromRawData(raw_tool_key, qstrlen(raw_tool_key));
	auto symbol = area_symbols.value(tool_key);
	if (symbol && symbol->getType() == Symbol::Area)
		return static_cast<AreaSymbol*>(symbol);
	
	auto area_symbol = duplicate<AreaSymbol>(*default_area_symbol);
	applyBrushColor(tool, area_symbol.get());
	
	auto key = style_string;
	key.detach();
	area_symbols.insert(key, area_symbol.get());
	
	if (key != tool_key)
	{
		tool_key.detach();
		area_symbols.insert(tool_key, area_symbol.get());
	}
	
	auto s = area_symbol.get();
	map->addSymbol(area_symbol.release(), map->getNumSymbols());
	return s;
}


MapCoord OgrFileImport::fromDrawing(double x, double y) const
{
	return MapCoord::load(x, -y, MapCoord::Flags{});
}

MapCoord OgrFileImport::fromProjected(double x, double y) const
{
	return MapCoord::load(map->getGeoreferencing().toMapCoordF(QPointF{ x, y }), MapCoord::Flags{});
}


// static
bool OgrFileImport::checkGeoreferencing(const QString& path, const Georeferencing& georef)
{
	if (georef.isLocal() || !georef.isValid())
		return false;
	
	GdalManager();
	
	// GDAL 2.0: ... = GDALOpenEx(template_path.toLatin1(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
	auto data_source = ogr::unique_datasource(OGROpen(path.toUtf8().constData(), 0, nullptr));
	if (!data_source)
	{
		throw FileFormatException(QString::fromLatin1(CPLGetLastErrorMsg()));
	}
	
	return checkGeoreferencing(data_source.get(), georef);
}


// static
bool OgrFileImport::checkGeoreferencing(OGRDataSourceH data_source, const Georeferencing& georef)
{
	auto spec = QByteArray(georef.getProjectedCRSSpec().toLatin1() + " +wktext");
	auto map_srs = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
	OSRSetProjCS(map_srs.get(), "Projected map SRS");
	OSRSetWellKnownGeogCS(map_srs.get(), "WGS84");
	OSRImportFromProj4(map_srs.get(), spec.constData());
	
	bool suitable_srs_found = false;
	auto num_layers = OGR_DS_GetLayerCount(data_source);
	for (int i = 0; i < num_layers; ++i)
	{
		if (auto layer = OGR_DS_GetLayer(data_source, i))
		{
			if (auto spatial_reference = OGR_L_GetSpatialRef(layer))
			{
				auto transformation = OCTNewCoordinateTransformation(spatial_reference, map_srs.get());
				if (!transformation)
				{
					qDebug("Failed to transform this SRS:\n%s", qPrintable(toPrettyWkt(spatial_reference)));
					return false;
				}
				
				OCTDestroyCoordinateTransformation(transformation);
				suitable_srs_found = true;
			}
		}
	}
	
	return suitable_srs_found;
}



// static
LatLon OgrFileImport::calcAverageLatLon(const QString& path)
{
	GdalManager();
	
	// GDAL 2.0: ... = GDALOpenEx(template_path.toLatin1(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
	auto data_source = ogr::unique_datasource(OGROpen(path.toUtf8().constData(), 0, nullptr));
	if (!data_source)
	{
		throw FileFormatException(QString::fromLatin1(CPLGetLastErrorMsg()));
	}
	
	return calcAverageLatLon(data_source.get());
}


// static
LatLon OgrFileImport::calcAverageLatLon(OGRDataSourceH data_source)
{
	auto geo_srs = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
	OSRSetWellKnownGeogCS(geo_srs.get(), "WGS84");
	
	auto num_coords = 0u;
	double x = 0, y = 0;
	auto num_layers = OGR_DS_GetLayerCount(data_source);
	for (int i = 0; i < num_layers; ++i)
	{
		if (auto layer = OGR_DS_GetLayer(data_source, i))
		{
			auto spatial_reference = OGR_L_GetSpatialRef(layer);
			if (!spatial_reference)
				continue;
			
			auto transformation = ogr::unique_transformation{ OCTNewCoordinateTransformation(spatial_reference, geo_srs.get()) };
			if (!transformation)
				continue;
			
			OGR_L_ResetReading(layer);
			while (auto feature = ogr::unique_feature(OGR_L_GetNextFeature(layer)))
			{
				auto geometry = OGR_F_GetGeometryRef(feature.get());
				if (!geometry || OGR_G_IsEmpty(geometry))
					continue;
				
				auto error = OGR_G_Transform(geometry, transformation.get());
				if (error)
					continue;
				
				auto num_points = OGR_G_GetPointCount(geometry);
				for (int i = 0; i < num_points; ++i)
				{
					x += OGR_G_GetX(geometry, i);
					y += OGR_G_GetY(geometry, i);
					++num_coords;
				}
			}
		}
	}
	
	return num_coords ? LatLon{ y / num_coords, x / num_coords } : LatLon{};
}


}  // namespace OpenOrienteering
