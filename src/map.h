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


#ifndef _OPENORIENTEERING_MAP_H_
#define _OPENORIENTEERING_MAP_H_

#include <vector>

#include <QString>
#include <QRect>
#include <QHash>
#include <QSet>
#include <QScopedPointer>
#include <QExplicitlySharedDataPointer>

#include "global.h"
#include "undo.h"
#include "map_coord.h"
#include "map_part.h"

QT_BEGIN_NAMESPACE
class QIODevice;
class QPainter;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Map;
class MapColor;
class MapWidget;
class MapView;
class MapEditorController;
class MapPrinterConfig;
class Symbol;
class CombinedSymbol;
class LineSymbol;
class PointSymbol;
class Object;
class Renderable;
class MapRenderables;
class Template;
class OCAD8FileImport;
class Georeferencing;
class MapGrid;

/** Central class for an OpenOrienteering map */
class Map : public QObject
{
Q_OBJECT
friend class MapRenderables;
friend class OCAD8FileImport;
friend class XMLFileImport;
friend class NativeFileImport;
friend class NativeFileExport;
friend class XMLFileImporter;
friend class XMLFileExporter;
public:
	/** A set of selected objects represented by a QSet of object pointers. */
	typedef QSet<Object*> ObjectSelection;
	
	/** Different strategies for importing things from another map into this map. */
	enum ImportMode
	{
		/**
		 * Imports all objects with minimal symbol and color dependencies.
		 */
		MinimalObjectImport = 0,
		
		/**
		 * Imports symbols (all, or filtered by the given filter) with minimal
		 * color dependencies. If a filter is given, symbols blocked by
		 * the filter but which are needed by included symbols are included, too.
		 */
		MinimalSymbolImport,
		
		/**
		 * Imports objects (all, or filtered by the given filter).
		 */
		ColorImport,
		
		/**
		 * Imports all colors, symbols and objects.
		 */
		CompleteImport
	};
	
	/** Creates a new, empty map. */
	Map();
	
	/** Destroys the map. */
	~Map();
	
	/**
	 * Attempts to save the map to the given file.
	 * If a MapEditorController is given, the widget positions and MapViews
	 * stored in the map file are also updated.
	 */
	bool saveTo(const QString& path, MapEditorController* map_editor = NULL);
	
	/**
	 * Attempts to save the map to the given file.
	 * If a MapEditorController is given, the widget positions and MapViews
	 * stored in the map file are also updated.
	 * If a FileFormat is given, it will be used, otherwise the file format
	 * is determined from the filename.
	 * If the map was modified, it will still be considered modified after
	 * successfully saving a copy.
	 */
	bool exportTo(const QString& path, MapEditorController* map_editor = NULL, const FileFormat* format = NULL);
	
	/**
	 * Attempts to load the map from the specified path. Returns true on success.
	 * 
	 * @param path The file path to load the map from.
	 * @param dialog_parent The parent widget for all dialogs.
	 *     This should never be NULL in a QWidgets application.
	 * @param map_editor If not NULL, restores that map editor controller.
	 *     Restores widget positions and MapViews.
	 * @param load_symbols_only Loads only symbols from the chosen file.
	 *     Useful to load symbol sets.
	 * @param show_error_messages Whether to show import errors and warnings.
	 */
	bool loadFrom(const QString& path,
	              QWidget* dialog_parent,
	              MapEditorController* map_editor = NULL,
	              bool load_symbols_only = false, bool show_error_messages = true);
	
	/**
	 * Imports the other map into this map with the following strategy:
	 *  - if the other map contains objects, import all objects with the minimum
	 *    amount of colors and symbols needed to display them
	 *  - if the other map does not contain objects, import all symbols with
	 *    the minimum amount of colors needed to display them
	 *  - if the other map does neither contain objects nor symbols, import all colors
	 * 
	 * WARNING: this method potentially changes the 'other' map if the
	 *          scales differ (by rescaling to fit this map's scale)!
	 */
	void importMap(Map* other, ImportMode mode, QWidget* dialog_parent = NULL, std::vector<bool>* filter = NULL, int symbol_insert_pos = -1,
				   bool merge_duplicate_symbols = true, QHash<Symbol*, Symbol*>* out_symbol_map = NULL);
	
	/**
	 * Serializes the map directly into the given IO device in a known format.
	 * This can be imported again using importFromIODevice().
	 * Returns true if successful.
	 */
	bool exportToIODevice(QIODevice* stream);
	
	/**
	 * Loads the map directly from the given IO device,
	 * where the data must have been written by exportToIODevice().
	 * Returns true if successful.
	 */
	bool importFromIODevice(QIODevice* stream);
	
	/**
	 * Deletes all map data and resets the map to its initial state
	 * containing one default map part.
	 */
	void clear();
	
	/**
	 * Draws the part of the map which is visible in the bounding box.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param bounding_box Bounding box of area to draw, given in map coordinates.
	 * @param force_min_size Forces a minimum size of 1 pixel for objects. Makes
	 *     maps look better at small zoom levels without antialiasing.
	 * @param scaling The used scaling factor. Used to calculate the final object
	 *     sizes when force_min_size is set.
	 * @param on_screen Set to true if drawing the map on screen. Potentially
	 *     enables optimizations which result in slightly lower
	 *     display quality (e.g. disable antialiasing for texts).
	 * @param show_helper_symbols Set this if symbols with the "helper symbol"
	 *     flag should be shown.
	 * @param opacity Opacity to draw the map with.
	 */
	void draw(QPainter* painter, QRectF bounding_box,
		bool force_min_size, float scaling, bool on_screen,
		bool show_helper_symbols, float opacity = 1.0);
	
