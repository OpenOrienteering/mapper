/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016 Kai Pastor
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


#include "map_editor.h"
#include "map_editor_p.h"

#include <limits>

#include <qmath.h>
#include <QApplication>
#include <QBuffer>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QSizeGrip>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniObject>
#endif

#include "core/georeferencing.h"
#include "gui/configure_grid_dialog.h"
#include "gui/georeferencing_dialog.h"
#include "gui/widgets/action_grid_bar.h"
#include "gui/widgets/compass_display.h"
#include "gui/widgets/symbol_widget.h"
#include "gui/widgets/template_list_widget.h"
#include "color_dock_widget.h"
#include "compass.h"
#include "file_format_registry.h"
#include "map.h"
#include "map_dialog_rotate.h"
#include "map_dialog_scale.h"
#include "map_editor_activity.h"
#include "map_part_undo.h"
#include "object_undo.h"
#include "map_widget.h"
#include "gps_display.h"
#include "gps_temporary_markers.h"
#include "gps_track_recorder.h"
#include "gui/main_window.h"
#include "gui/print_widget.h"
#include "gui/widgets/key_button_bar.h"
#include "gui/widgets/measure_widget.h"
#include "gui/widgets/tag_select_widget.h"
#include "gui/widgets/tags_widget.h"
#include "object_operations.h"
#include "object_text.h"
#include "renderable.h"
#include "settings.h"
#include "symbol.h"
#include "symbol_area.h"
#include "symbol_dialog_replace.h"
#include "symbol_point.h"
#include "template.h"
#include "template_dialog_reopen.h"
#include "template_track.h"
#include "template_position_dock_widget.h"
#include "template_tool_paint.h"
#include "tool.h"
#include "tool_boolean.h"
#include "tool_cut.h"
#include "tool_cut_hole.h"
#include "tool_cutout.h"
#include "tool_distribute_points.h"
#include "tool_draw_circle.h"
#include "tool_draw_freehand.h"
#include "tool_draw_path.h"
#include "tool_draw_point.h"
#include "tool_draw_point_gps.h"
#include "tool_draw_rectangle.h"
#include "tool_draw_text.h"
#include "tool_edit_point.h"
#include "tool_edit_line.h"
#include "tool_fill.h"
#include "tool_pan.h"
#include "tool_rotate.h"
#include "tool_rotate_pattern.h"
#include "tool_scale.h"
#include "undo_manager.h"
#include "util.h"
#include "util/backports.h"
#include "util/scoped_signals_blocker.h"


namespace
{
	/**
	 * Creates a partial, resizable widget overlay over the main window.
	 * 
	 * This widget is meant to be used as a dock widget replacement in the
	 * mobile app. In landscape mode, the child widget is placed on the right
	 * side, spanning the full height. In portrait mode, the child is placed
	 * on the top, spanning the full width.
	 */
	QSplitter* createDockWidgetSubstitute(MainWindow* window, QWidget* child)
	{
		auto splitter = new QSplitter(window);
		splitter->setChildrenCollapsible(false);
		
		auto placeholder = new QWidget();
		
		splitter->setAttribute(Qt::WA_NoSystemBackground, true);
		placeholder->setAttribute(Qt::WA_NoSystemBackground, true);
		child->setAutoFillBackground(true);
		
		auto geometry = window->geometry();
		splitter->setGeometry(geometry);
		if (geometry.height() > geometry.width())
		{
			splitter->setOrientation(Qt::Vertical);
			splitter->addWidget(child);
			splitter->addWidget(placeholder);
		}
		else
		{
			splitter->setOrientation(Qt::Horizontal);
			splitter->addWidget(placeholder);
			splitter->addWidget(child);
		}
		
		return splitter;
	}
	
} // namespace



namespace MimeType {

/// The index of a symbol during drag-and-drop
static const QString oo_objects { QStringLiteral("openorienteering/objects") };

}



// ### MapEditorController ###

MapEditorController::MapEditorController(OperatingMode mode, Map* map, MapView* map_view)
: MainWindowController()
, mobile_mode(MainWindow::mobileMode())
, active_symbol(NULL)
, template_list_widget(nullptr)
, mappart_remove_act(NULL)
, mappart_merge_act(NULL)
, mappart_merge_menu(NULL)
, mappart_move_menu(NULL)
, mappart_selector_box(NULL)
, mappart_merge_mapper(new QSignalMapper(this))
, mappart_move_mapper(new QSignalMapper(this))
{
	this->mode = mode;
	this->map = NULL;
	main_view = NULL;
	symbol_widget = NULL;
	window = NULL;
	editing_in_progress = false;
	
	cut_hole_menu = NULL;
	
	if (map)
		setMapAndView(map, map_view ? map_view : new MapView(this, map));
	
	editor_activity = NULL;
	current_tool = NULL;
	override_tool = NULL;
	last_painted_on_template = NULL;
	
	paste_act = NULL;
	reopen_template_act = NULL;
	overprinting_simulation_act = NULL;
	
	toolbar_view = NULL;
	toolbar_mapparts = NULL;
	toolbar_drawing = NULL;
	toolbar_editing = NULL;
	toolbar_advanced_editing = NULL;
	print_dock_widget = NULL;
	measure_dock_widget = NULL;
	symbol_dock_widget = NULL;
	
	statusbar_zoom_frame = NULL;
	statusbar_cursorpos_label = NULL;
	statusbar_objecttag_label = NULL;
	
	gps_display = NULL;
	gps_track_recorder = NULL;
	compass_display = NULL;
	gps_marker_display = NULL;
	
	actionsById[""] = new QAction(this); // dummy action
	
	connect(mappart_merge_mapper, SIGNAL(mapped(int)), this, SLOT(mergeCurrentMapPartTo(int)));
	connect(mappart_move_mapper, SIGNAL(mapped(int)), this, SLOT(reassignObjectsToMapPart(int)));
}

MapEditorController::~MapEditorController()
{
	paste_act = NULL;
	delete current_tool;
	delete override_tool;
	delete editor_activity;
	delete toolbar_view;
	delete toolbar_drawing;
	delete toolbar_editing;
	delete toolbar_advanced_editing;
	delete toolbar_mapparts;
	delete print_dock_widget;
	delete measure_dock_widget;
	if (color_dock_widget)
		delete color_dock_widget;
	delete symbol_dock_widget;
	if (template_dock_widget)
		delete template_dock_widget;
	if (tags_dock_widget)
		delete tags_dock_widget;
	if (tag_select_dock_widget)
		delete tag_select_dock_widget;
	delete cut_hole_menu;
	delete mappart_merge_act;
	delete mappart_merge_menu;
	delete mappart_move_menu;
	for (QHash<Template*, TemplatePositionDockWidget*>::iterator it = template_position_widgets.begin(); it != template_position_widgets.end(); ++it)
		delete it.value();
	delete gps_display;
	delete gps_track_recorder;
	delete compass_display;
	delete map;
}

bool MapEditorController::isInMobileMode() const
{
	return mobile_mode;
}

void MapEditorController::setTool(MapEditorTool* new_tool)
{
	if (current_tool)
	{
		if (current_tool->editingInProgress())
		{
			current_tool->finishEditing();
		}
		current_tool->deleteLater();
	}
	
	if (!override_tool)
	{
		map->clearDrawingBoundingBox();
		window->setStatusBarText(QString{});
	}
	
	current_tool = new_tool;
	if (current_tool && !override_tool)
	{
		current_tool->init();
	}
	
	if (!override_tool)
		map_widget->setTool(current_tool);
}

void MapEditorController::setEditTool()
{
	if (!current_tool || current_tool->toolType() != MapEditorTool::EditPoint)
		setTool(new EditPointTool(this, edit_tool_act));
}

void MapEditorController::setOverrideTool(MapEditorTool* new_override_tool)
{
	if (override_tool == new_override_tool)
		return;
	
	if (override_tool)
	{
		if (override_tool->editingInProgress())
		{
			override_tool->finishEditing();
		}
		delete override_tool;
	}
	
	map->clearDrawingBoundingBox();
	window->setStatusBarText(QString{});
	
	override_tool = new_override_tool;
	if (override_tool)
	{
		override_tool->init();
	}
	else if (current_tool)
	{
		current_tool->init();
	}
	
	map_widget->setTool(override_tool ? override_tool : current_tool);
}

MapEditorTool* MapEditorController::getDefaultDrawToolForSymbol(const Symbol* symbol)
{
	if (!symbol)
		return new EditPointTool(this, edit_tool_act);
	else if (symbol->getType() == Symbol::Point)
		return new DrawPointTool(this, draw_point_act);
	else if (symbol->getType() == Symbol::Line || symbol->getType() == Symbol::Area || symbol->getType() == Symbol::Combined)
		return new DrawPathTool(this, draw_path_act, false, true);
	else if (symbol->getType() == Symbol::Text)
		return new DrawTextTool(this, draw_text_act);
	else
		Q_ASSERT(false);
	return NULL;
}


void MapEditorController::setEditingInProgress(bool value)
{
	if (value != editing_in_progress)
	{
		editing_in_progress = value;
		
		// Widgets
		map_widget->setGesturesEnabled(!editing_in_progress);
		Q_ASSERT(symbol_widget);
		symbol_widget->setEnabled(!editing_in_progress);
		if (color_dock_widget)
			color_dock_widget->widget()->setEnabled(!editing_in_progress);
		if (mappart_selector_box)
			mappart_selector_box->setEnabled(!editing_in_progress);
		
		// Edit menu
		undo_act->setEnabled(!editing_in_progress && map->undoManager().canUndo());
		redo_act->setEnabled(!editing_in_progress && map->undoManager().canRedo());
		updatePasteAvailability();
		
		// Map menu
		georeferencing_act->setEnabled(!editing_in_progress);
		scale_map_act->setEnabled(!editing_in_progress);
		rotate_map_act->setEnabled(!editing_in_progress);
		map_notes_act->setEnabled(!editing_in_progress);
		
		// Map menu, continued
		const int num_parts = map->getNumParts();
		mappart_add_act->setEnabled(!editing_in_progress);
		mappart_rename_act->setEnabled(!editing_in_progress && num_parts > 0);
		mappart_remove_act->setEnabled(!editing_in_progress && num_parts > 1);
		mappart_move_menu->setEnabled(!editing_in_progress && num_parts > 1);
		mappart_merge_act->setEnabled(!editing_in_progress && num_parts > 1);
		mappart_merge_menu->setEnabled(!editing_in_progress && num_parts > 1);
		
		// Symbol menu
		scale_all_symbols_act->setEnabled(!editing_in_progress);
		load_symbols_from_act->setEnabled(!editing_in_progress);
		
		updateObjectDependentActions();
		updateSymbolDependentActions();
		updateSymbolAndObjectDependentActions();
	}
}

bool MapEditorController::isEditingInProgress() const
{
	return editing_in_progress;
}


void MapEditorController::setEditorActivity(MapEditorActivity* new_activity)
{
	delete editor_activity;
	map->clearActivityBoundingBox();
	
	editor_activity = new_activity;
	if (editor_activity)
		editor_activity->init();
	
	map_widget->setActivity(editor_activity);
}

void MapEditorController::addTemplatePositionDockWidget(Template* temp)
{
	Q_ASSERT(!existsTemplatePositionDockWidget(temp));
	TemplatePositionDockWidget* dock_widget = new TemplatePositionDockWidget(temp, this, window);
	addFloatingDockWidget(dock_widget);
	template_position_widgets.insert(temp, dock_widget);
}

void MapEditorController::removeTemplatePositionDockWidget(Template* temp)
{
	emit(templatePositionDockWidgetClosed(temp));
	
	delete getTemplatePositionDockWidget(temp);
	int num_deleted = template_position_widgets.remove(temp);
	Q_ASSERT(num_deleted == 1);
	Q_UNUSED(num_deleted);
}

void MapEditorController::showPopupWidget(QWidget* child_widget, const QString& title)
{
	if (mobile_mode)
	{
		// FIXME: This is used for KeyButtonBar only
		//        and not related to mobile_mode!
		QSize size = child_widget->sizeHint();
		QRect map_widget_rect = map_widget->rect();
		
		child_widget->setParent(map_widget);
		child_widget->setGeometry(
			qMax(0, qRound(map_widget_rect.center().x() - 0.5f * size.width())),
			qMax(0, map_widget_rect.bottom() - size.height()),
			qMin(size.width(), map_widget_rect.width()),
			qMin(size.height(), map_widget_rect.height())
			);
		child_widget->show();
	}
	else
	{
		QDockWidget* dock_widget = new QDockWidget(title, window);
		dock_widget->setFeatures(dock_widget->features() & ~QDockWidget::DockWidgetClosable);
		dock_widget->setWidget(child_widget);
		
		// Show dock in floating state
		dock_widget->setFloating(true);
		dock_widget->show();
		dock_widget->setGeometry(window->geometry().left() + 40, window->geometry().top() + 100, dock_widget->width(), dock_widget->height());
	}
}

void MapEditorController::deletePopupWidget(QWidget* child_widget)
{
	if (mobile_mode)
	{
		delete child_widget;
	}
	else
	{
		// Delete the dock widget
		delete child_widget->parentWidget();
	}
}

bool MapEditorController::save(const QString& path)
{
	if (map)
	{
		if (editing_in_progress)
		{
			QMessageBox::warning(window, tr("Editing in progress"), tr("The map is currently being edited. Please finish the edit operation before saving."));
			return false;
		}
		bool success = map->saveTo(path, main_view);
		if (success)
			window->showStatusBarMessage(tr("Map saved"), 1000);
		return success;
	}
	else
		return false;
}

bool MapEditorController::exportTo(const QString& path, const FileFormat* format)
{
	if (map && !editing_in_progress)
	{
		return map->exportTo(path, main_view, format);
	}
	
	return false;
}

bool MapEditorController::load(const QString& path, QWidget* dialog_parent)
{
	if (!dialog_parent)
		dialog_parent = window;
	
	if (!map)
	{
		map = new Map();
		main_view = new MapView(this, map);
	}
	
	bool success = map->loadFrom(path, dialog_parent, main_view);
	if (success)
	{
		setMapAndView(map, main_view);
	}
	else
	{
		delete map;
		map = nullptr;
		main_view = nullptr;
	}
	
	return success;
}

void MapEditorController::attach(MainWindow* window)
{
	print_dock_widget = NULL;
	measure_dock_widget = NULL;
	color_dock_widget = NULL;
	symbol_dock_widget = NULL;
	QLabel* statusbar_zoom_label = NULL;
	
	this->window = window;
	if (mode == MapEditor)
	{
		window->setHasUnsavedChanges(map->hasUnsavedChanged());
	}
	connect(map, SIGNAL(hasUnsavedChanges(bool)), window, SLOT(setHasUnsavedChanges(bool)));
	
#ifdef Q_OS_ANDROID
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
                                       "lockOrientation",
                                       "()V");
#endif
	if (mobile_mode)
	{
		window->setWindowState(window->windowState() | Qt::WindowFullScreen);
	}
	else
	{
		// Add zoom / cursor position field to status bar
		QLabel* statusbar_zoom_icon = new QLabel();
		auto fontmetrics = statusbar_zoom_icon->fontMetrics();
		auto pixmap = QPixmap(QLatin1String(":/images/magnifying-glass.png"));
		auto scale = qreal(fontmetrics.height()) / pixmap.height();
		if (scale < 0.9 || scale > 1.1)
			pixmap = pixmap.scaledToHeight(qRound(scale * pixmap.height()), Qt::SmoothTransformation);
		statusbar_zoom_icon->setPixmap(pixmap);
		
		statusbar_zoom_label = new QLabel();
		statusbar_zoom_label->setMinimumWidth(fontmetrics.width(QLatin1String(" 0.333x")));
		statusbar_zoom_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		statusbar_zoom_label->setFrameShape(QFrame::NoFrame);
		
		statusbar_zoom_frame = new QFrame();
#ifdef Q_OS_WIN
		statusbar_zoom_frame->setFrameShape(QFrame::NoFrame);
#else
		statusbar_zoom_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
#endif
		QHBoxLayout* statusbar_zoom_frame_layout = new QHBoxLayout();
		statusbar_zoom_frame_layout->setMargin(0);
		statusbar_zoom_frame_layout->setSpacing(0);
		statusbar_zoom_frame_layout->addSpacing(1);
		statusbar_zoom_frame_layout->addWidget(statusbar_zoom_icon);
		statusbar_zoom_frame_layout->addWidget(statusbar_zoom_label);
		statusbar_zoom_frame->setLayout(statusbar_zoom_frame_layout);
		
		statusbar_cursorpos_label = new QLabel();
#ifdef Q_OS_WIN
		statusbar_cursorpos_label->setFrameShape(QFrame::NoFrame);
#else
		statusbar_cursorpos_label->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
#endif
		statusbar_cursorpos_label->setMinimumWidth(fontmetrics.width(QLatin1String("-3,333.33 -333.33 (mm)")));
		statusbar_cursorpos_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		
		window->statusBar()->addPermanentWidget(statusbar_zoom_frame);
		window->statusBar()->addPermanentWidget(statusbar_cursorpos_label);
		
		if (mode == MapEditor)
		{
			statusbar_objecttag_label = new QLabel();
#ifdef Q_OS_WIN
			statusbar_objecttag_label->setFrameShape(QFrame::NoFrame);
#else
			statusbar_objecttag_label->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
#endif
			statusbar_objecttag_label->setMinimumWidth(statusbar_cursorpos_label->minimumWidth());
			statusbar_objecttag_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		}
	}
	
	// Create map widget
	map_widget = new MapWidget(mode == MapEditor, mode == SymbolEditor);
	map_widget->setMapView(main_view);
	
	// Create menu and toolbar together, so actions can be inserted into one or both
	if (mode == MapEditor)
	{
		gps_display = new GPSDisplay(map_widget, map->getGeoreferencing());
		gps_marker_display = new GPSTemporaryMarkers(map_widget, gps_display);
		
		createActions();
		if (mobile_mode)
		{
			createMobileGUI();
			compass_display = new CompassDisplay(map_widget->parentWidget());
			compass_display->setVisible(false);
		}
		else
		{
			map_widget->setZoomLabel(statusbar_zoom_label);
			map_widget->setCursorposLabel(statusbar_cursorpos_label);
			map_widget->setObjectTagLabel(statusbar_objecttag_label);
			window->setCentralWidget(map_widget);
			
			createMenuAndToolbars();
			restoreWindowState();
		}
	}
	else if (mode == SymbolEditor)
	{
		map_widget->setZoomLabel(statusbar_zoom_label);
		map_widget->setCursorposLabel(statusbar_cursorpos_label);
		window->setCentralWidget(map_widget);
	}
	
	// Update enabled/disabled state for the tools ...
	updateWidgets();
	templateAvailabilityChanged();
	// ... and make sure it is kept up to date for copy/paste
	connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));
	clipboardChanged(QClipboard::Clipboard);
	
	if (mode == MapEditor)
	{
		// Create/show the dock widgets
		if (mobile_mode)
		{
			createSymbolWidget(window);
		}
		else
		{
			symbol_window_act->trigger();
			createColorWindow();
			createTemplateWindow();
			createTagEditor();
			createTagSelector();
			
			if (map->getNumColors() == 0)
				QTimer::singleShot(0, color_dock_widget, SLOT(show()));
		}
		
		// Auto-select the edit tool
		edit_tool_act->setChecked(true);
		setEditTool();
		
		// Set the coordinates display mode
		coordsDisplayChanged();
	}
	else
	{
		// Set the coordinates display mode
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
	}
}

