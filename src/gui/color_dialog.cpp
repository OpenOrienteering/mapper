/*
 *    Copyright 2012-2017 Kai Pastor
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


#include "color_dialog.h"

#include <cstddef>
#include <memory>

#include <Qt>
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFlags>
#include <QGridLayout>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSize>
#include <QSpacerItem>
#include <QString>
#include <QStyle>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>

#include "core/map.h"
#include "core/map_color.h"
#include "gui/util_gui.h"
#include "gui/widgets/color_dropdown.h"
#include "util/backports.h"
#include "util/util.h"
#include "util/translation_util.h"


namespace OpenOrienteering {

namespace {
	
	constexpr QSize coloriconSize()
	{
		return QSize{32, 32};
	}
	
	
}  // namespace



ColorDialog::ColorDialog(const Map& map, const MapColor& source_color, QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f)
, map(map)
, source_color(source_color)
, color(source_color)
, color_modified(false)
, react_to_changes(true)
{
	setWindowTitle(tr("Edit map color"));
	setSizeGripEnabled(true);
	
	color_preview_label = new QLabel();
	color_name_label = new QLabel();
	mc_name_edit = new QLineEdit();
	language_combo = new QComboBox();
	name_edit_button = new QPushButton(tr("Edit"));
	
	prof_color_layout = new QGridLayout();
	int col = 0;
	prof_color_layout->setColumnStretch(col, 1);
	prof_color_layout->setColumnStretch(col+1, 3);
	
	int row = 0;
	prof_color_layout->addWidget(Util::Headline::create(tr("Spot color printing")), row, col, 1, 2);
	
	auto spot_color_options = new QButtonGroup(this);
	
	++row;
	full_tone_option = new QRadioButton(tr("Defines a spot color:"));
	spot_color_options->addButton(full_tone_option, MapColor::SpotColor);
	prof_color_layout->addWidget(full_tone_option, row, col, 1, 2);
	
	++row;
	sc_name_edit = new QLineEdit();
	prof_color_layout->addWidget(sc_name_edit, row, col, 1, 2);
	
	++row;
	prof_color_layout->addWidget(new QLabel(tr("Screen frequency:")), row, col, 1, 1);
	sc_frequency_edit = Util::SpinBox::create(1, 0.0, 4800.0, tr("lpi"), 10.0);
	sc_frequency_edit->setSpecialValueText(tr("Undefined"));
	prof_color_layout->addWidget(sc_frequency_edit, row, col+1, 1, 1);
	
	++row;
	prof_color_layout->addWidget(new QLabel(tr("Screen angle:")), row, col, 1, 1);
	sc_angle_edit = Util::SpinBox::create(1, 0.0, 359.9, trUtf8("°"), 1.0);
	prof_color_layout->addWidget(sc_angle_edit, row, col+1, 1, 1);
	
	++row;
	composition_option = new QRadioButton(tr("Mixture of spot colors (screens and overprint):"));
	spot_color_options->addButton(composition_option, MapColor::CustomColor);
	prof_color_layout->addWidget(composition_option, row, col, 1, 2);
	
	std::size_t num_components = 0 /*color.getComponents().size()*/; // FIXME: cleanup
	components_row0 = row+1;
	components_col0 = col;
	component_colors.resize(num_components+1);
	component_halftone.resize(num_components+1);
	for (std::size_t i = 0; i <= num_components; i++)
	{
		++row;
		component_colors[i] = new ColorDropDown(&map, &color, true);
		component_colors[i]->removeColor(&source_color);
		prof_color_layout->addWidget(component_colors[i], row, col);
		component_halftone[i] = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
		prof_color_layout->addWidget(component_halftone[i], row, col+1);
	}
	
	++row;
	knockout_option = new QCheckBox(tr("Knockout: erases lower colors"));
	prof_color_layout->addWidget(knockout_option, row, col, 1, 2);
	knockout_option->setEnabled(false);
	
	row = 0; col += 2;
	prof_color_layout->setColumnStretch(col, 1);
	
	const int spacing = style()->pixelMetric(QStyle::PM_LayoutTopMargin);
	prof_color_layout->addItem(new QSpacerItem(3*spacing, spacing), row, col, 7, 1);
	
	row = 0; col +=1;
	prof_color_layout->setColumnStretch(col, 1);
	prof_color_layout->setColumnStretch(col+1, 3);
	prof_color_layout->addWidget(Util::Headline::create(tr("CMYK")), row, col, 1, 2);
	
	auto cmyk_color_options = new QButtonGroup(this);
	
	++row;
	cmyk_spot_color_option = new QRadioButton(tr("Calculate from spot colors"));
	cmyk_color_options->addButton(cmyk_spot_color_option, MapColor::SpotColor);
	prof_color_layout->addWidget(cmyk_spot_color_option, row, col, 1, 2);
	
	++row;
	evaluate_rgb_option = new QRadioButton(tr("Calculate from RGB color"));
	cmyk_color_options->addButton(evaluate_rgb_option, MapColor::RgbColor);
	prof_color_layout->addWidget(evaluate_rgb_option, row, col, 1, 2);
	
	++row;
	custom_cmyk_option = new QRadioButton(tr("Custom process color:"));
	cmyk_color_options->addButton(custom_cmyk_option, MapColor::CustomColor);
	prof_color_layout->addWidget(custom_cmyk_option, row, col, 1, 2);
	
	++row;
	c_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Cyan")), row, col);
	prof_color_layout->addWidget(c_edit, row, col+1);
	
	++row;
	m_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Magenta")), row, col);
	prof_color_layout->addWidget(m_edit, row, col+1);
	
	++row;
	y_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Yellow")), row, col);
	prof_color_layout->addWidget(y_edit, row, col+1);
	
	++row;
	k_edit = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
	prof_color_layout->addWidget(new QLabel(tr("Black")), row, col);
	prof_color_layout->addWidget(k_edit, row, col+1);
	
	++row;
	stretch_row0 = row;
	stretch_col0 = col;
	stretch = new QWidget();
	prof_color_layout->addWidget(stretch, row, col);
	prof_color_layout->setRowStretch(row, 1);
	
	auto prof_color_widget = new QWidget();
	prof_color_widget->setLayout(prof_color_layout);
	prof_color_widget->setObjectName(QString::fromLatin1("professional"));
	
	
	auto desktop_layout = new QGridLayout();
	col = 0;
	desktop_layout->setColumnStretch(col, 1);
	desktop_layout->setColumnStretch(col+1, 3);
	
	row = 0;
	desktop_layout->addWidget(Util::Headline::create(tr("RGB")), row, col, 1, 2);
	
	auto rgb_color_options = new QButtonGroup(this);
	
	++row;
	rgb_spot_color_option = new QRadioButton(tr("Calculate from spot colors"));
	rgb_color_options->addButton(rgb_spot_color_option, MapColor::SpotColor);
	desktop_layout->addWidget(rgb_spot_color_option, row, col, 1, 2);
	
	++row;
	evaluate_cmyk_option = new QRadioButton(tr("Calculate from CMYK color"));
	rgb_color_options->addButton(evaluate_cmyk_option, MapColor::CmykColor);
	desktop_layout->addWidget(evaluate_cmyk_option, row, col, 1, 2);
	
	++row;
	custom_rgb_option = new QRadioButton(tr("Custom RGB color:"));
	rgb_color_options->addButton(custom_rgb_option, MapColor::CustomColor);
	desktop_layout->addWidget(custom_rgb_option, row, col, 1, 2);
	
	++row;
	r_edit = Util::SpinBox::create(1, 0.0, 255.0, {}, 5);
	desktop_layout->addWidget(new QLabel(tr("Red")), row, col);
	desktop_layout->addWidget(r_edit, row, col+1);
	
	++row;
	g_edit = Util::SpinBox::create(1, 0.0, 255.0, {}, 5);
	desktop_layout->addWidget(new QLabel(tr("Green")), row, col);
	desktop_layout->addWidget(g_edit, row, col+1);
	
	++row;
	b_edit = Util::SpinBox::create(1, 0.0, 255.0, {}, 5);
	desktop_layout->addWidget(new QLabel(tr("Blue")), row, col);
	desktop_layout->addWidget(b_edit, row, col+1);
	
	++row;
	html_edit = new QLineEdit();
	desktop_layout->addWidget(new QLabel(tr("#RRGGBB")), row, col);
	desktop_layout->addWidget(html_edit, row, col+1);
	
	++row;
	desktop_layout->addWidget(new QWidget(), row, col);
	desktop_layout->setRowStretch(row, 1);
	
	row = 0; col += 2;
	desktop_layout->setColumnStretch(col, 7);
	
	desktop_layout->addItem(new QSpacerItem(3*spacing, spacing), row, col, 7, 1);
	
	auto desktop_color_widget = new QWidget();
	desktop_color_widget->setLayout(desktop_layout);
	desktop_color_widget->setObjectName(QString::fromLatin1("desktop"));
	
	
	properties_widget = new QTabWidget();
	properties_widget->addTab(desktop_color_widget, tr("Desktop"));
	properties_widget->addTab(prof_color_widget, tr("Professional printing"));
	
	auto button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok | QDialogButtonBox::Reset | QDialogButtonBox::Help);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	reset_button = button_box->button(QDialogButtonBox::Reset);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(button_box, &QDialogButtonBox::accepted, this, &ColorDialog::accept);
	connect(reset_button, &QAbstractButton::clicked, this, &ColorDialog::reset);
	connect(button_box->button(QDialogButtonBox::Help), &QAbstractButton::clicked, this, &ColorDialog::showHelp);
	
	auto layout = new QGridLayout();
	row = 0; col = 0;
	layout->addWidget(color_preview_label, row, col); col++;
	layout->addWidget(color_name_label, row, col, 1, 4);
	row++; col = 0;
	layout->addWidget(new QLabel(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Text source:")), row, col, 1, 2); col+=2;
	layout->addWidget(language_combo, row, col); col++;
	layout->addWidget(name_edit_button, row, col);
	row++; col = 0;
	layout->addWidget(new QLabel(tr("Name")), row, col, 1, 2); col+=2;
	layout->addWidget(mc_name_edit, row, col, 1, 3);
	row++; col = 0;
	layout->addWidget(properties_widget, row, col, 1, 5);
	row++;
	layout->addWidget(button_box, row, col, 1, 5);
	layout->setColumnStretch(4, 1);
	setLayout(layout);
	
	reset();
	updateButtons();
	
	connect(language_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorDialog::languageChanged);
	connect(name_edit_button, &QPushButton::clicked, this, &ColorDialog::editClicked);
	connect(mc_name_edit, &QLineEdit::textChanged, this, &ColorDialog::mapColorNameChanged);
	
	connect(spot_color_options, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ColorDialog::spotColorTypeChanged);
	connect(sc_name_edit, &QLineEdit::textChanged, this, &ColorDialog::spotColorNameChanged);
	connect(sc_frequency_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::spotColorScreenChanged);
	connect(sc_angle_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::spotColorScreenChanged);
	for (std::size_t i = 0; i < component_colors.size(); i++)
	{
		connect(component_colors[i], QOverload<int>::of(&ColorDropDown::currentIndexChanged), this, &ColorDialog::spotColorCompositionChanged);
		connect(component_halftone[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::spotColorCompositionChanged);
	}
	connect(knockout_option, &QAbstractButton::clicked, this, &ColorDialog::knockoutChanged);
	
	connect(cmyk_color_options, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ColorDialog::cmykColorTypeChanged);
	connect(c_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::cmykValueChanged);
	connect(m_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::cmykValueChanged);
	connect(y_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::cmykValueChanged);
	connect(k_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::cmykValueChanged);
	
	connect(rgb_color_options, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ColorDialog::rgbColorTypeChanged);
	connect(r_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::rgbValueChanged);
	connect(g_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::rgbValueChanged);
	connect(b_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::rgbValueChanged);
	
	QSettings settings;
	settings.beginGroup(QString::fromLatin1("ColorDialog"));
	QString default_view = settings.value(QString::fromLatin1("view")).toString();
	settings.endGroup();
	properties_widget->setCurrentWidget(properties_widget->findChild<QWidget*>(default_view));
}


ColorDialog::~ColorDialog() = default;



void ColorDialog::updateColorLabel()
{
	auto name = map.translate(color.getName());
	if (name.isEmpty())
		name = tr("- unnamed -");
	color_name_label->setText(QLatin1String("<b>") + name + QLatin1String("</b>"));
}


void ColorDialog::updateWidgets()
{
	react_to_changes = false;
	
	QPixmap pixmap(coloriconSize());
	pixmap.fill(colorWithOpacity(color));
	color_preview_label->setPixmap(pixmap);
	
	sc_name_edit->setText(color.getSpotColorName());
	
	const MapColorCmyk& cmyk = color.getCmyk();
	c_edit->setValue(100.0 * double(cmyk.c));
	m_edit->setValue(100.0 * double(cmyk.m));
	y_edit->setValue(100.0 * double(cmyk.y));
	k_edit->setValue(100.0 * double(cmyk.k));
	
	knockout_option->setChecked(color.getKnockout());
	
	if (color.getSpotColorMethod() == MapColor::SpotColor)
	{
		full_tone_option->setChecked(true);
		sc_name_edit->setEnabled(true);
		sc_frequency_edit->setEnabled(true);
		sc_frequency_edit->setValue(color.getScreenFrequency());
		sc_angle_edit->setEnabled(sc_frequency_edit->value() > 0);
		sc_angle_edit->setValue(color.getScreenAngle());
		knockout_option->setEnabled(true);
		cmyk_spot_color_option->setEnabled(false);
		if (cmyk_spot_color_option->isChecked())
			custom_cmyk_option->setChecked(true);
		rgb_spot_color_option->setEnabled(false);
		if (rgb_spot_color_option->isChecked())
			custom_rgb_option->setChecked(true);
	}
	else if (color.getSpotColorMethod() == MapColor::CustomColor)
	{
		composition_option->setChecked(true);
		sc_name_edit->setEnabled(false);
		sc_frequency_edit->setEnabled(false);
		sc_angle_edit->setEnabled(false);
		knockout_option->setEnabled(true);
		cmyk_spot_color_option->setEnabled(true);
		rgb_spot_color_option->setEnabled(true);
	}
	else
	{
		composition_option->setChecked(true);
		sc_name_edit->setEnabled(false);
		sc_frequency_edit->setEnabled(false);
		sc_angle_edit->setEnabled(false);
		cmyk_spot_color_option->setEnabled(false);
		if (cmyk_spot_color_option->isChecked())
		{
			custom_cmyk_option->setChecked(true);
		}
		cmyk_spot_color_option->setEnabled(false);
		if (rgb_spot_color_option->isChecked())
		{
			custom_rgb_option->setChecked(true);
		}
		rgb_spot_color_option->setEnabled(false);
	}
	
	const SpotColorComponents& color_components = color.getComponents();
	auto num_components = color_components.size();
	auto num_editors = component_colors.size();
	
	for (auto i = num_components+1; i < num_editors; ++i)
	{
		prof_color_layout->removeWidget(component_colors[i]);
		delete component_colors[i];
		prof_color_layout->removeWidget(component_halftone[i]);
		delete component_halftone[i];
	}
	
	if (num_editors != num_components+1)
	{
		prof_color_layout->removeWidget(knockout_option);
		prof_color_layout->addWidget(knockout_option, components_row0+int(num_components)+1, components_col0);
	}
	
	component_colors.resize(num_components+1);
	component_halftone.resize(num_components+1);
	for (auto i = num_editors; i <= num_components; ++i)
	{
		component_colors[i] = new ColorDropDown(&map, &color, true);
		component_colors[i]->removeColor(&source_color);
		prof_color_layout->addWidget(component_colors[i], components_row0+int(i), components_col0);
		connect(component_colors[i], QOverload<int>::of(&ColorDropDown::currentIndexChanged), this, &ColorDialog::spotColorCompositionChanged);
		component_halftone[i] = Util::SpinBox::create(1, 0.0, 100.0, tr("%"), 10.0);
		prof_color_layout->addWidget(component_halftone[i], components_row0+int(i), components_col0+1);
		connect(component_halftone[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorDialog::spotColorCompositionChanged);
	}
	
	num_editors = component_colors.size();
	bool enable_component = composition_option->isChecked();
	for (std::size_t i = 0; i < num_editors; i++)
	{
		bool have_component = (i < num_components);
		const MapColor* component_color = have_component ? color_components[i].spot_color : nullptr;
		component_colors[i]->setEnabled(enable_component);
		component_colors[i]->setColor(component_color);
		
		bool enable_halftone = enable_component && have_component;
		auto component_factor = enable_halftone ? double(color_components[i].factor) : 0.0;
		component_halftone[i]->setEnabled(enable_halftone);
		component_halftone[i]->setValue(component_factor * 100.0);
		
		prof_color_layout->setRowStretch(components_row0 + int(i), 0);
		
		enable_component = enable_component && enable_halftone;
	}
	// At least one component must be editable to create a composition
	if (color.getSpotColorMethod() == MapColor::UndefinedMethod)
		component_colors[0]->setEnabled(true);
	
	auto stretch_row = qMax(stretch_row0, components_row0+int(num_editors));
	if (stretch_row != stretch_row0)
	{
		prof_color_layout->removeWidget(stretch);
		prof_color_layout->addWidget(stretch, stretch_row, stretch_col0);
		prof_color_layout->setRowStretch(stretch_row, 1);
	}
	
	bool custom_cmyk = false;
	if (color.getCmykColorMethod() == MapColor::SpotColor)
	{
		cmyk_spot_color_option->setChecked(true);
	}
	else if (color.getCmykColorMethod() == MapColor::RgbColor)
	{
		evaluate_rgb_option->setChecked(true);
	}
	else if (color.getCmykColorMethod() == MapColor::CustomColor)
	{
		custom_cmyk_option->setChecked(true);
		custom_cmyk = true;
	}
	c_edit->setEnabled(custom_cmyk);
	m_edit->setEnabled(custom_cmyk);
	y_edit->setEnabled(custom_cmyk);
	k_edit->setEnabled(custom_cmyk);
	
	
	const MapColorRgb& rgb = color.getRgb();
	r_edit->setValue(255.0 * double(rgb.r));
	g_edit->setValue(255.0 * double(rgb.g));
	b_edit->setValue(255.0 * double(rgb.b));
	html_edit->setText(QColor(rgb).name());
	
	bool custom_rgb = false;
	if (color.getRgbColorMethod() == MapColor::SpotColor)
	{
		rgb_spot_color_option->setChecked(true);
	}
	else if (color.getRgbColorMethod() == MapColor::CmykColor)
	{
		evaluate_cmyk_option->setChecked(true);
	}
	else if (color.getRgbColorMethod() == MapColor::CustomColor)
	{
		custom_rgb_option->setChecked(true);
		custom_rgb = true;
	}
	r_edit->setEnabled(custom_rgb);
	g_edit->setEnabled(custom_rgb);
	b_edit->setEnabled(custom_rgb);
	html_edit->setEnabled(false); // TODO: Editor
	
	
	react_to_changes = true;
}

void ColorDialog::accept()
{
	QSettings settings;
	settings.beginGroup(QString::fromLatin1("ColorDialog"));
	settings.setValue(QString::fromLatin1("view"), properties_widget->currentWidget()->objectName());
	settings.endGroup();
	
	QDialog::accept();
}

void ColorDialog::reset()
{
	color = source_color;
	updateWidgets();
	
	language_combo->clear();
	language_combo->addItem(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Map (%1)")
	                        .arg(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "undefined language")));
	auto name = color.getName();
	auto display_name = map.raw_translation(name);
	if (display_name.isEmpty())
	{
		language_combo->setEnabled(false);
		name_edit_button->setEnabled(false);
		mc_name_edit->setText(name);
		mc_name_edit->setEnabled(true);
	}
	else
	{
		auto language = TranslationUtil::languageFromSettings(QSettings());
		if (!language.isValid())
		{
			language.displayName = QApplication::translate("OpenOrienteering::MapSymbolTranslation", "undefined language");
		}

		language_combo->addItem(QApplication::translate("OpenOrienteering::MapSymbolTranslation", "Translation (%1)").arg(language.displayName));
		language_combo->setCurrentIndex(1);
		language_combo->setEnabled(true);
		name_edit_button->setEnabled(true);
		mc_name_edit->setText(display_name);
		mc_name_edit->setEnabled(false);
	}
	
	setColorModified(false);
}

void ColorDialog::setColorModified(bool modified)
{
	updateColorLabel();
	if (color_modified != modified)
	{
		color_modified = modified;
		updateButtons();
	}
}

void ColorDialog::updateButtons()
{
	ok_button->setEnabled(color_modified);
	reset_button->setEnabled(color_modified);
}

void ColorDialog::showHelp()
{
	Util::showHelp(this, "color_dock_widget.html", "editor");
}



void ColorDialog::languageChanged()
{
	auto name = color.getName();
	if (language_combo->currentIndex() == 1)
	{
		name = map.raw_translation(name);
	}
	QSignalBlocker block(mc_name_edit);
	mc_name_edit->setText(name);
}


void ColorDialog::editClicked()
{
	int result;
	auto question = QString{};
	if (language_combo->currentIndex() == 1)
	{
		question = QApplication::translate("OpenOrienteering::MapSymbolTranslation",
		             "Before editing, the stored text will be "
		             "replaced with the current translation. "
		             "Do you want to continue?");
	}
	else
	{
		question = QApplication::translate("OpenOrienteering::MapSymbolTranslation",
		             "After modifying the stored text, "
		             "the translation may no longer be found. "
		             "Do you want to continue?");
	}
	result = QMessageBox::warning(this, tr("Warning"), question,
	                              QMessageBox::Yes | QMessageBox::No,
	                              QMessageBox::Yes);
	if (result == QMessageBox::Yes)
	{
		language_combo->setEnabled(false);
		name_edit_button->setEnabled(false);
		mc_name_edit->setEnabled(true);
		if (language_combo->currentIndex() == 1)
		{
			{
				QSignalBlocker block(language_combo);
				language_combo->setCurrentIndex(0);
			}
			auto name = mc_name_edit->text();
			if (name.isEmpty())
			{
				QSignalBlocker block(mc_name_edit);
				name = color.getName();
				mc_name_edit->setText(name);
			}
			color.setName(name);
		}
		setColorModified(true);
	}
}



// slot
void ColorDialog::mapColorNameChanged()
{
	if (!react_to_changes)
		return;
	
	color.setName(mc_name_edit->text());
	
	setColorModified();
}

void ColorDialog::spotColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	QString name;
	switch (id)
	{
		case MapColor::SpotColor:
			name = color.getName();
			if (name.isEmpty())
				name = QLatin1Char('?');
			color.setSpotColorName(name);
			break;
		case MapColor::CustomColor:
			if (source_color.getSpotColorMethod() == MapColor::CustomColor)
				color.setSpotColorComposition(source_color.getComponents());
			else
				color.setSpotColorComposition({});
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::spotColorNameChanged()
{
	if (!react_to_changes)
		return;
	
	Q_ASSERT(full_tone_option->isChecked());
	color.setSpotColorName(sc_name_edit->text());
	
	setColorModified();
}

void ColorDialog::spotColorScreenChanged()
{
	if (!react_to_changes)
		return;
	
	auto modified = false;
	auto frequency = sc_frequency_edit->value();
	if (std::abs(frequency - color.getScreenFrequency()) >= 0.05)
	{
		color.setScreenFrequency(frequency);
		sc_angle_edit->setEnabled(frequency > 0);
		modified = true;
	}
	auto angle = sc_angle_edit->value();
	if (std::abs(angle - color.getScreenAngle()) >= 0.05)
	{
		color.setScreenAngle(angle);
		modified = true;
	}
	
	setColorModified(modified);
}

void ColorDialog::spotColorCompositionChanged()
{
	if (!react_to_changes)
		return;
	
	SpotColorComponents components;
	SpotColorComponent component;
	auto num_editors = component_colors.size();
	components.reserve(num_editors);
	for (std::size_t i = 0; i < num_editors; i++)
	{
		if (!component_colors[i]->isEnabled())
			break;
		
		const MapColor* spot_color = component_colors[i]->color();
		if (!spot_color)
			continue;
		
		component.spot_color = spot_color;
		component.factor = float(component_halftone[i]->value() / 100);
		components.push_back(component);
	}
	color.setSpotColorComposition(components);
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::knockoutChanged()
{
	if (!react_to_changes)
		return;
	
	color.setKnockout(knockout_option->isChecked());
	setColorModified();
}

void ColorDialog::cmykColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	switch (id)
	{
		case MapColor::SpotColor:
			color.setCmykFromSpotColors();
			break;
		case MapColor::RgbColor:
// 			color.setRgb(color.getRgb());
			color.setCmykFromRgb();
			break;
		case MapColor::CustomColor:
			color.setCmyk(color.getCmyk());
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::cmykValueChanged()
{
	if (!react_to_changes)
		return;
	
	if (custom_cmyk_option->isChecked())
	{
		color.setCmyk( MapColorCmyk(
		  float(c_edit->value()/100), float(m_edit->value()/100), float(y_edit->value()/100), float(k_edit->value()/100)
		) );
		updateWidgets();
		setColorModified(true);
	}
}

void ColorDialog::rgbColorTypeChanged(int id)
{
	if (!react_to_changes)
		return;
	
	switch (id)
	{
		case MapColor::SpotColor:
			color.setRgbFromSpotColors();
			break;
		case MapColor::CmykColor:
// 			color.setCmyk(color.getCmyk());
			color.setRgbFromCmyk();
			break;
		case MapColor::CustomColor:
			color.setRgb(color.getRgb());
			break;
		default:
			; // nothing
	}
	
	updateWidgets();
	setColorModified(true);
}

void ColorDialog::rgbValueChanged()
{
	if (!react_to_changes)
		return;
	
	if (custom_rgb_option->isChecked())
	{
		color.setRgb( MapColorRgb(
		  float(r_edit->value()/255), float(g_edit->value()/255), float(b_edit->value()/255)
		) );
		updateWidgets();
		setColorModified(true);
	}
}


}  // namespace OpenOrienteering
