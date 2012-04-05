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


#ifndef _OPENORIENTEERING_MAP_EDITOR_H_
#define _OPENORIENTEERING_MAP_EDITOR_H_

#include <QDockWidget>

#include "main_window.h"
#include "map.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class Template;
class MapView;
class Map;
class MapWidget;
class MapEditorActivity;
class MapEditorTool;
class EditorDockWidget;
class SymbolWidget;
class PrintWidget;
class TemplatePositionDockWidget;

class MapEditorController : public MainWindowController
{
friend class Map;
Q_OBJECT
public:
	enum OperatingMode
	{
		MapEditor = 0,
		SymbolEditor = 1
	};
	
	MapEditorController(OperatingMode mode, Map* map = NULL);
	~MapEditorController();
	
	void setTool(MapEditorTool* new_tool);
	void setEditTool();
	void setOverrideTool(MapEditorTool* new_override_tool);
	inline MapEditorTool* getTool() const {return current_tool;}
	MapEditorTool* getDefaultDrawToolForSymbol(Symbol* symbol);
	
	/// If this is set to true (usually by the current tool), undo/redo is deactivated
	void setEditingInProgress(bool value);
	
	/// Returns true if the widget was shown, false if it was hidden. Set new_widget to true if the widget is new and shown for the first time
	bool toggleFloatingDockWidget(QDockWidget* dock_widget, bool new_widget);
	
	void setEditorActivity(MapEditorActivity* new_activity);
	inline MapEditorActivity* getEditorActivity() const {return editor_activity;}
	
	inline Map* getMap() const {return map;}
	inline MapWidget* getMainWidget() const {return map_widget;}
	inline SymbolWidget* getSymbolWidget() const {return symbol_widget;}
	
	inline bool existsTemplatePositionDockWidget(Template* temp) const {return template_position_widgets.contains(temp);}
	inline TemplatePositionDockWidget* getTemplatePositionDockWidget(Template* temp) const {return template_position_widgets.value(temp);}
	void addTemplatePositionDockWidget(Template* temp);
	void removeTemplatePositionDockWidget(Template* temp); // should be called by the dock widget if it is closed or the template deleted; deletes the dock widget
	
    virtual bool save(const QString& path);
	virtual bool load(const QString& path);
	
    virtual void attach(MainWindow* window);
    virtual void detach();
	
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
	
public slots:
	void printClicked();
	
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	
	void zoomIn();
	void zoomOut();
	void setCustomZoomFactorClicked();
	
	void showSymbolWindow(bool show);
	void showColorWindow(bool show);
	void loadSymbolsFromClicked();
	void loadColorsFromClicked();
	void scaleAllSymbolsClicked();
	
	void scaleMapClicked();
	void mapNotesClicked();
	
	void showTemplateWindow(bool show);
	void openTemplateClicked();
	
	void editGPSProjectionParameters();
	
	void selectedSymbolsChanged();
	void objectSelectionChanged();
	void selectedSymbolsOrObjectsChanged();
	void undoStepAvailabilityChanged();
	
	void showWholeMap();
	
	void editToolClicked(bool checked);
	void drawPointClicked(bool checked);
	void drawPathClicked(bool checked);
	void drawCircleClicked(bool checked);
	void drawTextClicked(bool checked);
	
	void duplicateClicked();
	void switchSymbolClicked();
	void fillBorderClicked();
	void switchDashesClicked();
	void connectPathsClicked();
	void cutClicked(bool checked);
	void cutHoleClicked(bool checked);
	void cutHoleCircleClicked(bool checked);
	void rotateClicked(bool checked);
	void measureClicked(bool checked);
	
	void paintOnTemplateClicked(bool checked);
	void paintOnTemplateSelectClicked();
	
	void templateAdded(int pos, Template* temp);
	void templateDeleted(int pos, Template* temp);
	
signals:
	void templatePositionDockWidgetClosed(Template* temp);
	
private:
	void setMap(Map* map, bool create_new_map_view);
	
    QAction *newAction(const char *id, const QString &tr_text, QObject *receiver, const char *slot, const char *icon = NULL, const QString &tr_tip = QString::null);
    QAction *newCheckAction(const char *id, const QString &tr_text, QObject *receiver, const char *slot, const char *icon = NULL, const QString &tr_tip = QString::null);
    QAction *findAction(const char *id);
    void assignKeyboardShortcuts();
    void createMenuAndToolbars();
	
	void paintOnTemplate(Template* temp);
	void updatePaintOnTemplateAction();
	
	void doUndo(bool redo);
	
	Map* map;
	MapView* main_view;
	MapWidget* map_widget;
	
	OperatingMode mode;
	
	MapEditorTool* current_tool;
	MapEditorTool* override_tool;
	MapEditorActivity* editor_activity;
	
	bool editing_in_progress;

    // Action handling
    QHash<const char *, QAction *> actionsById;
	
	QAction* print_act;
	EditorDockWidget* print_dock_widget;
	PrintWidget* print_widget;
	
	QAction* undo_act;
	QAction* redo_act;
	
	QAction* color_window_act;
	EditorDockWidget* color_dock_widget;
	