	/**
	 * Draws a spot color overprinting simulation for the part of the map
	 * which is visible in the given bounding box.
	 * 
	 * See draw() for an explanation of the parameters.
	 * 
	 * @param painter Must be a QPainter on a QImage of Format_ARGB32_Premultiplied.
	 */
	void drawOverprintingSimulation(QPainter* painter, QRectF bounding_box,
		bool force_min_size, float scaling, bool on_screen,
		bool show_helper_symbols);
	
	/**
	 * Draws the separation for a particular spot color for the part of the
	 * map which is visible in the given bounding box.
	 * 
	 * Separations are normally drawn in levels of gray where black means
	 * full tone of the spot color. The parameter use_color can be used to
	 * draw in the actual spot color instead.
	 *
	 * See draw() for an explanation of the remaining parameters.
	 * 
	 * @param spot_color The spot color to draw the separation for.
	 * @param use_color  If true, forces the separation to be drawn in its actual color.
	 */
	void drawColorSeparation(QPainter* painter, QRectF bounding_box,
		bool force_min_size, float scaling, bool on_screen,
		bool show_helper_symbols,
		const MapColor* spot_color, bool use_color = false);
	
	/**
	 * Draws the map grid.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param bounding_box Bounding box of area to draw, given in map coordinates.
	 * @param on_screen If true, uses a cosmetic pen (one pixel wide),
	 *                  otherwise uses a 0.1 mm wide pen.
	 */
	void drawGrid(QPainter* painter, QRectF bounding_box, bool on_screen);
	
	/**
	 * Draws the templates with indices first_template until last_template which
	 * are visible in the given bouding box.
	 * 
	 * view determines template visibility and can be NULL to show all templates.
	 * The initial transform of the given QPainter must be the map-to-paintdevice transformation.
     * If on_screen is set to true, some optimizations will be applied, leading to a possibly lower display quality.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param bounding_box Bounding box of area to draw, given in map coordinates.
	 * @param first_template Lowest index of the template range to draw.
	 * @param last_template Highest index of the template range to draw.
	 * @param view Optional pointer to MapView object which is used to query
	 *     template visibilities.
	 * @param on_screen Potentially enables some drawing optimizations which
	 *     decrease drawing quality. Should be enabled when drawing on-screen.
	 */
	void drawTemplates(QPainter* painter, QRectF bounding_box, int first_template,
					   int last_template, MapView* view, bool on_screen);
	
	/**
	 * Updates the renderables and extent of all objects which have changed.
	 * This is automatically called by draw(), you normally do not need to call it directly.
	 */
	void updateObjects();
	
	/** 
	 * Calculates the extent of all map elements. 
	 * 
	 * If templates shall be included, view may either be NULL to include all 
	 * templates, or specify a MapView to take the template visibilities from.
	 */
	QRectF calculateExtent(bool include_helper_symbols = false, bool include_templates = false, const MapView* view = NULL) const;
	
	/**
	 * Must be called to notify the map of new widgets displaying it.
	 * Useful to notify the widgets about which parts of the map have changed
	 * and need to be redrawn.
	 */
	void addMapWidget(MapWidget* widget);
	
	/**
	 * Removes the map widget, see addMapWidget().
	 */
	void removeMapWidget(MapWidget* widget);
	
	/**
	 * Redraws all map widgets completely - this can be slow!
	 * Try to avoid this and do partial redraws instead, if possible.
	 */
	void updateAllMapWidgets();
	
	/**
	 * Makes sure that the selected object(s) are visible in all map widgets
	 * by moving the views in the widgets to the selected objects.
	 */
	void ensureVisibilityOfSelectedObjects();
	
	
	// Current drawing
	
	/**
	 * Sets the rect (given in map coordinates) as "dirty rect" for every
	 * map widget showing this map, enlarged by the given pixel border.
	 * This means that the area covered by the rect will be redrawn by
	 * the active tool. Use this if the current tool's display has changed.
	 * 
	 * @param map_coords_rect Area covered by the current tool's drawing in map coords.
	 * @param pixel_border Border around the map coords rect which is also covered,
	 *     given in pixels. Allows to enlarge the area given by the map coords
	 *     by some pixels which are independent from the zoom level.
	 *     For example if a tool displays markers with a radius of 3 pixels,
	 *     it would set the bounding box of all markers as map_coords_rect and
	 *     3 for pixel_border.
	 * @param do_update Whether a repaint of the covered area should be triggered.
	 */
	void setDrawingBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	
	/**
	 * Removes the drawing bounding box and triggers a repaint. Use this if
	 * the current drawing is hidden or erased.
	 */
	void clearDrawingBoundingBox();
	
	/**
	 * This is the analogon to setDrawingBoundingBox() for activities.
	 * See setDrawingBoundingBox().
	 */
	void setActivityBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	
	/**
	 * This is the analogon to clearDrawingBoundingBox() for activities.
	 * See clearDrawingBoundingBox().
	 */
	void clearActivityBoundingBox();
	
	/**
	 * Updates all dynamic drawings at the given positions,
	 * i.e. tool & activity drawings.
	 * 
	 * See setDrawingBoundingBox() and setActivityBoundingBox().
	 */
	void updateDrawing(QRectF map_coords_rect, int pixel_border);
	
	
	// Colors
	
	/** Returns the number of map colors defined in this map. */
	inline int getNumColors() const {return (int)color_set->colors.size();}
	
	/** Returns a pointer to the MapColor identified by the non-negative priority i.
	 *  Returns NULL if the color is not defined, or if it is a special color (i.e i<0).
	 *  Note: you can get a pointer to a _const_ special color from a
	 *  non-const map by const_casting the map:
	 * 
	 *    const MapColor* registration = const_cast<const Map&>(map).getColor(i);
	 */
	MapColor* getColor(int i);
	
