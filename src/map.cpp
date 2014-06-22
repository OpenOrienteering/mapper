/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2013, 2014 Thomas Schöps, Kai Pastor
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


#include "map.h"

#include <cassert>
#include <algorithm>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <qmath.h>
#include <QMessageBox>
#include <QPainter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/georeferencing.h"
#include "core/map_color.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "file_format_ocad8.h"
#include "file_format_registry.h"
#include "file_import_export.h"
#include "map_editor.h"
#include "map_grid.h"
#include "map_part.h"
#include "map_undo.h"
#include "map_widget.h"
#include "object.h"
#include "object_operations.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_combined.h"
#include "symbol_line.h"
#include "symbol_point.h"
#include "template.h"
#include "undo_manager.h"
#include "util.h"

// ### MapColorSet ###

Map::MapColorSet::MapColorSet(QObject *parent) : QObject(parent)
{
	ref_count = 1;
}
void Map::MapColorSet::addReference()
{
	++ref_count;
}
void Map::MapColorSet::dereference()
{
	--ref_count;
	if (ref_count == 0)
	{
		int size = colors.size();
		for (int i = 0; i < size; ++i)
			delete colors[i];
		this->deleteLater();
	}
}

// ### MapColorMergeItem ###

/** A record of information about the mapping of a color in a source MapColorSet
 *  to a color in a destination MapColorSet.
 */
struct MapColorSetMergeItem
{
	MapColor* src_color;
	MapColor* dest_color;
	std::size_t dest_index;
	std::size_t lower_bound;
	std::size_t upper_bound;
	int lower_errors;
	int upper_errors;
	bool filter;
	
	MapColorSetMergeItem()
	 : src_color(NULL),
	   dest_color(NULL),
	   dest_index(0),
	   lower_bound(0),
	   upper_bound(0),
	   lower_errors(0),
	   upper_errors(0),
	   filter(false)
	{ }
};

/** The mapping of all colors in a source MapColorSet
 *  to colors in a destination MapColorSet. */
typedef std::vector<MapColorSetMergeItem> MapColorSetMergeList;

// This algorithm tries to maintain the relative order of colors.
MapColorMap Map::MapColorSet::importSet(const Map::MapColorSet& other, std::vector< bool >* filter, Map* map)
{
	MapColorMap out_pointermap;
	
	// Determine number of colors to import
	std::size_t import_count = other.colors.size();
	if (filter)
	{
		Q_ASSERT(filter->size() == other.colors.size());
		
		for (std::size_t i = 0, end = other.colors.size(); i != end; ++i)
		{
			MapColor* color = other.colors[i];
			if (!(*filter)[i] || out_pointermap.contains(color))
			{
				continue;
			}
			
			out_pointermap[color] = NULL; // temporary used as a flag
			
			// Determine referenced spot colors, and add them to the filter
			if (color->getSpotColorMethod() == MapColor::CustomColor)
			{
				SpotColorComponents components(color->getComponents());
				for (SpotColorComponents::iterator it = components.begin(), end = components.end(); it != end; ++it)
				{
					if (!out_pointermap.contains(it->spot_color))
					{
						// Add this spot color to the filter
						int i = 0;
						while (other.colors[i] != it->spot_color)
							++i;
						(*filter)[i] = true;
						out_pointermap[it->spot_color] = NULL;
					}
				}
			}
		}
		import_count = out_pointermap.size();
		out_pointermap.clear();
	}
	
	if (import_count > 0)
	{
		colors.reserve(colors.size() + import_count);
		
		MapColorSetMergeList merge_list;
		merge_list.resize(other.colors.size());
		
		bool priorities_changed = false;
		
		// Initialize merge_list
		MapColorSetMergeList::iterator merge_list_item = merge_list.begin();
		for (std::size_t i = 0; i < other.colors.size(); ++i)
		{
			merge_list_item->filter = (!filter || (*filter)[i]);
			
			MapColor* src_color = other.colors[i];
			merge_list_item->src_color = src_color;
			for (std::size_t k = 0, colors_size = colors.size(); k < colors_size; ++k)
			{
				if (colors[k]->equals(*src_color, false))
				{
					merge_list_item->dest_color = colors[k];
					merge_list_item->dest_index = k;
					out_pointermap[src_color] = colors[k];
					// Prefer a matching color at the same priority,
					// so just abort early if priority matches
					if (merge_list_item->dest_color->getPriority() == merge_list_item->src_color->getPriority())
						break;
				}
			}
			++merge_list_item;
		}
		Q_ASSERT(merge_list_item == merge_list.end());
		
		size_t iteration_number = 1;
		while (true)
		{
			// Evaluate bounds and conflicting order of colors
			int max_conflict_reduction = 0;
			MapColorSetMergeList::iterator selected_item = merge_list.end();
			for (merge_list_item = merge_list.begin(); merge_list_item != merge_list.end(); ++merge_list_item)
			{
				// Check all lower colors for a higher dest_index
				std::size_t& lower_bound(merge_list_item->lower_bound);
				lower_bound = merge_list_item->dest_color ? merge_list_item->dest_index : 0;
				MapColorSetMergeList::iterator it = merge_list.begin();
				for (; it != merge_list_item; ++it)
				{
					if (it->dest_color)
					{
						if (it->dest_index > lower_bound)
						{
							lower_bound = it->dest_index;
						}
						if (merge_list_item->dest_color && merge_list_item->dest_index < it->dest_index)
						{
							++merge_list_item->lower_errors;
						}
					}
				}
				
				// Check all higher colors for a lower dest_index
				std::size_t& upper_bound(merge_list_item->upper_bound);
				upper_bound = merge_list_item->dest_color ? merge_list_item->dest_index : colors.size();
				for (++it; it != merge_list.end(); ++it)
				{
					if (it->dest_color)
					{
						if (it->dest_index < upper_bound)
						{
							upper_bound = it->dest_index;
						}
						if (merge_list_item->dest_color && merge_list_item->dest_index > it->dest_index)
						{
							++merge_list_item->upper_errors;
						}
					}
				}
				
				if (merge_list_item->filter)
				{
					if (merge_list_item->lower_errors == 0 && merge_list_item->upper_errors > max_conflict_reduction)
					{
						int conflict_reduction = merge_list_item->upper_errors;
						// Check new conflicts with insertion index: selected_item->upper_bound
						for (it = merge_list.begin(); it != merge_list_item; ++it)
						{
							if (it->dest_color && selected_item->upper_bound <= it->dest_index)
								--conflict_reduction;
						}
						// Also allow = here to make two-step resolves work
						if (conflict_reduction >= max_conflict_reduction)
						{
							selected_item = merge_list_item;
							max_conflict_reduction = conflict_reduction;
						}
					}
					else if (merge_list_item->upper_errors == 0 && merge_list_item->lower_errors > max_conflict_reduction)
					{
						int conflict_reduction = merge_list_item->lower_errors;
						// Check new conflicts with insertion index: (selected_item->lower_bound+1)
						it = merge_list_item;
						for (++it; it != merge_list.end(); ++it)
						{
							if (it->dest_color && (selected_item->lower_bound+1) > it->dest_index)
								--conflict_reduction;
						}
						// Also allow = here to make two-step resolves work
						if (conflict_reduction >= max_conflict_reduction)
						{
							selected_item = merge_list_item;
							max_conflict_reduction = conflict_reduction;
						}
					}
				}
			}
			
			// Abort if no conflicts or maximum iteration count reached.
			// The latter condition is just to prevent endless loops in
			// case of bugs and should not occur theoretically.
			if (selected_item == merge_list.end() ||
				iteration_number > merge_list.size())
				break;
			
			// Solve selected conflict item
			MapColor* new_color = new MapColor(*selected_item->dest_color);
			selected_item->dest_color = new_color;
			out_pointermap[selected_item->src_color] = new_color;
			std::size_t insertion_index = (selected_item->lower_errors == 0) ? selected_item->upper_bound : (selected_item->lower_bound+1);
			
			if (map)
				map->addColor(new_color, insertion_index);
			else
				colors.insert(colors.begin() + insertion_index, new_color);
			priorities_changed = true;
			
			for (merge_list_item = merge_list.begin(); merge_list_item != merge_list.end(); ++merge_list_item)
			{
				merge_list_item->lower_errors = 0;
				merge_list_item->upper_errors = 0;
				if (merge_list_item->dest_color && merge_list_item->dest_index >= insertion_index)
					++merge_list_item->dest_index;
			}
			selected_item->dest_index = insertion_index;
			
			++iteration_number;
		}
		
		// Some missing colors may be spot color compositions which can be 
		// resolved to new colors only after all colors have been created.
		// That is why we create all missing colors first.
		for (MapColorSetMergeList::reverse_iterator it = merge_list.rbegin(); it != merge_list.rend(); ++it)
		{
			if (it->filter && !it->dest_color)
			{
				it->dest_color = new MapColor(*it->src_color);
				out_pointermap[it->src_color] = it->dest_color;
			}
			else
			{
				// Existing colors don't need to be touched again.
				it->dest_color = NULL;
			}
		}
		
		// Now process all new colors for spot color resolution and insertion
		for (MapColorSetMergeList::reverse_iterator it = merge_list.rbegin(); it != merge_list.rend(); ++it)
		{
			MapColor* new_color = it->dest_color;
			if (new_color)
			{
				if (new_color->getSpotColorMethod() == MapColor::CustomColor)
				{
					SpotColorComponents components = new_color->getComponents();
					for (SpotColorComponents::iterator it = components.begin(), end = components.end(); it != end; ++it)
					{
						Q_ASSERT(out_pointermap.contains(it->spot_color));
						it->spot_color = const_cast< MapColor* >(out_pointermap[it->spot_color]);
					}
					new_color->setSpotColorComposition(components);
				}
				
				std::size_t insertion_index = it->upper_bound;
				if (map)
					map->addColor(new_color, insertion_index);
				else
					colors.insert(colors.begin() + insertion_index, new_color);
				priorities_changed = true;
			}
		}
		
		if (map && priorities_changed)
		{
			map->updateAllObjects();
		}
	}
	
	return out_pointermap;
}


