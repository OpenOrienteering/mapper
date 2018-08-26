/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_MAP_EDITOR_TOOL_H
#define OPENORIENTEERING_MAP_EDITOR_TOOL_H

#include <Qt>
#include <QtGlobal>
#include <QAction>
#include <QCursor>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QRgb>
#include <QString>
#include <QVariant>

#include "core/map_coord.h"
#include "tools/point_handles.h"

class QEvent;
class QFocusEvent;
class QGestureEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QPointF;
class QWidget;

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapEditorController;
class MapWidget;
class Object;
class Symbol;


/** 
 * @brief An abstract tool for editing a map.
 * 
 * A map editor tool uses mouse and key input to modify a map.
 * 
 * The given tool_button is unchecked when the tool is destroyed.
 * 
 * When deriving from MapEditorTool, do not make changes on the map editor or
 * window (e.g. status bar text) in the constructor. Another tool might still
 * be active at that point in time! Instead, reimplement the init() method.
 * 
 * This class provides a general but cumbersome interface. If you want to write
 * a simple tool and can live with some limitations, consider using
 * MapEditorToolBase instead.
 */
class MapEditorTool : public QObject
{
Q_OBJECT
public:
	/**
	 * @brief Types of tool.
	 */
	enum Type
	{
		EditPoint     = 1,
		EditLine      = 2,
		DrawPoint     = 3,
		DrawPath      = 4,
		DrawCircle    = 5,
		DrawRectangle = 6,
		DrawText      = 7,
		DrawFreehand  = 8,
		Pan           = 9,
		Other         = 0
	};
	
	/**
	 * @brief Constructs a new MapEditorTool.
	 * 
	 * @param editor       The MapEditorController in which the tool is used.
	 * @param type         The type of the tool. It is safe to use Other if it
	 *                     is not necessary to query for this type somewhere.
	 * @param tool_action  Optional button which will be unchecked on
	 *                     destruction of this tool.
	 */
	MapEditorTool(MapEditorController* editor, Type type, QAction* tool_action = nullptr);
	
	/**
	 * @brief Destructs the MapEditorTool.
	 */
	~MapEditorTool() override;
	
	/**
	 * @brief Performs initialization when the tool becomes active.
	 * 
	 * This method is called by the map editor when the tool shall become
	 * active. Reimplementations may make changes to the map editor or window
	 * (e.g. set status bar text) which are not allowed in the constructor.
	 * 
	 * Note that this method by call several times, without any deinitialization
	 * in between.
	 * 
	 * Reimplementations shall call parent implementations.
	 * 
	 * The implementation in this class marks the tool's action as checked.
	 */
	virtual void init();
	
	/**
	 * @brief Makes this tool inactive in the editor.
	 * 
	 * Remplementations shall call parent implementations.
	 * 
	 * This implementation will always schedule the tool's deletion. But
	 * marking the tool's action as not checked is left to the destructor
	 */
	virtual void deactivate();
	
	/**
	 * @brief Switch to a default draw tool for the given symbol.
	 * 
	 * Makes this tool inactive and schedules its deletion.
	 * 
	 * @todo Review for refactoring: no reimplementation found, maybe not in right class?
	 */
	virtual void switchToDefaultDrawTool(const Symbol* symbol) const;
	
	/**
	 * @brief Returns the cursor which should be used for the tool in the editor windows.
	 * 
	 * @todo How to change the cursor for all map widgets while a tool is active?
	 */
	virtual const QCursor& getCursor() const = 0;
	
