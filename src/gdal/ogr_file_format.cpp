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
#include "ogr_file_format_p.h"  // IWYU pragma: associated

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

#include <cpl_conv.h>
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(2,0,0)
#  include <Qt>
#endif

#include <QtGlobal>
#include <QtMath>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFlags>
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
#include "core/path_coord.h"
#include "core/objects/object.h"
#include "core/objects/text_object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "fileformats/file_import_export.h"
#include "gdal/gdal_manager.h"

// IWYU pragma: no_forward_declare QFile

namespace OpenOrienteering {

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
				qDebug("OgrFileImportFormat: Failed to parse dash pattern '%s'", raw_pattern);
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
					qDebug("OgrFileImportFormat: Unsupported font size unit '%s'", unit.constData());
				}
			}
		}
		else
		{
			qDebug("OgrFileImportFormat: Failed to parse font size '%s'", font_size_string);
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
	
	
	
	QByteArray toRgbString(const MapColor* color)
	{
		auto rgb = QColor(color->getRgb()).name(QColor::HexArgb).toLatin1();
		std::rotate(rgb.begin()+1, rgb.begin()+3, rgb.end()); // Move alpha to end
		return rgb;
	}
	
	QByteArray makeStyleString(const PointSymbol* point_symbol)
	{
		QByteArray style;
		if (auto main_color = point_symbol->guessDominantColor())
		{
			style.reserve(40);
			style += "SYMBOL(id:\"ogr-sym-0\"";
			style += ",c:" + toRgbString(main_color);
			style += ",l:" + QByteArray::number(-main_color->getPriority());
			style += ")";
		}
		return style;
	}
	
	QByteArray makeStyleString(const LineSymbol* line_symbol)
	{
		QByteArray style;
		style.reserve(200);
		auto main_color = line_symbol->getColor();
		if (main_color && line_symbol->getLineWidth())
		{
			style += "PEN(c:" + toRgbString(main_color);
			style += ",w:" + QByteArray::number(line_symbol->getLineWidth()/1000.0) + "mm";
			if (line_symbol->isDashed())
				style += ",p:\"2mm 1mm\""; // TODO
			style += ",l:" + QByteArray::number(-main_color->getPriority());
			style += ");";
		}
		if (line_symbol->hasBorder())
		{
			const auto& left_border = line_symbol->getBorder();
			if (left_border.isVisible())
			{
				// left border
				style += "PEN(c:" + toRgbString(left_border.color);
				style += ",w:" + QByteArray::number(left_border.width/1000.0) + "mm";
				style += ",dp:" + QByteArray::number(-left_border.shift/1000.0) + "mm";
				style += ",l:" + QByteArray::number(-left_border.color->getPriority());
				if (left_border.dashed)
					style += ",p:\"2mm 1mm\""; // TODO
				style += ");";
			}
			const auto& right_border = line_symbol->getBorder();
			if (right_border.isVisible())
			{
				// left border
				style += "PEN(c:" + toRgbString(right_border.color);
				style += ",w:" + QByteArray::number(right_border.width/1000.0) + "mm";
				style += ",dp:" + QByteArray::number(right_border.shift/1000.0) + "mm";
				style += ",l:" + QByteArray::number(-right_border.color->getPriority());
				if (right_border.dashed)
					style += ",p:\"2mm 1mm\""; // TODO
				style += ");";
			}
		}
		if (style.isEmpty())
		{
			if (auto main_color = line_symbol->guessDominantColor())
			{
				style += "PEN(c:" + toRgbString(main_color);
				style += ",w:1pt";
				style += ",l:" + QByteArray::number(-main_color->getPriority());
				style += ')';
			}
		}
		if (style.endsWith(';'))
		{
			style.chop(1);
		}
		return style;
	}
	
	QByteArray makeStyleString(const AreaSymbol* area_symbol)
	{
		QByteArray style;
		style.reserve(200);
		if (auto color = area_symbol->getColor())
		{
			style += "BRUSH(fc:" + toRgbString(color);
			style += ",l:" + QByteArray::number(-color->getPriority());
			style += ");";
		}
		
		auto num_fill_patterns = area_symbol->getNumFillPatterns();
		for (int i = 0; i < num_fill_patterns; ++i)
		{
			auto part = area_symbol->getFillPattern(i);
			switch (part.type)
			{
			case AreaSymbol::FillPattern::LinePattern:
				if (!part.line_color)
					continue;
				style += "BRUSH(fc:" + toRgbString(part.line_color);
				style += ",id:\"ogr-brush-2\"";  // parallel horizontal lines
				style += ",a:" + QByteArray::number(qRadiansToDegrees(part.angle));
				style += ",l:" + QByteArray::number(-part.line_color->getPriority());
				style += ");";
				break;
			case AreaSymbol::FillPattern::PointPattern:
				// TODO
				qWarning("Cannot handle point pattern in area symbol %s",
				         qPrintable(area_symbol->getName()));
			}
		}
		if (style.endsWith(';'))
			style.chop(1);
		return style;
	}
	
	QByteArray makeStyleString(const TextSymbol* text_symbol)
	{
		QByteArray style;
		style.reserve(200);
		style += "LABEL(c:" + toRgbString(text_symbol->getColor());
		style += ",f:\"" + text_symbol->getFontFamily().toUtf8() + "\"";
		style += ",s:"+QByteArray::number(text_symbol->getFontSize())+ "mm";
		style += ",t:\"{Name}\"";
		style += ')';
		return style;
	}
	
	
	QByteArray makeStyleString(const CombinedSymbol* combined_symbol)
	{
		QByteArray style;
		style.reserve(200);
		for (auto i = combined_symbol->getNumParts() - 1; i >= 0; i--)
		{
			const auto* subsymbol = combined_symbol->getPart(i);
			if (subsymbol)
			{
				switch (subsymbol->getType())
				{
				case Symbol::Line:
					style += makeStyleString(subsymbol->asLine()) + ';';
					break;
				case Symbol::Area:
					style += makeStyleString(subsymbol->asArea()) + ';';
					break;
				case Symbol::Combined:
					style += makeStyleString(subsymbol->asCombined()) + ';';
					break;
				case Symbol::Point:
				case Symbol::Text:
					qWarning("Cannot handle point or text symbol in combined symbol %s",
					         qPrintable(combined_symbol->getName()));
					break;
				case Symbol::NoSymbol:
				case Symbol::AllSymbols:
					Q_UNREACHABLE();
				}
			}
		}
		if (style.endsWith(';'))
			style.chop(1);
		return style;
	}
	
	
	class AverageLatLon
	{
	private:
		double x = 0;
		double y = 0;
		unsigned num_coords = 0u;
		
		void handleGeometry(OGRGeometryH geometry)
		{
			auto const geometry_type = wkbFlatten(OGR_G_GetGeometryType(geometry));
			switch (geometry_type)
			{
			case OGRwkbGeometryType::wkbPoint:
			case OGRwkbGeometryType::wkbLineString:
				for (auto num_points = OGR_G_GetPointCount(geometry), i = 0; i < num_points; ++i)
				{
					x += OGR_G_GetX(geometry, i);
					y += OGR_G_GetY(geometry, i);
					++num_coords;
				}
				break;
				
			case OGRwkbGeometryType::wkbPolygon:
			case OGRwkbGeometryType::wkbMultiPoint:
			case OGRwkbGeometryType::wkbMultiLineString:
			case OGRwkbGeometryType::wkbMultiPolygon:
			case OGRwkbGeometryType::wkbGeometryCollection:
				for (auto num_geometries = OGR_G_GetGeometryCount(geometry), i = 0; i < num_geometries; ++i)
				{
					handleGeometry(OGR_G_GetGeometryRef(geometry, i));
				}
				break;
				
			default:
				;  // unsupported type, will be reported in importGeometry
			}
		}
		
	public:
		AverageLatLon(OGRDataSourceH data_source)
		{
			auto geo_srs = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
			OSRSetWellKnownGeogCS(geo_srs.get(), "WGS84");
			
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
						
						handleGeometry(geometry);
					}
				}
			}
		}
		
		operator LatLon() const
		{
			return num_coords ? LatLon{ y / num_coords, x / num_coords } : LatLon{};
		}
		
	};
	
	
}  // namespace



