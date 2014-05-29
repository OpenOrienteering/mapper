/*
 *    Copyright 2012, 2013 Thomas Schöps, Pete Curtis
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

#include "file_format_native.h"

#include <QFile>

#include "core/georeferencing.h"
#include "core/map_color.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "file_import_export.h"
#include "map.h"
#include "map_grid.h"
#include "symbol.h"
#include "template.h"
#include "util.h"


// ### NativeFileImport declaration ###

/** An Importer for the native file format. This class delegates to the load() and loadImpl() methods of the
 *  model objects, but long-term this can be refactored out of the model into this class.
 * 
 *  \deprecated
 */
class NativeFileImport : public Importer
{
public:
	/** Creates a new native file importer.
	 */
	NativeFileImport(QIODevice* stream, Map *map, MapView *view);

	/** Destroys this importer.
	 */
	~NativeFileImport();

protected:
	/** Imports a native file.
	 */
	void import(bool load_symbols_only) throw (FileFormatException);
};


// ### GPSProjectionParameters ###
/** 
 * Legacy parameters for an ellipsoid and an orthographic projection of
 * ellipsoid coordinates to 2D map (template) coordinates.
 */
struct GPSProjectionParameters
{
	double a;					// ellipsoidal semi-major axis
	double b;					// ellipsoidal semi-minor axis
	double e_sq;				// eccentricity of the ellipsoid (squared)
	double center_latitude;		// latitude which gives map coordinate zero
	double center_longitude;	// longitude which gives map coordinate zero
	double sin_center_latitude;	// sine of center_latitude
	double cos_center_latitude;	// cosine of center_latitude
	double v0;					// prime vertical radius of curvature at latitude of origin center_latitude
};



// ### NativeFileFormat ###

const int NativeFileFormat::least_supported_file_format_version = 0;
const int NativeFileFormat::current_file_format_version = 30;
const char NativeFileFormat::magic_bytes[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"

NativeFileFormat::NativeFileFormat()
 : FileFormat(FileFormat::MapFile, "native (deprecated)", ImportExport::tr("OpenOrienteering Mapper").append(" pre-0.5"), "omap", 
              FileFormat::ImportSupported)
{
	// Nothing
}

bool NativeFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
	// The first four bytes of the file must be 'OMAP'.
	return (sz >= 4 && memcmp(buffer, magic_bytes, 4) == 0);
}

Importer *NativeFileFormat::createImporter(QIODevice* stream, Map* map, MapView* view) const throw (FileFormatException)
{
	return new NativeFileImport(stream, map, view);
}



// ### NativeFileImport ###

NativeFileImport::NativeFileImport(QIODevice* stream, Map *map, MapView *view) : Importer(stream, map, view)
{
}

NativeFileImport::~NativeFileImport()
{
}

