/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "area_symbol_settings.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>

#include "core/map.h"
#include "core/symbols/point_symbol.h"
#include "gui/symbols/point_symbol_editor_widget.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "gui/util_gui.h"


// ### AreaSymbol ###

SymbolPropertiesWidget* AreaSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new AreaSymbolSettings(this, dialog);
}



// ### AreaSymbolSettings ###

AreaSymbolSettings::AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog)
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	map = dialog->getPreviewMap();
	controller = dialog->getPreviewController();
	
	
	QFormLayout* general_layout = new QFormLayout();
	
	color_edit = new ColorDropDown(map);
	general_layout->addRow(tr("Area color:"), color_edit);
	
	minimum_size_edit = Util::SpinBox::create(1, 0.0, 999999.9, trUtf8("mm²"));
	general_layout->addRow(tr("Minimum size:"), minimum_size_edit);
	
	general_layout->addItem(Util::SpacerItem::create(this));
	
	
	QVBoxLayout* fill_patterns_list_layout = new QVBoxLayout();
	
	fill_patterns_list_layout->addWidget(Util::Headline::create(tr("Fills")), 0);
	
	pattern_list = new QListWidget();
	fill_patterns_list_layout->addWidget(pattern_list, 1);
	
	del_pattern_button = new QPushButton(QIcon(QString::fromLatin1(":/images/minus.png")), QString{});
	add_pattern_button = new QToolButton();
	add_pattern_button->setIcon(QIcon(QString::fromLatin1(":/images/plus.png")));
	add_pattern_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_pattern_button->setPopupMode(QToolButton::InstantPopup);
	add_pattern_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
	add_pattern_button->setMinimumSize(del_pattern_button->sizeHint());
	QMenu* add_fill_button_menu = new QMenu(add_pattern_button);
	add_fill_button_menu->addAction(tr("Line fill"), this, SLOT(addLinePattern()));
	add_fill_button_menu->addAction(tr("Pattern fill"), this, SLOT(addPointPattern()));
	add_pattern_button->setMenu(add_fill_button_menu);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_pattern_button);
	buttons_layout->addWidget(del_pattern_button);
	buttons_layout->addStretch(1);
	fill_patterns_list_layout->addLayout(buttons_layout, 0);
	
	
	QFormLayout* fill_pattern_layout = new QFormLayout();
	
	pattern_name_edit = new QLabel();
	fill_pattern_layout->addRow(pattern_name_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	/* From here, stacked widgets are used to unify the layout of pattern type dependant fields. */
	QStackedWidget* single_line_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_headline);
	
	QStackedWidget* single_line_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label1, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit1  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit1, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label1, single_line_edit1);
	
	QStackedWidget* single_line_label2 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label2, SLOT(setCurrentIndex(int)));
	QStackedWidget* single_line_edit2  = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_edit2, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(single_line_label2, single_line_edit2);
	
	QStackedWidget* single_line_label3 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), single_line_label3, SLOT(setCurrentIndex(int)));
	pattern_line_offset_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(single_line_label3, pattern_line_offset_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	QStackedWidget* parallel_lines_headline = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_headline, SLOT(setCurrentIndex(int)));
	fill_pattern_layout->addRow(parallel_lines_headline);
	
	QStackedWidget* parallel_lines_label1 = new QStackedWidget();
	connect(this, SIGNAL(switchPatternEdits(int)), parallel_lines_label1, SLOT(setCurrentIndex(int)));
	pattern_spacing_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(parallel_lines_label1, pattern_spacing_edit);
	
	/* Stacked widgets, index 0: line pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single line")));
	
	QLabel* pattern_linewidth_label = new QLabel(tr("Line width:"));
	pattern_linewidth_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_linewidth_label);
	single_line_edit1 ->addWidget(pattern_linewidth_edit);
	
	QLabel* pattern_color_label = new QLabel(tr("Line color:"));
	pattern_color_edit = new ColorDropDown(map);
	single_line_label2->addWidget(pattern_color_label);
	single_line_edit2 ->addWidget(pattern_color_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Line offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel lines")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Line spacing:")));
	
	/* Stacked widgets, index 1: point pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single row")));
	
	QLabel* pattern_pointdist_label = new QLabel(tr("Pattern interval:"));
	pattern_pointdist_edit = Util::SpinBox::create(2, 0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_pointdist_label);
	single_line_edit1 ->addWidget(pattern_pointdist_edit);
	
	QLabel* pattern_offset_along_line_label = new QLabel(tr("Pattern offset:"));
	pattern_offset_along_line_edit = Util::SpinBox::create(2, -999999.9, 999999.9, tr("mm"));
	single_line_label2->addWidget(pattern_offset_along_line_label);
	single_line_edit2 ->addWidget(pattern_offset_along_line_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Row offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel rows")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Row spacing:")));
	
	/* General fields again. Not stacked. */
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	fill_pattern_layout->addRow(Util::Headline::create(tr("Fill rotation")));
	
	pattern_angle_edit = Util::SpinBox::create(1, 0.0, 360.0, trUtf8("°"));
	pattern_angle_edit->setWrapping(true);
	fill_pattern_layout->addRow(tr("Angle:"), pattern_angle_edit);
	
	pattern_rotatable_check = new QCheckBox(tr("adjustable per object"));
	fill_pattern_layout->addRow(QString{}, pattern_rotatable_check);
	
	
	QHBoxLayout* fill_patterns_layout = new QHBoxLayout();
	fill_patterns_layout->addLayout(fill_patterns_list_layout, 1);
	fill_patterns_layout->addItem(Util::SpacerItem::create(this));
	fill_patterns_layout->addLayout(fill_pattern_layout, 3);
	
	QBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(general_layout);
	layout->addLayout(fill_patterns_layout);
	
	QWidget* area_tab = new QWidget();
	area_tab->setLayout(layout);
	addPropertiesGroup(tr("Area settings"), area_tab);
	
	connect(color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(colorChanged()));
	connect(minimum_size_edit, SIGNAL(valueChanged(double)), this, SLOT(minimumSizeChanged(double)));
	connect(del_pattern_button, SIGNAL(clicked(bool)), this, SLOT(deleteActivePattern()));
	connect(pattern_list, SIGNAL(currentRowChanged(int)), this, SLOT(selectPattern(int)));
	connect(pattern_angle_edit, SIGNAL(valueChanged(double)), this, SLOT(patternAngleChanged(double)));
	connect(pattern_rotatable_check, SIGNAL(clicked(bool)), this, SLOT(patternRotatableClicked(bool)));
	connect(pattern_spacing_edit, SIGNAL(valueChanged(double)), this, SLOT(patternSpacingChanged(double)));
	connect(pattern_line_offset_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineOffsetChanged(double)));
	connect(pattern_offset_along_line_edit, SIGNAL(valueChanged(double)), this, SLOT(patternOffsetAlongLineChanged(double)));
	
	connect(pattern_color_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(patternColorChanged()));
	connect(pattern_linewidth_edit, SIGNAL(valueChanged(double)), this, SLOT(patternLineWidthChanged(double)));
	connect(pattern_pointdist_edit, SIGNAL(valueChanged(double)), this, SLOT(patternPointDistChanged(double)));
	
	updateAreaGeneral();
	updatePatternWidgets();
	loadPatterns();
}

void AreaSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Area);
	
	clearPatterns();
	
	SymbolPropertiesWidget::reset(symbol);
	this->symbol = reinterpret_cast<AreaSymbol*>(symbol);
	
	updateAreaGeneral();
	loadPatterns();
}

void AreaSymbolSettings::updateAreaGeneral()
{
	color_edit->blockSignals(true);
	color_edit->setColor(symbol->getColor());
	color_edit->blockSignals(false);
	minimum_size_edit->blockSignals(true);
	minimum_size_edit->setValue(0.001 * symbol->minimum_area);
	minimum_size_edit->blockSignals(false);
}

void AreaSymbolSettings::clearPatterns()
{
	pattern_list->clear();
	for (int i = 0; i < (int)this->symbol->patterns.size(); ++i)
	{
		if (this->symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
			removePropertiesGroup(2);
	}
}

void AreaSymbolSettings::loadPatterns()
{
	Q_ASSERT(pattern_list->count() == 0);
	Q_ASSERT(count() == 2); // General + Area tab
	
	for (int i = 0; i < symbol->getNumFillPatterns(); ++i)
	{
		pattern_list->addItem(symbol->patterns[i].name);
		if (symbol->getFillPattern(i).type == AreaSymbol::FillPattern::PointPattern)
		{
			PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, symbol->getFillPattern(i).point, 16);
			connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
			addPropertiesGroup(symbol->patterns[i].name, editor);
		}
	}
	updatePatternNames();
	pattern_list->setCurrentRow(0);
}

