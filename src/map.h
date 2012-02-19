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

#include "undo.h"
#include "matrix.h"
#include "map_coord.h"
#include "renderable.h"

QT_BEGIN_NAMESPACE
class QFile;
class QPainter;
QT_END_NAMESPACE

class Map;
struct MapColor;
class MapWidget;
class MapView;
class Symbol;
class Template;
class Object;
class MapEditorController;
class OCAD8FileImport;
struct GPSProjectionParameters;

typedef std::vector< std::pair< int, Object* > > SelectionInfoVector;

class MapLayer
{
friend class OCAD8FileImport;
public:
	MapLayer(const QString& name, Map* map);
	~MapLayer();
	
	void save(QFile* file, Map* map);
	bool load(QFile* file, int version, Map* map);
	
	inline const QString& getName() const {return name;}
	inline void setName(const QString new_name) {name = new_name;}
	
	inline int getNumObjects() const {return (int)objects.size();}
	inline Object* getObject(int i) {return objects[i];}
	int findObjectIndex(Object* object);					// asserts that the object is contained in the layer
	void setObject(Object* object, int pos, bool delete_old);
	void addObject(Object* object, int pos);
	void deleteObject(int pos, bool remove_only);
	bool deleteObject(Object* object, bool remove_only);	// returns if the object was found
	
	void findObjectsAt(MapCoordF coord, float tolerance, bool extended_selection, SelectionInfoVector& out);
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, std::vector<Object*>& out);
	
	QRectF calculateExtent();
	void scaleAllObjects(double factor);
	void updateAllObjects();
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
friend class RenderableContainer;
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	typedef QSet<Object*> ObjectSelection;
	
	/// Creates a new, empty map
	Map();
	~Map();
	
	/// Attempts to save the map to the given file. If a MapEditorController is given, the widget positions and MapViews stored in the map file are also updated.
	bool saveTo(const QString& path, MapEditorController* map_editor = NULL);
	/// Attempts to load the map from the specified path. Returns true on success.
	bool loadFrom(const QString& path, MapEditorController* map_editor = NULL);

	/// Deletes all map data
	void clear();
	
	/// Draws the part of the map which is visible in the given bounding box in map coordinates
	void draw(QPainter* painter, QRectF bounding_box, bool force_min_size, float scaling);
	/// Draws the templates first_template until last_template which are visible in the given bouding box. view determines template visibility and can be NULL to show all templates.
	/// draw_untransformed_parts is only possible with a MapWidget (because of MapWidget::mapToViewport()). Otherwise, set it to NULL.
	void drawTemplates(QPainter* painter, QRectF bounding_box, int first_template, int last_template, bool draw_untransformed_parts, const QRect& untransformed_dirty_rect, MapWidget* widget, MapView* view);
	/// Updates the renderables and extent of all objects which have been changed. This is automatically called by draw(), you normally do not need it
	void updateObjects();
	/// Calculates the extent of all map objects (and possibly templates)
	QRectF calculateExtent(bool include_templates, MapView* view);
	
	/// Must be called to notify the map of new widgets displaying it. Useful to notify the widgets about which parts of the map have changed and need to be redrawn
	void addMapWidget(MapWidget* widget);
	void removeMapWidget(MapWidget* widget);
	/// Redraws all map widgets completely - that can be slow!
	void updateAllMapWidgets();
	
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
	inline Symbol* getSymbol(int i) {return symbols[i];}
	void setSymbol(Symbol* symbol, int pos);
	void addSymbol(Symbol* symbol, int pos);
	void moveSymbol(int from, int to);
    void sortSymbols(bool (*cmp)(Symbol *, Symbol *));
	void deleteSymbol(int pos);
	int findSymbolIndex(Symbol* symbol);
	void setSymbolsDirty();
	
	void scaleAllSymbols(double factor);
	
	// Templates
	
	inline int getNumTemplates() const {return templates.size();}
	inline Template* getTemplate(int i) {return templates[i];}
	int getTemplateNumber(Template* temp) const;
	inline void setFirstFrontTemplate(int pos) {first_front_template = pos;}
	inline int getFirstFrontTemplate() const {return first_front_template;}
	void setTemplate(Template* temp, int pos);
	void addTemplate(Template* temp, int pos);									// NOTE: adjust first_front_template manually!
	void deleteTemplate(int pos);												// NOTE: adjust first_front_template manually!
	void setTemplateAreaDirty(Template* temp, QRectF area, int pixel_border);	// marks the respective regions in the template caches as dirty; area is given in map coords (mm). Does nothing if the template is not visible in a widget! So make sure to call this and showing/hiding a template in the correct order!
	void setTemplateAreaDirty(int i);											// this does nothing for i == -1
	void setTemplatesDirty();
	
	// Objects
	
	inline UndoManager& objectUndoManager() {return object_undo_manager;}
	
	inline int getNumLayers() const {return (int)layers.size();}
	inline MapLayer* getLayer(int i) const {return layers[i];}
	inline MapLayer* getCurrentLayer() const {return current_layer;}
	inline int getCurrentLayerIndex() const {return current_layer_index;}
	// TODO: Layer management
	
	int getNumObjects();
	int addObject(Object* object, int layer_index = -1);						// returns the index of the added object in the layer
	void deleteObject(Object* object, bool remove_only);						// remove_only will remove the object from the map, but not call "delete object";
	void setObjectsDirty();
	
	void setObjectAreaDirty(QRectF map_coords_rect);
	void findObjectsAt(MapCoordF coord, float tolerance, bool extended_selection, SelectionInfoVector& out);
	void findObjectsAtBox(MapCoordF corner1, MapCoordF corner2, std::vector<Object*>& out);
	
	void scaleAllObjects(double factor);
	void updateAllObjects();
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
	
	/// Returns if 1) all selected objects are compatible to the given symbol and 2) at least one of the selected objects' symbols is different to the given symbol
	void getSelectionToSymbolCompatibility(Symbol* symbol, bool& out_compatible, bool& out_different);
	
	void includeSelectionRect(QRectF& rect); // enlarges rect to cover the selected objects
	void drawSelection(QPainter* painter, bool force_min_size, MapWidget* widget, RenderableContainer* replacement_renderables = NULL);
	
	void addObjectToSelection(Object* object, bool emit_selection_changed);
	void removeObjectFromSelection(Object* object, bool emit_selection_changed);
	bool isObjectSelected(Object* object);
	bool toggleObjectSelection(Object* object, bool emit_selection_changed);	// returns true if the object was selected, false if deselected
	void clearObjectSelection(bool emit_selection_changed);
	
	// Other settings
	
	inline void setScaleDenominator(int value) {scale_denominator = value;}
	inline int getScaleDenominator() const {return scale_denominator;}
	
	inline bool areGPSProjectionParametersSet() const {return gps_projection_params_set;}
	void setGPSProjectionParameters(const GPSProjectionParameters& params);
	inline const GPSProjectionParameters& getGPSProjectionParameters() const {return *gps_projection_parameters;}
	
	inline bool arePrintParametersSet() const {return print_params_set;}
	void setPrintParameters(int orientation, int format, float dpi, bool show_templates, bool center, float left, float top, float width, float height);
	void getPrintParameters(int& orientation, int& format, float& dpi, bool& show_templates, bool& center, float& left, float& top, float& width, float& height);
	
	void setHasUnsavedChanges(bool has_unsaved_changes = true);
	inline bool hasUnsavedChanged() const {return unsaved_changes;}
	
	// Static
	
	static MapColor* getCoveringWhite() {return &covering_white;}
	static MapColor* getCoveringRed() {return &covering_red;}
	static LineSymbol* getCoveringWhiteLine() {return covering_white_line;}
	static LineSymbol* getCoveringRedLine() {return covering_red_line;}
	
