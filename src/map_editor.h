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


#ifndef _OPENORIENTEERING_MAP_EDITOR_H_
#define _OPENORIENTEERING_MAP_EDITOR_H_

#include <vector>

#include <QAction>
#include <QDockWidget>
#include <QScopedPointer>
#include <QClipboard>

#include "gui/main_window_controller.h"
#include "map.h"

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
class QToolBar;
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
class GeoreferencingDialog;
class ReopenTemplateDialog;
class PieMenu;
typedef std::vector<Renderable*> RenderableVector;

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
	
	/// If this is set to true (usually by the current tool), undo/redo and saving the map is deactivated
	void setEditingInProgress(bool value);
	
	/// Adds a a floating dock widget to the main window. Adjusts some geometric properties.
	void addFloatingDockWidget(QDockWidget* dock_widget);
	
	void setEditorActivity(MapEditorActivity* new_activity);
	inline MapEditorActivity* getEditorActivity() const {return editor_activity;}
	
	inline Map* getMap() const {return map;}
	inline MapWidget* getMainWidget() const {return map_widget;}
	inline SymbolWidget* getSymbolWidget() const {return symbol_widget;}
	
	inline bool existsTemplatePositionDockWidget(Template* temp) const {return template_position_widgets.contains(temp);}
	inline TemplatePositionDockWidget* getTemplatePositionDockWidget(Template* temp) const {return template_position_widgets.value(temp);}
	void addTemplatePositionDockWidget(Template* temp);
	void removeTemplatePositionDockWidget(Template* temp); // should be called by the dock widget if it is closed or the template deleted; deletes the dock widget
	
	/**
	 * Returns the action identified by id if it exists, or NULL.
	 * This allows the reuse of the controller's actions in dock widgets etc.
	 */
	QAction* getAction(const char* id);
	
	virtual bool save(const QString& path);
	virtual bool load(const QString& path);
	
	virtual void attach(MainWindow* window);
	virtual void detach();
	
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	
public slots:
	/** Makes the print/export dock widget visible, and configures it for 
	 *  the given task (which is of type PrintWidget::TaskFlags). */
	void printClicked(int task);
	
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void clearUndoRedoHistory();
	
	void showGrid();
	void configureGrid();
	
	void pan();
	void zoomIn();
	void zoomOut();
	void setCustomZoomFactorClicked();
	
	void hatchAreas(bool checked);
	void baselineView(bool checked);
	void hideAllTemplates(bool checked);
	void overprintingSimulation(bool checked);
	
	void coordsDisplayChanged();
	
	void showSymbolWindow(bool show);
	void showColorWindow(bool show);
	void loadSymbolsFromClicked();
	void loadColorsFromClicked();
	void scaleAllSymbolsClicked();
	
	void scaleMapClicked();
	void rotateMapClicked();
	void mapNotesClicked();
	
	void showTemplateWindow(bool show);
	void openTemplateClicked();
	void reopenTemplateClicked();
	void closedTemplateAvailabilityChanged();
	
	void editGeoreferencing();
	
	void selectedSymbolsChanged();
	void objectSelectionChanged();
	void selectedSymbolsOrObjectsChanged();
	void undoStepAvailabilityChanged();
	void clipboardChanged(QClipboard::Mode mode);
	void updatePasteAvailability();
	
	/** This slot cause the map editor to check the presence of spot colors,
	 *  and to disable overprinting simulation if there are no spot colors. */
	void spotColorPresenceChanged(bool has_spot_colors);
	
	void showWholeMap();
	
	void editToolClicked();
	void editLineToolClicked();
	void drawPointClicked();
	void drawPathClicked();
	void drawCircleClicked();
	void drawRectangleClicked();
	void drawTextClicked();
	
	void duplicateClicked();
	void switchSymbolClicked();
	void fillBorderClicked();
	void selectObjectsClicked(bool select_exclusively);	// Selects all objects with the selected symbol(s)
	void switchDashesClicked();
	void connectPathsClicked();
	void cutClicked();
	void cutHoleClicked();
	void cutHoleCircleClicked();
	void cutHoleRectangleClicked();
	void rotateClicked();
	void rotatePatternClicked();
	void scaleClicked();
	void measureClicked(bool checked);
	void booleanUnionClicked();
	void booleanIntersectionClicked();
	void booleanDifferenceClicked();
	void booleanXOrClicked();
	void convertToCurvesClicked();
	void simplifyPathClicked();
	void cutoutPhysicalClicked();
	void cutawayPhysicalClicked();
	
	void paintOnTemplateClicked(bool checked);
	void paintOnTemplateSelectClicked();
	
	void templateAdded(int pos, Template* temp);
	void templateDeleted(int pos, Template* temp);
	
	void importGeoFile(const QString& filename);
	bool importMapFile(const QString& filename);
	void importClicked();
	
	/** Sets the enabled state of actions which change how the map is rendered,
	 *  such as with grid, with templates, with overprinting simulation. */
	void setViewOptionsEnabled(bool enabled = true);
	
	/** 
	 * Save the current toolbar and dock widget positions
	 */
	void saveWindowState();
	
	/**
	 * Restore previously saved toolbar and dock widget positions
	 */
	void restoreWindowState();
	
