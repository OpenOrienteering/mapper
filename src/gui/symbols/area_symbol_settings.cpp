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

#include <memory>

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLatin1String>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QString>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "core/map.h"
#include "core/symbols/symbol.h"
#include "core/symbols/point_symbol.h"
#include "gui/symbols/point_symbol_editor_widget.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "gui/util_gui.h"
#include "util/backports.h"
#include "util/scoped_signals_blocker.h"


namespace OpenOrienteering {

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
	
	
	auto general_layout = new QFormLayout();
	
	color_edit = new ColorDropDown(map);
	general_layout->addRow(tr("Area color:"), color_edit);
	
	minimum_size_edit = Util::SpinBox::create(1, 0.0, 999999.9, trUtf8("mm²"));
	general_layout->addRow(tr("Minimum size:"), minimum_size_edit);
	
	general_layout->addItem(Util::SpacerItem::create(this));
	
	
	auto fill_patterns_list_layout = new QVBoxLayout();
	
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
	auto add_fill_button_menu = new QMenu(add_pattern_button);
	add_fill_button_menu->addAction(tr("Line fill"), this, SLOT(addLinePattern()));
	add_fill_button_menu->addAction(tr("Pattern fill"), this, SLOT(addPointPattern()));
	add_pattern_button->setMenu(add_fill_button_menu);
	
	auto buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(add_pattern_button);
	buttons_layout->addWidget(del_pattern_button);
	buttons_layout->addStretch(1);
	fill_patterns_list_layout->addLayout(buttons_layout, 0);
	
	
	auto fill_pattern_layout = new QFormLayout();
	
	pattern_name_edit = new QLabel();
	fill_pattern_layout->addRow(pattern_name_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	
	/* From here, stacked widgets are used to unify the layout of pattern type dependant fields. */
	auto single_line_headline = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_headline, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(single_line_headline);
	
	auto single_line_label1 = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_label1, &QStackedWidget::setCurrentIndex);
	auto single_line_edit1  = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_edit1, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(single_line_label1, single_line_edit1);
	