// ### Map ###

bool Map::static_initialized = false;
MapColor Map::covering_white(MapColor::CoveringWhite);
MapColor Map::covering_red(MapColor::CoveringRed);
MapColor Map::undefined_symbol_color(MapColor::Undefined);
MapColor Map::registration_color(MapColor::Registration);
LineSymbol* Map::covering_white_line;
LineSymbol* Map::covering_red_line;
LineSymbol* Map::undefined_line;
PointSymbol* Map::undefined_point;
CombinedSymbol* Map::covering_combined_line;

Map::Map()
 : has_spot_colors(false),
   undo_manager(new UndoManager(this)),
   renderables(new MapRenderables(this)),
   selection_renderables(new MapRenderables(this)),
   printer_config(NULL)
{
	if (!static_initialized)
		initStatic();
	
	color_set = NULL;
	georeferencing = new Georeferencing();
	grid = new MapGrid();
	area_hatching_enabled = false;
	baseline_view_enabled = false;
	
	clear();
	
	connect(this, SIGNAL(colorAdded(int,MapColor*)), SLOT(checkSpotColorPresence()));
	connect(this, SIGNAL(colorChanged(int,MapColor*)), SLOT(checkSpotColorPresence()));
	connect(this, SIGNAL(colorDeleted(int,const MapColor*)), SLOT(checkSpotColorPresence()));
}
Map::~Map()
{
	int size = symbols.size();
	for (int i = 0; i < size; ++i)
		delete symbols[i];
	
	size = templates.size();
	for (int i = 0; i < size; ++i)
		delete templates[i];
	
	size = closed_templates.size();
	for (int i = 0; i < size; ++i)
		delete closed_templates[i];
	
	size = parts.size();
	for (int i = 0; i < size; ++i)
		delete parts[i];
	
	/*size = views.size();
	for (int i = size; i >= 0; --i)
		delete views[i];*/
	
	color_set->dereference();
	
	delete grid;
	delete georeferencing;
}

void Map::setScaleDenominator(unsigned int value)
{
	georeferencing->setScaleDenominator(value);
}

unsigned int Map::getScaleDenominator() const
{
	return georeferencing->getScaleDenominator();
}

void Map::changeScale(unsigned int new_scale_denominator, const MapCoord& scaling_center, bool scale_symbols, bool scale_objects, bool scale_georeferencing, bool scale_templates)
{
	if (new_scale_denominator == getScaleDenominator())
		return;
	
	double factor = getScaleDenominator() / (double)new_scale_denominator;
	
	if (scale_symbols)
		scaleAllSymbols(factor);
	if (scale_objects)
	{
		undo_manager->clear();
		scaleAllObjects(factor, scaling_center);
	}
	if (scale_georeferencing)
		georeferencing->setMapRefPoint(scaling_center + factor * (georeferencing->getMapRefPoint() - scaling_center));
	if (scale_templates)
	{
		for (int i = 0; i < getNumTemplates(); ++i)
		{
			Template* temp = getTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			setTemplateAreaDirty(i);
			temp->scale(factor, scaling_center);
			setTemplateAreaDirty(i);
		}
		for (int i = 0; i < getNumClosedTemplates(); ++i)
		{
			Template* temp = getClosedTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			temp->scale(factor, scaling_center);
		}
	}
	
	setScaleDenominator(new_scale_denominator);
	setOtherDirty(true);
	updateAllMapWidgets();
}
void Map::rotateMap(double rotation, const MapCoord& center, bool adjust_georeferencing, bool adjust_declination, bool adjust_templates)
{
	if (fmod(rotation, 2 * M_PI) == 0)
		return;
	
	undo_manager->clear();
	rotateAllObjects(rotation, center);
	
	if (adjust_georeferencing)
	{
		MapCoordF reference_point = MapCoordF(georeferencing->getMapRefPoint() - center);
		reference_point.rotate(-rotation);
		georeferencing->setMapRefPoint(center + reference_point.toMapCoord());
	}
	if (adjust_declination)
	{
		double rotation_degrees = 180 * rotation / M_PI;
		georeferencing->setDeclination(georeferencing->getDeclination() + rotation_degrees);
	}
	if (adjust_templates)
	{
		for (int i = 0; i < getNumTemplates(); ++i)
		{
			Template* temp = getTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			setTemplateAreaDirty(i);
			temp->rotate(rotation, center);
			setTemplateAreaDirty(i);
		}
		for (int i = 0; i < getNumClosedTemplates(); ++i)
		{
			Template* temp = getClosedTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			temp->rotate(rotation, center);
		}
	}
	
	setOtherDirty(true);
	updateAllMapWidgets();
}

bool Map::saveTo(const QString& path, MapEditorController* map_editor)
{
	assert(map_editor && "Preserving the widget&view information without retrieving it from a MapEditorController is not implemented yet!");
	
	exportTo(path, map_editor);
	
	colors_dirty = false;
	symbols_dirty = false;
	templates_dirty = false;
	objects_dirty = false;
	other_dirty = false;
	unsaved_changes = false;
	
	undoManager().setClean();
	
	return true;
}