	/** Returns a pointer to the const MapColor identified by the priority i.
	 *  Parameter i may also be negative (for special reserved colors).
	 *  Returns NULL if the color is not defined. */
	const MapColor* getColor(int i) const;
	
	/**
	 * Replaces the color at index pos with the given color, updates dependent
	 * colors and symbol icons.
	 * Emits colorChanged().
	 */
	void setColor(MapColor* color, int pos);
	
	/**
	 * Adds and returns a pointer to a new color at the given index.
	 * Emits colorAdded().
	 * TODO: obsolete, remove and use the other overload.
	 */
	MapColor* addColor(int pos);
	
	/**
	 * Adds the given color as a new color at the given index.
	 * Emits colorAdded().
	 */
	void addColor(MapColor* color, int pos);
	
	/**
	 * Deletes the color at the given index.
	 * Emits colorDeleted().
	 */
	void deleteColor(int pos);
	
	/**
	 * Loops through the color list, looking for the given color pointer.
	 * Returns the index of the color, or -1 if it is not found.
	 */
	int findColorIndex(const MapColor* color) const;
	
	/**
	 * Marks the colors as "dirty", i.e. as having unsaved changes.
	 * Emits gotUnsavedChanges() if the map did not have unsaved changed before.
	 */
	void setColorsDirty();
	
	/**
	 * Makes this map use the color set from the given map.
	 * Used to create the maps containing preview objects in the symbol editor.
	 */
	void useColorsFrom(Map* map);
	
	/**
	 * Checks and returns if the given color is used by at least one symbol.
	 */
	bool isColorUsedByASymbol(const MapColor* color) const;
	
	/**
	 * Checks which colors are in use by the symbols in this map.
	 * 
	 * WARNING (FIXME): returns an empty list if the map does not contain symbols!
	 * 
	 * @param by_which_symbols Must be of the same size as the symbol set.
	 *     If set to false for a symbol, it will be disregarded.
	 * @param out Output parameter: a vector of the same size as the color list,
	 *     where each element is set to true if the color is used by at least one symbol.
	 */
	void determineColorsInUse(const std::vector< bool >& by_which_symbols, std::vector< bool >& out);
	
	/** Returns true if the map contains spot colors. */
	bool hasSpotColors() const;
	
	// Symbols
	
	/** Returns the number of symbols in this map. */
	inline int getNumSymbols() const {return (int)symbols.size();}
	
	/** Returns a pointer to the i-th symbol. */
	Symbol* getSymbol(int i) const;
	
	/**
	 * Replaces the symbol at the given index with another symbol.
	 * Emits symbolChanged() and possibly selectedObjectEdited().
	 */
	void setSymbol(Symbol* symbol, int pos);
	
	/**
	 * Adds the given symbol at the specified index.
	 * Emits symbolAdded().
	 */
	void addSymbol(Symbol* symbol, int pos);
	
	/**
	 * Moves a symbol from one index to another in the symbol list.
	 */
	void moveSymbol(int from, int to);
	
	/**
	 * Sorts the symbol list using the given comparator.
	 */
	template<typename T> void sortSymbols(T compare)
	{
		std::stable_sort(symbols.begin(), symbols.end(), compare);
		// TODO: emit(symbolChanged(pos, symbol)); ? s/b same choice as for moveSymbol()
		setSymbolsDirty();
	}
	
	/**
	 * Deletes the symbol at the given index.
	 * Emits symbolDeleted().
	 */
	void deleteSymbol(int pos);
	
	/**
	 * Loops over all symbols, looking for the given symbol pointer.
	 * Returns the index of the symbol, or -1 if the symbol is not found.
	 * For the "undefined" symbols, returns special indices smaller than -1.
	 */
	int findSymbolIndex(const Symbol* symbol) const;
	
	/**
	 * Marks the symbols as "dirty", i.e. as having unsaved changes.
	 * Emits gotUnsavedChanges() if the map did not have unsaved changed before.
	 */
	void setSymbolsDirty();
	
	/**
	 * Scales all symbols by the given factor.
	 */
	void scaleAllSymbols(double factor);
	
	/**
	 * Checks which symbols are in use in this map.
	 * 
	 * Returns a vector of the same size as the symbol list, where each element
	 * is set to true if there is at least one object which uses this symbol or
	 * a derived (combined) symbol.
	 */
	void determineSymbolsInUse(std::vector<bool>& out);
	
	/**
	 * Adds to the given symbol bitfield all other symbols which are needed to
	 * display the symbols indicated by the bitfield because of symbol dependencies.
	 */
	void determineSymbolUseClosure(std::vector< bool >& symbol_bitfield);
	
	
	// Templates
	
	/** Returns the number of templates in this map. */
	inline int getNumTemplates() const {return templates.size();}
	
	/** Returns the i-th template. */
	inline Template* getTemplate(int i) {return templates[i];}
	
	/** Sets the template index which is the first (lowest) to be drawn in front of the map. */
	inline void setFirstFrontTemplate(int pos) {first_front_template = pos;}
	
	/** Returns the template index which is the first (lowest) to be drawn in front of the map. */
	inline int getFirstFrontTemplate() const {return first_front_template;}
	
	/**
	 * Replaces the template at the given index with another.
	 * Emits templateChanged().
	 */
	void setTemplate(Template* temp, int pos);
	
	/**
	 * Adds a new template at the given index.
	 * If a view is given, the template is made visible in this view
	 * (by default, templates are hidden).
	 * NOTE: if required, adjust first_front_template manually with setFirstFrontTemplate()!
	 */
	void addTemplate(Template* temp, int pos, MapView* view = NULL);
	