	auto single_line_label2 = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_label2, &QStackedWidget::setCurrentIndex);
	auto single_line_edit2  = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_edit2, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(single_line_label2, single_line_edit2);
	
	auto single_line_label3 = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, single_line_label3, &QStackedWidget::setCurrentIndex);
	pattern_line_offset_edit = Util::SpinBox::create(3, -999999.9, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(single_line_label3, pattern_line_offset_edit);
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	auto parallel_lines_headline = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, parallel_lines_headline, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(parallel_lines_headline);
	
	auto parallel_lines_label1 = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, parallel_lines_label1, &QStackedWidget::setCurrentIndex);
	pattern_spacing_edit = Util::SpinBox::create(3, 0.0, 999999.9, tr("mm"));
	fill_pattern_layout->addRow(parallel_lines_label1, pattern_spacing_edit);
	
	/* Stacked widgets, index 0: line pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single line")));
	
	auto pattern_linewidth_label = new QLabel(tr("Line width:"));
	pattern_linewidth_edit = Util::SpinBox::create(3, 0.0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_linewidth_label);
	single_line_edit1 ->addWidget(pattern_linewidth_edit);
	
	auto pattern_color_label = new QLabel(tr("Line color:"));
	pattern_color_edit = new ColorDropDown(map);
	single_line_label2->addWidget(pattern_color_label);
	single_line_edit2 ->addWidget(pattern_color_edit);
	
	single_line_label3->addWidget(new QLabel(tr("Line offset:")));
	
	parallel_lines_headline->addWidget(Util::Headline::create(tr("Parallel lines")));
	
	parallel_lines_label1->addWidget(new QLabel(tr("Line spacing:")));
	
	/* Stacked widgets, index 1: point pattern fields */
	single_line_headline->addWidget(Util::Headline::create(tr("Single row")));
	
	auto pattern_pointdist_label = new QLabel(tr("Pattern interval:"));
	pattern_pointdist_edit = Util::SpinBox::create(3, 0, 999999.9, tr("mm"));
	single_line_label1->addWidget(pattern_pointdist_label);
	single_line_edit1 ->addWidget(pattern_pointdist_edit);
	
	auto pattern_offset_along_line_label = new QLabel(tr("Pattern offset:"));
	pattern_offset_along_line_edit = Util::SpinBox::create(3, -999999.9, 999999.9, tr("mm"));
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
	
	
	fill_pattern_layout->addItem(Util::SpacerItem::create(this));
	
	/* Stacked widgets again. */
	auto clipping_headline = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, clipping_headline, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(clipping_headline);
	
	auto clipping_edit  = new QStackedWidget();
	connect(this, &AreaSymbolSettings::switchPatternEdits, clipping_edit, &QStackedWidget::setCurrentIndex);
	fill_pattern_layout->addRow(clipping_edit);
	
	// Line pattern: Clipping settings not used
	clipping_headline->addWidget(new QWidget());
	clipping_edit->addWidget(new QWidget());
	
	// Point pattern clipping
	clipping_headline->addWidget(Util::Headline::create(tr("Element drawing at boundary")));
	pattern_clipping_edit = new QComboBox();
	pattern_clipping_edit->addItem(tr("Clip elements at the boundary."), AreaSymbol::FillPattern::Default);
	pattern_clipping_edit->addItem(tr("Draw elements if the center is inside the boundary."), AreaSymbol::FillPattern::NoClippingIfCenterInside);
	pattern_clipping_edit->addItem(tr("Draw elements if any point is inside the boundary."), AreaSymbol::FillPattern::NoClippingIfPartiallyInside);
	pattern_clipping_edit->addItem(tr("Draw elements if all points are inside the boundary."), AreaSymbol::FillPattern::NoClippingIfCompletelyInside);
	clipping_edit->addWidget(pattern_clipping_edit);
	
	
	auto fill_patterns_layout = new QHBoxLayout();
	fill_patterns_layout->addLayout(fill_patterns_list_layout, 1);
	fill_patterns_layout->addItem(Util::SpacerItem::create(this));
	fill_patterns_layout->addLayout(fill_pattern_layout, 3);
	
	auto layout = new QVBoxLayout();
	layout->addLayout(general_layout);
	layout->addLayout(fill_patterns_layout);
	
	auto area_tab = new QWidget();
	area_tab->setLayout(layout);
	addPropertiesGroup(tr("Area settings"), area_tab);
	
	connect(color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AreaSymbolSettings::colorChanged);
	connect(minimum_size_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::minimumSizeChanged);
	connect(del_pattern_button, &QAbstractButton::clicked, this, &AreaSymbolSettings::deleteActivePattern);
	connect(pattern_list, &QListWidget::currentRowChanged, this, &AreaSymbolSettings::selectPattern);
	connect(pattern_angle_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternAngleChanged);
	connect(pattern_rotatable_check, &QAbstractButton::clicked, this, &AreaSymbolSettings::patternRotatableClicked);
	connect(pattern_spacing_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternSpacingChanged);
	connect(pattern_line_offset_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternLineOffsetChanged);
	connect(pattern_offset_along_line_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternOffsetAlongLineChanged);
	
	connect(pattern_color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AreaSymbolSettings::patternColorChanged);
	connect(pattern_linewidth_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternLineWidthChanged);
	connect(pattern_pointdist_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AreaSymbolSettings::patternPointDistChanged);
	
	connect(pattern_clipping_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](){
		active_pattern->setClipping(AreaSymbol::FillPattern::Options(pattern_clipping_edit->currentData(Qt::UserRole).toInt()));
		emit propertiesModified();
	});
	
	first_pattern_tab = count();
	
	updateAreaGeneral();
	updatePatternWidgets();
	loadPatterns();
}


AreaSymbolSettings::~AreaSymbolSettings() = default;



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
	ScopedMultiSignalsBlocker block(color_edit, minimum_size_edit);
	color_edit->setColor(symbol->getColor());
	minimum_size_edit->setValue(0.001 * symbol->minimum_area);
}


void AreaSymbolSettings::clearPatterns()
{
	pattern_list->clear();
	for (const auto& pattern : symbol->patterns)
	{
		if (pattern.type == AreaSymbol::FillPattern::PointPattern)
			removePropertiesGroup(first_pattern_tab);
	}
}


void AreaSymbolSettings::loadPatterns()
{
	Q_ASSERT(pattern_list->count() == 0);
	
	for (const auto& pattern : symbol->patterns)
	{
		pattern_list->addItem(pattern.name);
		if (pattern.type == AreaSymbol::FillPattern::PointPattern)
		{
			auto editor = new PointSymbolEditorWidget(controller, pattern.point, 16);
			connect(editor, &PointSymbolEditorWidget::symbolEdited, this, &SymbolPropertiesWidget::propertiesModified );
			addPropertiesGroup(pattern.name, editor);
		}
	}
	updatePatternNames();
	pattern_list->setCurrentRow(0);
}


