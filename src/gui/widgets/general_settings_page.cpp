/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2017  Kai Pastor
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

#include <algorithm>
#include <iterator>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QApplication>
#include <QAbstractButton>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFlags>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QScreen>
#include <QSettings> // IWYU pragma: keep
#include <QSignalBlocker>
#include <QSize>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringList>
#include <QTextCodec>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "settings.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/widgets/home_screen_widget.h"
#include "gui/widgets/settings_page.h"
#include "util/translation_util.h"


namespace OpenOrienteering {

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
	
	auto language_file_button = new QToolButton();
	if (MainWindow::mobileMode())
	{
		language_file_button->setVisible(false);
	}
	else
	{
		language_file_button->setIcon(QIcon(QLatin1String(":/images/open.png")));
	}
	language_layout->addWidget(language_file_button);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Screen")));
	
	auto ppi_widget = new QWidget();
	auto ppi_layout = new QHBoxLayout(ppi_widget);
	ppi_layout->setContentsMargins({});
	layout->addRow(tr("Pixels per inch:"), ppi_widget);
	
	ppi_edit = Util::SpinBox::create(2, 0.01, 9999);
	ppi_layout->addWidget(ppi_edit);
	
	auto ppi_calculate_button = new QToolButton();
	ppi_calculate_button->setIcon(QIcon(QLatin1String(":/images/settings.png")));
	ppi_layout->addWidget(ppi_calculate_button);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Program start")));
	
	open_mru_check = new QCheckBox(::OpenOrienteering::AbstractHomeScreenWidget::tr("Open most recently used file"));
	layout->addRow(open_mru_check);
	
	tips_visible_check = new QCheckBox(::OpenOrienteering::AbstractHomeScreenWidget::tr("Show tip of the day"));
	layout->addRow(tips_visible_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("Saving files")));
	
	compatibility_check = new QCheckBox(tr("Retain compatibility with Mapper %1").arg(QLatin1String("0.5")));
#ifdef MAPPER_ENABLE_COMPATIBILITY
	layout->addRow(compatibility_check);
#else
	// Let compatibility_check be valid, but not leak
	connect(this, &QObject::destroyed, compatibility_check, &QObject::deleteLater);
#endif
	
	undo_check = new QCheckBox(tr("Save undo/redo history"));
	layout->addRow(undo_check);
	
	autosave_check = new QCheckBox(tr("Save information for automatic recovery"));
	layout->addRow(autosave_check);
	
	autosave_interval_edit = Util::SpinBox::create(1, 120, tr("min", "unit minutes"), 1);
	layout->addRow(tr("Recovery information saving interval:"), autosave_interval_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(Util::Headline::create(tr("File import and export")));
	
	QStringList available_codecs;
	available_codecs.append(tr("Default"));
	encoding_box = new QComboBox();
	encoding_box->setEditable(true);
	encoding_box->addItem(available_codecs.first());
	encoding_box->addItem(QString::fromLatin1("Windows-1252")); // Serves as an example, not translated.
	const auto available_codecs_raw = QTextCodec::availableCodecs();
	available_codecs.reserve(available_codecs_raw.size());
	for (const QByteArray& item : available_codecs_raw)
	{
		available_codecs.append(QString::fromUtf8(item));
	}
	if (available_codecs.size() > 1)
	{
		available_codecs.sort(Qt::CaseInsensitive);
		available_codecs.removeDuplicates();
		encoding_box->addItem(tr("More..."));
	}
	auto completer = new QCompleter(available_codecs, this);
	completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	encoding_box->setCompleter(completer);
	layout->addRow(tr("8-bit encoding:"), encoding_box);
	
	updateWidgets();
	
	connect(language_file_button, &QAbstractButton::clicked, this, &GeneralSettingsPage::openTranslationFileDialog);
	connect(ppi_calculate_button, &QAbstractButton::clicked, this, &GeneralSettingsPage::openPPICalculationDialog);
	connect(encoding_box, &QComboBox::currentTextChanged, this, &GeneralSettingsPage::encodingChanged);
	connect(autosave_check, &QAbstractButton::toggled, autosave_interval_edit, &QWidget::setEnabled);
	connect(autosave_check, &QAbstractButton::toggled, layout->labelForField(autosave_interval_edit), &QWidget::setEnabled);
	
}

GeneralSettingsPage::~GeneralSettingsPage() = default;



QString GeneralSettingsPage::title() const
{
	return tr("General");
}

