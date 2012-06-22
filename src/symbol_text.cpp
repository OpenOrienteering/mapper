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


#include "symbol_text.h"

#include <QtGui>
#include <QIODevice>

#include "map.h"
#include "map_color.h"
#include "object_text.h"
#include "renderable_implementation.h"
#include "symbol_area.h"
#include "symbol_setting_dialog.h"
#include "util.h"
#include "util_gui.h"

const float TextSymbol::internal_point_size = 256;

TextSymbol::TextSymbol() : Symbol(Symbol::Text), metrics(QFont())
{
	color = NULL;
	font_family = "Arial";
	font_size = 4 * 1000;
	bold = false;
	italic = false;
	underline = false;
	line_spacing = 1;
	paragraph_spacing = 0;
	character_spacing = 0;
	kerning = true;
	icon_text = "";
	line_below = false;
	line_below_color = NULL;
	line_below_width = 0;
	line_below_distance = 0;
}

TextSymbol::~TextSymbol()
{
}

Symbol* TextSymbol::duplicate(const QHash<MapColor*, MapColor*>* color_map) const
{
	TextSymbol* new_text = new TextSymbol();
	new_text->duplicateImplCommon(this);
	new_text->color = color_map ? color_map->value(color) : color;
	new_text->font_family = font_family;
	new_text->font_size = font_size;
	new_text->bold = bold;
	new_text->italic = italic;
	new_text->underline = underline;
	new_text->line_spacing = line_spacing;
	new_text->paragraph_spacing = paragraph_spacing;
	new_text->character_spacing = character_spacing;
	new_text->kerning = kerning;
	new_text->icon_text = icon_text;
	new_text->line_below = line_below;
	new_text->line_below_color = line_below_color;
	new_text->line_below_width = line_below_width;
	new_text->line_below_distance = line_below_distance;
	new_text->custom_tabs = custom_tabs;
	new_text->updateQFont();
	return new_text;
}

void TextSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output)
{
	TextObject* text_object = reinterpret_cast<TextObject*>(object);
	
	double anchor_x = coords[0].getX();
	double anchor_y = coords[0].getY();
	
	text_object->prepareLineInfos();
	if (color)
		output.insertRenderable(new TextRenderable(this, text_object, anchor_x, anchor_y));
	if (line_below && line_below_color && line_below_width > 0)
		createLineBelowRenderables(object, output);
}

void TextSymbol::createLineBelowRenderables(Object* object, ObjectRenderables& output)
{
	TextObject* text_object = reinterpret_cast<TextObject*>(object);
	double scale_factor = calculateInternalScaling();
	AreaSymbol area_symbol;
	area_symbol.setColor(line_below_color);
	MapCoordVectorF line_coords;
	line_coords.reserve(text_object->getNumLines() * 4);
	
	QTransform transform = text_object->calcTextToMapTransform();
	
	for (int i = 0; i < text_object->getNumLines(); ++i)
	{
		TextObjectLineInfo* line_info = text_object->getLineInfo(i);
		if (!line_info->paragraph_end)
			continue;
		
		double line_below_x0;
		double line_below_x1;
		if (text_object->hasSingleAnchor())
		{
			line_below_x0 = line_info->line_x;
			line_below_x1 = line_below_x0 + line_info->width;
		}
		else
		{
			double box_width = text_object->getBoxWidth() * scale_factor;
			line_below_x0 = -0.5 * box_width;
			line_below_x1 = line_below_x0 + box_width;
		}
		double line_below_y0 = line_info->line_y + getLineBelowDistance() * scale_factor;
		double line_below_y1 = line_below_y0 + getLineBelowWidth() * scale_factor;
		line_coords.push_back(MapCoordF(transform.map(MapCoordF(line_below_x0, line_below_y0).toQPointF())));
		line_coords.push_back(MapCoordF(transform.map(MapCoordF(line_below_x1,  line_below_y0).toQPointF())));
		line_coords.push_back(MapCoordF(transform.map(MapCoordF(line_below_x1,  line_below_y1).toQPointF())));
		line_coords.push_back(MapCoordF(transform.map(MapCoordF(line_below_x0, line_below_y1).toQPointF())));
	}
	
	if (line_coords.empty())
		return;
	
	MapCoord no_flags;
	MapCoord hole_flag;
	hole_flag.setHolePoint(true);
	MapCoordVector line_flags;
	line_flags.resize(line_coords.size());
	for (int i = 0; i < (int)line_coords.size(); ++i)
		line_flags[i] = ((i % 4 == 3) ? hole_flag : no_flags);
	
	output.insertRenderable(new AreaRenderable(&area_symbol, line_coords, line_flags, NULL));
}