void AreaSymbolSettings::updatePatternNames()
{
	int line_pattern_num = 0, point_pattern_num = 0;
	for (int i = 0; i < (int)symbol->patterns.size(); ++i)
	{
		if (symbol->patterns[i].type == AreaSymbol::FillPattern::PointPattern)
		{
			point_pattern_num++;
			QString name = tr("Pattern fill %1").arg(point_pattern_num);
			symbol->patterns[i].name = name;
			symbol->patterns[i].point->setName(name);
			pattern_list->item(i)->setText(name);
			renamePropertiesGroup(point_pattern_num + 1, name);
		}
		else
		{
			line_pattern_num++;
			QString name = tr("Line fill %1").arg(line_pattern_num);
			symbol->patterns[i].name = name;
			pattern_list->item(i)->setText(symbol->patterns[i].name);
		}
	}
}

void AreaSymbolSettings::updatePatternWidgets()
{
	int index = pattern_list->currentRow();
	bool pattern_active = (index >= 0);
	if (pattern_active)
		active_pattern = symbol->patterns.begin() + index;
	
	del_pattern_button->setEnabled(pattern_active);
	
	AreaSymbol::FillPattern* pattern = pattern_active ? &*active_pattern : new AreaSymbol::FillPattern();
	pattern_name_edit->setText(pattern_active ? (QLatin1String("<b>") + active_pattern->name + QLatin1String("</b>")) : tr("No fill selected"));
	
	pattern_angle_edit->blockSignals(true);
	pattern_rotatable_check->blockSignals(true);
	pattern_spacing_edit->blockSignals(true);
	pattern_line_offset_edit->blockSignals(true);
	
	pattern_angle_edit->setValue(pattern->angle * 180.0 / M_PI);
	pattern_angle_edit->setEnabled(pattern_active);
	pattern_rotatable_check->setChecked(pattern->rotatable);
	pattern_rotatable_check->setEnabled(pattern_active);
	pattern_spacing_edit->setValue(0.001 * pattern->line_spacing);
	pattern_spacing_edit->setEnabled(pattern_active);
	pattern_line_offset_edit->setValue(0.001 * pattern->line_offset);
	pattern_line_offset_edit->setEnabled(pattern_active);
	
	pattern_angle_edit->blockSignals(false);
	pattern_rotatable_check->blockSignals(false);
	pattern_spacing_edit->blockSignals(false);
	pattern_line_offset_edit->blockSignals(false);
	
	if (pattern->type == AreaSymbol::FillPattern::LinePattern)
	{
		emit switchPatternEdits(0);
		
		pattern_linewidth_edit->blockSignals(true);
		pattern_linewidth_edit->setValue(0.001 * pattern->line_width);
		pattern_linewidth_edit->setEnabled(pattern_active);
		pattern_linewidth_edit->blockSignals(false);
		
		pattern_color_edit->blockSignals(true);
		pattern_color_edit->setColor(pattern->line_color);
		pattern_color_edit->setEnabled(pattern_active);
		pattern_color_edit->blockSignals(false);
	}
	else
	{
		emit switchPatternEdits(1);
		
		pattern_offset_along_line_edit->blockSignals(true);
		pattern_offset_along_line_edit->setValue(0.001 * pattern->offset_along_line);
		pattern_offset_along_line_edit->setEnabled(pattern_active);
		pattern_offset_along_line_edit->blockSignals(false);
		
		pattern_pointdist_edit->blockSignals(true);
		pattern_pointdist_edit->setValue(0.001 * pattern->point_distance);
		pattern_pointdist_edit->setEnabled(pattern_active);
		pattern_pointdist_edit->blockSignals(false);
	}
	
	if (!pattern_active)
		delete pattern;
}