	/**
	 * Removes the template with the given index from the template list,
	 * but does not delete it.
	 * NOTE: if required, adjust first_front_template manually with setFirstFrontTemplate()!
	 */
	void removeTemplate(int pos);
	
	/**
	 * Removes the template with the given position from the template list and deletes it.
	 * NOTE: if required, adjust first_front_template manually with setFirstFrontTemplate()!
	 */
	void deleteTemplate(int pos);
	
	/**
	 * Marks the area defined by the given QRectF (in map coordinates) and
	 * pixel border as "dirty", i.e. as needing a repaint, for the given template
	 * in all map widgets.
	 * 
	 * For an explanation of the area and pixel border, see setDrawingBoundingBox().
	 * 
	 * Warning: does nothing if the template is not visible in a widget!
	 * So make sure to call this and showing/hiding a template in the correct order!
	 */
	void setTemplateAreaDirty(Template* temp, QRectF area, int pixel_border);
	
	/**
	 * Marks the whole area of the i-th template as "to be repainted".
	 * See setTemplateAreaDirty().
	 * Does nothing for i == -1.
	 */
	void setTemplateAreaDirty(int i);
	
	/**
	 * Loops over all templates in the map and looks for the given template pointer.
	 * Returns the index of the template. The template must be contained in the map,
	 * otherwise an assert will be triggered!
	 */
	int findTemplateIndex(const Template* temp) const;
	
	/**
	 * Marks the template settings as "dirty", i.e. as having unsaved changes.
	 * Emits gotUnsavedChanges() if the map did not have unsaved changed before.
	 */
	void setTemplatesDirty();
	
	/** Emits templateChanged() for the given template. */
	void emitTemplateChanged(Template* temp);
	
	
	/**
	 * Returns the number of manually closed templates
	 * for which the settings are still stored.
	 */
	inline int getNumClosedTemplates() {return (int)closed_templates.size();}
	
	/** Returns the i-th closed template. */
	inline Template* getClosedTemplate(int i) {return closed_templates[i];}
	
	/** Empties the list of closed templates. */
	void clearClosedTemplates();
	
	/**
	 * Removes the template with the given index from the normal template list,
	 * unloads the template file and adds the template to the closed template list
	 * 
	 * NOTE: if required, adjust first_front_template manually with setFirstFrontTemplate()!
	 */
	void closeTemplate(int i);
	
	/**
	 * Removes the template with the given index from the closed template list,
	 * load the template file and adds the template to the normal template list again.
	 * The template is made visible in the given view.
	 * 
	 * NOTE: if required, adjust first_front_template manually with
	 * setFirstFrontTemplate() before calling this method!
	 * 
	 * @param i The index of the closed template to reload.
	 * @param target_pos The desired index in the normal template list after loading.
	 * @param dialog_parent Widget as parent for possible dialogs.
	 * @param view Optional view in which the template will be set to visible.
	 * @param map_path Path where the map is saved currently. Used as possible
	 *     search location to locate missing templates.
	 */
	bool reloadClosedTemplate(int i, int target_pos, QWidget* dialog_parent,
							  MapView* view, const QString& map_path = QString());
	
	
	// Map parts & Undo
	
	/** Returns the UndoManager instance for this map. */
	inline UndoManager& objectUndoManager() {return object_undo_manager;}
	
	/** Returns the number of map parts in this map. */
	inline int getNumParts() const {return (int)parts.size();}
	
	/** Returns the i-th map part. */
	inline MapPart* getPart(int i) const {return parts[i];}
	
	/** Adds the new part at the given index. */
	void addPart(MapPart* part, int pos);

	/** Removes the map part at position */
	void removePart(int index);
	
	/**
	 * Loops over all map parts, looking for the given part pointer.
	 * Returns the part's index in the list. The part must be contained in the
	 * map, otherwise an assert will be triggered!
	 */
	int findPartIndex(MapPart* part) const;
	
	/** Returns the current map part, i.e. the part where edit operations happen. */
	inline MapPart* getCurrentPart() const {return (current_part_index < 0) ? NULL : parts[current_part_index];}
	
	/** Changes the current map part. */
	inline void setCurrentPart(MapPart* part) {setCurrentPart(findPartIndex(part));}
	void setCurrentPart(int index);

	/** Returns the index of the current map part, see also getCurrentPart(). */
	inline int getCurrentPartIndex() const {return current_part_index;}

	/** Moves all specified objects to the specified map part */
	void reassignObjectsToMapPart(QSet<Object*>::const_iterator begin, QSet<Object*>::const_iterator end, int destination);
	
	/** Merges the source layer with the destination layer, and deletes the source layer */
	void mergeParts(int source, int destination);
	
	// Objects
	
	/** Returns the total number of objects in this map (sum of all parts) */
	int getNumObjects();
	
	/**
	 * Adds the object as new object in the part with the given index,
	 * or in the current part if the default -1 is passed.
	 * Returns the index of the added object in the part.
	 */
	int addObject(Object* object, int part_index = -1);
	
	/**
	 * Deletes the given object from the map.
	 * remove_only will remove the object from the map, but not call "delete object";
	 * be sure to call removeObjectFromSelection() if necessary.
	 * 
	 * TODO: make a separate method "removeObject()", remove_only is misleading!
	 */
	void deleteObject(Object* object, bool remove_only);
	
	/**
	 * Marks the objects as "dirty", i.e. as having unsaved changes.
	 * Emits gotUnsavedChanges() if the map did not have unsaved changed before.
	 */
	void setObjectsDirty();
	