bool Map::exportTo(const QString& path, MapEditorController* map_editor, const FileFormat* format)
{
	assert(map_editor && "Preserving the widget&view information without retrieving it from a MapEditorController is not implemented yet!");
	
	if (!format)
		format = FileFormats.findFormatForFilename(path);

	if (!format)
		format = FileFormats.findFormat(FileFormats.defaultFormat());
	
	if (!format)
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot export the map as\n\"%1\"\nbecause the format is unknown.").arg(path));
		return false;
	}
	else if (!format->supportsExport())
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Cannot export the map as\n\"%1\"\nbecause saving as %2 (.%3) is not supported.").arg(path).arg(format->description()).arg(format->fileExtensions().join(", ")));
		return false;
	}
	
	// If the given file exists already, instead of overwriting the old file directly which
	// could lead to data loss if the program crashes while exporting, find a free temporary
	// filename, export to this path first and copy this over the old file on success.
	QString temp_path;
	if (QFile::exists(path))
	{
		int num_attempts = 0;
		const int max_attempts = 100;
		do
		{
			temp_path = path + ".";
			for (int i = 0; i < 6; ++i)
				temp_path += 'A' + (qrand() % ('Z' - 'A' + 1));
			++num_attempts;
		} while (num_attempts < max_attempts && QFile::exists(temp_path));
		if (num_attempts == max_attempts && QFile::exists(temp_path))
			temp_path = QString();
	}
	
	QFile file(temp_path.isEmpty() ? path : temp_path);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("File does not exist or insufficient permissions to open:\n%1").arg(path));
		return false;
	}
	
	// Update the relative paths of templates
	QDir map_dir = QFileInfo(path).absoluteDir();
	for (int i = 0; i < getNumTemplates(); ++i)
	{
		Template* temp = getTemplate(i);
		if (temp->getTemplateState() == Template::Invalid)
			temp->setTemplateRelativePath("");
		else
			temp->setTemplateRelativePath(map_dir.relativeFilePath(temp->getTemplatePath()));
	}
	for (int i = 0; i < getNumClosedTemplates(); ++i)
	{
		Template* temp = getClosedTemplate(i);
		if (temp->getTemplateState() == Template::Invalid)
			temp->setTemplateRelativePath("");
		else
			temp->setTemplateRelativePath(map_dir.relativeFilePath(temp->getTemplatePath()));
	}
	
	try
	{
		QScopedPointer<Exporter> exporter(format->createExporter(&file, this, map_editor->main_view));
		exporter->doExport();
		
		// Display any warnings.
		if (!exporter->warnings().empty())
		{
			QString warnings = "";
			for (std::vector<QString>::const_iterator it = exporter->warnings().begin(); it != exporter->warnings().end(); ++it) {
				if (!warnings.isEmpty())
					warnings += '\n';
				warnings += *it;
			}
			QMessageBox msgBox(QMessageBox::Warning, tr("Warning"), tr("The map export generated warnings."), QMessageBox::Ok);
			msgBox.setDetailedText(warnings);
			msgBox.exec();
		}
	}
	catch (std::exception &e)
	{
		file.close();
		
		QMessageBox::warning(NULL, tr("Error"), tr("Internal error while saving:\n%1").arg(e.what()));
		if (temp_path.isEmpty())
			QFile::remove(path);
		else
			QFile::remove(temp_path);
		
		return false;
	}
	
	file.close();
	if (!temp_path.isEmpty())
	{
		QFile::remove(path);
		QFile::rename(temp_path, path);
	}
	
	return true;
}

bool Map::loadFrom(const QString& path, QWidget* dialog_parent, MapEditorController* map_editor, bool load_symbols_only, bool show_error_messages)
{
	MapView *view = new MapView(this);

	// Ensure the file exists and is readable.
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (show_error_messages)
			QMessageBox::warning(
			  dialog_parent,
			  tr("Error"),
			  tr("Cannot open file:\n%1\nfor reading.").arg(path)
			);
		return false;
	}
	
	// Delete previous objects
	clear();

	// Read a block at the beginning of the file, that we can use for magic number checking.
	unsigned char buffer[256];
	size_t total_read = file.read((char *)buffer, 256);
	file.seek(0);

	bool import_complete = false;
	QString error_msg = tr("Invalid file type.");
	Q_FOREACH(const FileFormat *format, FileFormats.formats())
	{
		// If the format supports import, and thinks it can understand the file header, then proceed.
		if (format->supportsImport() && format->understands(buffer, total_read))
		{
			Importer *importer = NULL;
			// Wrap everything in a try block, so we can gracefully recover if the importer balks.
			try {
				// Create an importer instance for this file and map.
				importer = format->createImporter(&file, this, view);

				// Run the first pass.
				importer->doImport(load_symbols_only, QFileInfo(path).absolutePath());

				// Are there any actions the user must take to complete the import?
				if (!importer->actions().empty())
				{
					// TODO: prompt the user to resolve the action items. All-in-one dialog.
				}

				// Finish the import.
				importer->finishImport();
				
				file.close();

				// Display any warnings.
				if (!importer->warnings().empty() && show_error_messages)
				{
					QString warnings = "";
					for (std::vector<QString>::const_iterator it = importer->warnings().begin(); it != importer->warnings().end(); ++it) {
						if (!warnings.isEmpty())
							warnings += '\n';
						warnings += *it;
					}
					QMessageBox msgBox(
					  QMessageBox::Warning,
					  tr("Warning"),
					  tr("The map import generated warnings."),
					  QMessageBox::Ok,
					  dialog_parent
					);
					msgBox.setDetailedText(warnings);
					msgBox.exec();
				}

				import_complete = true;
			}
			catch (std::exception &e)
			{
				qDebug() << "Exception:" << e.what();
				error_msg = e.what();
			}
			if (importer) delete importer;
		}
		// If the last importer finished successfully
		if (import_complete) break;
	}
	
	if (map_editor)
		map_editor->main_view = view;
	else
		delete view;	// TODO: HACK. Better not create the view at all in this case!

	if (!import_complete)
	{
		if (show_error_messages)
			QMessageBox::warning(
			 dialog_parent,
			 tr("Error"),
			 tr("Cannot open file:\n%1\n\n%2").arg(path).arg(error_msg)
			);
		return false;
	}

	// Update all objects without trying to remove their renderables first, this gives a significant speedup when loading large files
	updateAllObjects(); // TODO: is the comment above still applicable?
	
	setHasUnsavedChanges(false);

	return true;
}