void NativeFileImport::import(bool load_symbols_only) throw (FileFormatException)
{
    char buffer[4];
    stream->read(buffer, 4); // read the magic

    int version;
    stream->read((char*)&version, sizeof(int));
    if (version < 0)
    {
        addWarning(Importer::tr("Invalid file format version."));
    }
    else if (version < NativeFileFormat::least_supported_file_format_version)
    {
        throw FileFormatException(Importer::tr("Unsupported old file format version. Please use an older program version to load and update the file."));
    }
    else if (version > NativeFileFormat::current_file_format_version)
    {
        throw FileFormatException(Importer::tr("Unsupported new file format version. Some map features will not be loaded or saved by this version of the program. Consider updating."));
    }

    if (version <= 16)
	{
		Georeferencing georef;
		stream->read((char*)&georef.scale_denominator, sizeof(int));
		
		if (version >= 15)
			loadString(stream, map->map_notes);
		
		bool gps_projection_params_set; // obsolete
		stream->read((char*)&gps_projection_params_set, sizeof(bool));
		GPSProjectionParameters gps_projection_parameters; // obsolete
		stream->read((char*)&gps_projection_parameters, sizeof(GPSProjectionParameters));
		if (gps_projection_params_set)
		{
			LatLon ref_point = LatLon::fromRadiant(gps_projection_parameters.center_latitude, gps_projection_parameters.center_longitude);
			georef.setGeographicRefPoint(ref_point);
		}
		*map->georeferencing = georef;
	}
	else if (version >= 17)
	{
		loadString(stream, map->map_notes);
		
		Georeferencing georef;
		stream->read((char*)&georef.scale_denominator, sizeof(int));
		if (version >= 18)
			stream->read((char*)&georef.declination, sizeof(double));
		stream->read((char*)&georef.grivation, sizeof(double));
		double x,y;
		stream->read((char*)&x, sizeof(double));
		stream->read((char*)&y, sizeof(double));
		georef.map_ref_point = MapCoord(x,y);
		stream->read((char*)&x, sizeof(double));
		stream->read((char*)&y, sizeof(double));
		georef.projected_ref_point = QPointF(x,y);
		loadString(stream, georef.projected_crs_id);
		loadString(stream, georef.projected_crs_spec);
		stream->read((char*)&y, sizeof(double));
		stream->read((char*)&x, sizeof(double));
		georef.geographic_ref_point = LatLon::fromRadiant(y, x); 
		QString geographic_crs_id, geographic_crs_spec;
		loadString(stream, geographic_crs_id);   // reserved for geographic crs id
		loadString(stream, geographic_crs_spec); // reserved for full geographic crs specification
		if (geographic_crs_spec != Georeferencing::geographic_crs_spec)
		{
			addWarning(
			  Importer::tr("The geographic coordinate reference system of the map was \"%1\". This CRS is not supported. Using \"%2\".").
			  arg(geographic_crs_spec).
			  arg(Georeferencing::geographic_crs_spec)
			);
		}
		if (version <= 17)
			georef.initDeclination();
		// Correctly set georeferencing state
		georef.setProjectedCRS(georef.projected_crs_id, georef.projected_crs_spec);
		*map->georeferencing = georef;
	}
	
	if (version >= 24)
		map->getGrid().load(stream, version);
	
	if (version >= 25)
	{
		stream->read((char*)&map->area_hatching_enabled, sizeof(bool));
		stream->read((char*)&map->baseline_view_enabled, sizeof(bool));
	}
	else
	{
		map->area_hatching_enabled = false;
		map->baseline_view_enabled = false;
	}
	
	if (version >= 6)
	{
		bool print_params_set;
		stream->read((char*)&print_params_set, sizeof(bool));
		if (print_params_set)
		{
			MapPrinterConfig printer_config(*map);
			stream->read((char*)&printer_config.page_format.orientation, sizeof(int));
			stream->read((char*)&printer_config.page_format.paper_size, sizeof(int));
			
			float resolution;
			stream->read((char*)&resolution, sizeof(float));
			printer_config.options.resolution = qRound(resolution);
			stream->read((char*)&printer_config.options.show_templates, sizeof(bool));
			if (version >= 24)
				stream->read((char*)&printer_config.options.show_grid, sizeof(bool));
			else
				printer_config.options.show_grid = false;
			
			stream->read((char*)&printer_config.center_print_area, sizeof(bool));
			
			float print_area_left, print_area_top, print_area_width, print_area_height;
			stream->read((char*)&print_area_left, sizeof(float));
			stream->read((char*)&print_area_top, sizeof(float));
			stream->read((char*)&print_area_width, sizeof(float));
			stream->read((char*)&print_area_height, sizeof(float));
			printer_config.print_area = QRectF(print_area_left, print_area_top, print_area_width, print_area_height);
			
			if (version >= 26)
			{
				bool print_different_scale_enabled;
				stream->read((char*)&print_different_scale_enabled, sizeof(bool));
				stream->read((char*)&printer_config.options.scale, sizeof(int));
				if (!print_different_scale_enabled)
					printer_config.options.scale = map->getScaleDenominator();
			}
			map->setPrinterConfig(printer_config);
		}
	}
	
    if (version >= 16)
	{
		stream->read((char*)&map->image_template_use_meters_per_pixel, sizeof(bool));
		stream->read((char*)&map->image_template_meters_per_pixel, sizeof(double));
		stream->read((char*)&map->image_template_dpi, sizeof(double));
		stream->read((char*)&map->image_template_scale, sizeof(double));
	}

    // Load colors
    int num_colors;
    stream->read((char*)&num_colors, sizeof(int));
    map->color_set->colors.resize(num_colors);

    for (int i = 0; i < num_colors; ++i)
    {
        int priority;
        stream->read((char*)&priority, sizeof(int));
        MapColor* color = new MapColor(priority);

        MapColorCmyk cmyk;
        stream->read((char*)&cmyk.c, sizeof(float));
        stream->read((char*)&cmyk.m, sizeof(float));
        stream->read((char*)&cmyk.y, sizeof(float));
        stream->read((char*)&cmyk.k, sizeof(float));
        color->setCmyk(cmyk);
        float opacity;
        stream->read((char*)&opacity, sizeof(float));
        color->setOpacity(opacity);

        QString name;
        loadString(stream, name);
        color->setName(name);

        map->color_set->colors[i] = color;
    }

    // Load symbols
    int num_symbols;
    stream->read((char*)&num_symbols, sizeof(int));
    map->symbols.resize(num_symbols);

    for (int i = 0; i < num_symbols; ++i)
    {
        int symbol_type;
        stream->read((char*)&symbol_type, sizeof(int));

        Symbol* symbol = Symbol::getSymbolForType(static_cast<Symbol::Type>(symbol_type));
        if (!symbol)
        {
            throw FileFormatException(Importer::tr("Error while loading a symbol with type %2.").arg(symbol_type));
        }

        if (!symbol->load(stream, version, map))
        {
            throw FileFormatException(Importer::tr("Error while loading a symbol."));
        }
        map->symbols[i] = symbol;
    }

    if (!load_symbols_only)
	{
		// Load templates
		stream->read((char*)&map->first_front_template, sizeof(int));

		int num_templates;
		stream->read((char*)&num_templates, sizeof(int));
		map->templates.resize(num_templates);

		for (int i = 0; i < num_templates; ++i)
		{
			QString path;
			loadString(stream, path);
			Template* temp = Template::templateForFile(path, map);
			if (version >= 27)
			{
				loadString(stream, path);
				temp->setTemplateRelativePath(path);
			}
			
			temp->loadTemplateConfiguration(stream, version);

			map->templates[i] = temp;
		}
		
		if (version >= 28)
		{
			int num_closed_templates;
			stream->read((char*)&num_closed_templates, sizeof(int));
			map->closed_templates.resize(num_closed_templates);
			
			for (int i = 0; i < num_closed_templates; ++i)
			{
				QString path;
				loadString(stream, path);
				Template* temp = Template::templateForFile(path, map);
				loadString(stream, path);
				temp->setTemplateRelativePath(path);
				
				temp->loadTemplateConfiguration(stream, version);
				
				map->closed_templates[i] = temp;
			}
		}

		// Restore widgets and views
		if (view)
			view->load(stream, version);
		else
		{
			// TODO
		}

		// Load undo steps
		if (version >= 7)
		{
			if (!map->object_undo_manager.load(stream, version))
			{
				throw FileFormatException(Importer::tr("Error while loading undo steps."));
			}
		}

		// Load parts
		stream->read((char*)&map->current_part_index, sizeof(int));

		int num_parts;
		if (stream->read((char*)&num_parts, sizeof(int)) < (int)sizeof(int))
		{
			throw FileFormatException(Importer::tr("Error while reading map part count."));
		}
		delete map->parts[0];
		map->parts.resize(num_parts);

		for (int i = 0; i < num_parts; ++i)
		{
			MapPart* part = new MapPart("", map);
			if (!part->load(stream, version, map))
			{
				throw FileFormatException(Importer::tr("Error while loading map part %2.").arg(i+1));
			}
			map->parts[i] = part;
		}
	}
	
	emit map->currentMapPartChanged(map->current_part_index);
}
