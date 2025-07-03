/*
 *    Copyright 2021, 2025 Libor Pecháček
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

#include "paint_on_template_settings_page.h"

#include <array>
#include <vector>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QColorDialog>  // IWYU pragma: keep
#include <QFlags>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <Qt>
#include <QtGlobal>

#include "gui/map/map_editor.h"
#include "gui/util_gui.h"
#include "gui/widgets/color_wheel_widget.h"  // IWYU pragma: keep
#include "gui/widgets/segmented_button_layout.h"
#include "gui/widgets/settings_page.h"
#include "settings.h"

class QWidget;


namespace OpenOrienteering {

namespace {

struct ColorPresetEntry {
	const char* name;
	const char* color_string;
};


constexpr std::array<ColorPresetEntry, 3> color_presets = {
    //: Paint on template color preset entry name.
    ColorPresetEntry {
        QT_TRANSLATE_NOOP("OpenOrienteering::PaintOnTemplateSettingsPage", "Traditional"),
        "FF0000,FFFF00,00FF00,DB00D8,0000FF,D15C00,000000"
    },
    //: Paint on template color preset entry name.
    ColorPresetEntry {
        QT_TRANSLATE_NOOP("OpenOrienteering::PaintOnTemplateSettingsPage", "Confetti Hybrid"),
        "000000,FAF607,00A400,B4FF00,0000FE,81EEF7,F6260F,FCB9F4,FAEFD2"
    },
    //: Paint on template color preset entry name.
    ColorPresetEntry {
        QT_TRANSLATE_NOOP("OpenOrienteering::PaintOnTemplateSettingsPage", "Guacamole Full"),
        "000000,FF0000,A24802,DCC3A8,0000FF,31DAE5,FFFF00,FFD000,78FF00,B0CF1D,446C55,ECA8EB,D918A8"
    },
};


void fillTableRow(QTableWidget * table, int row, QColor color)
{
	auto* text_cell = new QTableWidgetItem(color.name().right(6).toUpper());
	text_cell->setFlags(text_cell->flags() ^ Qt::ItemIsEditable);
	table->setItem(row, 0, text_cell);

	auto* color_cell = new QTableWidgetItem();
	color_cell->setFlags(color_cell->flags() ^ Qt::ItemIsEditable);
	color_cell->setBackground(QBrush(color));
	table->setItem(row, 1, color_cell);
}


QColor spawnColorDialog(const QColor &initial = Qt::white, QWidget *parent = nullptr)
{
#ifdef Q_OS_ANDROID
	        return ColorWheelDialog::getColor(initial, parent);
#else
	        return QColorDialog::getColor(initial, parent);
#endif
}

}  // anonymous namespace


PaintOnTemplateSettingsPage::PaintOnTemplateSettingsPage(QWidget* parent)
    : SettingsPage(parent)
{
	auto* layout = new QVBoxLayout(this);

	color_table = new QTableWidget(0, 2);
	color_table->verticalHeader()->hide();
	auto* header_view = color_table->horizontalHeader();
	header_view->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(1, QHeaderView::Stretch);
	header_view->setVisible(false);
	color_table->setSelectionMode(QAbstractItemView::SingleSelection);
	layout->addWidget(color_table);

	auto* add_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/plus.png")), tr("Add color..."));
	delete_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/minus.png")), tr("Remove"));
	delete_button->setEnabled(false);

	auto* add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(add_button);
	add_remove_layout->addWidget(delete_button);

	move_up_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/arrow-up.png")), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_up_button->setEnabled(false);
	move_down_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/arrow-down.png")), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	move_down_button->setEnabled(false);

	auto* up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);

	edit_button = Util::ToolButton::create(QIcon(QString::fromLatin1(":/images/settings.png")),
	                            ::OpenOrienteering::MapEditorController::tr("&Edit").remove(QLatin1Char('&')));
	edit_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	edit_button->setEnabled(false);

	auto* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	list_buttons_layout->addWidget(edit_button);
	list_buttons_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
	layout->addLayout(list_buttons_layout);

	connect(add_button, &QAbstractButton::clicked, this, &PaintOnTemplateSettingsPage::addColor);
	connect(color_table, &QTableWidget::itemSelectionChanged, this, [this]() {
		auto const enabled = !color_table->selectedItems().empty();
		delete_button->setEnabled(enabled);
		edit_button->setEnabled(enabled);
		move_up_button->setEnabled(enabled && color_table->currentRow() > 0);
		move_down_button->setEnabled(enabled && color_table->currentRow() < color_table->rowCount() - 1);
	});
	connect(edit_button, &QAbstractButton::clicked, this, &PaintOnTemplateSettingsPage::editColor);
	connect(delete_button, &QAbstractButton::clicked, this, &PaintOnTemplateSettingsPage::dropColor);

	auto* presets_box = new QGroupBox(tr("Available palette presets"), this);
	auto* presets_layout = new QGridLayout(presets_box);

	auto row = 0U;
	preset_buttons.reserve(color_presets.size() + 1);
	for (const auto& entry : color_presets)
	{
		auto* preset_button = new QRadioButton(tr(entry.name), presets_box);
		preset_buttons.emplace_back(preset_button);
		presets_layout->addWidget(preset_button, row++, 0, 1, 2);
	}

	auto* custom = new QRadioButton(tr("Custom string"), presets_box);
	preset_buttons.emplace_back(custom);
	presets_layout->addWidget(custom, row, 0, 1, 1);
	custom_string_edit = new QLineEdit(presets_box);
	presets_layout->addWidget(custom_string_edit, row++, 2, 1, 1);

	auto* apply_preset_button = new QPushButton(tr("Activate preset"), presets_box);
	presets_layout->addWidget(apply_preset_button, row, 0, 1, 1);
	auto* c_p_buttons_layout = new QHBoxLayout();
	c_p_buttons_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	auto* copy_preset_button = new QPushButton(tr("Copy"), presets_box);
	c_p_buttons_layout->addWidget(copy_preset_button);
	auto* paste_preset_button = new QPushButton(tr("Paste"), presets_box);
	c_p_buttons_layout->addWidget(paste_preset_button);
	presets_layout->addItem(c_p_buttons_layout, row, 2, 1, 1);

	layout->addWidget(presets_box);

	connect(move_up_button, &QAbstractButton::clicked, this, &PaintOnTemplateSettingsPage::moveColorUp);
	connect(move_down_button, &QAbstractButton::clicked, this, &PaintOnTemplateSettingsPage::moveColorDown);

	connect(copy_preset_button, &QPushButton::clicked,
	        this, [this]() {
		QApplication::clipboard()->setText(custom_string_edit->text());
	} );
	connect(paste_preset_button, &QPushButton::clicked,
	        this, [this]() { 
		custom_string_edit->setText(QApplication::clipboard()->text());
		preset_buttons.back()->setChecked(true);
	} );
	connect(apply_preset_button, &QPushButton::clicked,
	        this, &PaintOnTemplateSettingsPage::applyPresets);
	connect(custom_string_edit, &QLineEdit::textEdited,
	        this, [this]() { preset_buttons.back()->setChecked(true); });

	setLayout(layout);

	initializePage(Settings::getInstance().paintOnTemplateColors());
}


PaintOnTemplateSettingsPage::~PaintOnTemplateSettingsPage() = default;


QString PaintOnTemplateSettingsPage::title() const
{
	return tr("Paint on template");
}


std::vector<QColor> PaintOnTemplateSettingsPage::getWorkingColorsFromPage()
{
	auto new_colors = std::vector<QColor>();
	auto const color_count = color_table->rowCount();
	new_colors.reserve(color_count);

	for (auto row = 0; row < color_count; ++row)
		new_colors.emplace_back(color_table->item(row, 1)->background().color());

	return new_colors;
}


void PaintOnTemplateSettingsPage::apply()
{
	Settings::getInstance().setPaintOnTemplateColors(getWorkingColorsFromPage());
}


void PaintOnTemplateSettingsPage::reset()
{
	initializePage(Settings::getInstance().paintOnTemplateColors());
}


void PaintOnTemplateSettingsPage::initializePage(const std::vector<QColor>& working_colors)
{
	color_table->clear();
	color_table->setRowCount(working_colors.size());

	for (auto row = 0U; row < working_colors.size(); ++row)
		fillTableRow(color_table, row, working_colors[row]);

	updateCustomColorsString();
}


void PaintOnTemplateSettingsPage::applyPresets()
{
	auto button_num = 0U;
	for (const auto* preset_button : preset_buttons)
	{
		if (preset_button->isChecked())
			break;
		button_num++;
	}

	if (button_num < color_presets.size())
	{
		initializePage(Settings::colorsStringToVector(QLatin1String(color_presets[button_num].color_string)));
	}
	else if (button_num == color_presets.size() && !custom_string_edit->text().isEmpty())
	{
		initializePage(Settings::colorsStringToVector(custom_string_edit->text()));
	}
}


void PaintOnTemplateSettingsPage::updateCustomColorsString()
{
	custom_string_edit->setText(Settings::colorsVectorToString(getWorkingColorsFromPage()));
}


void PaintOnTemplateSettingsPage::addColor(bool clicked)
{
	Q_UNUSED(clicked);

	auto const new_color = spawnColorDialog({}, this);

	if (new_color.isValid())
	{
		auto current_row = color_table->currentRow();
		auto row = current_row >= 0 ? current_row+1 : color_table->rowCount();
		color_table->insertRow(row);
		fillTableRow(color_table, row, new_color);
		color_table->setCurrentCell(row, 0);
		updateCustomColorsString();
	}
}


void PaintOnTemplateSettingsPage::dropColor(bool clicked)
{
	Q_UNUSED(clicked);

	auto current_row = color_table->currentRow();

	if (current_row < 0)
		return;

	color_table->removeRow(current_row);
	color_table->setCurrentCell(-1, 0);

	updateCustomColorsString();
}


void PaintOnTemplateSettingsPage::moveColor(const std::function<bool(int)>& is_within_bounds,
                                           int amount)
{
	auto current_row = color_table->currentRow();

	if (!is_within_bounds(current_row))
		return;

	auto* const text_cell = color_table->takeItem(current_row, 0);
	auto* const color_cell = color_table->takeItem(current_row, 1);
	color_table->removeRow(current_row);

	auto const new_row = current_row + amount;
	color_table->insertRow(new_row);
	color_table->setItem(new_row, 0, text_cell);
	color_table->setItem(new_row, 1, color_cell);
	color_table->setCurrentCell(new_row, 0);

	updateCustomColorsString();
}


void PaintOnTemplateSettingsPage::moveColorUp(bool clicked)
{
	Q_UNUSED(clicked);

	moveColor([](int row_number)->bool { return row_number > 0; }, -1);
}

void PaintOnTemplateSettingsPage::moveColorDown(bool clicked)
{
	Q_UNUSED(clicked);

	moveColor([this](int row_number)->bool { return row_number < color_table->rowCount()-1; }, 1);
}


void PaintOnTemplateSettingsPage::editColor(bool clicked)
{
	Q_UNUSED(clicked);

	auto const color = color_table->item(color_table->currentRow(), 1)->background().color();

	auto const new_color = spawnColorDialog(color, this);

	if (new_color.isValid())
	{
		fillTableRow(color_table, color_table->currentRow(), new_color);
		updateCustomColorsString();
	}
}


}  // namespace OpenOrienteering
