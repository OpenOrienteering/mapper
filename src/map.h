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


#ifndef _OPENORIENTEERING_MAP_H_
#define _OPENORIENTEERING_MAP_H_

#include <vector>

#include <QString>
#include <QRect>
#include <QHash>
#include <QSet>
#include <QScopedPointer>

#include "global.h"
#include "undo.h"
#include "matrix.h"
#include "map_coord.h"

class QIODevice;
class QPainter;

class Map;
struct MapColor;
class MapWidget;
class MapView;
class MapEditorController;
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

typedef std::vector< std::pair< int, Object* > > SelectionInfoVector;

class MapLayer
{
friend class OCAD8FileImport;
public:
	MapLayer(const QString& name, Map* map);
	~MapLayer();
	
	void save(QIODevice* file, Map* map);
	bool load(QIODevice* file, int version, Map* map);
	
	inline const QString& getName() const {return name;}
	inline void setName(const QString new_name) {name = new_name;}
	
	inline int getNumObjects() const {return (int)objects.size();}
	inline Object* getObject(int i) {return objects[i];}
    inline const Object* getObject(int i) const {return objects[i];}
	int findObjectIndex(Object* object);					// asserts that the object is contained in the layer
	void setObject(Object* object, int pos, bool delete_old);
	void addObject(Object* object, int pos);
	void deleteObject(int pos, bool remove_only);
	bool deleteObject(Object* object, bool remove_only);	// returns if the object was found
	
	/// Imports the contents of the other layer (which can be from another map) into this layer.
	/// Uses symbol_map to replace all symbols contained there. No replacement is done for symbols which are not in the map.
	void importLayer(MapLayer* other, QHash<Symbol*, Symbol*>& symbol_map, bool select_new_objects);
	
	void findObjectsAt(MapCoordF coord, float tolerance, bool extended_selection, bool include_hidden_objects, bool include_protected_objects, SelectionInfoVector& out);
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, bool include_hidden_objects, bool include_protected_objects, std::vector<Object*>& out);
	
	QRectF calculateExtent(bool include_helper_symbols);
	void scaleAllObjects(double factor);
	void updateAllObjects(bool remove_old_renderables = true);
	void updateAllObjectsWithSymbol(Symbol* symbol);
	void changeSymbolForAllObjects(Symbol* old_symbol, Symbol* new_symbol);
	bool deleteAllObjectsWithSymbol(Symbol* symbol);		// returns if there was an object that was deleted
	bool doObjectsExistWithSymbol(Symbol* symbol);
	void forceUpdateOfAllObjects(Symbol* with_symbol = NULL);
	
private:
	QString name;
	std::vector<Object*> objects;	// TODO: this could / should be a spatial representation optimized for quick access
	Map* map;
};

/// Central class for an OpenOrienteering map
class Map : public QObject
{
Q_OBJECT
friend class MapRenderables;
friend class OCAD8FileImport;
friend class XMLFileImport;
friend class NativeFileImport;
friend class NativeFileExport;
public:
	typedef QSet<Object*> ObjectSelection;
	
	enum ImportMode
	{
		/// Imports all objects with minimal symbol and color dependencies
		MinimalObjectImport = 0,
		
		/// Imports symbols (all, or filtered by the given filter) with minimal color dependencies.
		/// If a filter is given, symbols blocked by the filter but which are needed by included symbols are included, too.
		MinimalSymbolImport,
		
		/// Imports objects (all, or filtered by the given filter)
		ColorImport,
		
		/// Imports all colors, symbols and objects
		CompleteImport
	};
	
	/// Creates a new, empty map
	Map();
	~Map();
	
