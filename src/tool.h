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


#ifndef _OPENORIENTEERING_TOOL_H_
#define _OPENORIENTEERING_TOOL_H_

#include <vector>

#include <QHash>
#include <QScopedPointer>
#include <QAction>

#include "map_coord.h"

class Map;
class Object;
class MapEditorController;
class MapWidget;
class ConstrainAngleToolHelper;
class SnappingToolHelper;
class TextObject;
class Renderable;
class MapRenderables;
typedef std::vector<Renderable*> RenderableVector;

/// Represents a tool which works usually by using the mouse.
/// The given button is unchecked when the tool is destroyed.
/// NOTE 1: do not change any settings (e.g. status bar text) in the constructor, as another tool still might be
///         active at that point in time! Instead, use the init() method.
/// NOTE 2: this class provides a general but cumbersome interface. If you want to write a simple tool and can live
///         with some limitations, consider using MapEditorToolBase instead.
class MapEditorTool : public QObject
{
public:
	enum Type
	{
		Other = 0,
		EditPoint = 1,
		EditLine = 2
	};
	
	/// The numbers correspond to the columns in point-handles.png
	enum PointHandleType
	{
		StartHandle = 0,
		EndHandle = 1,
		NormalHandle = 2,
		CurveHandle = 3,
		DashHandle = 4
	};
	
	/// The numbers correspond to the rows in point-handles.png
	enum PointHandleState
	{
		NormalHandleState = 0,
		ActiveHandleState = 1,
		SelectedHandleState = 2,
		DisabledHandleState = 3
	};
	
	MapEditorTool(MapEditorController* editor, Type type, QAction* tool_button = NULL);
	virtual ~MapEditorTool();
	
	/// This is called when the tool is activated and should be used to change any settings, e.g. the status bar text
	virtual void init() {}
	/// Must return the cursor which should be used for the tool in the editor windows. TODO: How to change the cursor for all map widgets while active?
	virtual QCursor* getCursor() = 0;
	
	/// All dynamic drawings must be drawn here using the given painter. Drawing is only possible in the area specified by calling map->setDrawingBoundingBox().
	virtual void draw(QPainter* painter, MapWidget* widget) {};
	
	// Mouse input
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) {return false;}
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) {return false;}
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) {return false;}
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) {return false;}
	virtual void leaveEvent(QEvent* event) {}
	
	// Key input
	virtual bool keyPressEvent(QKeyEvent* event) {return false;}
	virtual bool keyReleaseEvent(QKeyEvent* event) {return false;}
	virtual void focusOutEvent(QFocusEvent* event) {}
	
	inline Type getType() const {return type;}
	inline QAction* getAction() const {return tool_button;}
	Map* map() const;
	
	// Helper methods for object handles
	
	/** Must be called once on startup to load the point handle image. */
	static void loadPointHandles();
	
	/** Returns the color in which point handles with the given state will be displayed. */
	static QRgb getPointHandleStateColor(PointHandleState state);
	
	/**
	 * Draws point handles for the given object.
	 * @param hover_point Index of a point which should be drawn in 'active' state. Pass a negative number to disable.
	 * @param painter QPainter object with correct transformation set
	 * @param object Object to draw point handles for
	 * @param widget Map widget in which to draw
	 * @param draw_curve_handles Draw curve handles for path objects?
	 * @param base_state The state in which all points except hover_point should be drawn
	 */
	static void drawPointHandles(int hover_point, QPainter* painter, Object* object, MapWidget* widget, bool draw_curve_handles = true, PointHandleState base_state = NormalHandleState);
	
	/** Draws a single point handle. */
	static void drawPointHandle(QPainter* painter, QPointF point, PointHandleType type, PointHandleState state);
	
	/** Draws a point handle line (used between curve anchor points and their handles). */
	static void drawCurveHandleLine(QPainter* painter, QPointF point, PointHandleType type, QPointF curve_handle, PointHandleState state);
	
	/** Includes the rectangle which encloses all of the object's control points in rect. */
	static void includeControlPointRect(QRectF& rect, Object* object);
	
	/**
	 * Calculates the map positions of the box text handles for the given object.
	 * text_object must be a box text object! Text objects with single
	 * anchor trigger an exception.
	 */
	static void calculateBoxTextHandles(QPointF* out, TextObject* text_object);
	
	// General color definitions which are used by all tools
	
	/// Color for normal (not active) elements
	static const QRgb inactive_color;
	/// Color for active elements (which are hovered over by the cursor)
	static const QRgb active_color;
	/// Color for selected elements
	static const QRgb selection_color;
	
	/// The global point handle image
	static QImage* point_handles;
	
