/*
 *    Copyright 2012 Jan Dalheimer
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

#include "settings_dialog.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include <mapper_config.h>

#include "map_editor.h"
#include "map_widget.h"
#include "main_window.h"
#include "util_gui.h"
#include "util_translation.h"
#include "settings.h"

#include "gui/widgets/home_screen_widget.h"

void SettingsPage::apply()
{
	QSettings settings;
	for (int i = 0; i < changes.size(); i++)
		settings.setValue(changes.keys().at(i), changes.values().at(i));
	changes.clear();
}

// ### SettingsDialog ###

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Settings"));
	QVBoxLayout* layout = new QVBoxLayout();
	this->setLayout(layout);
	
	tab_widget = new QTabWidget(this);
	layout->addWidget(tab_widget);
	
	button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
									   Qt::Horizontal, this);
	layout->addWidget(button_box);
	connect(button_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));
	
	// Add all pages
	addPage(new GeneralPage(this));
	addPage(new EditorPage(this));
	//addPage(new PrintingPage(this));
}

void SettingsDialog::addPage(SettingsPage* page)
{
	pages.push_back(page);
	tab_widget->addTab(page, page->title());
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	if (id == QDialogButtonBox::Ok)
	{
		foreach(SettingsPage* page, pages)
			page->ok();
		Settings::getInstance().applySettings();
		this->accept();
	}
	else if (id == QDialogButtonBox::Apply)
	{
		foreach(SettingsPage* page, pages)
			page->apply();
		Settings::getInstance().applySettings();
	}
	else if (id == QDialogButtonBox::Cancel)
	{
		foreach(SettingsPage* page, pages)
			page->cancel();
		this->reject();
	}
}

// ### EditorPage ###

EditorPage::EditorPage(QWidget* parent) : SettingsPage(parent)
{
	QGridLayout* layout = new QGridLayout();
	this->setLayout(layout);
	
	int row = 0;
	
	antialiasing = new QCheckBox(tr("High quality map display (antialiasing)"), this);
	antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addWidget(antialiasing, row++, 0, 1, 2);
	
	text_antialiasing = new QCheckBox(tr("High quality text display in map (antialiasing), slow"), this);
	text_antialiasing->setToolTip(tr("Antialiasing makes the map look much better, but also slows down the map display"));
	layout->addWidget(text_antialiasing, row++, 0, 1, 2);
	
	QLabel* tolerance_label = new QLabel(tr("Click tolerance:"));
	QSpinBox* tolerance = Util::SpinBox::create(0, 50, tr("px"));
	layout->addWidget(tolerance_label, row, 0);
	layout->addWidget(tolerance, row++, 1);
	
	QLabel* snap_distance_label = new QLabel(tr("Snap distance (Shift):"));
	QSpinBox* snap_distance = Util::SpinBox::create(0, 100, tr("px"));
	layout->addWidget(snap_distance_label, row, 0);
	layout->addWidget(snap_distance, row++, 1);
	
	QLabel* fixed_angle_stepping_label = new QLabel(tr("Stepping of fixed angle mode (Ctrl):"));
	QSpinBox* fixed_angle_stepping = Util::SpinBox::create(1, 180, trUtf8("°", "Degree sign for angles"));
	layout->addWidget(fixed_angle_stepping_label, row, 0);
	layout->addWidget(fixed_angle_stepping, row++, 1);

	QCheckBox* select_symbol_of_objects = new QCheckBox(tr("When selecting an object, automatically select its symbol, too"));
	layout->addWidget(select_symbol_of_objects, row++, 0, 1, 2);
	
	QCheckBox* zoom_out_away_from_cursor = new QCheckBox(tr("Zoom away from cursor when zooming out"));
	layout->addWidget(zoom_out_away_from_cursor, row++, 0, 1, 2);
	
	QCheckBox* draw_last_point_on_right_click = new QCheckBox(tr("Drawing tools: set last point on finishing with right click"));
	layout->addWidget(draw_last_point_on_right_click, row++, 0, 1, 2);
	
	QCheckBox* keep_settings_of_closed_templates = new QCheckBox(tr("Templates: keep settings of closed templates"));
	layout->addWidget(keep_settings_of_closed_templates, row++, 0, 1, 2);
	
	
	layout->setRowMinimumHeight(row++, 16);
	layout->addWidget(new QLabel("<b>" % tr("Edit tool:") % "</b>"), row++, 0, 1, 2);
	
	edit_tool_delete_bezier_point_action = new QComboBox();
	edit_tool_delete_bezier_point_action->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addWidget(new QLabel(tr("Action on deleting a bezier spline point with Ctrl:")), row, 0);
	layout->addWidget(edit_tool_delete_bezier_point_action, row++, 1);
	
	edit_tool_delete_bezier_point_action_alternative = new QComboBox();
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addWidget(new QLabel(tr("Action on deleting a bezier spline point with Ctrl-Shift:")), row, 0);
	layout->addWidget(edit_tool_delete_bezier_point_action_alternative, row++, 1);
	
	layout->setRowMinimumHeight(row++, 16);
	layout->addWidget(new QLabel("<b>" % tr("Rectangle tool:") % "</b>"), row++, 0, 1, 2);
	
	QLabel* rectangle_helper_cross_radius_label = new QLabel(tr("Radius of helper cross:"));
	QSpinBox* rectangle_helper_cross_radius = Util::SpinBox::create(0, 999999, tr("px"));
	layout->addWidget(rectangle_helper_cross_radius_label, row, 0);
	layout->addWidget(rectangle_helper_cross_radius, row++, 1);
	
	QCheckBox* rectangle_preview_line_width = new QCheckBox(tr("Preview the width of lines with helper cross"));
	layout->addWidget(rectangle_preview_line_width, row++, 0, 1, 2);
	
	
	antialiasing->setChecked(Settings::getInstance().getSetting(Settings::MapDisplay_Antialiasing).toBool());
	text_antialiasing->setChecked(Settings::getInstance().getSetting(Settings::MapDisplay_TextAntialiasing).toBool());
	tolerance->setValue(Settings::getInstance().getSetting(Settings::MapEditor_ClickTolerance).toInt());
	snap_distance->setValue(Settings::getInstance().getSetting(Settings::MapEditor_SnapDistance).toInt());
	fixed_angle_stepping->setValue(Settings::getInstance().getSetting(Settings::MapEditor_FixedAngleStepping).toInt());
	select_symbol_of_objects->setChecked(Settings::getInstance().getSetting(Settings::MapEditor_ChangeSymbolWhenSelecting).toBool());
	zoom_out_away_from_cursor->setChecked(Settings::getInstance().getSetting(Settings::MapEditor_ZoomOutAwayFromCursor).toBool());
	draw_last_point_on_right_click->setChecked(Settings::getInstance().getSetting(Settings::MapEditor_DrawLastPointOnRightClick).toBool());
	keep_settings_of_closed_templates->setChecked(Settings::getInstance().getSetting(Settings::Templates_KeepSettingsOfClosed).toBool());
	
	edit_tool_delete_bezier_point_action->setCurrentIndex(edit_tool_delete_bezier_point_action->findData(Settings::getInstance().getSetting(Settings::EditTool_DeleteBezierPointAction).toInt()));
	edit_tool_delete_bezier_point_action_alternative->setCurrentIndex(edit_tool_delete_bezier_point_action_alternative->findData(Settings::getInstance().getSetting(Settings::EditTool_DeleteBezierPointActionAlternative).toInt()));
	
	rectangle_helper_cross_radius->setValue(Settings::getInstance().getSetting(Settings::RectangleTool_HelperCrossRadius).toInt());
	rectangle_preview_line_width->setChecked(Settings::getInstance().getSetting(Settings::RectangleTool_PreviewLineWidth).toBool());
	
	layout->setRowStretch(row, 1);
	
	updateWidgets();

	connect(antialiasing, SIGNAL(toggled(bool)), this, SLOT(antialiasingClicked(bool)));
	connect(text_antialiasing, SIGNAL(toggled(bool)), this, SLOT(textAntialiasingClicked(bool)));
	connect(tolerance, SIGNAL(valueChanged(int)), this, SLOT(toleranceChanged(int)));
	connect(snap_distance, SIGNAL(valueChanged(int)), this, SLOT(snapDistanceChanged(int)));
	connect(fixed_angle_stepping, SIGNAL(valueChanged(int)), this, SLOT(fixedAngleSteppingChanged(int)));
	connect(select_symbol_of_objects, SIGNAL(clicked(bool)), this, SLOT(selectSymbolOfObjectsClicked(bool)));
	connect(zoom_out_away_from_cursor, SIGNAL(clicked(bool)), this, SLOT(zoomOutAwayFromCursorClicked(bool)));
	connect(draw_last_point_on_right_click, SIGNAL(clicked(bool)), this, SLOT(drawLastPointOnRightClickClicked(bool)));
	connect(keep_settings_of_closed_templates, SIGNAL(clicked(bool)), this, SLOT(keepSettingsOfClosedTemplatesClicked(bool)));
	
	connect(edit_tool_delete_bezier_point_action, SIGNAL(currentIndexChanged(int)), this, SLOT(editToolDeleteBezierPointActionChanged(int)));
	connect(edit_tool_delete_bezier_point_action_alternative, SIGNAL(currentIndexChanged(int)), this, SLOT(editToolDeleteBezierPointActionAlternativeChanged(int)));
	
	connect(rectangle_helper_cross_radius,  SIGNAL(valueChanged(int)), this, SLOT(rectangleHelperCrossRadiusChanged(int)));
	connect(rectangle_preview_line_width, SIGNAL(clicked(bool)), this, SLOT(rectanglePreviewLineWidthChanged(bool)));
}

void EditorPage::updateWidgets()
{
	text_antialiasing->setEnabled(antialiasing->isChecked());
}

void EditorPage::antialiasingClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapDisplay_Antialiasing), QVariant(checked));
	updateWidgets();
}

void EditorPage::textAntialiasingClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapDisplay_TextAntialiasing), QVariant(checked));
}

void EditorPage::toleranceChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_ClickTolerance), QVariant(value));
}

void EditorPage::snapDistanceChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_SnapDistance), QVariant(value));
}

void EditorPage::fixedAngleSteppingChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_FixedAngleStepping), QVariant(value));
}

void EditorPage::selectSymbolOfObjectsClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_ChangeSymbolWhenSelecting), QVariant(checked));
}

void EditorPage::zoomOutAwayFromCursorClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_ZoomOutAwayFromCursor), QVariant(checked));
}

void EditorPage::drawLastPointOnRightClickClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::MapEditor_DrawLastPointOnRightClick), QVariant(checked));
}

void EditorPage::keepSettingsOfClosedTemplatesClicked(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::Templates_KeepSettingsOfClosed), QVariant(checked));
}

void EditorPage::editToolDeleteBezierPointActionChanged(int index)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::EditTool_DeleteBezierPointAction), edit_tool_delete_bezier_point_action->itemData(index));
}

void EditorPage::editToolDeleteBezierPointActionAlternativeChanged(int index)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::EditTool_DeleteBezierPointActionAlternative), edit_tool_delete_bezier_point_action_alternative->itemData(index));
}

void EditorPage::rectangleHelperCrossRadiusChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::RectangleTool_HelperCrossRadius), QVariant(value));
}

void EditorPage::rectanglePreviewLineWidthChanged(bool checked)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::RectangleTool_PreviewLineWidth), QVariant(checked));
}

// ### PrintingPage ###

/*PrintingPage::PrintingPage(QWidget* parent) : SettingsPage(parent)
{
	QGridLayout* layout = new QGridLayout();
	this->setLayout(layout);
	
	int row = 0;
	
	QLabel* preview_border_label = new QLabel(tr("Preview border size per side:"));
	QDoubleSpinBox* preview_border_edit = Util::SpinBox::create(1, 0, 999999, tr("mm", "millimeters"));
	layout->addWidget(preview_border_label, row, 0);
	layout->addWidget(preview_border_edit, row++, 1);
	
	preview_border_edit->setValue(Settings::getInstance().getSetting(Settings::Printing_PreviewBorder).toDouble());
	
	layout->setRowStretch(row, 1);
	
	connect(preview_border_edit, SIGNAL(valueChanged(double)), this, SLOT(previewBorderChanged(double)));
}*/