void GeneralSettingsPage::apply()
{
	auto language = language_box->currentData().toString();
	if (language != getSetting(Settings::General_Language).toString()
	    || translation_file != getSetting(Settings::General_TranslationFile).toString())
	{
		// Show an message box in the new language.
		TranslationUtil translation(language, translation_file);
		auto new_language = QLocale(translation.code()).language();
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
		
		setSetting(Settings::General_Language, translation.code());
		setSetting(Settings::General_TranslationFile, translation_file);
#if defined(Q_OS_MACOS)
		// The native [file] dialogs will use the first element of the
		// AppleLanguages array in the application's .plist file -
		// and this file is also the one used by QSettings.
		QSettings().setValue(QString::fromLatin1("AppleLanguages"), QStringList{ translation.code() });
#endif
	}
	
	setSetting(Settings::General_OpenMRUFile, open_mru_check->isChecked());
	setSetting(Settings::HomeScreen_TipsVisible, tips_visible_check->isChecked());
	setSetting(Settings::General_RetainCompatiblity, compatibility_check->isChecked());
	setSetting(Settings::General_SaveUndoRedo, undo_check->isChecked());
	setSetting(Settings::General_PixelsPerInch, ppi_edit->value());
	
	auto encoding = encoding_box->currentText().toLatin1();
	if (QLatin1String(encoding) == encoding_box->itemText(0)
	    || !QTextCodec::codecForName(encoding))
	{
		encoding = "Default";
	}
	setSetting(Settings::General_Local8BitEncoding, encoding);
	
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

void GeneralSettingsPage::updateLanguageBox(QVariant code)
{
	auto languages = TranslationUtil::availableLanguages();
	std::sort(begin(languages), end(languages));
	
	const QSignalBlocker block(language_box);
	language_box->clear();
	for (const auto& language : languages)
		language_box->addItem(language.displayName, language.code);
	
	// If there is an explicit translation file, make sure it is in the box.
	auto language = TranslationUtil::languageFromFilename(translation_file);
	if (language.isValid())
	{
		auto index = language_box->findData(language.code);
		if (index < 0)
			language_box->addItem(language.displayName, language.code);
	}		
	
	// Select current language
	auto index = language_box->findData(code);
	if (index < 0)
	{
		code = QString::fromLatin1("en");
		index = language_box->findData(code);
	}
	language_box->setCurrentIndex(index);
}

void GeneralSettingsPage::updateWidgets()
{
	updateLanguageBox(getSetting(Settings::General_Language));
	
	ppi_edit->setValue(getSetting(Settings::General_PixelsPerInch).toDouble());
	open_mru_check->setChecked(getSetting(Settings::General_OpenMRUFile).toBool());
	tips_visible_check->setChecked(getSetting(Settings::HomeScreen_TipsVisible).toBool());
	compatibility_check->setChecked(getSetting(Settings::General_RetainCompatiblity).toBool());
	undo_check->setChecked(getSetting(Settings::General_SaveUndoRedo).toBool());
	int autosave_interval = getSetting(Settings::General_AutosaveInterval).toInt();
	autosave_check->setChecked(autosave_interval > 0);
	autosave_interval_edit->setEnabled(autosave_interval > 0);
	autosave_interval_edit->setValue(qAbs(autosave_interval));
	
	auto encoding = getSetting(Settings::General_Local8BitEncoding).toByteArray();
	if (encoding != "Default"
	    && QTextCodec::codecForName(encoding))
	{
		encoding_box->setCurrentText(QString::fromLatin1(encoding));
	}
}

// slot
void GeneralSettingsPage::encodingChanged(const QString& input)
{
	if (input == tr("More..."))
	{
		const QSignalBlocker block(encoding_box);
		encoding_box->setCurrentText(last_encoding_input);
		encoding_box->completer()->setCompletionPrefix(last_encoding_input);
		encoding_box->completer()->complete();
	}
	else
	{
		// Inline completition, in addition to UnfilteredPopupCompletition
		// Don't complete after pressing Backspace or Del
		if (!last_encoding_input.startsWith(input))
		{
			if (last_matching_completition.startsWith(input))
				encoding_box->completer()->setCompletionPrefix(last_matching_completition);
			
			auto text = encoding_box->completer()->currentCompletion();
			auto line_edit = encoding_box->lineEdit();
			if (text.startsWith(input)
			    && line_edit)
			{
				const QSignalBlocker block(encoding_box);
				auto pos = line_edit->cursorPosition();
				line_edit->setText(text);
				line_edit->setSelection(text.length(), pos - text.length());
				last_encoding_input = input.left(pos);
				last_matching_completition = text;
				return;
			}
		}
		last_encoding_input = input;
	}
}

// slot
void GeneralSettingsPage::openTranslationFileDialog()
{
	QString filename = translation_file;
	if (filename.isEmpty())
		filename = getSetting(Settings::General_TranslationFile).toString();
	
	filename = FileDialog::getOpenFileName(this,
	  tr("Open translation"), filename, tr("Translation files (*.qm)"));
	if (!filename.isNull())
	{
		auto language = TranslationUtil::languageFromFilename(filename);
		if (!language.isValid())
		{
			QMessageBox::critical(this, tr("Open translation"),
			  tr("The selected file is not a valid translation.") );
		}
		else
		{
			translation_file = filename;
			updateLanguageBox(language.code);
		}
	}
}

// slot
void GeneralSettingsPage::openPPICalculationDialog()
{
	int primary_screen_width = QApplication::primaryScreen()->size().width();
	int primary_screen_height = QApplication::primaryScreen()->size().height();
	double screen_diagonal_pixels = double(qSqrt(primary_screen_width*primary_screen_width + primary_screen_height*primary_screen_height));
	
	double old_ppi = ppi_edit->value();
	double old_screen_diagonal_inches = screen_diagonal_pixels / old_ppi;
	
	auto dialog = new QDialog(window(), Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	if (MainWindow::mobileMode())
	{
		dialog->setGeometry(window()->geometry());
	}
	
	auto layout = new QVBoxLayout(dialog);
	
	auto form_layout = new QFormLayout();
	
	auto resolution_display = new QLabel(tr("%1 x %2").arg(primary_screen_width).arg(primary_screen_height));
	form_layout->addRow(tr("Primary screen resolution in pixels:"), resolution_display);
	
	auto size_edit = Util::SpinBox::create(2, 0.01, 9999);
	size_edit->setValue(old_screen_diagonal_inches);
	form_layout->addRow(tr("Primary screen size in inches (diagonal):"), size_edit);
	
	layout->addLayout(form_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
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
		return true; // NOLINT
	
	return false;
}


}  // namespace OpenOrienteering
