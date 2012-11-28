/*
 *    Copyright 2012 Thomas Schöps
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

#include "map_color.h"
#include "map_editor.h"
#include "map_grid.h"
#include "map_part.h"
#include "map_widget.h"
#include "map_undo.h"
#include "util.h"
#include "template.h"
#include "gps_coordinates.h"
#include "object.h"
#include "object_operations.h"
#include "renderable.h"
#include "symbol.h"
#include "symbol_point.h"
#include "symbol_line.h"
#include "symbol_combined.h"
#include "file_format_ocad8.h"
#include "georeferencing.h"

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

void Map::MapColorSet::importSet(Map::MapColorSet* other, Map* map, std::vector< bool >* filter, QHash< int, int >* out_indexmap, QHash<MapColor*, MapColor*>* out_pointermap)
{
	// Count colors to import
	size_t import_count;
	if (filter)
	{
		import_count = 0;
		for (size_t i = 0, end = other->colors.size(); i < end; ++i)
		{
			if (filter->at(i))
				++import_count;
		}
	}
	else
		import_count = other->colors.size();
	if (import_count == 0)
		return;
	
	colors.reserve(colors.size() + import_count);
	
	// Import colors
	int start_insertion_index = 0;
	bool priorities_changed = false;
	for (int i = 0; i < (int)other->colors.size(); ++i)
	{
		if (filter && !filter->at(i))
			continue;
		MapColor* other_color = other->colors.at(i);
		
		// Check if color is already present, first with comparing the priority, then without
		// TODO: priorities are shifted as soon as one new color is inserted, so this breaks
		int found_index = -1;
		for (size_t k = 0, colors_size = colors.size(); k < colors_size; ++k)
		{
			if (colors.at(k)->equals(*other_color, true))
			{
				found_index = k;
				break;
			}
		}
		if (found_index == -1)
		{
			for (size_t k = 0, colors_size = colors.size(); k < colors_size; ++k)
			{
				if (colors.at(k)->equals(*other_color, false))
				{
					found_index = k;
					break;
				}
			}
		}
		
		if (found_index >= 0)
		{
			if (out_indexmap)
				out_indexmap->insert(i, found_index);
			if (out_pointermap)
				out_pointermap->insert(other_color, colors[found_index]);
		}
		else
		{
			// Color does not exist in this map yet.
			// Check if the color above in other also exists in this set
			int found_above_index = -1;
			if (i > 0)
			{
				MapColor* other_color_above = other->colors.at(i - 1);
				for (size_t k = 0, colors_size = colors.size(); k < colors_size; ++k)
				{
					if (colors.at(k)->equals(*other_color_above, false))
					{
						found_above_index = k;
						break;
					}
				}
			}
			
			int insertion_index;
			if (found_above_index >= 0)
			{
				// Add it below the same color under which it was before
				insertion_index = found_above_index + 1;
			}
			else
			{
				// Add it at the beginning
				insertion_index = start_insertion_index;
				++start_insertion_index;
			}
			
			MapColor* new_color = new MapColor(*other_color);
			if (map)
				map->addColor(new_color, insertion_index);
			else
				colors.insert(colors.begin() + insertion_index, new_color);
			
			priorities_changed = true;
			
			if (out_indexmap)
			{
				QHash<int, int>::iterator it = out_indexmap->begin();
				while (it != out_indexmap->end())
				{
					if (it.value() >= insertion_index)
						++it.value();
					++it;
				}
				out_indexmap->insert(i, insertion_index);
			}
			if (out_pointermap)
				out_pointermap->insert(other_color, new_color);
		}
	}
	
	if (map && priorities_changed)
		map->updateAllObjects();
}

// ### Map ###

bool Map::static_initialized = false;
MapColor Map::covering_white(MapColor::CoveringWhite);
MapColor Map::covering_red(MapColor::CoveringRed);
MapColor Map::undefined_symbol_color(MapColor::Undefined);
LineSymbol* Map::covering_white_line;
LineSymbol* Map::covering_red_line;
LineSymbol* Map::undefined_line;
PointSymbol* Map::undefined_point;
CombinedSymbol* Map::covering_combined_line;

Map::Map() : renderables(new MapRenderables(this)), selection_renderables(new MapRenderables(this))
{
	if (!static_initialized)
		initStatic();
	
	color_set = NULL;
	object_undo_manager.setOwner(this);
	georeferencing = new Georeferencing();
	grid = new MapGrid();
	area_hatching_enabled = false;
	baseline_view_enabled = false;
	
	clear();
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

void Map::setScaleDenominator(int value)
{
	georeferencing->setScaleDenominator(value);
}

int Map::getScaleDenominator() const
{
	return georeferencing->getScaleDenominator();
}

void Map::changeScale(int new_scale_denominator, bool scale_symbols, bool scale_objects, bool scale_georeferencing, bool scale_templates)
{
	if (new_scale_denominator == getScaleDenominator())
		return;
	
	double factor = getScaleDenominator() / (double)new_scale_denominator;
	
	if (scale_symbols)
		scaleAllSymbols(factor);
	if (scale_objects)
	{
		object_undo_manager.clear(false);
		scaleAllObjects(factor);
	}
	if (scale_georeferencing)
		georeferencing->setMapRefPoint(factor * georeferencing->getMapRefPoint());
	if (scale_templates)
	{
		for (int i = 0; i < getNumTemplates(); ++i)
		{
			Template* temp = getTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			setTemplateAreaDirty(i);
			temp->scaleFromOrigin(factor);
			setTemplateAreaDirty(i);
		}
		for (int i = 0; i < getNumClosedTemplates(); ++i)
		{
			Template* temp = getClosedTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			temp->scaleFromOrigin(factor);
		}
	}
	
	setScaleDenominator(new_scale_denominator);
	setOtherDirty(true);
}
void Map::rotateMap(double rotation, bool adjust_georeferencing, bool adjust_declination, bool adjust_templates)
{
	if (fmod(rotation, 360) == 0)
		return;
	
	object_undo_manager.clear(false);
	rotateAllObjects(rotation);
	
	if (adjust_georeferencing)
	{
		MapCoordF reference_point = MapCoordF(georeferencing->getMapRefPoint());
		reference_point.rotate(-rotation);
		georeferencing->setMapRefPoint(reference_point.toMapCoord());
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
			temp->rotateAroundOrigin(rotation);
			setTemplateAreaDirty(i);
		}
		for (int i = 0; i < getNumClosedTemplates(); ++i)
		{
			Template* temp = getClosedTemplate(i);
			if (temp->isTemplateGeoreferenced())
				continue;
			temp->rotateAroundOrigin(rotation);
		}
	}
	
	setOtherDirty(true);
}

bool Map::saveTo(const QString& path, MapEditorController* map_editor)
{
	assert(map_editor && "Preserving the widget&view information without retrieving it from a MapEditorController is not implemented yet!");
	
	const Format *format = FileFormats.findFormatForFilename(path);
	if (!format) format = FileFormats.findFormat(FileFormats.defaultFormat());
	
	if (!format || !format->supportsExport())
	{
		if (format)
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot export the map as\n\"%1\"\nbecause saving as %2 (.%3) is not supported.").arg(path).arg(format->description()).arg(format->fileExtension()));
		else
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot export the map as\n\"%1\"\nbecause the format is unknown.").arg(path));
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
	
	Exporter* exporter = NULL;
	try {
		// Create an exporter instance for this file and map.
		exporter = format->createExporter(&file, this, map_editor->main_view);
		
		// Run the first pass.
		exporter->doExport();
		
		file.close();
		
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
		QMessageBox::warning(NULL, tr("Error"), tr("Internal error while saving:\n%1").arg(e.what()));
		if (temp_path.isEmpty())
			QFile::remove(path);
		else
			QFile::remove(temp_path);
		return false;
	}
	if (exporter) delete exporter;
	
	if (!temp_path.isEmpty())
	{
		QFile::remove(path);
		QFile::rename(temp_path, path);
	}
	
	
	colors_dirty = false;
	symbols_dirty = false;
	templates_dirty = false;
	objects_dirty = false;
	other_dirty = false;
	unsaved_changes = false;
	
	objectUndoManager().notifyOfSave();
	
	return true;
}
bool Map::loadFrom(const QString& path, MapEditorController* map_editor, bool load_symbols_only, bool show_error_messages)
{
	MapView *view = new MapView(this);

	// Ensure the file exists and is readable.
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (show_error_messages)
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\nfor reading.").arg(path));
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
	Q_FOREACH(const Format *format, FileFormats.formats())
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
					QMessageBox msgBox(QMessageBox::Warning, tr("Warning"), tr("The map import generated warnings."), QMessageBox::Ok);
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
			QMessageBox::warning(NULL, tr("Error"), tr("Cannot open file:\n%1\n\n%2").arg(path).arg(error_msg));
		return false;
	}

	// Update all objects without trying to remove their renderables first, this gives a significant speedup when loading large files
	updateAllObjects(); // TODO: is the comment above still applicable?

	return true;
}

void Map::importMap(Map* other, ImportMode mode, QWidget* dialog_parent, std::vector<bool>* filter, int symbol_insert_pos,
					bool merge_duplicate_symbols, QHash<Symbol*, Symbol*>* out_symbol_map)
{
	// Check if there is something to import
	if (other->getNumColors() == 0)
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
			other->changeScale(getScaleDenominator(), true, true, true, true);
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
	QHash<MapColor*, MapColor*> color_map;
	color_set->importSet(other->color_set, this, &color_filter, NULL, &color_map);
	
	if (mode == ColorImport)
		return;
	
	if (other->getNumSymbols() > 0)
	{
		// Import symbols
		QHash<Symbol*, Symbol*> symbol_map;
		importSymbols(other, color_map, symbol_insert_pos, merge_duplicate_symbols, &symbol_filter, NULL, &symbol_map);
		if (out_symbol_map != NULL)
			*out_symbol_map = symbol_map;
		
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
}

bool Map::exportToNative(QIODevice* stream)
{
	stream->open(QIODevice::WriteOnly);
	Exporter* exporter = NULL;
	try {
		const Format* native_format = FileFormats.findFormat("native");
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

bool Map::importFromNative(QIODevice* stream)
{
	Importer* importer = NULL;
	try {
		const Format* native_format = FileFormats.findFormat("native");
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
	object_undo_manager.clear(true);
	
	map_notes = "";
	
	print_params_set = false;
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

void Map::draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool show_helper_symbols, float opacity)
{
	// Update the renderables of all objects marked as dirty
	updateObjects();
	
	// The actual drawing
	renderables->draw(painter, bounding_box, force_min_size, scaling, show_helper_symbols, opacity);
}
void Map::drawGrid(QPainter* painter, QRectF bounding_box)
{
	grid->draw(painter, bounding_box, this);
}
void Map::drawTemplates(QPainter* painter, QRectF bounding_box, int first_template, int last_template, MapView* view)
{
	for (int i = first_template; i <= last_template; ++i)
	{
		Template* temp = getTemplate(i);
		if ((view && !view->isTemplateVisible(temp)) || (temp->getTemplateState() != Template::Loaded))
			continue;
		float scale = (view ? view->getZoom() : 1) * std::max(temp->getTemplateScaleX(), temp->getTemplateScaleY());
		
		painter->save();
		temp->drawTemplate(painter, bounding_box, scale, view ? view->getTemplateVisibility(temp)->opacity : 1);
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
	AddObjectsUndoStep* undo_step = new AddObjectsUndoStep(this);
	MapPart* part = getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = selectedObjectsBegin(); it != it_end; ++it)
	{
		int index = part->findObjectIndex(*it);
		undo_step->addObject(index, *it);
	}
	for (Map::ObjectSelection::const_iterator it = selectedObjectsBegin(); it != it_end; ++it)
		deleteObject(*it, true);
	clearObjectSelection(true);
	objectUndoManager().addNewUndoStep(undo_step);
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
	replacement_renderables->draw(painter, view->calculateViewedRect(widget->viewportToView(widget->rect())), force_min_size, view->calculateFinalZoomFactor(), true, selection_opacity_factor, !draw_normal);
	
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
		widgets[i]->ensureVisibilityOfRect(rect);
}

/*void Map::addMapView(MapView* view)
{
	views.push_back(view);
}
void Map::removeMapView(MapView* view)
{
	for (int i = 0; i < (int)views.size(); ++i)
	{
		if (views[i] == view)
		{
			views.erase(views.begin() + i);
			return;
		}
	}
	assert(false);
}*/

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
	color_set->colors.erase(color_set->colors.begin() + pos);
	adjustColorPriorities(pos, color_set->colors.size() - 1);
	
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
int Map::findColorIndex(MapColor* color) const
{
	int size = (int)color_set->colors.size();
	for (int i = 0; i < size; ++i)
	{
		if (color_set->colors[i] == color)
			return i;
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
bool Map::isColorUsedByASymbol(MapColor* color)
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

void Map::importSymbols(Map* other, const QHash<MapColor*, MapColor*>& color_map, int insert_pos, bool merge_duplicates, std::vector< bool >* filter,
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
		object_undo_manager.clear(false);
	
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
int Map::findTemplateIndex(Template* temp)
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

void Map::addPart(MapPart* part, int pos)
{
	parts.insert(parts.begin() + pos, part);
	if (current_part_index >= pos)
		++current_part_index;
	setOtherDirty(true);
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

QRectF Map::calculateExtent(bool include_helper_symbols, bool include_templates, MapView* view)
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

void Map::scaleAllObjects(double factor)
{
	operationOnAllObjects(ObjectOp::Scale(factor));
}
void Map::rotateAllObjects(double rotation)
{
	operationOnAllObjects(ObjectOp::Rotate(rotation));
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
}

void Map::setPrintParameters(int orientation, int format, float dpi, bool show_templates, bool show_grid, bool center, float left, float top, float width, float height, bool different_scale_enabled, int different_scale)
{
	if ((print_orientation != orientation) || (print_format != format) || (print_dpi != dpi) || (print_show_templates != show_templates) ||
		(print_center != center) || (print_area_left != left) || (print_area_top != top) || (print_area_width != width) || (print_area_height != height))
		setHasUnsavedChanges();

	print_orientation = orientation;
	print_format = format;
	print_dpi = dpi;
	print_show_templates = show_templates;
	print_show_grid = show_grid;
	print_center = center;
	print_area_left = left;
	print_area_top = top;
	print_area_width = width;
	print_area_height = height;
	print_different_scale_enabled = different_scale_enabled;
	print_different_scale = different_scale;
	
	print_params_set = true;
}
void Map::getPrintParameters(int& orientation, int& format, float& dpi, bool& show_templates, bool& show_grid, bool& center, float& left, float& top, float& width, float& height, bool& different_scale_enabled, int& different_scale)
{
	orientation = print_orientation;
	format = print_format;
	dpi = print_dpi;
	show_templates = print_show_templates;
	show_grid = print_show_grid;
	center = print_center;
	left = print_area_left;
	top = print_area_top;
	width = print_area_width;
	height = print_area_height;
	different_scale_enabled = print_different_scale_enabled;
	different_scale = print_different_scale;
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
		if (!unsaved_changes)
		{
			emit(gotUnsavedChanges());
			unsaved_changes = true;
		}
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

// ### MapView ###

const double screen_pixel_per_mm = 4.999838577;	// TODO: make configurable (by specifying screen diameter in inch + resolution, or dpi)
// Calculation: pixel_height / ( sqrt((c^2)/(aspect^2 + 1)) * 2.54 )

const double MapView::zoom_in_limit = 512;
const double MapView::zoom_out_limit = 1 / 16.0;

MapView::MapView(Map* map) : map(map)
{
	zoom = 1;
	rotation = 0;
	position_x = 0;
	position_y = 0;
	view_x = 0;
	view_y = 0;
	map_visibility = new TemplateVisibility();
	map_visibility->visible = true;
	all_templates_hidden = false;
	grid_visible = false;
	update();
	//map->addMapView(this);
}
MapView::~MapView()
{
	//map->removeMapView(this);
	
	foreach (TemplateVisibility* vis, template_visibilities)
		delete vis;
	delete map_visibility;
}

void MapView::save(QIODevice* file)
{
	file->write((const char*)&zoom, sizeof(double));
	file->write((const char*)&rotation, sizeof(double));
	file->write((const char*)&position_x, sizeof(qint64));
	file->write((const char*)&position_y, sizeof(qint64));
	file->write((const char*)&view_x, sizeof(int));
	file->write((const char*)&view_y, sizeof(int));
	file->write((const char*)&drag_offset, sizeof(QPoint));
	
	file->write((const char*)&map_visibility->visible, sizeof(bool));
	file->write((const char*)&map_visibility->opacity, sizeof(float));
	
	int num_template_visibilities = template_visibilities.size();
	file->write((const char*)&num_template_visibilities, sizeof(int));
	QHash<Template*, TemplateVisibility*>::const_iterator it = template_visibilities.constBegin();
	while (it != template_visibilities.constEnd())
	{
		int pos = map->findTemplateIndex(it.key());
		file->write((const char*)&pos, sizeof(int));
		
		file->write((const char*)&it.value()->visible, sizeof(bool));
		file->write((const char*)&it.value()->opacity, sizeof(float));
		
		++it;
	}
	
	file->write((const char*)&all_templates_hidden, sizeof(bool));
	
	file->write((const char*)&grid_visible, sizeof(bool));
}

void MapView::load(QIODevice* file, int version)
{
	file->read((char*)&zoom, sizeof(double));
	file->read((char*)&rotation, sizeof(double));
	file->read((char*)&position_x, sizeof(qint64));
	file->read((char*)&position_y, sizeof(qint64));
	file->read((char*)&view_x, sizeof(int));
	file->read((char*)&view_y, sizeof(int));
	file->read((char*)&drag_offset, sizeof(QPoint));
	update();
	
	if (version >= 26)
	{
		file->read((char*)&map_visibility->visible, sizeof(bool));
		file->read((char*)&map_visibility->opacity, sizeof(float));
	}
	
	int num_template_visibilities;
	file->read((char*)&num_template_visibilities, sizeof(int));
	
	for (int i = 0; i < num_template_visibilities; ++i)
	{
		int pos;
		file->read((char*)&pos, sizeof(int));
		
		TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
		file->read((char*)&vis->visible, sizeof(bool));
		file->read((char*)&vis->opacity, sizeof(float));
	}
	
	if (version >= 29)
		file->read((char*)&all_templates_hidden, sizeof(bool));
	
	if (version >= 24)
		file->read((char*)&grid_visible, sizeof(bool));
}

void MapView::save(QXmlStreamWriter& xml)
{
	xml.writeStartElement("map_view");
	
	xml.writeAttribute("zoom", QString::number(zoom));
	xml.writeAttribute("rotation", QString::number(rotation));
	xml.writeAttribute("position_x", QString::number(position_x));
	xml.writeAttribute("position_y", QString::number(position_y));
	xml.writeAttribute("view_x", QString::number(view_x));
	xml.writeAttribute("view_y", QString::number(view_y));
	xml.writeAttribute("drag_offset_x", QString::number(drag_offset.x()));
	xml.writeAttribute("drag_offset_y", QString::number(drag_offset.y()));
	if (grid_visible)
		xml.writeAttribute("grid", "true");
	
	xml.writeEmptyElement("map");
	if (!map_visibility->visible)
		xml.writeAttribute("visible", "false");
	xml.writeAttribute("opacity", QString::number(map_visibility->opacity));
	
	xml.writeStartElement("templates");
	int num_template_visibilities = template_visibilities.size();
	xml.writeAttribute("count", QString::number(num_template_visibilities));
	if (all_templates_hidden)
		xml.writeAttribute("hidden", "true");
	QHash<Template*, TemplateVisibility*>::const_iterator it = template_visibilities.constBegin();
	for ( ; it != template_visibilities.constEnd(); ++it)
	{
		xml.writeEmptyElement("ref");
		int pos = map->findTemplateIndex(it.key());
		xml.writeAttribute("template", QString::number(pos));
		xml.writeAttribute("visible", (*it)->visible ? "true" : "false");
		xml.writeAttribute("opacity", QString::number((*it)->opacity));
	}
	xml.writeEndElement(/*templates*/);
	
	xml.writeEndElement(/*map_view*/); 
}

void MapView::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == "map_view");
	
	{
		// keep variable "attributes" local to this block
		QXmlStreamAttributes attributes = xml.attributes();
		zoom = attributes.value("zoom").toString().toDouble();
		if (zoom < 0.001)
			zoom = 1.0;
		rotation = attributes.value("rotation").toString().toDouble();
		position_x = attributes.value("position_x").toString().toLongLong();
		position_y = attributes.value("position_y").toString().toLongLong();
		view_x = attributes.value("view_x").toString().toInt();
		view_y = attributes.value("view_y").toString().toInt();
		drag_offset.setX(attributes.value("drag_offset_x").toString().toInt());
		drag_offset.setY(attributes.value("drag_offset_y").toString().toInt());
		grid_visible = (attributes.value("grid") == "true");
		update();
	}
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "map")
		{
			map_visibility->visible = !(xml.attributes().value("visible") == "false");
			map_visibility->opacity = xml.attributes().value("opacity").toString().toFloat();
			xml.skipCurrentElement();
		}
		else if (xml.name() == "templates")
		{
			int num_template_visibilities = xml.attributes().value("count").toString().toInt();
			template_visibilities.reserve(qMin(num_template_visibilities, 20)); // 20 is not a limit
			all_templates_hidden = (xml.attributes().value("hidden") == "true");
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == "ref")
				{
					int pos = xml.attributes().value("template").toString().toInt();
					if (pos >= 0 && pos < map->getNumTemplates())
					{
						TemplateVisibility* vis = getTemplateVisibility(map->getTemplate(pos));
						vis->visible = !(xml.attributes().value("visible") == "false");
						vis->opacity = xml.attributes().value("opacity").toString().toFloat();
					}
					else
						qDebug() << QString("Invalid template visibility reference %1 at %2:%3").
						            arg(pos).arg(xml.lineNumber()).arg(xml.columnNumber());
				}
				xml.skipCurrentElement();
			}
		}
		else
			xml.skipCurrentElement(); // unsupported
	}
}