	/**
	 * @brief Draws the tool's visualisation for a map widget.
	 * 
	 * All dynamic drawing must be done here using the given painter. Drawing
	 * is only possible in the area specified by calling map->setDrawingBoundingBox().
	 */
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	// Mouse input
	virtual bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget);
	virtual void leaveEvent(QEvent* event);
	
	// Key input
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	virtual void focusOutEvent(QFocusEvent* event);
	
	// Input method support
	virtual bool inputMethodEvent(QInputMethodEvent* event);
	virtual QVariant inputMethodQuery(Qt::InputMethodQuery property, const QVariant& argument) const;
	
	// Gesture input
	virtual bool gestureEvent(QGestureEvent* event, MapWidget* widget);
	
	/**
	 * This is called to signal the start of a gesture.
	 * 
	 * Tools may reimplement this if they want to cancel ongoing interactions
	 * such as dragging.
	 */	
	virtual void gestureStarted();
	
	/**
	 * @brief Returns the type of this tool.
	 */
	Type toolType() const { return tool_type; }
	
	
	/**
	 * @brief Returns the action which repesents this tool.
	 */
	QAction* toolAction() const { return tool_action; }
	
	
	/**
	 * @brief Returns whether to use the touch helper cursor for this tool.
	 */
	bool usesTouchCursor() const { return uses_touch_cursor; }
	
	
	/** 
	 * @brief Returns the map being edited.
	 */
	Map* map() const;
	
	/**
	 * @brief Returns the map widget being operated on.
	 */
	MapWidget* mapWidget() const;
	
	/**
	 * @brief Returns the main window the controller is attached to.
	 */
	MainWindow* mainWindow() const;
	
	/**
	 * @brief Returns the main window the controller is attached to as a QWidget.
	 * 
	 * This function can be used without the need to include main_window.h.
	 */
	QWidget* window() const;
	
	
	/**
	 * Returns true if Mapper is configured to finish drawing on right click.
	 */
	bool drawOnRightClickEnabled() const { return draw_on_right_click; }
	
	
	/**
	 * @brief Returns whether an editing operation is currently in progress.
	 * 
	 * Some editing operation, such as drawing a path, need several user inputs
	 * over some period of time. This flag indicates that such an operation is
	 * currently active, limiting the number of other actions which may be
	 * triggered in this time.
	 * 
	 * @return Returns true if there is an ongoing edition operation, false otherwise.
	 */
	bool editingInProgress() const { return editing_in_progress; }
	
	/**
	 * @brief Finishes editing if it is currently in progress.
	 * 
	 * Deriving functions shall call this class' implementation
	 * (which calls setEditingInProgress(false)).
	 */
	virtual void finishEditing();
	
	
	/**
	 * @brief Returns true if the given tool is for drawing new objects.
	 * 
	 * @todo This shall be rewritten as virtual/reimplemented function.
	 */
	bool isDrawTool() const;
	
	/**
	 * @brief Returns the point handles utility for this tool.
	 */
	const PointHandles& pointHandles() const { return point_handles; }
	
	/**
	 * @brief The factor by which all drawing shall be scaled.
	 * 
	 * @see PointHandles::scaleFactor()
	 */
	unsigned int scaleFactor() const { return scale_factor; }
	
	/**
	 * @brief A value representing how close the user must click or hover to select a point.
	 */
	qreal clickTolerance() const { return click_tolerance; }
	
	/**
	 * The number of pixels the mouse has to be moved to start dragging.
	 */
	int startDragDistance() const { return start_drag_distance; }
	
	
	// General color definitions which are used by all tools
	
	/// Color for normal (not active) elements
	static const QRgb inactive_color;
	/// Color for active elements (which are hovered over by the cursor)
	static const QRgb active_color;
	/// Color for selected elements
	static const QRgb selection_color;
	
protected:
	/**
	 * Returns a cursor which is scaled to the screen's resolution.
	 * 
	 * Return standard cursor shapes unchanged, but scales pixmap cursors
	 * from (default) 96 dpi to the screen's physical dpi (according to the
	 * settings) if the difference is significant.
	 */
	static QCursor scaledToScreen(const QCursor& unscaled_cursor);
	
	/**
	 * Sets the flag which indicates whether the touch cursor shall be used.
	 * 
	 * @see usesTouchCursor()
	 */
	void useTouchCursor(bool enabled);
	
	/**
	 * @brief Sets a flag which indicates an active editing operation.
	 * 
	 * This function takes care of informing the MapEditorController about the change.
	 * 
	 * @see editingInProgress()
	 */
	void setEditingInProgress(bool state);
	
	
	/**
	 * @brief Sends text to the window's status bar.
	 */
	void setStatusBarText(const QString& text);
	
	
	/**
	 * @brief Draws a selection box for the given corner points.
	 * 
	 * A selection box is drawn while selecting objects by dragging.
	 */
	void drawSelectionBox(QPainter* painter, MapWidget* widget, const MapCoordF& corner1, const MapCoordF& corner2) const;
	
	
	/**
	 * @brief Finds and returns the point of the given object over which the cursor hovers.
	 * 
	 * Returns no_point if not hovering over a point.
	 */
	MapCoordVector::size_type findHoverPoint(const QPointF& cursor, const MapWidget* widget, const Object* object, bool include_curve_handles, MapCoordF* out_handle_pos = nullptr) const;
	
	
	/**
	 * Checks if the given buttons contain one which controls drawing.
	 * 
	 * To be used for mouse move events.
	 */
	bool containsDrawingButtons(Qt::MouseButtons buttons) const;
	
	/**
	 * Checks if the given event was triggered by press or release of a mouse button for drawing.
	 * 
	 * To be used for press and release events.
	 */
	bool isDrawingButton(Qt::MouseButton button) const;
	
	
private:
	/**
	 * Updates cached settings.
	 */
	void settingsChanged();
	
	
protected:
	/**
	 * @brief The map editor which uses this tool.
	 */
	MapEditorController* const editor;
	
private:	
	PointHandles point_handles;
	QPointer<QAction> tool_action;
	Type tool_type;
	qreal click_tolerance     = 4;
	int start_drag_distance   = 4;
	unsigned int scale_factor = 1;
	bool editing_in_progress  = false;
	bool uses_touch_cursor    = true;
	bool draw_on_right_click  = false;
};


}  // namespace OpenOrienteering

#endif