// ### OgrFileImportFormat ###

OgrFileImportFormat::OgrFileImportFormat()
 : FileFormat(OgrFile, "OGR",
              ::OpenOrienteering::ImportExport::tr("Geospatial vector data"),
              QString{},
              Feature::FileOpen | Feature::FileImport | Feature::ReadingLossy )
{
	for (const auto& extension : GdalManager().supportedVectorImportExtensions())
		addExtension(QString::fromLatin1(extension));
}


std::unique_ptr<Importer> OgrFileImportFormat::makeImporter(const QString& path, Map* map, MapView* view) const
{
	return std::make_unique<OgrFileImport>(path, map, view);
}


// ### OgrFileExportFormat ###

OgrFileExportFormat::OgrFileExportFormat()
 : FileFormat(OgrFile, "OGR-export",
              ::OpenOrienteering::ImportExport::tr("Geospatial vector data"),
              QString{},
              Feature::FileExport | Feature::WritingLossy )
{
	for (const auto& extension : GdalManager().supportedVectorExportExtensions())
		addExtension(QString::fromLatin1(extension));
}

std::unique_ptr<Exporter> OgrFileExportFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	return std::make_unique<OgrFileExport>(path, map, view);
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
	default_pen_color = new MapColor(QLatin1String{"Purple"}, 0); 
	default_pen_color->setSpotColorName(QLatin1String{"PURPLE"});
	default_pen_color->setCmyk({0.35f, 0.85f, 0.0, 0.0});
	default_pen_color->setRgbFromCmyk();
	map->addColor(default_pen_color, 0);
	
	// 50% opacity of 80% Purple should result in app. 40% Purple (on white) in
	// normal view and in an opaque Purple slightly lighter than lines and
	// points in overprinting simulation mode.
	auto default_brush_color = new MapColor(default_pen_color->getName() + QLatin1String(" 40%"), 0);
	default_brush_color->setSpotColorComposition({ {default_pen_color, 0.8f} });
	default_brush_color->setCmykFromSpotColors();
	default_brush_color->setRgbFromSpotColors();
	default_brush_color->setOpacity(0.5f);
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
	return AverageLatLon(data_source);
}

