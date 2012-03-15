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

const float TextSymbol::pt_in_mm = 0.2526f;
const float TextSymbol::internal_point_size = 64;

TextSymbol::TextSymbol() : Symbol(Symbol::Text), metrics(QFont())
{
	color = NULL;
	font_family = "Arial";
	ascent_size = 4 * 1000;
	bold = false;
	italic = false;
	underline = false;
	line_spacing = 1;
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
	new_text->ascent_size = ascent_size;
	new_text->bold = bold;
	new_text->italic = italic;
	new_text->underline = underline;
	new_text->line_spacing = line_spacing;
	new_text->updateQFont();
	return new_text;
}

void TextSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output)
{
	TextObject* text_object = reinterpret_cast<TextObject*>(object);
	
	if (!color)
		return;
	
	double anchor_x = coords[0].getX();
	double anchor_y = coords[0].getY();
	
	text_object->prepareLineInfos();
	output.push_back(new TextRenderable(this, text_object, anchor_x, anchor_y));
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
	ascent_size = qRound(factor * ascent_size);
	
	updateQFont();
}

void TextSymbol::updateQFont()
{
	qfont = QFont();
	qfont.setBold(bold);
	qfont.setItalic(italic);
	qfont.setUnderline(underline);
	qfont.setPointSizeF(internal_point_size);
	qfont.setFamily(font_family);
	#if (QT_VERSION >= 0x040800)
	qfont.setHintingPreference(QFont::PreferNoHinting);
	#endif
	
	qfont.setStyleStrategy(QFont::ForceOutline);

	metrics = QFontMetricsF(qfont);
	tab_interval = 8.0 * metrics.averageCharWidth();
}

void TextSymbol::saveImpl(QFile* file, Map* map)
{
	int temp = map->findColorIndex(color);
	file->write((const char*)&temp, sizeof(int));
	saveString(file, font_family);
	file->write((const char*)&ascent_size, sizeof(int));
	file->write((const char*)&bold, sizeof(bool));
	file->write((const char*)&italic, sizeof(bool));
	file->write((const char*)&underline, sizeof(bool));
	file->write((const char*)&line_spacing, sizeof(float));
}
bool TextSymbol::loadImpl(QFile* file, int version, Map* map)
{
	int temp;
	file->read((char*)&temp, sizeof(int));
	color = (temp >= 0) ? map->getColor(temp) : NULL;
	loadString(file, font_family);
	file->read((char*)&ascent_size, sizeof(int));
	file->read((char*)&bold, sizeof(bool));
	file->read((char*)&italic, sizeof(bool));
	file->read((char*)&underline, sizeof(bool));
	file->read((char*)&line_spacing, sizeof(float));
	
	updateQFont();
	return true;
}

double TextSymbol::getNextTab(double pos) const
{
	double next_tab = (floor(pos / tab_interval) + 1.0) * tab_interval;
	if (next_tab <= pos)
		next_tab += tab_interval;
 	return next_tab;
}

// ### TextSymbolSettings ###

TextSymbolSettings::TextSymbolSettings(TextSymbol* symbol, Map* map, SymbolSettingDialog* parent): QGroupBox(tr("Text settings"), parent), symbol(symbol), dialog(parent)
{
	QLabel* font_label = new QLabel(tr("Font family:"));
	font_edit = new QFontComboBox();
	font_edit->setCurrentFont(QFont(symbol->font_family));
	
	QLabel* size_label = new QLabel(tr("Font size:"));
	size_edit = new QLineEdit(QString::number(0.001 * symbol->ascent_size));
	size_edit->setValidator(new DoubleValidator(0, 999999, size_edit));
	size_option_mm = new QPushButton(tr("mm"));
	size_option_pt = new QPushButton(tr("pt"));
	size_option_mm->setCheckable(true);
	size_option_pt->setCheckable(true);
	size_option_mm->setChecked(true);
	
	QHBoxLayout* size_layout = new QHBoxLayout();
	size_layout->setMargin(0);
	size_layout->setSpacing(0);
	size_layout->addWidget(size_edit);
	size_layout->addWidget(size_option_mm);
	size_layout->addWidget(size_option_pt);
	
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
	
	QHBoxLayout* line_spacing_layout = new QHBoxLayout();
	line_spacing_layout->setMargin(0);
	line_spacing_layout->setSpacing(0);
	line_spacing_layout->addWidget(line_spacing_edit);
	line_spacing_layout->addWidget(line_spacing_label_2);
	
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
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addLayout(upper_layout);
	layout->addWidget(bold_check);
	layout->addWidget(italic_check);
	layout->addWidget(underline_check);
	layout->addLayout(lower_layout);
	setLayout(layout);
	
	connect(font_edit, SIGNAL(currentFontChanged(QFont)), this, SLOT(fontChanged(QFont)));
	connect(size_edit, SIGNAL(textEdited(QString)), this, SLOT(sizeChanged(QString)));
	connect(size_option_mm, SIGNAL(clicked(bool)), this, SLOT(sizeOptionMMClicked(bool)));
	connect(size_option_pt, SIGNAL(clicked(bool)), this, SLOT(sizeOptionPTClicked(bool)));
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(bold_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(italic_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(underline_check, SIGNAL(clicked(bool)), this, SLOT(checkToggled(bool)));
	connect(line_spacing_edit, SIGNAL(textEdited(QString)), this, SLOT(lineSpacingChanged(QString)));
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
	if (size_option_pt->isChecked())
		new_size *= TextSymbol::pt_in_mm;
	symbol->ascent_size = qRound(1000 * new_size);
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::sizeOptionMMClicked(bool checked)
{
	if (checked)
		size_edit->setText(QString::number(0.001*symbol->ascent_size));
	else
		size_edit->setText(QString::number(0.001*symbol->ascent_size / TextSymbol::pt_in_mm));
	
	size_option_pt->setChecked(!checked);
}
void TextSymbolSettings::sizeOptionPTClicked(bool checked)
{
	if (checked)
		size_edit->setText(QString::number(0.001*symbol->ascent_size / TextSymbol::pt_in_mm));
	else
		size_edit->setText(QString::number(0.001*symbol->ascent_size));
	
	size_option_mm->setChecked(!checked);
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
	symbol->updateQFont();
	dialog->updatePreview();
}
void TextSymbolSettings::lineSpacingChanged(QString text)
{
	symbol->line_spacing = 0.01f * text.toFloat();
	symbol->updateQFont();
	dialog->updatePreview();
}

#include "symbol_text.moc"
