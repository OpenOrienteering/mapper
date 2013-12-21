/*
 *    Copyright 2012, 2013 Jan Dalheimer, Kai Pastor
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
#include "settings_dialog_p.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "../settings.h"
#include "../util.h"
#include "../util_gui.h"
#include "../util_translation.h"
#include "../util/scoped_signals_blocker.h"
#include "modifier_key.h"
#include "widgets/home_screen_widget.h"


// ### SettingsDialog ###

SettingsDialog::SettingsDialog(QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle(tr("Settings"));
	
	QVBoxLayout* layout = new QVBoxLayout();
	this->setLayout(layout);
	
	tab_widget = new QTabWidget();
	layout->addWidget(tab_widget);
	
	button_box = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Help,
	  Qt::Horizontal );
	layout->addWidget(button_box);
	connect(button_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));
	
	// Add all pages
	addPage(new GeneralPage(this));
	addPage(new EditorPage(this));
	//addPage(new PrintingPage(this));
}

SettingsDialog::~SettingsDialog()
{
	// Nothing, not inlined.
}

void SettingsDialog::addPage(SettingsPage* page)
{
	tab_widget->addTab(page, page->title());
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	int count = tab_widget->count();
	if (id == QDialogButtonBox::Ok)
	{
		for (int i = 0; i < count; i++)
			static_cast< SettingsPage* >(tab_widget->widget(i))->ok();
		Settings::getInstance().applySettings();
		this->accept();
	}
	else if (id == QDialogButtonBox::Apply)
	{
		for (int i = 0; i < count; i++)
			static_cast< SettingsPage* >(tab_widget->widget(i))->apply();
		Settings::getInstance().applySettings();
	}
	else if (id == QDialogButtonBox::Cancel)
	{
		for (int i = 0; i < count; i++)
			static_cast< SettingsPage* >(tab_widget->widget(i))->cancel();
		this->reject();
	}
	else if (id == QDialogButtonBox::Help)
	{
		Util::showHelp(this, "settings.html");
	}
}


// ### SettingsPage ###

void SettingsPage::apply()
{
	QSettings settings;
	for (int i = 0; i < changes.size(); i++)
		settings.setValue(changes.keys().at(i), changes.values().at(i));
	changes.clear();
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
	
	QLabel* snap_distance_label = new QLabel(tr("Snap distance (%1):").arg(ModifierKey::shift()));
	QSpinBox* snap_distance = Util::SpinBox::create(0, 100, tr("px"));
	layout->addWidget(snap_distance_label, row, 0);
	layout->addWidget(snap_distance, row++, 1);
	
	QLabel* fixed_angle_stepping_label = new QLabel(tr("Stepping of fixed angle mode (%1):").arg(ModifierKey::control()));
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
	layout->addWidget(new QLabel(tr("Action on deleting a curve point with %1:").arg(ModifierKey::control())), row, 0);
	layout->addWidget(edit_tool_delete_bezier_point_action, row++, 1);
	
	edit_tool_delete_bezier_point_action_alternative = new QComboBox();
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Retain old shape"), (int)Settings::DeleteBezierPoint_RetainExistingShape);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Reset outer curve handles"), (int)Settings::DeleteBezierPoint_ResetHandles);
	edit_tool_delete_bezier_point_action_alternative->addItem(tr("Keep outer curve handles"), (int)Settings::DeleteBezierPoint_KeepHandles);
	layout->addWidget(new QLabel(tr("Action on deleting a curve point with %1:").arg(ModifierKey::controlShift())), row, 0);
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

const int GeneralPage::TranslationFromFile = -1;

GeneralPage::GeneralPage(QWidget* parent) : SettingsPage(parent)
{
	QGridLayout* layout = new QGridLayout();
	setLayout(layout);
	
	int row = 0;
	layout->addWidget(Util::Headline::create(tr("Appearance")), row, 1, 1, 2);
	
	row++;
	QLabel* language_label = new QLabel(tr("Language:"));
	layout->addWidget(language_label, row, 1);
	
	language_box = new QComboBox(this);
	updateLanguageBox();
	layout->addWidget(language_box, row, 2);
	
	QAbstractButton* language_file_button = new QToolButton();
	language_file_button->setIcon(QIcon(":/images/open.png"));
	layout->addWidget(language_file_button, row, 3);
	
	row++;
	layout->addItem(Util::SpacerItem::create(this), row, 1);
	
	row++;
	layout->addWidget(Util::Headline::create(tr("Program start")), row, 1, 1, 2);
	
	row++;
	QCheckBox* open_mru_check = new QCheckBox(HomeScreenWidget::tr("Open most recently used file"));
	open_mru_check->setChecked(Settings::getInstance().getSetting(Settings::General_OpenMRUFile).toBool());
	layout->addWidget(open_mru_check, row, 1, 1, 2);
	
	row++;
	QCheckBox* tips_visible_check = new QCheckBox(HomeScreenWidget::tr("Show tip of the day"));
	tips_visible_check->setChecked(Settings::getInstance().getSetting(Settings::HomeScreen_TipsVisible).toBool());
	layout->addWidget(tips_visible_check, row, 1, 1, 2);
	
	row++;
	layout->addItem(Util::SpacerItem::create(this), row, 1);
	
	row++;
	layout->addWidget(Util::Headline::create(tr("Saving files")), row, 1, 1, 2);
	
	// Possible point: limit size of undo/redo journal
	
	int auto_save_interval = Settings::getInstance().getSetting(Settings::General_AutoSaveInterval).toInt();
	
	row++;
	QCheckBox* auto_save_check = new QCheckBox(tr("Enable automatic saving of the current file"));
	auto_save_check->setChecked(auto_save_interval > 0);
	layout->addWidget(auto_save_check, row, 1, 1, 2);
	
	row++;
	auto_save_interval_label = new QLabel(tr("Auto-save interval:"));
	layout->addWidget(auto_save_interval_label, row, 1);
	
	auto_save_interval_edit = Util::SpinBox::create(1, 120, tr("min", "unit minutes"), 1);
	auto_save_interval_edit->setValue(qAbs(auto_save_interval));
	auto_save_interval_edit->setEnabled(auto_save_interval > 0);
	layout->addWidget(auto_save_interval_edit, row, 2);
	
	row++;
	layout->addItem(Util::SpacerItem::create(this), row, 1);
	
	row++;
	layout->addWidget(Util::Headline::create(tr("File import and export")), row, 1, 1, 2);
	
	row++;
	QLabel* encoding_label = new QLabel(tr("8-bit encoding:"));
	layout->addWidget(encoding_label, row, 1);
	
	encoding_box = new QComboBox();
	encoding_box->addItem("System");
	encoding_box->addItem("Windows-1250");
	encoding_box->addItem("Windows-1252");
	encoding_box->addItem("ISO-8859-1");
	encoding_box->addItem("ISO-8859-15");
	encoding_box->setEditable(true);
	QStringList availableCodecs;
	Q_FOREACH(QByteArray item, QTextCodec::availableCodecs())
	{
		availableCodecs.append(QString(item));
	}
	QCompleter* completer = new QCompleter(availableCodecs, this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	encoding_box->setCompleter(completer);
	encoding_box->setCurrentText(Settings::getInstance().getSetting(Settings::General_Local8BitEncoding).toString());
	layout->addWidget(encoding_box, row, 2);
	
	row++;
	QCheckBox* ocd_importer_check = new QCheckBox(tr("Use the new OCD importer also for version 8 files"));
	ocd_importer_check->setChecked(Settings::getInstance().getSetting(Settings::General_NewOcd8Implementation).toBool());
	layout->addWidget(ocd_importer_check, row, 1, 1, 2);
	
	row++;
	layout->setRowStretch(row, 1);
	
#if defined(Q_OS_MAC)
	// Center setting items
	layout->setColumnStretch(0, 2);
#endif
	layout->setColumnStretch(2, 1);
	layout->setColumnStretch(2, 2);
	layout->setColumnStretch(layout->columnCount(), 2);
	
	connect(language_box, SIGNAL(currentIndexChanged(int)), this, SLOT(languageChanged(int)));
	connect(language_file_button, SIGNAL(clicked(bool)), this, SLOT(openTranslationFileDialog()));
	connect(open_mru_check, SIGNAL(clicked(bool)), this, SLOT(openMRUFileClicked(bool)));
	connect(tips_visible_check, SIGNAL(clicked(bool)), this, SLOT(tipsVisibleClicked(bool)));
	connect(encoding_box, SIGNAL(currentTextChanged(QString)), this, SLOT(encodingChanged(QString)));
	connect(ocd_importer_check, SIGNAL(clicked(bool)), this, SLOT(ocdImporterClicked(bool)));
	connect(auto_save_check, SIGNAL(clicked(bool)), this, SLOT(autoSaveChanged(bool)));
	connect(auto_save_interval_edit, SIGNAL(valueChanged(int)), this, SLOT(autoSaveIntervalChanged(int)));
}

void GeneralPage::apply()
{
	const Settings& settings = Settings::getInstance();
	const QString translation_file_key(settings.getSettingPath(Settings::General_TranslationFile));
	const QString language_key(settings.getSettingPath(Settings::General_Language));
	
	if (changes.contains(language_key) || changes.contains(translation_file_key))
	{
		if (!changes.contains(translation_file_key))
		{
			// Set an empty file name when changing the language without setting a filename.
			changes[translation_file_key] = QString();
		}
		
		QVariant lang = changes.contains(language_key) ? changes[language_key] : settings.getSetting(Settings::General_Language);
		QVariant translation_file = changes[translation_file_key];
		if ( settings.getSetting(Settings::General_Language) != lang ||
		     settings.getSetting(Settings::General_TranslationFile) != translation_file )
		{
			// Show an message box in the new language.
			TranslationUtil translation((QLocale::Language)lang.toInt(), translation_file.toString());
			qApp->installTranslator(&translation.getQtTranslator());
			qApp->installTranslator(&translation.getAppTranslator());
			if (lang.toInt() <= 1 || lang.toInt() == 31) // cf. QLocale::Language
			{
				QMessageBox::information(window(), "Notice", "The program must be restarted for the language change to take effect!");
			}
			else
			{
				QMessageBox::information(window(), tr("Notice"), tr("The program must be restarted for the language change to take effect!"));
			}
			qApp->removeTranslator(&translation.getAppTranslator());
			qApp->removeTranslator(&translation.getQtTranslator());

#if defined(Q_OS_MAC)
			// The native [file] dialogs will use the first element of the
			// AppleLanguages array in the application's .plist file -
			// and this file is also the one used by QSettings.
			const QString mapper_language(translation.getLocale().name().left(2));
			changes["AppleLanguages"] = ( QStringList() << mapper_language );
#endif
		}
	}
	SettingsPage::apply();
}

void GeneralPage::languageChanged(int index)
{
	if (language_box->itemData(index) == TranslationFromFile)
	{
		openTranslationFileDialog();
	}
	else
	{
		changes.insert(Settings::getInstance().getSettingPath(Settings::General_Language), language_box->itemData(index).toInt());
	}
}

void GeneralPage::updateLanguageBox()
{
	const Settings& settings = Settings::getInstance();
	const QString translation_file_key(settings.getSettingPath(Settings::General_TranslationFile));
	const QString language_key(settings.getSettingPath(Settings::General_Language));
	
	LanguageCollection language_map = TranslationUtil::getAvailableLanguages();
	
	// Add the locale of the explicit translation file
	QString translation_file;
	if (changes.contains(translation_file_key))
		translation_file = changes[translation_file_key].toString();
	else 
		translation_file = settings.getSetting(Settings::General_TranslationFile).toString();
	
	QString locale_name = TranslationUtil::localeNameForFile(translation_file);
	if (!locale_name.isEmpty())
	{
		QLocale file_locale(locale_name);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
		QString language_name = file_locale.nativeLanguageName();
#else
		QString language_name = QLocale::languageToString(file_locale.language());
#endif
		if (!language_map.contains(language_name))
			language_map.insert(language_name, file_locale.language());
	}
	
	// Update the language box
	ScopedSignalsBlocker block(language_box);
	language_box->clear();
	
	LanguageCollection::const_iterator end = language_map.constEnd();
	for (LanguageCollection::const_iterator it = language_map.constBegin(); it != end; ++it)
		language_box->addItem(it.key(), (int)it.value());
	language_box->addItem(tr("Use translation file..."), TranslationFromFile);
	
	// Select current language
	int index = language_box->findData(changes.contains(language_key) ? 
	  changes[language_key].toInt() :
	  settings.getSetting(Settings::General_Language).toInt() );
	
	if (index < 0)
		index = language_box->findData((int)QLocale::English);
	language_box->setCurrentIndex(index);
}

void GeneralPage::openMRUFileClicked(bool state)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_OpenMRUFile), state);
}

void GeneralPage::tipsVisibleClicked(bool state)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::HomeScreen_TipsVisible), state);
}

void GeneralPage::encodingChanged(const QString& name)
{
	QByteArray old_name = Settings::getInstance().getSetting(Settings::General_Local8BitEncoding).toByteArray();
	QTextCodec* codec = QTextCodec::codecForName(name.toLatin1());
	if (!codec)
	{
		codec = QTextCodec::codecForName(old_name);
	}
	if (!codec)
	{
		codec = QTextCodec::codecForLocale();
	}
	
	if (codec && codec->name() != old_name)
	{
		changes.insert(Settings::getInstance().getSettingPath(Settings::General_Local8BitEncoding), codec->name());
		ScopedSignalsBlocker block(encoding_box);
		encoding_box->setCurrentText(codec->name());
	}
}

// slot
void GeneralPage::ocdImporterClicked(bool state)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_NewOcd8Implementation), state);
}

void GeneralPage::openTranslationFileDialog()
{
	Settings& settings = Settings::getInstance();
	const QString translation_file_key(settings.getSettingPath(Settings::General_TranslationFile));
	const QString language_key(settings.getSettingPath(Settings::General_Language));
	
	QString current_filename(settings.getSetting(Settings::General_Language).toString());
	if (changes.contains(translation_file_key))
		current_filename = changes[translation_file_key].toString();
	
	QString filename = QFileDialog::getOpenFileName(this,
	  tr("Open translation"), current_filename, tr("Translation files (*.qm)"));
	if (!filename.isNull())
	{
		QString locale_name(TranslationUtil::localeNameForFile(filename));
		if (locale_name.isEmpty())
		{
			QMessageBox::critical(this, tr("Open translation"),
			  tr("The selected file is not a valid translation.") );
		}
		else
		{
			QLocale locale(locale_name);
			changes.insert(translation_file_key, filename);
			changes.insert(language_key, locale.language());
		}
	}
	updateLanguageBox();
}

void GeneralPage::autoSaveChanged(bool state)
{
	auto_save_interval_label->setEnabled(state);
	auto_save_interval_edit->setEnabled(state);
	
	int interval = auto_save_interval_edit->value();
	if (!state)
		interval = -interval;
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_AutoSaveInterval), interval);
}

void GeneralPage::autoSaveIntervalChanged(int value)
{
	changes.insert(Settings::getInstance().getSettingPath(Settings::General_AutoSaveInterval), value);
}