protected:
    /// Attempts to load an OCAD 7/8 map from the specified file. Returns true on success.
    bool loadFromOCAD78(const QString &path, MapEditorController* map_editor = NULL);
    /// Attempts to load a map in native file format from the specified file. Returns true on success.
    bool loadFromNative(const QString &path, MapEditorController* map_editor = NULL);

signals:
	void gotUnsavedChanges();
	
	void colorAdded(int pos, MapColor* color);
	void colorChanged(int pos, MapColor* color);
	void colorDeleted(int pos, MapColor* old_color);
	
	void symbolAdded(int pos, Symbol* symbol);
	void symbolChanged(int pos, Symbol* new_symbol, Symbol* old_symbol);
	void symbolDeleted(int pos, Symbol* old_symbol);
	
	void templateAdded(int pos, Template* temp);
	void templateChanged(int pos, Template* temp);
	void templateDeleted(int pos, Template* old_temp);
	
	void selectedObjectsChanged();
	void gpsProjectionParametersChanged();
	
private:
	typedef std::vector<MapColor*> ColorVector;
	typedef std::vector<Symbol*> SymbolVector;
	typedef std::vector<Template*> TemplateVector;
	typedef std::vector<MapLayer*> LayerVector;
	typedef std::vector<MapWidget*> WidgetVector;
	typedef std::vector<MapView*> ViewVector;
	
	struct MapColorSet
	{
		ColorVector colors;
		
		MapColorSet();
		void addReference();
		void dereference();
		
	private:
		int ref_count;
	};
	
	void checkIfFirstColorAdded();
	void checkIfFirstSymbolAdded();
	void checkIfFirstTemplateAdded();
	
	void adjustColorPriorities(int first, int last);
	
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
	UndoManager object_undo_manager;
	MapLayer* current_layer;
	int current_layer_index;
	WidgetVector widgets;
	ViewVector views;
	RenderableContainer renderables;
	RenderableContainer selection_renderables;
	
	bool gps_projection_params_set;	// have the parameters been set (are they valid)?
	GPSProjectionParameters* gps_projection_parameters;
	
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
	
	int scale_denominator;			// this is the number x if the scale is written as 1:x
	
	bool colors_dirty;				// are there unsaved changes for the colors?
	bool symbols_dirty;				//    ... for the symbols?
	bool templates_dirty;			//    ... for the templates?
	bool objects_dirty;				//    ... for the objects?
	bool unsaved_changes;			// are there unsaved changes for any component?
	
	// Static
	
	static bool static_initialized;
	static MapColor covering_white;
	static MapColor covering_red;
	static LineSymbol* covering_white_line;
	static LineSymbol* covering_red_line;
	
	static const int least_supported_file_format_version;
	static const int current_file_format_version;
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
	
	void save(QFile* file);
	void load(QFile* file);
	
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
	/// Note 1: The transform is combined with the painter's existing tranfsorm.
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
	
	QHash<Template*, TemplateVisibility*> template_visibilities;
	
	WidgetVector widgets;
};

#endif