	/**
	 * Marks the area given by map_coords_rect as "dirty" in all map widgets,
	 * i.e. as needing to be redrawn because some object(s) changed there.
	 */
	void setObjectAreaDirty(QRectF map_coords_rect);
	
	/**
	 * Finds and returns all objects at the given position.
	 * 
	 * @param coord Coordinate where to query objects.
	 * @param tolerance Allowed distance from the query coordinate to the objects.
	 * @param treat_areas_as_paths If set to true, areas will be treated as paths,
	 *     i.e. they will be returned only if the query coordinate is close to
	 *     their border, not in all cases where the query coordinate is inside the area.
	 * @param extended_selection If set to true, more object than defined by the
	 *     default behavior will be selected. For example, by default point objects
	 *     will only be returned if the query coord is close to the point object
	 *     coord. With extended_selection, they are also returned if the query
	 *     coord is just inside the graphical extent of the point object.
	 *     This can be used for a two-pass algorithm to find the most relevant
	 *     objects first, and then query for all objects if no objects are found
	 *     in the first pass.
	 * @param include_hidden_objects Set to true if you want to find hidden objects.
	 * @param include_protected_objects Set to true if you want to find protected objects.
	 * @param out Output parameter. Will be filled with pairs of symbol types
	 *     and corresponding objects. The symbol type describes the way in which
	 *     an object has been found and is taken from Symbol::Type. This is e.g.
	 *     important for combined symbols, which can be found from a line or
	 *     an area.
	 */
	void findObjectsAt(MapCoordF coord, float tolerance, bool treat_areas_as_paths,
		bool extended_selection, bool include_hidden_objects,
		bool include_protected_objects, SelectionInfoVector& out);
	