// ### OgrFileExport ###

OgrFileExport::OgrFileExport(const QString& path, const Map* map, const MapView* view)
: Exporter(path, map, view)
{
	GdalManager manager;
	bool one_layer_per_symbol = manager.isExportOptionEnabled(GdalManager::OneLayerPerSymbol);
	setOption(QString::fromLatin1("Per Symbol Layers"), one_layer_per_symbol);
}

OgrFileExport::~OgrFileExport() = default;

bool OgrFileExport::supportsQIODevice() const noexcept
{
	return false;
}

bool OgrFileExport::exportImplementation()
{
	// Choose driver and setup format-specific features
	QFileInfo info(path);
	QString file_extn = info.completeSuffix();
	GDALDriverH po_driver = nullptr;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
	auto count = GDALGetDriverCount();
	for (auto i = 0; i < count; ++i)
	{
		auto driver_data = GDALGetDriver(i);

		auto type = GDALGetMetadataItem(driver_data, GDAL_DCAP_VECTOR, nullptr);
		if (qstrcmp(type, "YES") != 0)
			continue;

		auto cap_create = GDALGetMetadataItem(driver_data, GDAL_DCAP_CREATE, nullptr);
		if (qstrcmp(cap_create, "YES") != 0)
			continue;

		auto extensions_raw = GDALGetMetadataItem(driver_data, GDAL_DMD_EXTENSIONS, nullptr);
		auto extensions = QByteArray::fromRawData(extensions_raw, int(qstrlen(extensions_raw)));
		for (auto pos = 0; pos >= 0; )
		{
			auto start = pos ? pos + 1 : 0;
			pos = extensions.indexOf(' ', start);
			auto extension = extensions.mid(start, pos - start);
			if (file_extn == QString::fromLatin1(extension))
			{
				po_driver = driver_data;
				break;
			}
		}
	}
#else
	const char *psz_driver_name;
	if (file_extn.compare(QString::fromLatin1("gpx"), Qt::CaseInsensitive) == 0)
	{
		psz_driver_name = "GPX";
	}
	else if (file_extn.compare(QString::fromLatin1("kml"), Qt::CaseInsensitive) == 0)
	{
		if (OGRGetDriverByName("LIBKML") != nullptr)
			psz_driver_name = "LIBKML";
		else
			psz_driver_name = "KML";
	}
	else if (file_extn.compare(QString::fromLatin1("shp"), Qt::CaseInsensitive) == 0)
	{
		psz_driver_name = "ESRI Shapefile";
	}
	else
	{
		throw FileFormatException(tr("Unknown file extension %1, only GPX, KML, and SHP files are supported.").arg(file_extn));
	}

	po_driver = OGRGetDriverByName(psz_driver_name);
#endif

	if (!po_driver)
		throw FileFormatException(tr("Couldn't find a driver for file extension %1").arg(file_extn));

	setupQuirks(po_driver);

	setupGeoreferencing(po_driver);

	// Create output dataset
	po_ds = ogr::unique_datasource(OGR_Dr_CreateDataSource(
	                                   po_driver,
	                                   path.toLatin1(),
	                                   nullptr));
	if (!po_ds)
		throw FileFormatException(tr("Failed to create dataset: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));

	// Name field definition
	if (quirks.testFlag(UseLayerField))
	{
		symbol_field = "Layer";
		o_name_field = nullptr;
	}
	else
	{
		symbol_field = "Name";
		o_name_field = ogr::unique_fielddefn(OGR_Fld_Create(symbol_field, OFTString));
		OGR_Fld_SetWidth(o_name_field.get(), 32);
	}

	auto symbols = symbolsForExport();
	
	// Setup style table
	populateStyleTable(symbols);

	auto is_point_object = [](const Object* object) {
		const auto* symbol = object->getSymbol();
		return symbol && symbol->getContainedTypes() & Symbol::Point;
	};

	auto is_text_object = [](const Object* object) {
		const auto* symbol = object->getSymbol();
		return symbol && symbol->getContainedTypes() & Symbol::Text;
	};

	auto is_line_object = [](const Object* object) {
		const auto* symbol = object->getSymbol();
		return symbol && (symbol->getType() == Symbol::Line
		                  || (symbol->getType() == Symbol::Combined && !(symbol->getContainedTypes() & Symbol::Area)));
	};

	auto is_area_object = [](const Object* object) {
		const auto* symbol = object->getSymbol();
		return symbol && symbol->getContainedTypes() & Symbol::Area;
	};

	if (quirks & SingleLayer)
	{
		auto layer = createLayer("Layer", wkbUnknown);
		if (layer == nullptr)
			throw FileFormatException(tr("Failed to create layer: %2").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
		
		for (auto symbol : symbols)
		{
			auto match_symbol = [symbol](auto object) { return object->getSymbol() == symbol; };
			switch (symbol->getType())
			{
			case Symbol::Point:
				addPointsToLayer(layer, match_symbol);
				break;
			case Symbol::Text:
				addTextToLayer(layer, match_symbol);
				break;
			case Symbol::Line:
				addLinesToLayer(layer, match_symbol);
				break;
			case Symbol::Combined:
				if (!symbol->getContainedTypes() & Symbol::Area)
				{
					addLinesToLayer(layer, match_symbol);
					break;
				}
				// fall through
			case Symbol::Area:
				addAreasToLayer(layer, match_symbol);
				break;
			case Symbol::NoSymbol:
			case Symbol::AllSymbols:
				Q_UNREACHABLE();
			}
		}
	}
	else if (option(QString::fromLatin1("Per Symbol Layers")).toBool())
	{
		// Add points, lines, areas in this order for driver compatability (esp GPX)
		for (auto symbol : symbols)
		{
			if (symbol->getType() == Symbol::Point)
			{
				auto layer = createLayer(QString::fromUtf8("%1_%2").arg(info.baseName(), symbol->getPlainTextName()).toUtf8(), wkbPoint);
				if (layer != nullptr)
					addPointsToLayer(layer, [&symbol](const Object* object) {
						const auto* sym = object->getSymbol();
						return sym == symbol;
					});
			}
			else if (symbol->getType() == Symbol::Text)
			{
				auto layer = createLayer(QString::fromUtf8("%1_%2").arg(info.baseName(), symbol->getPlainTextName()).toUtf8(), wkbPoint);
				if (layer != nullptr)
					addTextToLayer(layer, [&symbol](const Object* object) {
						const auto* sym = object->getSymbol();
						return sym == symbol;
					});
			}
		}

		// Line symbols
		for (auto symbol : symbols)
		{
			if (symbol->getType() == Symbol::Line
			    || (symbol->getType() == Symbol::Combined && !(symbol->getContainedTypes() & Symbol::Area)))
			{
				auto layer = createLayer(QString::fromUtf8("%1_%2").arg(info.baseName(), symbol->getPlainTextName()).toUtf8(), wkbLineString);
				if (layer != nullptr)
					addLinesToLayer(layer, [&symbol](const Object* object) {
						const auto* sym = object->getSymbol();
						return sym == symbol;
					});
			}
		}

		// Area symbols
		for (auto symbol : symbols)
		{
			if (symbol->getContainedTypes() & Symbol::Area)
			{
				auto layer = createLayer(QString::fromUtf8("%1_%2").arg(info.baseName(), symbol->getPlainTextName()).toUtf8(), wkbPolygon);
				if (layer != nullptr)
					addAreasToLayer(layer, [&symbol](const Object* object) {
						const auto* sym = object->getSymbol();
						return sym == symbol;
					});
			}
		}
	}
	else
	{
		// Add points, lines, areas in this order for driver compatability (esp GPX)
		auto point_layer = createLayer(QString::fromLatin1("%1_points").arg(info.baseName()).toLatin1(), wkbPoint);
		if (point_layer != nullptr)
		{
			addPointsToLayer(point_layer, is_point_object);
			addTextToLayer(point_layer, is_text_object);
		}

		auto line_layer = createLayer(QString::fromLatin1("%1_lines").arg(info.baseName()).toLatin1(), wkbLineString);
		if (line_layer != nullptr)
			addLinesToLayer(line_layer, is_line_object);

		auto area_layer = createLayer(QString::fromLatin1("%1_areas").arg(info.baseName()).toLatin1(), wkbPolygon);
		if (area_layer != nullptr)
			addAreasToLayer(area_layer, is_area_object);
	}

	return true;
}


std::vector<const Symbol*> OgrFileExport::symbolsForExport() const
{
	std::vector<bool> symbols_in_use;
	map->determineSymbolsInUse(symbols_in_use);
	
	const auto num_symbols = map->getNumSymbols();
	std::vector<const Symbol*> symbols;
	symbols.reserve(std::size_t(num_symbols));
	for (auto i = 0; i < num_symbols; ++i)
	{
		auto symbol = map->getSymbol(i);
		if (symbols_in_use[std::size_t(i)]
		    && !symbol->isHidden()
		    && !symbol->isHelperSymbol())
			symbols.push_back(symbol);
	}
	std::sort(begin(symbols), end(symbols), Symbol::lessByColorPriority);
	return symbols;
}


void OgrFileExport::setupGeoreferencing(GDALDriverH po_driver)
{
	// Check if map is georeferenced
	const auto& georef = map->getGeoreferencing();
	bool local_only = false;
	if (georef.getState() == Georeferencing::Local)
	{
		local_only = true;
		addWarning(tr("The map is not georeferenced. Local georeferencing only."));
	}

	// Make sure GDAL can work with the georeferencing info
	map_srs = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
	if (!local_only)
	{
		OSRSetProjCS(map_srs.get(), "Projected map SRS");
		OSRSetWellKnownGeogCS(map_srs.get(), "WGS84");
		auto spec = QByteArray(georef.getProjectedCRSSpec().toLatin1() + " +wktext");
		if (OSRImportFromProj4(map_srs.get(), spec) != OGRERR_NONE)
		{
			local_only = true;
			addWarning(tr("Failed to properly export the georeferencing info. Local georeferencing only."));
		}
	}

	/**
	 * Only certain drivers work without georeferencing info.
	 * GeorefOptional based on http://www.gdal.org/ogr_formats.html as of March 5, 2018
	 */
	if (local_only && !quirks.testFlag(GeorefOptional))
	{
		throw FileFormatException(tr("The %1 driver requires valid georefencing info.")
		                          .arg(QString::fromLatin1(GDALGetDriverShortName(po_driver))));
	}

	if (quirks & NeedsWgs84)
	{
		// Formats with NeedsWgs84 quirk need coords in EPSG:4326/WGS 1984
		auto geo_srs = ogr::unique_srs { OSRNewSpatialReference(nullptr) };
		OSRSetWellKnownGeogCS(geo_srs.get(), "WGS84");
		transformation = ogr::unique_transformation { OCTNewCoordinateTransformation(map_srs.get(), geo_srs.get()) };
	}
}

void OgrFileExport::setupQuirks(GDALDriverH po_driver)
{
	static struct {
		const char* name;
		OgrQuirks quirks;
	} driver_quirks[] = {
	    { "ARCGEN",        GeorefOptional },
	    { "BNA",           GeorefOptional },
	    { "CSV",           GeorefOptional },
	    { "DGN",           GeorefOptional },
	    { "DGNv8",         GeorefOptional },
	    { "DWG",           GeorefOptional },
	    { "DXF",           OgrQuirks() | GeorefOptional | SingleLayer | UseLayerField },
	    { "Geomedia",      GeorefOptional },
	    { "GPX",           NeedsWgs84 },
	    { "INGRES",        GeorefOptional },
	    { "LIBKML",        NeedsWgs84 },
	    { "ODS",           GeorefOptional },
	    { "OpenJUMP .jml", GeorefOptional },
	    { "REC",           GeorefOptional },
	    { "SEGY",          GeorefOptional },
	    { "XLS",           GeorefOptional },
	    { "XLSX",          GeorefOptional },
	};
	using std::begin;
	using std::end;
	auto driver_name = GDALGetDriverShortName(po_driver);
	auto driver_info = std::find_if(begin(driver_quirks), end(driver_quirks), [driver_name](auto entry) {
		return qstrcmp(driver_name, entry.name) == 0;
	});
	if (driver_info != end(driver_quirks))
		quirks |= driver_info->quirks;
}

void OgrFileExport::addPointsToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition)
{
	const auto& georef = map->getGeoreferencing();

	auto add_feature = [&](const Object* object) {
		auto symbol = object->getSymbol();
		auto po_feature = ogr::unique_feature(OGR_F_Create(OGR_L_GetLayerDefn(layer)));

		QString sym_name = symbol->getPlainTextName();
		sym_name.truncate(32);
		OGR_F_SetFieldString(po_feature.get(), OGR_F_GetFieldIndex(po_feature.get(), symbol_field), sym_name.toLatin1().constData());

		auto pt = ogr::unique_geometry(OGR_G_CreateGeometry(wkbPoint));
		QPointF proj_cord = georef.toProjectedCoords(object->asPoint()->getCoordF());

		OGR_G_SetPoint_2D(pt.get(), 0, proj_cord.x(), proj_cord.y());

		if (quirks & NeedsWgs84)
			OGR_G_Transform(pt.get(), transformation.get());

		OGR_F_SetGeometry(po_feature.get(), pt.get());

		OGR_F_SetStyleString(po_feature.get(), OGR_STBL_Find(table.get(), symbolId(symbol)));

		if (OGR_L_CreateFeature(layer, po_feature.get()) != OGRERR_NONE)
			throw FileFormatException(tr("Failed to create feature in layer: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
	};

	map->applyOnMatchingObjects(add_feature, condition);
}

void OgrFileExport::addTextToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition)
{
	const auto& georef = map->getGeoreferencing();

	auto add_feature = [&](const Object* object) {
		auto symbol = object->getSymbol();
		auto po_feature = ogr::unique_feature(OGR_F_Create(OGR_L_GetLayerDefn(layer)));

		QString sym_name = symbol->getPlainTextName();
		sym_name.truncate(32);
		OGR_F_SetFieldString(po_feature.get(), OGR_F_GetFieldIndex(po_feature.get(), symbol_field), sym_name.toLatin1().constData());

		auto text = object->asText()->getText();
		if (o_name_field)
		{
			// Use the name field for the text (useful e.g. for KML).
			// This may overwrite the symbol name, and
			// it may be too short for the full text.
			auto index = OGR_F_GetFieldIndex(po_feature.get(), OGR_Fld_GetNameRef(o_name_field.get()));
			OGR_F_SetFieldString(po_feature.get(), index, text.leftRef(32).toUtf8().constData());
		}

		auto pt = ogr::unique_geometry(OGR_G_CreateGeometry(wkbPoint));
		QPointF proj_cord = georef.toProjectedCoords(object->asText()->getAnchorCoordF());

		OGR_G_SetPoint_2D(pt.get(), 0, proj_cord.x(), proj_cord.y());

		if (quirks & NeedsWgs84)
			OGR_G_Transform(pt.get(), transformation.get());

		OGR_F_SetGeometry(po_feature.get(), pt.get());

		QByteArray style = OGR_STBL_Find(table.get(), symbolId(symbol));
		if (!o_name_field || text.length() > 32)
		{
			// There is no label field, or the text is too long.
			// Put the text directly in the label.
			text.replace(QRegularExpression(QLatin1String("([\"\\\\])"), QRegularExpression::MultilineOption), QLatin1String("\\\\1"));
			style.replace("{Name}", text.toUtf8());
		}
		OGR_F_SetStyleString(po_feature.get(), style);

		if (OGR_L_CreateFeature(layer, po_feature.get()) != OGRERR_NONE)
			throw FileFormatException(tr("Failed to create feature in layer: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
	};

	map->applyOnMatchingObjects(add_feature, condition);
}

void OgrFileExport::addLinesToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition)
{
	const auto& georef = map->getGeoreferencing();

	auto add_feature = [&](const Object* object) {
		const auto* symbol = object->getSymbol();
		const auto* path = object->asPath();
		if (path->parts().size() < 1)
			return;

		QString sym_name = symbol->getPlainTextName();
		sym_name.truncate(32);

		auto line_string = ogr::unique_geometry(OGR_G_CreateGeometry(wkbLineString));
		const auto& parts = path->parts();
		for (const auto& part : parts)
		{
			auto po_feature = ogr::unique_feature(OGR_F_Create(OGR_L_GetLayerDefn(layer)));
			OGR_F_SetFieldString(po_feature.get(), OGR_F_GetFieldIndex(po_feature.get(), symbol_field), sym_name.toLatin1().constData());

			for (const auto& coord : part.path_coords)
			{
				QPointF proj_cord = georef.toProjectedCoords(coord.pos);
				OGR_G_AddPoint_2D(line_string.get(), proj_cord.x(), proj_cord.y());
			}

			if (quirks & NeedsWgs84)
				OGR_G_Transform(line_string.get(), transformation.get());

			OGR_F_SetGeometry(po_feature.get(), line_string.get());

			OGR_F_SetStyleString(po_feature.get(), OGR_STBL_Find(table.get(), symbolId(symbol)));

			if (OGR_L_CreateFeature(layer, po_feature.get()) != OGRERR_NONE)
				throw FileFormatException(tr("Failed to create feature in layer: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
		}
	};

	map->applyOnMatchingObjects(add_feature, condition);
}

void OgrFileExport::addAreasToLayer(OGRLayerH layer, const std::function<bool (const Object*)>& condition)
{
	const auto& georef = map->getGeoreferencing();

	auto add_feature = [&](const Object* object) {
		const auto* symbol = object->getSymbol();
		const auto* path = object->asPath();
		if (path->parts().size() < 1)
			return;

		auto po_feature = ogr::unique_feature(OGR_F_Create(OGR_L_GetLayerDefn(layer)));

		QString sym_name = symbol->getPlainTextName();
		sym_name.truncate(32);
		OGR_F_SetFieldString(po_feature.get(), OGR_F_GetFieldIndex(po_feature.get(), symbol_field), sym_name.toLatin1().constData());

		auto polygon = ogr::unique_geometry(OGR_G_CreateGeometry(wkbPolygon));
		auto cur_ring = ogr::unique_geometry(OGR_G_CreateGeometry(wkbLinearRing));

		const auto& parts = path->parts();
		for (const auto& part : parts)
		{
			for (const auto& coord : part.path_coords)
			{
				QPointF proj_cord = georef.toProjectedCoords(coord.pos);
				OGR_G_AddPoint_2D(cur_ring.get(), proj_cord.x(), proj_cord.y());
			}
			OGR_G_CloseRings(cur_ring.get());
			if (quirks & NeedsWgs84)
				OGR_G_Transform(cur_ring.get(), transformation.get());
			OGR_G_AddGeometry(polygon.get(), cur_ring.get());
			cur_ring.reset(OGR_G_CreateGeometry(wkbLinearRing));
		}

		OGR_F_SetGeometry(po_feature.get(), polygon.get());

		OGR_F_SetStyleString(po_feature.get(), OGR_STBL_Find(table.get(), symbolId(symbol)));

		if (OGR_L_CreateFeature(layer, po_feature.get()) != OGRERR_NONE)
			throw FileFormatException(tr("Failed to create feature in layer: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
	};

	map->applyOnMatchingObjects(add_feature, condition);
}

OGRLayerH OgrFileExport::createLayer(const char* layer_name, OGRwkbGeometryType type)
{
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
	auto po_layer = GDALDatasetCreateLayer(po_ds.get(), layer_name, map_srs.get(), type, nullptr);
#else
	auto po_layer = OGR_DS_CreateLayer(po_ds.get(), layer_name, map_srs.get(), type, nullptr);
#endif
	if (!po_layer) {
		addWarning(tr("Failed to create layer %1: %2").arg(QString::fromUtf8(layer_name), QString::fromLatin1(CPLGetLastErrorMsg())));
		return nullptr;
	}

	if (!quirks.testFlag(UseLayerField)
	    && OGR_L_CreateField(po_layer, o_name_field.get(), 1) != OGRERR_NONE)
	{
		addWarning(tr("Failed to create name field: %1").arg(QString::fromLatin1(CPLGetLastErrorMsg())));
	}

	return po_layer;
}

void OgrFileExport::populateStyleTable(const std::vector<const Symbol*>& symbols)
{
	table = ogr::unique_styletable(OGR_STBL_Create());
	auto manager = ogr::unique_stylemanager(OGR_SM_Create(table.get()));

	// Go through all used symbols and create a style table
	for (auto symbol : symbols)
	{
		QByteArray style_string;
		switch (symbol->getType())
		{
		case Symbol::Text:
			style_string = makeStyleString(symbol->asText());
			break;
		case Symbol::Point:
			style_string = makeStyleString(symbol->asPoint());
			break;
		case Symbol::Line:
			style_string = makeStyleString(symbol->asLine());
			break;
		case Symbol::Area:
			style_string = makeStyleString(symbol->asArea());
			break;
		case Symbol::Combined:
			style_string = makeStyleString(symbol->asCombined());
			break;
		case Symbol::NoSymbol:
		case Symbol::AllSymbols:
			Q_UNREACHABLE();
		}
		
#ifdef MAPPER_DEVELOPMENT_BUILD
		if (qEnvironmentVariableIsSet("MAPPER_DEBUG_OGR"))
			qDebug("%s:\t \"%s\"", qPrintable(symbol->getPlainTextName()), style_string.constData());
#endif
		OGR_SM_AddStyle(manager.get(), symbolId(symbol), style_string);
	}
}

}  // namespace OpenOrienteering