protected:
	/// Can be called by subclasses to display help text in the status bar
	void setStatusBarText(const QString& text);
	
	// Helper methods for editing the selected objects with preview
	void startEditingSelection(MapRenderables& old_renderables, std::vector<Object*>* undo_duplicates = NULL);
	void resetEditedObjects(std::vector<Object*>* undo_duplicates);
	void finishEditingSelection(MapRenderables& renderables, MapRenderables& old_renderables, bool create_undo_step, std::vector<Object*>* undo_duplicates = NULL, bool delete_objects = false);
	void updateSelectionEditPreview(MapRenderables& renderables);
	void deleteOldSelectionRenderables(MapRenderables& old_renderables, bool set_area_dirty);
	
	/**
	 * Finds and returns the point of the given object over which the cursor hovers.
	 * Returns -1 if not hovering over a point.
	 */
	int findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QRectF* selection_extent, MapWidget* widget, MapCoordF* out_handle_pos = NULL);
	inline float distanceSquared(const QPointF& a, const QPointF& b) {float dx = b.x() - a.x(); float dy = b.y() - a.y(); return dx*dx + dy*dy;}
	/// Checks if a mouse button for drawing is currently held, according to the given event
	bool drawMouseButtonHeld(QMouseEvent* event);
	/// Checks if a mouse button for drawing is clicked, according to the given event
	bool drawMouseButtonClicked(QMouseEvent* event);
	
	QAction* tool_button;
	Type type;
	MapEditorController* editor;
};

/// Provides a simple interface to base tools on.
/// Look at the top of the protected section for the methods to override, and below that for some helper methods.
class MapEditorToolBase : public MapEditorTool
{
Q_OBJECT
public:
	MapEditorToolBase(const QCursor cursor, MapEditorTool::Type type, MapEditorController* editor, QAction* tool_button);
	virtual ~MapEditorToolBase();
	
    virtual void init();
    virtual QCursor* getCursor() {return &cursor;}
	
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual bool keyPressEvent(QKeyEvent* event);
    virtual bool keyReleaseEvent(QKeyEvent* event);
	
	/// Draws the preview renderables. Should be overridden to draw custom elements.
    virtual void draw(QPainter* painter, MapWidget* widget);
	
protected slots:
	void updateDirtyRect();
	void objectSelectionChanged();
	void updatePreviewObjectsSlot();
	
protected:
	// Virtual methods to be implemented by the derived class.
	// To access the cursor position, use [click/cur]_pos(_map) to get the mouse press
	// respectively current position on the screen or on the map, or constrained_pos(_map)
	// to get the position constrained by the snap and angle tool helpers, if activated.
	
	/// Can do additional initializations at a time where no other tool is active (in contrast to the constructor)
	virtual void initImpl() {}
	/// Must include the area of all custom drawings into the rect,
	/// which aleady contains the area of the selection preview and activated tool helpers when this method is called.
	/// Must return the size of the pixel border, or -1 to clear the drawing.
	virtual int updateDirtyRectImpl(QRectF& rect) {return -1;}
	/// Must draw the tool's graphics.
	/// The implementation draws the preview renderables.
	/// MapEditorToolBase::draw() draws the activated tool helpers afterwards.
	/// If this is not desired, you can override draw() directly.
	virtual void drawImpl(QPainter* painter, MapWidget* widget);
	/// Must update the status bar text
	virtual void updateStatusText() = 0;
	/// Called when the object selection in the map is changed.
	/// This can happen for example as result of an undo/redo operation.
	virtual void objectSelectionChangedImpl() = 0;
	
