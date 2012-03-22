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
#include <QFile>

#include "symbol_setting_dialog.h"
#include "map_color.h"
#include "util.h"
#include "object_text.h"
#include "symbol_area.h"

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
	line_below = false;
	line_below_color = NULL;
	line_below_width = 0;
	line_below_distance = 0;
}
TextSymbol::~TextSymbol()
{
}
Symbol* TextSymbol::duplicate()
{
	TextSymbol* new_text = new TextSymbol();
	new_text->duplicateImplCommon(this);
	new_text->color = color;
	new_text->font_family = font_family;
	new_text->font_size = font_size;
	new_text->bold = bold;
	new_text->italic = italic;
	new_text->underline = underline;
	new_text->line_spacing = line_spacing;
	new_text->paragraph_spacing = paragraph_spacing;
	new_text->character_spacing = character_spacing;
	new_text->kerning = kerning;
	new_text->line_below = line_below;
	new_text->line_below_color = line_below_color;
	new_text->line_below_width = line_below_width;
	new_text->line_below_distance = line_below_distance;
	new_text->custom_tabs = custom_tabs;
	new_text->updateQFont();
	return new_text;
}

void TextSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	TextObject* text_object = reinterpret_cast<TextObject*>(object);
	
	double anchor_x = coords[0].getX();
	double anchor_y = coords[0].getY();
	
	text_object->prepareLineInfos();
	if (color)
		output.push_back(new TextRenderable(this, text_object, anchor_x, anchor_y));
	if (line_below && line_below_color && line_below_width > 0)
		createLineBelowRenderables(object, output);
}
void TextSymbol::createLineBelowRenderables(Object* object, RenderableVector& output)
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
	
	output.push_back(new AreaRenderable(&area_symbol, line_coords, line_flags, NULL));
}

void TextSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
	if (color == this->color)
	{
		this->color = NULL;
		getIcon(map, true);
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

void TextSymbol::saveImpl(QFile* file, Map* map)
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
bool TextSymbol::loadImpl(QFile* file, int version, Map* map)
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

// ### TextSymbolSettings ###

TextSymbolSettings::TextSymbolSettings(TextSymbol* symbol, Map* map, SymbolSettingDialog* parent): QGroupBox(tr("Text settings"), parent), symbol(symbol), dialog(parent)
{
	QLabel* font_label = new QLabel(tr("Font family:"));
	font_edit = new QFontComboBox();
	font_edit->setCurrentFont(QFont(symbol->font_family));
	
	QLabel* size_label = new QLabel(tr("Font size:"));
	size_edit = new QLineEdit(QString::number(0.001 * symbol->font_size));
	size_edit->setValidator(new DoubleValidator(0, 999999, size_edit));
	size_determine_button = new QPushButton(tr("Determine size..."));
	
	QHBoxLayout* size_layout = new QHBoxLayout();
	size_layout->setMargin(0);
	size_layout->setSpacing(0);
	size_layout->addWidget(size_edit);
	size_layout->addWidget(size_determine_button);
	
	QLabel* color_label = new QLabel(tr("Text color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	bold_check = new QCheckBox(tr("bold"));
	bold_check->setChecked(symbol->bold);
	italic_check = new QCheckBox(tr("italic"));
	italic_check->setChecked(symbol->italic);
	underline_check = new QCheckBox(tr("underlined"));
	underline_check->setChecked(symbol->underline);
	
	QLabel* line_spacing_label = new QLabel(tr("Line spacing:"));
	line_spacing_edit = new QLineEdit(QString::number(100 * symbol->line_spacing));
	line_spacing_edit->setValidator(new DoubleValidator(0, 999999, line_spacing_edit));
	QLabel* line_spacing_label_2 = new QLabel("%");
	
	QLabel* paragraph_spacing_label = new QLabel(tr("Paragraph spacing [mm]:"));
	paragraph_spacing_edit = new QLineEdit(QString::number(0.001 * symbol->paragraph_spacing));
	paragraph_spacing_edit->setValidator(new DoubleValidator(-999999, 999999, paragraph_spacing_edit));
	
	QLabel* character_spacing_label = new QLabel(tr("Character spacing [percentage of space character]:"));
	character_spacing_edit = new QLineEdit(QString::number(100 * symbol->character_spacing));
	character_spacing_edit->setValidator(new DoubleValidator(-999999, 999999, character_spacing_edit));
	QLabel* character_spacing_label_2 = new QLabel("%");
	
	kerning_check = new QCheckBox(tr("use kerning"));
	kerning_check->setChecked(symbol->kerning);
	
	ocad_compat_button = new QPushButton(tr("Show OCAD compatibility settings"));
	ocad_compat_widget = new QWidget();
	if (symbol->line_below || symbol->getNumCustomTabs() > 0)
		ocad_compat_button->hide();
	else
		ocad_compat_widget->hide();
	
	QLabel* line_below_label = new QLabel("<br/><b>" + tr("Line below paragraphs") + "</b>");
	line_below_check = new QCheckBox(tr("Enable"));
	line_below_check->setChecked(symbol->line_below);
	line_below_color_edit = new ColorDropDown(map, symbol->line_below_color);
	line_below_width_label = new QLabel(tr("Width:"));
	line_below_width_edit = new QLineEdit(QString::number(0.001 * symbol->line_below_width));
	line_below_width_edit->setValidator(new DoubleValidator(0, 999999, line_below_width_edit));
	line_below_distance_label = new QLabel(tr("Distance from baseline:"));
	line_below_distance_edit = new QLineEdit(QString::number(0.001 * symbol->line_below_distance));
	line_below_distance_edit->setValidator(new DoubleValidator(0, 999999, line_below_distance_edit));
	if (!symbol->line_below)
	{
		line_below_color_edit->hide();
		line_below_width_label->hide();
		line_below_width_edit->hide();
		line_below_distance_label->hide();
		line_below_distance_edit->hide();
	}
	
	QLabel* custom_tabs_label = new QLabel("<br/><b>" + tr("Custom tabulator positions") + "</b>");
	custom_tab_list = new QListWidget();
	for (int i = 0; i < symbol->getNumCustomTabs(); ++i)
		custom_tab_list->addItem(QString::number(0.001 * symbol->getCustomTab(i)));
	custom_tab_add = new QPushButton(QIcon(":/images/plus.png"), "");
	custom_tab_remove = new QPushButton(QIcon(":/images/minus.png"), "");
	customTabRowChanged(custom_tab_list->row(custom_tab_list->currentItem()));
	
	QGridLayout* line_below_layout = new QGridLayout();
	line_below_layout->addWidget(line_below_width_label, 0, 0);
	line_below_layout->addWidget(line_below_width_edit, 0, 1);
	line_below_layout->addWidget(line_below_distance_label, 1, 0);
	line_below_layout->addWidget(line_below_distance_edit, 1, 1);
	
	QHBoxLayout* custom_tabs_button_layout = new QHBoxLayout();
	custom_tabs_button_layout->addWidget(custom_tab_add);
	custom_tabs_button_layout->addWidget(custom_tab_remove);
	custom_tabs_button_layout->addSpacing(1);
	
	QVBoxLayout* ocad_compat_layout = new QVBoxLayout();
	ocad_compat_layout->setMargin(0);
	ocad_compat_layout->setSpacing(0);
	ocad_compat_layout->addWidget(line_below_label);
	ocad_compat_layout->addWidget(line_below_check);
	ocad_compat_layout->addWidget(line_below_color_edit);
	ocad_compat_layout->addLayout(line_below_layout);
	ocad_compat_layout->addWidget(custom_tabs_label);
	ocad_compat_layout->addWidget(custom_tab_list);
	ocad_compat_layout->addLayout(custom_tabs_button_layout);
	ocad_compat_widget->setLayout(ocad_compat_layout);
	
	QHBoxLayout* line_spacing_layout = new QHBoxLayout();
	line_spacing_layout->setMargin(0);
	line_spacing_layout->setSpacing(0);
	line_spacing_layout->addWidget(line_spacing_edit);
	line_spacing_layout->addWidget(line_spacing_label_2);
	
	QHBoxLayout* character_spacing_layout = new QHBoxLayout();
	character_spacing_layout->setMargin(0);
	character_spacing_layout->setSpacing(0);
	character_spacing_layout->addWidget(character_spacing_edit);
	character_spacing_layout->addWidget(character_spacing_label_2);
	
	QGridLayout* upper_layout = new QGridLayout();
	upper_layout->addWidget(font_label, 0, 0);
	upper_layout->addWidget(font_edit, 0, 1);
	upper_layout->addWidget(size_label, 1, 0);
	upper_layout->addLayout(size_layout, 1, 1);
	upper_layout->addWidget(color_label, 2, 0);
	upper_layout->addWidget(color_edit, 2, 1);
	
	QGridLayout* lower_layout = new QGridLayout();
	lower_layout->addWidget(line_spacing_label, 0, 0);
	lower_layout->addLayout(line_spacing_layout, 0, 1);
	lower_layout->addWidget(paragraph_spacing_label, 1, 0);
	lower_layout->addWidget(paragraph_spacing_edit, 1, 1);
	lower_layout->addWidget(character_spacing_label, 2, 0);
	lower_layout->addLayout(character_spacing_layout, 2, 1);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addLayout(upper_layout);
	layout->addWidget(bold_check);
	layout->addWidget(italic_check);
	layout->addWidget(underline_check);
	layout->addLayout(lower_layout);
	layout->addWidget(kerning_check);
	layout->addWidget(ocad_compat_button);
	layout->addWidget(ocad_compat_widget);
	setLayout(layout);
	
	connect(font_edit, SIGNAL(currentFontChanged(QFont)), this, SLOT(fontChanged(QFont)));
	connect(size_edit, SIGNAL(textEdited(QString)), this, SLOT(sizeChanged(QString)));
	connect(size_determine_button, SIGNAL(clicked(bool)), this, SLOT(determineSizeClicked()));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(bold_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(italic_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(underline_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(line_spacing_edit, SIGNAL(textEdited(QString)), this, SLOT(spacingChanged(QString)));
	connect(paragraph_spacing_edit, SIGNAL(textEdited(QString)), this, SLOT(spacingChanged(QString)));
	connect(character_spacing_edit, SIGNAL(textEdited(QString)), this, SLOT(spacingChanged(QString)));
	connect(kerning_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(ocad_compat_button, SIGNAL(clicked(bool)), this, SLOT(ocadCompatibilityButtonClicked()));
	connect(line_below_check, SIGNAL(clicked(bool)), this, SLOT(lineBelowCheckClicked(bool)));
	connect(line_below_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_width_edit, SIGNAL(textEdited(QString)), this, SLOT(lineBelowSettingChanged()));
	connect(line_below_distance_edit, SIGNAL(textEdited(QString)), this, SLOT(lineBelowSettingChanged()));
	connect(custom_tab_list, SIGNAL(currentRowChanged(int)), this, SLOT(customTabRowChanged(int)));
	connect(custom_tab_add, SIGNAL(clicked(bool)), this, SLOT(addCustomTabClicked()));
	connect(custom_tab_remove, SIGNAL(clicked(bool)), this, SLOT(removeCustomTabClicked()));
}

void TextSymbolSettings::fontChanged(QFont font)
{
	symbol->font_family = font.family();
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::sizeChanged(QString text)
{
	float new_size = text.toFloat();
	symbol->font_size = qRound(1000 * new_size);
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::determineSizeClicked()
{
	DetermineFontSizeDialog modal_dialog(this, symbol);
	modal_dialog.setWindowModality(Qt::WindowModal);
	if (modal_dialog.exec() == QDialog::Accepted)
	{
		size_edit->setText(QString::number(0.001 * symbol->font_size));
		dialog->updatePreview();
	}
}
void TextSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::checkToggled(bool checked)
{
	symbol->bold = bold_check->isChecked();
	symbol->italic = italic_check->isChecked();
	symbol->underline = underline_check->isChecked();
	symbol->kerning = kerning_check->isChecked();
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::spacingChanged(QString text)
{
	symbol->line_spacing = 0.01f * line_spacing_edit->text().toFloat();
	symbol->paragraph_spacing = qRound(1000 * paragraph_spacing_edit->text().toFloat());
	symbol->character_spacing = 0.01f * character_spacing_edit->text().toFloat();
	symbol->updateQFont();
	dialog->updatePreview();
}

void TextSymbolSettings::ocadCompatibilityButtonClicked()
{
	ocad_compat_button->hide();
	ocad_compat_widget->show();
}
void TextSymbolSettings::lineBelowCheckClicked(bool checked)
{
	symbol->line_below = checked;
	symbol->updateQFont();
	dialog->updatePreview();
	
	line_below_color_edit->setVisible(checked);
	line_below_width_label->setVisible(checked);
	line_below_width_edit->setVisible(checked);
	line_below_distance_label->setVisible(checked);
	line_below_distance_edit->setVisible(checked);
}
void TextSymbolSettings::lineBelowSettingChanged()
{
	symbol->line_below_color = line_below_color_edit->color();
	symbol->line_below_width = qRound(1000 * line_below_width_edit->text().toFloat());
	symbol->line_below_distance = qRound(1000 * line_below_distance_edit->text().toFloat());
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::customTabRowChanged(int row)
{
	custom_tab_remove->setEnabled(row >= 0);
}
void TextSymbolSettings::addCustomTabClicked()
{
	bool ok = false;
	double position = QInputDialog::getDouble(dialog, tr("Add custom tabulator"), tr("Position [mm]:"), 0, 0, 999999, 3, &ok);
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
		
		custom_tab_list->insertItem(row, QString::number(position));
		custom_tab_list->setCurrentRow(row);
		symbol->custom_tabs.insert(symbol->custom_tabs.begin() + row, int_position);
	}
}
void TextSymbolSettings::removeCustomTabClicked()
{
	int row = custom_tab_list->row(custom_tab_list->currentItem());
	delete custom_tab_list->item(row);
	custom_tab_list->setCurrentRow(row - 1);
	symbol->custom_tabs.erase(symbol->custom_tabs.begin() + row);
}


// ### DetermineFontSizeDialog ###

DetermineFontSizeDialog::DetermineFontSizeDialog(QWidget* parent, TextSymbol* symbol) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), symbol(symbol)
{
	setWindowTitle(tr("Determine font size"));
	
	QLabel* explanation_label = new QLabel(tr("How big a letter in a font is depends on the design of the font.\nThis dialog allows to choose a font size which results in a given exact height for a specific letter."));
	explanation_label->setWordWrap(true);
	
	QLabel* character_label = new QLabel(tr("Letter:"));
	character_edit = new QLineEdit(tr("A"));
	character_edit->setMaxLength(1);
	
	QLabel* size_label = new QLabel(tr("Height:"));
	size_edit = new QLineEdit(QString::number(symbol->getFontSize()));
	size_edit->setValidator(new DoubleValidator(0, 999999));
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("OK"));
	ok_button->setDefault(true);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QGridLayout* settings_layout = new QGridLayout();
	settings_layout->addWidget(explanation_label, 0, 0, 1, 2);
	settings_layout->addWidget(character_label, 1, 0);
	settings_layout->addWidget(character_edit, 1, 1);
	settings_layout->addWidget(size_label, 2, 0);
	settings_layout->addWidget(size_edit, 2, 1);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(settings_layout);
	layout->addSpacing(16);
	layout->addLayout(buttons_layout);
	setLayout(layout);
	
	connect(character_edit, SIGNAL(textEdited(QString)), this, SLOT(characterEdited(QString)));
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(okClicked()));
}
void DetermineFontSizeDialog::characterEdited(QString text)
{
	ok_button->setEnabled(!text.isEmpty());
}
void DetermineFontSizeDialog::okClicked()
{
	QChar character = character_edit->text().at(0);
	double character_internal_height = symbol->getFontMetrics().tightBoundingRect(character).height();
	double character_height_at_size_one = character_internal_height / TextSymbol::internal_point_size;
	double desired_height = size_edit->text().toFloat();
	
	symbol->font_size = qRound(1000 * desired_height / character_height_at_size_one);
	symbol->updateQFont();
	accept();
}

#include "symbol_text.moc"