	/// Attempts to save the map to the given file. If a MapEditorController is given, the widget positions and MapViews stored in the map file are also updated.
    bool saveTo(const QString& path, MapEditorController* map_editor = NULL);
	/// Attempts to load the map from the specified path. Returns true on success.
	bool loadFrom(const QString& path, MapEditorController* map_editor = NULL, bool load_symbols_only = false);
	/// Imports the other map into this map with the following strategy:
	///  - if the other map contains objects, import all objects with the minimum amount of colors and symbols needed to display them
	///  - if the other map does not contain objects, import all symbols with the minimum amount of colors needed to display them
	///  - if the other map does neither contain objects nor symbols, import all colors
	/// WARNING: this method potentially changes the 'other' map if the scales differ (by rescaling to fit this map's scale)!
	void importMap(Map* other, ImportMode mode, QWidget* dialog_parent = NULL, std::vector<bool>* filter = NULL, int symbol_insert_pos = -1, bool merge_duplicate_symbols = true);

	/// Deletes all map data and resets the map to its initial state containing one default layer
	void clear();
	
	/// Draws the part of the map which is visible in the given bounding box in map coordinates
	void draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling, bool show_helper_symbols, float opacity = 1.0);
	/// Draws the templates first_template until last_template which are visible in the given bouding box. view determines template visibility and can be NULL to show all templates.
	/// draw_untransformed_parts is only possible with a MapWidget (because of MapWidget::mapToViewport()). Otherwise, set it to NULL.
	void drawTemplates(QPainter* painter, QRectF bounding_box, int first_template, int last_template, bool draw_untransformed_parts, const QRect& untransformed_dirty_rect, MapWidget* widget, MapView* view);
	/// Updates the renderables and extent of all objects which have been changed. This is automatically called by draw(), you normally do not need it
	void updateObjects();
	/// Calculates the extent of all map objects (and possibly templates). If templates should be included, a view can be given to take the template visibilities from.
	/// view can also be NULL to include all templates.
	QRectF calculateExtent(bool include_helper_symbols, bool include_templates, MapView* view);
	
	/// Must be called to notify the map of new widgets displaying it. Useful to notify the widgets about which parts of the map have changed and need to be redrawn
	void addMapWidget(MapWidget* widget);
	void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	/// Makes sure that the selected object(s) are visible in all map widgets
	void ensureVisibilityOfSelectedObjects();
	
	/// MapViews register themselves using this method (and deregister using removeMapView()).
	/// If there are map views left when the map is destructed, it will delete them
	/*void addMapView(MapView* view);
	void removeMapView(MapView* view);*/
	
	// Current drawing
	
	/// Sets the rect (given in map coordinates) as dirty rect for every map widget, enlarged by the given pixel border
	void setDrawingBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	void clearDrawingBoundingBox();
	
	void setActivityBoundingBox(QRectF map_coords_rect, int pixel_border, bool do_update = true);
	void clearActivityBoundingBox();
	
	void updateDrawing(QRectF map_coords_rect, int pixel_border);	// updates all dynamic drawing: tool & activity drawings
	
	// Colors
	
	inline int getNumColors() const {return (int)color_set->colors.size();}
	inline MapColor* getColor(int i) {return color_set->colors[i];}
	void setColor(MapColor* color, int pos);
	MapColor* addColor(int pos);
	void addColor(MapColor* color, int pos);
	void deleteColor(int pos);
	int findColorIndex(MapColor* color);	// returns -1 if not found
	void setColorsDirty();
	
	void useColorsFrom(Map* map);
	bool isColorUsedByASymbol(MapColor* color);
	
	// Symbols
	
	inline int getNumSymbols() const {return (int)symbols.size();}
	Symbol* getSymbol(int i) const;
	void setSymbol(Symbol* symbol, int pos);
	void addSymbol(Symbol* symbol, int pos);
	void moveSymbol(int from, int to);
    void sortSymbols(bool (*cmp)(Symbol *, Symbol *));
	void deleteSymbol(int pos);
	int findSymbolIndex(Symbol* symbol);
	void setSymbolsDirty();
	
	void scaleAllSymbols(double factor);
	/// Returns a vector of the same size as the symbol list, where each element is set to true if
	/// there is at least one object which uses this symbol or a derived (combined) symbol
	void determineSymbolsInUse(std::vector<bool>& out);
	
	// Templates
	
	inline int getNumTemplates() const {return templates.size();}
	inline Template* getTemplate(int i) {return templates[i];}
	inline void setFirstFrontTemplate(int pos) {first_front_template = pos;}
	inline int getFirstFrontTemplate() const {return first_front_template;}
	void setTemplate(Template* temp, int pos);
	void addTemplate(Template* temp, int pos);									// NOTE: adjust first_front_template manually!
	void deleteTemplate(int pos);												// NOTE: adjust first_front_template manually!
	void setTemplateAreaDirty(Template* temp, QRectF area, int pixel_border);	// marks the respective regions in the template caches as dirty; area is given in map coords (mm). Does nothing if the template is not visible in a widget! So make sure to call this and showing/hiding a template in the correct order!
	void setTemplateAreaDirty(int i);											// this does nothing for i == -1
	int findTemplateIndex(Template* temp);
	void setTemplatesDirty();
	void emitTemplateChanged(Template* temp);
	
	// Layers & Undo
	
	inline UndoManager& objectUndoManager() {return object_undo_manager;}
	
	inline int getNumLayers() const {return (int)layers.size();}
	inline MapLayer* getLayer(int i) const {return layers[i];}
	void addLayer(MapLayer* layer, int pos);
	int findLayerIndex(MapLayer* layer) const;
	
	// TODO: Layer management
	
	inline MapLayer* getCurrentLayer() const {return (current_layer_index < 0) ? NULL : layers[current_layer_index];}
	inline void setCurrentLayer(MapLayer* layer) {current_layer_index = findLayerIndex(layer);}
	inline int getCurrentLayerIndex() const {return current_layer_index;}
	
	// Objects
	
	int getNumObjects();
	int addObject(Object* object, int layer_index = -1);						// returns the index of the added object in the layer
	void deleteObject(Object* object, bool remove_only);						// remove_only will remove the object from the map, but not call "delete object"; be sure to call removeObjectFromSelection() if necessary
	void setObjectsDirty();
	
	void setObjectAreaDirty(QRectF map_coords_rect);
	void findObjectsAt(MapCoordF coord, float tolerance, bool extended_selection, bool include_hidden_objects, bool include_protected_objects, SelectionInfoVector& out);
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, bool include_hidden_objects, bool include_protected_objects, std::vector<Object*>& out);
	
	void scaleAllObjects(double factor);
	void updateAllObjects(bool remove_old_renderables = true);
	void updateAllObjectsWithSymbol(Symbol* symbol);
	void changeSymbolForAllObjects(Symbol* old_symbol, Symbol* new_symbol);
	bool deleteAllObjectsWithSymbol(Symbol* symbol);							// returns if there was an object that was deleted
	bool doObjectsExistWithSymbol(Symbol* symbol);
	void forceUpdateOfAllObjects(Symbol* with_symbol = NULL);					// if with_symbol == NULL, all objects are affected
	
	void removeRenderablesOfObject(Object* object, bool mark_area_as_dirty);	// NOTE: does not delete the renderables, just removes them from display
	void insertRenderablesOfObject(Object* object);
	
	// Object selection
	
	inline int getNumSelectedObjects() {return (int)object_selection.size();}
	inline ObjectSelection::const_iterator selectedObjectsBegin() {return object_selection.constBegin();}
	inline ObjectSelection::const_iterator selectedObjectsEnd() {return object_selection.constEnd();}
	
	/** Returns the object in the selection which was selected first by the user.
	 *  If she later deselects it while other objects are still selected or if the selection is done as box selection,
	 *  this "first" selected object is just a more or less random object from the selection.
	 */
	inline Object* getFirstSelectedObject() const {return first_selected_object;}
	
	/** Checks the selected objects for compatibiliy with the given symbol.
	 * @param symbol the symbol to check compatibiliy for
	 * @param out_compatible returns if all selected objects are compatible to the given symbol
	 * @param out_different returns if at least one of the selected objects' symbols is different to the given symbol
	 */
	void getSelectionToSymbolCompatibility(Symbol* symbol, bool& out_compatible, bool& out_different);
	
	void includeSelectionRect(QRectF& rect); // enlarges rect to cover the selected objects
	void drawSelection(QPainter* painter, bool force_min_size, MapWidget* widget, MapRenderables* replacement_renderables = NULL, bool draw_normal = false);
	
	void addObjectToSelection(Object* object, bool emit_selection_changed);
	void removeObjectFromSelection(Object* object, bool emit_selection_changed);
	bool removeSymbolFromSelection(Symbol* symbol, bool emit_selection_changed);	// removes all objects with this symbol; returns true if at least one object has been removed
	bool isObjectSelected(Object* object);
	bool toggleObjectSelection(Object* object, bool emit_selection_changed);	// returns true if the object was selected, false if deselected
	void clearObjectSelection(bool emit_selection_changed);
	void emitSelectionChanged();
	void emitSelectionEdited();
	
	// Other settings
	
	void setScaleDenominator(int value);
	int getScaleDenominator() const;
	void changeScale(int new_scale_denominator, bool scale_symbols, bool scale_objects);
	
	inline const QString& getMapNotes() const {return map_notes;}
	inline void setMapNotes(const QString& text) {map_notes = text;}
	
	void setGeoreferencing(const Georeferencing& georeferencing);
	inline const Georeferencing& getGeoreferencing() const {return *georeferencing;}
	
	inline bool arePrintParametersSet() const {return print_params_set;}
	void setPrintParameters(int orientation, int format, float dpi, bool show_templates, bool center, float left, float top, float width, float height);
	void getPrintParameters(int& orientation, int& format, float& dpi, bool& show_templates, bool& center, float& left, float& top, float& width, float& height);
	
	void setImageTemplateDefaults(bool use_meters_per_pixel, double meters_per_pixel, double dpi, double scale);
	void getImageTemplateDefaults(bool& use_meters_per_pixel, double& meters_per_pixel, double& dpi, double& scale);
	
	void setHasUnsavedChanges(bool has_unsaved_changes = true);
	inline bool hasUnsavedChanged() const {return unsaved_changes;}
	
	inline bool areColorsDirty() const {return colors_dirty;}
	inline bool areSymbolsDirty() const {return symbols_dirty;}
	inline bool areTemplatesDirty() const {return templates_dirty;}
	inline bool areObjectsDirty() const {return objects_dirty;}
	inline bool isOtherDirty() const {return other_dirty;}
	void setOtherDirty(bool value = true);
	
	// Static
	
	static MapColor* getCoveringWhite() {return &covering_white;}
	static MapColor* getCoveringRed() {return &covering_red;}
	static MapColor* getUndefinedColor() {return &undefined_symbol_color;}
	static LineSymbol* getCoveringWhiteLine() {return covering_white_line;}
	static LineSymbol* getCoveringRedLine() {return covering_red_line;}
	static CombinedSymbol* getCoveringCombinedLine() {return covering_combined_line;}
	static LineSymbol* getUndefinedLine() {return undefined_line;}
	static PointSymbol* getUndefinedPoint() {return undefined_point;}
	