// ### GeneralPage ###

GeneralPage::GeneralPage(QWidget* parent) : SettingsPage(parent)
{
	QGridLayout* layout = new QGridLayout();
	setLayout(layout);
	
	int row = 0;
	QLabel* language_label = new QLabel(tr("Language:"));
	language_box = new QComboBox(this);
	
	//  Add the available languages to the widget
	LanguageCollection language_map = TranslationUtil::getAvailableLanguages();
	LanguageCollection::const_iterator end = language_map.constEnd();
	for (LanguageCollection::const_iterator it = language_map.constBegin(); it != end; ++it)
		language_box->addItem(it.key(), (int)it.value());
	
	// Select current language
	int index = language_box->findData(Settings::getInstance().getSetting(Settings::General_Language).toInt());
	language_default_index = (index >= 0) ? index : language_box->findData((int)QLocale::English);
	language_box->setCurrentIndex(language_default_index);
	
	layout->addWidget(language_label, row, 0);
	layout->addWidget(language_box, row, 1);
	
	row++;
	QCheckBox* open_mru_check = new QCheckBox(HomeScreenWidget::tr("Open most recently used file on start"));
	open_mru_check->setChecked(Settings::getInstance().getSetting(Settings::General_OpenMRUFile).toBool());
	layout->addWidget(open_mru_check, row, 0, 1, 2);
	
	row++;
	QCheckBox* tips_visible_check = new QCheckBox(HomeScreenWidget::tr("Show tip of the day"));
	tips_visible_check->setChecked(Settings::getInstance().getSetting(Settings::HomeScreen_TipsVisible).toBool());
	layout->addWidget(tips_visible_check, row, 0, 1, 2);
	
	row++;
	layout->setRowStretch(row, 1);
	
	connect(language_box, SIGNAL(currentIndexChanged(int)), this, SLOT(languageChanged(int)));
	connect(open_mru_check, SIGNAL(clicked(bool)), this, SLOT(openMRUFileClicked(bool)));
	connect(tips_visible_check, SIGNAL(clicked(bool)), this, SLOT(tipsVisibleClicked(bool)));
}

void GeneralPage::apply()
{
	if (language_box->currentIndex() != language_default_index)
		QMessageBox::information(window(), tr("Notice"), tr("The program must be restarted for the language change to take effect!"));
	SettingsPage::apply();
	language_default_index = language_box->currentIndex();
}

void GeneralPage::languageChanged(int index)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_Language), language_box->itemData(index).toInt());
}

void GeneralPage::openMRUFileClicked(bool state)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_OpenMRUFile), state);
}

void GeneralPage::tipsVisibleClicked(bool state)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::HomeScreen_TipsVisible), state);
}
