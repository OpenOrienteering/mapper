/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2019 Kai Pastor
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


#include "text_symbol_settings.h"

#include <algorithm>
#include <vector>

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLatin1Char>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLocale>
#include <QPainterPath>
#include <QPushButton>
#include <QRadioButton>
#include <QRectF>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

#include "core/symbols/symbol.h"
#include "core/symbols/text_symbol.h"
#include "gui/util_gui.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

// ### DetermineFontSizeDialog ###

/**
 * \todo Move translation items to TextSymbolSettings class.
 */
class DetermineFontSizeDialog
{
public:
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::DetermineFontSizeDialog)
};



// ### TextSymbol ###

SymbolPropertiesWidget* TextSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new TextSymbolSettings(this, dialog);
}


// ### TextSymbolSettings ###

TextSymbolSettings::TextSymbolSettings(TextSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), 
  symbol(symbol), 
  dialog(dialog)
{
	auto map = dialog->getPreviewMap();
	react_to_changes = true;
	
	auto text_tab = new QWidget();
	addPropertiesGroup(tr("Text settings"), text_tab);
	
	auto layout = new QFormLayout();
	text_tab->setLayout(layout);
	
	oriented_to_north = new QCheckBox(QCoreApplication::translate("OpenOrienteering::PointSymbolEditorWidget", "Always oriented to north (not rotatable)"));
	oriented_to_north->setChecked(!symbol->isRotatable());
	layout->addRow(oriented_to_north);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	font_edit = new QFontComboBox();
	layout->addRow(tr("Font family:"), font_edit);
	
	// 0.04 pt font size is app. 0.01 mm letter height, the non-zero minimum
	font_size_edit = Util::SpinBox::create(2, 0.04, 40000.0, tr("pt"));
	layout->addRow(tr("Font size:"), font_size_edit);
	
	auto letter_size_layout = new QHBoxLayout();
	letter_size_layout->setMargin(0);
	
	letter_size_layout->addWidget(new QLabel(::OpenOrienteering::DetermineFontSizeDialog::tr("Letter:")));
	//: "A" is the default letter which is used for determining letter height.
	letter_edit = new QLineEdit(DetermineFontSizeDialog::tr("A"));
	letter_edit->setMaxLength(3);
	letter_size_layout->addWidget(letter_edit);
	
	letter_size_layout->addSpacing(8);
	
	letter_size_layout->addWidget(new QLabel(::OpenOrienteering::DetermineFontSizeDialog::tr("Height:")));
	letter_size_edit = Util::SpinBox::create(2, 0.01, 10000.0, tr("mm"));
	letter_size_layout->addWidget(letter_size_edit);
	
	layout->addRow(new QWidget(), letter_size_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	color_edit = new ColorDropDown(map, symbol->getColor());
	layout->addRow(tr("Text color:"), color_edit);
	
	auto text_style_layout = new QVBoxLayout();
	bold_check = new QCheckBox(tr("bold"));
	text_style_layout->addWidget(bold_check);
	italic_check = new QCheckBox(tr("italic"));
	text_style_layout->addWidget(italic_check);
	underline_check = new QCheckBox(tr("underlined"));
	text_style_layout->addWidget(underline_check);
	
	layout->addRow(tr("Text style:"), text_style_layout);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	line_spacing_edit = Util::SpinBox::create(1, 0.0, 999999.9, tr("%"));
	layout->addRow(tr("Line spacing:"), line_spacing_edit);
	
	paragraph_spacing_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	layout->addRow(tr("Paragraph spacing:"), paragraph_spacing_edit);
	
	character_spacing_edit = Util::SpinBox::create(1, -999999.9, 999999.9, tr("%"));
	layout->addRow(tr("Character spacing:"), character_spacing_edit);
	
	kerning_check = new QCheckBox(tr("Kerning"));
	layout->addRow(QString{}, kerning_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	icon_text_edit = new QLineEdit();
	icon_text_edit->setMaxLength(3);
	layout->addRow(tr("Symbol icon text:"), icon_text_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	framing_check = new QCheckBox(tr("Framing"));
	layout->addRow(framing_check);
	
	ocad_compat_check = new QCheckBox(tr("OCAD compatibility settings"));
	layout->addRow(ocad_compat_check);
	
	
	framing_widget = new QWidget();
	addPropertiesGroup(tr("Framing"), framing_widget);
	
	auto framing_layout = new QFormLayout();
	framing_widget->setLayout(framing_layout);
	
	framing_color_edit = new ColorDropDown(map, symbol->getFramingColor());
	framing_layout->addRow(tr("Framing color:"), framing_color_edit);
	
	framing_line_radio = new QRadioButton(tr("Line framing"));
	framing_layout->addRow(framing_line_radio);
	
	framing_line_half_width_edit = Util::SpinBox::create(1, 0.0, 999999.9);
	framing_layout->addRow(tr("Width:"), framing_line_half_width_edit);
	
	framing_shadow_radio = new QRadioButton(tr("Shadow framing"));
	framing_layout->addRow(framing_shadow_radio);
	
	framing_shadow_x_offset_edit = Util::SpinBox::create(1, -999999.9, 999999.9);
	framing_layout->addRow(tr("Left/Right Offset:"), framing_shadow_x_offset_edit);
	
	framing_shadow_y_offset_edit = Util::SpinBox::create(1, -999999.9, 999999.9);
	framing_layout->addRow(tr("Top/Down Offset:"), framing_shadow_y_offset_edit);
	
	
	ocad_compat_widget = new QWidget();
	addPropertiesGroup(tr("OCAD compatibility"), ocad_compat_widget);
	
	auto ocad_compat_layout = new QFormLayout();
	ocad_compat_widget->setLayout(ocad_compat_layout);
	
	ocad_compat_layout->addRow(Util::Headline::create(tr("Line below paragraphs")));
	
	line_below_check = new QCheckBox(tr("enabled"));
	ocad_compat_layout->addRow(line_below_check);
	
	line_below_width_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	ocad_compat_layout->addRow(tr("Line width:"), line_below_width_edit);
	
	line_below_color_edit = new ColorDropDown(map);
	ocad_compat_layout->addRow(tr("Line color:"), line_below_color_edit);
	
	line_below_distance_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	ocad_compat_layout->addRow(tr("Distance from baseline:"), line_below_distance_edit);
	
	ocad_compat_layout->addItem(Util::SpacerItem::create(this));
	
	ocad_compat_layout->addRow(Util::Headline::create(tr("Custom tabulator positions")));
	
	custom_tab_list = new QListWidget();
	ocad_compat_layout->addRow(custom_tab_list);
	
	auto custom_tabs_button_layout = new QHBoxLayout();
	custom_tab_add = new QPushButton(QIcon(QStringLiteral(":/images/plus.png")), QString{});
	custom_tabs_button_layout->addWidget(custom_tab_add);
	custom_tab_remove = new QPushButton(QIcon(QStringLiteral(":/images/minus.png")), QString{});
	custom_tab_remove->setEnabled(false);
	custom_tabs_button_layout->addWidget(custom_tab_remove);
	custom_tabs_button_layout->addStretch(1);
	
	ocad_compat_layout->addRow(custom_tabs_button_layout);
	
	
	updateGeneralContents();
	updateFramingContents();
	updateCompatibilityContents();
	
	connect(oriented_to_north, &QAbstractButton::clicked, this, &TextSymbolSettings::orientedToNorthClicked);
	connect(font_edit, &QFontComboBox::currentFontChanged, this, &TextSymbolSettings::fontChanged);
	connect(font_size_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::fontSizeChanged);
	connect(letter_edit, &QLineEdit::textEdited, this, &TextSymbolSettings::letterSizeChanged);
	connect(letter_size_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::letterSizeChanged);
	connect(color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextSymbolSettings::colorChanged);
	connect(bold_check, &QAbstractButton::clicked, this, &TextSymbolSettings::checkToggled);
	connect(italic_check, &QAbstractButton::clicked, this, &TextSymbolSettings::checkToggled);
	connect(underline_check, &QAbstractButton::clicked, this, &TextSymbolSettings::checkToggled);
	connect(line_spacing_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::spacingChanged);
	connect(paragraph_spacing_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::spacingChanged);
	connect(character_spacing_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::spacingChanged);
	connect(kerning_check, &QAbstractButton::clicked, this, &TextSymbolSettings::checkToggled);
	connect(icon_text_edit, &QLineEdit::textEdited, this, &TextSymbolSettings::iconTextEdited);
	connect(framing_check, &QAbstractButton::clicked, this, &TextSymbolSettings::framingCheckClicked);
	connect(ocad_compat_check, &QAbstractButton::clicked, this, &TextSymbolSettings::ocadCompatibilityButtonClicked);
	
	connect(framing_color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextSymbolSettings::framingColorChanged);
	connect(framing_line_radio, &QAbstractButton::clicked, this, &TextSymbolSettings::framingModeChanged);
	connect(framing_line_half_width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::framingSettingChanged);
	connect(framing_shadow_radio, &QAbstractButton::clicked, this, &TextSymbolSettings::framingModeChanged);
	connect(framing_shadow_x_offset_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::framingSettingChanged);
	connect(framing_shadow_y_offset_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::framingSettingChanged);
	
	connect(line_below_check, &QAbstractButton::clicked, this, &TextSymbolSettings::lineBelowCheckClicked);
	connect(line_below_color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextSymbolSettings::lineBelowSettingChanged);
	connect(line_below_width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::lineBelowSettingChanged);
	connect(line_below_distance_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TextSymbolSettings::lineBelowSettingChanged);
	connect(custom_tab_list, &QListWidget::currentRowChanged, this, &TextSymbolSettings::customTabRowChanged);
	connect(custom_tab_add, &QAbstractButton::clicked, this, &TextSymbolSettings::addCustomTabClicked);
	connect(custom_tab_remove, &QAbstractButton::clicked, this, &TextSymbolSettings::removeCustomTabClicked);
}

TextSymbolSettings::~TextSymbolSettings() = default;



void TextSymbolSettings::orientedToNorthClicked(bool checked)
{
	symbol->setRotatable(!checked);
	emit propertiesModified();
}

void TextSymbolSettings::fontChanged(const QFont& font)
{
	if (!react_to_changes)
		return;
	
	symbol->font_family = font.family();
	symbol->updateQFont();
	updateLetterSizeEdit();
	emit propertiesModified();
}

void TextSymbolSettings::fontSizeChanged(double value)
{
	if (!react_to_changes)
		return;
	
	symbol->font_size = qRound(value * 25400.0 / 72.0);
	symbol->updateQFont();
	updateLetterSizeEdit();
	emit propertiesModified();
}

void TextSymbolSettings::updateFontSizeEdit()
{
	QSignalBlocker block { font_size_edit };
	font_size_edit->setValue(0.01 * qRound(symbol->font_size * 7.2 / 25.4));
}

void TextSymbolSettings::letterSizeChanged()
{
	auto letter_height = calculateLetterHeight();
	if (letter_height > 0.0)
	{
		symbol->font_size = qRound(1000.0 * letter_size_edit->value() / letter_height);
		symbol->updateQFont();
		updateFontSizeEdit();
		emit propertiesModified();
	}
}

qreal TextSymbolSettings::calculateLetterHeight() const
{
	QPainterPath path;
	path.addText(0.0, 0.0, symbol->getQFont(), letter_edit->text());
	return path.boundingRect().height() / TextSymbol::internal_point_size;
}

void TextSymbolSettings::updateLetterSizeEdit()
{
	auto letter_height = calculateLetterHeight();
	if (letter_height > 0.0)
	{
		QSignalBlocker block { letter_size_edit };
		letter_size_edit->setValue(0.01 * qRound(symbol->font_size * letter_height / 10.0));
	}
}

void TextSymbolSettings::colorChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->color = color_edit->color();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::checkToggled(bool checked)
{
	Q_UNUSED(checked);
	
	if (!react_to_changes)
		return;
	
	symbol->bold = bold_check->isChecked();
	symbol->italic = italic_check->isChecked();
	symbol->underline = underline_check->isChecked();
	symbol->kerning = kerning_check->isChecked();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::spacingChanged(double value)
{
	Q_UNUSED(value);
	
	if (!react_to_changes)
		return;
	
	symbol->line_spacing = 0.01 * line_spacing_edit->value();
	symbol->paragraph_spacing = qRound(1000.0 * paragraph_spacing_edit->value());
	symbol->character_spacing = 0.01 * character_spacing_edit->value();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::iconTextEdited(const QString& text)
{
	if (!react_to_changes)
		return;
	
	symbol->icon_text = text;
	emit propertiesModified();
}

void TextSymbolSettings::framingColorChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->framing_color = framing_color_edit->color();
	updateFramingContents();
	emit propertiesModified();
}

void TextSymbolSettings::framingModeChanged()
{
	if (!react_to_changes)
		return;
	
	if (framing_line_radio->isChecked())
		symbol->framing_mode = TextSymbol::LineFraming;
	else if (framing_shadow_radio->isChecked())
		symbol->framing_mode = TextSymbol::ShadowFraming;
	updateFramingContents();
	emit propertiesModified();
}

void TextSymbolSettings::framingSettingChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->framing_line_half_width = qRound(1000.0 * framing_line_half_width_edit->value());
	symbol->framing_shadow_x_offset = qRound(1000.0 * framing_shadow_x_offset_edit->value());
	symbol->framing_shadow_y_offset = qRound(-1000.0 * framing_shadow_y_offset_edit->value());
	emit propertiesModified();
}

void TextSymbolSettings::framingCheckClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	setTabEnabled(indexOf(framing_widget), checked);
	symbol->framing = checked;
	emit propertiesModified();
}

void TextSymbolSettings::ocadCompatibilityButtonClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	setTabEnabled(indexOf(ocad_compat_widget), checked);
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::lineBelowCheckClicked(bool checked)
{
	if (!react_to_changes)
		return;
	
	symbol->line_below = checked;
	if (checked)
	{
		// Set defaults such that the line becomes visible
		if (symbol->line_below_width == 0)
			line_below_width_edit->setValue(0.1);
		if (!symbol->line_below_color)
			line_below_color_edit->setCurrentIndex(1);
	}
	
	symbol->updateQFont();
	emit propertiesModified();
	
	line_below_color_edit->setEnabled(checked);
	line_below_width_edit->setEnabled(checked);
	line_below_distance_edit->setEnabled(checked);
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::lineBelowSettingChanged()
{
	if (!react_to_changes)
		return;
	
	symbol->line_below_color = line_below_color_edit->color();
	symbol->line_below_width = qRound(1000.0 * line_below_width_edit->value());
	symbol->line_below_distance = qRound(1000.0 * line_below_distance_edit->value());
	symbol->updateQFont();
	emit propertiesModified();
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::customTabRowChanged(int row)
{
	custom_tab_remove->setEnabled(row >= 0);
}

void TextSymbolSettings::addCustomTabClicked()
{
	bool ok = false;
	// FIXME: Unit of measurement display in unusual way
	double position = QInputDialog::getDouble(dialog, 
	                                          tr("Add custom tabulator"),
	                                          QStringLiteral("%1 (%2)").arg(tr("Position:"), tr("mm")),
	                                          0, 0, 999999, 3, &ok);
	if (ok)
	{
		int int_position = qRound(1000 * position);
		
		int row = symbol->getNumCustomTabs();
		for (int i = 0; i < symbol->getNumCustomTabs(); ++i)
		{
			if (int_position == symbol->custom_tabs[i])
				return;	// do not add a double position
			else if (int_position < symbol->custom_tabs[i])
			{
				row = i;
				break;
			}
		}
		
		custom_tab_list->insertItem(row, locale().toString(position, 'g', 3) + QLatin1Char(' ') + tr("mm"));
		custom_tab_list->setCurrentRow(row);
		symbol->custom_tabs.insert(symbol->custom_tabs.begin() + row, int_position);
		emit propertiesModified();
		updateCompatibilityCheckEnabled();
	}
}

void TextSymbolSettings::removeCustomTabClicked()
{
	if (auto item = custom_tab_list->currentItem())
	{
		int row = custom_tab_list->row(item);
		delete custom_tab_list->item(row);
		custom_tab_list->setCurrentRow(row - 1);
		symbol->custom_tabs.erase(symbol->custom_tabs.begin() + row);
		emit propertiesModified();
		updateCompatibilityCheckEnabled();
	}
}

void TextSymbolSettings::updateGeneralContents()
{
	react_to_changes = false;
	oriented_to_north->setChecked(!symbol->isRotatable());
	font_edit->setCurrentFont(QFont(symbol->font_family));
	updateFontSizeEdit();
	updateLetterSizeEdit();
	color_edit->setColor(symbol->color);
	bold_check->setChecked(symbol->bold);
	italic_check->setChecked(symbol->italic);
	underline_check->setChecked(symbol->underline);
	line_spacing_edit->setValue(100.0 * symbol->line_spacing);
	paragraph_spacing_edit->setValue(0.001 * symbol->paragraph_spacing);
	character_spacing_edit->setValue(100 * symbol->character_spacing);
	kerning_check->setChecked(symbol->kerning);
	icon_text_edit->setText(symbol->getIconText());
	framing_check->setChecked(symbol->framing);
	ocad_compat_check->setChecked(symbol->line_below || symbol->getNumCustomTabs() > 0);
	react_to_changes = true;
	
	framingCheckClicked(framing_check->isChecked());
	ocadCompatibilityButtonClicked(ocad_compat_check->isChecked());
}

void TextSymbolSettings::updateCompatibilityCheckEnabled()
{
	ocad_compat_check->setEnabled(!symbol->line_below && symbol->getNumCustomTabs() == 0);
}

void TextSymbolSettings::updateFramingContents()
{
	react_to_changes = false;
	
	framing_color_edit->setColor(symbol->framing_color);
	framing_line_radio->setChecked(symbol->framing_mode == TextSymbol::LineFraming);
	framing_line_half_width_edit->setValue(0.001 * symbol->framing_line_half_width);
	framing_shadow_radio->setChecked(symbol->framing_mode == TextSymbol::ShadowFraming);
	framing_shadow_x_offset_edit->setValue(0.001 * symbol->framing_shadow_x_offset);
	framing_shadow_y_offset_edit->setValue(-0.001 * symbol->framing_shadow_y_offset);
	
	
	framing_line_radio->setEnabled(symbol->framing_color);
	framing_shadow_radio->setEnabled(symbol->framing_color);
	if (!symbol->framing_color)
	{
		framing_line_half_width_edit->setEnabled(false);
		framing_shadow_x_offset_edit->setEnabled(false);
		framing_shadow_y_offset_edit->setEnabled(false);
	}
	else
	{
		framing_line_half_width_edit->setEnabled(symbol->framing_mode == TextSymbol::LineFraming);
		framing_shadow_x_offset_edit->setEnabled(symbol->framing_mode == TextSymbol::ShadowFraming);
		framing_shadow_y_offset_edit->setEnabled(symbol->framing_mode == TextSymbol::ShadowFraming);
	}
	
	react_to_changes = true;
}

void TextSymbolSettings::updateCompatibilityContents()
{
	react_to_changes = false;
	line_below_check->setChecked(symbol->line_below);
	line_below_width_edit->setValue(0.001 * symbol->line_below_width);
	line_below_color_edit->setColor(symbol->line_below_color);
	line_below_distance_edit->setValue(0.001 * symbol->line_below_distance);
	
	line_below_color_edit->setEnabled(symbol->line_below);
	line_below_width_edit->setEnabled(symbol->line_below);
	line_below_distance_edit->setEnabled(symbol->line_below);

	custom_tab_list->clear();
	for (int i = 0; i < symbol->getNumCustomTabs(); ++i)
		custom_tab_list->addItem(locale().toString(0.001 * symbol->getCustomTab(i), 'g', 3) + QLatin1Char(' ') + tr("mm"));
	
	react_to_changes = true;
}

void TextSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Text);
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	updateGeneralContents();
	updateFramingContents();
	updateCompatibilityContents();
}


}  // namespace OpenOrienteering