void Map::importMap(Map* other, ImportMode mode, QWidget* dialog_parent, std::vector<bool>* filter, int symbol_insert_pos,
					bool merge_duplicate_symbols, QHash<Symbol*, Symbol*>* out_symbol_map)
{
	// Check if there is something to import
	if (other->getNumColors() == 0 && other->getNumSymbols() == 0 && other->getNumObjects() == 0)
	{
		QMessageBox::critical(dialog_parent, tr("Error"), tr("Nothing to import."));
		return;
	}
	
	// Check scale
	if (other->getNumSymbols() > 0 && other->getScaleDenominator() != getScaleDenominator())
	{
		int answer = QMessageBox::question(dialog_parent, tr("Question"),
										   tr("The scale of the imported data is 1:%1 which is different from this map's scale of 1:%2.\n\nRescale the imported data?")
										   .arg(QLocale().toString(other->getScaleDenominator()))
										   .arg(QLocale().toString(getScaleDenominator())), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (answer == QMessageBox::Yes)
			other->changeScale(getScaleDenominator(), MapCoord(0, 0), true, true, true, true);
	}
	
	// TODO: As a special case if both maps are georeferenced, the location of the imported objects could be corrected
	
	// Determine which symbols to import
	std::vector<bool> symbol_filter;
	symbol_filter.resize(other->getNumSymbols(), true);
	if (mode == MinimalObjectImport)
	{
		if (other->getNumObjects() > 0)
			other->determineSymbolsInUse(symbol_filter);
	}
	else if (mode == MinimalSymbolImport && filter)
	{
		assert(filter->size() == symbol_filter.size());
		symbol_filter = *filter;
		other->determineSymbolUseClosure(symbol_filter);
	}
	
	// Determine which colors to import
	std::vector<bool> color_filter;
	color_filter.resize(other->getNumColors(), true);
	if (mode == MinimalObjectImport || mode == MinimalSymbolImport)
	{
		if (other->getNumSymbols() > 0)
			other->determineColorsInUse(symbol_filter, color_filter);
	}
	else if (mode == ColorImport && filter)
	{
		assert(filter->size() == color_filter.size());
		color_filter = *filter;
	}
	
	// Import colors
	MapColorMap color_map(color_set->importSet(*other->color_set, &color_filter, this));
	
	if (mode == ColorImport)
		return;
	
	QHash<Symbol*, Symbol*> symbol_map;
	if (other->getNumSymbols() > 0)
	{
		// Import symbols
		importSymbols(other, color_map, symbol_insert_pos, merge_duplicate_symbols, &symbol_filter, NULL, &symbol_map);
		if (out_symbol_map != NULL)
			*out_symbol_map = symbol_map;
	}
	
	if (mode == MinimalSymbolImport)
		return;
	
	if (other->getNumObjects() > 0)
	{
		// Import parts like this:
		//  - if the other map has only one part, import it into the current part
		//  - else check if there is already a part with an equal name for every part to import and import into this part if found, else create a new part
		for (int part = 0; part < other->getNumParts(); ++part)
		{
			MapPart* part_to_import = other->getPart(part);
			MapPart* dest_part = NULL;
			if (other->getNumParts() == 1)
				dest_part = getCurrentPart();
			else
			{
				for (int check_part = 0; check_part < getNumParts(); ++check_part)
				{
					if (getPart(check_part)->getName().compare(other->getPart(part)->getName(), Qt::CaseInsensitive) == 0)
					{
						dest_part = getPart(check_part);
						break;
					}
				}
				if (dest_part == NULL)
				{
					// Import as new part
					dest_part = new MapPart(part_to_import->getName(), this);
					addPart(dest_part, 0);
				}
			}
			
			// Temporarily switch the current part for importing so the undo step gets created for the right part
			MapPart* temp_current_part = getCurrentPart();
			current_part_index = findPartIndex(dest_part);
			
			bool select_and_center_objects = dest_part == temp_current_part;
			dest_part->importPart(part_to_import, symbol_map, select_and_center_objects);
			if (select_and_center_objects)
				ensureVisibilityOfSelectedObjects();
			
			current_part_index = findPartIndex(temp_current_part);
		}
	}
}

bool Map::exportToIODevice(QIODevice* stream)
{
	stream->open(QIODevice::WriteOnly);
	Exporter* exporter = NULL;
	try {
		const FileFormat* native_format = FileFormats.findFormat("XML");
		exporter = native_format->createExporter(stream, this, NULL);
		exporter->doExport();
		stream->close();
	}
	catch (std::exception &e)
	{
		if (exporter)
			delete exporter;
		return false;
	}
	if (exporter)
		delete exporter;
	return true;
}

bool Map::importFromIODevice(QIODevice* stream)
{
	Importer* importer = NULL;
	try {
		const FileFormat* native_format = FileFormats.findFormat("XML");
		importer = native_format->createImporter(stream, this, NULL);
		importer->doImport(false);
		importer->finishImport();
		stream->close();
	}
	catch (std::exception &e)
	{
		if (importer)
			delete importer;
		return false;
	}
	if (importer)
		delete importer;
	return true;
}

void Map::clear()
{
	if (color_set)
		color_set->dereference();
	color_set = new MapColorSet(this);
	
	int size = symbols.size();
	for (int i = 0; i < size; ++i)
		delete symbols[i];
	symbols.clear();
	
	size = templates.size();
	for (int i = 0; i < size; ++i)
		delete templates[i];
	templates.clear();
	first_front_template = 0;
	
	size = parts.size();
	for (int i = 0; i < size; ++i)
		delete parts[i];
	parts.clear();
	
	parts.push_back(new MapPart(tr("default part"), this));
	current_part_index = 0;
	
	object_selection.clear();
	first_selected_object = NULL;
	
	widgets.clear();
	undo_manager->clear();
	undo_manager->setClean();
	
	map_notes = "";
	
	printer_config.reset();
	
	image_template_use_meters_per_pixel = true;
	image_template_meters_per_pixel = 0;
	image_template_dpi = 0;
	image_template_scale = 0;
	
	colors_dirty = false;
	symbols_dirty = false;
	templates_dirty = false;
	objects_dirty = false;
	other_dirty = false;
	unsaved_changes = false;
}

void Map::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, float opacity)
{
	// Update the renderables of all objects marked as dirty
	updateObjects();
	
	// The actual drawing
	renderables->draw(painter, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols, opacity);
}

void Map::drawOverprintingSimulation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols)
{
	// Update the renderables of all objects marked as dirty
	updateObjects();
	
	// The actual drawing
	renderables->drawOverprintingSimulation(painter, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols);
}

void Map::drawColorSeparation(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool on_screen, bool show_helper_symbols, const MapColor* spot_color, bool use_color)
{
	// Update the renderables of all objects marked as dirty
	updateObjects();
	
	// The actual drawing
	renderables->drawColorSeparation(painter, bounding_box, force_min_size, scaling, on_screen, show_helper_symbols, spot_color, use_color);
}

void Map::drawGrid(QPainter* painter, QRectF bounding_box, bool on_screen)
{
	grid->draw(painter, bounding_box, this, on_screen);
}

