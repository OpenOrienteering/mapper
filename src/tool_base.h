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


#ifndef _OPENORIENTEERING_TOOL_BASE_H_
#define _OPENORIENTEERING_TOOL_BASE_H_

#include <vector>

#include <QHash>
#include <QScopedPointer>
#include <QAction>

#include "tool.h"

class KeyButtonBar;
class ConstrainAngleToolHelper;
class SnappingToolHelper;

/**
 * Provides a simple interface to base tools on.
 * 
 * Look at the top of the protected section for the methods to override,
 * and below that for some helper methods.
 */
class MapEditorToolBase : public MapEditorTool
{
Q_OBJECT
public:
	MapEditorToolBase(const QCursor cursor, MapEditorTool::Type tool_type, MapEditorController* editor, QAction* tool_action);
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
	virtual int updateDirtyRectImpl(QRectF& rect);
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
	virtual void clickPress();
	/// Called when the left mouse button is released without a drag operation before
	virtual void clickRelease();
	
	/// Called when the mouse is moved without the left mouse button being pressed
	virtual void mouseMove();
	
	/// Called when a drag operation is started. This happens when dragging the mouse some pixels
	/// away from the mouse press position. The distance is determined by start_drag_distance.
	/// dragMove() is called immediately after this call to account for the already moved distance.
	virtual void dragStart();
	/// Called when the mouse is moved with the left mouse button being pressed
	virtual void dragMove();
	/// Called when a drag operation is finished. There is no need to update the edit operation with the
	/// current cursor coordinates in this call as it is ensured that dragMove() is called before.
	virtual void dragFinish();
	
	/// Called when a key is pressed down. Return true if the key was processed by the tool.
	virtual bool keyPress(QKeyEvent* event);
	/// Called when a key is released. Return true if the key was processed by the tool.
	virtual bool keyRelease(QKeyEvent* event);
	
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
	
	/// Activates or deactivates the angle helper, recalculates (un-)constrained cursor position,
	/// and calls mouseMove() or dragMove() to update the tool.
	void activateAngleHelperWhileEditing(bool enable = true);
	
	/// Activates or deactivates the snap helper, recalculates (un-)constrained cursor position,
	/// and calls mouseMove() or dragMove() to update the tool.
	void activateSnapHelperWhileEditing(bool enable = true);
	
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
	/// The amount of pixels the mouse has to be moved to start dragging. Defaults to Settings::getInstance().getStartDragDistancePx().
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
	
	/// Must be set by derived classes if a key button bar is used.
	/// MapEditorToolBase will take care of including its modifiers into
	/// active_modifiers and destruct it when the tool is destructed.
	KeyButtonBar* key_button_bar;
	
private:
	// Miscellaneous internals
	QCursor cursor;
	bool preview_update_triggered;
	std::vector<Object*> undo_duplicates;
	QScopedPointer<MapRenderables> renderables;
	QScopedPointer<MapRenderables> old_renderables;
};

#endif