	QAction* symbol_window_act;
	EditorDockWidget* symbol_dock_widget;
	SymbolWidget* symbol_widget;
	
	QAction* template_window_act;
	EditorDockWidget* template_dock_widget;
	
	QAction* edit_tool_act;
	QAction* draw_point_act;
	QAction* draw_path_act;
	QAction* draw_circle_act;
	QAction* draw_text_act;
	
	QAction* duplicate_act;
	QAction* switch_symbol_act;
	QAction* fill_border_act;
	QAction* switch_dashes_act;
	QAction* connect_paths_act;
	QAction* cut_tool_act;
	QAction* cut_hole_act;
	QAction* cut_hole_circle_act;
	QAction* rotate_act;
	QAction* measure_act;
	EditorDockWidget* measure_dock_widget;
	
	QAction* paint_on_template_act;
	Template* last_painted_on_template;
	
	QLabel* statusbar_zoom_label;
	QLabel* statusbar_cursorpos_label;
	
	QToolBar* toolbar_view;
	QToolBar* toolbar_drawing;
	QToolBar* toolbar_editing;
	
	QHash<Template*, TemplatePositionDockWidget*> template_position_widgets;
};

class EditorDockWidgetChild : public QWidget
{
Q_OBJECT
public:
	inline EditorDockWidgetChild(QWidget* parent) : QWidget(parent) {}
	virtual void closed() {}
};
/// Custom QDockWidget which unchecks the associated menu action when closed and delivers a notification to its child
class EditorDockWidget : public QDockWidget
{
Q_OBJECT
public:
	EditorDockWidget(const QString title, QAction* action, MapEditorController* editor, QWidget* parent = NULL);
	void setChild(EditorDockWidgetChild* child);
    virtual bool event(QEvent* event);
    virtual void closeEvent(QCloseEvent* event);
signals:
	void closed();
private:
	QAction* action;
	EditorDockWidgetChild* child;
	MapEditorController* editor;
};

/// Represents a type of editing activity, e.g. georeferencing. Only one activity can be active at a time.
/// This is for example used to close the georeferencing window when selecting an edit tool.
class MapEditorActivity : public QObject
{
Q_OBJECT
public:
	virtual ~MapEditorActivity() {}
	
	/// All initializations apart from setting variables like the activity object should be done here instead of in the constructor,
	/// as now the old activity was properly destroyed (including reseting the activity drawing).
	virtual void init() {}
	
	void setActivityObject(void* address) {activity_object = address;}
	inline void* getActivityObject() const {return activity_object;}
	
	/// All dynamic drawings must be drawn here using the given painter. Drawing is only possible in the area specified by calling map->setActivityBoundingBox().
	virtual void draw(QPainter* painter, MapWidget* widget) {};
	
protected:
	void* activity_object;
};

/// Represents a tool which works usually by using the mouse.
/// The given button is unchecked when the tool is destroyed.
/// NOTE: do not change any settings (e.g. status bar text) in the constructor, as another tool still might be
/// active at that point in time! Instead, use the init() method.
class MapEditorTool : public QObject
{
public:
	enum Type
	{
		Other = 0,
		Edit = 1
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
	
	static void loadPointHandles();
	
	static const int click_tolerance;
	static const QRgb inactive_color;
	static const QRgb active_color;
	static const QRgb selection_color;
	static QImage* point_handles;
	
protected:
	/// The numbers correspond to the position in point-handles.png
	enum PointHandleType
	{
		StartHandle = 0,
		EndHandle = 1,
		NormalHandle = 2,
		CurveHandle = 3,
		DashHandle = 4
	};
	
	/// Can be called by subclasses to display help text in the status bar
	void setStatusBarText(const QString& text);
	
	// Helper methods for editing the selected objects with preview
	void startEditingSelection(RenderableVector& old_renderables, std::vector<Object*>* undo_duplicates = NULL);
	void finishEditingSelection(RenderableContainer& renderables, RenderableVector& old_renderables, bool create_undo_step, std::vector<Object*>* undo_duplicates = NULL, bool delete_objects = false);
	void updateSelectionEditPreview(RenderableContainer& renderables);
	void deleteOldSelectionRenderables(RenderableVector& old_renderables, bool set_area_dirty);
	
	// Helper methods for object handles
	void includeControlPointRect(QRectF& rect, Object* object, QPointF* box_text_handles);
	void drawPointHandles(int hover_point, QPainter* painter, Object* object, MapWidget* widget);
	void drawPointHandle(QPainter* painter, QPointF point, PointHandleType type, bool active);
	void drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active);
	bool calculateBoxTextHandles(QPointF* out);
	
	int findHoverPoint(QPointF cursor, Object* object, bool include_curve_handles, QPointF* box_text_handles, QRectF* selection_extent, MapWidget* widget);
	inline float distanceSquared(const QPointF& a, const QPointF& b) {float dx = b.x() - a.x(); float dy = b.y() - a.y(); return dx*dx + dy*dy;}
	
	QAction* tool_button;
	Type type;
	MapEditorController* editor;
};

#endif