void Map::drawTemplates(QPainter* painter, QRectF bounding_box, int first_template, int last_template, MapView* view, bool on_screen)
{
	for (int i = first_template; i <= last_template; ++i)
	{
		Template* temp = getTemplate(i);
		if ((view && !view->isTemplateVisible(temp)) || (temp->getTemplateState() != Template::Loaded))
			continue;
		float scale = (view ? view->getZoom() : 1) * std::max(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		
		painter->save();
		temp->drawTemplate(painter, bounding_box, scale, on_screen, view ? view->getTemplateVisibility(temp)->opacity : 1);
		painter->restore();
	}
}
void Map::updateObjects()
{
	// TODO: It maybe would be better if the objects entered themselves into a separate list when they get dirty so not all objects have to be traversed here
	int size = parts.size();
	for (int l = 0; l < size; ++l)
	{
		MapPart* part = parts[l];
		int obj_size = part->getNumObjects();
		for (int i = 0; i < obj_size; ++i)
		{
			Object* object = part->getObject(i);
			if (!object->update())
				continue;
		}
	}
}
void Map::removeRenderablesOfObject(Object* object, bool mark_area_as_dirty)
{
	renderables->removeRenderablesOfObject(object, mark_area_as_dirty);
	if (isObjectSelected(object))
		removeSelectionRenderables(object);
}
void Map::insertRenderablesOfObject(Object* object)
{
	renderables->insertRenderablesOfObject(object);
	if (isObjectSelected(object))
		addSelectionRenderables(object);
}

void Map::getSelectionToSymbolCompatibility(Symbol* symbol, bool& out_compatible, bool& out_different)
{
	out_compatible = symbol != NULL && (getNumSelectedObjects() > 0);
	out_different = false;
	
	if (symbol)
	{
		ObjectSelection::const_iterator it_end = selectedObjectsEnd();
		for (ObjectSelection::const_iterator it = selectedObjectsBegin(); it != it_end; ++it)
		{
			if (!symbol->isTypeCompatibleTo(*it))
			{
				out_compatible = false;
				out_different = true;
				return;
			}
			else if (symbol != (*it)->getSymbol())
				out_different = true;
		}
	}
}

void Map::deleteSelectedObjects()
{
	Map::ObjectSelection::const_iterator obj = selectedObjectsBegin();
	Map::ObjectSelection::const_iterator end = selectedObjectsEnd();
	if (obj != end)
	{
		// FIXME: this is not ready for multiple map parts.
		AddObjectsUndoStep* undo_step = new AddObjectsUndoStep(this);
		MapPart* part = getCurrentPart();
	
		for (; obj != end; ++obj)
		{
			int index = part->findObjectIndex(*obj);
			if (index >= 0)
			{
				undo_step->addObject(index, *obj);
				part->deleteObject(index, true);
			}
			else
			{
				qDebug() << this << "::deleteSelectedObjects(): Object" << *obj << "not found in current map part.";
			}
		}
		
		setObjectsDirty();
		clearObjectSelection(true);
		push(undo_step);
	}
}

void Map::includeSelectionRect(QRectF& rect)
{
	ObjectSelection::const_iterator it_end = selectedObjectsEnd();
	ObjectSelection::const_iterator it = selectedObjectsBegin();
	if (it != it_end)
		rectIncludeSafe(rect, (*it)->getExtent());
	else
		return;
	++it;
	
	while (it != it_end)
	{
		rectInclude(rect, (*it)->getExtent());
		++it;
	}
}
void Map::drawSelection(QPainter* painter, bool force_min_size, MapWidget* widget, MapRenderables* replacement_renderables, bool draw_normal)
{
	const float selection_opacity_factor = draw_normal ? 1 : 0.4f;
	
	MapView* view = widget->getMapView();
	
	painter->save();
	painter->translate(widget->width() / 2.0 + view->getDragOffset().x(), widget->height() / 2.0 + view->getDragOffset().y());
	view->applyTransform(painter);
	
	if (!replacement_renderables)
		replacement_renderables = selection_renderables.data();
	replacement_renderables->draw(painter, view->calculateViewedRect(widget->viewportToView(widget->rect())), force_min_size, view->calculateFinalZoomFactor(), true, true, selection_opacity_factor, !draw_normal);
	
	painter->restore();
}

void Map::addObjectToSelection(Object* object, bool emit_selection_changed)
{
	assert(!isObjectSelected(object));
	object_selection.insert(object);
	addSelectionRenderables(object);
	if (!first_selected_object)
		first_selected_object = object;
	if (emit_selection_changed)
		emit(objectSelectionChanged());
}
void Map::removeObjectFromSelection(Object* object, bool emit_selection_changed)
{
	bool removed = object_selection.remove(object);
	assert(removed && "Map::removeObjectFromSelection: object was not selected!");
	removeSelectionRenderables(object);
	if (first_selected_object == object)
		first_selected_object = object_selection.isEmpty() ? NULL : *object_selection.begin();
	if (emit_selection_changed)
		emit(objectSelectionChanged());
}
bool Map::removeSymbolFromSelection(Symbol* symbol, bool emit_selection_changed)
{
	bool removed_at_least_one_object = false;
	ObjectSelection::iterator it_end = object_selection.end();
	for (ObjectSelection::iterator it = object_selection.begin(); it != it_end; )
	{
		if ((*it)->getSymbol() != symbol)
		{
			++it;
			continue;
		}
		
		removed_at_least_one_object = true;
		removeSelectionRenderables(*it);
		Object* removed_object = *it;
		it = object_selection.erase(it);
		if (first_selected_object == removed_object)
			first_selected_object = object_selection.isEmpty() ? NULL : *object_selection.begin();
	}
	if (emit_selection_changed && removed_at_least_one_object)
		emit(objectSelectionChanged());
	return removed_at_least_one_object;
}
bool Map::isObjectSelected(Object* object)
{
	return object_selection.contains(object);
}
bool Map::toggleObjectSelection(Object* object, bool emit_selection_changed)
{
	if (isObjectSelected(object))
	{
		removeObjectFromSelection(object, emit_selection_changed);
		return false;
	}
	else
	{
		addObjectToSelection(object, emit_selection_changed);
		return true;
	}
}
void Map::clearObjectSelection(bool emit_selection_changed)
{
	selection_renderables->clear();
	object_selection.clear();
	first_selected_object = NULL;
	
	if (emit_selection_changed)
		emit(objectSelectionChanged());
}
void Map::emitSelectionChanged()
{
	emit(objectSelectionChanged());
}
void Map::emitSelectionEdited()
{
	emit(selectedObjectEdited());
}

void Map::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
}
void Map::removeMapWidget(MapWidget* widget)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
	{
		if (widgets[i] == widget)
		{
			widgets.erase(widgets.begin() + i);
			return;
		}
	}
	assert(false);
}
void Map::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateEverything();
}
void Map::ensureVisibilityOfSelectedObjects()
{
	if (getNumSelectedObjects() == 0)
		return;
	
	QRectF rect;
	includeSelectionRect(rect);
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->ensureVisibilityOfRect(rect, true, true);
}

void Map::setDrawingBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setDrawingBoundingBox(map_coords_rect, pixel_border, do_update);
}
void Map::clearDrawingBoundingBox()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->clearDrawingBoundingBox();
}

void Map::setActivityBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setActivityBoundingBox(map_coords_rect, pixel_border, do_update);
}
void Map::clearActivityBoundingBox()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->clearActivityBoundingBox();
}

void Map::updateDrawing(QRectF map_coords_rect, int pixel_border)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateDrawing(map_coords_rect, pixel_border);
}

void Map::setColor(MapColor* color, int pos)
{
	MapColor* old_color = color_set->colors[pos];
	
	color_set->colors[pos] = color;
	color->setPriority(pos);
	
	if (color->getSpotColorMethod() == MapColor::SpotColor)
	{
		// Update dependent colors
		Q_FOREACH(MapColor* map_color, color_set->colors)
		{
			if (map_color->getSpotColorMethod() != MapColor::CustomColor)
				continue;
			
			Q_FOREACH(SpotColorComponent component, map_color->getComponents())
			{
				if (component.spot_color == old_color)
				{
					component.spot_color = color;
					// Assuming each spot color is rarely used more than once per composition
					if (map_color->getCmykColorMethod() == MapColor::SpotColor)
						map_color->setCmykFromSpotColors();
					if (map_color->getRgbColorMethod() == MapColor::SpotColor)
						map_color->setRgbFromSpotColors();
					emit colorChanged(map_color->getPriority(), map_color);
				}
			}
		}
	}
	
	// Regenerate all symbols' icons
	int size = (int)symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (symbols[i]->containsColor(color))
		{
			symbols[i]->resetIcon();
			emit(symbolIconChanged(i));
		}
	}
	
	emit(colorChanged(pos, color));
}
MapColor* Map::addColor(int pos)
{
	MapColor* new_color = new MapColor(tr("New color"), pos);
	
	color_set->colors.insert(color_set->colors.begin() + pos, new_color);
	adjustColorPriorities(pos + 1, color_set->colors.size() - 1);
	checkIfFirstColorAdded();
	setColorsDirty();
	emit(colorAdded(pos, new_color));
	return new_color;
}
void Map::addColor(MapColor* color, int pos)
{
	color_set->colors.insert(color_set->colors.begin() + pos, color);
	adjustColorPriorities(pos + 1, color_set->colors.size() - 1);
	checkIfFirstColorAdded();
	setColorsDirty();
	emit(colorAdded(pos, color));
	color->setPriority(pos);
}
void Map::deleteColor(int pos)
{
	MapColor* color = color_set->colors[pos];
	if (color->getSpotColorMethod() == MapColor::SpotColor)
	{
		// Update dependent colors
		Q_FOREACH(MapColor* map_color, color_set->colors)
		{
			if (map_color->getSpotColorMethod() != MapColor::CustomColor)
				continue;
			
			SpotColorComponents out_components = map_color->getComponents();
			SpotColorComponents::iterator com_it = out_components.begin();
			while(com_it != out_components.end())
			{
				if (com_it->spot_color == color)
				{
					com_it = out_components.erase(com_it);
					// Assuming each spot color is rarely used more than once per composition
					map_color->setSpotColorComposition(out_components);
					emit colorChanged(map_color->getPriority(), map_color);
				}
				else
					com_it++;
			}
		}
	}
	
	color_set->colors.erase(color_set->colors.begin() + pos);
	adjustColorPriorities(pos, color_set->colors.size() - 1);
	
	if (getNumColors() == 0)
	{
		// That was the last color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
	
	int size = (int)symbols.size();
	// Treat combined symbols first before their parts
	for (int i = 0; i < size; ++i)
	{
		if (symbols[i]->getType() == Symbol::Combined)
			symbols[i]->colorDeleted(color);
	}
	for (int i = 0; i < size; ++i)
	{
		if (symbols[i]->getType() != Symbol::Combined)
			symbols[i]->colorDeleted(color);
	}
	emit(colorDeleted(pos, color));
	
	delete color;
}

int Map::findColorIndex(const MapColor* color) const
{
	int size = (int)color_set->colors.size();
	for (int i = 0; i < size; ++i)
	{
		if (color_set->colors[i] == color)
			return i;
	}
	if (color && color->getPriority() == MapColor::Registration)
	{
		return MapColor::Registration;
	}
	return -1;
}
void Map::setColorsDirty()
{
	setHasUnsavedChanges();
	colors_dirty = true;
}

void Map::useColorsFrom(Map* map)
{
	color_set->dereference();
	color_set = map->color_set;
	color_set->addReference();
}

bool Map::isColorUsedByASymbol(const MapColor* color) const
{
	int size = (int)symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (symbols[i]->containsColor(color))
			return true;
	}
	return false;
}

