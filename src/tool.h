/*
 *    Copyright 2012, 2013 Thomas Schöps
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

class MainWindow;
class Map;
class MapEditorController;
class MapRenderables;
class MapWidget;
class Object;
class Renderable;
class Symbol;
class TextObject;
typedef std::vector<Renderable*> RenderableVector;

/** 
 * Represents a tool which works usually by using the mouse.
 * The given button is unchecked when the tool is destroyed.
 * 
 * NOTE 1: Do not change any settings (e.g. status bar text) in the constructor,
 * as another tool still might be active at that point in time!
 * Instead, use the init() method.
 * 
 * NOTE 2: This class provides a general but cumbersome interface. If you want
 * to write a simple tool and can live with some limitations, consider using
 * MapEditorToolBase instead.
 */
class MapEditorTool : public QObject
{
Q_OBJECT
public:
	/** Type enum for identification of tools */
	enum Type
	{
		Other = 0,
		EditPoint = 1,
		EditLine = 2,
		DrawPoint = 3,
		DrawPath = 4,
		DrawCircle = 5,
		DrawRectangle = 6,
		DrawText = 7
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
	
	/**
	 * Constructs a new MapEditorTool.
	 * @param editor The MapEditorController in which the tool is used.
	 * @param type The type of the tool (it is safe to use Other if it is not
	 *     necessary to query for this type somewhere).
	 * @param tool_button Optional button which will be unchecked on destruction
	 *     of this tool.
	 */
	MapEditorTool(MapEditorController* editor, Type type, QAction* tool_button = NULL);
	
	/** Destructs the MapEditorTool. */
	virtual ~MapEditorTool();
	
	/**
	 * This is called when the tool is activated and should be used to
	 * change any settings, e.g. the status bar text.
	 */
	virtual void init();
	
	/** Makes this tool inactive in the editor. 
	 *  This will also schedule this tool's deletion. */
	virtual void deactivate();
	
	/** Switch to a default draw tool for the given symbol.
	 *  Makes this tool inactive and schedules its deletion. */
	virtual void switchToDefaultDrawTool(Symbol* symbol) const;
	
	void setEditingInProgress(bool state);
	
	/**
	 * Must return the cursor which should be used for the tool in the editor windows.
	 * TODO: How to change the cursor for all map widgets while active?
	 */
	virtual QCursor* getCursor() = 0;
	
	/**
	 * All dynamic drawings must be drawn here using the given painter.
	 * Drawing is only possible in the area specified by calling map->setDrawingBoundingBox().
	 */
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	// Mouse input
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual void leaveEvent(QEvent* event);
	
	// Key input
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	virtual void focusOutEvent(QFocusEvent* event);
	
	inline Type getType() const {return type;}
	inline QAction* getAction() const {return tool_button;}
	/** Returns whether the touch helper cursor should be used for this tool if it is enabled. */
	inline bool usesTouchCursor() const {return uses_touch_cursor;}
	inline void setAction(QAction* new_tool_button) {tool_button = new_tool_button;}
	
	/** Returns the map being edited. */
	Map* map() const;
	
	/** Returns the map widget being operated on. */
	MapWidget* mapWidget() const;
	
	/** Returns the main window the controller is attached to.*/
	MainWindow* mainWindow() const;
	
	/** Returns the main window the controller is attached to as a QWidget.*/
	QWidget* window() const;
	
	
	static bool isDrawTool(Type type);
	
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
	
	/// The global point handle image (in different resolutions)
	static const int num_point_handle_resolutions = 3;
	static QImage* point_handles[num_point_handle_resolutions];
	
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
	bool uses_touch_cursor;
	static int resolution_index;
	static int resolution_scale_factor;
	MapEditorController* editor;
};

#endif