void TextSymbol::colorDeleted(MapColor* color)
{
	if (color == this->color)
	{
		this->color = NULL;
		resetIcon();
	}
}

bool TextSymbol::containsColor(MapColor* color)
{
	return color == this->color;
}

void TextSymbol::scale(double factor)
{
	font_size = qRound(factor * font_size);
	
	updateQFont();
	
	resetIcon();
}

void TextSymbol::updateQFont()
{
	qfont = QFont();
	qfont.setBold(bold);
	qfont.setItalic(italic);
	qfont.setUnderline(underline);
	qfont.setPixelSize(internal_point_size);
	qfont.setFamily(font_family);
	#if (QT_VERSION >= 0x040800)
	qfont.setHintingPreference(QFont::PreferNoHinting);
	#endif
	qfont.setKerning(kerning);
	metrics = QFontMetricsF(qfont);
	qfont.setLetterSpacing(QFont::AbsoluteSpacing, metrics.width(" ") * character_spacing);
	
	qfont.setStyleStrategy(QFont::ForceOutline);

	metrics = QFontMetricsF(qfont);
	tab_interval = 8.0 * metrics.averageCharWidth();
}

void TextSymbol::saveImpl(QIODevice* file, Map* map)
{
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
	saveString(file, font_family);
	file->write((const char*)&font_size, sizeof(int));
	file->write((const char*)&bold, sizeof(bool));
	file->write((const char*)&italic, sizeof(bool));
	file->write((const char*)&underline, sizeof(bool));
	file->write((const char*)&line_spacing, sizeof(float));
	file->write((const char*)&paragraph_spacing, sizeof(double));
	file->write((const char*)&character_spacing, sizeof(double));
	file->write((const char*)&kerning, sizeof(bool));
	saveString(file, icon_text);
	file->write((const char*)&line_below, sizeof(bool));
	temp = map->findColorIndex(line_below_color);
	file->write((const char*)&temp, sizeof(int));
	file->write((const char*)&line_below_width, sizeof(int));
	file->write((const char*)&line_below_distance, sizeof(int));
	int num_custom_tabs = getNumCustomTabs();
	file->write((const char*)&num_custom_tabs, sizeof(int));
	for (int i = 0; i < num_custom_tabs; ++i)
		file->write((const char*)&custom_tabs[i], sizeof(int));
}

bool TextSymbol::loadImpl(QIODevice* file, int version, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	loadString(file, font_family);
	file->read((char*)&font_size, sizeof(int));
	file->read((char*)&bold, sizeof(bool));
	file->read((char*)&italic, sizeof(bool));
	file->read((char*)&underline, sizeof(bool));
	file->read((char*)&line_spacing, sizeof(float));
	if (version >= 13)
		file->read((char*)&paragraph_spacing, sizeof(double));
	if (version >= 14)
		file->read((char*)&character_spacing, sizeof(double));
	if (version >= 12)
		file->read((char*)&kerning, sizeof(bool));
	if (version >= 19)
		loadString(file, icon_text);
	if (version >= 13)
	{
		file->read((char*)&line_below, sizeof(bool));
		file->read((char*)&temp, sizeof(int));
		line_below_color = (temp >= 0) ? map->getColor(temp) : NULL;
		file->read((char*)&line_below_width, sizeof(int));
		file->read((char*)&line_below_distance, sizeof(int));
		int num_custom_tabs;
		file->read((char*)&num_custom_tabs, sizeof(int));
		custom_tabs.resize(num_custom_tabs);
		for (int i = 0; i < num_custom_tabs; ++i)
			file->read((char*)&custom_tabs[i], sizeof(int));
	}
	
	updateQFont();
	return true;
}