void Map::adjustColorPriorities(int first, int last)
{
	// TODO: delete or update RenderStates with these colors
	for (int i = first; i <= last; ++i)
		color_set->colors[i]->setPriority(i);
}

void Map::determineColorsInUse(const std::vector< bool >& by_which_symbols, std::vector< bool >& out)
{
	if (getNumSymbols() == 0)
	{
		out.clear();
		return;
	}
	
	Q_ASSERT((int)by_which_symbols.size() == getNumSymbols());
	out.assign(getNumColors(), false);
	for (int c = 0; c < getNumColors(); ++c)
	{
		for (int s = 0; s < getNumSymbols(); ++s)
		{
			if (by_which_symbols[s] && getSymbol(s)->containsColor(getColor(c)))
			{
				out[c] = true;
				break;
			}
		}
	}
}

void Map::checkSpotColorPresence()
{
	const bool has_spot_colors = hasSpotColors();
	if (this->has_spot_colors != has_spot_colors)
	{
		this->has_spot_colors = has_spot_colors;
		emit spotColorPresenceChanged(has_spot_colors);
	}
}

bool Map::hasSpotColors() const
{
	for (ColorVector::const_iterator c = color_set->colors.begin(), end = color_set->colors.end(); c != end; ++c)
	{
		if ((*c)->getSpotColorMethod() == MapColor::SpotColor)
			return true;
	}
	return false;
}

void Map::importSymbols(Map* other, const MapColorMap& color_map, int insert_pos, bool merge_duplicates, std::vector< bool >* filter,
						QHash< int, int >* out_indexmap, QHash< Symbol*, Symbol* >* out_pointermap)
{
	// We need a pointer map (and keep track of added symbols) to adjust the references of combined symbols
	std::vector<Symbol*> added_symbols;
	QHash< Symbol*, Symbol* > local_pointermap;
	if (!out_pointermap)
		out_pointermap = &local_pointermap;
	
	for (size_t i = 0, end = other->symbols.size(); i < end; ++i)
	{
		if (filter && !filter->at(i))
			continue;
		Symbol* other_symbol = other->symbols.at(i);
		
		if (!merge_duplicates)
		{
			added_symbols.push_back(other_symbol);
			continue;
		}
		
		// Check if symbol is already present
		size_t k = 0;
		size_t symbols_size = symbols.size();
		for (; k < symbols_size; ++k)
		{
			if (symbols[k]->equals(other_symbol, Qt::CaseInsensitive, false))
			{
				// Symbol is already present
				if (out_indexmap)
					out_indexmap->insert(i, k);
				if (out_pointermap)
					out_pointermap->insert(other_symbol, symbols[k]);
				break;
			}
		}
		
		if (k >= symbols_size)
		{
			// Symbols does not exist in this map yet, mark it to be added
			added_symbols.push_back(other_symbol);
		}
	}
	
	// Really add all the added symbols
	if (insert_pos < 0)
		insert_pos = getNumSymbols();
	for (size_t i = 0, end = added_symbols.size(); i < end; ++i)
	{
		Symbol* old_symbol = added_symbols[i];
		Symbol* new_symbol = added_symbols[i]->duplicate(&color_map);
		added_symbols[i] = new_symbol;
		
		addSymbol(new_symbol, insert_pos);
		++insert_pos;
		
		if (out_indexmap)
			out_indexmap->insert(i, insert_pos);
		if (out_pointermap)
			out_pointermap->insert(old_symbol, new_symbol);
	}
	
	// Notify added symbols of other "environment"
	for (size_t i = 0, end = added_symbols.size(); i < end; ++i)
	{
		QHash<Symbol*, Symbol*>::iterator it = out_pointermap->begin();
		while (it != out_pointermap->end())
		{
			added_symbols[i]->symbolChanged(it.key(), it.value());
			++it;
		}
	}
}

void Map::addSelectionRenderables(Object* object)
{
	object->update(false);
	selection_renderables->insertRenderablesOfObject(object);
}
void Map::updateSelectionRenderables(Object* object)
{
	removeSelectionRenderables(object);
	addSelectionRenderables(object);
}
void Map::removeSelectionRenderables(Object* object)
{
	selection_renderables->removeRenderablesOfObject(object, false);
}

void Map::initStatic()
{
	static_initialized = true;
	
	covering_white_line = new LineSymbol();
	covering_white_line->setColor(&covering_white);
	covering_white_line->setLineWidth(3);
	
	covering_red_line = new LineSymbol();
	covering_red_line->setColor(&covering_red);
	covering_red_line->setLineWidth(0.1);
	
	covering_combined_line = new CombinedSymbol();
	covering_combined_line->setNumParts(2);
	covering_combined_line->setPart(0, covering_white_line, false);
	covering_combined_line->setPart(1, covering_red_line, false);
	
	// Undefined symbols
	undefined_line = new LineSymbol();
	undefined_line->setColor(&undefined_symbol_color);
	undefined_line->setLineWidth(1);
	undefined_line->setIsHelperSymbol(true);

	undefined_point = new PointSymbol();
	undefined_point->setInnerRadius(100);
	undefined_point->setInnerColor(&undefined_symbol_color);
	undefined_point->setIsHelperSymbol(true);
}

void Map::addSymbol(Symbol* symbol, int pos)
{
	symbols.insert(symbols.begin() + pos, symbol);
	checkIfFirstSymbolAdded();
	emit(symbolAdded(pos, symbol));
	setSymbolsDirty();
}
void Map::moveSymbol(int from, int to)
{
	symbols.insert(symbols.begin() + to, symbols[from]);
	if (from > to)
		++from;
	symbols.erase(symbols.begin() + from);
	// TODO: emit(symbolChanged(pos, symbol)); ?
	setSymbolsDirty();
}

Symbol* Map::getSymbol(int i) const
{
	if (i >= 0)
		return symbols[i];
	else if (i == -1)
		return NULL;
	else if (i == -2)
		return getUndefinedPoint();
	else if (i == -3)
		return getUndefinedLine();
	else
	{
		assert(!"Invalid symbol index given");
		return getUndefinedLine();
	}
}

