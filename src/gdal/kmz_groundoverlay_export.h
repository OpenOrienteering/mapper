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

#ifndef OPENORIENTEERING_KMZ_GROUNDOVERLAY_EXPORT_H
#define OPENORIENTEERING_KMZ_GROUNDOVERLAY_EXPORT_H

#include <vector>

#include <QtGlobal>
#include <QByteArray>
#include <QCoreApplication>
#include <QRectF>
#include <QString>

class QImage;
class QProgressDialog;
class QTransform;

// IWYU pragma: no_forward_declare QRectF

namespace OpenOrienteering {

class Map;
class MapPrinter;


/**
 * A class which generates KML/KMZ files with ground overlay raster tiles.
 * 
 * This class makes use of GDAL's virtual file systems for handling KMZ
 * (a zipped archive of a KML document and a set of image files).
 */
class KmzGroundOverlayExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::KmzGroundOverlayExport)
	
	struct Tile
	{
		QByteArray name;
		QByteArray filepath;
		QRectF  rect_lonlat;
		QRectF  rect_map;
	};
	
	struct Metrics
	{
		int tile_size_px   = 512;
		qreal resolution_dpi = 300;
		qreal units_per_mm = resolution_dpi / 25.4;
		qreal tile_size_mm = tile_size_px / units_per_mm;
	};
	
	static Metrics makeMetrics(qreal resolution_dpi, int tile_size_px = 512) noexcept;
	
public:
	~KmzGroundOverlayExport();
	
	KmzGroundOverlayExport(const QString& path, const Map& map);
	
	void setProgressObserver(QProgressDialog* observer) noexcept;
	
	QString errorString() const;
	
	bool doExport(const MapPrinter& map_printer);
	
	
protected:
	bool doExport(const MapPrinter& map_printer, const Metrics& metrics, const std::vector<Tile>& tiles);
	
	std::vector<Tile> makeTiles(const QRectF& extent_map, const Metrics& metrics) const;
	
	void writeKml(QByteArray& buffer, const std::vector<Tile>& tiles) const;
	
	
	static QTransform makeTileTransform(const QRectF& tile_map, const Metrics& metrics, qreal declination);
	
	static void saveToBuffer(const QImage& image, QByteArray& data);
	
	static void writeToVSI(const QByteArray& filepath, const QByteArray& data);
	
	void mkdir(const QByteArray& path) const;
	
	
	void setMaximumProgress(int value) const;
	
	int maximumProgress() const;
	
	void setProgress(int value) const;
	
	bool wasCanceled() const;
	
private:
	const Map& map;
	QProgressDialog* progress_observer = nullptr;
	QByteArray basepath_utf8;
	QByteArray doc_filepath_utf8;
	QString error_message;
	bool is_kmz = false;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_KMZ_GROUNDOVERLAY_EXPORT_H