signals:
	void gotUnsavedChanges();
	
	void colorAdded(int pos, MapColor* color);
	void colorChanged(int pos, MapColor* color);
	void colorDeleted(int pos, MapColor* old_color);
	
	void symbolAdded(int pos, Symbol* symbol);
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolIconChanged(int pos);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
	void templateAdded(int pos, Template* temp);
	void templateChanged(int pos, Template* temp);
	void templateDeleted(int pos, Template* old_temp);
	
	void objectSelectionChanged();
	void selectedObjectEdited();
	
private:
	typedef std::vector<MapColor*> ColorVector;
	typedef std::vector<Symbol*> SymbolVector;
	typedef std::vector<Template*> TemplateVector;
	typedef std::vector<MapLayer*> LayerVector;
	typedef std::vector<MapWidget*> WidgetVector;
	typedef std::vector<MapView*> ViewVector;
	
	class MapColorSet : public QObject
	{
	public:
		ColorVector colors;
		
		MapColorSet(QObject *parent = 0);
		void addReference();
		void dereference();
		
		/// Imports the other set into this set, only importing the colors for which filter[color_index] == true and
		/// returning the map from color indices in other to imported indices. Imported colors are placed above the existing colors.
		/// If a map is given, the color is properly inserted into the map.
		void importSet(MapColorSet* other, Map* map = NULL, std::vector<bool>* filter = NULL, QHash<int, int>* out_indexmap = NULL,
					   QHash<MapColor*, MapColor*>* out_pointermap = NULL);
		
	private:
		int ref_count;
	};
	
	void checkIfFirstColorAdded();
	void checkIfFirstSymbolAdded();
	void checkIfFirstTemplateAdded();
	
	void adjustColorPriorities(int first, int last);
	void determineColorsInUse(const std::vector< bool >& by_which_symbols, std::vector< bool >& out);
	/// Imports the other symbol set into this set, only importing the symbols for which filter[color_index] == true and
	/// returning the map from symbol indices in other to imported indices. Imported symbols are placed after the existing symbols.
	void importSymbols(Map* other, const QHash<MapColor*, MapColor*>& color_map, int insert_pos = -1, bool merge_duplicates = true, std::vector<bool>* filter = NULL,
					   QHash<int, int>* out_indexmap = NULL, QHash<Symbol*, Symbol*>* out_pointermap = NULL);
	void determineSymbolUseClosure(std::vector< bool >& symbol_bitfield);
	
	void addSelectionRenderables(Object* object);
	void updateSelectionRenderables(Object* object);
	void removeSelectionRenderables(Object* object);
	
	static void initStatic();
	
	MapColorSet* color_set;
	SymbolVector symbols;
	TemplateVector templates;
	int first_front_template;		// index of the first template in templates which should be drawn in front of the map
	LayerVector layers;
	ObjectSelection object_selection;
	Object* first_selected_object;
	UndoManager object_undo_manager;
	int current_layer_index;
	WidgetVector widgets;
	ViewVector views;
	QScopedPointer<MapRenderables> renderables;
	QScopedPointer<MapRenderables> selection_renderables;
	
	QString map_notes;
	
	Georeferencing* georeferencing;
	
	bool print_params_set;			// have the parameters been set (are they valid)?
	int print_orientation;			// QPrinter::Orientation
	int print_format;				// QPrinter::PaperSize
	float print_dpi;
	bool print_show_templates;
	bool print_center;
	float print_area_left;
	float print_area_top;
	float print_area_width;
	float print_area_height;
	
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
	static LineSymbol* covering_white_line;
	static LineSymbol* covering_red_line;
	static LineSymbol* undefined_line;
	static PointSymbol* undefined_point;
	static CombinedSymbol* covering_combined_line;
};