void Map::setSymbol(Symbol* symbol, int pos)
{
	Symbol* old_symbol = symbols[pos];
	
	// Check if an object with this symbol is selected
	bool object_with_symbol_selected = false;
	for (ObjectSelection::iterator it = object_selection.begin(), it_end = object_selection.end(); it != it_end; ++it)
	{
		if ((*it)->getSymbol() == old_symbol || (*it)->getSymbol()->containsSymbol(old_symbol))
		{
			object_with_symbol_selected = true;
			break;
		}
	}
	
	changeSymbolForAllObjects(old_symbol, symbol);
	
	int size = (int)symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (i == pos)
			continue;
		
		if (symbols[i]->symbolChanged(symbols[pos], symbol))
			updateAllObjectsWithSymbol(symbols[i]);
	}
	
	// Change the symbol
	symbols[pos] = symbol;
	emit(symbolChanged(pos, symbol, old_symbol));
	setSymbolsDirty();
	delete old_symbol;
	
	if (object_with_symbol_selected)
		emit selectedObjectEdited();
}
void Map::deleteSymbol(int pos)
{
	if (deleteAllObjectsWithSymbol(symbols[pos]))
		undo_manager->clear();
	
	int size = (int)symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (i == pos)
			continue;
		
		if (symbols[i]->symbolChanged(symbols[pos], NULL))
			updateAllObjectsWithSymbol(symbols[i]);
	}
	
	// Delete the symbol
	Symbol* temp = symbols[pos];
	delete symbols[pos];
	symbols.erase(symbols.begin() + pos);
	
	if (getNumSymbols() == 0)
	{
		// That was the last symbol - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
	
	emit(symbolDeleted(pos, temp));
	setSymbolsDirty();
}

int Map::findSymbolIndex(const Symbol* symbol) const
{
	if (!symbol)
		return -1;
	int size = (int)symbols.size();
	for (int i = 0; i < size; ++i)
	{
		if (symbols[i] == symbol)
			return i;
	}
	
	if (symbol == undefined_point)
		return -2;
	else if (symbol == undefined_line)
		return -3;
	
	// maybe element of point symbol
	return -1;
}

void Map::setSymbolsDirty()
{
	setHasUnsavedChanges();
	symbols_dirty = true;
}

void Map::scaleAllSymbols(double factor)
{
	int size = getNumSymbols();
	for (int i = 0; i < size; ++i)
	{
		Symbol* symbol = getSymbol(i);
		symbol->scale(factor);
		emit(symbolChanged(i, symbol, symbol));
	}
	updateAllObjects();
	
	setSymbolsDirty();
}

void Map::determineSymbolsInUse(std::vector< bool >& out)
{
	out.assign(symbols.size(), false);
	for (int l = 0; l < (int)parts.size(); ++l)
	{
		for (int o = 0; o < parts[l]->getNumObjects(); ++o)
		{
			Symbol* symbol = parts[l]->getObject(o)->getSymbol();
			int index = findSymbolIndex(symbol);
			if (index >= 0)
				out[index] = true;
		}
	}
	
	determineSymbolUseClosure(out);
}

void Map::determineSymbolUseClosure(std::vector< bool >& symbol_bitfield)
{
	bool change;
	do
	{
		change = false;
		
		for (size_t i = 0, end = symbol_bitfield.size(); i < end; ++i)
		{
			if (symbol_bitfield[i])
				continue;
			
			// Check if this symbol is needed by any included symbol
			for (size_t k = 0; k < end; ++k)
			{
				if (i == k)
					continue;
				if (!symbol_bitfield[k])
					continue;
				if (symbols[k]->containsSymbol(symbols[i]))
				{
					symbol_bitfield[i] = true;
					change = true;
					break;
				}
			}
		}
		
	} while (change);
}

void Map::setTemplate(Template* temp, int pos)
{
	templates[pos] = temp;
	emit(templateChanged(pos, templates[pos]));
}
void Map::addTemplate(Template* temp, int pos, MapView* view)
{
	templates.insert(templates.begin() + pos, temp);
	checkIfFirstTemplateAdded();
	
	if (view)
	{
		TemplateVisibility* vis = view->getTemplateVisibility(temp);
		vis->visible = true;
		vis->opacity = 1;
	}
	
	emit(templateAdded(pos, temp));
}

void Map::removeTemplate(int pos)
{
	TemplateVector::iterator it = templates.begin() + pos;
	Template* temp = *it;
	
	// Delete visibility information for the template from all views - get the views indirectly by iterating over all widgets
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->getMapView()->deleteTemplateVisibility(temp);
	
	templates.erase(it);
	
	if (getNumTemplates() == 0)
	{
		// That was the last tempate - the help text in the map widget(s) should maybe be updated (if there are no objects)
		updateAllMapWidgets();
	}
	
	emit(templateDeleted(pos, temp));
}

void Map::deleteTemplate(int pos)
{
	Template* temp = getTemplate(pos);
	removeTemplate(pos);
	delete temp;
}
void Map::setTemplateAreaDirty(Template* temp, QRectF area, int pixel_border)
{
	bool front_cache = findTemplateIndex(temp) >= getFirstFrontTemplate();	// TODO: is there a better way to find out if that is a front or back template?
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		if (widgets[i]->getMapView()->isTemplateVisible(temp))
			widgets[i]->markTemplateCacheDirty(widgets[i]->getMapView()->calculateViewBoundingBox(area), pixel_border, front_cache);
}
void Map::setTemplateAreaDirty(int i)
{
	if (i == -1)
		return;	// no assert here as convenience, so setTemplateAreaDirty(-1) can be called without effect for the map part
	assert(i >= 0 && i < (int)templates.size());
	
	templates[i]->setTemplateAreaDirty();
}
int Map::findTemplateIndex(const Template* temp) const
{
	int size = (int)templates.size();
	for (int i = 0; i < size; ++i)
	{
		if (templates[i] == temp)
			return i;
	}
	assert(false);
	return -1;
}
void Map::setTemplatesDirty()
{
	setHasUnsavedChanges();
	templates_dirty = true;
}
void Map::emitTemplateChanged(Template* temp)
{
	emit(templateChanged(findTemplateIndex(temp), temp));
}

void Map::clearClosedTemplates()
{
	if (closed_templates.size() == 0)
		return;
	for (int i = 0; i < (int)closed_templates.size(); ++i)
		delete closed_templates[i];
	closed_templates.clear();
	setTemplatesDirty();
	emit closedTemplateAvailabilityChanged();
}

void Map::closeTemplate(int i)
{
	Template* temp = getTemplate(i);
	removeTemplate(i);
	
	if (temp->getTemplateState() == Template::Loaded)
		temp->unloadTemplateFile();
	
	closed_templates.push_back(temp);
	setTemplatesDirty();
	if (closed_templates.size() == 1)
		emit closedTemplateAvailabilityChanged();
}

bool Map::reloadClosedTemplate(int i, int target_pos, QWidget* dialog_parent, MapView* view, const QString& map_path)
{
	Template* temp = closed_templates[i];
	
	// Try to find and load the template again
	if (temp->getTemplateState() != Template::Loaded)
	{
		if (!temp->tryToFindAndReloadTemplateFile(map_path))
		{
			if (!temp->execSwitchTemplateFileDialog(dialog_parent))
				return false;
		}
	}
	
	// If successfully loaded, add to template list again
	if (temp->getTemplateState() == Template::Loaded)
	{
		closed_templates.erase(closed_templates.begin() + i);
		addTemplate(temp, target_pos, view);
		temp->setTemplateAreaDirty();
		setTemplatesDirty();
		if (closed_templates.size() == 0)
			emit closedTemplateAvailabilityChanged();
		return true;
	}
	return false;
}

void Map::push(UndoStep *step)
{
	undo_manager->push(step);
}

void Map::addPart(MapPart* part, int pos)
{
	parts.insert(parts.begin() + pos, part);
	if (current_part_index >= pos)
		++current_part_index;
	
	emit currentMapPartChanged(current_part_index);
	
	setOtherDirty(true);
	for(unsigned int i = 0; i < widgets.size(); i++)
		widgets[i]->updateEverything();
}

void Map::removePart(int index)
{
	MapPart* part = parts[index];
	
	// FIXME: This loop should move to MapPart.
	while(part->getNumObjects())
		part->deleteObject(0, false);
	delete part;
	
	parts.erase(parts.begin() + index);
	
	if(current_part_index == index) {
		current_part_index = 0;
		emit currentMapPartChanged(current_part_index);
	}
	
	setOtherDirty(true);
	for(unsigned int i = 0; i < widgets.size(); i++)
		widgets[i]->updateEverything();
}

