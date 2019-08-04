/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2013-2016  Kai Pastor
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

#ifndef OPENORIENTEERING_EDITOR_SETTINGS_PAGE_H
#define OPENORIENTEERING_EDITOR_SETTINGS_PAGE_H

#include <QObject>
#include <QString>

#include "settings_page.h"

class QCheckBox;
class QDoubleSpinBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QWidget;


namespace OpenOrienteering {

class EditorSettingsPage : public SettingsPage
{
Q_OBJECT
public:
	explicit EditorSettingsPage(QWidget* parent = nullptr);
	
	~EditorSettingsPage() override;
	
	QString title() const override;

	void apply() override;
	
	void reset() override;
	
protected:
	void updateWidgets();
	
private:
	QDoubleSpinBox* button_size;
	QSpinBox* icon_size;
	QCheckBox* antialiasing;
	QCheckBox* text_antialiasing;
	QSpinBox* tolerance;
	QSpinBox* snap_distance;
	QDoubleSpinBox* fixed_angle_stepping;
	QCheckBox* select_symbol_of_objects;
	QCheckBox* zoom_out_away_from_cursor;
	QCheckBox* draw_last_point_on_right_click;
	QCheckBox* keep_settings_of_closed_templates;
	
	QComboBox* edit_tool_delete_bezier_point_action;
	QComboBox* edit_tool_delete_bezier_point_action_alternative;
	
	QSpinBox* rectangle_helper_cross_radius;
	QCheckBox* rectangle_preview_line_width;
};


}  // namespace OpenOrienteering

#endif