signals:
	void templatePositionDockWidgetClosed(Template* temp);
	
protected slots:
	void projectionChanged();
	void georeferencingDialogFinished();
	
private:
	void setMap(Map* map, bool create_new_map_view);
	
	/// Updates enabled state of all widgets
	void updateWidgets();
	
	QAction* newAction(const char* id, const QString& tr_text, QObject* receiver, const char* slot, const char* icon = NULL, const QString& tr_tip = QString::null, const QString& whatsThisLink = QString::null);
	QAction* newCheckAction(const char* id, const QString& tr_text, QObject* receiver, const char* slot, const char* icon = NULL, const QString& tr_tip = QString::null, const QString& whatsThisLink = QString::null);
	QAction* newToolAction(const char* id, const QString& tr_text, QObject* receiver, const char* slot, const char* icon = NULL, const QString& tr_tip = QString::null, const QString& whatsThisLink = QString::null);
	QAction* findAction(const char* id);
	void assignKeyboardShortcuts();
	void createMenuAndToolbars();
	void createPieMenu(PieMenu* menu);
	
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
	QHash<QString, QAction *> actionsById;
	
	EditorDockWidget* print_dock_widget;
	PrintWidget* print_widget;
	
	QAction* undo_act;
	QAction* redo_act;
	QAction* cut_act;
	QAction* copy_act;
	QAction* paste_act;
	QAction* clear_undo_redo_history_act;
	
	QAction* pan_act;
	QAction* show_grid_act;
	QAction* hatch_areas_view_act;
	QAction* baseline_view_act;
	QAction* hide_all_templates_act;
	QAction* overprinting_simulation_act;
	
	QAction* map_coordinates_act;
	QAction* projected_coordinates_act;
	QAction* geographic_coordinates_act;
	QAction* geographic_coordinates_dms_act;
	
	QAction* color_window_act;
	EditorDockWidget* color_dock_widget;
	
	QAction* symbol_window_act;
	EditorDockWidget* symbol_dock_widget;
	SymbolWidget* symbol_widget;
	
	QAction* template_window_act;
	EditorDockWidget* template_dock_widget;
	QAction* reopen_template_act;
	
	QAction* edit_tool_act;
	QAction* edit_line_tool_act;
	QAction* draw_point_act;
	QAction* draw_path_act;
	QAction* draw_circle_act;
	QAction* draw_rectangle_act;
	QAction* draw_text_act;
	
	QAction* duplicate_act;
	QAction* switch_symbol_act;
	QAction* fill_border_act;
	QAction* switch_dashes_act;
	QAction* connect_paths_act;
	QAction* cut_tool_act;
	QMenu* cut_hole_menu;
	QAction* cut_hole_act;
	QAction* cut_hole_circle_act;
	QAction* cut_hole_rectangle_act;
	QAction* rotate_act;
	QAction* rotate_pattern_act;
	QAction* scale_act;
	QAction* measure_act;
	EditorDockWidget* measure_dock_widget;
	QAction* boolean_union_act;
	QAction* boolean_intersection_act;
	QAction* boolean_difference_act;
	QAction* boolean_xor_act;
	QAction* convert_to_curves_act;
	QAction* simplify_path_act;
	QAction* cutout_physical_act;
	QAction* cutaway_physical_act;
	
	QAction* paint_on_template_act;
	Template* last_painted_on_template;
	
	QFrame* statusbar_zoom_frame;
	QLabel* statusbar_cursorpos_label;
	
	QToolBar* toolbar_view;
	QToolBar* toolbar_drawing;
	QToolBar* toolbar_editing;
	QToolBar* toolbar_advanced_editing;
	
	QScopedPointer<GeoreferencingDialog> georeferencing_dialog;
	QScopedPointer<ReopenTemplateDialog> reopen_template_dialog;
	
	QHash<Template*, TemplatePositionDockWidget*> template_position_widgets;
	
	bool being_destructed;
};

/// Custom QDockWidget which unchecks the associated menu action when closed and delivers a notification to its child
class EditorDockWidget : public QDockWidget
{
Q_OBJECT
public:
	EditorDockWidget(const QString title, QAction* action, MapEditorController* editor, QWidget* parent = NULL);
	virtual bool event(QEvent* event);
	
private:
	QAction* action;
	MapEditorController* editor;
};

/// Helper class which disallows deselecting the checkable action by the user
class MapEditorToolAction : public QAction
{
Q_OBJECT
public:
	MapEditorToolAction(const QIcon& icon, const QString& text, QObject* parent);
	
signals:
	void activated();
	
private slots:
	void triggeredImpl(bool checked);
};

#endif