QAction* MapEditorController::newAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const char* whats_this_link)
{
	Q_ASSERT(window); // Qt documentation recommends that actions are created as children of the window they are used in.
	QAction* action = new QAction(icon ? QIcon(QLatin1String(":/images/") + QLatin1String(icon)) : QIcon(), tr_text, window);
	if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
	if (whats_this_link) action->setWhatsThis(Util::makeWhatThis(whats_this_link));
	if (receiver) QObject::connect(action, SIGNAL(triggered()), receiver, slot);
	actionsById[id] = action;
	return action;
}

QAction* MapEditorController::newCheckAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const char* whats_this_link)
{
	auto action = newAction(id, tr_text, nullptr, nullptr, icon, tr_tip, whats_this_link);
	action->setCheckable(true);
	if (receiver) QObject::connect(action, SIGNAL(triggered(bool)), receiver, slot);
	return action;
}

QAction* MapEditorController::newToolAction(const char* id, const QString &tr_text, QObject* receiver, const char* slot, const char* icon, const QString& tr_tip, const char* whats_this_link)
{
	Q_ASSERT(window); // Qt documentation recommends that actions are created as children of the window they are used in.
	QAction* action = new MapEditorToolAction(icon ? QIcon(QLatin1String(":/images/") + QLatin1String(icon)) : QIcon(), tr_text, window);
	if (!tr_tip.isEmpty()) action->setStatusTip(tr_tip);
	if (whats_this_link) action->setWhatsThis(Util::makeWhatThis(whats_this_link));
	if (receiver) QObject::connect(action, SIGNAL(activated()), receiver, slot);
	actionsById[id] = action;
	return action;
}

QAction* MapEditorController::findAction(const char* id)
{
	if (!actionsById.contains(id)) return actionsById[""];
	else return actionsById[id];
}

QAction* MapEditorController::getAction(const char* id)
{
	return actionsById.contains(id) ? actionsById[id] : NULL;
}

void MapEditorController::assignKeyboardShortcuts()
{
	// Standard keyboard shortcuts
	findAction("print")->setShortcut(QKeySequence::Print);
	findAction("undo")->setShortcut(QKeySequence::Undo);
	findAction("redo")->setShortcut(QKeySequence::Redo);
	findAction("cut")->setShortcut(QKeySequence::Cut);
	findAction("copy")->setShortcut(QKeySequence::Copy);
	findAction("paste")->setShortcut(QKeySequence::Paste);
	
	// Custom keyboard shortcuts
	
	// QKeySequence::Deselect is empty for Windows, so be explicit all select-*
	findAction("select-all")->setShortcut(QKeySequence(tr("Ctrl+A")));
	findAction("select-nothing")->setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
	findAction("invert-selection")->setShortcut(QKeySequence(tr("Ctrl+I")));
	
	findAction("showgrid")->setShortcut(QKeySequence(tr("G")));
	findAction("zoomin")->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::Key_F7) << QKeySequence(Qt::Key_Plus) << QKeySequence(Qt::KeypadModifier + Qt::Key_Plus));
	findAction("zoomout")->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::Key_F8) << QKeySequence(Qt::Key_Minus) << QKeySequence(Qt::KeypadModifier + Qt::Key_Minus));
	findAction("hatchareasview")->setShortcut(QKeySequence(Qt::Key_F2));
	findAction("baselineview")->setShortcut(QKeySequence(Qt::Key_F3));
	findAction("hidealltemplates")->setShortcut(QKeySequence(Qt::Key_F10));
	findAction("overprintsimulation")->setShortcut(QKeySequence(Qt::Key_F4));
	findAction("fullscreen")->setShortcut(QKeySequence(Qt::Key_F11));
	tag_select_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_5));
	tags_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_6));
	color_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_7));
	symbol_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_8));
	template_window_act->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_9));
	
	findAction("editobjects")->setShortcut(QKeySequence(tr("E")));
	findAction("editlines")->setShortcut(QKeySequence(tr("L")));
	findAction("drawpoint")->setShortcut(QKeySequence(tr("S")));
	findAction("drawpath")->setShortcut(QKeySequence(tr("P")));
	findAction("drawcircle")->setShortcut(QKeySequence(tr("O")));
	findAction("drawrectangle")->setShortcut(QKeySequence(tr("Ctrl+R")));
	findAction("drawfill")->setShortcut(QKeySequence(tr("F")));
	findAction("drawtext")->setShortcut(QKeySequence(tr("T")));
	
	findAction("duplicate")->setShortcut(QKeySequence(tr("D")));
	findAction("switchsymbol")->setShortcut(QKeySequence(tr("Ctrl+G")));
	findAction("fillborder")->setShortcut(QKeySequence(tr("Ctrl+F")));
	findAction("switchdashes")->setShortcut(QKeySequence(tr("Ctrl+D")));
	findAction("connectpaths")->setShortcut(QKeySequence(tr("C")));
	findAction("rotateobjects")->setShortcut(QKeySequence(tr("R")));
	findAction("scaleobjects")->setShortcut(QKeySequence(tr("Z")));
	findAction("cutobject")->setShortcut(QKeySequence(tr("K")));
	findAction("cuthole")->setShortcut(QKeySequence(tr("H")));
	findAction("measure")->setShortcut(QKeySequence(tr("M")));
	findAction("booleanunion")->setShortcut(QKeySequence(tr("U")));
	findAction("converttocurves")->setShortcut(QKeySequence(tr("N")));
	findAction("simplify")->setShortcut(QKeySequence(tr("Ctrl+M")));
}

void MapEditorController::createActions()
{
	// Define all the actions, saving them into variables as necessary. Can also get them by ID.
#ifdef QT_PRINTSUPPORT_LIB
	QSignalMapper* print_act_mapper = new QSignalMapper(this);
	connect(print_act_mapper, SIGNAL(mapped(int)), this, SLOT(printClicked(int)));
	print_act = newAction("print", tr("Print..."), print_act_mapper, SLOT(map()), "print.png", QString{}, "file_menu.html");
	print_act_mapper->setMapping(print_act, PrintWidget::PRINT_TASK);
	export_image_act = newAction("export-image", tr("&Image"), print_act_mapper, SLOT(map()), NULL, QString{}, "file_menu.html");
	print_act_mapper->setMapping(export_image_act, PrintWidget::EXPORT_IMAGE_TASK);
	export_pdf_act = newAction("export-pdf", tr("&PDF"), print_act_mapper, SLOT(map()), NULL, QString{}, "file_menu.html");
	print_act_mapper->setMapping(export_pdf_act, PrintWidget::EXPORT_PDF_TASK);
#else
	print_act = NULL;
	export_image_act = NULL;
	export_pdf_act = NULL;
#endif
	
	undo_act = newAction("undo", tr("Undo"), this, SLOT(undo()), "undo.png", tr("Undo the last step"), "edit_menu.html");
	redo_act = newAction("redo", tr("Redo"), this, SLOT(redo()), "redo.png", tr("Redo the last step"), "edit_menu.html");
	cut_act = newAction("cut", tr("Cu&t"), this, SLOT(cut()), "cut.png", QString{}, "edit_menu.html");
	copy_act = newAction("copy", tr("C&opy"), this, SLOT(copy()), "copy.png", QString{}, "edit_menu.html");
	paste_act = newAction("paste", tr("&Paste"), this, SLOT(paste()), "paste", QString{}, "edit_menu.html");
	delete_act = newAction("delete", tr("Delete"), this, SLOT(deleteClicked()), "delete.png", QString{}, "toolbars.html#delete");
	select_all_act = newAction("select-all", tr("Select all"), this, SLOT(selectAll()), nullptr, QString{}, "edit_menu.html");
	select_nothing_act = newAction("select-nothing", tr("Select nothing"), this, SLOT(selectNothing()), nullptr, QString{}, "edit_menu.html");
	invert_selection_act = newAction("invert-selection", tr("Invert selection"), this, SLOT(invertSelection()), nullptr, QString{}, "edit_menu.html");
	select_by_current_symbol_act = newAction("select-by-symbol", QApplication::translate("SymbolRenderWidget", "Select all objects with selected symbols"), this, SLOT(selectByCurrentSymbols()), nullptr, QString{}, "edit_menu.html");
	clear_undo_redo_history_act = newAction("clearundoredohistory", tr("Clear undo / redo history"), this, SLOT(clearUndoRedoHistory()), NULL, tr("Clear the undo / redo history to reduce map file size."), "edit_menu.html");
	
	show_grid_act = newCheckAction("showgrid", tr("Show grid"), this, SLOT(showGrid()), "grid.png", QString{}, "grid.html");
	configure_grid_act = newAction("configuregrid", tr("Configure grid..."), this, SLOT(configureGrid()), "grid.png", QString{}, "grid.html");
#if defined(Q_OS_MAC)
	configure_grid_act->setMenuRole(QAction::NoRole);
#endif
	pan_act = newToolAction("panmap", tr("Pan"), this, SLOT(pan()), "move.png", QString{}, "view_menu.html");
	zoom_in_act = newAction("zoomin", tr("Zoom in"), this, SLOT(zoomIn()), "view-zoom-in.png", QString{}, "view_menu.html");
	zoom_out_act = newAction("zoomout", tr("Zoom out"), this, SLOT(zoomOut()), "view-zoom-out.png", QString{}, "view_menu.html");
	show_all_act = newAction("showall", tr("Show whole map"), this, SLOT(showWholeMap()), "view-show-all.png", QString{}, "view_menu.html");
	fullscreen_act = newAction("fullscreen", tr("Toggle fullscreen mode"), window, SLOT(toggleFullscreenMode()), NULL, QString{}, "view_menu.html");
	custom_zoom_act = newAction("setzoom", tr("Set custom zoom factor..."), this, SLOT(setCustomZoomFactorClicked()), NULL, QString{}, "view_menu.html");
	
	hatch_areas_view_act = newCheckAction("hatchareasview", tr("Hatch areas"), this, SLOT(hatchAreas(bool)), NULL, QString{}, "view_menu.html");
	baseline_view_act = newCheckAction("baselineview", tr("Baseline view"), this, SLOT(baselineView(bool)), NULL, QString{}, "view_menu.html");
	hide_all_templates_act = newCheckAction("hidealltemplates", tr("Hide all templates"), this, SLOT(hideAllTemplates(bool)), NULL, QString{}, "view_menu.html");
	overprinting_simulation_act = newCheckAction("overprintsimulation", tr("Overprinting simulation"), this, SLOT(overprintingSimulation(bool)), NULL, QString{}, "view_menu.html");
	
	symbol_window_act = newCheckAction("symbolwindow", tr("Symbol window"), this, SLOT(showSymbolWindow(bool)), "symbols.png", tr("Show/Hide the symbol window"), "symbol_dock_widget.html");
	color_window_act = newCheckAction("colorwindow", tr("Color window"), this, SLOT(showColorWindow(bool)), "colors.png", tr("Show/Hide the color window"), "color_dock_widget.html");
	load_symbols_from_act = newAction("loadsymbols", tr("Replace symbol set..."), this, SLOT(loadSymbolsFromClicked()), NULL, tr("Replace the symbols with those from another map file"), "symbol_replace_dialog.html");
	/*QAction* load_colors_from_act = newAction("loadcolors", tr("Load colors from..."), this, SLOT(loadColorsFromClicked()), NULL, tr("Replace the colors with those from another map file"));*/
	
	scale_all_symbols_act = newAction("scaleall", tr("Scale all symbols..."), this, SLOT(scaleAllSymbolsClicked()), NULL, tr("Scale the whole symbol set"), "map_menu.html");
	georeferencing_act = newAction("georef", tr("Georeferencing..."), this, SLOT(editGeoreferencing()), NULL, QString{}, "georeferencing.html");
	scale_map_act = newAction("scalemap", tr("Change map scale..."), this, SLOT(scaleMapClicked()), "tool-scale.png", tr("Change the map scale and adjust map objects and symbol sizes"), "map_menu.html");
	rotate_map_act = newAction("rotatemap", tr("Rotate map..."), this, SLOT(rotateMapClicked()), "tool-rotate.png", tr("Rotate the whole map"), "map_menu.html");
	map_notes_act = newAction("mapnotes", tr("Map notes..."), this, SLOT(mapNotesClicked()), NULL, QString{}, "map_menu.html");
	
	template_window_act = newCheckAction("templatewindow", tr("Template setup window"), this, SLOT(showTemplateWindow(bool)), "templates", tr("Show/Hide the template window"), "templates_menu.html");
	//QAction* template_config_window_act = newCheckAction("templateconfigwindow", tr("Template configurations window"), this, SLOT(showTemplateConfigurationsWindow(bool)), "window-new", tr("Show/Hide the template configurations window"));
	//QAction* template_visibilities_window_act = newCheckAction("templatevisibilitieswindow", tr("Template visibilities window"), this, SLOT(showTemplateVisbilitiesWindow(bool)), "window-new", tr("Show/Hide the template visibilities window"));
	open_template_act = newAction("opentemplate", tr("Open template..."), this, SLOT(openTemplateClicked()), NULL, QString{}, "templates_menu.html");
	reopen_template_act = newAction("reopentemplate", tr("Reopen template..."), this, SLOT(reopenTemplateClicked()), NULL, QString{}, "templates_menu.html");
	
	tags_window_act = newCheckAction("tagswindow", tr("Tag editor"), this, SLOT(showTagsWindow(bool)), "window-new", tr("Show/Hide the tag editor window"), "tag_editor.html");

	tag_select_window_act = newCheckAction("tagselectwindow", tr("Tag Selection"), this, SLOT(showTagSelectWindow(bool)), "tag-selector", tr("Show/Hide the tag selection window"), "tag_selector.html");
	
	edit_tool_act = newToolAction("editobjects", tr("Edit objects"), this, SLOT(editToolClicked()), "tool-edit.png", QString{}, "toolbars.html#tool_edit_point");
	edit_line_tool_act = newToolAction("editlines", tr("Edit lines"), this, SLOT(editLineToolClicked()), "tool-edit-line.png", QString{}, "toolbars.html#tool_edit_line");
	draw_point_act = newToolAction("drawpoint", tr("Set point objects"), this, SLOT(drawPointClicked()), "draw-point.png", QString{}, "toolbars.html#tool_draw_point");
	draw_path_act = newToolAction("drawpath", tr("Draw paths"), this, SLOT(drawPathClicked()), "draw-path.png", QString{}, "toolbars.html#tool_draw_path");
	draw_circle_act = newToolAction("drawcircle", tr("Draw circles and ellipses"), this, SLOT(drawCircleClicked()), "draw-circle.png", QString{}, "toolbars.html#tool_draw_circle");
	draw_rectangle_act = newToolAction("drawrectangle", tr("Draw rectangles"), this, SLOT(drawRectangleClicked()), "draw-rectangle.png", QString{}, "toolbars.html#tool_draw_rectangle");
	draw_freehand_act = newToolAction("drawfreehand", tr("Draw free-handedly"), this, SLOT(drawFreehandClicked()), "draw-freehand.png", QString{}, "toolbars.html#tool_draw_freehand"); // TODO: documentation
	draw_fill_act = newToolAction("drawfill", tr("Fill bounded areas"), this, SLOT(drawFillClicked()), "tool-fill.png", QString{}, "toolbars.html#tool_draw_fill"); // TODO: documentation
	draw_text_act = newToolAction("drawtext", tr("Write text"), this, SLOT(drawTextClicked()), "draw-text.png", QString{}, "toolbars.html#tool_draw_text");
	duplicate_act = newAction("duplicate", tr("Duplicate"), this, SLOT(duplicateClicked()), "tool-duplicate.png", QString{}, "toolbars.html#duplicate");
	switch_symbol_act = newAction("switchsymbol", tr("Switch symbol"), this, SLOT(switchSymbolClicked()), "tool-switch-symbol.png", QString{}, "toolbars.html#switch_symbol");
	fill_border_act = newAction("fillborder", tr("Fill / Create border"), this, SLOT(fillBorderClicked()), "tool-fill-border.png", QString{}, "toolbars.html#fill_create_border");
	switch_dashes_act = newAction("switchdashes", tr("Switch dash direction"), this, SLOT(switchDashesClicked()), "tool-switch-dashes", QString{}, "toolbars.html#switch_dashes");
	connect_paths_act = newAction("connectpaths", tr("Connect paths"), this, SLOT(connectPathsClicked()), "tool-connect-paths.png", QString{}, "toolbars.html#connect");
	
	cut_tool_act = newToolAction("cutobject", tr("Cut object"), this, SLOT(cutClicked()), "tool-cut.png", QString{}, "toolbars.html#cut_tool");
	cut_hole_act = newToolAction("cuthole", tr("Cut free form hole"), this, SLOT(cutHoleClicked()), "tool-cut-hole.png", QString{}, "toolbars.html#cut_hole"); // cut hole using a path
	cut_hole_circle_act = new MapEditorToolAction(QIcon(QString::fromLatin1(":/images/tool-cut-hole.png")), tr("Cut round hole"), this);
	cut_hole_circle_act->setWhatsThis(Util::makeWhatThis("toolbars.html#cut_hole"));
	cut_hole_circle_act->setCheckable(true);
	QObject::connect(cut_hole_circle_act, SIGNAL(activated()), this, SLOT(cutHoleCircleClicked()));
	cut_hole_rectangle_act = new MapEditorToolAction(QIcon(QString::fromLatin1(":/images/tool-cut-hole.png")), tr("Cut rectangular hole"), this);
	cut_hole_rectangle_act->setWhatsThis(Util::makeWhatThis("toolbars.html#cut_hole"));
	cut_hole_rectangle_act->setCheckable(true);
	QObject::connect(cut_hole_rectangle_act, SIGNAL(activated()), this, SLOT(cutHoleRectangleClicked()));
	cut_hole_menu = new QMenu(tr("Cut hole"));
	cut_hole_menu->setIcon(QIcon(QString::fromLatin1(":/images/tool-cut-hole.png")));
	cut_hole_menu->addAction(cut_hole_act);
	cut_hole_menu->addAction(cut_hole_circle_act);
	cut_hole_menu->addAction(cut_hole_rectangle_act);
	
	rotate_act = newToolAction("rotateobjects", tr("Rotate object(s)"), this, SLOT(rotateClicked()), "tool-rotate.png", QString{}, "toolbars.html#rotate");
	rotate_pattern_act = newToolAction("rotatepatterns", tr("Rotate pattern"), this, SLOT(rotatePatternClicked()), "tool-rotate-pattern.png", QString{}, "toolbars.html#tool_rotate_pattern");
	scale_act = newToolAction("scaleobjects", tr("Scale object(s)"), this, SLOT(scaleClicked()), "tool-scale.png", QString{}, "toolbars.html#scale");
	measure_act = newCheckAction("measure", tr("Measure lengths and areas"), this, SLOT(measureClicked(bool)), "tool-measure.png", QString{}, "toolbars.html#measure");
	boolean_union_act = newAction("booleanunion", tr("Unify areas"), this, SLOT(booleanUnionClicked()), "tool-boolean-union.png", QString{}, "toolbars.html#unify_areas");
	boolean_intersection_act = newAction("booleanintersection", tr("Intersect areas"), this, SLOT(booleanIntersectionClicked()), "tool-boolean-intersection.png", QString{}, "toolbars.html#intersect_areas");
	boolean_difference_act = newAction("booleandifference", tr("Cut away from area"), this, SLOT(booleanDifferenceClicked()), "tool-boolean-difference.png", QString{}, "toolbars.html#area_difference");
	boolean_xor_act = newAction("booleanxor", tr("Area XOr"), this, SLOT(booleanXOrClicked()), "tool-boolean-xor.png", QString{}, "toolbars.html#area_xor");
	boolean_merge_holes_act = newAction("booleanmergeholes", tr("Merge area holes"), this, SLOT(booleanMergeHolesClicked()), "tool-boolean-merge-holes.png", QString{}, "toolbars.html#area_merge_holes"); // TODO:documentation
	convert_to_curves_act = newAction("converttocurves", tr("Convert to curves"), this, SLOT(convertToCurvesClicked()), "tool-convert-to-curves.png", QString{}, "toolbars.html#convert_to_curves");
	simplify_path_act = newAction("simplify", tr("Simplify path"), this, SLOT(simplifyPathClicked()), "tool-simplify-path.png", QString{}, "toolbars.html#simplify_path");
	cutout_physical_act = newToolAction("cutoutphysical", tr("Cutout"), this, SLOT(cutoutPhysicalClicked()), "tool-cutout-physical.png", QString{}, "toolbars.html#cutout_physical");
	cutaway_physical_act = newToolAction("cutawayphysical", tr("Cut away"), this, SLOT(cutawayPhysicalClicked()), "tool-cutout-physical-inner.png", QString{}, "toolbars.html#cutaway_physical");
	distribute_points_act = newAction("distributepoints", tr("Distribute points along path"), this, SLOT(distributePointsClicked()), "tool-distribute-points.png", QString{}, "toolbars.html#distribute_points"); // TODO: write documentation
	
	paint_on_template_act = new QAction(QIcon(QString::fromLatin1(":/images/pencil.png")), tr("Paint on template"), this);
	paint_on_template_act->setCheckable(true);
	paint_on_template_act->setWhatsThis(Util::makeWhatThis("toolbars.html#draw_on_template"));
	connect(paint_on_template_act, SIGNAL(triggered(bool)), this, SLOT(paintOnTemplateClicked(bool)));

	paint_on_template_settings_act = new QAction(QIcon(QString::fromLatin1(":/images/paint-on-template-settings.png")), tr("Paint on template settings"), this);
	paint_on_template_settings_act->setWhatsThis(Util::makeWhatThis("toolbars.html#draw_on_template"));
	connect(paint_on_template_settings_act, SIGNAL(triggered(bool)), this, SLOT(paintOnTemplateSelectClicked()));

	updatePaintOnTemplateAction();
	
	touch_cursor_action = newCheckAction("touchcursor", tr("Enable touch cursor"), map_widget, SLOT(enableTouchCursor(bool)), "tool-touch-cursor.png", QString{}, "toolbars.html#touch_cursor"); // TODO: write documentation
	gps_display_action = newCheckAction("gpsdisplay", tr("Enable GPS display"), this, SLOT(enableGPSDisplay(bool)), "tool-gps-display.png", QString{}, "toolbars.html#gps_display"); // TODO: write documentation
	gps_display_action->setEnabled(map->getGeoreferencing().isValid() && ! map->getGeoreferencing().isLocal());
	gps_distance_rings_action = newCheckAction("gpsdistancerings", tr("Enable GPS distance rings"), this, SLOT(enableGPSDistanceRings(bool)), "gps-distance-rings.png", QString{}, "toolbars.html#gps_distance_rings"); // TODO: write documentation
	gps_distance_rings_action->setEnabled(false);
	draw_point_gps_act = newToolAction("drawpointgps", tr("Set point object at GPS position"), this, SLOT(drawPointGPSClicked()), "draw-point-gps.png", QString{}, "toolbars.html#tool_draw_point_gps"); // TODO: write documentation
	draw_point_gps_act->setEnabled(false);
	gps_temporary_point_act = newAction("gpstemporarypoint", tr("Set temporary marker at GPS position"), this, SLOT(gpsTemporaryPointClicked()), "gps-temporary-point.png", QString{}, "toolbars.html#gps_temporary_point"); // TODO: write documentation
	gps_temporary_point_act->setEnabled(false);
	gps_temporary_path_act = newCheckAction("gpstemporarypath", tr("Create temporary path at GPS position"), this, SLOT(gpsTemporaryPathClicked(bool)), "gps-temporary-path.png", QString{}, "toolbars.html#gps_temporary_path"); // TODO: write documentation
	gps_temporary_path_act->setEnabled(false);
	gps_temporary_clear_act = newAction("gpstemporaryclear", tr("Clear temporary GPS markers"), this, SLOT(gpsTemporaryClearClicked()), "gps-temporary-clear.png", QString{}, "toolbars.html#gps_temporary_clear"); // TODO: write documentation
	gps_temporary_clear_act->setEnabled(false);
	
	compass_action = newCheckAction("compassdisplay", tr("Enable compass display"), this, SLOT(enableCompassDisplay(bool)), "compass.png", QString{}, "toolbars.html#compass_display"); // TODO: write documentation
	align_map_with_north_act = newCheckAction("alignmapwithnorth", tr("Align map with north"), this, SLOT(alignMapWithNorth(bool)), "rotate-map.png", QString{}, "toolbars.html#align_map_with_north"); // TODO: write documentation
	
	mappart_add_act = newAction("addmappart", tr("Add new part..."), this, SLOT(addMapPart()));
	mappart_rename_act = newAction("renamemappart", tr("Rename current part..."), this, SLOT(renameMapPart()));
	mappart_remove_act = newAction("removemappart", tr("Remove current part"), this, SLOT(removeMapPart()));
	mappart_merge_act = newAction("mergemapparts", tr("Merge all parts"), this, SLOT(mergeAllMapParts()));
	
	import_act = newAction("import", tr("Import..."), this, SLOT(importClicked()), NULL, QString{}, "file_menu.html");
	
	map_coordinates_act = new QAction(tr("Map coordinates"), this);
	map_coordinates_act->setCheckable(true);
	projected_coordinates_act = new QAction(tr("Projected coordinates"), this);
	projected_coordinates_act->setCheckable(true);
	geographic_coordinates_act = new QAction(tr("Latitude/Longitude (Dec)"), this);
	geographic_coordinates_act->setCheckable(true);
	geographic_coordinates_dms_act = new QAction(tr("Latitude/Longitude (DMS)"), this);
	geographic_coordinates_dms_act->setCheckable(true);
	QActionGroup* coordinates_group = new QActionGroup(this);
	coordinates_group->addAction(map_coordinates_act);
	coordinates_group->addAction(projected_coordinates_act);
	coordinates_group->addAction(geographic_coordinates_act);
	coordinates_group->addAction(geographic_coordinates_dms_act);
	QObject::connect(coordinates_group, SIGNAL(triggered(QAction*)), this, SLOT(coordsDisplayChanged()));
	map_coordinates_act->setChecked(true);
	QObject::connect(&map->getGeoreferencing(), SIGNAL(projectionChanged()), this, SLOT(projectionChanged()));
	projectionChanged();
	
	copy_coords_act = newAction("copy-coords", tr("Copy position"), this, SLOT(copyDisplayedCoords()), "copy-coords.png", tr("Copy position to clipboard."));
}

