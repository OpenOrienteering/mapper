/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2016, 2017 Kai Pastor
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

#include <Qt>
#include <QtGlobal>
#include <QtMath>
#include <QAbstractButton>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFlags>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLatin1Char>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QStyle>
#include <QVariant>

#include "core/georeferencing.h"
#include "core/map.h"
#include "gui/util_gui.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

class MapCoordF;


ConfigureGridDialog::ConfigureGridDialog(QWidget* parent, const Map& map, bool grid_visible)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, map(map)
, grid(map.getGrid())
, grid_visible(grid_visible)
, current_color(grid.getColor())
, current_unit(grid.getUnit())
{
	setWindowTitle(tr("Configure grid"));
	
	show_grid_check = new QCheckBox(tr("Show grid"));
	snap_to_grid_check = new QCheckBox(tr("Snap to grid"));
	choose_color_button = new QPushButton(tr("Choose..."));
	
	display_mode_combo = new QComboBox();
	display_mode_combo->addItem(tr("All lines"), int(MapGrid::AllLines));
	display_mode_combo->addItem(tr("Horizontal lines"), int(MapGrid::HorizontalLines));
	display_mode_combo->addItem(tr("Vertical lines"), int(MapGrid::VerticalLines));
	
	mag_north_radio = new QRadioButton(tr("Align with magnetic north"));
	grid_north_radio = new QRadioButton(tr("Align with grid north"));
	true_north_radio = new QRadioButton(tr("Align with true north"));
	
	auto rotate_label = new QLabel(tr("Additional rotation (counter-clockwise):"));
	additional_rotation_edit = Util::SpinBox::create(Georeferencing::declinationPrecision(), -360, +360, trUtf8("°"));
	additional_rotation_edit->setWrapping(true);
	
	
	unit_combo = new QComboBox();
	unit_combo->addItem(tr("meters in terrain"), int(MapGrid::MetersInTerrain));
	unit_combo->addItem(tr("millimeters on map"), int(MapGrid::MillimetersOnMap));
	
	auto horz_spacing_label = new QLabel(tr("Horizontal spacing:"));
	horz_spacing_edit = Util::SpinBox::create(1, 0.1, Util::InputProperties<MapCoordF>::max());
	auto vert_spacing_label = new QLabel(tr("Vertical spacing:"));
	vert_spacing_edit = Util::SpinBox::create(1, 0.1, Util::InputProperties<MapCoordF>::max());
	
	origin_label = new QLabel();
	auto horz_offset_label = new QLabel(tr("Horizontal offset:"));
	horz_offset_edit = Util::SpinBox::create(1, Util::InputProperties<MapCoordF>::min(), Util::InputProperties<MapCoordF>::max());
	auto vert_offset_label = new QLabel(tr("Vertical offset:"));
	vert_offset_edit = Util::SpinBox::create(1, Util::InputProperties<MapCoordF>::min(), Util::InputProperties<MapCoordF>::max());

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal);
	
	
	show_grid_check->setChecked(grid_visible);
	snap_to_grid_check->setChecked(grid.isSnappingEnabled());
	display_mode_combo->setCurrentIndex(display_mode_combo->findData(int(grid.getDisplayMode())));
	if (grid.getAlignment() == MapGrid::MagneticNorth)
		mag_north_radio->setChecked(true);
	else if (grid.getAlignment() == MapGrid::GridNorth)
		grid_north_radio->setChecked(true);
	else // if (grid.getAlignment() == MapGrid::TrueNorth)
		true_north_radio->setChecked(true);
	additional_rotation_edit->setValue(grid.getAdditionalRotation() * 180 / M_PI);
	unit_combo->setCurrentIndex(unit_combo->findData(current_unit));
	horz_spacing_edit->setValue(grid.getHorizontalSpacing());
	vert_spacing_edit->setValue(grid.getVerticalSpacing());
	horz_offset_edit->setValue(grid.getHorizontalOffset());
	vert_offset_edit->setValue(-1 * grid.getVerticalOffset());
	
	auto layout = new QFormLayout();
	layout->addRow(show_grid_check);
	layout->addRow(snap_to_grid_check);
	layout->addRow(tr("Line color:"), choose_color_button);
	layout->addRow(tr("Display:"), display_mode_combo);
	layout->addItem(Util::SpacerItem::create(this));
	
	layout->addRow(Util::Headline::create(tr("Alignment")));
	layout->addRow(mag_north_radio);
	layout->addRow(grid_north_radio);
	layout->addRow(true_north_radio);
	layout->addRow(rotate_label, additional_rotation_edit);
	layout->addItem(Util::SpacerItem::create(this));
	
	layout->addRow(Util::Headline::create(tr("Positioning")));
	layout->addRow(tr("Unit:", "measurement unit"), unit_combo);
	layout->addRow(horz_spacing_label, horz_spacing_edit);
	layout->addRow(vert_spacing_label, vert_spacing_edit);
	layout->addItem(Util::SpacerItem::create(this));
	layout->addRow(origin_label);
	layout->addRow(horz_offset_label, horz_offset_edit);
	layout->addRow(vert_offset_label, vert_offset_edit);
	layout->addItem(Util::SpacerItem::create(this));
	
	layout->addRow(button_box);
	setLayout(layout);
	
	updateStates();
	updateColorDisplay();
	
	connect(show_grid_check, &QAbstractButton::clicked, this, &ConfigureGridDialog::updateStates);
	connect(choose_color_button, &QAbstractButton::clicked, this, &ConfigureGridDialog::chooseColor);
	connect(display_mode_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigureGridDialog::updateStates);
	connect(mag_north_radio, &QAbstractButton::clicked, this, &ConfigureGridDialog::updateStates);
	connect(grid_north_radio, &QAbstractButton::clicked, this, &ConfigureGridDialog::updateStates);
	connect(true_north_radio, &QAbstractButton::clicked, this, &ConfigureGridDialog::updateStates);
	connect(unit_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigureGridDialog::unitChanged);
	connect(button_box, &QDialogButtonBox::helpRequested, this, &ConfigureGridDialog::showHelp);
	
	connect(button_box, &QDialogButtonBox::accepted, this, &ConfigureGridDialog::okClicked);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ConfigureGridDialog::~ConfigureGridDialog() = default;



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