bool TextSymbol::equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity)
{
	TextSymbol* text = static_cast<TextSymbol*>(other);
	
	if (!colorEquals(color, text->color))
		return false;
	if (font_family.compare(text->font_family, Qt::CaseInsensitive) != 0)
		return false;
	if (font_size != text->font_size ||
		bold != text->bold ||
		italic != text->italic ||
		underline != text->underline ||
		line_spacing != text->line_spacing ||
		paragraph_spacing != text->paragraph_spacing ||
		character_spacing != text->character_spacing ||
		kerning != text->kerning ||
		line_below != text->line_below)
		return false;
	if (line_below)
	{
		if (!colorEquals(line_below_color, text->line_below_color))
			return false;
		if (line_below_width != text->line_below_width ||
			line_below_distance != text->line_below_distance)
			return false;
	}
	if (tab_interval != text->tab_interval)
		return false;
	if (custom_tabs.size() != text->custom_tabs.size())
		return false;
	for (size_t i = 0, end = custom_tabs.size(); i < end; ++i)
	{
		if (custom_tabs[i] != text->custom_tabs[i])
			return false;
	}
	
	return true;
}

QString TextSymbol::getIconText() const
{
	if (icon_text.isEmpty())
		return QObject::tr("A", "First capital letter of the local alphabet");
	return icon_text;
}