void MapView::addMapWidget(MapWidget* widget)
{
	widgets.push_back(widget);
	
	map->addMapWidget(widget);
}
void MapView::removeMapWidget(MapWidget* widget)
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
	
	map->removeMapWidget(widget);
}
void MapView::updateAllMapWidgets()
{
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateEverything();
}

double MapView::lengthToPixel(qint64 length)
{
	return zoom * screen_pixel_per_mm * (length / 1000.0);
}
qint64 MapView::pixelToLength(double pixel)
{
	return qRound64(1000 * pixel / (zoom * screen_pixel_per_mm));
}

QRectF MapView::calculateViewedRect(QRectF view_rect)
{
	QPointF min = view_rect.topLeft();
	QPointF max = view_rect.bottomRight();
	MapCoordF top_left = viewToMapF(min.x(), min.y());
	MapCoordF top_right = viewToMapF(max.x(), min.y());
	MapCoordF bottom_right = viewToMapF(max.x(), max.y());
	MapCoordF bottom_left = viewToMapF(min.x(), max.y());
	
	QRectF result = QRectF(top_left.getX(), top_left.getY(), 0, 0);
	rectInclude(result, QPointF(top_right.getX(), top_right.getY()));
	rectInclude(result, QPointF(bottom_right.getX(), bottom_right.getY()));
	rectInclude(result, QPointF(bottom_left.getX(), bottom_left.getY()));
	
	return QRectF(result.left() - 0.001, result.top() - 0.001, result.width() + 0.002, result.height() + 0.002);
}
QRectF MapView::calculateViewBoundingBox(QRectF map_rect)
{
	QPointF min = map_rect.topLeft();
	MapCoord map_min = MapCoord(min.x(), min.y());
	QPointF max = map_rect.bottomRight();
	MapCoord map_max = MapCoord(max.x(), max.y());
	
	QPointF top_left;
	mapToView(map_min, top_left.rx(), top_left.ry());
	QPointF bottom_right;
	mapToView(map_max, bottom_right.rx(), bottom_right.ry());
	
	MapCoord map_top_right = MapCoord(max.x(), min.y());
	QPointF top_right;
	mapToView(map_top_right, top_right.rx(), top_right.ry());
	
	MapCoord map_bottom_left = MapCoord(min.x(), max.y());
	QPointF bottom_left;
	mapToView(map_bottom_left, bottom_left.rx(), bottom_left.ry());
	
	QRectF result = QRectF(top_left.x(), top_left.y(), 0, 0);
	rectInclude(result, QPointF(top_right.x(), top_right.y()));
	rectInclude(result, QPointF(bottom_right.x(), bottom_right.y()));
	rectInclude(result, QPointF(bottom_left.x(), bottom_left.y()));
	
	return QRectF(result.left() - 1, result.top() - 1, result.width() + 2, result.height() + 2);
}