	/// Called when the left mouse button is pressed
	virtual void clickPress() {}
	/// Called when the left mouse button is released without a drag operation before
	virtual void clickRelease() {}
	
	/// Called when the mouse is moved without the left mouse button being pressed
	virtual void mouseMove() {}
	
	/// Called when a drag operation is started. This happens when dragging the mouse some pixels
	/// away from the mouse press position. The distance is determined by start_drag_distance.
	/// dragMove() is called immediately after this call to account for the already moved distance.
	virtual void dragStart() {}
	/// Called when the mouse is moved with the left mouse button being pressed
	virtual void dragMove() {}
	/// Called when a drag operation is finished. There is no need to update the edit operation with the
	/// current cursor coordinates in this call as it is ensured that dragMove() is called before.
	virtual void dragFinish() {}
	
	/// Called when a key is pressed down. Return true if the key was processed by the tool.
	virtual bool keyPress(QKeyEvent* event) {return false;}
	/// Called when a key is released. Return true if the key was processed by the tool.
	virtual bool keyRelease(QKeyEvent* event) {return false;}
	
	// Helper methods
	
	/// Call this before editing the selected objects, and finish/abortEditing() afterwards.
	/// Takes care of the preview renderables handling, map dirty flag, and objects edited signal.
	void startEditing();
	void abortEditing();
	void finishEditing(bool delete_objects = false, bool create_undo_step = true);
	
	/// Call this to display changes to the preview objects between startEditing() and finish/abortEditing().
	virtual void updatePreviewObjects();
	
	/// Call this to display changes to the preview objects between startEditing() and finish/abortEditing().
	/// This method delays the actual redraw by a short amount of time to reduce the load when editing many objects.
	void updatePreviewObjectsAsynchronously();
	
	/// If the tool created custom renderables (e.g. with updatePreviewObjects()), draws the preview renderables,
	/// else draws the renderables of the selected map objects.
	void drawSelectionOrPreviewObjects(QPainter* painter, MapWidget* widget, bool draw_opaque = false);
	
	/// Calculates the constrained cursor position from the current position.
	/// Normally it is not needed to call this yourself, as it is called by the mouse handling methods,
	/// only in case a constrain tool helper is activated it is useful to get instant feedback.
	void calcConstrainedPositions(MapWidget* widget);
	
	// Mouse handling
	
	/// Position where the left mouse button was pressed
	QPoint click_pos;
	MapCoordF click_pos_map;
	/// Position where the cursor is currently
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	/// Position where the cursor is currently, constrained by tool helpers, if active
	QPoint constrained_pos;
	MapCoordF constrained_pos_map;
	/// This is set to true when constrained_pos(_map) is a snapped position
	bool snapped_to_pos;
	/// Is the left mouse button pressed and has a drag move been started (by moving the mouse a minimum amount of pixels)?
	bool dragging;
	/// The amount of pixels the mouse has to be moved to start dragging. Defaults to QApplication::startDragDistance().
	int start_drag_distance;
	
	/// Angle tool helper. If activated, it is included in the dirty rect and drawn automatically.
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	/// Snapping tool helper. If a non-null filter is set, it is included in the dirty rect and drawn automatically.
	QScopedPointer<SnappingToolHelper> snap_helper;
	/// An object to exclude from snapping, or NULL to snap to all objects.
	Object* snap_exclude_object;
	
	// Key handling
	
	/// Bitmask containing active modifiers
	Qt::KeyboardModifiers active_modifiers;
	
	// Other
	
	/// The map widget in which the tool was last used
	MapWidget* cur_map_widget;
	
	/// True if startEditing() has been called and editing is not finished yet.
	bool editing;
	
private:
	// Miscellaneous internals
	QCursor cursor;
	bool preview_update_triggered;
	std::vector<Object*> undo_duplicates;
	QScopedPointer<MapRenderables> renderables;
	QScopedPointer<MapRenderables> old_renderables;
};

#endif