void AreaSymbolSettings::updatePatternNames()
{
	int i = 0, line_pattern_num = 0, point_pattern_num = 0;
	for (auto& pattern : symbol->patterns)
	{
		if (pattern.type == AreaSymbol::FillPattern::PointPattern)
		{
			auto pattern_tab = first_pattern_tab + point_pattern_num;
			++point_pattern_num;
			QString name = tr("Pattern fill %1").arg(point_pattern_num);
			pattern.name = name;
			pattern.point->setName(name);
			pattern_list->item(i)->setText(name);
			renamePropertiesGroup(pattern_tab, name);
		}
		else
		{
			++line_pattern_num;
			QString name = tr("Line fill %1").arg(line_pattern_num);
			pattern.name = name;
			pattern_list->item(i)->setText(pattern.name);
		}
		++i;
	}
}


void AreaSymbolSettings::updatePatternWidgets()
{
	int index = pattern_list->currentRow();
	bool pattern_active = (index >= 0);
	if (pattern_active)
		active_pattern = symbol->patterns.begin() + index;
	
	del_pattern_button->setEnabled(pattern_active);
	
	auto pattern = pattern_active ? &*active_pattern : new AreaSymbol::FillPattern();
	pattern_name_edit->setText(pattern_active ? (QLatin1String("<b>") + active_pattern->name + QLatin1String("</b>")) : tr("No fill selected"));
	
	{
		ScopedMultiSignalsBlocker block(
		  pattern_angle_edit,
		  pattern_rotatable_check,
		  pattern_spacing_edit,
		  pattern_line_offset_edit
		);
		pattern_angle_edit->setValue(qRadiansToDegrees(pattern->angle));
		pattern_angle_edit->setEnabled(pattern_active);
		pattern_rotatable_check->setChecked(pattern->rotatable());
		pattern_rotatable_check->setEnabled(pattern_active);
		pattern_spacing_edit->setValue(0.001 * pattern->line_spacing);
		pattern_spacing_edit->setEnabled(pattern_active);
		pattern_line_offset_edit->setValue(0.001 * pattern->line_offset);
		pattern_line_offset_edit->setEnabled(pattern_active);
	}
	
	switch (pattern->type)
	{
	case AreaSymbol::FillPattern::LinePattern:
		{
			emit switchPatternEdits(0);
			
			ScopedMultiSignalsBlocker block(pattern_linewidth_edit, pattern_color_edit);
			pattern_linewidth_edit->setValue(0.001 * pattern->line_width);
			pattern_linewidth_edit->setEnabled(pattern_active);
			pattern_color_edit->setColor(pattern->line_color);
			pattern_color_edit->setEnabled(pattern_active);
			break;
		}
	case AreaSymbol::FillPattern::PointPattern:
		{
			emit switchPatternEdits(1);
			
			ScopedMultiSignalsBlocker block(
			  pattern_offset_along_line_edit,
			  pattern_pointdist_edit,
			  pattern_clipping_edit
			);
			pattern_offset_along_line_edit->setValue(0.001 * pattern->offset_along_line);
			pattern_offset_along_line_edit->setEnabled(pattern_active);
			pattern_pointdist_edit->setValue(0.001 * pattern->point_distance);
			pattern_pointdist_edit->setEnabled(pattern_active);
			pattern_clipping_edit->setCurrentIndex(pattern_clipping_edit->findData(int(pattern->clipping())));
			break;
		}
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
		index = int(symbol->patterns.size());
	
	active_pattern = symbol->patterns.insert(symbol->patterns.begin() + index, AreaSymbol::FillPattern());
	auto temp_name = QString::fromLatin1("new");
	pattern_list->insertItem(index, temp_name);
	Q_ASSERT(int(symbol->patterns.size()) == pattern_list->count());
	
	active_pattern->type = type;
	if (type == AreaSymbol::FillPattern::PointPattern)
	{
		active_pattern->point = new PointSymbol();
		active_pattern->point->setRotatable(true);
		auto editor = new PointSymbolEditorWidget(controller, active_pattern->point, 16);
		connect(editor, &PointSymbolEditorWidget::symbolEdited, this, &SymbolPropertiesWidget::propertiesModified );
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
		qWarning("AreaSymbolSettings::deleteActivePattern(): no fill selected.");
		return;
	}
	
	if (active_pattern->type == AreaSymbol::FillPattern::PointPattern)
	{
		removePropertiesGroup(active_pattern->name);
		delete active_pattern->point;
		active_pattern->point = nullptr;
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
	active_pattern->angle = qDegreesToRadians(value);
	emit propertiesModified();
}

void AreaSymbolSettings::patternRotatableClicked(bool checked)
{
	active_pattern->setRotatable(checked);
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


}  // namespace OpenOrienteering
