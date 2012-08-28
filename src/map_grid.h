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


#ifndef _OPENORIENTEERING_MAP_DIALOG_GRID_H_
#define _OPENORIENTEERING_MAP_DIALOG_GRID_H_

#include <map>

#include <QDialog>

#include "map_coord.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QDoubleSpinBox;
class QRadioButton;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

class Map;
class MapView;


class MapGrid
{
public:
	enum Alignment
	{
		MagneticNorth = 0,
		GridNorth = 1,
		TrueNorth = 2
	};
	enum Unit
	{
		MillimetersOnMap = 0,
		MetersInTerrain = 1
	};
	enum DisplayMode
	{
		AllLines = 0,
		HorizontalLines = 1,
		VerticalLines = 2
	};
	
	MapGrid();
	
	void save(QIODevice* file);
	void load(QIODevice* file, int version);
	
	void draw(QPainter* painter, QRectF bounding_box, Map* map);
	
	/// Calculates the "final" parameters with the following properties:
	/// - spacings and offsets are in millimeters on the map
	/// - rotation is relative to the vector (1, 0) and counterclockwise
	void calculateFinalParameters(double& final_horz_spacing, double& final_vert_spacing,
								   double& final_horz_offset, double& final_vert_offset, double& final_rotation, Map* map);
	
	/// Returns the grid point which is closest to the given position
	MapCoordF getClosestPointOnGrid(MapCoordF position, Map* map);
	
	// Getters / Setters
	
	inline bool isSnappingEnabled() const {return snapping_enabled;}
	inline void setSnappingEnabled(bool enable) {snapping_enabled = enable;}
	inline QRgb getColor() const {return color;}
	inline void setColor(QRgb color) {this->color = color;}
	
	inline DisplayMode getDisplayMode() const {return display;}
	inline void setDisplayMode(DisplayMode mode) {display = mode;}
	
	inline Alignment getAlignment() const {return alignment;}
	inline void setAlignment(Alignment alignment) {this->alignment = alignment;}
	inline double getAdditionalRotation() const {return additional_rotation;}
	inline void setAdditionalRotation(double rotation) {additional_rotation = rotation;}
	
	inline Unit getUnit() const {return unit;}
	inline void setUnit(Unit unit) {this->unit = unit;}
	inline double getHorizontalSpacing() const {return horz_spacing;}
	inline void setHorizontalSpacing(double spacing) {horz_spacing = spacing;}
	inline double getVerticalSpacing() const {return vert_spacing;}
	inline void setVerticalSpacing(double spacing) {vert_spacing = spacing;}
	inline double getHorizontalOffset() const {return horz_offset;}
	inline void setHorizontalOffset(double offset) {horz_offset = offset;}
	inline double getVerticalOffset() const {return vert_offset;}
	inline void setVerticalOffset(double offset) {vert_offset = offset;}
	
private:
	bool snapping_enabled;
	QRgb color;
	DisplayMode display;
	
	Alignment alignment;
	double additional_rotation;
	
	Unit unit;
	double horz_spacing;
	double vert_spacing;
	double horz_offset;
	double vert_offset;
	
	struct ProcessLine
	{
		QPainter* painter;
		void processLine(QPointF a, QPointF b);
	};
};


class ConfigureGridDialog : public QDialog
{
Q_OBJECT
public:
	ConfigureGridDialog(QWidget* parent, Map* map, MapView* main_view);
	
private slots:
	void chooseColor();
	void updateColorDisplay();
	void okClicked();
	void updateStates();
	
private:
	QCheckBox* show_grid_check;
	QCheckBox* snap_to_grid_check;
	QPushButton* choose_color_button;
	QRgb current_color;
	QComboBox* display_mode_combo;

	QRadioButton* mag_north_radio;
	QRadioButton* grid_north_radio;
	QRadioButton* true_north_radio;
	QDoubleSpinBox* additional_rotation_edit;
	
	QComboBox* unit_combo;
	QDoubleSpinBox* horz_spacing_edit;
	QDoubleSpinBox* vert_spacing_edit;
	QLabel* origin_label;
	QDoubleSpinBox* horz_offset_edit;
	QDoubleSpinBox* vert_offset_edit;
	
	Map* map;
	MapView* main_view;
};

#endif