void MapView::applyTransform(QPainter* painter)
{
	QTransform world_transform;
	// NOTE: transposing the matrix here ...
	world_transform.setMatrix(map_to_view.get(0, 0), map_to_view.get(1, 0), map_to_view.get(2, 0),
							  map_to_view.get(0, 1), map_to_view.get(1, 1), map_to_view.get(2, 1),
							  map_to_view.get(0, 2), map_to_view.get(1, 2), map_to_view.get(2, 2));
	painter->setWorldTransform(world_transform, true);
}

void MapView::setDragOffset(QPoint offset)
{
	drag_offset = offset;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->setDragOffset(drag_offset);
}
void MapView::completeDragging(QPoint offset)
{
	drag_offset = QPoint(0, 0);
	qint64 move_x = -pixelToLength(offset.x());
	qint64 move_y = -pixelToLength(offset.y());
	
	position_x += move_x;
	position_y += move_y;
	update();
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->completeDragging(offset, move_x, move_y);
}

bool MapView::zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view)
{
	num_steps = 0.5f * num_steps;
	
	if (num_steps > 0)
	{
		// Zooming in - adjust camera position so the cursor stays at the same position on the map
		if (getZoom() >= zoom_in_limit)
			return false;
		
		bool set_to_limit = false;
		double zoom_to = pow(2, (log10(getZoom()) / LOG2) + num_steps);
		double zoom_factor = zoom_to / getZoom();
		if (getZoom() * zoom_factor > zoom_in_limit)
		{
			zoom_factor = zoom_in_limit / getZoom();
			set_to_limit = true;
		}
		
		MapCoordF mouse_pos_map(0, 0);
		MapCoordF mouse_pos_to_view_center(0, 0);
		if (preserve_cursor_pos)
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			mouse_pos_to_view_center = MapCoordF(getPositionX()/1000.0 - mouse_pos_map.getX(), getPositionY()/1000.0 - mouse_pos_map.getY());
			mouse_pos_to_view_center = MapCoordF(mouse_pos_to_view_center.getX() * 1 / zoom_factor, mouse_pos_to_view_center.getY() * 1 / zoom_factor);
		}
		
		setZoom(set_to_limit ? zoom_in_limit : (getZoom() * zoom_factor));
		if (preserve_cursor_pos)
		{
			setPositionX(qRound64(1000 * (mouse_pos_map.getX() + mouse_pos_to_view_center.getX())));
			setPositionY(qRound64(1000 * (mouse_pos_map.getY() + mouse_pos_to_view_center.getY())));
		}
	}
	else
	{
		// Zooming out
		if (getZoom() <= zoom_out_limit)
			return false;
		
		bool set_to_limit = false;
		double zoom_to = pow(2, (log10(getZoom()) / LOG2) + num_steps);
		double zoom_factor = zoom_to / getZoom();
		if (getZoom() * zoom_factor < zoom_out_limit)
		{
			zoom_factor = zoom_out_limit / getZoom();
			set_to_limit = true;
		}
		
		MapCoordF mouse_pos_map(0, 0);
		MapCoordF mouse_pos_to_view_center(0, 0);
		if (preserve_cursor_pos)
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			mouse_pos_to_view_center = MapCoordF(getPositionX()/1000.0 - mouse_pos_map.getX(), getPositionY()/1000.0 - mouse_pos_map.getY());
			mouse_pos_to_view_center = MapCoordF(mouse_pos_to_view_center.getX() * 1 / zoom_factor, mouse_pos_to_view_center.getY() * 1 / zoom_factor);
		}
		
		setZoom(set_to_limit ? zoom_out_limit : (getZoom() * zoom_factor));
		
		if (preserve_cursor_pos)
		{
			setPositionX(qRound64(1000 * (mouse_pos_map.getX() + mouse_pos_to_view_center.getX())));
			setPositionY(qRound64(1000 * (mouse_pos_map.getY() + mouse_pos_to_view_center.getY())));
		}
		else
		{
			mouse_pos_map = viewToMapF(cursor_pos_view);
			for (int i = 0; i < (int)widgets.size(); ++i)
				widgets[i]->updateCursorposLabel(mouse_pos_map);
		}
	}
	return true;
}

