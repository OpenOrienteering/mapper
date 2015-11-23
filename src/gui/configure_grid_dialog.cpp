/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#include "configure_grid_dialog.h"

#include <limits>

#include <qmath.h>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../core/georeferencing.h"
#include "../map.h"
#include "../util.h"
#include "../util_gui.h"


ConfigureGridDialog::ConfigureGridDialog(QWidget* parent, const MapGrid& grid, bool grid_visible)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, result_grid(grid)
, grid_visible(grid_visible)
{
	setWindowTitle(tr("Configure grid"));
	
	show_grid_check = new QCheckBox(tr("Show grid"));
	snap_to_grid_check = new QCheckBox(tr("Snap to grid"));
	choose_color_button = new QPushButton(tr("Choose..."));
	
	display_mode_combo = new QComboBox();
	display_mode_combo->addItem(tr("All lines"), (int)MapGrid::AllLines);
	display_mode_combo->addItem(tr("Horizontal lines"), (int)MapGrid::HorizontalLines);
	display_mode_combo->addItem(tr("Vertical lines"), (int)MapGrid::VerticalLines);
	
	
	QGroupBox* alignment_group = new QGroupBox(tr("Alignment"));
	
	mag_north_radio = new QRadioButton(tr("Align with magnetic north"));
	grid_north_radio = new QRadioButton(tr("Align with grid north"));
	true_north_radio = new QRadioButton(tr("Align with true north"));
	
	QLabel* rotate_label = new QLabel(tr("Additional rotation (counter-clockwise):"));
	additional_rotation_edit = Util::SpinBox::create(Georeferencing::declinationPrecision(), -360, +360, trUtf8("°"));
	additional_rotation_edit->setWrapping(true);
	
	
	QGroupBox* position_group = new QGroupBox(tr("Positioning"));
	
	unit_combo = new QComboBox();
	unit_combo->addItem(tr("meters in terrain"), (int)MapGrid::MetersInTerrain);
	unit_combo->addItem(tr("millimeters on map"), (int)MapGrid::MillimetersOnMap);
	
	QLabel* horz_spacing_label = new QLabel(tr("Horizontal spacing:"));
	horz_spacing_edit = Util::SpinBox::create(1, 0.1, Util::InputProperties<MapCoordF>::max());
	QLabel* vert_spacing_label = new QLabel(tr("Vertical spacing:"));
	vert_spacing_edit = Util::SpinBox::create(1, 0.1, Util::InputProperties<MapCoordF>::max());
	
	origin_label = new QLabel();
	QLabel* horz_offset_label = new QLabel(tr("Horizontal offset:"));
	horz_offset_edit = Util::SpinBox::create(1, Util::InputProperties<MapCoordF>::min(), Util::InputProperties<MapCoordF>::max());
	QLabel* vert_offset_label = new QLabel(tr("Vertical offset:"));
	vert_offset_edit = Util::SpinBox::create(1, Util::InputProperties<MapCoordF>::min(), Util::InputProperties<MapCoordF>::max());

	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal);
	
	
	show_grid_check->setChecked(grid_visible);
	snap_to_grid_check->setChecked(grid.isSnappingEnabled());
	current_color = grid.getColor();
	display_mode_combo->setCurrentIndex(display_mode_combo->findData((int)grid.getDisplayMode()));
	if (grid.getAlignment() == MapGrid::MagneticNorth)
		mag_north_radio->setChecked(true);
	else if (grid.getAlignment() == MapGrid::GridNorth)
		grid_north_radio->setChecked(true);
	else // if (grid.getAlignment() == MapGrid::TrueNorth)
		true_north_radio->setChecked(true);
	additional_rotation_edit->setValue(grid.getAdditionalRotation() * 180 / M_PI);
	unit_combo->setCurrentIndex(unit_combo->findData((int)grid.getUnit()));
	horz_spacing_edit->setValue(grid.getHorizontalSpacing());
	vert_spacing_edit->setValue(grid.getVerticalSpacing());
	horz_offset_edit->setValue(grid.getHorizontalOffset());
	vert_offset_edit->setValue(-1 * grid.getVerticalOffset());
	
	
	QFormLayout* alignment_layout = new QFormLayout();
	alignment_layout->addRow(mag_north_radio);
	alignment_layout->addRow(grid_north_radio);
	alignment_layout->addRow(true_north_radio);
	alignment_layout->addRow(rotate_label, additional_rotation_edit);
	alignment_group->setLayout(alignment_layout);
	
	QFormLayout* position_layout = new QFormLayout();
	position_layout->addRow(tr("Unit:", "measurement unit"), unit_combo);
	position_layout->addRow(horz_spacing_label, horz_spacing_edit);
	position_layout->addRow(vert_spacing_label, vert_spacing_edit);
	position_layout->addRow(new QLabel(" "));
	position_layout->addRow(origin_label);
	position_layout->addRow(horz_offset_label, horz_offset_edit);
	position_layout->addRow(vert_offset_label, vert_offset_edit);
	position_group->setLayout(position_layout);
	
	QFormLayout* layout = new QFormLayout();
	layout->addRow(show_grid_check);
	layout->addRow(snap_to_grid_check);
	layout->addRow(tr("Line color:"), choose_color_button);
	layout->addRow(tr("Display:"), display_mode_combo);
	layout->addRow(new QLabel(" "));
	layout->addRow(alignment_group);
	layout->addRow(position_group);
	layout->addRow(button_box);
	setLayout(layout);
	
	updateStates();
	updateColorDisplay();
	
	connect(show_grid_check, SIGNAL(clicked(bool)), this, SLOT(updateStates()));
	connect(choose_color_button, SIGNAL(clicked(bool)), this, SLOT(chooseColor()));
	connect(display_mode_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateStates()));
	connect(mag_north_radio, SIGNAL(clicked(bool)), this, SLOT(updateStates()));
	connect(grid_north_radio, SIGNAL(clicked(bool)), this, SLOT(updateStates()));
	connect(true_north_radio, SIGNAL(clicked(bool)), this, SLOT(updateStates()));
	connect(unit_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateStates()));
	connect(button_box, SIGNAL(helpRequested()), this, SLOT(showHelp()));
	
	connect(button_box, SIGNAL(accepted()), this, SLOT(okClicked()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
}

void ConfigureGridDialog::chooseColor()
{
	qDebug() << qAlpha(current_color);
	QColor new_color = QColorDialog::getColor(current_color, this, tr("Choose grid line color"), QColorDialog::ShowAlphaChannel);
	if (new_color.isValid())
	{
		current_color = new_color.rgba();
		updateColorDisplay();
	}
}

void ConfigureGridDialog::updateColorDisplay()
{
	int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(icon_size, icon_size);
	pixmap.fill(current_color);
	QIcon icon(pixmap);
	choose_color_button->setIcon(icon);
}

void ConfigureGridDialog::okClicked()
{
	grid_visible = show_grid_check->isChecked();
	
	result_grid.setSnappingEnabled(snap_to_grid_check->isChecked());
	result_grid.setColor(current_color);
	result_grid.setDisplayMode((MapGrid::DisplayMode)display_mode_combo->itemData(display_mode_combo->currentIndex()).toInt());
	
	if (mag_north_radio->isChecked())
		result_grid.setAlignment(MapGrid::MagneticNorth);
	else if (grid_north_radio->isChecked())
		result_grid.setAlignment(MapGrid::GridNorth);
	else // if (true_north_radio->isChecked())
		result_grid.setAlignment(MapGrid::TrueNorth);
	result_grid.setAdditionalRotation(additional_rotation_edit->value() * M_PI / 180);
	
	result_grid.setUnit((MapGrid::Unit)unit_combo->itemData(unit_combo->currentIndex()).toInt());
	
	result_grid.setHorizontalSpacing(horz_spacing_edit->value());
	result_grid.setVerticalSpacing(vert_spacing_edit->value());
	result_grid.setHorizontalOffset(horz_offset_edit->value());
	result_grid.setVerticalOffset(-1 * vert_offset_edit->value());
	
	accept();
}

void ConfigureGridDialog::updateStates()
{
	MapGrid::DisplayMode display_mode = (MapGrid::DisplayMode)display_mode_combo->itemData(display_mode_combo->currentIndex()).toInt();
	choose_color_button->setEnabled(show_grid_check->isChecked());
	display_mode_combo->setEnabled(show_grid_check->isChecked());
	snap_to_grid_check->setEnabled(show_grid_check->isChecked());
	
	mag_north_radio->setEnabled(show_grid_check->isChecked());
	grid_north_radio->setEnabled(show_grid_check->isChecked());
	true_north_radio->setEnabled(show_grid_check->isChecked());
	additional_rotation_edit->setEnabled(show_grid_check->isChecked());
	
	unit_combo->setEnabled(show_grid_check->isChecked());
	
	QString unit_suffix = QString(" ") % ((unit_combo->itemData(unit_combo->currentIndex()).toInt() == (int)MapGrid::MetersInTerrain) ? tr("m", "meters") : tr("mm", "millimeters"));
	horz_spacing_edit->setEnabled(show_grid_check->isChecked() && display_mode != MapGrid::HorizontalLines);
	horz_spacing_edit->setSuffix(unit_suffix);
	vert_spacing_edit->setEnabled(show_grid_check->isChecked() && display_mode != MapGrid::VerticalLines);
	vert_spacing_edit->setSuffix(unit_suffix);
	
	QString origin_text = tr("Origin at: %1");
	if (mag_north_radio->isChecked() || true_north_radio->isChecked())
		origin_text = origin_text.arg(tr("paper coordinates origin"));
	else // if (grid_north_radio->isChecked())
		origin_text = origin_text.arg(tr("projected coordinates origin"));
	origin_label->setText(origin_text);
	
	horz_offset_edit->setEnabled(show_grid_check->isChecked() && display_mode != MapGrid::HorizontalLines);
	horz_offset_edit->setSuffix(unit_suffix);
	vert_offset_edit->setEnabled(show_grid_check->isChecked() && display_mode != MapGrid::VerticalLines);
	vert_offset_edit->setSuffix(unit_suffix);
}

void ConfigureGridDialog::showHelp()
{
	Util::showHelp(this, "grid.html");
}
