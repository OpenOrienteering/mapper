/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#include "map_grid.h"

#include <cassert>
#include <limits>

#include <qmath.h>
#include <QtWidgets>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/map_view.h"
#include "georeferencing.h"
#include "map.h"
#include "util.h"
#include "util_gui.h"

// ### MapGrid ###

MapGrid::MapGrid()
{
	snapping_enabled = true;
	color = qRgba(100, 100, 100, 128);
	display = AllLines;
	
	alignment = MagneticNorth;
	additional_rotation = 0;
	
	unit = MetersInTerrain;
	horz_spacing = 500;
	vert_spacing = 500;
	horz_offset = 0;
	vert_offset = 0;
}

void MapGrid::load(QIODevice* file, int version)
{
	Q_UNUSED(version);
	file->read((char*)&snapping_enabled, sizeof(bool));
	file->read((char*)&color, sizeof(QRgb));
	int temp;
	file->read((char*)&temp, sizeof(int));
	display = (DisplayMode)temp;
	file->read((char*)&temp, sizeof(int));
	alignment = (Alignment)temp;
	file->read((char*)&additional_rotation, sizeof(double));
	file->read((char*)&temp, sizeof(int));
	unit = (Unit)temp;
	file->read((char*)&horz_spacing, sizeof(double));
	file->read((char*)&vert_spacing, sizeof(double));
	file->read((char*)&horz_offset, sizeof(double));
	file->read((char*)&vert_offset, sizeof(double));
}

void MapGrid::save(QXmlStreamWriter& xml)
{
	xml.writeEmptyElement("grid");
	xml.writeAttribute("color", QColor(color).name());
	xml.writeAttribute("display", QString::number(display));
	xml.writeAttribute("alignment", QString::number(alignment));
	xml.writeAttribute("additional_rotation", QString::number(additional_rotation));
	xml.writeAttribute("unit", QString::number(unit));
	xml.writeAttribute("h_spacing", QString::number(horz_spacing));
	xml.writeAttribute("v_spacing", QString::number(vert_spacing));
	xml.writeAttribute("h_offset", QString::number(horz_offset));
	xml.writeAttribute("v_offset", QString::number(vert_offset));
	xml.writeAttribute("snapping_enabled", snapping_enabled ? "true" : "false");
}

void MapGrid::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == "grid");
	
	QXmlStreamAttributes attributes = xml.attributes();
	color = QColor(attributes.value("color").toString()).rgba();
	display = (MapGrid::DisplayMode) attributes.value("display").toString().toInt();
	alignment = (MapGrid::Alignment) attributes.value("alignment").toString().toInt();
	additional_rotation = attributes.value("additional_rotation").toString().toDouble();
	unit = (MapGrid::Unit) attributes.value("unit").toString().toInt();
	horz_spacing = attributes.value("h_spacing").toString().toDouble();
	vert_spacing = attributes.value("v_spacing").toString().toDouble();
	horz_offset = attributes.value("h_offset").toString().toDouble();
	vert_offset = attributes.value("v_offset").toString().toDouble();
	snapping_enabled = (attributes.value("snapping_enabled") == "true");
	xml.skipCurrentElement();
}

void MapGrid::draw(QPainter* painter, QRectF bounding_box, Map* map, bool on_screen)
{
	double final_horz_spacing, final_vert_spacing;
	double final_horz_offset, final_vert_offset;
	double final_rotation;
	calculateFinalParameters(final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, map);
	
	QPen pen(color);
	if (on_screen)
	{
		// zero-width cosmetic pen (effectively one pixel)
		pen.setWidth(0);
		pen.setCosmetic(true);
	}
	else
	{
		// 0.1 mm wide non-cosmetic pen
		pen.setWidthF(0.1f);
	}
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->setOpacity(qAlpha(color) / 255.0);
	
	ProcessLine process_line;
	process_line.painter = painter;
	
	if (display == AllLines)
		Util::gridOperation<ProcessLine>(bounding_box, final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, process_line);
	else if (display == HorizontalLines)
		Util::hatchingOperation<ProcessLine>(bounding_box, final_vert_spacing, final_vert_offset, final_rotation + M_PI / 2, process_line);
	else // if (display == VeritcalLines)
		Util::hatchingOperation<ProcessLine>(bounding_box, final_horz_spacing, final_horz_offset, final_rotation, process_line);
}