int Map::findPartIndex(MapPart* part) const
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i] == part)
			return i;
	}
	assert(false);
	return -1;
}

void Map::setCurrentPart(int index)
{
	Q_ASSERT(index >= 0 && index < (int)parts.size());
	if (index == current_part_index)
		return;
	
	clearObjectSelection(true);
	
	current_part_index = index;
	emit currentMapPartChanged(index);
}

void Map::reassignObjectsToMapPart(QSet<Object*>::const_iterator begin, QSet<Object*>::const_iterator end, int destination)
{
	for (QSet<Object*>::const_iterator it = begin; it != end; it++) {
		Object* object = *it;
		deleteObject(object, true);
		addObject(object, destination);
	}
	
	setOtherDirty();
}

void Map::mergeParts(int source, int destination)
{
	// FIXME: Retain object selection
	clearObjectSelection(false);
	
	MapPart* source_part = parts[source];
	
	while (source_part->getNumObjects()) {
		int obj_index = source_part->getNumObjects() - 1;
		Object* obj = source_part->getObject(obj_index);
		source_part->deleteObject(obj_index, true);
		addObject(obj, destination);
		addObjectToSelection(obj, false);
	}
	
	removePart(source);
	
	emit objectSelectionChanged();
}

int Map::getNumObjects()
{
	int num_objects = 0;
	int size = parts.size();
	for (int i = 0; i < size; ++i)
		num_objects += parts[i]->getNumObjects();
	return num_objects;
}
int Map::addObject(Object* object, int part_index)
{
	MapPart* part = parts[(part_index < 0) ? current_part_index : part_index];
	int object_index = part->getNumObjects();
	part->addObject(object, object_index);
	
	return object_index;
}
void Map::deleteObject(Object* object, bool remove_only)
{
	int size = parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i]->deleteObject(object, remove_only))
			return;
	}
	
	qCritical().nospace() << this << "::deleteObject(" << object << "," << remove_only << "): Object not found. This is a bug.";
	if (!remove_only)
		delete object;
}
void Map::setObjectsDirty()
{
	setHasUnsavedChanges();
	objects_dirty = true;
}

QRectF Map::calculateExtent(bool include_helper_symbols, bool include_templates, const MapView* view) const
{
	QRectF rect;
	
	// Objects
	int size = parts.size();
	for (int i = 0; i < size; ++i)
		rectIncludeSafe(rect, parts[i]->calculateExtent(include_helper_symbols));
	
	// Templates
	if (include_templates)
	{
		size = templates.size();
		for (int i = 0; i < size; ++i)
		{
			if (view && !view->isTemplateVisible(templates[i]))
				continue;
            if (templates[i]->getTemplateState() != Template::Loaded)
              continue;
			
			QRectF template_bbox = templates[i]->calculateTemplateBoundingBox();
			rectIncludeSafe(rect, template_bbox);
		}
	}
	
	return rect;
}
void Map::setObjectAreaDirty(QRectF map_coords_rect)
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->markObjectAreaDirty(map_coords_rect);
}
void Map::findObjectsAt(MapCoordF coord, float tolerance, bool treat_areas_as_paths, bool extended_selection, bool include_hidden_objects, bool include_protected_objects, SelectionInfoVector& out)
{
	getCurrentPart()->findObjectsAt(coord, tolerance, treat_areas_as_paths, extended_selection, include_hidden_objects, include_protected_objects, out);
}
void Map::findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, bool include_hidden_objects, bool include_protected_objects, std::vector< Object* >& out)
{
	getCurrentPart()->findObjectsAtBox(corner1, corner2, include_hidden_objects, include_protected_objects, out);
}

int Map::countObjectsInRect(QRectF map_coord_rect, bool include_hidden_objects)
{
	int count = 0;
	int size = parts.size();
	for (int i = 0; i < size; ++i)
		count += parts[i]->countObjectsInRect(map_coord_rect, include_hidden_objects);
	return count;
}

void Map::scaleAllObjects(double factor, const MapCoord& scaling_center)
{
	operationOnAllObjects(ObjectOp::Scale(factor, scaling_center));
}
void Map::rotateAllObjects(double rotation, const MapCoord& center)
{
	operationOnAllObjects(ObjectOp::Rotate(rotation, center));
}
void Map::updateAllObjects()
{
	operationOnAllObjects(ObjectOp::Update(true));
}
void Map::updateAllObjectsWithSymbol(Symbol* symbol)
{
	operationOnAllObjects(ObjectOp::Update(true), ObjectOp::HasSymbol(symbol));
}
void Map::changeSymbolForAllObjects(Symbol* old_symbol, Symbol* new_symbol)
{
	operationOnAllObjects(ObjectOp::ChangeSymbol(new_symbol), ObjectOp::HasSymbol(old_symbol));
}
bool Map::deleteAllObjectsWithSymbol(Symbol* symbol)
{
	// Remove objects from selection
	removeSymbolFromSelection(symbol, true);
	
	// Delete objects from map
	return operationOnAllObjects(ObjectOp::Delete(), ObjectOp::HasSymbol(symbol)) & ObjectOperationResult::Success;
}
bool Map::doObjectsExistWithSymbol(Symbol* symbol)
{
	return operationOnAllObjects(ObjectOp::NoOp(), ObjectOp::HasSymbol(symbol)) & ObjectOperationResult::Success;
}

void Map::setGeoreferencing(const Georeferencing& georeferencing)
{
	*this->georeferencing = georeferencing;
	setOtherDirty(true);
}

const MapPrinterConfig& Map::printerConfig()
{
	if (printer_config.isNull())
		printer_config.reset(new MapPrinterConfig(*this));
	
	return *printer_config;
}

MapPrinterConfig Map::printerConfig() const
{
	if (printer_config.isNull())
		return MapPrinterConfig(*this);
	
	return *printer_config;
}

void Map::setPrinterConfig(const MapPrinterConfig& config)
{
	if (printer_config.isNull())
	{
		printer_config.reset(new MapPrinterConfig(config));
		setOtherDirty(true);
	}
	else if (*printer_config != config)
	{
		*printer_config = config;
		setOtherDirty(true);
	}
}

void Map::setImageTemplateDefaults(bool use_meters_per_pixel, double meters_per_pixel, double dpi, double scale)
{
	image_template_use_meters_per_pixel = use_meters_per_pixel;
	image_template_meters_per_pixel = meters_per_pixel;
	image_template_dpi = dpi;
	image_template_scale = scale;
}
void Map::getImageTemplateDefaults(bool& use_meters_per_pixel, double& meters_per_pixel, double& dpi, double& scale)
{
	use_meters_per_pixel = image_template_use_meters_per_pixel;
	meters_per_pixel = image_template_meters_per_pixel;
	dpi = image_template_dpi;
	scale = image_template_scale;
}

void Map::setHasUnsavedChanges(bool has_unsaved_changes)
{
	if (!has_unsaved_changes)
	{
		colors_dirty = false;
		symbols_dirty = false;
		templates_dirty = false;
		objects_dirty = false;
		other_dirty = false;
		unsaved_changes = false;
	}
	else
	{
		emit gotUnsavedChanges(); // always needed to trigger autosave
		unsaved_changes = true;
	}
}
void Map::setOtherDirty(bool value)
{
	if (!other_dirty && value)
		setHasUnsavedChanges();
	other_dirty = value;
}

void Map::checkIfFirstColorAdded()
{
	if (getNumColors() == 1)
	{
		// This is the first color - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
void Map::checkIfFirstSymbolAdded()
{
	if (getNumSymbols() == 1)
	{
		// This is the first symbol - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
void Map::checkIfFirstTemplateAdded()
{
	if (getNumTemplates() == 1)
	{
		// This is the first template - the help text in the map widget(s) should be updated
		updateAllMapWidgets();
	}
}
