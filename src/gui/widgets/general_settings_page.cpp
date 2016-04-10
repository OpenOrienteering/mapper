/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016  Kai Pastor
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

#include "general_settings_page.h"

#include <QtMath>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QMessageBox>
#include <QScreen>
#include <QTextCodec>
#include <QToolButton>

#include "home_screen_widget.h"
#include "../../util_gui.h"
#include "../../util_translation.h"
#include "../../util/scoped_signals_blocker.h"


GeneralSettingsPage::GeneralSettingsPage(QWidget* parent)
: SettingsPage(parent)
, translation_file(getSetting(Settings::General_TranslationFile).toString())
{
	auto layout = new QFormLayout(this);
	
	layout->addRow(Util::Headline::create(tr("Appearance")));
	
	auto language_widget = new QWidget();
	auto language_layout = new QHBoxLayout(language_widget);
	language_layout->setContentsMargins({});
	layout->addRow(tr("Language:"), language_widget);
	
	language_box = new QComboBox(this);
	language_layout->addWidget(language_box);
	
	QAbstractButton* language_file_button = new QToolButton();
	language_file_button->setIcon(QIcon(QLatin1String(":/images/open.png")));
	language_layout->addWidget(language_file_button);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Screen")));
	
	auto ppi_widget = new QWidget();
	auto ppi_layout = new QHBoxLayout(ppi_widget);
	ppi_layout->setContentsMargins({});
	layout->addRow(tr("Pixels per inch:"), ppi_widget);
	
	ppi_edit = Util::SpinBox::create(2, 0.01, 9999);
	ppi_layout->addWidget(ppi_edit);
	
	QAbstractButton* ppi_calculate_button = new QToolButton();
	ppi_calculate_button->setIcon(QIcon(QLatin1String(":/images/settings.png")));
	ppi_layout->addWidget(ppi_calculate_button);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Program start")));
	
	open_mru_check = new QCheckBox(AbstractHomeScreenWidget::tr("Open most recently used file"));
	layout->addRow(open_mru_check);
	
	tips_visible_check = new QCheckBox(AbstractHomeScreenWidget::tr("Show tip of the day"));
	layout->addRow(tips_visible_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Saving files")));
	
	compatibility_check = new QCheckBox(tr("Retain compatibility with Mapper %1").arg(QLatin1String("0.5")));
	layout->addRow(compatibility_check);
	
	// Possible point: limit size of undo/redo journal
	
	autosave_check = new QCheckBox(tr("Save information for automatic recovery"));
	layout->addRow(autosave_check);
	
	autosave_interval_edit = Util::SpinBox::create(1, 120, tr("min", "unit minutes"), 1);
	layout->addRow(tr("Recovery information saving interval:"), autosave_interval_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("File import and export")));
	
	encoding_box = new QComboBox();
	encoding_box->addItem(QLatin1String("System"));
	encoding_box->addItem(QLatin1String("Windows-1250"));
	encoding_box->addItem(QLatin1String("Windows-1252"));
	encoding_box->addItem(QLatin1String("ISO-8859-1"));
	encoding_box->addItem(QLatin1String("ISO-8859-15"));
	encoding_box->setEditable(true);
	QStringList availableCodecs;
	for (const QByteArray& item : QTextCodec::availableCodecs())
	{
		availableCodecs.append(QString::fromUtf8(item));
	}
	if (!availableCodecs.empty())
	{
		availableCodecs.sort(Qt::CaseInsensitive);
		availableCodecs.removeDuplicates();
		encoding_box->addItem(tr("More..."));
	}
	QCompleter* completer = new QCompleter(availableCodecs, this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	encoding_box->setCompleter(completer);
	layout->addRow(tr("8-bit encoding:"), encoding_box);
	
	ocd_importer_check = new QCheckBox(tr("Use the new OCD importer also for version 8 files").replace(QLatin1Char('8'), QString::fromLatin1("6-8")));
	layout->addRow(ocd_importer_check);
	
	updateWidgets();
	
	connect(language_file_button, &QAbstractButton::clicked, this, &GeneralSettingsPage::openTranslationFileDialog);
	connect(ppi_calculate_button, &QAbstractButton::clicked, this, &GeneralSettingsPage::openPPICalculationDialog);
	connect(encoding_box, &QComboBox::currentTextChanged, this, &GeneralSettingsPage::encodingChanged);
	connect(autosave_check, &QAbstractButton::toggled, autosave_interval_edit, &QWidget::setEnabled);
	connect(autosave_check, &QAbstractButton::toggled, layout->labelForField(autosave_interval_edit), &QWidget::setEnabled);
	
}

GeneralSettingsPage::~GeneralSettingsPage()
{
	// nothing, not inlined
}

QString GeneralSettingsPage::title() const
{
	return tr("General");
}

void GeneralSettingsPage::apply()
{
	auto language = language_box->currentData();
	if (language != getSetting(Settings::General_Language)
	    || translation_file != getSetting(Settings::General_TranslationFile).toString())
	{
		// Show an message box in the new language.
		TranslationUtil translation((QLocale::Language)language.toInt(), translation_file);
		auto new_language = translation.getLocale().language();
		switch (new_language)
		{
		case QLocale::AnyLanguage:
		case QLocale::C:
		case QLocale::English:
			QMessageBox::information(window(), QLatin1String("Notice"), QLatin1String("The program must be restarted for the language change to take effect!"));
			break;
			
		default:
			qApp->installEventFilter(this);
			qApp->installTranslator(&translation.getQtTranslator());
			qApp->installTranslator(&translation.getAppTranslator());
			QMessageBox::information(window(), tr("Notice"), tr("The program must be restarted for the language change to take effect!"));
			qApp->removeTranslator(&translation.getAppTranslator());
			qApp->removeTranslator(&translation.getQtTranslator());
			qApp->removeEventFilter(this);
		}
		
		setSetting(Settings::General_Language, new_language);
		setSetting(Settings::General_TranslationFile, translation_file);
#if defined(Q_OS_MAC)
		// The native [file] dialogs will use the first element of the
		// AppleLanguages array in the application's .plist file -
		// and this file is also the one used by QSettings.
		const QString mapper_language(translation.getLocale().name().left(2));
		QSettings().setValue(QLatin1String{"AppleLanguages"}, { mapper_language });
#endif
	}
	
	setSetting(Settings::General_OpenMRUFile, open_mru_check->isChecked());
	setSetting(Settings::HomeScreen_TipsVisible, tips_visible_check->isChecked());
	setSetting(Settings::General_NewOcd8Implementation, ocd_importer_check->isChecked());
	setSetting(Settings::General_RetainCompatiblity, compatibility_check->isChecked());
	setSetting(Settings::General_PixelsPerInch, ppi_edit->value());
	
	auto name_latin1 = encoding_box->currentText().toLatin1();
	if (name_latin1 == "System"
	    || QTextCodec::codecForName(name_latin1))
	{
		setSetting(Settings::General_Local8BitEncoding, name_latin1);
	}
	
	int interval = autosave_interval_edit->value();
	if (!autosave_check->isChecked())
		interval = -interval;
	setSetting(Settings::General_AutosaveInterval, interval);
}

void GeneralSettingsPage::reset()
{
	translation_file = getSetting(Settings::General_TranslationFile).toString();
	updateWidgets();
}

void GeneralSettingsPage::updateLanguageBox(QVariant language)
{
	LanguageCollection language_map = TranslationUtil::getAvailableLanguages();
	
	// If there is an explicit translation file, use its locale
	QString locale_name = TranslationUtil::localeNameForFile(translation_file);
	if (!locale_name.isEmpty())
	{
		QLocale file_locale(locale_name);
		QString language_name = file_locale.nativeLanguageName();
		if (!language_map.contains(language_name))
			language_map.insert(language_name, file_locale.language());
	}
	
	// Update the language box
	const QSignalBlocker block(language_box);
	language_box->clear();
	
	for (auto it = language_map.constBegin(),end = language_map.constEnd(); it != end; ++it)
		language_box->addItem(it.key(), (int)it.value());
	
	// Select current language
	int index = language_box->findData(language);
	if (index < 0)
	{
		language = QLocale::English;
		index = language_box->findData(language);
	}
	language_box->setCurrentIndex(index);
}

void GeneralSettingsPage::updateWidgets()
{
	updateLanguageBox(getSetting(Settings::General_Language));
	
	ppi_edit->setValue(getSetting(Settings::General_PixelsPerInch).toFloat());
	open_mru_check->setChecked(getSetting(Settings::General_OpenMRUFile).toBool());
	tips_visible_check->setChecked(getSetting(Settings::HomeScreen_TipsVisible).toBool());
	compatibility_check->setChecked(getSetting(Settings::General_RetainCompatiblity).toBool());
	
	int autosave_interval = getSetting(Settings::General_AutosaveInterval).toInt();
	autosave_check->setChecked(autosave_interval > 0);
	autosave_interval_edit->setEnabled(autosave_interval > 0);
	autosave_interval_edit->setValue(qAbs(autosave_interval));
	
	encoding_box->setCurrentText(getSetting(Settings::General_Local8BitEncoding).toString());
	ocd_importer_check->setChecked(getSetting(Settings::General_NewOcd8Implementation).toBool());
}

// slot
void GeneralSettingsPage::encodingChanged(const QString& name)
{
	const QSignalBlocker block(encoding_box);
	
	if (name == tr("More..."))
	{
		encoding_box->setCurrentText(QString());
		encoding_box->completer()->setCompletionPrefix(QString());
		encoding_box->completer()->complete();
		return;
	}
	
	auto name_latin1 = name.toLatin1();
	QTextCodec* codec = (name_latin1 == "System")
	                    ? QTextCodec::codecForLocale()
	                    : QTextCodec::codecForName(name_latin1);
	if (codec)
	{
		encoding_box->setCurrentText(name);
	}
}

// slot
void GeneralSettingsPage::openTranslationFileDialog()
{
	QString filename = translation_file;
	if (filename.isEmpty())
		filename = getSetting(Settings::General_TranslationFile).toString();
	
	filename = QFileDialog::getOpenFileName(this,
	  tr("Open translation"), filename, tr("Translation files (*.qm)"));
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
			translation_file = filename;
			updateLanguageBox(QLocale(locale_name).language());
		}
	}
}

