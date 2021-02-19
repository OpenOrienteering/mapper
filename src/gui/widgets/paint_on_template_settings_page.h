/*
 *    Copyright 2021 Libor Pecháček
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

#ifndef OPENORIENTEERING_PAINT_ON_TEMPLATE_SETTINGS_PAGE_H
#define OPENORIENTEERING_PAINT_ON_TEMPLATE_SETTINGS_PAGE_H

#include <functional>
#include <vector>

#include <QObject>
#include <QString>

#include "settings_page.h"

class QColor;
class QLineEdit;
class QTableWidget;
class QRadioButton;
class QToolButton;
class QWidget;


namespace OpenOrienteering {


class PaintOnTemplateSettingsPage : public SettingsPage
{
	Q_OBJECT

public:
	explicit PaintOnTemplateSettingsPage(QWidget* parent = nullptr);
	~PaintOnTemplateSettingsPage();
	QString title() const override;
	void apply() override;
	void reset() override;

public slots:
	void editColor(bool clicked);
	void moveColorUp(bool clicked);
	void moveColorDown(bool clicked);
	void dropColor(bool clicked);
	void addColor(bool clicked);

protected:
	QTableWidget* color_table = {};
	std::vector<QRadioButton*> preset_buttons;
	QLineEdit* custom_string_edit = {};
	QToolButton* delete_button;
	QToolButton* edit_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;

	void initializePage(const std::vector<QColor>& working_colors);
	void applyPresets();
	void updateCustomColorsString();
	std::vector<QColor> getWorkingColorsFromPage();

private:
	void moveColor(const std::function<bool (int)>& is_within_bounds, int amount);
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_PAINT_ON_TEMPLATE_SETTINGS_PAGE_H
