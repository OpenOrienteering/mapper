/*
 *    Copyright 2020 Kai Pastor
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

#include "kmz_groundoverlay_export.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

#include <cpl_vsi.h>
#include <cpl_vsi_error.h>
// IWYU pragma: no_include <gdal.h>

#include <Qt>
#include <QtGlobal>
#include <QApplication>
#include <QBuffer>
#include <QEventLoop>
#include <QFileInfo>
#include <QImage>
#include <QIODevice>
#include <QLatin1String>
#include <QLineF>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QProgressDialog>
#include <QTransform>

#include "mapper_config.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "core/map_printer.h"
#include "fileformats/file_format.h"
#include "gdal/gdal_file.h"
#include "util/util.h"

// IWYU pragma: no_forward_declare QRectF
// IWYU pragma: no_forward_declare QXmlStreamWriter


namespace OpenOrienteering {

namespace {

static constexpr const char* format = "jpg";
static constexpr int quality        = 75;


QPointF toLonLat(const LatLon& latlon) noexcept
{
	return QPointF(latlon.longitude(), latlon.latitude());
}

LatLon fromLonLat(const QPointF& p) noexcept
{
	return LatLon(p.y(), p.x());
}


QRectF boundingBox(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3) noexcept
{
	auto result = QRectF{p0.x(), p0.y(), 0, 0};
	rectInclude(result, p1);
	rectInclude(result, p2);
	rectInclude(result, p3);
	return result;
}


QRectF boundingBoxLonLat(const Georeferencing& georef, const MapCoordF& p0, const MapCoordF& p1, const MapCoordF& p2, const MapCoordF& p3)
{
	return boundingBox(
	            toLonLat(georef.toGeographicCoords(p0)),
	            toLonLat(georef.toGeographicCoords(p1)),
	            toLonLat(georef.toGeographicCoords(p2)),
	            toLonLat(georef.toGeographicCoords(p3))
	);
}

QRectF boundingBoxLonLat(const Georeferencing& georef, const QRectF& extent_map_coord)
{
	return boundingBoxLonLat(
	            georef,
	            MapCoordF(extent_map_coord.topLeft()),
	            MapCoordF(extent_map_coord.topRight()),
	            MapCoordF(extent_map_coord.bottomRight()),
	            MapCoordF(extent_map_coord.bottomLeft())
	);
}


QRectF boundingBoxMap(const Georeferencing& georef, const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3)
{
	return boundingBox(
	            georef.toMapCoordF(fromLonLat(p0)),
	            georef.toMapCoordF(fromLonLat(p1)),
	            georef.toMapCoordF(fromLonLat(p2)),
	            georef.toMapCoordF(fromLonLat(p3))
	);
}

QRectF boundingBoxMap(const Georeferencing& georef, const QRectF& extent_lonlat)
{
	return boundingBoxMap(
	            georef,
	            extent_lonlat.topLeft(),
	            extent_lonlat.topRight(),
	            extent_lonlat.bottomRight(),
	            extent_lonlat.bottomLeft()
	);
}

	
}  // namespace



// ### KmzGroudOverlayExport ###

// static
KmzGroundOverlayExport::Metrics KmzGroundOverlayExport::makeMetrics(qreal const resolution_dpi, int const tile_size_px) noexcept
{
	auto const units_per_mm = resolution_dpi / 25.4;
	return Metrics { tile_size_px, resolution_dpi, units_per_mm, tile_size_px / units_per_mm };
}


KmzGroundOverlayExport::~KmzGroundOverlayExport() = default;

KmzGroundOverlayExport::KmzGroundOverlayExport(const QString& path, const Map& map)
: map(map)
, overlap(std::max(2 * (std::nexttoward(180.0, 181.0) - 180.0), 0.000000000000001))
, precision(std::max(std::ceil(log(overlap)/log(0.1) + 0.5), 12.0))
, is_kmz(path.endsWith(QLatin1String(".kmz"), Qt::CaseInsensitive))
{
	auto const fileinfo = QFileInfo(path);
	if (is_kmz)
	{
		basepath_utf8 = "/vsizip/" + fileinfo.absoluteFilePath().toUtf8();
		doc_filepath_utf8 = basepath_utf8 + "/doc.kml";
	}
	else
	{
		basepath_utf8 = fileinfo.absolutePath().toUtf8();
		doc_filepath_utf8 = fileinfo.absoluteFilePath().toUtf8();
	}
}


void KmzGroundOverlayExport::setProgressObserver(QProgressDialog* observer) noexcept
{
	progress_observer = observer;
}

QString KmzGroundOverlayExport::errorString() const
{
	return error_message;
}


bool KmzGroundOverlayExport::doExport(const MapPrinter& map_printer)
{
	error_message.clear();
	
#ifdef QT_PRINTSUPPORT_LIB
	auto const& georef = map.getGeoreferencing();
	if (georef.getState() != Georeferencing::Geospatial)
	{
		error_message = tr("For KML/KMZ export, the map must be georeferenced.");
		return false;
	}
	
	auto const resolution = map_printer.getOptions().resolution * map_printer.getScaleAdjustment();
	if (resolution < 0)
	{
		error_message = tr("Unknown error");
		return false;
	}
	
	auto const metrics = makeMetrics(resolution);
	auto const tiles = makeTiles(map_printer.getPrintArea(), metrics);
	
	if (!is_kmz)
	{
		// When not creating a KMZ container file, do not overwrite existing image files.
		for (auto const& tile : tiles)
		{
			auto const filepath_utf8 = QByteArray(basepath_utf8 + '/' + tile.filepath);
			if (GdalFile::exists(filepath_utf8))
			{
				error_message = tr("%1 already exists.").arg(QString::fromUtf8(filepath_utf8));
				return false;
			}
		}
	}
	
	VSIErrorReset();
	
	// Create the KMZ container file if needed.
	auto* kmz_file = is_kmz ? VSIFOpenL(basepath_utf8, "wb") : nullptr;
	if (is_kmz && !kmz_file)
	{
		error_message = QString::fromUtf8(VSIGetLastErrorMsg());
		return false;
	}
	
	// Create KML document and image files.
	setMaximumProgress(int(tiles.size()));
	auto const result = doExport(map_printer, metrics, tiles);
	if (is_kmz)
	{
		VSIFCloseL(kmz_file);
		if (wasCanceled())
			VSIUnlink(basepath_utf8);
	}
	else if (wasCanceled())
	{
		VSIUnlink(doc_filepath_utf8);
	}
	setProgress(maximumProgress());
	return result;
#else
	Q_UNUSED(map_printer)
	return false;
#endif // QT_PRINTSUPPORT_LIB
}

bool KmzGroundOverlayExport::doExport(const MapPrinter& map_printer, const Metrics& metrics, const std::vector<Tile>& tiles)
try
{
#ifdef QT_PRINTSUPPORT_LIB
	// Reusing the same QByteArray allocation for KML document and for tiles.
	QByteArray byte_array;
	byte_array.reserve(100000);
	
	// Create the KML document.
	writeKml(byte_array, tiles);
	writeToVSI(doc_filepath_utf8, byte_array);
	
	// Create the tile files, reusing the same QImage allocation.
	mkdir(basepath_utf8 + "/files");
	
	QImage image(metrics.tile_size_px, metrics.tile_size_px, QImage::Format_ARGB32_Premultiplied);
	QImage buffer(metrics.tile_size_px, metrics.tile_size_px, QImage::Format_RGB32);
	auto progress = 0;
	for (auto const& tile : tiles)
	{
		image.fill(Qt::white);
		buffer.fill(Qt::white);
		QPainter painter(&image);
		const auto tile_transform = makeTileTransform(tile.rect_map, metrics, map.getGeoreferencing().getDeclination());
		map_printer.drawPage(&painter, tile.rect_map.adjusted(-5, -5, 5, 5), tile_transform, &buffer);
		saveToBuffer(image, byte_array);
		writeToVSI(basepath_utf8 + '/' + tile.filepath, byte_array);
		
		setProgress(++progress);
		if (wasCanceled())
			break;
	}
	return true;
#else
	Q_UNUSED(map_printer)
	Q_UNUSED(metrics)
	Q_UNUSED(tiles)
	return false;
#endif  // QT_PRINTSUPPORT_LIB
}
catch (FileFormatException& e)
{
	error_message = e.message();
	return false;
}


std::vector<KmzGroundOverlayExport::Tile> KmzGroundOverlayExport::makeTiles(const QRectF& extent_map, const Metrics& metrics) const
{
	std::vector<Tile> tiles;
	
	auto const& georef = map.getGeoreferencing();
	// Non-zero declination rotates the original extent box, causing growing bounding boxes.
	auto const bounding_box_lonlat = boundingBoxLonLat(georef, extent_map);
	auto const bounding_box_map = boundingBoxMap(georef, bounding_box_lonlat);
	
	auto eastwards = QLineF::fromPolar(metrics.tile_size_mm, georef.getDeclination()).p2();
	auto northwards = QLineF::fromPolar(metrics.tile_size_mm, georef.getDeclination() + 90).p2();
	
	auto const start_map = georef.toMapCoordF(fromLonLat(bounding_box_lonlat.topLeft()));
	auto tile_lonlat = QRectF(bounding_box_lonlat.topLeft(),
	                          toLonLat(georef.toGeographicCoords(MapCoordF(start_map + eastwards + northwards))))
	                   .normalized();
	auto const delta = QPointF(tile_lonlat.width() - overlap, tile_lonlat.height() - overlap);
	
	auto const last_y = int(std::ceil((bounding_box_lonlat.height() - overlap) / delta.y()));
	for (int y = 0; y < last_y; ++y)
	{
		auto const last_x = int(std::ceil((bounding_box_lonlat.width() - overlap) / delta.x()));
		for (int x = 0; x < last_x; ++x)
		{
			MapCoordF tile_map[] = {
			    georef.toMapCoordF(fromLonLat(tile_lonlat.topLeft())),
			    georef.toMapCoordF(fromLonLat(tile_lonlat.topRight())),
			    georef.toMapCoordF(fromLonLat(tile_lonlat.bottomRight())),
			    georef.toMapCoordF(fromLonLat(tile_lonlat.bottomLeft()))
			};
			using std::begin; using std::end;
			if (std::any_of(begin(tile_map), end(tile_map), [&bounding_box_map](auto& p) { return p.x() >= bounding_box_map.left(); })
			    && std::any_of(begin(tile_map), end(tile_map), [&bounding_box_map](auto& p) { return p.x() <= bounding_box_map.right(); })
			    && std::any_of(begin(tile_map), end(tile_map), [&bounding_box_map](auto& p) { return p.y() >= bounding_box_map.top(); })
			    && std::any_of(begin(tile_map), end(tile_map), [&bounding_box_map](auto& p) { return p.y() <= bounding_box_map.bottom(); }))
			{
				auto const name = QByteArray("tile_" + QByteArray::number(x) + '_' + QByteArray::number(y) + ".jpg");
				auto const filepath = QByteArray("files/" + name);
				tiles.push_back({name, filepath, tile_lonlat, boundingBox(tile_map[0], tile_map[1], tile_map[2], tile_map[3])});
			}
			
			tile_lonlat.moveLeft(tile_lonlat.left() + delta.x());
		}
		tile_lonlat.moveLeft(bounding_box_lonlat.left());
		tile_lonlat.moveTop(tile_lonlat.top() + delta.y());
	}
	return tiles;
}


void KmzGroundOverlayExport::writeKml(QByteArray& buffer, const std::vector<KmzGroundOverlayExport::Tile>& tiles) const
{
	buffer.append(
	  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	  "<!-- Generator: OpenOrienteering Mapper " APP_VERSION " -->\n"
	  "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
	  "<Folder>\n"
	  " <name>Map</name>\n"
	);
	for (auto const& tile : tiles)
	{
		buffer.append(
		  " <GroundOverlay id=\"").append(tile.name).append("\">\n"
		  "  <name>").append(tile.name).append("</name>\n"
		  "  <Icon>\n"
		  "   <href>").append(tile.filepath).append("</href>\n"
		  "  </Icon>\n"
		  "  <LatLonBox>\n"
		  "   <north>").append(QByteArray::number(tile.rect_lonlat.bottom(), 'f', precision)).append("</north>\n"
		  "   <south>").append(QByteArray::number(tile.rect_lonlat.top(), 'f', precision)).append("</south>\n"
		  "   <east>").append(QByteArray::number(tile.rect_lonlat.right(), 'f', precision)).append("</east>\n"
		  "   <west>").append(QByteArray::number(tile.rect_lonlat.left(), 'f', precision)).append("</west>\n"
		  "   <rotation>0</rotation>\n"
		  "  </LatLonBox>\n"
		  " </GroundOverlay>\n"
		);
	}
	buffer.append(
	  "</Folder>\n"
	  "</kml>\n"
	);
}


// static
QTransform KmzGroundOverlayExport::makeTileTransform(const QRectF& tile_map, const KmzGroundOverlayExport::Metrics& metrics, qreal const declination)
{
	auto transform = QTransform::fromScale(metrics.units_per_mm, metrics.units_per_mm);
	transform.translate(metrics.tile_size_mm / 2, metrics.tile_size_mm / 2);
	transform.rotate(declination);
	transform.translate(-tile_map.left() - tile_map.width() / 2, -tile_map.top() - tile_map.height() / 2);
	return transform;
}

// static
void KmzGroundOverlayExport::saveToBuffer(const QImage& image, QByteArray& data)
{
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly | QIODevice::Truncate);
	image.save(&buffer, format, quality);
	buffer.close();
}

// static
void KmzGroundOverlayExport::writeToVSI(const QByteArray& filepath_utf8, const QByteArray& data)
{
	auto* file = VSIFOpenL(filepath_utf8, "wb");
	if (!file)
		throw FileFormatException(QString::fromUtf8(VSIGetLastErrorMsg()));
	VSIFWriteL(data, 1, data.size(), file);
	VSIFCloseL(file);
}

void KmzGroundOverlayExport::mkdir(const QByteArray& path_utf8) const
{
	// We must not read/stat kmz during creation.
	if ((is_kmz || !GdalFile::isDir(path_utf8))
	    && !GdalFile::mkdir(path_utf8))
	{
		throw FileFormatException(QString::fromUtf8(VSIGetLastErrorMsg()));
	}
}


void KmzGroundOverlayExport::setMaximumProgress(int value) const
{
	if (progress_observer)
	{
		progress_observer->setMaximum(value);
	}
}

int KmzGroundOverlayExport::maximumProgress() const
{
	return progress_observer ? progress_observer->maximum() : 100;
}

void KmzGroundOverlayExport::setProgress(int value) const
{
	if (progress_observer)
	{
		progress_observer->setValue(value);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100 /* ms */); // Drawing and Cancel events
	}
}

bool KmzGroundOverlayExport::wasCanceled() const
{
	return progress_observer && progress_observer->wasCanceled();
}


}  // namespace OpenOrienteering