void MapEditorController::createMenuAndToolbars()
{
	statusbar_cursorpos_label->setContextMenuPolicy(Qt::ActionsContextMenu);
	statusbar_cursorpos_label->addAction(map_coordinates_act);
	statusbar_cursorpos_label->addAction(projected_coordinates_act);
	statusbar_cursorpos_label->addAction(geographic_coordinates_act);
	statusbar_cursorpos_label->addAction(geographic_coordinates_dms_act);
	
	// Refactored so we can do custom key bindings in the future
	assignKeyboardShortcuts();
	
	// Extend file menu
	QMenu* file_menu = window->getFileMenu();
	QAction* insertion_act = window->getFileMenuExtensionAct();
#ifdef QT_PRINTSUPPORT_LIB
	file_menu->insertAction(insertion_act, print_act);
	file_menu->insertSeparator(insertion_act);
	insertion_act = print_act;
#endif
	file_menu->insertAction(insertion_act, import_act);
#ifdef QT_PRINTSUPPORT_LIB
	QMenu* export_menu = new QMenu(tr("&Export as..."), file_menu);
	export_menu->addAction(export_image_act);
	export_menu->addAction(export_pdf_act);
	file_menu->insertMenu(insertion_act, export_menu);
#endif
	file_menu->insertSeparator(insertion_act);
		
	// Edit menu
	QMenu* edit_menu = window->menuBar()->addMenu(tr("&Edit"));
	edit_menu->setWhatsThis(Util::makeWhatThis("edit_menu.html"));
	edit_menu->addAction(undo_act);
	edit_menu->addAction(redo_act);
	edit_menu->addSeparator();
	edit_menu->addAction(cut_act);
	edit_menu->addAction(copy_act);
	edit_menu->addAction(paste_act);
	edit_menu->addAction(delete_act);
	edit_menu->addSeparator();
	edit_menu->addAction(select_all_act);
	edit_menu->addAction(select_nothing_act);
	edit_menu->addAction(invert_selection_act);
	edit_menu->addAction(select_by_current_symbol_act);
	edit_menu->addSeparator();
	edit_menu->addAction(clear_undo_redo_history_act);
	
	// View menu
	QMenu* view_menu = window->menuBar()->addMenu(tr("&View"));
	view_menu->setWhatsThis(Util::makeWhatThis("view_menu.html"));
	view_menu->addAction(pan_act);
	view_menu->addAction(zoom_in_act);
	view_menu->addAction(zoom_out_act);
	view_menu->addAction(show_all_act);
	view_menu->addAction(custom_zoom_act);
	view_menu->addSeparator();
	view_menu->addAction(show_grid_act);
	view_menu->addAction(hatch_areas_view_act);
	view_menu->addAction(baseline_view_act);
	view_menu->addAction(overprinting_simulation_act);
	view_menu->addAction(hide_all_templates_act);
	view_menu->addSeparator();
	QMenu* coordinates_menu = new QMenu(tr("Display coordinates as..."), view_menu);
	coordinates_menu->addAction(map_coordinates_act);
	coordinates_menu->addAction(projected_coordinates_act);
	coordinates_menu->addAction(geographic_coordinates_act);
	coordinates_menu->addAction(geographic_coordinates_dms_act);
	view_menu->addMenu(coordinates_menu);
	view_menu->addSeparator();
	view_menu->addAction(fullscreen_act);
	view_menu->addSeparator();
	view_menu->addAction(tag_select_window_act);
	view_menu->addAction(tags_window_act);
	view_menu->addAction(color_window_act);
	view_menu->addAction(symbol_window_act);
	view_menu->addAction(template_window_act);
	
	// Tools menu
	QMenu *tools_menu = window->menuBar()->addMenu(tr("&Tools"));
	tools_menu->setWhatsThis(Util::makeWhatThis("tools_menu.html"));
	tools_menu->addAction(edit_tool_act);
	tools_menu->addAction(edit_line_tool_act);
	tools_menu->addAction(draw_point_act);
	tools_menu->addAction(draw_path_act);
	tools_menu->addAction(draw_circle_act);
	tools_menu->addAction(draw_rectangle_act);
	tools_menu->addAction(draw_freehand_act);
	tools_menu->addAction(draw_fill_act);
	tools_menu->addAction(draw_text_act);
	tools_menu->addAction(duplicate_act);
	tools_menu->addAction(switch_symbol_act);
	tools_menu->addAction(fill_border_act);
	tools_menu->addAction(switch_dashes_act);
	tools_menu->addAction(connect_paths_act);
	tools_menu->addAction(boolean_union_act);
	tools_menu->addAction(boolean_intersection_act);
	tools_menu->addAction(boolean_difference_act);
	tools_menu->addAction(boolean_xor_act);
	tools_menu->addAction(boolean_merge_holes_act);
	tools_menu->addAction(cut_tool_act);
	tools_menu->addMenu(cut_hole_menu);
	tools_menu->addAction(rotate_act);
	tools_menu->addAction(rotate_pattern_act);
	tools_menu->addAction(scale_act);
	tools_menu->addAction(measure_act);
	tools_menu->addAction(convert_to_curves_act);
	tools_menu->addAction(simplify_path_act);
	tools_menu->addAction(cutout_physical_act);
	tools_menu->addAction(cutaway_physical_act);
	tools_menu->addAction(distribute_points_act);
	tools_menu->addAction(touch_cursor_action);
	
	// Map menu
	QMenu* map_menu = window->menuBar()->addMenu(tr("M&ap"));
	map_menu->setWhatsThis(Util::makeWhatThis("map_menu.html"));
	map_menu->addAction(georeferencing_act);
	map_menu->addAction(configure_grid_act);
	map_menu->addSeparator();
	map_menu->addAction(scale_map_act);
	map_menu->addAction(rotate_map_act);
	map_menu->addAction(map_notes_act);
	map_menu->addSeparator();
	updateMapPartsUI();
	map_menu->addAction(mappart_add_act);
	map_menu->addAction(mappart_rename_act);
	map_menu->addAction(mappart_remove_act);
	map_menu->addMenu(mappart_move_menu);
	map_menu->addMenu(mappart_merge_menu);
	map_menu->addAction(mappart_merge_act);
	
	// Symbols menu
	QMenu* symbols_menu = window->menuBar()->addMenu(tr("Sy&mbols"));
	symbols_menu->setWhatsThis(Util::makeWhatThis("symbols_menu.html"));
	symbols_menu->addAction(symbol_window_act);
	symbols_menu->addAction(color_window_act);
	symbols_menu->addSeparator();
	symbols_menu->addAction(scale_all_symbols_act);
	symbols_menu->addAction(load_symbols_from_act);
	/*symbols_menu->addAction(load_colors_from_act);*/
	
	// Templates menu
	QMenu* template_menu = window->menuBar()->addMenu(tr("&Templates"));
	template_menu->setWhatsThis(Util::makeWhatThis("templates_menu.html"));
	template_menu->addAction(template_window_act);
	/*template_menu->addAction(template_config_window_act);
	template_menu->addAction(template_visibilities_window_act);*/
	template_menu->addSeparator();
	template_menu->addAction(open_template_act);
	template_menu->addAction(reopen_template_act);
	
	// Extend and activate general toolbar
	QToolBar* main_toolbar = window->getGeneralToolBar();
#ifdef QT_PRINTSUPPORT_LIB
	main_toolbar->addAction(print_act);
	main_toolbar->addSeparator();
#endif
	main_toolbar->addAction(cut_act);
	main_toolbar->addAction(copy_act);
	main_toolbar->addAction(paste_act);
	main_toolbar->addSeparator();
	main_toolbar->addAction(undo_act);
	main_toolbar->addAction(redo_act);
	window->addToolBar(main_toolbar);
	
	// View toolbar
	toolbar_view = window->addToolBar(tr("View"));
	toolbar_view->setObjectName(QString::fromLatin1("View toolbar"));
	QToolButton* grid_button = new QToolButton();
	grid_button->setCheckable(true);
	grid_button->setDefaultAction(show_grid_act);
	grid_button->setPopupMode(QToolButton::MenuButtonPopup);
	QMenu* grid_menu = new QMenu(grid_button);
	grid_menu->addAction(tr("Configure grid..."));
	grid_button->setMenu(grid_menu);
	connect(grid_menu, SIGNAL(triggered(QAction*)), this, SLOT(configureGrid()));
	toolbar_view->addWidget(grid_button);
	toolbar_view->addSeparator();
	toolbar_view->addAction(pan_act);
	toolbar_view->addAction(zoom_in_act);
	toolbar_view->addAction(zoom_out_act);
	toolbar_view->addAction(show_all_act);
	
	// MapParts toolbar
	toolbar_mapparts = window->addToolBar(tr("Map parts"));
	toolbar_mapparts->setObjectName(QString::fromLatin1("Map parts toolbar"));
	if (!mappart_selector_box)
	{
		mappart_selector_box = new QComboBox(toolbar_mapparts);
		mappart_selector_box->setToolTip(tr("Map parts"));
	}
	mappart_selector_box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	connect(mappart_selector_box, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMapPart(int)));
	toolbar_mapparts->addWidget(mappart_selector_box);
	
	window->addToolBarBreak();
	
	// Drawing toolbar
	toolbar_drawing = window->addToolBar(tr("Drawing"));
	toolbar_drawing->setObjectName(QString::fromLatin1("Drawing toolbar"));
	toolbar_drawing->addAction(edit_tool_act);
	toolbar_drawing->addAction(edit_line_tool_act);
	toolbar_drawing->addAction(draw_point_act);
	toolbar_drawing->addAction(draw_path_act);
	toolbar_drawing->addAction(draw_circle_act);
	toolbar_drawing->addAction(draw_rectangle_act);
	toolbar_drawing->addAction(draw_freehand_act);
	toolbar_drawing->addAction(draw_fill_act);
	toolbar_drawing->addAction(draw_text_act);
	toolbar_drawing->addSeparator();
	
	QToolButton* paint_on_template_button = new QToolButton();
	paint_on_template_button->setCheckable(true);
	paint_on_template_button->setDefaultAction(paint_on_template_act);
	paint_on_template_button->setPopupMode(QToolButton::MenuButtonPopup);
	QMenu* paint_on_template_menu = new QMenu(paint_on_template_button);
	paint_on_template_menu->addAction(tr("Select template..."));
	paint_on_template_button->setMenu(paint_on_template_menu);
	connect(paint_on_template_menu, SIGNAL(triggered(QAction*)), this, SLOT(paintOnTemplateSelectClicked()));
	toolbar_drawing->addWidget(paint_on_template_button);
	
	// Editing toolbar
	toolbar_editing = window->addToolBar(tr("Editing"));
	toolbar_editing->setObjectName(QString::fromLatin1("Editing toolbar"));
	toolbar_editing->addAction(delete_act);
	toolbar_editing->addAction(duplicate_act);
	toolbar_editing->addAction(switch_symbol_act);
	toolbar_editing->addAction(fill_border_act);
	toolbar_editing->addAction(switch_dashes_act);
	toolbar_editing->addAction(connect_paths_act);
	toolbar_editing->addAction(boolean_union_act);
	toolbar_editing->addAction(cut_tool_act);
	
	QToolButton* cut_hole_button = new QToolButton();
	cut_hole_button->setCheckable(true);
	cut_hole_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	cut_hole_button->setDefaultAction(cut_hole_act);
	cut_hole_button->setPopupMode(QToolButton::MenuButtonPopup);
	cut_hole_button->setMenu(cut_hole_menu);
	toolbar_editing->addWidget(cut_hole_button);
	
	toolbar_editing->addAction(rotate_act);
	toolbar_editing->addAction(rotate_pattern_act);
	toolbar_editing->addAction(scale_act);
	toolbar_editing->addAction(measure_act);
	
	// Advanced editing toolbar
	toolbar_advanced_editing = window->addToolBar(tr("Advanced editing"));
	toolbar_advanced_editing->setObjectName(QString::fromLatin1("Advanved editing toolbar"));
	toolbar_advanced_editing->addAction(cutout_physical_act);
	toolbar_advanced_editing->addAction(cutaway_physical_act);
	toolbar_advanced_editing->addAction(convert_to_curves_act);
	toolbar_advanced_editing->addAction(simplify_path_act);
	toolbar_advanced_editing->addAction(distribute_points_act);
	toolbar_advanced_editing->addAction(boolean_intersection_act);
	toolbar_advanced_editing->addAction(boolean_difference_act);
	toolbar_advanced_editing->addAction(boolean_xor_act);
	toolbar_advanced_editing->addAction(boolean_merge_holes_act);
	
	QWidget* context_menu = map_widget->getContextMenu();
	context_menu->addAction(edit_tool_act);
	context_menu->addAction(draw_point_act);
	context_menu->addAction(draw_path_act);
	context_menu->addAction(draw_rectangle_act);
	context_menu->addAction(cut_tool_act);
	context_menu->addAction(cut_hole_act);
	context_menu->addAction(switch_dashes_act);
	context_menu->addAction(connect_paths_act);
	context_menu->addAction(copy_coords_act);
}