	/**
	 * Finds and returns all objects at the given box, i.e. objects that
	 * intersect the box.
	 * 
	 * @param corner1 First corner of the query box.
	 * @param corner2 Second corner of the query box.
	 * @param include_hidden_objects Set to true if you want to find hidden objects.
	 * @param include_protected_objects Set to true if you want to find protected objects.
	 * @param out Output parameter. Will be filled with an object list.
	 */
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2,
		bool include_hidden_objects, bool include_protected_objects,
		std::vector<Object*>& out);
	
	/**
	 * Counts the objects whose bounding boxes intersect the given rect.
	 * This may be inaccurate because the true object shapes usually differ
	 * from the bounding boxes.
	 * 
	 * @param map_coord_rect The query rect.
	 * @param include_hidden_objects Set to true if you want to find hidden objects.
	 */
	int countObjectsInRect(QRectF map_coord_rect, bool include_hidden_objects);
	
	/**
	 * Goes through all objects and for each object where condition(object)
	 * returns true, applies processor(object, map_part, object_index).
	 * If processor() returns false, aborts the operation and includes
	 * ObjectOperationResult::Aborted in the return value.
	 * If the operation is performed on any object (i.e. the condition returns
	 * true at least once), includes ObjectOperationResult::Success in the return value.
	 */
	template<typename Processor, typename Condition> ObjectOperationResult::Enum operationOnAllObjects(const Processor& processor, const Condition& condition)
	{
		int result = ObjectOperationResult::NoResult;
		int size = parts.size();
		for (int i = 0; i < size; ++i)
		{
			result |= parts[i]->operationOnAllObjects<Processor, Condition>(processor, condition);
			if (result & ObjectOperationResult::Abort)
				return (ObjectOperationResult::Enum)result;
		}
		return (ObjectOperationResult::Enum)result;
	}
	
	/** Version of operationOnAllObjects() without condition. */
	template<typename Processor> ObjectOperationResult::Enum operationOnAllObjects(const Processor& processor)
	{
		int result = ObjectOperationResult::NoResult;
		int size = parts.size();
		for (int i = 0; i < size; ++i)
		{
			result |= parts[i]->operationOnAllObjects<Processor>(processor);
			if (result & ObjectOperationResult::Abort)
				return (ObjectOperationResult::Enum)result;
		}
		return (ObjectOperationResult::Enum)result;
	}
	
	/** Scales all objects by the given factor. */
	void scaleAllObjects(double factor, const MapCoord& scaling_center);
	
	/** Rotates all objects by the given rotation angle (in radians). */
	void rotateAllObjects(double rotation, const MapCoord& center);
	
	/** Forces an update of all objects, i.e. calls update(true) on each map object. */
	void updateAllObjects();
	
	/** Forces an update of all objects with the given symbol. */
	void updateAllObjectsWithSymbol(Symbol* symbol);
	
	/** For all symbols with old_symbol, replaces the symbol by new_symbol. */
	void changeSymbolForAllObjects(Symbol* old_symbol, Symbol* new_symbol);
	
	/**
	 * Deleted all objects with the given symbol.
	 * Returns true if there was an object that was deleted.
	 */
	bool deleteAllObjectsWithSymbol(Symbol* symbol);
	
	/**
	 * Returns if at least one object with the given symbol exists in the map.
	 * WARNING: Even if no objects exist directly, the symbol could still be
	 *          required by another (combined) symbol used by an object!
	 */
	bool doObjectsExistWithSymbol(Symbol* symbol);
	
	/**
	 * Removes the renderables of the given object from display (does not
	 * delete them!).
	 */
	void removeRenderablesOfObject(Object* object, bool mark_area_as_dirty);
	
	/**
	 * Inserts the renderables of the given object, so they will be displayed.
	 */
	void insertRenderablesOfObject(Object* object);
	
	// Object selection
	
	/** Returns the number of selected objects. */
	inline int getNumSelectedObjects() {return (int)object_selection.size();}
	
	/** Returns an iterator allowing to iterate over the selected objects. */
	inline ObjectSelection::const_iterator selectedObjectsBegin() {return object_selection.constBegin();}
	
	/** Returns an end iterator allowing to iterate over the selected objects. */
	inline ObjectSelection::const_iterator selectedObjectsEnd() {return object_selection.constEnd();}
	
	/**
	 * Returns the object in the selection which was selected first by the user.
	 * If she later deselects it while other objects are still selected or if
	 * the selection is done as box selection, this "first" selected object is
	 * just a more or less random object from the selection.
	 */
	inline Object* getFirstSelectedObject() const {return first_selected_object;}
	
	/**
	 * Checks the selected objects for compatibility with the given symbol.
	 * @param symbol the symbol to check compatibility for
	 * @param out_compatible returns if all selected objects are compatible
	 *     to the given symbol
	 * @param out_different returns if at least one of the selected objects'
	 *     symbols is different to the given symbol
	 */
	void getSelectionToSymbolCompatibility(Symbol* symbol, bool& out_compatible, bool& out_different);
	
	/**
	 * Deletes the selected objects and creates an undo step for this action.
	 */
	void deleteSelectedObjects();
	
	/**
	 * Enlarges the given rect to cover all selected objects.
	 */
	void includeSelectionRect(QRectF& rect);
	
	/**
	 * Draws the selected objects.
	 * 
	 * @param painter The QPainter used for drawing.
	 * @param force_min_size See draw().
	 * @param widget The widget in which the drawing happens.
	 *     Used to get view and viewport information.
	 * @param replacement_renderables If given, draws these renderables instead
	 *     Of the selection renderables. TODO: HACK
	 * @param draw_normal If set to true, draws the objects like normal objects,
	 *     otherwise draws transparent highlights.
	 */
	void drawSelection(QPainter* painter, bool force_min_size, MapWidget* widget,
		MapRenderables* replacement_renderables = NULL, bool draw_normal = false);
	
	/**
	 * Adds the given object to the selection.
	 * @param object The object to add.
	 * @param emit_selection_changed Set to true if objectSelectionChanged()
	 *     should be emitted. Do this only for the last in a
	 *     sequence of selection change oparations to prevent bad performance!
	 */
	void addObjectToSelection(Object* object, bool emit_selection_changed);
	
	/**
	 * Removes the given object from the selection.
	 * @param object The object to remove.
	 * @param emit_selection_changed See addObjectToSelection().
	 */
	void removeObjectFromSelection(Object* object, bool emit_selection_changed);
	
	/**
	 * Removes from the selection all objects with the given symbol.
	 * Returns true if at least one object has been removed.
	 * @param symbol The symbol of the objects to remove.
	 * @param emit_selection_changed See addObjectToSelection().
	 */
	bool removeSymbolFromSelection(Symbol* symbol, bool emit_selection_changed);
	
	/** Returns true if the given object is selected. */
	bool isObjectSelected(Object* object);
	
	/**
	 * Toggles the selection of the given object.
	 * Returns true if the object was selected, false if deselected.
	 * @param object The object to select or deselect.
	 * @param emit_selection_changed See addObjectToSelection().
	 */
	bool toggleObjectSelection(Object* object, bool emit_selection_changed);
	
	/**
	 * Empties the object selection.
	 * @param emit_selection_changed See addObjectToSelection().
	 */
	void clearObjectSelection(bool emit_selection_changed);
	
	/**
	 * Emits objectSelectionChanged(). Use this if setting emit_selection_changed
	 * in a selection change method is unfeasible.
	 */
	void emitSelectionChanged();
	
	/**
	 * Emits selectedObjectEdited(). Use this after making changes
	 * to a selected object.
	 */
	void emitSelectionEdited();
	
	// Other settings
	
	/** Sets the map's scale denominator. */
	void setScaleDenominator(unsigned int value);
	
	/** Returns the map's scale denominator. */
	unsigned int getScaleDenominator() const;
	
	/**
	 * Changes the map's scale.
	 * 
	 * @param new_scale_denominator The new scale denominator.
	 * @param scaling_center The coordinate to use as scaling center.
	 * @param scale_symbols Whether to scale the map symbols.
	 * @param scale_objects Whether to scale the map object coordinates.
	 * @param scale_georeferencing Whether to adjust the map's georeferencing reference point.
	 * @param scale_templates Whether to scale non-georeferenced templates.
	 */
	void changeScale(unsigned int new_scale_denominator,
		const MapCoord& scaling_center, bool scale_symbols, bool scale_objects,
		bool scale_georeferencing, bool scale_templates);
	
	/**
	 * Rotate the map around a point.
	 * 
	 * @param rotation The rotation angle (in radians).
	 * @param center The rotation center point.
	 * @param adjust_georeferencing Whether to adjust the georeferencing reference point.
	 * @param adjust_declination Whether to adjust the georeferencing declination.
	 * @param adjust_templates Whether to adjust non-georeferenced templates.
	 */
	void rotateMap(double rotation, const MapCoord& center,
		bool adjust_georeferencing, bool adjust_declination,
		bool adjust_templates);
	
	/** Returns the map notes string. */
	inline const QString& getMapNotes() const {return map_notes;}
	
	/**
	 * Sets the map notes string.
	 * NOTE: Set the map to dirty manually!
	 */
	inline void setMapNotes(const QString& text) {map_notes = text;}
	
	/**
	 * Assigns georeferencing settings for the map from the given object and
	 * sets the map to have unsaved changes.
	 */
	void setGeoreferencing(const Georeferencing& georeferencing);
	
	/** Returns the map's georeferencing object. */
	inline const Georeferencing& getGeoreferencing() const {return *georeferencing;}
	
	/** Returns the map's grid object. */
	inline MapGrid& getGrid() {return *grid;}
	
	/**
	 * TODO: These two options should really be view options, but are not due
	 * to a limitation:
	 * the current architecture makes it impossible to have different
	 * renderables of the same objects in different views!
	 */
	
	/** Returns if area hatching is enabled. */
	inline bool isAreaHatchingEnabled() const {return area_hatching_enabled;}
	
	/** Sets if area hatching is enabled. */
	inline void setAreaHatchingEnabled(bool enabled) {area_hatching_enabled = enabled;}
	
	/** Returns if the baseline view is enabled. */
	inline bool isBaselineViewEnabled() const {return baseline_view_enabled;}
	
	/** Sets if the baseline view is enabled. */
	inline void setBaselineViewEnabled(bool enabled) {baseline_view_enabled = enabled;}
	
	
	/** Returns a copy of the current print configuration. */
	MapPrinterConfig printerConfig() const;
	
	/** Returns a const reference to the current print configuration. */
	const MapPrinterConfig& printerConfig();
	
	/** Sets the current print configuration. */
	void setPrinterConfig(const MapPrinterConfig& config);
	
	
	/**
	 * Sets default parameters for loading of image templates.
	 * TODO: put these into a struct.
	 */
	void setImageTemplateDefaults(bool use_meters_per_pixel, double meters_per_pixel,
		double dpi, double scale);
	
	/** Returns the default parameters for loading of image tempaltes. */
	void getImageTemplateDefaults(bool& use_meters_per_pixel, double& meters_per_pixel,
		double& dpi, double& scale);
	
	/**
	 * Returns whether there are unsaved changes in the map.
	 * 
	 * To toggle this state, never use setHasUnsavedChanges() directly unless
	 * you know what you are doing, instead use setOtherDirty() or set one of
	 * the more specific 'dirty' flags. This is because a call to
	 * setHasUnsavedChanges() alone followed by a map change and an undo would
	 * result in no changed flag.
	 */
	inline bool hasUnsavedChanged() const {return unsaved_changes;}
	
	/** Do not use this in usual cases, see hasUnsavedChanged(). */
	void setHasUnsavedChanges(bool has_unsaved_changes = true);
	
	/** Returns if there are unsaved changes to the colors. */
	inline bool areColorsDirty() const {return colors_dirty;}
	/** Returns if there are unsaved changes to the symbols. */
	inline bool areSymbolsDirty() const {return symbols_dirty;}
	/** Returns if there are unsaved changes to the templates. */
	inline bool areTemplatesDirty() const {return templates_dirty;}
	/** Returns if there are unsaved changes to the objects. */
	inline bool areObjectsDirty() const {return objects_dirty;}
	/** Returns if there are unsaved changes to anything else than the above. */
	inline bool isOtherDirty() const {return other_dirty;}
	
	/**
	 * Marks somthing unspecific in the map as "dirty", i.e. as having unsaved changes.
	 * Emits gotUnsavedChanges() if the map did not have unsaved changed before.
	 * 
	 * Use setColorsDirty(), setSymbolsDirty(), setTemplatesDirty() or
	 * setObjectsDirty() if you know more specificly what has changed.
	 */
	void setOtherDirty(bool value = true);
	
	// Static
	
	/** Returns the special covering white color. */
	static const MapColor* getCoveringWhite();
	
	/** Returns the special covering red color. */
	static const MapColor* getCoveringRed();
	
	/** Returns the special covering gray color for "undefined" objects. */
	static const MapColor* getUndefinedColor();
	
	/** Returns the special registration color. */
	static const MapColor* getRegistrationColor();
	
	/** Returns the special covering white line symbol. */
	static LineSymbol* getCoveringWhiteLine() {return covering_white_line;}
	/** Returns the special covering red line symbol. */
	static LineSymbol* getCoveringRedLine() {return covering_red_line;}
	/** Returns the special covering combined symbol (white + red). */
	static CombinedSymbol* getCoveringCombinedLine() {return covering_combined_line;}
	/** Returns the special gray "undefined" line symbol. */
	static LineSymbol* getUndefinedLine() {return undefined_line;}
	/** Returns the special gray "undefined" point symbol. */
	static PointSymbol* getUndefinedPoint() {return undefined_point;}
	