/// Contains all visibility information for a template. This is stored in the MapViews
struct TemplateVisibility
{
	float opacity;	// 0 to 1
	bool visible;
	
	inline TemplateVisibility()
	{
		opacity = 1;
		visible = false;
	}
};

/// Stores view position, zoom, rotation and template visibilities to define a view onto a map
class MapView
{
public:
	/// Creates a default view looking at the origin
	MapView(Map* map);
	~MapView();
	
	void save(QIODevice* file);
	void load(QIODevice* file);
	
	/// Must be called to notify the map view of new widgets displaying it. Useful to notify the widgets which need to be redrawn
	void addMapWidget(MapWidget* widget);
    void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	
	/// Converts x, y (with origin at the center of the view) to map coordinates
	inline MapCoord viewToMap(double x, double y)
	{
		return MapCoord(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
	}
	inline MapCoord viewToMap(QPointF point)
	{
		return viewToMap(point.x(), point.y());
	}
	inline MapCoordF viewToMapF(double x, double y)
	{
		return MapCoordF(view_to_map.get(0, 0) * x + view_to_map.get(0, 1) * y + view_to_map.get(0, 2), view_to_map.get(1, 0) * x + view_to_map.get(1, 1) * y + view_to_map.get(1, 2));
	}
	inline MapCoordF viewToMapF(QPointF point)
	{
		return viewToMapF(point.x(), point.y());
	}
	/// Converts map coordinates to view coordinates (with origin at the center of the view)
	inline void mapToView(MapCoord coords, double& out_x, double& out_y)
	{
		out_x = map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2);
		out_y = map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2);
	}
	inline QPointF mapToView(MapCoord coords)
	{
		return QPointF(map_to_view.get(0, 0) * coords.xd() + map_to_view.get(0, 1) * coords.yd() + map_to_view.get(0, 2),
					   map_to_view.get(1, 0) * coords.xd() + map_to_view.get(1, 1) * coords.yd() + map_to_view.get(1, 2));
	}
	inline QPointF mapToView(MapCoordF coords)
	{
		return QPointF(map_to_view.get(0, 0) * coords.getX() + map_to_view.get(0, 1) * coords.getY() + map_to_view.get(0, 2),
					   map_to_view.get(1, 0) * coords.getX() + map_to_view.get(1, 1) * coords.getY() + map_to_view.get(1, 2));
	}
	
	/// Convert lengths between map and pixel display
	double lengthToPixel(qint64 length);
	qint64 pixelToLength(double pixel);
	
	/// Calculates the bounding box of the map coordinates which can be viewed using the given view coordinates rect
	QRectF calculateViewedRect(QRectF view_rect);
	/// Calculates the bounding box in view coordinates of the given map coordinates rect
	QRectF calculateViewBoundingBox(QRectF map_rect);
	
	/// Applies the view transform to the given painter.
	/// Note 1: The transform is combined with the painter's existing transform.
	/// Note 2: If you want to use the transform to draw something, you will probably also want to make sure that
	///         the view center is in the center of your viewport. Because the view does not know the viewport size,
	///         it cannot do that. So this offset must be applied before calling this method.
	void applyTransform(QPainter* painter);
	
	// Dragging
	void setDragOffset(QPoint offset);
	void completeDragging(QPoint offset);
	
	// Zooming, returns if the view was zoomed
	bool zoomSteps(float num_steps, bool preserve_cursor_pos, QPointF cursor_pos_view = QPointF());
	
	inline Map* getMap() const {return map;}
	
	inline float calculateFinalZoomFactor() {return lengthToPixel(1000);}
	inline float getZoom() const {return zoom;}
	void setZoom(float value);
	inline float getRotation() const {return rotation;}
	inline void setRotation(float value) {rotation = value; update();}
	inline qint64 getPositionX() const {return position_x;}
	void setPositionX(qint64 value);
	inline qint64 getPositionY() const {return position_y;}
	void setPositionY(qint64 value);
	inline int getViewX() const {return view_x;}
	inline void setViewX(int value) {view_x = value; update();}
	inline int getViewY() const {return view_y;}
	inline void setViewY(int value) {view_y = value; update();}
	inline QPoint getDragOffset() const {return drag_offset;}
	
    // Map visibility
    TemplateVisibility* getMapVisibility();

	// Template visibilities
	bool isTemplateVisible(Template* temp);						// checks if the template is visible without creating a template visibility object if none exists
	TemplateVisibility* getTemplateVisibility(Template* temp);	// returns the template visibility object, creates one if not there yet with the default settings (invisible)
	void deleteTemplateVisibility(Template* temp);				// call this when a template is deleted to destroy the template visibility object
	
	// Static
	static const double zoom_in_limit;
	static const double zoom_out_limit;
	
private:
	typedef std::vector<MapWidget*> WidgetVector;
	
	void update();		// recalculates the x_to_y matrices
	
	Map* map;
	
	double zoom;		// factor
	double rotation;	// counterclockwise 0 to 2*PI. This is the viewer rotation, so the map is rotated clockwise
	qint64 position_x;	// position of the viewer, positive values move the map to the left; the position is in 1/1000 mm
	qint64 position_y;
	int view_x;			// view offset (so the view can be rotated around a point which is not in its center) ...
	int view_y;			// ... this can be used to always look ahead of the GPS position if a digital compass is present
	QPoint drag_offset;	// the distance the content of the view was dragged with the mouse, in pixels
	
	Matrix view_to_map;
	Matrix map_to_view;
	
    TemplateVisibility *map_visibility;
	QHash<Template*, TemplateVisibility*> template_visibilities;
	
	WidgetVector widgets;
};

#endif