void MapEditorController::createMobileGUI()
{
	// Create mobile-specific actions
	mobile_symbol_selector_action = new QAction(tr("Select symbol"), this);
	connect(mobile_symbol_selector_action, SIGNAL(triggered()), this, SLOT(mobileSymbolSelectorClicked()));
	
	QAction* hide_top_bar_action = new QAction(QIcon(QString::fromLatin1(":/images/arrow-thin-upleft.png")), tr("Hide top bar"), this);
 	connect(hide_top_bar_action, SIGNAL(triggered()), this, SLOT(hideTopActionBar()));
	
	QAction* show_top_bar_action = new QAction(QIcon(QString::fromLatin1(":/images/arrow-thin-downright.png")), tr("Show top bar"), this);
 	connect(show_top_bar_action, SIGNAL(triggered()), this, SLOT(showTopActionBar()));
	
	Q_ASSERT(mappart_selector_box);
	QAction* mappart_action = new QAction(QIcon(QString::fromLatin1(":/images/map-parts.png")), tr("Map parts"), this);
	connect(mappart_action, &QAction::triggered, [this, mappart_action]() {
		auto mappart_button = top_action_bar->getButtonForAction(mappart_action);
		if (!mappart_button)
			mappart_button = top_action_bar->getButtonForAction(top_action_bar->getOverflowAction());
		mappart_selector_box->setGeometry(mappart_button->geometry());
		mappart_selector_box->showPopup();
	});
	connect(mappart_selector_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MapEditorController::changeMapPart);
	
	// Create button for showing the top bar again after hiding it
	int icon_size_pixel = qRound(Util::mmToPixelLogical(10));
	const int button_icon_size = icon_size_pixel - 12;
	QSize icon_size = QSize(button_icon_size, button_icon_size);
	
	QIcon icon = show_top_bar_action->icon();
	QPixmap pixmap = icon.pixmap(icon_size, QIcon::Normal, QIcon::Off);
	if (pixmap.width() < button_icon_size)
	{
		pixmap = pixmap.scaled(icon_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		icon.addPixmap(pixmap);
		show_top_bar_action->setIcon(icon);
	}
	
	show_top_bar_button = new QToolButton(window);
	show_top_bar_button->setDefaultAction(show_top_bar_action);
	show_top_bar_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	show_top_bar_button->setAutoRaise(true);
	show_top_bar_button->setIconSize(icon_size);
	show_top_bar_button->setGeometry(0, 0, button_icon_size, button_icon_size);
	
	
	// Create bottom action bar
	bottom_action_bar = new ActionGridBar(ActionGridBar::Horizontal, 2);
	
	// Left side
	int col = 0;
	bottom_action_bar->addAction(zoom_in_act, 0, col);
	bottom_action_bar->addAction(pan_act, 1, col++);
	
	bottom_action_bar->addAction(zoom_out_act, 0, col);
	bottom_action_bar->addAction(gps_temporary_clear_act, 1, col++);
	
	bottom_action_bar->addAction(gps_temporary_path_act, 0, col);
	bottom_action_bar->addAction(gps_temporary_point_act, 1, col++);
	
	bottom_action_bar->addAction(paint_on_template_act, 0, col);
	bottom_action_bar->addAction(paint_on_template_settings_act, 1, col++);
	
	// Right side
	bottom_action_bar->addActionAtEnd(mobile_symbol_selector_action, 0, 1, 2, 2);
	
	col = 2;
	bottom_action_bar->addActionAtEnd(draw_point_act, 0, col);
	bottom_action_bar->addActionAtEnd(draw_point_gps_act, 1, col++);
	
	bottom_action_bar->addActionAtEnd(draw_path_act, 0, col);
	bottom_action_bar->addActionAtEnd(draw_freehand_act, 1, col++);
	
	bottom_action_bar->addActionAtEnd(draw_rectangle_act, 0, col);
	bottom_action_bar->addActionAtEnd(draw_circle_act, 1, col++);
	
	//bottom_action_bar->addActionAtEnd(draw_fill_act, 0, col);
	//bottom_action_bar->addActionAtEnd(draw_text_act, 1, col++);
	
	
	// Create top action bar
	top_action_bar = new ActionGridBar(ActionGridBar::Horizontal, 2);
	
	// Left side
	col = 0;
	top_action_bar->addAction(hide_top_bar_action, 0, col);
	top_action_bar->addAction(window->getSaveAct(), 1, col++);
	
	top_action_bar->addAction(compass_action, 0, col);
	top_action_bar->addAction(gps_display_action, 1, col++);
	
	top_action_bar->addAction(gps_distance_rings_action, 0, col);
	top_action_bar->addAction(align_map_with_north_act, 1, col++);
	
	top_action_bar->addAction(show_grid_act, 0, col);
	top_action_bar->addAction(show_all_act, 1, col++);
	
	// Right side
	col = 0;
	top_action_bar->addActionAtEnd(window->getCloseAct(), 0, col);
	top_action_bar->addActionAtEnd(top_action_bar->getOverflowAction(), 1, col++);
	
	top_action_bar->addActionAtEnd(redo_act, 0, col);
	top_action_bar->addActionAtEnd(undo_act, 1, col++);
	
	top_action_bar->addActionAtEnd(touch_cursor_action, 0, col);
	top_action_bar->addActionAtEnd(template_window_act, 1, col++);
	
	top_action_bar->addActionAtEnd(edit_tool_act, 0, col);
	top_action_bar->addActionAtEnd(edit_line_tool_act, 1, col++);
	
	top_action_bar->addActionAtEnd(delete_act, 0, col);
	top_action_bar->addActionAtEnd(duplicate_act, 1, col++);
	
	top_action_bar->addActionAtEnd(switch_symbol_act, 0, col);
	top_action_bar->addActionAtEnd(fill_border_act, 1, col++);
	
	top_action_bar->addActionAtEnd(switch_dashes_act, 0, col);
	top_action_bar->addActionAtEnd(boolean_union_act, 1, col++);
	
	top_action_bar->addActionAtEnd(cut_tool_act, 0, col);
	top_action_bar->addActionAtEnd(connect_paths_act, 1, col++);
	
	top_action_bar->addActionAtEnd(rotate_act, 0, col);
	top_action_bar->addActionAtEnd(cut_hole_act, 1, col++);
	
	top_action_bar->addActionAtEnd(scale_act, 0, col);
	top_action_bar->addActionAtEnd(rotate_pattern_act, 1, col++);
	
	top_action_bar->addActionAtEnd(convert_to_curves_act, 0, col);
	top_action_bar->addActionAtEnd(simplify_path_act, 1, col++);
	
	top_action_bar->addActionAtEnd(distribute_points_act, 0, col);
	top_action_bar->addActionAtEnd(boolean_difference_act, 1, col++);
	
	top_action_bar->addActionAtEnd(measure_act, 0, col);
	top_action_bar->addActionAtEnd(boolean_merge_holes_act, 1, col++);
	
	top_action_bar->addActionAtEnd(mappart_action, 1, col++);
	
	bottom_action_bar->setToUseOverflowActionFrom(top_action_bar);
	
	top_action_bar->setParent(map_widget);
	
	QWidget* container_widget = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(map_widget, 1);
	layout->addWidget(bottom_action_bar);
	container_widget->setLayout(layout);
	window->setCentralWidget(container_widget);
}

void MapEditorController::detach()
{
	// Terminate all editing
	setTool(nullptr);
	setOverrideTool(nullptr);
	
	saveWindowState();
	
	// Avoid a crash triggered by pressing Ctrl-W during loading.
	if (NULL != symbol_dock_widget)
		window->removeDockWidget(symbol_dock_widget);
	else if (NULL != symbol_widget)
		delete symbol_widget;
	
	delete gps_display;
	gps_display = NULL;
	delete gps_track_recorder;
	gps_track_recorder = NULL;
	delete compass_display;
	compass_display = NULL;
	delete gps_marker_display;
	gps_marker_display = NULL;
	
	window->setCentralWidget(NULL);
	delete map_widget;
	
	delete statusbar_zoom_frame;
	delete statusbar_cursorpos_label;
	delete statusbar_objecttag_label;
	
#ifdef Q_OS_ANDROID
	QAndroidJniObject::callStaticMethod<void>("org/openorienteering/mapper/MapperActivity",
                                       "unlockOrientation",
                                       "()V");
#endif
	
	if (mobile_mode)
		window->setWindowState(window->windowState() & ~Qt::WindowFullScreen);
}

void MapEditorController::saveWindowState()
{
	if (!mobile_mode && mode != SymbolEditor)
	{
		QSettings settings;
		settings.beginGroup(QString::fromUtf8(metaObject()->className()));
		settings.setValue(QString::fromLatin1("state"), window->saveState());
	}
}

void MapEditorController::restoreWindowState()
{
	if (!mobile_mode)
	{
		QSettings settings;
		settings.beginGroup(QString::fromUtf8(metaObject()->className()));
		window->restoreState(settings.value(QString::fromLatin1("state")).toByteArray());
		if (toolbar_mapparts && mappart_selector_box)
			toolbar_mapparts->setVisible(mappart_selector_box->count() > 1);
	}
}

bool MapEditorController::keyPressEventFilter(QKeyEvent* event)
{
#if defined(Q_OS_ANDROID)
	if (event->key() == Qt::Key_Back)
	{
		if (symbol_widget && symbol_widget->isVisible())
		{
			mobileSymbolSelectorFinished();
			return true;
		}
		
		if (isEditingInProgress() && !map_widget->findChild<KeyButtonBar*>())
		{
			QKeyEvent escape_pressed{ QEvent::KeyPress, Qt::Key_Escape, event->modifiers() };
			QCoreApplication::sendEvent(window, &escape_pressed);
			QKeyEvent escape_released{ QEvent::KeyRelease, Qt::Key_Escape, event->modifiers() };
			QCoreApplication::sendEvent(window, &escape_released);
			return true;
		}
	}
#endif
	
	return map_widget->keyPressEventFilter(event);
}

bool MapEditorController::keyReleaseEventFilter(QKeyEvent* event)
{
	return map_widget->keyReleaseEventFilter(event);
}

void MapEditorController::printClicked(int task)
{
#ifdef QT_PRINTSUPPORT_LIB
	if (!print_dock_widget)
	{
		print_dock_widget = new EditorDockWidget(QString{}, NULL, this, window);
		print_dock_widget->setAllowedAreas(Qt::NoDockWidgetArea);
		print_dock_widget->toggleViewAction()->setVisible(false);
		print_widget = new PrintWidget(map, window, main_view, this, print_dock_widget);
		connect(print_dock_widget, &QDockWidget::visibilityChanged, [this]() {
			print_widget->setActive(print_dock_widget->isVisible());
		} );
		connect(print_widget, SIGNAL(closeClicked()), print_dock_widget, SLOT(close()));
		connect(print_widget, SIGNAL(finished(int)), print_dock_widget, SLOT(close()));
		connect(print_widget, SIGNAL(taskChanged(QString)), print_dock_widget, SLOT(setWindowTitle(QString)));
		print_dock_widget->setWidget(print_widget);
		print_dock_widget->setObjectName(QString::fromLatin1("Print dock widget"));
		addFloatingDockWidget(print_dock_widget);
	}
	
	print_widget->setActive(true); // Make sure to save the state before setting the task.
	print_widget->setTask((PrintWidget::TaskFlags)task);
	print_dock_widget->show();
	print_dock_widget->raise();
#else
	Q_UNUSED(task)
	QMessageBox::warning(window, tr("Error"), tr("Print / Export is not available in this program version!"));
#endif
}

void MapEditorController::undo()
{
	doUndo(false);
}

void MapEditorController::redo()
{
	doUndo(true);
}

void MapEditorController::doUndo(bool redo)
{
	if ((!redo && !map->undoManager().canUndo()) ||
		(redo && !map->undoManager().canRedo()))
	{
		// This should not happen as the action should be deactivated in this case!
		QMessageBox::critical(window, tr("Error"), tr("No undo steps available."));
		return;
	}
	
	if (redo)
		map->undoManager().redo(window);
	else
		map->undoManager().undo(window);
}

void MapEditorController::cut()
{
	copy();
	//: Past tense. Displayed when an Edit > Cut operation is completed.
	window->showStatusBarMessage(tr("Cut %1 object(s)").arg(map->getNumSelectedObjects()), 2000);
	map->deleteSelectedObjects();
}

void MapEditorController::copy()
{
	if (map->getNumSelectedObjects() == 0)
		return;
	
	// Create map containing required objects and their symbol and color dependencies
	Map copy_map;
	copy_map.setScaleDenominator(map->getScaleDenominator());
	
	std::vector<bool> symbol_filter;
	symbol_filter.assign(map->getNumSymbols(), false);
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(), end = map->selectedObjectsEnd(); it != end; ++it)
	{
		int symbol_index = map->findSymbolIndex((*it)->getSymbol());
		if (symbol_index >= 0)
			symbol_filter[symbol_index] = true;
	}
	
	// Copy all colors. This improves preservation of relative order during paste.
	copy_map.importMap(map, Map::ColorImport, window);
	
	// Export symbols and colors into copy_map
	QHash<const Symbol*, Symbol*> symbol_map;
	copy_map.importMap(map, Map::MinimalSymbolImport, window, &symbol_filter, -1, true, &symbol_map);
	
	// Duplicate all selected objects into copy map
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(), end = map->selectedObjectsEnd(); it != end; ++it)
	{
		Object* new_object = (*it)->duplicate();
		if (symbol_map.contains(new_object->getSymbol()))
			new_object->setSymbol(symbol_map.value(new_object->getSymbol()), true);
		
		copy_map.addObject(new_object);
	}
	
	// Save map to memory
	QBuffer buffer;
	if (!copy_map.exportToIODevice(&buffer))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	// Put buffer into clipboard
	QMimeData* mime_data = new QMimeData();
	mime_data->setData(MimeType::oo_objects, buffer.data());
	QApplication::clipboard()->setMimeData(mime_data);
	
	// Show message
	window->showStatusBarMessage(tr("Copied %1 object(s)").arg(map->getNumSelectedObjects()), 2000);
}


void MapEditorController::paste()
{
	if (editing_in_progress)
		return;
	if (!QApplication::clipboard()->mimeData()->hasFormat(MimeType::oo_objects))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("There are no objects in clipboard which could be pasted!"));
		return;
	}
	
	// Get buffer from clipboard
	QByteArray byte_array = QApplication::clipboard()->mimeData()->data(MimeType::oo_objects);
	QBuffer buffer(&byte_array);
	buffer.open(QIODevice::ReadOnly);
	
	// Create map from buffer
	Map paste_map;
	if (!paste_map.importFromIODevice(&buffer))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("An internal error occurred, sorry!"));
		return;
	}
	
	// Move objects in paste_map so their bounding box center is at this map's viewport center.
	// This makes the pasted objects appear at the center of the viewport.
	QRectF paste_extent = paste_map.calculateExtent(true, false, NULL);
	auto offset = main_view->center() - paste_extent.center();
	
	MapPart* part = paste_map.getCurrentPart();
	for (int i = 0; i < part->getNumObjects(); ++i)
		part->getObject(i)->move(offset);
	
	// Import pasted map. Do not blindly import all colors.
	map->importMap(&paste_map, Map::MinimalObjectImport, window);
	
	// Show message
	window->showStatusBarMessage(tr("Pasted %1 object(s)").arg(paste_map.getNumObjects()), 2000);
}


void MapEditorController::clearUndoRedoHistory()
{
	map->undoManager().clear();
	map->setOtherDirty();
}

void MapEditorController::spotColorPresenceChanged(bool has_spot_colors)
{
	if (overprinting_simulation_act != NULL)
	{
		if (has_spot_colors)
		{
			overprinting_simulation_act->setEnabled(true);
		}
		else
		{
			overprinting_simulation_act->setChecked(false);
			overprinting_simulation_act->setEnabled(false);
		}
	}
}

void MapEditorController::showGrid()
{
	main_view->setGridVisible(show_grid_act->isChecked());
}

void MapEditorController::configureGrid()
{
	ConfigureGridDialog dialog(window, *map, show_grid_act->isChecked());
	dialog.setWindowModality(Qt::WindowModal);
	if (dialog.exec() == QDialog::Accepted)
	{
		map->setGrid(dialog.resultGrid());
		if (dialog.gridVisible() != show_grid_act->isChecked())
			show_grid_act->trigger();
	}
}