signals:
	/**
	 * Emitted when a change is made which causes the map to contain
	 * unsaved changes, when it had not unsaved changes before.
	 */
	void gotUnsavedChanges();
	
	/** Emitted when a color is added to the map, gives the color's index and pointer. */
	void colorAdded(int pos, MapColor* color);
	/** Emitted when a map color is changed, gives the color's index and pointer. */
	void colorChanged(int pos, MapColor* color);
	/** Emitted when a map color is deleted, gives the color's index and pointer. */
	void colorDeleted(int pos, const MapColor* old_color);
	/** Emitted when the presence of spot colors in the map changes. */
	void spotColorPresenceChanged(bool has_spot_colors) const;
	
	/** Emitted when a symbol is added to the map, gives the symbol's index and pointer. */
	void symbolAdded(int pos, Symbol* symbol);
	/** Emitted when a symbol in the map is changed. */
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	/** Emitted when the icon of the symbol with the given index changes. */
	void symbolIconChanged(int pos);
	/** Emitted when a symbol in the map is deleted. */
	void symbolDeleted(int pos, Symbol* old_symbol);
	
	/** Emitted when a template is added to the map, gives the template's index and pointer. */
	void templateAdded(int pos, Template* temp);
	/** Emitted when a template in the map is changed, gives the template's index and pointer. */
	void templateChanged(int pos, Template* temp);
	/** Emitted when a template in the map is deleted, gives the template's index and pointer. */
	void templateDeleted(int pos, Template* old_temp);
	
	/** Emitted when the number of closed templates changes between zero and one. */
	void closedTemplateAvailabilityChanged();
	
	/** Emitted when the set of selected objects changes. Also emitted when the
	 *  symbol of a selected object changes (which is similar to selecting another
	 *  object). */
	void objectSelectionChanged();
	
	/**
	 * Emitted when at least one of the selected objects is edited in any way.
	 * For example, this includes the case where a symbol of one of the
	 * selected objects is edited, too.
	 */
	void selectedObjectEdited();

	/**
	 * Emitted when the map part currently used for drawing changes
	 */
	void currentMapPartChanged(int index);
	