// slot
void GeneralSettingsPage::openPPICalculationDialog()
{
	int primary_screen_width = QApplication::primaryScreen()->size().width();
	int primary_screen_height = QApplication::primaryScreen()->size().height();
	float screen_diagonal_pixels = qSqrt(primary_screen_width*primary_screen_width + primary_screen_height*primary_screen_height);
	
	float old_ppi = ppi_edit->value();
	float old_screen_diagonal_inches = screen_diagonal_pixels / old_ppi;
	
	QDialog* dialog = new QDialog(window(), Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	
	auto layout = new QVBoxLayout(dialog);
	
	auto form_layout = new QFormLayout();
	
	QLabel* resolution_display = new QLabel(tr("%1 x %2").arg(primary_screen_width).arg(primary_screen_height));
	form_layout->addRow(tr("Primary screen resolution in pixels:"), resolution_display);
	
	QDoubleSpinBox* size_edit = Util::SpinBox::create(2, 0.01, 9999);
	size_edit->setValue(old_screen_diagonal_inches);
	form_layout->addRow(tr("Primary screen size in inches (diagonal):"), size_edit);
	
	layout->addLayout(form_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	layout->addWidget(button_box);
	
	connect(button_box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	dialog->exec();
	
	if (dialog->result() == QDialog::Accepted)
	{
		ppi_edit->setValue(screen_diagonal_pixels / size_edit->value());
	}
}

bool GeneralSettingsPage::eventFilter(QObject* /* watched */, QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
		return true;
	
	return false;
}