void MapGrid::calculateFinalParameters(double& final_horz_spacing, double& final_vert_spacing, double& final_horz_offset, double& final_vert_offset, double& final_rotation, Map* map)
{
	double factor = ((unit == MetersInTerrain) ? (1000.0 / map->getScaleDenominator()) : 1);
	final_horz_spacing = factor * horz_spacing;
	final_vert_spacing = factor * vert_spacing;
	final_horz_offset = factor * horz_offset;
	final_vert_offset = factor * vert_offset;
	final_rotation = additional_rotation + M_PI / 2;
	
	const Georeferencing& georeferencing = map->getGeoreferencing();
	if (alignment == GridNorth)
	{
		final_rotation += georeferencing.getGrivation() * M_PI / 180;
		
		// Shift origin to projected coordinates origin
		double prev_horz_offset = MapCoordF(0, -1).rotate(final_rotation).dot(MapCoordF(georeferencing.getMapRefPoint().xd(), -1 * georeferencing.getMapRefPoint().yd()));
		double target_horz_offset = MapCoordF(0, -1).rotate(additional_rotation + M_PI / 2).dot(MapCoordF(georeferencing.getProjectedRefPoint().x(), georeferencing.getProjectedRefPoint().y()));
		final_horz_offset -= factor * target_horz_offset - prev_horz_offset;
		
		double prev_vert_offset = MapCoordF(1, 0).rotate(final_rotation).dot(MapCoordF(georeferencing.getMapRefPoint().xd(), -1 * georeferencing.getMapRefPoint().yd()));
		double target_vert_offset = MapCoordF(1, 0).rotate(additional_rotation + M_PI / 2).dot(MapCoordF(georeferencing.getProjectedRefPoint().x(), georeferencing.getProjectedRefPoint().y()));
		final_vert_offset += factor * target_vert_offset - prev_vert_offset;
	}
	else if (alignment == TrueNorth)
		final_rotation += georeferencing.getDeclination() * M_PI / 180;
}

MapCoordF MapGrid::getClosestPointOnGrid(MapCoordF position, Map* map)
{
	double final_horz_spacing, final_vert_spacing;
	double final_horz_offset, final_vert_offset;
	double final_rotation;
	calculateFinalParameters(final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, map);
	
	position.rotate(final_rotation - M_PI / 2);
	return MapCoordF(qRound((position.getX() - final_horz_offset) / final_horz_spacing) * final_horz_spacing + final_horz_offset,
					 qRound((position.getY() - final_vert_offset) / final_vert_spacing) * final_vert_spacing + final_vert_offset).rotate(-1 * (final_rotation - M_PI / 2));
}

void MapGrid::ProcessLine::processLine(QPointF a, QPointF b)
{
	painter->drawLine(a, b);
}

// ### ConfigureGridDialog ###

ConfigureGridDialog::ConfigureGridDialog(QWidget* parent, Map* map, MapView* main_view) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), map(map), main_view(main_view)
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
	additional_rotation_edit = Util::SpinBox::create(1, -360, +360, trUtf8("°"));
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
	
	
	MapGrid& grid = map->getGrid();
	show_grid_check->setChecked(main_view->isGridVisible());
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
	main_view->setGridVisible(show_grid_check->isChecked());
	
	MapGrid& grid = map->getGrid();
	grid.setSnappingEnabled(snap_to_grid_check->isChecked());
	grid.setColor(current_color);
	grid.setDisplayMode((MapGrid::DisplayMode)display_mode_combo->itemData(display_mode_combo->currentIndex()).toInt());
	
	if (mag_north_radio->isChecked())
		grid.setAlignment(MapGrid::MagneticNorth);
	else if (grid_north_radio->isChecked())
		grid.setAlignment(MapGrid::GridNorth);
	else // if (true_north_radio->isChecked())
		grid.setAlignment(MapGrid::TrueNorth);
	grid.setAdditionalRotation(additional_rotation_edit->value() * M_PI / 180);
	
	grid.setUnit((MapGrid::Unit)unit_combo->itemData(unit_combo->currentIndex()).toInt());
	
	grid.setHorizontalSpacing(horz_spacing_edit->value());
	grid.setVerticalSpacing(vert_spacing_edit->value());
	grid.setHorizontalOffset(horz_offset_edit->value());
	grid.setVerticalOffset(-1 * vert_offset_edit->value());
	
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