void AreaSymbolSettings::addLinePattern()
{
	addPattern(AreaSymbol::FillPattern::LinePattern);
}

void AreaSymbolSettings::addPointPattern()
{
	addPattern(AreaSymbol::FillPattern::PointPattern);
}

void AreaSymbolSettings::addPattern(AreaSymbol::FillPattern::Type type)
{
	int index = pattern_list->currentRow() + 1;
	if (index < 0)
		index = symbol->patterns.size();
	
	active_pattern = symbol->patterns.insert(symbol->patterns.begin() + index, AreaSymbol::FillPattern());
	auto temp_name = QString::fromLatin1("new");
	pattern_list->insertItem(index, temp_name);
	Q_ASSERT(int(symbol->patterns.size()) == pattern_list->count());
	
	active_pattern->type = type;
	if (type == AreaSymbol::FillPattern::PointPattern)
	{
		active_pattern->point = new PointSymbol();
		active_pattern->point->setRotatable(true);
		PointSymbolEditorWidget* editor = new PointSymbolEditorWidget(controller, active_pattern->point, 16);
		connect(editor, SIGNAL(symbolEdited()), this, SIGNAL(propertiesModified()) );
		if (pattern_list->currentRow() == int(symbol->patterns.size()) - 1)
		{
			addPropertiesGroup(temp_name, editor);
		}
		else
		{
			int group_index = indexOfPropertiesGroup(symbol->patterns[pattern_list->currentRow()+1].name);
			insertPropertiesGroup(group_index, temp_name, editor);
		}
	}
	else if (map->getNumColors() > 0)
	{
		// Default color for lines
		active_pattern->line_color = map->getColor(0);
	}
	updatePatternNames();
	emit propertiesModified();
	
	pattern_list->setCurrentRow(index);
}

void AreaSymbolSettings::deleteActivePattern()
{
	int index = pattern_list->currentRow();
	if (index < 0)
	{
		qWarning() << "AreaSymbolSettings::deleteActivePattern(): no fill selected."; // not translated
		return;
	}
	
	if (active_pattern->type == AreaSymbol::FillPattern::PointPattern)
	{
		removePropertiesGroup(active_pattern->name);
		delete active_pattern->point;
		active_pattern->point = NULL;
	}
	symbol->patterns.erase(active_pattern);
	
	delete pattern_list->takeItem(index);
	updatePatternNames();
	updatePatternWidgets();
	
	emit propertiesModified();
	
	index = pattern_list->currentRow();
	selectPattern(index);
}

void AreaSymbolSettings::selectPattern(int index)
{
	active_pattern = symbol->patterns.begin() + index;
	updatePatternWidgets();
}

void AreaSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::minimumSizeChanged(double value)
{
	symbol->minimum_area = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternAngleChanged(double value)
{
	active_pattern->angle = value * M_PI / 180.0;
	emit propertiesModified();
}

void AreaSymbolSettings::patternRotatableClicked(bool checked)
{
	active_pattern->rotatable = checked;
	emit propertiesModified();
}

void AreaSymbolSettings::patternSpacingChanged(double value)
{
	active_pattern->line_spacing = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineOffsetChanged(double value)
{
	active_pattern->line_offset = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternOffsetAlongLineChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->offset_along_line = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternColorChanged()
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_color = pattern_color_edit->color();
	emit propertiesModified();
}

void AreaSymbolSettings::patternLineWidthChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::LinePattern);
	active_pattern->line_width = qRound(1000.0 * value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternPointDistChanged(double value)
{
	Q_ASSERT(active_pattern->type == AreaSymbol::FillPattern::PointPattern);
	active_pattern->point_distance = qRound(1000.0 * value);
	emit propertiesModified();
}
