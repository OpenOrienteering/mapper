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
#include "map_grid.h"
#include "symbol.h"
#include "template.h"
#include "util.h"

const int NativeFileFormat::least_supported_file_format_version = 0;
const int NativeFileFormat::current_file_format_version = 28;
const char NativeFileFormat::magic_bytes[4] = {0x4F, 0x4D, 0x41, 0x50};	// "OMAP"

bool NativeFileFormat::understands(const unsigned char *buffer, size_t sz) const
{
    // The first four bytes of the file must be 'OMAP'.
    if (sz >= 4 && memcmp(buffer, magic_bytes, 4) == 0) return true;
    return false;
}

Importer *NativeFileFormat::createImporter(QIODevice* stream, Map* map, MapView* view) const throw (FormatException)
{
	return new NativeFileImport(stream, map, view);
}

Exporter *NativeFileFormat::createExporter(QIODevice* stream, Map* map, MapView* view) const throw (FormatException)
{
    return new NativeFileExport(stream, map, view);
}



NativeFileImport::NativeFileImport(QIODevice* stream, Map *map, MapView *view) : Importer(stream, map, view)
{
}

NativeFileImport::~NativeFileImport()
{
}

void NativeFileImport::import(bool load_symbols_only) throw (FormatException)
{
    char buffer[4];
    stream->read(buffer, 4); // read the magic

    int version;
    stream->read((char*)&version, sizeof(int));
    if (version < 0)
    {
        addWarning(QObject::tr("Invalid file format version."));
    }
    else if (version < NativeFileFormat::least_supported_file_format_version)
    {
        throw FormatException(QObject::tr("Unsupported file format version. Please use an older program version to load and update the file."));
    }
    else if (version > NativeFileFormat::current_file_format_version)
    {
        throw FormatException(QObject::tr("File format version too high. Please update to a newer program version to load this file."));
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
			LatLon ref_point(gps_projection_parameters.center_latitude, gps_projection_parameters.center_longitude);
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
		georef.geographic_ref_point = LatLon(y, x); 
		QString geographic_crs_id, geographic_crs_spec;
		loadString(stream, geographic_crs_id);   // reserved for geographic crs id
		loadString(stream, geographic_crs_spec); // reserved for full geographic crs specification
		if (geographic_crs_spec != Georeferencing::geographic_crs_spec)
		{
			addWarning(
			  QObject::tr("The geographic coordinate reference system of the map was \"%1\". This CRS is not supported. Using \"%2\".").
			  arg(geographic_crs_spec).
			  arg(Georeferencing::geographic_crs_spec)
			);
		}
		if (version <= 17)
			georef.initDeclination();
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
        stream->read((char*)&map->print_params_set, sizeof(bool));
        if (map->print_params_set)
        {
            stream->read((char*)&map->print_orientation, sizeof(int));
            stream->read((char*)&map->print_format, sizeof(int));
            stream->read((char*)&map->print_dpi, sizeof(float));
            stream->read((char*)&map->print_show_templates, sizeof(bool));
			if (version >= 24)
				stream->read((char*)&map->print_show_grid, sizeof(bool));
			else
				map->print_show_grid = false;
            stream->read((char*)&map->print_center, sizeof(bool));
            stream->read((char*)&map->print_area_left, sizeof(float));
            stream->read((char*)&map->print_area_top, sizeof(float));
            stream->read((char*)&map->print_area_width, sizeof(float));
            stream->read((char*)&map->print_area_height, sizeof(float));
			if (version >= 26)
			{
				stream->read((char*)&map->print_different_scale_enabled, sizeof(bool));
				stream->read((char*)&map->print_different_scale, sizeof(int));
			}
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
        MapColor* color = new MapColor();

        stream->read((char*)&color->priority, sizeof(int));
        stream->read((char*)&color->c, sizeof(float));
        stream->read((char*)&color->m, sizeof(float));
        stream->read((char*)&color->y, sizeof(float));
        stream->read((char*)&color->k, sizeof(float));
        stream->read((char*)&color->opacity, sizeof(float));
        color->updateFromCMYK();

        loadString(stream, color->name);

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
            throw FormatException(QObject::tr("Error while loading a symbol with type %2.").arg(symbol_type));
        }

        if (!symbol->load(stream, version, map))
        {
            throw FormatException(QObject::tr("Error while loading a symbol."));
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
				throw FormatException(QObject::tr("Error while loading undo steps."));
			}
		}

		// Load layers
		stream->read((char*)&map->current_layer_index, sizeof(int));

		int num_layers;
		if (stream->read((char*)&num_layers, sizeof(int)) < (int)sizeof(int))
		{
			throw FormatException(QObject::tr("Error while reading layer count."));
		}
		delete map->layers[0];
		map->layers.resize(num_layers);

		for (int i = 0; i < num_layers; ++i)
		{
			MapLayer* layer = new MapLayer("", map);
			if (!layer->load(stream, version, map))
			{
				throw FormatException(QObject::tr("Error while loading layer %2.").arg(i+1));
			}
			map->layers[i] = layer;
		}
	}
}





NativeFileExport::NativeFileExport(QIODevice* stream, Map *map, MapView *view) : Exporter(stream, map, view)
{
}

NativeFileExport::~NativeFileExport()
{
}

