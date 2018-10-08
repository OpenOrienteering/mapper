/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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


#include "line_symbol_settings.h"

#include <iterator>

#include <QtGlobal>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QSpinBox>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include "core/symbols/symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "gui/util_gui.h"
#include "gui/symbols/point_symbol_editor_widget.h"
#include "gui/symbols/symbol_setting_dialog.h"
#include "gui/widgets/color_dropdown.h"
#include "util/backports.h"  // IWYU pragma: keep
#include "util/util.h"


namespace OpenOrienteering {

// ### LineSymbol ###

SymbolPropertiesWidget* LineSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new LineSymbolSettings(this, dialog);
}


// ### LineSymbolSettings ###

LineSymbolSettings::LineSymbolSettings(LineSymbol* symbol, SymbolSettingDialog* dialog)
: SymbolPropertiesWidget(symbol, dialog), symbol(symbol), dialog(dialog)
{
	auto map = dialog->getPreviewMap();
	
	auto line_tab = new QWidget();
	auto layout = new QGridLayout();
	layout->setColumnStretch(1, 1);
	line_tab->setLayout(layout);
	
	auto width_label = new QLabel(tr("Line width:"));
	width_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto color_label = new QLabel(tr("Line color:"));
	color_edit = new ColorDropDown(map, symbol->getColor());
	
	int row = 0, col = 0;
	layout->addWidget(width_label, row, col++);
	layout->addWidget(width_edit,  row, col);
	
	row++; col = 0;
	layout->addWidget(color_label, row, col++);
	layout->addWidget(color_edit,  row, col, 1, -1);
	
	
	auto minimum_length_label = new QLabel(tr("Minimum line length:"));
	minimum_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto line_join_label = new QLabel(tr("Line join:"));
	line_join_combo = new QComboBox();
	line_join_combo->addItem(tr("miter"), QVariant(LineSymbol::MiterJoin));
	line_join_combo->addItem(tr("round"), QVariant(LineSymbol::RoundJoin));
	line_join_combo->addItem(tr("bevel"), QVariant(LineSymbol::BevelJoin));
	
	auto line_cap_label = new QLabel(tr("Line cap:"));
	line_cap_combo = new QComboBox();
	line_cap_combo->addItem(tr("flat"), QVariant(LineSymbol::FlatCap));
	line_cap_combo->addItem(tr("round"), QVariant(LineSymbol::RoundCap));
	line_cap_combo->addItem(tr("square"), QVariant(LineSymbol::SquareCap));
	line_cap_combo->addItem(tr("pointed"), QVariant(LineSymbol::PointedCap));
	
	start_offset_label = new QLabel();  // later
	start_offset_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	end_offset_label = new QLabel();  // later
	end_offset_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	dashed_check = new QCheckBox(tr("Line is dashed"));
	
	border_check = new QCheckBox(tr("Enable border lines"));
	
	line_settings_list = { 
	    minimum_length_label, minimum_length_edit,
	    line_cap_label, line_cap_combo,
	    line_join_label, line_join_combo,
	    start_offset_label, start_offset_edit,
	    end_offset_label, end_offset_edit,
	    border_check,
	};
	
	row++; col = 0;
	layout->addWidget(minimum_length_label, row, col++);
	layout->addWidget(minimum_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(line_join_label, row, col++);
	layout->addWidget(line_join_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(line_cap_label, row, col++);
	layout->addWidget(line_cap_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(start_offset_label, row, col++);
	layout->addWidget(start_offset_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(end_offset_label, row, col++);
	layout->addWidget(end_offset_edit, row, col, 1, -1);
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(Util::Headline::create(tr("Dashed line")), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(dashed_check, row, col, 1, -1);
	
	auto dash_length_label = new QLabel(tr("Dash length:"));
	dash_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto break_length_label = new QLabel(tr("Break length:"));
	break_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto dash_group_label = new QLabel(tr("Dashes grouped together:"));
	dash_group_combo = new QComboBox();
	dash_group_combo->addItem(tr("none"), QVariant(1));
	dash_group_combo->addItem(tr("2"), QVariant(2));
	dash_group_combo->addItem(tr("3"), QVariant(3));
	dash_group_combo->addItem(tr("4"), QVariant(4));
	
	in_group_break_length_label = new QLabel(tr("In-group break length:"));
	in_group_break_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	half_outer_dashes_check = new QCheckBox(tr("Half length of first and last dash"));
	
	auto mid_symbol_placement_label = new QLabel(tr("Mid symbols placement:"));
	mid_symbol_placement_combo = new QComboBox();
	mid_symbol_placement_combo->addItem(tr("Center of dashes"), QVariant(LineSymbol::CenterOfDash));
	mid_symbol_placement_combo->addItem(tr("Center of dash groups"), QVariant(LineSymbol::CenterOfDashGroup));
	mid_symbol_placement_combo->addItem(tr("Center of gaps"), QVariant(LineSymbol::CenterOfGap));
	
	dashed_widget_list = {
	    dash_length_label, dash_length_edit,
	    break_length_label, break_length_edit,
	    dash_group_label, dash_group_combo,
	    in_group_break_length_label, in_group_break_length_edit,
	    half_outer_dashes_check,
	    mid_symbol_placement_label, mid_symbol_placement_combo,
	};
	
	row++; col = 0;
	layout->addWidget(dash_length_label, row, col++);
	layout->addWidget(dash_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(break_length_label, row, col++);
	layout->addWidget(break_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(dash_group_label, row, col++);
	layout->addWidget(dash_group_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(in_group_break_length_label, row, col++);
	layout->addWidget(in_group_break_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(half_outer_dashes_check, row, col, 1, -1);
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(Util::Headline::create(tr("Mid symbols")), row, col, 1, -1);
	
	auto mid_symbol_per_spot_label = new QLabel(tr("Mid symbols per spot:"));
	mid_symbol_per_spot_edit = Util::SpinBox::create(1, 99);
	
	mid_symbol_distance_label = new QLabel(tr("Mid symbol distance:"));
	mid_symbol_distance_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto segment_length_label = new QLabel(tr("Distance between spots:"));
	segment_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto end_length_label = new QLabel(tr("Distance from line end:"));
	end_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	show_at_least_one_symbol_check = new QCheckBox(tr("Show at least one mid symbol"));
	
	auto minimum_mid_symbol_count_label = new QLabel(tr("Minimum mid symbol count:"));
	minimum_mid_symbol_count_edit = Util::SpinBox::create(0, 99);
	
	auto minimum_mid_symbol_count_when_closed_label = new QLabel(tr("Minimum mid symbol count when closed:"));
	minimum_mid_symbol_count_when_closed_edit = Util::SpinBox::create(0, 99);
	
	row++; col = 0;
	layout->addWidget(mid_symbol_placement_label, row, col++);
	layout->addWidget(mid_symbol_placement_combo, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(mid_symbol_per_spot_label, row, col++);
	layout->addWidget(mid_symbol_per_spot_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(mid_symbol_distance_label, row, col++);
	layout->addWidget(mid_symbol_distance_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(segment_length_label, row, col++);
	layout->addWidget(segment_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(end_length_label, row, col++);
	layout->addWidget(end_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(show_at_least_one_symbol_check, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(minimum_mid_symbol_count_label, row, col++);
	layout->addWidget(minimum_mid_symbol_count_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(minimum_mid_symbol_count_when_closed_label, row, col++);
	layout->addWidget(minimum_mid_symbol_count_when_closed_edit, row, col, 1, -1);
	
	mid_symbol_widget_list = {
	    mid_symbol_placement_label, mid_symbol_placement_combo,
	    mid_symbol_per_spot_label, mid_symbol_per_spot_edit,
	    show_at_least_one_symbol_check,
	};
	
	undashed_widget_list = {
	    segment_length_label, segment_length_edit,
	    end_length_label, end_length_edit,
	    minimum_mid_symbol_count_label, minimum_mid_symbol_count_edit,
	    minimum_mid_symbol_count_when_closed_label, minimum_mid_symbol_count_when_closed_edit,
	};
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(Util::Headline::create(tr("Borders")), row, col++, 1, -1);
	row++; col = 0;
	layout->addWidget(border_check, row, col, 1, -1);
	
	different_borders_check = new QCheckBox(tr("Different borders on left and right sides"));
	row++; col = 0;
	layout->addWidget(different_borders_check, row, col, 1, -1);
	
	auto left_border_label = Util::Headline::create(tr("Left border:"));
	row++; col = 0;
	layout->addWidget(left_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getBorder(), map, row, col, layout, border_widgets);
	
	auto right_border_label = Util::Headline::create(tr("Right border:"));
	row++; col = 0;
	layout->addWidget(right_border_label, row, col++, 1, -1);
	createBorderWidgets(symbol->getRightBorder(), map, row, col, layout, right_border_widgets);
	
	border_widget_list.reserve(1 + border_widgets.widget_list.size());
	border_widget_list.push_back(different_borders_check);
	border_widget_list.insert(border_widget_list.end(), begin(border_widgets.widget_list), end(border_widgets.widget_list));
	
	different_borders_widget_list.reserve(2 + right_border_widgets.widget_list.size());
	different_borders_widget_list.push_back(left_border_label);
	different_borders_widget_list.push_back(right_border_label);
	different_borders_widget_list.insert(different_borders_widget_list.end(), begin(right_border_widgets.widget_list), end(right_border_widgets.widget_list));
	
	
	row++; col = 0;
	layout->addWidget(new QWidget(), row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(Util::Headline::create(tr("Dash symbol")), row, col++, 1, -1);
	
	supress_dash_symbol_check = new QCheckBox(tr("Suppress the dash symbol at line start and line end"));
	row++; col = 0;
	layout->addWidget(supress_dash_symbol_check, row, col, 1, -1);
	
	scale_dash_symbol_check = new QCheckBox(tr("Scale the dash symbol at corners"));
	row++; col = 0;
	layout->addWidget(scale_dash_symbol_check, row, col, 1, -1);
	
	row++;
	layout->setRowStretch(row, 1);
	
	const int line_tab_width = line_tab->sizeHint().width();
	
	scroll_area = new QScrollArea();
	scroll_area->setWidget(line_tab);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setMinimumWidth(line_tab_width + scroll_area->verticalScrollBar()->sizeHint().width());
	addPropertiesGroup(tr("Line settings"), scroll_area);
	
	PointSymbolEditorWidget* point_symbol_editor = nullptr;
	auto controller = dialog->getPreviewController();
	
	symbol->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
	for (auto point_symbol : { symbol->getStartSymbol(), symbol->getMidSymbol(), symbol->getEndSymbol(), symbol->getDashSymbol() })
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, &PointSymbolEditorWidget::symbolEdited, this, &LineSymbolSettings::pointSymbolEdited);
	}
	
	updateStates();
	updateContents();
	
	connect(width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::widthChanged);
	connect(color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::colorChanged);
	connect(minimum_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::minimumDimensionsEdited);
	connect(line_cap_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::lineCapChanged);
	connect(line_join_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::lineJoinChanged);
	connect(start_offset_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::startOffsetChanged);
	connect(end_offset_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::endOffsetChanged);
	connect(dashed_check, &QAbstractButton::clicked, this, &LineSymbolSettings::dashedChanged);
	connect(segment_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::segmentLengthChanged);
	connect(end_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::endLengthChanged);
	connect(show_at_least_one_symbol_check, &QAbstractButton::clicked, this, &LineSymbolSettings::showAtLeastOneSymbolChanged);
	connect(minimum_mid_symbol_count_edit, QOverload<int>::of(&QSpinBox::valueChanged), this, &LineSymbolSettings::minimumDimensionsEdited);
	connect(minimum_mid_symbol_count_when_closed_edit, QOverload<int>::of(&QSpinBox::valueChanged), this, &LineSymbolSettings::minimumDimensionsEdited);
	connect(dash_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::dashLengthChanged);
	connect(break_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::breakLengthChanged);
	connect(dash_group_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::dashGroupsChanged);
	connect(in_group_break_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::inGroupBreakLengthChanged);
	connect(half_outer_dashes_check, &QAbstractButton::clicked, this, &LineSymbolSettings::halfOuterDashesChanged);
	connect(mid_symbol_placement_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::midSymbolPlacementChanged);
	connect(mid_symbol_per_spot_edit, QOverload<int>::of(&QSpinBox::valueChanged), this, &LineSymbolSettings::midSymbolsPerDashChanged);
	connect(mid_symbol_distance_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::midSymbolDistanceChanged);
	connect(border_check, &QAbstractButton::clicked, this, &LineSymbolSettings::borderCheckClicked);
	connect(different_borders_check, &QAbstractButton::clicked, this, &LineSymbolSettings::differentBordersClicked);
	connect(supress_dash_symbol_check, &QAbstractButton::clicked, this, &LineSymbolSettings::suppressDashSymbolClicked);
	connect(scale_dash_symbol_check, &QCheckBox::clicked, this, &LineSymbolSettings::scaleDashSymbolClicked);
}

LineSymbolSettings::~LineSymbolSettings() = default;



void LineSymbolSettings::pointSymbolEdited()
{
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::widthChanged(double value)
{
	symbol->line_width = qRound(1000 * value);
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::colorChanged()
{
	symbol->color = color_edit->color();
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::minimumDimensionsEdited()
{
	symbol->minimum_length = qRound(1000.0 * minimum_length_edit->value());
	symbol->minimum_mid_symbol_count = minimum_mid_symbol_count_edit->value();
	symbol->minimum_mid_symbol_count_when_closed = minimum_mid_symbol_count_when_closed_edit->value();
	emit propertiesModified();
}

void LineSymbolSettings::lineCapChanged(int index)
{
	symbol->cap_style = LineSymbol::CapStyle(line_cap_combo->itemData(index).toInt());
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::lineJoinChanged(int index)
{
	symbol->join_style = LineSymbol::JoinStyle(line_join_combo->itemData(index).toInt());
	emit propertiesModified();
}

void LineSymbolSettings::startOffsetChanged(double value)
{
	symbol->start_offset = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::endOffsetChanged(double value)
{
	symbol->end_offset = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::dashedChanged(bool checked)
{
	symbol->dashed = checked;
	if (!symbol->color)
	{
		symbol->break_length = 0;
		break_length_edit->setValue(0);
	}
	emit propertiesModified();
	updateStates();
	if (checked && symbol->color)
		ensureWidgetVisible(half_outer_dashes_check);
}

void LineSymbolSettings::segmentLengthChanged(double value)
{
	symbol->segment_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::endLengthChanged(double value)
{
	symbol->end_length = qRound(1000.0 * value);
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::showAtLeastOneSymbolChanged(bool checked)
{
	symbol->show_at_least_one_symbol = checked;
	emit propertiesModified();
}

void LineSymbolSettings::dashLengthChanged(double value)
{
	symbol->dash_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::breakLengthChanged(double value)
{
	symbol->break_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::dashGroupsChanged(int index)
{
	symbol->dashes_in_group = dash_group_combo->itemData(index).toInt();
	if (symbol->dashes_in_group > 1)
	{
		symbol->half_outer_dashes = false;
		half_outer_dashes_check->setChecked(false);
	}
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::inGroupBreakLengthChanged(double value)
{
	symbol->in_group_break_length = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::halfOuterDashesChanged(bool checked)
{
	symbol->half_outer_dashes = checked;
	emit propertiesModified();
}

void LineSymbolSettings::midSymbolPlacementChanged(int index)
{
	symbol->setMidSymbolPlacement(LineSymbol::MidSymbolPlacement(mid_symbol_placement_combo->itemData(index).toInt()));
	emit propertiesModified();
}

void LineSymbolSettings::midSymbolsPerDashChanged(int value)
{
	symbol->mid_symbols_per_spot = qMax(1, value);
	emit propertiesModified();
	updateStates();
}

void LineSymbolSettings::midSymbolDistanceChanged(double value)
{
	symbol->mid_symbol_distance = qRound(1000.0 * value);
	emit propertiesModified();
}

void LineSymbolSettings::borderCheckClicked(bool checked)
{
	symbol->have_border_lines = checked;
	emit propertiesModified();
	updateStates();
	if (checked)
	{
		if (symbol->areBordersDifferent())
			ensureWidgetVisible(right_border_widgets.break_length_edit);
		else
			ensureWidgetVisible(border_widgets.break_length_edit);
	}
}

void LineSymbolSettings::differentBordersClicked(bool checked)
{
	if (checked)
	{
		updateStates();
		blockSignalsRecursively(this, true);
		updateBorderContents(symbol->getRightBorder(), right_border_widgets);
		blockSignalsRecursively(this, false);
		ensureWidgetVisible(right_border_widgets.break_length_edit);
	}
	else
	{
		symbol->getRightBorder() = symbol->getBorder();
		emit propertiesModified();
		updateStates();
	}
}

void LineSymbolSettings::borderChanged()
{
	updateBorder(symbol->getBorder(), border_widgets);
	if (different_borders_check->isChecked())
		updateBorder(symbol->getRightBorder(), right_border_widgets);
	else
		symbol->getRightBorder() = symbol->getBorder();
	
	emit propertiesModified();
}

void LineSymbolSettings::suppressDashSymbolClicked(bool checked)
{
	symbol->suppress_dash_symbol_at_ends = checked;
	emit propertiesModified();
}

void LineSymbolSettings::scaleDashSymbolClicked(bool checked)
{
	symbol->setScaleDashSymbol(checked);
	emit propertiesModified();
}

void LineSymbolSettings::createBorderWidgets(LineSymbolBorder& border, Map* map, int& row, int col, QGridLayout* layout, BorderWidgets& widgets)
{
	auto width_label = new QLabel(tr("Border width:"));
	widgets.width_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto color_label = new QLabel(tr("Border color:"));
	widgets.color_edit = new ColorDropDown(map, border.color);
	
	auto shift_label = new QLabel(tr("Border shift:"));
	widgets.shift_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	widgets.dashed_check = new QCheckBox(tr("Border is dashed"));
	widgets.dashed_check->setChecked(border.dashed);
	
	widgets.widget_list = {
		width_label, widgets.width_edit,
		color_label, widgets.color_edit,
		shift_label, widgets.shift_edit,
		widgets.dashed_check
	};
	
	row++; col = 0;
	layout->addWidget(width_label, row, col++);
	layout->addWidget(widgets.width_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(color_label, row, col++);
	layout->addWidget(widgets.color_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(shift_label, row, col++);
	layout->addWidget(widgets.shift_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(widgets.dashed_check, row, col, 1, -1);
	
	auto dash_length_label = new QLabel(tr("Border dash length:"));
	widgets.dash_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	auto break_length_label = new QLabel(tr("Border break length:"));
	widgets.break_length_edit = Util::SpinBox::create(2, 0.0, 999999.9, tr("mm"));
	
	widgets.dash_widget_list = {
		dash_length_label, widgets.dash_length_edit,
		break_length_label, widgets.break_length_edit,
	};
	
	row++; col = 0;
	layout->addWidget(dash_length_label, row, col++);
	layout->addWidget(widgets.dash_length_edit, row, col, 1, -1);
	row++; col = 0;
	layout->addWidget(break_length_label, row, col++);
	layout->addWidget(widgets.break_length_edit, row, col, 1, -1);
	
	
	connect(widgets.width_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::borderChanged);
	connect(widgets.color_edit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineSymbolSettings::borderChanged);
	connect(widgets.shift_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::borderChanged);
	connect(widgets.dashed_check, &QAbstractButton::clicked, this, &LineSymbolSettings::borderChanged);
	connect(widgets.dash_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::borderChanged);
	connect(widgets.break_length_edit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineSymbolSettings::borderChanged);
}

void LineSymbolSettings::updateBorder(LineSymbolBorder& border, LineSymbolSettings::BorderWidgets& widgets)
{
	bool ensure_last_widget_visible = false;
	border.width = qRound(1000.0 * widgets.width_edit->value());
	border.color = widgets.color_edit->color();
	border.shift = qRound(1000.0 * widgets.shift_edit->value());
	if (widgets.dashed_check->isChecked() && !border.dashed)
		ensure_last_widget_visible = true;
	border.dashed = widgets.dashed_check->isChecked();
	border.dash_length = qRound(1000.0 * widgets.dash_length_edit->value());
	border.break_length = qRound(1000.0 * widgets.break_length_edit->value());
	
	updateStates();
	if (ensure_last_widget_visible)
		ensureWidgetVisible(widgets.break_length_edit);
}

void LineSymbolSettings::updateBorderContents(LineSymbolBorder& border, LineSymbolSettings::BorderWidgets& widgets)
{
	Q_ASSERT(this->signalsBlocked());
	
	widgets.width_edit->setValue(0.001 * border.width);
	widgets.color_edit->setColor(border.color);
	widgets.shift_edit->setValue(0.001 * border.shift);
	widgets.dashed_check->setChecked(border.dashed);
	
	widgets.dash_length_edit->setValue(0.001 * border.dash_length);
	widgets.break_length_edit->setValue(0.001 * border.break_length);
}

void LineSymbolSettings::ensureWidgetVisible(QWidget* widget)
{
	widget_to_ensure_visible = widget;
	QTimer::singleShot(0, this, SLOT(ensureWidgetVisible()));  // clazy:exclude=old-style-connect (needs Qt 5.4)
}

void LineSymbolSettings::ensureWidgetVisible()
{
	scroll_area->ensureWidgetVisible(widget_to_ensure_visible, 5, 5);
}

void LineSymbolSettings::updateStates()
{
	const bool symbol_active = symbol->line_width > 0;
	color_edit->setEnabled(symbol_active);
	
	const bool line_active = symbol_active && symbol->color;
	for (auto line_settings_widget : line_settings_list)
	{
		line_settings_widget->setEnabled(line_active);
	}
	
	if (symbol->cap_style == LineSymbol::PointedCap)
	{
		start_offset_label->setText(tr("Cap length at start:"));
		end_offset_label->setText(tr("Cap length at end:"));
	}
	else
	{
		start_offset_label->setText(tr("Offset at start:"));
		end_offset_label->setText(tr("Offset at end:"));
	}
	
	const bool line_dashed = symbol->dashed && symbol->color;
	if (line_dashed)
	{
		for (auto undashed_widget : undashed_widget_list)
		{
			undashed_widget->setVisible(false);
		}
		for (auto dashed_widget : dashed_widget_list)
		{
			dashed_widget->setVisible(true);
			dashed_widget->setEnabled(line_active);
		}
		in_group_break_length_label->setEnabled(line_active && symbol->dashes_in_group > 1);
		in_group_break_length_edit->setEnabled(line_active && symbol->dashes_in_group > 1);
		half_outer_dashes_check->setEnabled(line_active && symbol->dashes_in_group  == 1);
	}
	else
	{
		for (auto undashed_widget : undashed_widget_list)
		{
			undashed_widget->setVisible(true);
			undashed_widget->setEnabled(!symbol->mid_symbol->isEmpty());
		}
		show_at_least_one_symbol_check->setEnabled(show_at_least_one_symbol_check->isEnabled() && symbol->end_length > 0);
		for (auto dashed_widget : dashed_widget_list)
		{
			dashed_widget->setVisible(false);
		}
	}
	
	for (auto mid_symbol_widget : mid_symbol_widget_list)
	{
		mid_symbol_widget->setEnabled(!symbol->mid_symbol->isEmpty());
	}
	mid_symbol_distance_label->setEnabled(mid_symbol_distance_label->isEnabled() && symbol->mid_symbols_per_spot > 1);
	mid_symbol_distance_edit->setEnabled(mid_symbol_distance_edit->isEnabled() && symbol->mid_symbols_per_spot > 1);
	
	const bool border_active = symbol_active && symbol->have_border_lines;
	for (auto border_widget : border_widget_list)
	{
		border_widget->setVisible(border_active);
		border_widget->setEnabled(border_active);
	}
	for (auto border_dash_widget : border_widgets.dash_widget_list)
	{
		border_dash_widget->setVisible(border_active);
		border_dash_widget->setEnabled(border_active && symbol->getBorder().dashed);
	}
	const bool different_borders = border_active && different_borders_check->isChecked();
	for (auto different_border_widget : different_borders_widget_list)
	{
		different_border_widget->setVisible(different_borders);
		different_border_widget->setEnabled(different_borders);
	}
	for (auto border_dash_widget : right_border_widgets.dash_widget_list)
	{
		border_dash_widget->setVisible(different_borders);
		border_dash_widget->setEnabled(different_borders && symbol->getRightBorder().dashed);
	}
}

void LineSymbolSettings::updateContents()
{
	blockSignalsRecursively(this, true);
	width_edit->setValue(0.001 * symbol->getLineWidth());
	color_edit->setColor(symbol->getColor());
	
	minimum_length_edit->setValue(0.001 * symbol->minimum_length);
	line_cap_combo->setCurrentIndex(line_cap_combo->findData(symbol->cap_style));
	start_offset_edit->setValue(0.001 * symbol->start_offset);
	end_offset_edit->setValue(0.001 * symbol->end_offset);
	line_join_combo->setCurrentIndex(line_join_combo->findData(symbol->join_style));
	dashed_check->setChecked(symbol->dashed);
	border_check->setChecked(symbol->have_border_lines);
	different_borders_check->setChecked(symbol->areBordersDifferent());
	
	dash_length_edit->setValue(0.001 * symbol->dash_length);
	break_length_edit->setValue(0.001 * symbol->break_length);
	dash_group_combo->setCurrentIndex(dash_group_combo->findData(QVariant(symbol->dashes_in_group)));
	in_group_break_length_edit->setValue(0.001 * symbol->in_group_break_length);
	half_outer_dashes_check->setChecked(symbol->half_outer_dashes);
	
	mid_symbol_placement_combo->setCurrentIndex(mid_symbol_placement_combo->findData(symbol->getMidSymbolPlacement()));
	mid_symbol_per_spot_edit->setValue(symbol->mid_symbols_per_spot);
	mid_symbol_distance_edit->setValue(0.001 * symbol->mid_symbol_distance);
	
	segment_length_edit->setValue(0.001 * symbol->segment_length);
	end_length_edit->setValue(0.001 * symbol->end_length);
	show_at_least_one_symbol_check->setChecked(symbol->show_at_least_one_symbol);
	minimum_mid_symbol_count_edit->setValue(symbol->minimum_mid_symbol_count);
	minimum_mid_symbol_count_when_closed_edit->setValue(symbol->minimum_mid_symbol_count_when_closed);
	
	updateBorderContents(symbol->getBorder(), border_widgets);
	updateBorderContents(symbol->getRightBorder(), right_border_widgets);
	
	supress_dash_symbol_check->setChecked(symbol->getSuppressDashSymbolAtLineEnds());
	scale_dash_symbol_check->setChecked(symbol->getScaleDashSymbol());
	
	blockSignalsRecursively(this, false);
/*	
	PointSymbolEditorWidget* point_symbol_editor = nullptr;
	auto controller = dialog->getPreviewController();
	
	QList<PointSymbol*> point_symbols;
	point_symbols << symbol->getStartSymbol() << symbol->getMidSymbol() << symbol->getEndSymbol() << symbol->getDashSymbol();
	for (auto point_symbol : point_symbols)
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		addPropertiesGroup(point_symbol->getName(), point_symbol_editor);
		connect(point_symbol_editor, SIGNAL(symbolEdited()), this, SLOT(pointSymbolEdited()));
	}	*/

	updateStates();
}

void LineSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Line);
	
	SymbolPropertiesWidget::reset(symbol);
	
	auto old_symbol = this->symbol;
	this->symbol = reinterpret_cast<LineSymbol*>(symbol);
	
	PointSymbolEditorWidget* point_symbol_editor = nullptr;
	auto controller = dialog->getPreviewController();
	
	int current = currentIndex();
	setUpdatesEnabled(false);
	this->symbol->ensurePointSymbols(tr("Start symbol"), tr("Mid symbol"), tr("End symbol"), tr("Dash symbol"));
	for (auto point_symbol : { this->symbol->getStartSymbol(), this->symbol->getMidSymbol(), this->symbol->getEndSymbol(), this->symbol->getDashSymbol() })
	{
		point_symbol_editor = new PointSymbolEditorWidget(controller, point_symbol, 16);
		connect(point_symbol_editor, &PointSymbolEditorWidget::symbolEdited, this, &LineSymbolSettings::pointSymbolEdited);
		
		int index = indexOfPropertiesGroup(point_symbol->getName()); // existing symbol editor
		removePropertiesGroup(index);
		insertPropertiesGroup(index, point_symbol->getName(), point_symbol_editor);
		if (index == current)
			setCurrentIndex(current);
	}
	updateContents();
	setUpdatesEnabled(true);
	old_symbol->cleanupPointSymbols();
}


}  // namespace OpenOrienteering
