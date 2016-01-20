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

#ifndef OPENORIENTEERING_OGR_FILE_FORMAT_P_H
#define OPENORIENTEERING_OGR_FILE_FORMAT_P_H

#include <QHash>

// The GDAL/OGR C API is more stable than the C++ API.
#include <ogr_api.h>

#include "../file_import_export.h"

class MapPart;
class Symbol;

/**
 * An Importer for geospatial vector data supported by OGR.
 */
class OgrFileImport : public Importer
{
Q_OBJECT
public:
	OgrFileImport(QIODevice* stream, Map *map, MapView *view);
	
	~OgrFileImport() override;
	
protected:
	void import(bool load_symbols_only) override;
	
	void importStyles(OGRDataSourceH data_source);
	
	void importLayer(OGRLayerH layer);
	
	virtual void importGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry);
	
	void importGeometryCollection(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry);
	
	void importPointGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry);
	
	void importLineStringGeometry(MapPart* map_part, OGRFeatureH feature, OGRGeometryH geometry);
	
	virtual Symbol* getSymbol(const QString& identifier);
	
private:
	QHash<QString, Symbol*> symbol_table;
};



#endif // OPENORIENTEERING_OGR_FILE_FORMAT_P_H