void NativeFileExport::doExport() throw (FormatException)
{
    // Basic stuff
    stream->write(NativeFileFormat::magic_bytes, 4);
    stream->write((const char*)&NativeFileFormat::current_file_format_version, sizeof(int));

	saveString(stream, map->map_notes);
	
	const Georeferencing& georef = map->getGeoreferencing();
	stream->write((const char*)&georef.scale_denominator, sizeof(int));
	stream->write((const char*)&georef.declination, sizeof(double));
	stream->write((const char*)&georef.grivation, sizeof(double));
	double x,y;
	x = georef.map_ref_point.xd(); 
	y = georef.map_ref_point.yd();
	stream->write((const char*)&x, sizeof(double));
	stream->write((const char*)&y, sizeof(double));
	x = georef.projected_ref_point.x();
	y = georef.projected_ref_point.y();
	stream->write((const char*)&x, sizeof(double));
	stream->write((const char*)&y, sizeof(double));
	saveString(stream, georef.projected_crs_id);
	saveString(stream, georef.projected_crs_spec);
	y = georef.geographic_ref_point.latitude;
	x = georef.geographic_ref_point.longitude;
	stream->write((const char*)&y, sizeof(double));
	stream->write((const char*)&x, sizeof(double));
	saveString(stream, QString("Geographic coordinates")); // reserved for geographic crs parameter or specification id
	saveString(stream, georef.geographic_crs_spec);
	
	map->getGrid().save(stream);

	stream->write((const char*)&map->area_hatching_enabled, sizeof(bool));
	stream->write((const char*)&map->baseline_view_enabled, sizeof(bool));

	stream->write((const char*)&map->print_params_set, sizeof(bool));
	if (map->print_params_set)
	{
		stream->write((const char*)&map->print_orientation, sizeof(int));
		stream->write((const char*)&map->print_format, sizeof(int));
		stream->write((const char*)&map->print_dpi, sizeof(float));
		stream->write((const char*)&map->print_show_templates, sizeof(bool));
		stream->write((const char*)&map->print_show_grid, sizeof(bool));
		stream->write((const char*)&map->print_center, sizeof(bool));
		stream->write((const char*)&map->print_area_left, sizeof(float));
		stream->write((const char*)&map->print_area_top, sizeof(float));
		stream->write((const char*)&map->print_area_width, sizeof(float));
		stream->write((const char*)&map->print_area_height, sizeof(float));
		stream->write((const char*)&map->print_different_scale_enabled, sizeof(bool));
		stream->write((const char*)&map->print_different_scale, sizeof(int));
	}

	stream->write((const char*)&map->image_template_use_meters_per_pixel, sizeof(bool));
	stream->write((const char*)&map->image_template_meters_per_pixel, sizeof(double));
	stream->write((const char*)&map->image_template_dpi, sizeof(double));
	stream->write((const char*)&map->image_template_scale, sizeof(double));

	// Write colors
	int num_colors = (int)map->color_set->colors.size();
	stream->write((const char*)&num_colors, sizeof(int));

	for (int i = 0; i < num_colors; ++i)
	{
		MapColor* color = map->color_set->colors[i];

		stream->write((const char*)&color->priority, sizeof(int));
		stream->write((const char*)&color->c, sizeof(float));
		stream->write((const char*)&color->m, sizeof(float));
		stream->write((const char*)&color->y, sizeof(float));
		stream->write((const char*)&color->k, sizeof(float));
		stream->write((const char*)&color->opacity, sizeof(float));

		saveString(stream, color->name);
	}

    // Write symbols
    int num_symbols = map->getNumSymbols();
    stream->write((const char*)&num_symbols, sizeof(int));

    for (int i = 0; i < num_symbols; ++i)
    {
        Symbol* symbol = map->getSymbol(i);

        int type = static_cast<int>(symbol->getType());
        stream->write((const char*)&type, sizeof(int));
        symbol->save(stream, map);
    }

    // Write templates
    stream->write((const char*)&map->first_front_template, sizeof(int));

    int num_templates = map->getNumTemplates();
    stream->write((const char*)&num_templates, sizeof(int));

    for (int i = 0; i < num_templates; ++i)
    {
		Template* temp = map->getTemplate(i);

		saveString(stream, temp->getTemplatePath());
		saveString(stream, temp->getTemplateRelativePath());

		temp->saveTemplateConfiguration(stream);	// save transformation etc.
		if (temp->hasUnsavedChanges())
		{
			// Save the template itself (e.g. image, gpx file, etc.)
			temp->saveTemplateFile();
			temp->setHasUnsavedChanges(false);
		}
	}

	// Write closed template settings
	int num_closed_templates = map->getNumClosedTemplates();
	stream->write((const char*)&num_closed_templates, sizeof(int));
	
	for (int i = 0; i < num_closed_templates; ++i)
	{
		Template* temp = map->getClosedTemplate(i);
		
		saveString(stream, temp->getTemplatePath());
		saveString(stream, temp->getTemplateRelativePath());
		
		temp->saveTemplateConfiguration(stream);	// save transformation etc.
	}

	// Write widgets and views; replaces MapEditorController::saveWidgetsAndViews()
    if (view)
    {
        // which only does this anyway
        view->save(stream);
    }
    else
    {
        // TODO
    }

    // Write undo steps
    map->object_undo_manager.save(stream);

    // Write layers
    stream->write((const char*)&map->current_layer_index, sizeof(int));

    int num_layers = map->getNumLayers();
    stream->write((const char*)&num_layers, sizeof(int));

    for (int i = 0; i < num_layers; ++i)
    {
        MapLayer* layer = map->getLayer(i);
        layer->save(stream, map);
    }
}
