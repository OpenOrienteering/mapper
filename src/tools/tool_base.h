/*
 *    Copyright 2012, 2014 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_EDITOR_TOOL_BASE_H
#define OPENORIENTEERING_MAP_EDITOR_TOOL_BASE_H

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include <Qt>
#include <QCursor>
#include <QObject>
#include <QPoint>
#include <QPointF>

#include <QPointer>

#include "core/map_coord.h"
#include "core/objects/object.h"
#include "tools/tool.h"

class QAction;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRectF;

namespace OpenOrienteering {

class ConstrainAngleToolHelper;
class KeyButtonBar;
class MapEditorController;
class MapRenderables;
class MapWidget;
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
	
	/**
	 * An edited item consist of an active object and a corresponding duplicate.
	 * 
	 * The active object is owned by the map.
	 * The duplicate is owned by this item.
	 * The duplicate may be used when for restoring the active's object's data
	 * when aborting editing,
	 * or it can be transfered to an undo step when committing the changes.
	 */
	struct EditedItem
	{
		Object* active_object;
		std::unique_ptr<Object> duplicate;
		
		// Convenience constructor
		EditedItem(Object* active_object);
		
		// All methods which are needed for efficient containers
		EditedItem() noexcept = default;
		EditedItem(const EditedItem& prototype);
		EditedItem(EditedItem&& prototype) noexcept;
		EditedItem& operator=(const EditedItem& prototype);
		EditedItem& operator=(EditedItem&& prototype) noexcept;
		
		~EditedItem() = default;
		
		bool isModified() const;
	};
	
public:
	/**
	 * An accessor to the active objects in a std::vector<EditedItem>.
	 * 
	 * This makes the edited objects available in range-for loops
	 * by providing forward iteration, but hides the implementation details.
	 */
	class ObjectsRange
	{
	private:
		using container = std::vector<EditedItem>;
		container::iterator range_begin;
		container::iterator range_end;
		
	public:
		explicit ObjectsRange(container& items) : range_begin { items.begin() }, range_end { items.end() } {}
		struct iterator : private container::iterator
		{
			explicit iterator(const container::iterator& it) noexcept : container::iterator { it } {}
			explicit iterator(container::iterator&& it) noexcept : container::iterator { std::move(it) } {}
			Object* operator*() noexcept  { return container::iterator::operator*().active_object; }
			Object* operator->() noexcept { return operator*(); }
			iterator& operator++() noexcept { container::iterator::operator++(); return *this; }
			bool operator==(const iterator& rhs) noexcept { return static_cast<container::iterator>(*this)==static_cast<container::iterator>(rhs); }
			bool operator!=(const iterator& rhs) noexcept { return !operator==(rhs); }
		};
		iterator begin() noexcept { return iterator { range_begin }; }
		iterator end() noexcept { return iterator { range_end }; }
	};
	
	
	MapEditorToolBase(const QCursor& cursor, MapEditorTool::Type type, MapEditorController* editor, QAction* tool_action);
	~MapEditorToolBase() override;
	
	void init() override;
	const QCursor& getCursor() const override;
	
	/**
	 * Updates the saved current position (raw and constrained), map widget, and modifiers.
	 * 
	 * This function is called by the other mouse*Event handlers of this class.
	 * Derived classes which override the handlers may wish to call this, too.
	 */
	void mousePositionEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	bool keyReleaseEvent(QKeyEvent* event) override;
	
	/// Draws the preview renderables. Should be overridden to draw custom elements.
	void draw(QPainter* painter, MapWidget* widget) override;
	
	void finishEditing() override;
	
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
	virtual void initImpl();
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
	
	void gestureStarted() override;
	
	void startDragging();
	void updateDragging();
	void finishDragging();
	void cancelDragging();
	/// Is the left mouse button pressed and has a drag move been started (by moving the mouse a minimum amount of pixels)?
	bool isDragging() const { return dragging; }
	
	/// Called when a drag operation is started. This happens when dragging the mouse some pixels
	/// away from the mouse press position. The distance is determined by start_drag_distance.
	/// dragMove() is called immediately after this call to account for the already moved distance.
	virtual void dragStart();
	/// Called when the mouse is moved with the left mouse button being pressed
	virtual void dragMove();
	/// Called when a drag operation is finished. There is no need to update the edit operation with the
	/// current cursor coordinates in this call as it is ensured that dragMove() is called before.
	virtual void dragFinish();
	
	virtual void dragCanceled();
	
	/// Called when a key is pressed down. Return true if the key was processed by the tool.
	virtual bool keyPress(QKeyEvent* event);
	/// Called when a key is released. Return true if the key was processed by the tool.
	virtual bool keyRelease(QKeyEvent* event);
	
	// Helper methods
	
	/**
	 * Applies the constraint helpers and generates a pointer event.
	 * 
	 * This function re-applies the constraint helpers to the current pointer
	 * position. Then it calls either mouseMove() or dragMove(), depending on
	 * the current state.
	 */
	void reapplyConstraintHelpers();
	
	/// Call this before editing the selected objects, and finish/abortEditing() afterwards.
	/// Takes care of the preview renderables handling, map dirty flag, and objects edited signal.
	void startEditing();
	void startEditing(Object* object);
	void startEditing(const std::set<Object*>& objects);
	void abortEditing();
	
	ObjectsRange editedObjects() { return ObjectsRange { edited_items }; }
	bool editedObjectsModified() const;
	void resetEditedObjects();
	
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
	
	/**
	 * Calculates the constrained cursor position for the current position and map widget.
	 * 
	 * Key event handlers which change constraint helper activation must call
	 * this to be able to visualize the effect of the change. Overrides of
	 * mouse*Event handlers shall call mousePositionEvent() instead. This is
	 * also done by the default implementation of these handlers. Thus it is not
	 * neccessary to call this explicitly from clickPress(), clickRelease(),
	 * mouseMove(), dragStart(), dragMove(), or dragFinish() handlers.
	 */
	void updateConstrainedPositions();
	