double TextSymbol::getNextTab(double pos) const
{
	if (!custom_tabs.empty())
	{
		double scaling = calculateInternalScaling();
		double map_pos = pos / scaling;
		for (int i = 0; i < (int)custom_tabs.size(); ++i)
		{
			if (0.001 * custom_tabs[i] > map_pos)
				return scaling * 0.001 * custom_tabs[i];
		}
		
		// After the given positions, OCAD repeats the distance between the last two tab positions
		double custom_tab_interval = (custom_tabs.size() > 1) ?
									   (custom_tabs[custom_tabs.size() - 1] - custom_tabs[custom_tabs.size() - 2]) :
									   custom_tabs[0];
		custom_tab_interval *= 0.001;
		return scaling * (0.001 * custom_tabs[custom_tabs.size() - 1] + (floor((map_pos - 0.001 * custom_tabs[custom_tabs.size() - 1]) / custom_tab_interval) + 1.0) * custom_tab_interval);
	}
	
	double next_tab = (floor(pos / tab_interval) + 1.0) * tab_interval;
	assert(next_tab > pos);
 	return next_tab;
}

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
	Map* map = dialog->getPreviewMap();
	react_to_changes = true;
	
	QWidget* text_tab = new QWidget();
	addPropertiesGroup(tr("Text settings"), text_tab);
	
	QFormLayout* layout = new QFormLayout();
	text_tab->setLayout(layout);
	
	font_edit = new QFontComboBox();
	layout->addRow(tr("Font family:"), font_edit);
	
	QHBoxLayout* size_layout = new QHBoxLayout();
	size_layout->setMargin(0);
	size_layout->setSpacing(0);
	
	size_edit = Util::SpinBox::create(1, 0.0, 999999.9);
	size_layout->addWidget(size_edit);
	
	size_unit_combo = new QComboBox();
	size_unit_combo->addItem(tr("mm"), QVariant((int)SizeInMM));
	size_unit_combo->addItem(tr("pt"), QVariant((int)SizeInPT));
	size_unit_combo->setCurrentIndex(0);
	size_layout->addWidget(size_unit_combo);
	
	size_determine_button = new QPushButton(tr("Determine size..."));
	size_layout->addSpacing(8);
	size_layout->addWidget(size_determine_button);
	
	layout->addRow(tr("Font size:"), size_layout);
	
	color_edit = new ColorDropDown(map, symbol->getColor());
	layout->addRow(tr("Text color:"), color_edit);
	
	QVBoxLayout* text_style_layout = new QVBoxLayout();
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
	layout->addRow("", kerning_check);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	icon_text_edit = new QLineEdit();
	icon_text_edit->setMaxLength(3);
	layout->addRow(tr("Symbol icon text:"), icon_text_edit);
	
	layout->addItem(Util::SpacerItem::create(this));
	
	ocad_compat_check = new QCheckBox(tr("OCAD compatibility settings"));
	layout->addRow(ocad_compat_check);
	
	
	ocad_compat_widget = new QWidget();
	addPropertiesGroup(tr("OCAD compatibility"), ocad_compat_widget);
	
	QFormLayout* ocad_compat_layout = new QFormLayout();
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
	
	QHBoxLayout* custom_tabs_button_layout = new QHBoxLayout();
	custom_tab_add = new QPushButton(QIcon(":/images/plus.png"), "");
	custom_tabs_button_layout->addWidget(custom_tab_add);
	custom_tab_remove = new QPushButton(QIcon(":/images/minus.png"), "");
	custom_tabs_button_layout->addWidget(custom_tab_remove);
	custom_tabs_button_layout->addStretch(1);
	
	ocad_compat_layout->addRow(custom_tabs_button_layout);
	
	
	updateGeneralContents();
	updateCompatibilityContents();
	
	
	connect(font_edit, SIGNAL(currentFontChanged(QFont)), this, SLOT(fontChanged(QFont)));
	connect(size_edit, SIGNAL(valueChanged(double)), this, SLOT(sizeChanged(double)));
	connect(size_unit_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(sizeUnitChanged(int)));
	connect(size_determine_button, SIGNAL(clicked(bool)), this, SLOT(determineSizeClicked()));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(bold_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(italic_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(underline_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(line_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(paragraph_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(character_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(spacingChanged(double)));
	connect(kerning_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(icon_text_edit, SIGNAL(textEdited(QString)), this, SLOT(iconTextEdited(QString)));
	connect(ocad_compat_check, SIGNAL(clicked(bool)), this, SLOT(ocadCompatibilityButtonClicked(bool)));
	
	connect(line_below_check, SIGNAL(clicked(bool)), this, SLOT(lineBelowCheckClicked(bool)));
	connect(line_below_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_width_edit, SIGNAL(valueChanged(double)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_distance_edit, SIGNAL(valueChanged(double)), this, SLOT(lineBelowSettingChanged()));
	connect(custom_tab_list, SIGNAL(currentRowChanged(int)), this, SLOT(customTabRowChanged(int)));
	connect(custom_tab_add, SIGNAL(clicked(bool)), this, SLOT(addCustomTabClicked()));
	connect(custom_tab_remove, SIGNAL(clicked(bool)), this, SLOT(removeCustomTabClicked()));
}

TextSymbolSettings::~TextSymbolSettings()
{
}

void TextSymbolSettings::fontChanged(QFont font)
{
	if (!react_to_changes)
		return;
	
	symbol->font_family = font.family();
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::sizeChanged(double value)
{
	if (!react_to_changes)
		return;
	
	SizeUnit unit = (SizeUnit)size_unit_combo->itemData(size_unit_combo->currentIndex()).toInt();
	if (unit == SizeInMM)
		symbol->font_size = qRound(1000 * value);
	else if (unit == SizeInPT)
		symbol->font_size = qRound(1000 * value * (1 / 72.0 * 25.4));
	
	symbol->updateQFont();
	emit propertiesModified();
}

void TextSymbolSettings::sizeUnitChanged(int index)
{
	if (!react_to_changes)
		return;
	
	updateSizeEdit();
}

void TextSymbolSettings::updateSizeEdit()
{
	SizeUnit unit = (SizeUnit)size_unit_combo->itemData(size_unit_combo->currentIndex()).toInt();
	
	react_to_changes = false;
	if (unit == SizeInMM)
		size_edit->setValue(0.001 * symbol->font_size);
	else if (unit == SizeInPT)
		size_edit->setValue(0.001 * symbol->font_size / (1 / 72.0 * 25.4));
	react_to_changes = true;
}

void TextSymbolSettings::determineSizeClicked()
{
	DetermineFontSizeDialog modal_dialog(this, symbol);
	modal_dialog.setWindowModality(Qt::WindowModal);
	if (modal_dialog.exec() == QDialog::Accepted)
	{
		updateSizeEdit();
		symbol->updateQFont();
		emit propertiesModified();
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
	if (!react_to_changes)
		return;
	
	symbol->line_spacing = 0.01 * line_spacing_edit->value();
	symbol->paragraph_spacing = qRound(1000.0 * paragraph_spacing_edit->value());
	symbol->character_spacing = 0.01f * character_spacing_edit->value();
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
		if (symbol->line_below_color == NULL)
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
	double position = QInputDialog::getDouble(dialog, tr("Add custom tabulator"), QString("%1 (%2)").arg(tr("Position:")).arg(tr("mm")), 0, 0, 999999, 3, &ok);
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
		
		custom_tab_list->insertItem(row, locale().toString(position, 'g', 3) % ' ' % tr("mm"));
		custom_tab_list->setCurrentRow(row);
		symbol->custom_tabs.insert(symbol->custom_tabs.begin() + row, int_position);
		emit propertiesModified();
		updateCompatibilityCheckEnabled();
	}
}

void TextSymbolSettings::removeCustomTabClicked()
{
	int row = custom_tab_list->row(custom_tab_list->currentItem());
	delete custom_tab_list->item(row);
	custom_tab_list->setCurrentRow(row - 1);
	symbol->custom_tabs.erase(symbol->custom_tabs.begin() + row);
	emit propertiesModified();
	updateCompatibilityCheckEnabled();
}

void TextSymbolSettings::updateGeneralContents()
{
	react_to_changes = false;
	font_edit->setCurrentFont(QFont(symbol->font_family));
	updateSizeEdit();
	bold_check->setChecked(symbol->bold);
	italic_check->setChecked(symbol->italic);
	underline_check->setChecked(symbol->underline);
	line_spacing_edit->setValue(100.0 * symbol->line_spacing);
	paragraph_spacing_edit->setValue(0.001 * symbol->paragraph_spacing);
	character_spacing_edit->setValue(100 * symbol->character_spacing);
	kerning_check->setChecked(symbol->kerning);
	icon_text_edit->setText(symbol->getIconText());
	ocad_compat_check->setChecked(symbol->line_below || symbol->getNumCustomTabs() > 0);
	react_to_changes = true;
	
	ocadCompatibilityButtonClicked(ocad_compat_check->isChecked());
}

void TextSymbolSettings::updateCompatibilityCheckEnabled()
{
	ocad_compat_check->setEnabled(!symbol->line_below && symbol->getNumCustomTabs() == 0);
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
		custom_tab_list->addItem(locale().toString(0.001 * symbol->getCustomTab(i), 'g', 3) % ' ' % tr("mm"));
	
	react_to_changes = true;
}

void TextSymbolSettings::reset(Symbol* symbol)
{
	assert(symbol->getType() == Symbol::Text);
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<TextSymbol*>(symbol);
	
	updateGeneralContents();
	updateCompatibilityContents();
}


// ### DetermineFontSizeDialog ###

DetermineFontSizeDialog::DetermineFontSizeDialog(QWidget* parent, TextSymbol* symbol) 
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), 
  symbol(symbol)
{
	setWindowTitle(tr("Determine font size"));
	
	QFormLayout *form_layout = new QFormLayout();
	
	QLabel* explanation_label = new QLabel(tr("This dialog allows to choose a font size which results in a given exact height for a specific letter."));
	explanation_label->setWordWrap(true);
	form_layout->addRow(explanation_label);
	
	character_edit = new QLineEdit(tr("A"));
	character_edit->setMaxLength(1);
	form_layout->addRow(tr("Letter:"), character_edit);

	size_edit = Util::SpinBox::create(1, 0.0, 999999.9, tr("mm"));
	size_edit->setValue(symbol->getFontSize());
	form_layout->addRow(tr("Height:"), size_edit);
	
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	ok_button = button_box->button(QDialogButtonBox::Ok);
	updateOkButton();
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(form_layout, 1);
	layout->addItem(Util::SpacerItem::create(this));
	layout->addWidget(button_box, 0);
	
	setLayout(layout);
	
	connect(character_edit, SIGNAL(textEdited(QString)), this, SLOT(updateOkButton()));
	connect(size_edit, SIGNAL(valueChanged(double)), this, SLOT(updateOkButton()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
}

void DetermineFontSizeDialog::updateOkButton()
{
	ok_button->setEnabled(!character_edit->text().isEmpty() && size_edit->value() > 0);
}

void DetermineFontSizeDialog::accept()
{
	QChar character = character_edit->text().at(0);
	double character_internal_height = symbol->getFontMetrics().tightBoundingRect(character).height();
	double character_height_at_size_one = character_internal_height / TextSymbol::internal_point_size;
	double desired_height = size_edit->value();
	
	symbol->font_size = qRound(1000.0 * desired_height / character_height_at_size_one);
	symbol->updateQFont();
	QDialog::accept();
}
