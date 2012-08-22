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
		Edit = 1
	};
	
	/// The numbers correspond to the position in point-handles.png
	enum PointHandleType
	{
		StartHandle = 0,
		EndHandle = 1,
		NormalHandle = 2,
		CurveHandle = 3,
		DashHandle = 4
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
	
	// Helper methods for object handles
	static void loadPointHandles();
	static void drawPointHandles(int hover_point, QPainter* painter, Object* object, MapWidget* widget);
	static void drawPointHandle(QPainter* painter, QPointF point, PointHandleType type, bool active);
	static void drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active);
	static void includeControlPointRect(QRectF& rect, Object* object, QPointF* box_text_handles);
	static bool calculateBoxTextHandles(QPointF* out, Map* map);
	
	static const QRgb inactive_color;
	static const QRgb active_color;
	static const QRgb selection_color;
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
	
	int findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QPointF* box_text_handles, QRectF* selection_extent, MapWidget* widget);
	inline float distanceSquared(const QPointF& a, const QPointF& b) {float dx = b.x() - a.x(); float dy = b.y() - a.y(); return dx*dx + dy*dy;}
	
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
	
	/// Draws the preview renderables. Should be overridden to draw custom elements.
    virtual void draw(QPainter* painter, MapWidget* widget);
	
protected slots:
	void updateDirtyRect();
	void objectSelectionChanged();
	
protected:
	// Virtual methods to be implemented by the derived class.
	// To access the cursor position, use [click/cur]_pos(_map) to get the mouse press
	// respectively current position on the screen or on the map.
	
	// To be implemented from MapEditorTool:
	// virtual void draw(QPainter* painter, MapWidget* widget)
	// The implementation of MapEditorToolBase draws the preview renderables.
	
	/// Can do additional initializations at a time where no other tool is active (in contrast to the constructor)
	virtual void initImpl() {}
	/// Must include the area of all custom drawings into the rect,
	/// which aleady contains the area of the selection preview when this method is called.
	/// Must return the size of the pixel border, or -1 to clear the drawing.
	virtual int updateDirtyRectImpl(QRectF& rect) {return -1;};
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
	/// away from the mouse press position. The distance is determined by QApplication::startDragDistance().
	virtual void dragStart() {}
	/// Called when the mouse is moved with the left mouse button being pressed
	virtual void dragMove() {}
	/// Called when a drag operation is finished. There is no need to update the edit operation with the
	/// current cursor coordinates in this call as it is ensured that dragMove() is called before.
	virtual void dragFinish() {}
	
	// Helper methods
	
	/// Call this before editing the selected objects, and finish/abortEditing() afterwards.
	/// Takes care of the preview renderables handling, map dirty flag, and objects edited signal.
	void startEditing();
	void abortEditing();
	void finishEditing();
	
	/// Call this to display changes to the preview objects between startEditing() and finish/abortEditing().
	void updatePreviewObjects();
	
	// Mouse handling
	
	/// Position where the left mouse button was pressed
	QPoint click_pos;
	MapCoordF click_pos_map;
	/// Position where the cursor is currently
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	/// Is the left mouse button pressed and has a drag move been started (by moving the mouse a minimum amount of pixels)?
	bool dragging;
	
	/// The map widget in which the tool was last used
	MapWidget* cur_map_widget;
	
private:
	// Miscellaneous internals
	QCursor cursor;
	bool editing;
	std::vector<Object*> undo_duplicates;
	QScopedPointer<MapRenderables> old_renderables;
	QScopedPointer<MapRenderables> renderables;
};

#endif
