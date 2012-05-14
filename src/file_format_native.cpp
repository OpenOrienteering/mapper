/*
 *    Copyright 2012 Thomas Schöps, Pete Curtis
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

#include <QFile>

#include "file_format_native.h"
#include "georeferencing.h"
#include "gps_coordinates.h"
#include "map_color.h"
#include "symbol.h"
#include "template.h"
#include "util.h"

const int NativeFileFormat::least_supported_file_format_version = 0;
const int NativeFileFormat::current_file_format_version = 17;
const char NativeFileFormat::magic_bytes[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"

bool NativeFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
    // The first four bytes of the file must be 'OMAP'.
    if (sz >= 4 && memcmp(buffer, magic_bytes, 4) == 0) return true;
    return false;
}

Importer *NativeFileFormat::createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
	return new NativeFileImport(path, map, view);
}

Exporter *NativeFileFormat::createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    return new NativeFileExport(path, map, view);
}



NativeFileImport::NativeFileImport(const QString &path, Map *map, MapView *view) : Importer(path, map, view)
{
}

NativeFileImport::~NativeFileImport()
{
}

void NativeFileImport::doImport(bool load_symbols_only) throw (FormatException)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        throw FormatException(QObject::tr("Cannot open file:\n%1\nfor reading.").arg(path));
    }
    // note: ~QFile() will close the file when this method returns and the variable goes out of scope.

    char buffer[4];
    file.read(buffer, 4); // read the magic
    // TODO: should we assert? this shouldn't be called except from loadTo(), where it's already been checked...

    int version;
    file.read((char*)&version, sizeof(int));
    if (version < 0)
    {
        addWarning(QObject::tr("Problem while opening file:\n%1\n\nInvalid file format version.").arg(file.fileName()));
    }
    else if (version < NativeFileFormat::least_supported_file_format_version)
    {
        throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nUnsupported file format version. Please use an older program version to load and update the file.").arg(file.fileName()));
    }
    else if (version > NativeFileFormat::current_file_format_version)
    {
        throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nFile format version too high. Please update to a newer program version to load this file.").arg(file.fileName()));
    }

    if (version <= 16)
	{
		Georeferencing georef;
		file.read((char*)&georef.scale_denominator, sizeof(int));
		
		if (version >= 15)
			loadString(&file, map->map_notes);
		
		bool gps_projection_params_set; // obsolete
		file.read((char*)&gps_projection_params_set, sizeof(bool));
		GPSProjectionParameters gps_projection_parameters; // obsolete
		file.read((char*)&gps_projection_parameters, sizeof(GPSProjectionParameters));
		if (gps_projection_params_set)
		{
			LatLon ref_point(gps_projection_parameters.center_latitude, gps_projection_parameters.center_longitude);
			georef.setGeographicRefPoint(ref_point);
		}
		*map->georeferencing = georef;
	}
	else if (version >= 17)
	{
		if (version >= 15)
			loadString(&file, map->map_notes);
		
		Georeferencing georef;
		file.read((char*)&georef.scale_denominator, sizeof(int));
		file.read((char*)&georef.grivation, sizeof(double));
		double x,y;
		file.read((char*)&x, sizeof(double));
		file.read((char*)&y, sizeof(double));
		georef.setMapRefPoint(MapCoord(x,y));
		file.read((char*)&x, sizeof(double));
		file.read((char*)&y, sizeof(double));
		georef.setProjectedRefPoint(QPointF(x,y));
		loadString(&file, georef.projected_crs_id);
		loadString(&file, georef.projected_crs_spec);
		file.read((char*)&y, sizeof(double));
		file.read((char*)&x, sizeof(double));
		georef.setGeographicRefPoint(LatLon(y, x));
		QString geographic_crs_id, geographic_crs_spec;
		loadString(&file, geographic_crs_id);   // reserved for geographic crs id
		loadString(&file, geographic_crs_spec); // reserved for full geographic crs specification
		if (geographic_crs_spec != Georeferencing::geographic_crs_spec)
		{
			addWarning(
			  QObject::tr("The geographic coordinate reference system of the map was \"%1\". This CRS is not supported. Using \"%2\".").
			  arg(geographic_crs_spec).
			  arg(Georeferencing::geographic_crs_spec)
			);
		}
		*map->georeferencing = georef;
	}

    if (version >= 6)
    {
        file.read((char*)&map->print_params_set, sizeof(bool));
        if (map->print_params_set)
        {
            file.read((char*)&map->print_orientation, sizeof(int));
            file.read((char*)&map->print_format, sizeof(int));
            file.read((char*)&map->print_dpi, sizeof(float));
            file.read((char*)&map->print_show_templates, sizeof(bool));
            file.read((char*)&map->print_center, sizeof(bool));
            file.read((char*)&map->print_area_left, sizeof(float));
            file.read((char*)&map->print_area_top, sizeof(float));
            file.read((char*)&map->print_area_width, sizeof(float));
            file.read((char*)&map->print_area_height, sizeof(float));
        }
    }
    
    if (version >= 16)
	{
		file.read((char*)&map->image_template_use_meters_per_pixel, sizeof(bool));
		file.read((char*)&map->image_template_meters_per_pixel, sizeof(double));
		file.read((char*)&map->image_template_dpi, sizeof(double));
		file.read((char*)&map->image_template_scale, sizeof(double));
	}

    // Load colors
    int num_colors;
    file.read((char*)&num_colors, sizeof(int));
    map->color_set->colors.resize(num_colors);

    for (int i = 0; i < num_colors; ++i)
    {
        MapColor* color = new MapColor();

        file.read((char*)&color->priority, sizeof(int));
        file.read((char*)&color->c, sizeof(float));
        file.read((char*)&color->m, sizeof(float));
        file.read((char*)&color->y, sizeof(float));
        file.read((char*)&color->k, sizeof(float));
        file.read((char*)&color->opacity, sizeof(float));
        color->updateFromCMYK();

        loadString(&file, color->name);

        map->color_set->colors[i] = color;
    }

    // Load symbols
    int num_symbols;
    file.read((char*)&num_symbols, sizeof(int));
    map->symbols.resize(num_symbols);

    for (int i = 0; i < num_symbols; ++i)
    {
        int symbol_type;
        file.read((char*)&symbol_type, sizeof(int));

        Symbol* symbol = Symbol::getSymbolForType(static_cast<Symbol::Type>(symbol_type));
        if (!symbol)
        {
            throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nError while loading a symbol with type %2.")
                                  .arg(file.fileName()).arg(symbol_type));
        }

        if (!symbol->load(&file, version, map))
        {
            throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nError while loading a symbol.").arg(file.fileName()));
        }
        map->symbols[i] = symbol;
    }

    if (!load_symbols_only)
	{
		// Load templates
		file.read((char*)&map->first_front_template, sizeof(int));

		int num_templates;
		file.read((char*)&num_templates, sizeof(int));
		map->templates.resize(num_templates);

		for (int i = 0; i < num_templates; ++i)
		{
			QString path;
			loadString(&file, path);

			Template* temp = Template::templateForFile(path, map);
			temp->loadTemplateParameters(&file);

			map->templates[i] = temp;
		}

		// Restore widgets and views
		view->load(&file);

		// Load undo steps
		if (version >= 7)
		{
			if (!map->object_undo_manager.load(&file, version))
			{
				throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nError while loading undo steps.").arg(file.fileName()));
			}
		}

		// Load layers
		file.read((char*)&map->current_layer_index, sizeof(int));

		int num_layers;
		if (file.read((char*)&num_layers, sizeof(int)) < (int)sizeof(int))
		{
			throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nError while reading layer count.").arg(file.fileName()));
		}
		delete map->layers[0];
		map->layers.resize(num_layers);

		for (int i = 0; i < num_layers; ++i)
		{
			MapLayer* layer = new MapLayer("", map);
			if (!layer->load(&file, version, map))
			{
				throw FormatException(QObject::tr("Problem while opening file:\n%1\n\nError while loading layer %2.").arg(file.fileName()).arg(i+1));
			}
			map->layers[i] = layer;
		}
	}
}





NativeFileExport::NativeFileExport(const QString &path, Map *map, MapView *view) : Exporter(path, map, view)
{
}

NativeFileExport::~NativeFileExport()
{
}

void NativeFileExport::doExport() throw (FormatException)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        throw FormatException(QObject::tr("Cannot open file:\n%1\nfor writing.").arg(path));
    }

    // Basic stuff
    file.write(NativeFileFormat::magic_bytes, 4);
    file.write((const char*)&NativeFileFormat::current_file_format_version, sizeof(int));

	saveString(&file, map->map_notes);
	
	const Georeferencing& georef = map->getGeoreferencing();
	file.write((const char*)&georef.scale_denominator, sizeof(int));
	file.write((const char*)&georef.grivation, sizeof(double));
	double x,y;
	x = georef.map_ref_point.xd(); 
	y = georef.map_ref_point.yd();
	file.write((const char*)&x, sizeof(double));
	file.write((const char*)&y, sizeof(double));
	x = georef.projected_ref_point.x();
	y = georef.projected_ref_point.y();
	file.write((const char*)&x, sizeof(double));
	file.write((const char*)&y, sizeof(double));
	saveString(&file, georef.projected_crs_id);
	saveString(&file, georef.projected_crs_spec);
	y = georef.geographic_ref_point.latitude;
	x = georef.geographic_ref_point.longitude;
	file.write((const char*)&y, sizeof(double));
	file.write((const char*)&x, sizeof(double));
	saveString(&file, QString("Geographic coordinates")); // reserved for geographic crs parameter or specification id
	saveString(&file, georef.geographic_crs_spec);
	
    file.write((const char*)&map->print_params_set, sizeof(bool));
    if (map->print_params_set)
    {
        file.write((const char*)&map->print_orientation, sizeof(int));
        file.write((const char*)&map->print_format, sizeof(int));
        file.write((const char*)&map->print_dpi, sizeof(float));
        file.write((const char*)&map->print_show_templates, sizeof(bool));
        file.write((const char*)&map->print_center, sizeof(bool));
        file.write((const char*)&map->print_area_left, sizeof(float));
        file.write((const char*)&map->print_area_top, sizeof(float));
        file.write((const char*)&map->print_area_width, sizeof(float));
        file.write((const char*)&map->print_area_height, sizeof(float));
    }
    
    file.write((const char*)&map->image_template_use_meters_per_pixel, sizeof(bool));
	file.write((const char*)&map->image_template_meters_per_pixel, sizeof(double));
	file.write((const char*)&map->image_template_dpi, sizeof(double));
	file.write((const char*)&map->image_template_scale, sizeof(double));

    // Write colors
    int num_colors = (int)map->color_set->colors.size();
    file.write((const char*)&num_colors, sizeof(int));

    for (int i = 0; i < num_colors; ++i)
    {
        MapColor* color = map->color_set->colors[i];

        file.write((const char*)&color->priority, sizeof(int));
        file.write((const char*)&color->c, sizeof(float));
        file.write((const char*)&color->m, sizeof(float));
        file.write((const char*)&color->y, sizeof(float));
        file.write((const char*)&color->k, sizeof(float));
        file.write((const char*)&color->opacity, sizeof(float));

        saveString(&file, color->name);
    }

    // Write symbols
    int num_symbols = map->getNumSymbols();
    file.write((const char*)&num_symbols, sizeof(int));

    for (int i = 0; i < num_symbols; ++i)
    {
        Symbol* symbol = map->getSymbol(i);

        int type = static_cast<int>(symbol->getType());
        file.write((const char*)&type, sizeof(int));
        symbol->save(&file, map);
    }

    // Write templates
    file.write((const char*)&map->first_front_template, sizeof(int));

    int num_templates = map->getNumTemplates();
    file.write((const char*)&num_templates, sizeof(int));

    for (int i = 0; i < num_templates; ++i)
    {
        Template* temp = map->getTemplate(i);

        saveString(&file, temp->getTemplatePath());

        temp->saveTemplateParameters(&file);	// save transformation etc.
        if (temp->hasUnsavedChanges())
        {
            // Save the template itself (e.g. image, gpx file, etc.)
            temp->saveTemplateFile();
            temp->setHasUnsavedChanges(false);
        }
    }

    // Write widgets and views; replaces MapEditorController::saveWidgetsAndViews()
    if (view)
    {
        // which only does this anyway
        view->save(&file);
    }
    else
    {
        // TODO
    }

    // Write undo steps
    map->object_undo_manager.save(&file);

    // Write layers
    file.write((const char*)&map->current_layer_index, sizeof(int));

    int num_layers = map->getNumLayers();
    file.write((const char*)&num_layers, sizeof(int));

    for (int i = 0; i < num_layers; ++i)
    {
        MapLayer* layer = map->getLayer(i);
        layer->save(&file, map);
    }

    file.close();

}