void ConfigureGridDialog::unitChanged(int index)
{
	auto unit = MapGrid::Unit(unit_combo->itemData(index).toInt());
	if (unit != current_unit)
	{
		current_unit = unit;
		double factor = 1.0;
		switch (current_unit)
		{
		case MapGrid::MetersInTerrain:
			factor = 0.001 * map.getScaleDenominator();
			break;
		case MapGrid::MillimetersOnMap:
			factor = 1000.0 / map.getScaleDenominator();
		    break;
		default:
			Q_ASSERT(!"Illegal unit");
		}
		
		for (auto editor : { horz_spacing_edit, vert_spacing_edit, horz_offset_edit, vert_offset_edit })
		{
			editor->setValue(editor->value() * factor);
		}
	}
	updateStates();
}

void ConfigureGridDialog::okClicked()
{
	grid_visible = show_grid_check->isChecked();
	
	grid.setSnappingEnabled(snap_to_grid_check->isChecked());
	grid.setColor(current_color);
	grid.setDisplayMode(MapGrid::DisplayMode(display_mode_combo->itemData(display_mode_combo->currentIndex()).toInt()));
	
	if (mag_north_radio->isChecked())
		grid.setAlignment(MapGrid::MagneticNorth);
	else if (grid_north_radio->isChecked())
		grid.setAlignment(MapGrid::GridNorth);
	else // if (true_north_radio->isChecked())
		grid.setAlignment(MapGrid::TrueNorth);
	grid.setAdditionalRotation(additional_rotation_edit->value() * M_PI / 180);
	
	grid.setUnit(current_unit);
	
	grid.setHorizontalSpacing(horz_spacing_edit->value());
	grid.setVerticalSpacing(vert_spacing_edit->value());
	grid.setHorizontalOffset(horz_offset_edit->value());
	grid.setVerticalOffset(-1 * vert_offset_edit->value());
	
	accept();
}

void ConfigureGridDialog::updateStates()
{
	MapGrid::DisplayMode display_mode = MapGrid::DisplayMode(display_mode_combo->itemData(display_mode_combo->currentIndex()).toInt());
	choose_color_button->setEnabled(show_grid_check->isChecked());
	display_mode_combo->setEnabled(show_grid_check->isChecked());
	snap_to_grid_check->setEnabled(show_grid_check->isChecked());
	
	mag_north_radio->setEnabled(show_grid_check->isChecked());
	grid_north_radio->setEnabled(show_grid_check->isChecked());
	true_north_radio->setEnabled(show_grid_check->isChecked());
	additional_rotation_edit->setEnabled(show_grid_check->isChecked());
	
	unit_combo->setEnabled(show_grid_check->isChecked());
	
	QString unit_suffix = QLatin1Char(' ') + ((current_unit == MapGrid::MetersInTerrain) ? tr("m", "meters") : tr("mm", "millimeters"));
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


}  // namespace OpenOrienteering