#ifdef MAPPER_DEVELOPMENT_BUILD
	
protected slots:
	/**
	 * Sends simulated sequences of mouse button-press, move, button-release.
	 * 
	 * The mouse position is modified randomly for every single event.
	 * After each event, there is a small delay.
	 * 
	 * This a debugging aid. The implementation is meant to be modified as
	 * needed with regard to modifier keys, stopping the simulation, etc.
	 * It is to be started from an event handler via a single-shot timer:
	 * 
	 *     QTimer::singleShot(200, this, SLOT(generateNextSimulatedEvent()));
	 */
	void generateNextSimulatedEvent();
	
private:
	int simulation_state = 0;
	
#endif  // MAPPER_DEVELOPMENT_BUILD
	
protected:
	// Mouse handling
	
	/// Position where the left mouse button was pressed, with no constraints applied.
	QPoint click_pos;
	MapCoordF click_pos_map;
	/// Position where the left mouse button was pressed, constrained by tool helpers, if active.
	QPointF constrained_click_pos;
	MapCoordF constrained_click_pos_map;
	/// Position where the cursor is currently
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	/// Position where the cursor is currently, constrained by tool helpers, if active
	QPointF constrained_pos;
	MapCoordF constrained_pos_map;
	/// This is set to true when constrained_pos(_map) is a snapped position
	bool snapped_to_pos;
	/// The amount of pixels the mouse has to be moved to start dragging.
	/// Defaults to Settings::getInstance().getStartDragDistancePx(), but can be modified.
	int effective_start_drag_distance;
	
	/// Angle tool helper. If activated, it is included in the dirty rect and drawn automatically.
	std::unique_ptr<ConstrainAngleToolHelper> angle_helper;
	/// Snapping tool helper. If a non-null filter is set, it is included in the dirty rect and drawn automatically.
	std::unique_ptr<SnappingToolHelper> snap_helper;
	/// An object to exclude from snapping, or nullptr to snap to all objects.
	Object* snap_exclude_object = nullptr;
	
	// Key handling
	
	/// Bitmask containing active modifiers
	Qt::KeyboardModifiers active_modifiers;
	
	// Other
	
	/// The map widget in which the tool was last used
	MapWidget* cur_map_widget;
	
	/// Must be set by derived classes if a key button bar is used.
	/// MapEditorToolBase will take care of including its modifiers into
	/// active_modifiers and destruct it when the tool is destructed.
	QPointer<KeyButtonBar> key_button_bar;
	
private:
	// Miscellaneous internals
	QCursor cursor;
	bool preview_update_triggered = false;
	bool dragging                 = false;
	bool dragging_canceled        = false;
	std::unique_ptr<MapRenderables> renderables;
	std::unique_ptr<MapRenderables> old_renderables;
	std::vector<EditedItem> edited_items;
};


}  // namespace OpenOrienteering
#endif