void MapEditorController::pan()
{
	setTool(new PanTool(this, pan_act));
}
void MapEditorController::zoomIn()
{
	main_view->zoomSteps(1);
}
void MapEditorController::zoomOut()
{
	main_view->zoomSteps(-1);
}
void MapEditorController::setCustomZoomFactorClicked()
{
	bool ok;
	double factor = QInputDialog::getDouble(window, tr("Set custom zoom factor"), tr("Zoom factor:"), main_view->getZoom(), MapView::zoom_out_limit, MapView::zoom_in_limit, 3, &ok);
	if (!ok || factor == main_view->getZoom())
		return;
	
	main_view->setZoom(factor);
}

void MapEditorController::hatchAreas(bool checked)
{
	map->setAreaHatchingEnabled(checked);
	// Update all areas
	map->applyOnMatchingObjects(ObjectOp::ForceUpdate(), ObjectOp::ContainsSymbolType(Symbol::Area));
}

void MapEditorController::baselineView(bool checked)
{
	map->setBaselineViewEnabled(checked);
	map->updateAllObjects();
}

void MapEditorController::hideAllTemplates(bool checked)
{
	hide_all_templates_act->setChecked(checked);
	main_view->setAllTemplatesHidden(checked);
}

void MapEditorController::overprintingSimulation(bool checked)
{
	if (checked && !map->hasSpotColors())
	{
		checked = false;
	}
	main_view->setOverprintingSimulationEnabled(checked);
}

void MapEditorController::coordsDisplayChanged()
{
	if (geographic_coordinates_dms_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::GEOGRAPHIC_COORDS_DMS);
	else if (geographic_coordinates_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::GEOGRAPHIC_COORDS);
	else if (projected_coordinates_act->isChecked())
		map_widget->setCoordsDisplay(MapWidget::PROJECTED_COORDS);	
	else
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
}

void MapEditorController::copyDisplayedCoords()
{
	QApplication::clipboard()->setText(statusbar_cursorpos_label->text());
}

void MapEditorController::projectionChanged()
{
	const Georeferencing& geo(map->getGeoreferencing());
	
	projected_coordinates_act->setText(geo.getProjectedCoordinatesName());
	
	bool enable_geographic = !geo.isLocal();
	geographic_coordinates_act->setEnabled(enable_geographic);
	geographic_coordinates_dms_act->setEnabled(enable_geographic);
	if (!enable_geographic &&
		(map_widget->getCoordsDisplay() == MapWidget::GEOGRAPHIC_COORDS ||
		map_widget->getCoordsDisplay() == MapWidget::GEOGRAPHIC_COORDS_DMS))
	{
		map_widget->setCoordsDisplay(MapWidget::MAP_COORDS);
		map_coordinates_act->setChecked(true);
	}
}

void MapEditorController::showSymbolWindow(bool show)
{
	if (!symbol_dock_widget)
	{
		symbol_dock_widget = new EditorDockWidget(tr("Symbols"), symbol_window_act, this, window);
		createSymbolWidget();
		symbol_dock_widget->setObjectName(QString::fromLatin1("Symbol dock widget"));
		if (!window->restoreDockWidget(symbol_dock_widget))
			window->addDockWidget(Qt::RightDockWidgetArea, symbol_dock_widget, Qt::Vertical);
	}
	
	symbol_dock_widget->setVisible(show);
}

void MapEditorController::createColorWindow()
{
	Q_ASSERT(!color_dock_widget);
	
	color_dock_widget = new EditorDockWidget(tr("Colors"), color_window_act, this, window);
	color_dock_widget->setWidget(new ColorWidget(map, window, color_dock_widget));
	color_dock_widget->widget()->setEnabled(!editing_in_progress);
	color_dock_widget->setObjectName(QString::fromLatin1("Color dock widget"));
	if (!window->restoreDockWidget(color_dock_widget))
		window->addDockWidget(Qt::LeftDockWidgetArea, color_dock_widget, Qt::Vertical);
	color_dock_widget->setVisible(false);
}

void MapEditorController::showColorWindow(bool show)
{
	if (!color_dock_widget)
		createColorWindow();
	
	color_dock_widget->setVisible(show);
}

void MapEditorController::loadSymbolsFromClicked()
{
	ReplaceSymbolSetDialog::showDialog(window, map);
}
void MapEditorController::loadColorsFromClicked()
{
	// TODO
}

void MapEditorController::scaleAllSymbolsClicked()
{
	bool ok;
	double percent = QInputDialog::getDouble(window, tr("Scale all symbols"), tr("Scale to percentage:"), 100, 0, 999999, 6, &ok);
	if (!ok || percent == 100)
		return;
	
	map->scaleAllSymbols(percent / 100.0);
}

void MapEditorController::scaleMapClicked()
{
	ScaleMapDialog dialog(window, map);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
}

void MapEditorController::rotateMapClicked()
{
	RotateMapDialog dialog(window, map);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
}