void MapView::setZoom(float value)
{
	float zoom_factor = value / zoom;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->zoom(zoom_factor);
	
	zoom = value;
	update();
	
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->updateZoomLabel();
}
void MapView::setPositionX(qint64 value)
{
	qint64 offset = value - position_x;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(offset, 0);
	
	position_x = value;
	update();
}
void MapView::setPositionY(qint64 value)
{
	qint64 offset = value - position_y;
	for (int i = 0; i < (int)widgets.size(); ++i)
		widgets[i]->moveView(0, offset);
	
	position_y = value;
	update();
}

void MapView::update()
{
	double cosr = cos(rotation);
	double sinr = sin(rotation);
	
	double final_zoom = lengthToPixel(1000);
	
	// Create map_to_view
	map_to_view.setSize(3, 3);
	map_to_view.set(0, 0, final_zoom * cosr);
	map_to_view.set(0, 1, final_zoom * (-sinr));
	map_to_view.set(1, 0, final_zoom * sinr);
	map_to_view.set(1, 1, final_zoom * cosr);
	map_to_view.set(0, 2, -final_zoom*(position_x/1000.0)*cosr + final_zoom*(position_y/1000.0)*sinr - view_x);
	map_to_view.set(1, 2, -final_zoom*(position_x/1000.0)*sinr - final_zoom*(position_y/1000.0)*cosr - view_y);
	map_to_view.set(2, 0, 0);
	map_to_view.set(2, 1, 0);
	map_to_view.set(2, 2, 1);
	
	// Create view_to_map
	map_to_view.invert(view_to_map);
}

TemplateVisibility *MapView::getMapVisibility()
{
	return map_visibility;
}

bool MapView::isTemplateVisible(Template* temp)
{
	if (template_visibilities.contains(temp))
	{
		TemplateVisibility* vis = template_visibilities.value(temp);
		return vis->visible && vis->opacity > 0;
	}
	else
		return false;
}

TemplateVisibility* MapView::getTemplateVisibility(Template* temp)
{
	if (!template_visibilities.contains(temp))
	{
		TemplateVisibility* vis = new TemplateVisibility();
		template_visibilities.insert(temp, vis);
		return vis;
	}
	else
		return template_visibilities.value(temp);
}

void MapView::deleteTemplateVisibility(Template* temp)
{
	delete template_visibilities.value(temp);
	template_visibilities.remove(temp);
}

void MapView::setHideAllTemplates(bool value)
{
	all_templates_hidden = value;
	updateAllMapWidgets();
}