protected slots:
	void checkSpotColorPresence();
	
protected slots:
	void checkSpotColorPresence();
	
private:
	typedef std::vector<MapColor*> ColorVector;
	typedef std::vector<Symbol*> SymbolVector;
	typedef std::vector<Template*> TemplateVector;
	typedef std::vector<MapPart*> PartVector;
	typedef std::vector<MapWidget*> WidgetVector;
	typedef std::vector<MapView*> ViewVector;
	
	class MapColorSet : public QObject
	{
	public:
		ColorVector colors;
		
		MapColorSet(QObject *parent = 0);
		void addReference();
		void dereference();
		
		/** Merges another MapColorSet into this set, trying to maintain
		 *  the relative order of colors.
		 *  If a filter is given, imports only the colors for  which
		 *  filter[color_index] is true, or which are spot colors referenced
		 *  by the selected colors.
		 *  If a map is given, this color set is modified through the map's
		 *  color accessor methods so that other object become aware of the
		 *  changes.
		 *  @return a mapping from the imported color pointer in the other set
		 *          to color pointers in this set.
		 */
		MapColorMap importSet(const MapColorSet& other, 
		                      std::vector<bool>* filter = NULL,
		                      Map* map = NULL);
		
	private:
		int ref_count;
	};
	
	void checkIfFirstColorAdded();
	void checkIfFirstSymbolAdded();
	void checkIfFirstTemplateAdded();
	
	void adjustColorPriorities(int first, int last);
	
	/// Imports the other symbol set into this set, only importing the symbols for which filter[color_index] == true and
	/// returning the map from symbol indices in other to imported indices. Imported symbols are placed after the existing symbols.
	void importSymbols(Map* other, const MapColorMap& color_map, int insert_pos = -1, bool merge_duplicates = true, std::vector<bool>* filter = NULL,
					   QHash<int, int>* out_indexmap = NULL, QHash<Symbol*, Symbol*>* out_pointermap = NULL);
	
	void addSelectionRenderables(Object* object);
	void updateSelectionRenderables(Object* object);
	void removeSelectionRenderables(Object* object);
	
	static void initStatic();
	
	MapColorSet* color_set;
	bool has_spot_colors;
	SymbolVector symbols;
	TemplateVector templates;
	TemplateVector closed_templates;
	int first_front_template;		// index of the first template in templates which should be drawn in front of the map
	PartVector parts;
	ObjectSelection object_selection;
	Object* first_selected_object;
	UndoManager object_undo_manager;
	int current_part_index;
	WidgetVector widgets;
	ViewVector views;
	QScopedPointer<MapRenderables> renderables;
	QScopedPointer<MapRenderables> selection_renderables;
	
	QString map_notes;
	
	Georeferencing* georeferencing;
	
	MapGrid* grid;
	
	bool area_hatching_enabled;
	bool baseline_view_enabled;
	
	QScopedPointer<MapPrinterConfig> printer_config;
	
	bool image_template_use_meters_per_pixel;
	double image_template_meters_per_pixel;
	double image_template_dpi;
	double image_template_scale;
	
	bool colors_dirty;				// are there unsaved changes for the colors?
	bool symbols_dirty;				//    ... for the symbols?
	bool templates_dirty;			//    ... for the templates?
	bool objects_dirty;				//    ... for the objects?
	bool other_dirty;				//    ... for any other settings?
	bool unsaved_changes;			// are there unsaved changes for any component?
	
	// Static
	
	static bool static_initialized;
	static MapColor covering_white;
	static MapColor covering_red;
	static MapColor undefined_symbol_color;
	static MapColor registration_color;
	static LineSymbol* covering_white_line;
	static LineSymbol* covering_red_line;
	static LineSymbol* undefined_line;
	static PointSymbol* undefined_point;
	static CombinedSymbol* covering_combined_line;
};


// ### Map inline code ###

inline
const MapColor* Map::getCoveringRed()
{
	return &covering_red;
}

inline
const MapColor* Map::getCoveringWhite()
{
	return &covering_white;
}

inline
const MapColor* Map::getUndefinedColor()
{
	return &undefined_symbol_color;
}

inline
const MapColor* Map::getRegistrationColor()
{
	return &registration_color;
}

inline
MapColor* Map::getColor(int i)
{
	if (0 <= i && i < (int)color_set->colors.size())
	{
		return color_set->colors[i];
	}
	return NULL;
}

inline
const MapColor* Map::getColor(int i) const
{
	if (0 <= i && i < (int)color_set->colors.size())
	{
		return color_set->colors[i];
	}
	else switch (i)
	{
		case -1005:
			return getCoveringRed();
		case -1000:
			return getCoveringWhite();
		case -900:
			return getRegistrationColor();
		case -500:
			return getUndefinedColor();
		default:
			return NULL;
	}
}


#endif