void MapEditorController::mapNotesClicked()
{
	QDialog dialog(window, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	dialog.setWindowTitle(tr("Map notes"));
	dialog.setWindowModality(Qt::WindowModal);
	
	QTextEdit* text_edit = new QTextEdit();
	text_edit->setPlainText(map->getMapNotes());
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	QPushButton* ok_button = new QPushButton(QIcon(QString::fromLatin1(":/images/arrow-right.png")), tr("OK"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(text_edit);
	layout->addLayout(buttons_layout);
	dialog.setLayout(layout);
	
	connect(cancel_button, SIGNAL(clicked(bool)), &dialog, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), &dialog, SLOT(accept()));
	
	if (dialog.exec() == QDialog::Accepted)
	{
		if (text_edit->toPlainText() != map->getMapNotes())
		{
			map->setMapNotes(text_edit->toPlainText());
			map->setHasUnsavedChanges(true);
		}
	}
}

void MapEditorController::createTemplateWindow()
{
	Q_ASSERT(!template_dock_widget);
	
	template_list_widget = new TemplateListWidget(map, main_view, this);
	connect(hide_all_templates_act, &QAction::toggled, template_list_widget, &TemplateListWidget::setAllTemplatesHidden);
	
	if (isInMobileMode())
	{
		template_dock_widget = createDockWidgetSubstitute(window, template_list_widget);
		connect(template_list_widget, &TemplateListWidget::closeClicked, [this]() { showTemplateWindow(false); });
	}
	else
	{
		auto dock_widget = new EditorDockWidget(tr("Templates"), template_window_act, this, window);
		dock_widget->setWidget(template_list_widget);
		dock_widget->setObjectName(QString::fromLatin1("Templates dock widget"));
		if (!window->restoreDockWidget(dock_widget))
			window->addDockWidget(Qt::RightDockWidgetArea, dock_widget, Qt::Vertical);
		dock_widget->setVisible(false);
		
		template_dock_widget = dock_widget;
	}
}

void MapEditorController::showTemplateWindow(bool show)
{
	if (!template_dock_widget)
		createTemplateWindow();
	
	template_window_act->setChecked(show);
	template_dock_widget->setVisible(show);
}

void MapEditorController::openTemplateClicked()
{
	auto new_template = TemplateListWidget::showOpenTemplateDialog(window, this);
	if (new_template)
	{
		hideAllTemplates(false);
		showTemplateWindow(true);
		
		// FIXME: this should be done through the core map, not through the UI
		template_list_widget->addTemplateAt(new_template.release(), -1);
	}
}

void MapEditorController::reopenTemplateClicked()
{
	hideAllTemplates(false);
	
	QString map_directory = window->currentPath();
	if (!map_directory.isEmpty())
		map_directory = QFileInfo(map_directory).canonicalPath();
	ReopenTemplateDialog* dialog = new ReopenTemplateDialog(window, map, map_directory); 
	dialog->setWindowModality(Qt::WindowModal);
	dialog->exec();
	delete dialog;
}

void MapEditorController::templateAvailabilityChanged()
{
	// Nothing
}

void MapEditorController::closedTemplateAvailabilityChanged()
{
	if (reopen_template_act)
		reopen_template_act->setEnabled(map->getNumClosedTemplates() > 0);
}

void MapEditorController::createTagEditor()
{
	Q_ASSERT(!tags_dock_widget);
	
	TagsWidget* tags_widget = new TagsWidget(map, main_view, this);
	tags_dock_widget = new EditorDockWidget(tr("Tag Editor"), tags_window_act, this, window);
	tags_dock_widget->setWidget(tags_widget);
	tags_dock_widget->setObjectName(QString::fromLatin1("Tag editor dock widget"));
	if (!window->restoreDockWidget(tags_dock_widget))
		window->addDockWidget(Qt::RightDockWidgetArea, tags_dock_widget, Qt::Vertical);
	tags_dock_widget->setVisible(false);
}

void MapEditorController::showTagsWindow(bool show)
{
	if (!tags_dock_widget)
		createTagEditor();
	
	tags_window_act->setChecked(show);
	tags_dock_widget->setVisible(show);
}

void MapEditorController::createTagSelector()
{
	Q_ASSERT(!tag_select_dock_widget);

	TagSelectWidget* tag_widget = new TagSelectWidget(map, main_view, this);
	tag_select_dock_widget = new EditorDockWidget(tr("Tag Selector"), tag_select_window_act, this, window);
	tag_select_dock_widget->setWidget(tag_widget);
	tag_select_dock_widget->setObjectName(QString::fromLatin1("Tag selection dock widget"));
	if (!window->restoreDockWidget(tag_select_dock_widget))
		window->addDockWidget(Qt::RightDockWidgetArea, tag_select_dock_widget, Qt::Vertical);
	tag_select_dock_widget->setVisible(false);
}


void MapEditorController::showTagSelectWindow(bool show)
{
	if (!tag_select_dock_widget)
		createTagSelector();

	tag_select_window_act->setChecked(show);
	tag_select_dock_widget->setVisible(show);
}

void MapEditorController::editGeoreferencing()
{
	if (georeferencing_dialog.isNull())
	{
		GeoreferencingDialog* dialog = new GeoreferencingDialog(this); 
		georeferencing_dialog.reset(dialog);
		connect(dialog, SIGNAL(finished(int)), this, SLOT(georeferencingDialogFinished()));
	}
	georeferencing_dialog->exec();
}

void MapEditorController::georeferencingDialogFinished()
{
	georeferencing_dialog.take()->deleteLater();
	map->updateAllMapWidgets();
	
	bool gps_display_possible = map->getGeoreferencing().isValid() && ! map->getGeoreferencing().isLocal();
	if (!gps_display_possible)
	{
		gps_display_action->setChecked(false);
		if (gps_display)
			enableGPSDisplay(false);
	}
	gps_display_action->setEnabled(gps_display_possible);
}

void MapEditorController::selectedSymbolsChanged()
{
	Symbol* symbol = symbol_widget->getSingleSelectedSymbol();
	
	if (mobile_mode)
	{
		// (Re-)create the mobile_symbol_selector_action icon
		QSize icon_size = bottom_action_bar->getIconSize(2, 2);
		QPixmap pixmap(icon_size);
		pixmap.fill(Qt::white);
		if (symbol_widget->selectedSymbolsCount() != 1)
		{
			QFont font(window->font());
			font.setPixelSize(icon_size.height() / 5);
			QPainter painter(&pixmap);
			painter.setFont(font);
			QString text = (symbol_widget->selectedSymbolsCount() == 0) ?
				tr("No\nsymbol\nselected", "Keep it short. Should not be much longer per line than the longest word in the original.") :
				tr("Multiple\nsymbols\nselected", "Keep it short. Should not be much longer per line than the longest word in the original.");
			painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
		}
		else //if (symbol_widget->getNumSelectedSymbols() == 1)
		{
			QImage image = symbol->createIcon(map, qMin(icon_size.width(), icon_size.height()), Util::isAntialiasingRequired(), 0, 4.0f);
			QPainter painter(&pixmap);
			painter.drawImage(pixmap.rect(), image);
		}
		mobile_symbol_selector_action->setIcon(QIcon(pixmap));
	}
	
	// FIXME: Postpone switch of active symbol while editing is progress
	if (active_symbol != symbol)
	{
		active_symbol = symbol;
		
		if (symbol && !symbol->isHidden() && !symbol->isProtected() && current_tool)
		{
			// Auto-switch to a draw tool when selecting a symbol under certain conditions
			if (current_tool->toolType() == MapEditorTool::Pan
			    || ((current_tool->toolType() == MapEditorTool::EditLine || current_tool->toolType() == MapEditorTool::EditPoint) && map->getNumSelectedObjects() == 0))
			{
				current_tool->switchToDefaultDrawTool(active_symbol);
			}
		}
	}
	
	// Even when the symbol (pointer) hasn't changed,
	// the symbol's visibility may constitute a change.
	emit activeSymbolChanged(active_symbol);
	
	updateSymbolDependentActions();
	updateSymbolAndObjectDependentActions();
}

void MapEditorController::objectSelectionChanged()
{
	if (mode != MapEditor)
		return;
	
	updateObjectDependentActions();
	
	// Automatic symbol selection of selected objects
	if (symbol_widget && !editing_in_progress)
	{
		bool uniform_symbol_selected = true;
		const Symbol* uniform_symbol = NULL;
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		{
			const Symbol* symbol = (*it)->getSymbol();
			if (!uniform_symbol)
			{
				uniform_symbol = symbol;
			}
			else if (uniform_symbol != symbol)
			{
				uniform_symbol_selected = false;
				break;
			}
		}
		if (uniform_symbol_selected && Settings::getInstance().getSettingCached(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool())
			symbol_widget->selectSingleSymbol(uniform_symbol);
	}
	
	updateSymbolAndObjectDependentActions();
}

void MapEditorController::updateSymbolDependentActions()
{
	const Symbol* symbol = activeSymbol();
	const Symbol::Type type = (symbol && !editing_in_progress) ? symbol->getType() : Symbol::NoSymbol;
	
	updateDrawPointGPSAvailability();
	draw_point_act->setEnabled(type == Symbol::Point && !symbol->isHidden());
	draw_point_act->setStatusTip(tr("Place point objects on the map.") + (draw_point_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a point symbol to be able to use this tool."))));
	draw_path_act->setEnabled((type == Symbol::Line || type == Symbol::Area || type == Symbol::Combined) && !symbol->isHidden());
	draw_path_act->setStatusTip(tr("Draw polygonal and curved lines.") + (draw_path_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_circle_act->setEnabled(draw_path_act->isEnabled());
	draw_circle_act->setStatusTip(tr("Draw circles and ellipses.") + (draw_circle_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_rectangle_act->setEnabled(draw_path_act->isEnabled());
	draw_rectangle_act->setStatusTip(tr("Draw rectangles.") + (draw_rectangle_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_freehand_act->setEnabled(draw_path_act->isEnabled());
	draw_freehand_act->setStatusTip(tr("Draw paths free-handedly.") + (draw_freehand_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_fill_act->setEnabled(draw_path_act->isEnabled());
	draw_fill_act->setStatusTip(tr("Fill bounded areas.") + (draw_fill_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a line, area or combined symbol to be able to use this tool."))));
	draw_text_act->setEnabled(type == Symbol::Text && !symbol->isHidden());
	draw_text_act->setStatusTip(tr("Write text on the map.") + (draw_text_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a text symbol to be able to use this tool."))));
}

void MapEditorController::updateObjectDependentActions()
{
	bool have_multiple_parts     = map->getNumParts() > 1;
	bool have_selection          = map->getNumSelectedObjects() > 0 && !editing_in_progress;
	bool single_object_selected  = map->getNumSelectedObjects() == 1 && !editing_in_progress;
	bool have_line               = false;
	bool have_area               = false;
	bool have_area_with_holes    = false;
	bool have_rotatable_pattern  = false;
	bool have_rotatable_point    = false;
	int  num_selected_paths      = 0;
	bool first_selected_is_path  = have_selection && map->getFirstSelectedObject()->getType() == Object::Path;
	bool uniform_symbol_selected = true;
	const Symbol* uniform_symbol = NULL;
	const Symbol* first_selected_symbol= have_selection ? map->getFirstSelectedObject()->getSymbol() : NULL;
	std::vector< bool > symbols_in_selection(map->getNumSymbols(), false);
	
	if (!editing_in_progress)
	{
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		{
			const Symbol* symbol = (*it)->getSymbol();
			int symbol_index = map->findSymbolIndex(symbol);
			if (symbol_index >= 0 && symbol_index < (int)symbols_in_selection.size())
				symbols_in_selection[symbol_index] = true;
			
			if (uniform_symbol_selected)
			{
				if (!uniform_symbol)
				{
					uniform_symbol = symbol;
				}
				else if (uniform_symbol != symbol)
				{
					uniform_symbol = NULL;
					uniform_symbol_selected = false;
				}
			}
			
			if (symbol->getType() == Symbol::Point)
			{
				have_rotatable_point |= symbol->asPoint()->isRotatable();
			}
			else if (Symbol::areTypesCompatible(symbol->getType(), Symbol::Area))
			{
				++num_selected_paths;
				
				if (symbol->getType() == Symbol::Area)
				{
					have_rotatable_pattern |= symbol->asArea()->hasRotatableFillPattern();
				}
				
				int const contained_types = symbol->getContainedTypes();
				
				if (contained_types & Symbol::Line)
				{
					have_line = true;
				}
				
				if (contained_types & Symbol::Area)
				{
					have_area = true;
					have_area_with_holes |= (*it)->asPath()->parts().size() > 1;
				}
			}
		}
		
		if (have_area && !have_rotatable_pattern)
		{
			map->determineSymbolUseClosure(symbols_in_selection);
			for (size_t i = 0, end = symbols_in_selection.size(); i < end; ++i)
			{
				if (symbols_in_selection[i])
				{
					Symbol* symbol = map->getSymbol(i);
					if (symbol->getType() == Symbol::Area)
					{
						have_rotatable_pattern = symbol->asArea()->hasRotatableFillPattern();
						break;
					}
				}
			}
		}
	}
	
	// have_selection
	cut_act->setEnabled(have_selection);
	copy_act->setEnabled(have_selection);
	delete_act->setEnabled(have_selection);
	delete_act->setStatusTip(tr("Deletes the selected object(s).") + (delete_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object to activate this tool."))));
	duplicate_act->setEnabled(have_selection);
	duplicate_act->setStatusTip(tr("Duplicate the selected object(s).") + (duplicate_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object to activate this tool."))));
	rotate_act->setEnabled(have_selection);
	rotate_act->setStatusTip(tr("Rotate the selected object(s).") + (rotate_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object to activate this tool."))));
	scale_act->setEnabled(have_selection);
	scale_act->setStatusTip(tr("Scale the selected object(s).") + (scale_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object to activate this tool."))));
	mappart_move_menu->setEnabled(have_selection && have_multiple_parts);
	
	// have_rotatable_pattern || have_rotatable_point
	rotate_pattern_act->setEnabled(have_rotatable_pattern || have_rotatable_point);
	rotate_pattern_act->setStatusTip(tr("Set the direction of area fill patterns or point objects.") + (rotate_pattern_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select an area object with rotatable fill pattern or a rotatable point object to activate this tool."))));
	
	// have_line
	switch_dashes_act->setEnabled(have_line);
	switch_dashes_act->setStatusTip(tr("Switch the direction of symbols on line objects.") + (switch_dashes_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one line object to activate this tool."))));
	connect_paths_act->setEnabled(have_line);
	connect_paths_act->setStatusTip(tr("Connect endpoints of paths which are close together.") + (connect_paths_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one line object to activate this tool."))));
	
	// have_are || have_line
	cut_tool_act->setEnabled(have_area || have_line);
	cut_tool_act->setStatusTip(tr("Cut the selected object(s) into smaller parts.") + (cut_tool_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one line or area object to activate this tool."))));
	convert_to_curves_act->setEnabled(have_area || have_line);
	convert_to_curves_act->setStatusTip(tr("Turn paths made of straight segments into smooth bezier splines.") + (convert_to_curves_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a path object to activate this tool."))));
	simplify_path_act->setEnabled(have_area || have_line);
	simplify_path_act->setStatusTip(tr("Reduce the number of points in path objects while trying to retain their shape.") + (simplify_path_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a path object to activate this tool."))));
	
	// cut_hole_enabled
	const bool cut_hole_enabled = single_object_selected && have_area;
	cut_hole_act->setEnabled(cut_hole_enabled);
	cut_hole_act->setStatusTip(tr("Cut a hole into the selected area object.") + (cut_hole_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a single area object to activate this tool."))));
	cut_hole_circle_act->setEnabled(cut_hole_enabled);
	cut_hole_circle_act->setStatusTip(cut_hole_act->statusTip());
	cut_hole_rectangle_act->setEnabled(cut_hole_enabled);
	cut_hole_rectangle_act->setStatusTip(cut_hole_act->statusTip());
	cut_hole_menu->setEnabled(cut_hole_enabled);
	
	// boolean_prerequisite [&& x]
	bool const boolean_prerequisite = first_selected_is_path && num_selected_paths >= 2;
	QString const extra_status_tip = QLatin1Char(' ') +
	                                 ( boolean_prerequisite
	                                 ? tr("Resulting symbol: %1 %2.").arg(first_selected_symbol->getNumberAsString(), first_selected_symbol->getPlainTextName())
	                                 : tr("Select at least two area or path objects activate this tool.") );
	boolean_union_act->setEnabled(boolean_prerequisite);
	boolean_union_act->setStatusTip(tr("Unify overlapping objects.") + extra_status_tip);
	boolean_intersection_act->setEnabled(boolean_prerequisite);
	boolean_intersection_act->setStatusTip(tr("Remove all parts which are not overlaps with the first selected object.") + extra_status_tip);
	boolean_difference_act->setEnabled(boolean_prerequisite);
	boolean_difference_act->setStatusTip(tr("Remove overlapped parts of the first selected object.") + (boolean_prerequisite ? QString{} : extra_status_tip));
	boolean_xor_act->setEnabled(boolean_prerequisite);
	boolean_xor_act->setStatusTip(tr("Remove all parts which overlap the first selected object.") + extra_status_tip);
	
	// special
	boolean_merge_holes_act->setEnabled(single_object_selected && have_area_with_holes);
	boolean_merge_holes_act->setStatusTip(tr("Merge area holes together, or merge holes with the object boundary to cut out this part.") + (boolean_merge_holes_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select one area object with holes to activate this tool."))));
	
	// cutout_enabled
	bool const cutout_enabled = single_object_selected && (have_area || have_line) && !have_area_with_holes && (*(map->selectedObjectsBegin()))->asPath()->parts().front().isClosed();
	cutout_physical_act->setEnabled(cutout_enabled);
	cutout_physical_act->setStatusTip(tr("Create a cutout of some objects or the whole map.") + (cutout_physical_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a closed path object as cutout shape to activate this tool."))));
	cutaway_physical_act->setEnabled(cutout_enabled);
	cutaway_physical_act->setStatusTip(tr("Cut away some objects or everything in a limited area.") + (cutaway_physical_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select a closed path object as cutout shape to activate this tool."))));
}

void MapEditorController::updateSymbolAndObjectDependentActions()
{
	const Symbol* single_symbol = activeSymbol();
	bool path_object_among_selection = false;
	bool single_symbol_compatible = false;
	bool single_symbol_different  = false;
	if (!editing_in_progress)
	{
		Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
		for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
		{
			if ((*it)->getType() == Object::Path)
			{
				path_object_among_selection = true;
				break;
			}
		}
		
		map->getSelectionToSymbolCompatibility(single_symbol, single_symbol_compatible, single_symbol_different);
	}
	
	switch_symbol_act->setEnabled(single_symbol_compatible && single_symbol_different);
	switch_symbol_act->setStatusTip(tr("Switches the symbol of the selected object(s) to the selected symbol.") + (switch_symbol_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object and a fitting, different symbol to activate this tool."))));
	fill_border_act->setEnabled(single_symbol_compatible && single_symbol_different);
	fill_border_act->setStatusTip(tr("Fill the selected line(s) or create a border for the selected area(s).") + (fill_border_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one object and a fitting, different symbol to activate this tool."))));
	distribute_points_act->setEnabled(
		single_symbol
		&& single_symbol->getType() == Symbol::Point
		&& path_object_among_selection);
	distribute_points_act->setStatusTip(tr("Places evenly spaced point objects along an existing path object") + (distribute_points_act->isEnabled() ? QString{} : QString(QLatin1Char(' ') + tr("Select at least one path object and a single point symbol to activate this tool."))));
}

void MapEditorController::undoStepAvailabilityChanged()
{
	if (mode != MapEditor)
		return;
	
	undo_act->setEnabled(map->undoManager().canUndo());
	redo_act->setEnabled(map->undoManager().canRedo());
	clear_undo_redo_history_act->setEnabled(undo_act->isEnabled() || redo_act->isEnabled());
}

void MapEditorController::clipboardChanged(QClipboard::Mode mode)
{
	if (mode == QClipboard::Clipboard)
		updatePasteAvailability();
}

void MapEditorController::updatePasteAvailability()
{
	if (paste_act)
	{
		paste_act->setEnabled(
			QApplication::clipboard()->mimeData()
			&& QApplication::clipboard()->mimeData()->hasFormat(MimeType::oo_objects)
			&& !editing_in_progress);
	}
}

void MapEditorController::showWholeMap()
{
	QRectF map_extent = map->calculateExtent(true, !main_view->areAllTemplatesHidden(), main_view);
	map_widget->adjustViewToRect(map_extent, MapWidget::ContinuousZoom);
}

void MapEditorController::editToolClicked()
{
	setEditTool();
}

void MapEditorController::editLineToolClicked()
{
	if (!current_tool || current_tool->toolType() != MapEditorTool::EditLine)
		setTool(new EditLineTool(this, edit_line_tool_act));
}

void MapEditorController::drawPointClicked()
{
	setTool(new DrawPointTool(this, draw_point_act));
}

void MapEditorController::drawPathClicked()
{
	setTool(new DrawPathTool(this, draw_path_act, false, true));
}

void MapEditorController::drawCircleClicked()
{
	setTool(new DrawCircleTool(this, draw_circle_act, false));
}

void MapEditorController::drawRectangleClicked()
{
	setTool(new DrawRectangleTool(this, draw_rectangle_act, false));
}

void MapEditorController::drawFreehandClicked()
{
	setTool(new DrawFreehandTool(this, draw_freehand_act, false));
}

void MapEditorController::drawFillClicked()
{
	setTool(new FillTool(this, draw_fill_act));
}

void MapEditorController::drawTextClicked()
{
	setTool(new DrawTextTool(this, draw_text_act));
}

void MapEditorController::deleteClicked()
{
	if (editing_in_progress)
		return;
	map->deleteSelectedObjects();
}

void MapEditorController::duplicateClicked()
{
	std::vector<Object*> new_objects;
	new_objects.reserve(map->getNumSelectedObjects());
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* duplicate = (*it)->duplicate();
		map->addObject(duplicate);
		new_objects.push_back(duplicate);
	}
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		undo_step->addObject(part->findObjectIndex(new_objects[i]));
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
	}
	
	map->setObjectsDirty();
	map->push(undo_step);
	setEditTool();
	window->showStatusBarMessage(tr("%1 object(s) duplicated").arg((int)new_objects.size()), 2000);
}

void MapEditorController::switchSymbolClicked()
{
	SwitchSymbolUndoStep* switch_step = NULL;
	ReplaceObjectsUndoStep* replace_step = NULL;
	AddObjectsUndoStep* add_step = NULL;
	DeleteObjectsUndoStep* delete_step = NULL;
	std::vector<Object*> old_objects;
	std::vector<Object*> new_objects;
	MapPart* part = map->getCurrentPart();
	Symbol* symbol = activeSymbol();
	
	bool close_paths = false, split_up = false;
	Symbol::Type contained_types = symbol->getContainedTypes();
	if (contained_types & Symbol::Area && !(contained_types & Symbol::Line))
		close_paths = true;
	else if (contained_types & Symbol::Line && !(contained_types & Symbol::Area))
		split_up = true;
	
	if (close_paths)
	{
		replace_step = new ReplaceObjectsUndoStep(map);
	}
	else if (split_up)
	{
		add_step = new AddObjectsUndoStep(map);
		delete_step = new DeleteObjectsUndoStep(map);
	}
	else
	{
		switch_step = new SwitchSymbolUndoStep(map);
	}
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
		
		if (close_paths)
		{
			replace_step->addObject(part->findObjectIndex(object), object->duplicate());
		}
		else if (split_up)
		{
			add_step->addObject(part->findObjectIndex(object), object);
			old_objects.push_back(object);
		}
		else
		{
			switch_step->addObject(part->findObjectIndex(object), object->getSymbol());
		}
		
		if (!split_up)
		{
			object->setSymbol(symbol, true);
		}
		
		if (object->getType() == Object::Path)
		{
			PathObject* path_object = object->asPath();
			if (close_paths)
			{
				path_object->closeAllParts();
			}
			else if (split_up)
			{
				for (const auto& part : path_object->parts())
				{
					PathObject* new_object = new PathObject { part };
					new_object->setSymbol(symbol, true);
					new_objects.push_back(new_object);
				}
				
				Q_ASSERT(!new_objects.empty());
				if (!new_objects.empty())
				{
					new_objects.front()->setTags(path_object->tags());
				}
			}
		}
		
		object->update();
	}
	
	if (split_up)	
	{
		map->clearObjectSelection(false);
		for (size_t i = 0; i < old_objects.size(); ++i)
		{
			Object* object = old_objects[i];
			map->deleteObject(object, true);
		}
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			map->addObject(object);
			map->addObjectToSelection(object, i == new_objects.size() - 1);
		}
		// Do not merge this loop into the upper one;
		// theoretically undo step indices could be wrong this way.
		for (size_t i = 0; i < new_objects.size(); ++i)
		{
			Object* object = new_objects[i];
			delete_step->addObject(part->findObjectIndex(object));
		}
	}
	
	map->setObjectsDirty();
	if (close_paths)
		map->push(replace_step);
	else if (split_up)
	{
		CombinedUndoStep* combined_step = new CombinedUndoStep(map);
		combined_step->push(add_step);
		combined_step->push(delete_step);
		map->push(combined_step);
	}
	else
		map->push(switch_step);
	map->emitSelectionEdited();
	// Also emit selectionChanged, as symbols of selected objects changed
	map->emitSelectionChanged();
}

void MapEditorController::fillBorderClicked()
{
	Symbol* symbol = activeSymbol();
	std::vector<Object*> new_objects;
	new_objects.reserve(map->getNumSelectedObjects());
	
	bool close_paths = false, split_up = false;
	Symbol::Type contained_types = symbol->getContainedTypes();
	if (contained_types & Symbol::Area && !(contained_types & Symbol::Line))
		close_paths = true;
	else if (contained_types & Symbol::Line && !(contained_types & Symbol::Area))
		split_up = true;
	
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		Object* object = *it;
		if (split_up && object->getType() == Object::Path)
		{
			PathObject* path_object = object->asPath();
			for (const auto& part : path_object->parts())
			{
				PathObject* new_object = new PathObject { part };
				new_object->setSymbol(symbol, true);
				map->addObject(new_object);
				new_objects.push_back(new_object);
			}
		}
		else
		{
			Object* duplicate = object->duplicate();
			duplicate->setSymbol(symbol, true);
			if (close_paths && duplicate->getType() == Object::Path)
			{
				PathObject* path_object = duplicate->asPath();
				path_object->closeAllParts();
			}
			map->addObject(duplicate);
			new_objects.push_back(duplicate);
		}
	}
	
	map->setObjectsDirty();
	map->clearObjectSelection(false);
	for (int i = 0; i < (int)new_objects.size(); ++i)
	{
		map->addObjectToSelection(new_objects[i], i == (int)new_objects.size() - 1);
		undo_step->addObject(part->findObjectIndex(new_objects[i]));
	}
	map->push(undo_step);
}
void MapEditorController::selectObjectsClicked(bool select_exclusively)
{
	bool selection_changed = false;
	if (select_exclusively)
	{
		if (map->getNumSelectedObjects() > 0)
			selection_changed = true;
		map->clearObjectSelection(false);
	}

	bool object_selected = false;	
	MapPart* part = map->getCurrentPart();
	for (int i = 0, size = part->getNumObjects(); i < size; ++i)
	{
		Object* object = part->getObject(i);
		if (symbol_widget->isSymbolSelected(object->getSymbol()) && !(!select_exclusively && map->isObjectSelected(object)))
		{
			map->addObjectToSelection(object, false);
			object_selected = true;
		}
	}
	
	selection_changed |= object_selected;
	if (selection_changed)
		map->emitSelectionChanged();
	
	if (object_selected)
	{
		if (current_tool && current_tool->isDrawTool())
			setEditTool();
	}
	else
		QMessageBox::warning(window, tr("Object selection"), tr("No objects were selected because there are no objects with the selected symbol(s)."));
}
void MapEditorController::deselectObjectsClicked()
{
	bool selection_changed = false;
	
	MapPart* part = map->getCurrentPart();
	for (int i = 0, size = part->getNumObjects(); i < size; ++i)
	{
		Object* object = part->getObject(i);
		if (symbol_widget->isSymbolSelected(object->getSymbol()) && map->isObjectSelected(object))
		{
			map->removeObjectFromSelection(object, false);
			selection_changed = true;
		}
	}
	
	if (selection_changed)
	{
		map->emitSelectionChanged();
		
		if (current_tool && current_tool->isDrawTool())
			setEditTool();
	}
}

void MapEditorController::selectAll()
{
	auto num_selected_objects = map->getNumSelectedObjects();
	map->clearObjectSelection(false);
	map->getCurrentPart()->applyOnAllObjects([this](Object* object, MapPart*, int) {
		map->addObjectToSelection(object, false);
		return true;
	});
	
	if (map->getNumSelectedObjects() != num_selected_objects)
	{
		map->emitSelectionChanged();
		if (current_tool && current_tool->isDrawTool())
			setEditTool();
	}
}

void MapEditorController::selectNothing()
{
	if (map->getNumSelectedObjects() > 0)
		map->clearObjectSelection(true);
}

void MapEditorController::invertSelection()
{
	auto selection = Map::ObjectSelection{ map->selectedObjects() };
	map->clearObjectSelection(false);
	map->getCurrentPart()->applyOnAllObjects([this, &selection](Object* object, MapPart*, int) {
		if (selection.find(object) == end(selection))
		{
			map->addObjectToSelection(object, false);
		}
		return true;
	});
	
	if (map->getCurrentPart()->getNumObjects() > 0)
	{
		map->emitSelectionChanged();
		if (current_tool && current_tool->isDrawTool())
			setEditTool();
	}
}

void MapEditorController::selectByCurrentSymbols()
{
	selectObjectsClicked(true);
}

void MapEditorController::switchDashesClicked()
{
	SwitchDashesUndoStep* undo_step = new SwitchDashesUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line)
		{
			PathObject* path = reinterpret_cast<PathObject*>(*it);
			path->reverse();
			(*it)->update();
			
			undo_step->addObject(part->findObjectIndex(*it));
		}
	}
	
	map->setObjectsDirty();
	map->push(undo_step);
	map->emitSelectionEdited();
}

/// \todo Review use of container API
float connectPaths_FindClosestEnd(const std::vector<Object*>& objects, PathObject* a, int a_index, PathPartVector::size_type path_part_a, bool path_part_a_begin, PathObject** out_b, int* out_b_index, int* out_path_part_b, bool* out_path_part_b_begin)
{
	float best_dist_sq = std::numeric_limits<float>::max();
	for (int i = a_index; i < (int)objects.size(); ++i)
	{
		PathObject* b = reinterpret_cast<PathObject*>(objects[i]);
		if (b->getSymbol() != a->getSymbol())
			continue;
		
		auto num_parts = b->parts().size();
		for (PathPartVector::size_type path_part_b = (a == b) ? path_part_a : 0; path_part_b < num_parts; ++path_part_b)
		{
			PathPart& part = b->parts()[path_part_b];
			if (!part.isClosed())
			{
				for (int begin = 0; begin < 2; ++begin)
				{
					bool path_part_b_begin = (begin == 0);
					if (a == b && path_part_a == path_part_b && path_part_a_begin == path_part_b_begin)
						continue;
					
					MapCoord& coord_a = a->getCoordinate(path_part_a_begin ? a->parts()[path_part_a].first_index : (a->parts()[path_part_a].last_index));
					MapCoord& coord_b = b->getCoordinate(path_part_b_begin ? b->parts()[path_part_b].first_index : (b->parts()[path_part_b].last_index));
					float distance_sq = coord_a.distanceSquaredTo(coord_b);
					if (distance_sq < best_dist_sq)
					{
						best_dist_sq = distance_sq;
						*out_b = b;
						*out_b_index = i;
						*out_path_part_b = path_part_b;
						*out_path_part_b_begin = path_part_b_begin;
						if (distance_sq == 0)
							return 0;
					}
				}
			}
		}
	}
	return best_dist_sq;
}

void MapEditorController::connectPathsClicked()
{
	std::vector<Object*> objects;
	std::vector<Object*> undo_objects;
	std::vector<Object*> deleted_objects;
	
	// Collect all objects in question
	objects.reserve(map->getNumSelectedObjects());
	undo_objects.reserve(map->getNumSelectedObjects());
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getSymbol()->getContainedTypes() & Symbol::Line && (*it)->getType() == Object::Path)
		{
			(*it)->update();
			objects.push_back(*it);
			undo_objects.push_back(NULL);
		}
	}
	
	AddObjectsUndoStep* add_step = NULL;
	MapPart* part = map->getCurrentPart();
	
	while (true)
	{
		// Find the closest pair of open ends of objects with the same symbol,
		// which is closer than a threshold
		PathObject* best_object_a = NULL;
		PathObject* best_object_b = NULL;
		int best_object_a_index = 0;
		int best_object_b_index = 0;
		auto best_part_a = PathPartVector::size_type { 0 };
		auto best_part_b = PathPartVector::size_type { 0 };
		bool best_part_a_begin = false;
		bool best_part_b_begin = false;
		float best_dist_sq = std::numeric_limits<float>::max();
		
		for (int i = 0; i < (int)objects.size(); ++i)
		{
			PathObject* a = reinterpret_cast<PathObject*>(objects[i]);
			
			// Choose connection threshold as maximum of 0.35mm, 1.5 * largest line extent, and 6 pixels
			// TODO: instead of 6 pixels, use a physical size as soon as screen dpi is in the settings
			auto close_distance_sq = qMax(0.35, 1.5 * a->getSymbol()->calculateLargestLineExtent(map));
			close_distance_sq = qMax(close_distance_sq, 0.001 * main_view->pixelToLength(6));
			close_distance_sq = qPow(close_distance_sq, 2);
			
			auto num_parts = a->parts().size();
			for (PathPartVector::size_type path_part_a = 0; path_part_a < num_parts; ++path_part_a)
			{
				PathPart& part = a->parts()[path_part_a];
				if (!part.isClosed())
				{
					PathObject* b;
					int b_index;
					int path_part_b;
					bool path_part_b_begin;
					float distance_sq;
					
					for (int begin = 0; begin < 2; ++begin)
					{
						bool path_part_a_begin = (begin == 0);
						distance_sq = connectPaths_FindClosestEnd(objects, a, i, path_part_a, path_part_a_begin, &b, &b_index, &path_part_b, &path_part_b_begin);
						if (distance_sq <= close_distance_sq && distance_sq < best_dist_sq)
						{
							best_dist_sq = distance_sq;
							best_object_a = a;
							best_object_b = b;
							best_object_a_index = i;
							best_object_b_index = b_index;
							best_part_a = path_part_a;
							best_part_b = path_part_b;
							best_part_a_begin = path_part_a_begin;
							best_part_b_begin = path_part_b_begin;
						}
					}
				}
			}
		}
		
		// Abort if no possible connections found
		if (best_dist_sq == std::numeric_limits<float>::max())
			break;
		
		// Create undo objects for a and b
		if (!undo_objects[best_object_a_index])
			undo_objects[best_object_a_index] = best_object_a->duplicate();
		if (!undo_objects[best_object_b_index])
			undo_objects[best_object_b_index] = best_object_b->duplicate();
		
		// Connect the best parts
		if (best_part_a_begin && best_part_b_begin)
		{
			best_object_b->parts()[best_part_b].reverse();
			best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, true);
		}
		else if (best_part_a_begin && !best_part_b_begin)
		{
			if (best_object_a == best_object_b && best_part_a == best_part_b)
				best_object_a->parts()[best_part_a].connectEnds();
			else
				best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, true);
		}
		else if (!best_part_a_begin && best_part_b_begin)
		{
			if (best_object_a == best_object_b && best_part_a == best_part_b)
				best_object_a->parts()[best_part_a].connectEnds();
			else
				best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, false);
		}
		else //if (!best_part_a_begin && !best_part_b_begin)
		{
			best_object_b->parts()[best_part_b].reverse();
			best_object_a->connectPathParts(best_part_a, best_object_b, best_part_b, false);
		}
		
		if (best_object_a != best_object_b)
		{
			// Copy all remaining parts of object b over to a
			best_object_a->getCoordinate(best_object_a->getCoordinateCount() - 1).setHolePoint(true);
			for (auto i = PathPartVector::size_type { 0 }; i < best_object_b->parts().size(); ++i)
			{
				if (i != best_part_b)
					best_object_a->appendPathPart(best_object_b->parts()[i]);
			}
			
			// Create an add step for b
			int b_index_in_part = part->findObjectIndex(best_object_b);
			deleted_objects.push_back(best_object_b);
			if (!add_step)
				add_step = new AddObjectsUndoStep(map);
			add_step->addObject(b_index_in_part, undo_objects[best_object_b_index]);
			undo_objects[best_object_b_index] = NULL;
			
			// Delete b from the active list
			objects.erase(objects.begin() + best_object_b_index);
			undo_objects.erase(undo_objects.begin() + best_object_b_index);
		}
		else
		{
			// Delete the part which has been merged with another part
			if (best_part_a != best_part_b)
				best_object_a->deletePart(best_part_b);
		}
	}
	
	// Create undo step?
	ReplaceObjectsUndoStep* replace_step = NULL;
	for (int i = 0; i < (int)undo_objects.size(); ++i)
	{
		if (undo_objects[i])
		{
			// The object was changed, update the new version
			objects[i]->forceUpdate(); /// @todo get rid of force if possible
			
			// Add the old version to the undo step
			if (!replace_step)
				replace_step = new ReplaceObjectsUndoStep(map);
			replace_step->addObject(part->findObjectIndex(objects[i]), undo_objects[i]);
		}
	}
	
	if (add_step)
	{
		for (int i = 0; i < (int)deleted_objects.size(); ++i)
		{
			map->removeObjectFromSelection(deleted_objects[i], false);
			map->getCurrentPart()->deleteObject(deleted_objects[i], false);
		}
	}
	
	if (add_step || replace_step)
	{
		CombinedUndoStep* undo_step = new CombinedUndoStep(map);
		if (replace_step)
			undo_step->push(replace_step);
		if (add_step)
			undo_step->push(add_step);
		map->push(undo_step);
		map->setObjectsDirty();
	}
	
	map->emitSelectionChanged();
	map->emitSelectionEdited();
}
void MapEditorController::cutClicked()
{
	setTool(new CutTool(this, cut_tool_act));
}
void MapEditorController::cutHoleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_act, CutHoleTool::Path));
}

void MapEditorController::cutHoleCircleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_circle_act, CutHoleTool::Circle));
}

void MapEditorController::cutHoleRectangleClicked()
{
	setTool(new CutHoleTool(this, cut_hole_rectangle_act, CutHoleTool::Rect));
}

void MapEditorController::rotateClicked()
{
	setTool(new RotateTool(this, rotate_act));
}

void MapEditorController::rotatePatternClicked()
{
	setTool(new RotatePatternTool(this, rotate_pattern_act));
}

void MapEditorController::scaleClicked()
{
	setTool(new ScaleTool(this, scale_act));
}

void MapEditorController::measureClicked(bool checked)
{
	if (!measure_dock_widget)
	{
		measure_dock_widget = new EditorDockWidget(tr("Measure"), measure_act, this, window);
		measure_dock_widget->toggleViewAction()->setVisible(false);
		MeasureWidget* measure_widget = new MeasureWidget(map);
		measure_dock_widget->setWidget(measure_widget);
		measure_dock_widget->setObjectName(QString::fromLatin1("Measure dock widget"));
		addFloatingDockWidget(measure_dock_widget);
	}
	
	measure_dock_widget->setVisible(checked);
}

void MapEditorController::booleanUnionClicked()
{
	if (!BooleanTool(BooleanTool::Union, map).executePerSymbol())
		QMessageBox::warning(window, tr("Error"), tr("Unification failed."));
}

void MapEditorController::booleanIntersectionClicked()
{
	if (!BooleanTool(BooleanTool::Intersection, map).execute())
		QMessageBox::warning(window, tr("Error"), tr("Intersection failed."));
}

void MapEditorController::booleanDifferenceClicked()
{
	if (!BooleanTool(BooleanTool::Difference, map).execute())
		QMessageBox::warning(window, tr("Error"), tr("Difference failed."));
}

void MapEditorController::booleanXOrClicked()
{
	if (!BooleanTool(BooleanTool::XOr, map).execute())
		QMessageBox::warning(window, tr("Error"), tr("XOr failed."));
}

void MapEditorController::booleanMergeHolesClicked()
{
	if (map->getNumSelectedObjects() != 1)
		return;
	
	if (!BooleanTool(BooleanTool::MergeHoles, map).execute())
		QMessageBox::warning(window, tr("Error"), tr("Merging holes failed."));
}

void MapEditorController::convertToCurvesClicked()
{
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getType() != Object::Path)
			continue;
		
		PathObject* path = (*it)->asPath();
		PathObject* undo_duplicate = NULL;
		if (path->convertToCurves(&undo_duplicate))
		{
			undo_step->addObject(part->findObjectIndex(path), undo_duplicate);
			// TODO: make threshold configurable?
			const auto threshold = 0.08;
			path->simplify(NULL, threshold);
		}
		path->update();
	}
	
	if (undo_step->isEmpty())
		delete undo_step;
	else
	{
		map->setObjectsDirty();
		map->push(undo_step);
		map->emitSelectionEdited();
	}
}

void MapEditorController::simplifyPathClicked()
{
	// TODO: make threshold configurable!
	const auto threshold = 0.1;
	
	ReplaceObjectsUndoStep* undo_step = new ReplaceObjectsUndoStep(map);
	MapPart* part = map->getCurrentPart();
	
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getType() != Object::Path)
			continue;
		
		PathObject* path = (*it)->asPath();
		PathObject* undo_duplicate = NULL;
		if (path->simplify(&undo_duplicate, threshold))
			undo_step->addObject(part->findObjectIndex(path), undo_duplicate);
		path->update();
	}
	
	if (undo_step->isEmpty())
		delete undo_step;
	else
	{
		map->setObjectsDirty();
		map->push(undo_step);
		map->emitSelectionEdited();
	}
}

void MapEditorController::cutoutPhysicalClicked()
{
	setTool(new CutoutTool(this, cutout_physical_act, false));
}

void MapEditorController::cutawayPhysicalClicked()
{
	setTool(new CutoutTool(this, cutaway_physical_act, true));
}

void MapEditorController::distributePointsClicked()
{
	Q_ASSERT(activeSymbol()->getType() == Symbol::Point);
	PointSymbol* point = activeSymbol()->asPoint();
	
	DistributePointsTool::Settings settings;
	if (!DistributePointsTool::showSettingsDialog(window, point, settings))
		return;
	
	// Create points along paths
	std::vector<PointObject*> created_objects;
	Map::ObjectSelection::const_iterator it_end = map->selectedObjectsEnd();
	for (Map::ObjectSelection::const_iterator it = map->selectedObjectsBegin(); it != it_end; ++it)
	{
		if ((*it)->getType() != Object::Path)
			continue;
		
		DistributePointsTool::execute((*it)->asPath(), point, settings, created_objects);
	}
	if (created_objects.empty())
		return;
	
	// Add points to map
	for (size_t i = 0; i < created_objects.size(); ++i)
		map->addObject(created_objects[i]);
	
	// Create undo step and select new objects
	map->clearObjectSelection(false);
	MapPart* part = map->getCurrentPart();
	DeleteObjectsUndoStep* delete_step = new DeleteObjectsUndoStep(map);
	for (size_t i = 0; i < created_objects.size(); ++i)
	{
		Object* object = created_objects[i];
		delete_step->addObject(part->findObjectIndex(object));
		map->addObjectToSelection(object, i == created_objects.size() - 1);
	}
	map->push(delete_step);
	map->setObjectsDirty();
}

void MapEditorController::addFloatingDockWidget(QDockWidget* dock_widget)
{
	if (!window->restoreDockWidget(dock_widget))
	{
		dock_widget->setFloating(true);
		// We must set the size from the sizeHint() ourselves,
		// otherwise QDockWidget may use the minimum size.
		QRect geometry(window->pos(), dock_widget->sizeHint());
		geometry.translate(window->width() - geometry.width(), window->centralWidget()->y());
		const int max_height = window->centralWidget()->height();
		if (geometry.height() > max_height)
			geometry.setHeight(max_height);
		dock_widget->setGeometry(geometry);
		connect(dock_widget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), SLOT(saveWindowState()));
	}
}

void MapEditorController::paintOnTemplateClicked(bool checked)
{
	if (checked)
	{
		if (!last_painted_on_template)
			paintOnTemplateSelectClicked();
		else
			paintOnTemplate(last_painted_on_template);
	}
	else
		setTool(NULL);
}

void MapEditorController::paintOnTemplateSelectClicked()
{
	Template* only_paintable_template = NULL;
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		if (map->getTemplate(i)->canBeDrawnOnto())
		{
			if (!only_paintable_template)
				only_paintable_template = map->getTemplate(i);
			else
			{
				only_paintable_template = NULL;
				break;
			}
		}
	}
	
	if (only_paintable_template)
		last_painted_on_template = only_paintable_template;
	else
	{
		PaintOnTemplateSelectDialog paintDialog(map, window);
		paintDialog.setWindowModality(Qt::WindowModal);
		if (paintDialog.exec() == QDialog::Rejected)
		{
			paint_on_template_act->setChecked(false);
			return;
		}
		
		last_painted_on_template = paintDialog.getSelectedTemplate();
	}
	paintOnTemplate(last_painted_on_template);
}

void MapEditorController::enableGPSDisplay(bool enable)
{
	if (enable)
	{
		gps_display->startUpdates();
		
		// Create gps_track_recorder if we can determine a template track filename
		const float gps_track_draw_update_interval = 10 * 1000; // in milliseconds
		if (! window->currentPath().isEmpty())
		{
			// Find or create a template for the track with a specific name
			QString gpx_file_path =
				QFileInfo(window->currentPath()).absoluteDir().canonicalPath()
				+ QLatin1Char('/')
				+ QFileInfo(window->currentPath()).completeBaseName()
				+ QLatin1String(" - GPS-")
				+ QDate::currentDate().toString(Qt::ISODate)
				+ QLatin1String(".gpx");
			
			int template_index = -1;
			for (int i = 0; i < map->getNumTemplates(); ++ i)
			{
				if (map->getTemplate(i)->getTemplatePath().compare(gpx_file_path) == 0
				    && qstrcmp(map->getTemplate(i)->getTemplateType(), "TemplateTrack") == 0)
				{
					template_index = i;
					if (map->getTemplate(i)->getTemplateState() != Template::Loaded)
					{
						// If the template file could not be loaded, don't care
						// at this point; simply create a new file.
						TemplateTrack* track = static_cast<TemplateTrack*>(map->getTemplate(i));
						track->configureForGPSTrack();
						track->setHasUnsavedChanges(true);
					}
					break;
				}
			}
			
			if (template_index == -1)
			{
				// Create a new template
				TemplateTrack* new_template = new TemplateTrack(gpx_file_path, map);
				new_template->configureForGPSTrack();
				template_index = map->getNumTemplates();
				map->addTemplate(new_template, template_index);
				map->setTemplateAreaDirty(template_index);
				map->setTemplatesDirty();
			}
			
			gps_track_recorder = new GPSTrackRecorder(gps_display, static_cast<TemplateTrack*>(map->getTemplate(template_index)), gps_track_draw_update_interval, map_widget);
		}
	}
	else
	{
		gps_display->stopUpdates();
		
		delete gps_track_recorder;
		gps_track_recorder = NULL;
	}
	gps_display->setVisible(enable);
	
	if (! enable)
	{
		gps_marker_display->stopPath();
		gps_temporary_path_act->setChecked(false);
	}
	
	gps_distance_rings_action->setEnabled(enable);
	gps_temporary_point_act->setEnabled(enable);
	gps_temporary_path_act->setEnabled(enable);
	updateDrawPointGPSAvailability();
}

void MapEditorController::enableGPSDistanceRings(bool enable)
{
	gps_display->enableDistanceRings(enable);
}

void MapEditorController::updateDrawPointGPSAvailability()
{
	const Symbol* symbol = activeSymbol();
	draw_point_gps_act->setEnabled(symbol && gps_display->isVisible() && symbol->getType() == Symbol::Point && !symbol->isHidden());
}

void MapEditorController::drawPointGPSClicked()
{
	setTool(new DrawPointGPSTool(gps_display, this, draw_point_gps_act));
}

void MapEditorController::gpsTemporaryPointClicked()
{
	if (gps_marker_display->addPoint())
		gps_temporary_clear_act->setEnabled(true);
}

void MapEditorController::gpsTemporaryPathClicked(bool enable)
{
	if (enable)
	{
		gps_marker_display->startPath();
		gps_temporary_clear_act->setEnabled(true);
	}
	else
		gps_marker_display->stopPath();
}

void MapEditorController::gpsTemporaryClearClicked()
{
	if (QMessageBox::question(window, tr("Clear temporary markers"), tr("Are you sure you want to delete all temporary GPS markers? This cannot be undone."), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		return;
	
	gps_marker_display->stopPath();
	gps_temporary_path_act->setChecked(false);
	gps_marker_display->clear();
	gps_temporary_clear_act->setEnabled(false);
}

void MapEditorController::enableCompassDisplay(bool enable)
{
	if (!compass_display || !show_top_bar_button || !top_action_bar)
	{
		Q_ASSERT(!"Prerequisites must be initialized");
	}
	else if (enable)
	{
		auto size = compass_display->sizeHint();
		if (top_action_bar->isVisible())
			compass_display->setGeometry(0, top_action_bar->size().height(), size.width(), size.height());
		else
			compass_display->setGeometry(show_top_bar_button->size().width(), 0, size.width(), size.height());
		connect(&Compass::getInstance(), &Compass::azimuthChanged, compass_display, &CompassDisplay::setAzimuth);
		compass_display->show();
	}
	else
	{
		disconnect(&Compass::getInstance(), &Compass::azimuthChanged, compass_display, &CompassDisplay::setAzimuth);
		compass_display->hide();
	}
}

void MapEditorController::alignMapWithNorth(bool enable)
{
	const int update_interval = 1000; // milliseconds
	
	if (enable)
	{
		Compass::getInstance().startUsage();
		gps_display->enableHeadingIndicator(true);
		
		connect(&align_map_with_north_timer, SIGNAL(timeout()), this, SLOT(alignMapWithNorthUpdate()));
		align_map_with_north_timer.start(update_interval);
		alignMapWithNorthUpdate();
	}
	else
	{
		align_map_with_north_timer.disconnect();
		align_map_with_north_timer.stop();
		
		gps_display->enableHeadingIndicator(false);
		Compass::getInstance().stopUsage();
		
		main_view->setRotation(0);
	}
}

void MapEditorController::alignMapWithNorthUpdate()
{
	// Time in milliseconds for which the rotation should not be updated after
	// the user interacted with the map widget
	const int interaction_time_threshold = 1500;
	if (map_widget->getTimeSinceLastInteraction() < interaction_time_threshold)
		return;
	
	// Set map rotation
	main_view->setRotation(M_PI / -180.0 * Compass::getInstance().getCurrentAzimuth());
}

void MapEditorController::hideTopActionBar()
{
	top_action_bar->hide();
	show_top_bar_button->show();
	show_top_bar_button->raise();
	
	compass_display->move(show_top_bar_button->size().width(), 0);
}

void MapEditorController::showTopActionBar()
{
	show_top_bar_button->hide();
	top_action_bar->show();
	top_action_bar->raise();
	
	compass_display->move(0, top_action_bar->size().height());
}

void MapEditorController::mobileSymbolSelectorClicked()
{
	// NOTE: this does not handle window resizes, however it is assumed that in
	// mobile mode there is no window, instead the application is running fullscreen.
	symbol_widget->setGeometry(window->rect());
	symbol_widget->raise();
	symbol_widget->show();
	connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(mobileSymbolSelectorFinished()));
}

void MapEditorController::mobileSymbolSelectorFinished()
{
	disconnect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(mobileSymbolSelectorFinished()));
	symbol_widget->hide();
}

void MapEditorController::updateMapPartsUI()
{
	if (!mappart_selector_box)
	{
		mappart_selector_box = new QComboBox();
		mappart_selector_box->setToolTip(tr("Map parts"));
	}
	
	if (!mappart_merge_menu)
	{
		mappart_merge_menu = new QMenu();
		mappart_merge_menu->setTitle(tr("Merge this part with"));
	}
	
	if (!mappart_move_menu)
	{
		mappart_move_menu = new QMenu();
		mappart_move_menu->setTitle(tr("Move selected objects to"));
	}
	
	const QSignalBlocker selector_box_blocker(mappart_selector_box);
	mappart_selector_box->clear();
	mappart_merge_menu->clear();
	mappart_move_menu->clear();
	
	const int count = map ? map->getNumParts() : 0;
	const bool have_multiple_parts = (count > 1);
	mappart_merge_menu->setEnabled(have_multiple_parts);
	mappart_move_menu->setEnabled(have_multiple_parts && map->getNumSelectedObjects() > 0);
	if (mappart_remove_act)
	{
		mappart_remove_act->setEnabled(have_multiple_parts);
		mappart_merge_act->setEnabled(have_multiple_parts);
	}
	if (toolbar_mapparts && !toolbar_mapparts->isVisible())
	{
		toolbar_mapparts->setVisible(have_multiple_parts);
	}
	
	if (count > 0)
	{
		const int current = map->getCurrentPartIndex();
		for (int i = 0; i < count; ++i)
		{
			QString part_name = map->getPart(i)->getName();
			mappart_selector_box->addItem(part_name);
			
			if (i != current)
			{
				QAction* action = new QAction(part_name, this);
				mappart_merge_mapper->setMapping(action, i);
				connect(action, SIGNAL(triggered()), mappart_merge_mapper, SLOT(map()));
				mappart_merge_menu->addAction(action);
				
				action = new QAction(part_name, this);
				mappart_move_mapper->setMapping(action, i);
				connect(action, SIGNAL(triggered()), mappart_move_mapper, SLOT(map()));
				mappart_move_menu->addAction(action);
			}
		}
		
		mappart_selector_box->setCurrentIndex(current);
	}
}

void MapEditorController::addMapPart()
{
	bool accepted = false;
	QString name = QInputDialog::getText(
	                   window,
	                   tr("Add new part..."),
	                   tr("Enter the name of the map part:"),
	                   QLineEdit::Normal,
	                   QString(),
	                   &accepted );
	if (accepted && !name.isEmpty())
	{
		MapPart* part = new MapPart(name, map);
		map->addPart(part, map->getCurrentPartIndex() + 1);
		map->setCurrentPart(part);
		map->push(new MapPartUndoStep(map, MapPartUndoStep::RemoveMapPart, part));
	}
}

void MapEditorController::removeMapPart()
{
	auto part = map->getCurrentPart();
	
	QMessageBox::StandardButton button =
	        QMessageBox::question(
	            window,
                tr("Remove current part"),
                tr("Do you want to remove map part \"%1\" and all its objects?").arg(part->getName()),
                QMessageBox::Yes | QMessageBox::No );
	
	if (button == QMessageBox::Yes)
	{
		auto index = map->getCurrentPartIndex();
		UndoStep* undo_step = new MapPartUndoStep(map, MapPartUndoStep::AddMapPart, index);
		
		auto i = part->getNumObjects();
		if (i > 0)
		{
			auto add_step = new AddObjectsUndoStep(map);
			do
			{
				--i;
				auto object = part->getObject(i);
				add_step->addObject(i, object);
				part->deleteObject(object, true);
			}
			while (i > 0);
			
			auto combined_step = new CombinedUndoStep(map);
			combined_step->push(add_step);
			combined_step->push(undo_step);
			undo_step = combined_step;
		}
		
		map->push(undo_step);
		map->removePart(index);
	}
}

void MapEditorController::renameMapPart()
{
	bool accepted = false;
	QString name =
	        QInputDialog::getText(
	            window,
	            tr("Rename current part..."),
	            tr("Enter the name of the map part:"),
	            QLineEdit::Normal,
	            map->getCurrentPart()->getName(),
	            &accepted );
	if (accepted && !name.isEmpty())
	{
		map->push(new MapPartUndoStep(map, MapPartUndoStep::ModifyMapPart, map->getCurrentPartIndex()));
		map->getCurrentPart()->setName(name);
	}
}

void MapEditorController::changeMapPart(int index)
{
	if (index >= 0)
	{
		map->setCurrentPartIndex(std::size_t(index));
		window->showStatusBarMessage(tr("Switched to map part '%1'.").arg(map->getCurrentPart()->getName()), 1000);
	}
}

void MapEditorController::reassignObjectsToMapPart(int target)
{
	auto current = map->getCurrentPartIndex();
	auto begin   = map->reassignObjectsToMapPart(map->selectedObjectsBegin(), map->selectedObjectsEnd(), current, target);
	
	SwitchPartUndoStep* undo = new SwitchPartUndoStep(map, target, current);
	for (std::size_t i = begin, end = map->getPart(target)->getNumObjects(); i < end; ++i)
		undo->addObject(i);
	map->push(undo);
}

void MapEditorController::mergeCurrentMapPartTo(int target)
{
	MapPart* const source_part = map->getCurrentPart();
	MapPart* const target_part = map->getPart(target);
	const QMessageBox::StandardButton button =
	        QMessageBox::question(
	            window,
                tr("Merge map parts"),
                tr("Do you want to move all objects from map part \"%1\" to \"%2\", and to remove \"%1\"?").arg(source_part->getName()).arg(target_part->getName()),
                QMessageBox::Yes | QMessageBox::No );
	
	if (button == QMessageBox::Yes)
	{
		// Beware that the source part is removed, and
		// the target part's index might change during merge.
		auto source = map->getCurrentPartIndex();
		UndoStep* add_part_step = new MapPartUndoStep(map, MapPartUndoStep::AddMapPart, source);
		
		auto begin  = map->mergeParts(source, target);
		
		SwitchPartUndoStep* switch_part_undo = new SwitchPartUndoStep(map, target, source);
		for (std::size_t i = begin, end = target_part->getNumObjects(); i < end; ++i)
			switch_part_undo->addObject(i);
		
		CombinedUndoStep* undo = new CombinedUndoStep(map);
		undo->push(switch_part_undo);
		undo->push(add_part_step);
		map->push(undo);
	}
}

void MapEditorController::mergeAllMapParts()
{
	QString const name = map->getCurrentPart()->getName();
	const QMessageBox::StandardButton button =
	        QMessageBox::question(
	            window,
                tr("Merge map parts"),
                tr("Do you want to move all objects to map part \"%1\", and to remove all other map parts?").arg(name),
                QMessageBox::Yes | QMessageBox::No );
	
	if (button == QMessageBox::Yes)
	{
		CombinedUndoStep* undo = new CombinedUndoStep(map);
		
		// For simplicity, we merge to the first part,
		// but keep the properties (i.e. name) of the current part.
		map->setCurrentPartIndex(0);
		MapPart* target_part = map->getPart(0);
		
		for (auto i = map->getNumParts() - 1; i > 0; --i)
		{
			UndoStep* add_part_step = new MapPartUndoStep(map, MapPartUndoStep::AddMapPart, i);
			auto begin = map->mergeParts(i, 0);
			SwitchPartUndoStep* switch_part_undo = new SwitchPartUndoStep(map, 0, i);
			for (std::size_t j = begin, end = target_part->getNumObjects(); j < end; ++j)
				switch_part_undo->addObject(j);
			undo->push(switch_part_undo);
			undo->push(add_part_step);
		}
		
		undo->push(new MapPartUndoStep(map, MapPartUndoStep::ModifyMapPart, 0));
		target_part->setName(name);
		
		map->push(undo);
	}
}


void MapEditorController::paintOnTemplate(Template* temp)
{
	setTool(new PaintOnTemplateTool(this, paint_on_template_act, temp));
	paint_on_template_act->setChecked(true);
}

void MapEditorController::updatePaintOnTemplateAction()
{
	if (map)
	{
		int i;
		for (i = 0; i < map->getNumTemplates(); ++i)
		{
			// TODO: check for visibility too?!
			if (map->getTemplate(i)->canBeDrawnOnto() && map->getTemplate(i)->getTemplateState() != Template::Invalid)
				break;
		}
		paint_on_template_act->setEnabled(i != map->getNumTemplates());
	}
	else
		paint_on_template_act->setEnabled(false);
	
	if (paint_on_template_act->isEnabled())
		paint_on_template_act->setStatusTip(tr("Paint free-handedly on a template"));
	else
		paint_on_template_act->setStatusTip(tr("Paint free-handedly on a template. Create or load a template which can be drawn onto to activate this button"));

	paint_on_template_settings_act->setEnabled(paint_on_template_act->isEnabled());
}

void MapEditorController::templateAdded(int pos, const Template* temp)
{
	Q_UNUSED(pos);
	if (mode == MapEditor && temp->canBeDrawnOnto())
		updatePaintOnTemplateAction();
	if (map->getNumTemplates() == 1)
		templateAvailabilityChanged();
}

void MapEditorController::templateDeleted(int pos, const Template* temp)
{
	Q_UNUSED(pos);
	if (mode == MapEditor && temp->canBeDrawnOnto())
		updatePaintOnTemplateAction();
	if (map->getNumTemplates() == 0)
		templateAvailabilityChanged();
}

void MapEditorController::setMapAndView(Map* map, MapView* map_view)
{
	Q_ASSERT(map);
	Q_ASSERT(map_view);
	
	if (this->map)
	{
		this->map->disconnect(this);
		if (symbol_widget)
		{
			this->map->disconnect(symbol_widget);
		}
		updateMapPartsUI();
	}
	
	this->map = map;
	this->main_view = map_view;
	
	connect(&map->undoManager(), SIGNAL(canRedoChanged(bool)), this, SLOT(undoStepAvailabilityChanged()));
	connect(&map->undoManager(), SIGNAL(canUndoChanged(bool)), this, SLOT(undoStepAvailabilityChanged()));
	connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(objectSelectionChanged()));
	connect(map, SIGNAL(templateAdded(int, const Template*)), this, SLOT(templateAdded(int, const Template*)));
	connect(map, SIGNAL(templateDeleted(int, const Template*)), this, SLOT(templateDeleted(int, const Template*)));
	connect(map, SIGNAL(closedTemplateAvailabilityChanged()), this, SLOT(closedTemplateAvailabilityChanged()));
	connect(map, SIGNAL(spotColorPresenceChanged(bool)), this, SLOT(spotColorPresenceChanged(bool)));
	connect(map, SIGNAL(currentMapPartChanged(const MapPart*)), this, SLOT(updateMapPartsUI()));
	connect(map, SIGNAL(mapPartAdded(std::size_t,const MapPart*)), this, SLOT(updateMapPartsUI()));
	connect(map, SIGNAL(mapPartChanged(std::size_t,const MapPart*)), this, SLOT(updateMapPartsUI()));
	connect(map, SIGNAL(mapPartDeleted(std::size_t,const MapPart*)), this, SLOT(updateMapPartsUI()));
	
	if (symbol_widget)
	{
		delete symbol_widget;
		symbol_widget = NULL;
		createSymbolWidget();
	}
	
	if (window)
	{
		updateWidgets();
	}
}

void MapEditorController::updateWidgets()
{
	undoStepAvailabilityChanged();
	objectSelectionChanged();
	if (mode == MapEditor)
	{
		if (main_view)
		{
			show_grid_act->setChecked(main_view->isGridVisible());
			hide_all_templates_act->setChecked(main_view->areAllTemplatesHidden());
			overprinting_simulation_act->setChecked(main_view->isOverprintingSimulationEnabled());
		}
		if (map)
		{
			hatch_areas_view_act->setChecked(map->isAreaHatchingEnabled());
			baseline_view_act->setChecked(map->isBaselineViewEnabled());
			closedTemplateAvailabilityChanged();
			spotColorPresenceChanged(map->hasSpotColors());
			updateMapPartsUI();
		}
	}
}

void MapEditorController::createSymbolWidget(QWidget* parent)
{
	if (!symbol_widget)
	{
		symbol_widget = new SymbolWidget(map, mobile_mode, parent);
		connect(symbol_widget, SIGNAL(switchSymbolClicked()), this, SLOT(switchSymbolClicked()));
		connect(symbol_widget, SIGNAL(fillBorderClicked()), this, SLOT(fillBorderClicked()));
		connect(symbol_widget, SIGNAL(selectObjectsClicked(bool)), this, SLOT(selectObjectsClicked(bool)));
		connect(symbol_widget, SIGNAL(deselectObjectsClicked()), this, SLOT(deselectObjectsClicked()));
		connect(symbol_widget, SIGNAL(selectedSymbolsChanged()), this, SLOT(selectedSymbolsChanged()));
		if (symbol_dock_widget)
		{
			Q_ASSERT(!parent);
			symbol_dock_widget->setWidget(symbol_widget);
		}
		
		selectedSymbolsChanged();
	}
}

void MapEditorController::importClicked()
{
	QSettings settings;
	QString import_directory = settings.value(QString::fromLatin1("importFileDirectory"), QDir::homePath()).toString();
	
	QStringList map_names;
	QStringList map_extensions;
	for (auto format : FileFormats.formats())
	{
		if (!format->supportsImport())
			continue;
		
		map_names.push_back(format->primaryExtension().toUpper());
		map_extensions.append(format->fileExtensions());
	}
	map_names.removeDuplicates();
	map_extensions.removeDuplicates();
	
	QString filename = QFileDialog::getOpenFileName(
		window,
		tr("Import %1, GPX, OSM or DXF file").arg(
	      map_names.join(QString::fromLatin1(", "))),
		import_directory,
		QString::fromLatin1("%1 (%2 *.gpx *.osm *.dxf);;%3 (*.*)").arg(
	      tr("Importable files"), QLatin1String("*.") + map_extensions.join(QString::fromLatin1(" *.")), tr("All files")) );
	if (filename.isEmpty() || filename.isNull())
		return;
	
	settings.setValue(QString::fromLatin1("importFileDirectory"), QFileInfo(filename).canonicalPath());
	
	bool success = false;
	auto map_format = FileFormats.findFormatForFilename(filename);
	if (map_format)
	{
		// Map format recognized by filename extension
		importMapFile(filename, true); // Error reporting in Map::loadFrom()
		return;
	}
	else if (filename.endsWith(QLatin1String(".dxf"), Qt::CaseInsensitive)
	         || filename.endsWith(QLatin1String(".gpx"), Qt::CaseInsensitive)
	         || filename.endsWith(QLatin1String(".osm"), Qt::CaseInsensitive))
	{
		// Fallback: Legacy geo file import
		importGeoFile(filename);
		return; // Error reporting in Track::import()
	}
	else if (importMapFile(filename, false))
	{
		// Last resort: Map format recognition by try-and-error
		success = true;
	}
	else
	{
		QMessageBox::critical(window, tr("Error"), tr("Cannot import the selected file because its file format is not supported."));
		return;
	}
	
	if (!success)
	{
		/// \todo Reword message (not a map file here). Requires new translations
		QMessageBox::critical(window, tr("Error"), tr("Cannot import the selected map file because it could not be loaded."));
	}
}

bool MapEditorController::importGeoFile(const QString& filename)
{
	TemplateTrack temp(filename, map);
	return !temp.configureAndLoad(window, main_view)
	       || temp.import(window);
}

bool MapEditorController::importMapFile(const QString& filename, bool show_errors)
{
	Map imported_map;
	imported_map.setScaleDenominator(map->getScaleDenominator()); // for non-scaled geodata
	
	bool result = imported_map.loadFrom(filename, window, nullptr, false, show_errors);
	if (result)
	{
		map->setProperty("import-georeferencing", QVariant{true});
		map->importMap(&imported_map, Map::MinimalObjectImport, window);
	}
	
	return result;
}

// slot
void MapEditorController::setViewOptionsEnabled(bool enabled)
{
	pan_act->setEnabled(enabled);
	show_grid_act->setEnabled(enabled);
// 	hatch_areas_view_act->setEnabled(enabled);
// 	baseline_view_act->setEnabled(enabled);
	overprinting_simulation_act->setEnabled(enabled);
	hide_all_templates_act->setEnabled(enabled);
}


// ### EditorDockWidget ###

EditorDockWidget::EditorDockWidget(const QString& title, QAction* action, MapEditorController* editor, QWidget* parent)
: QDockWidget(title, parent)
, action(action)
, editor(editor)
{
	if (editor)
	{
		connect(this, &EditorDockWidget::dockLocationChanged, editor, &MapEditorController::saveWindowState);
	}
	
	if (action)
	{
		connect(toggleViewAction(), &QAction::toggled, action, &QAction::setChecked);
	}
	
#ifdef Q_OS_ANDROID
	size_grip = new QSizeGrip(this);
#endif
}

bool EditorDockWidget::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::ShortcutOverride:
		if (editor->getWindow()->shortcutsBlocked())
			event->accept();
		break;
		
	case QEvent::Show:
		QTimer::singleShot(0, this, SLOT(raise()));
		break;
		
	default:
		; // nothing
	}

	return QDockWidget::event(event);
}

void EditorDockWidget::resizeEvent(QResizeEvent* event)
{
#ifdef Q_OS_ANDROID
	QRect rect(QPoint(0,0), size_grip->sizeHint());
	rect.moveBottomRight(geometry().bottomRight() - geometry().topLeft());
	int fw = style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, this);
	rect.translate(-fw, -fw);
	size_grip->setGeometry(rect);
	size_grip->raise();
#endif
	
	QDockWidget::resizeEvent(event);
}



// ### MapEditorToolAction ###

MapEditorToolAction::MapEditorToolAction(const QIcon& icon, const QString& text, QObject* parent)
 : QAction(icon, text, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(triggered(bool)), this, SLOT(triggeredImpl(bool)));
}

void MapEditorToolAction::triggeredImpl(bool checked)
{
	if (checked)
		emit(activated());
	else
		setChecked(true);
}
