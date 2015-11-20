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


#include "template_dock_widget.h"

#include "georeferencing.h"
#include "gui/main_window.h"
#include "gui/widgets/segmented_button_layout.h"
#include "map.h"
#include "map_editor.h"
#include "map_widget.h"
#include "object.h"
#include "settings.h"
#include "template.h"
#include "template_adjust.h"
#include "template_map.h"
#include "template_position_dock_widget.h"
#include "template_tool_move.h"
#include "util.h"
#include "util/item_delegates.h"

// TODO: Review formatting et al.

// ### ApplyTemplateTransform ###

/**
 * A local functor which is used when importing map templates into the current map.
 */
struct ApplyTemplateTransform
{
	inline ApplyTemplateTransform(const TemplateTransform& transform) : transform(transform) {}
	inline bool operator()(Object* object, MapPart* part, int object_index) const
	{
		Q_UNUSED(part);
		Q_UNUSED(object_index);
		object->rotate(transform.template_rotation);
		object->scale(transform.template_scale_x, transform.template_scale_y);
		object->move(transform.template_x, transform.template_y);
		object->update(true, true);
		return true;
	}
private:
	const TemplateTransform& transform;
};



// ### TemplateWidget ###

TemplateWidget::TemplateWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget(parent), 
  map(map), 
  main_view(main_view), 
  controller(controller)
{
	react_to_changes = true;
	
	this->setWhatsThis("<a href=\"templates.html#setup\">See more</a>");
	
	// Wrap the checkbox in a widget and layout to force a margin.
	QLayout* all_hidden_layout = new QHBoxLayout();
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	all_hidden_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		0, // Covered by the main layout's spacing.
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	QWidget* all_hidden_widget = new QWidget();
	all_hidden_widget->setLayout(all_hidden_layout);
	// Reuse the translation from MapEditorController action.
	all_hidden_check = new QCheckBox(MapEditorController::tr("Hide all templates"));
	all_hidden_layout->addWidget(all_hidden_check);
	
	// Template table
	template_table = new QTableWidget(map->getNumTemplates() + 1, 4);
	template_table->installEventFilter(this);
	template_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	template_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	template_table->setSelectionMode(QAbstractItemView::SingleSelection);
	template_table->setHorizontalHeaderLabels(QStringList() << "" << tr("Opacity") << tr("Group") << tr("Filename"));
	template_table->verticalHeader()->setVisible(false);
#if 1
	// Template grouping is not yet implemented.
	template_table->hideColumn(2);
#endif
	
	template_table->horizontalHeaderItem(0)->setData(Qt::ToolTipRole, tr("Show"));
	QCheckBox header_check;
	QSize header_check_size(header_check.sizeHint());
	if (header_check_size.isValid())
	{
		header_check.setChecked(true);
		header_check.setEnabled(false);
		QPixmap pixmap(header_check_size);
		header_check.render(&pixmap);
		template_table->horizontalHeaderItem(0)->setData(Qt::DecorationRole, pixmap);
	}
	
	percentage_delegate = new PercentageDelegate(this, 5);
	template_table->setItemDelegateForColumn(1, percentage_delegate);
	
	QHeaderView* header_view = template_table->horizontalHeader();
	for (int i = 0; i < 3; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(3, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	for (int i = 0; i < map->getNumTemplates() + 1; ++i)
		addRow(i);
	
	all_templates_layout = new QVBoxLayout();
	all_templates_layout->setMargin(0);
	all_templates_layout->addWidget(all_hidden_widget);
	all_templates_layout->addWidget(template_table, 1);
	
	QMenu* new_button_menu = new QMenu(this);
	(void) new_button_menu->addAction(QIcon(":/images/open.png"), tr("Open..."), this, SLOT(openTemplate()));
	new_button_menu->addAction(controller->getAction("reopentemplate"));
	duplicate_action = new_button_menu->addAction(QIcon(":/images/tool-duplicate.png"), tr("Duplicate"), this, SLOT(duplicateTemplate()));
#if 0
	current_action = new_button_menu->addAction(tr("Sketch"));
	current_action->setDisabled(true);
	current_action = new_button_menu->addAction(tr("GPS"));
	current_action->setDisabled(true);
#endif
	
	QToolButton* new_button = newToolButton(QIcon(":/images/plus.png"), tr("Add template..."));
	new_button->setPopupMode(QToolButton::DelayedPopup); // or MenuButtonPopup
	new_button->setMenu(new_button_menu);
	
	delete_button = newToolButton(QIcon(":/images/minus.png"), QString());
	updateDeleteButtonText();
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(updateDeleteButtonText()));
	
	SegmentedButtonLayout* add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(new_button);
	add_remove_layout->addWidget(delete_button);
	
	move_up_button = newToolButton(QIcon(":/images/arrow-up.png"), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_down_button = newToolButton(QIcon(":/images/arrow-down.png"), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	
	SegmentedButtonLayout* up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);
	
	// TODO: Fix string
	georef_button = newToolButton(QIcon(":/images/grid.png"), tr("Georeferenced: %1").remove(": %1"));
	georef_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	georef_button->setCheckable(true);
	georef_button->setChecked(true);
	georef_button->setEnabled(false); // TODO
	move_by_hand_action = new QAction(QIcon(":/images/move.png"), tr("Move by hand"), this);
	move_by_hand_action->setCheckable(true);
	move_by_hand_button = newToolButton(move_by_hand_action->icon(), move_by_hand_action->text());
	move_by_hand_button->setDefaultAction(move_by_hand_action);
	adjust_button = newToolButton(QIcon(":/images/georeferencing.png"), tr("Adjust..."));
	adjust_button->setCheckable(true);
	
	QMenu* edit_menu = new QMenu(this);
	position_action = edit_menu->addAction(tr("Positioning..."));
	position_action->setCheckable(true);
	import_action =  edit_menu->addAction(tr("Import and remove"), this, SLOT(importClicked()));
	
	edit_button = newToolButton(QIcon(":/images/settings.png"), QApplication::translate("MapEditorController", "&Edit").remove(QChar('&')));
	edit_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	edit_button->setPopupMode(QToolButton::InstantPopup);
	edit_button->setMenu(edit_menu);
	
	// The buttons row layout
	QBoxLayout* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	list_buttons_layout->addWidget(adjust_button);
	list_buttons_layout->addWidget(move_by_hand_button);
	list_buttons_layout->addWidget(georef_button);
	list_buttons_layout->addWidget(edit_button);
	
	list_buttons_group = new QWidget();
	list_buttons_group->setLayout(list_buttons_layout);
	
	QToolButton* help_button = newToolButton(QIcon(":/images/help.png"), tr("Help"));
	help_button->setAutoRaise(true);
	
	QBoxLayout* all_buttons_layout = new QHBoxLayout();
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		0, // Covered by the main layout's spacing.
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(list_buttons_group);
	all_buttons_layout->addWidget(new QLabel("   "), 1);
	all_buttons_layout->addWidget(help_button);
	
	all_templates_layout->addLayout(all_buttons_layout);
	setLayout(all_templates_layout);
	
	//group_button = new QPushButton(QIcon(":/images/group.png"), tr("(Un)group"));
	/*more_button = new QToolButton();
	more_button->setText(tr("More..."));
	more_button->setPopupMode(QToolButton::InstantPopup);
	more_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	QMenu* more_button_menu = new QMenu(more_button);
	more_button_menu->addAction(QIcon(":/images/window-new.png"), tr("Numeric transformation window"));
	more_button_menu->addAction(tr("Set transparent color..."));
	more_button_menu->addAction(tr("Trace lines..."));
	more_button->setMenu(more_button_menu);*/
	
	updateButtons();
	
	setAllTemplatesHidden(main_view->areAllTemplatesHidden());
	
	// Connections
	connect(all_hidden_check, SIGNAL(toggled(bool)), controller, SLOT(hideAllTemplates(bool)));
	
	connect(template_table, SIGNAL(cellChanged(int,int)), this, SLOT(cellChange(int,int)));
	connect(template_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateButtons()));
	connect(template_table, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentCellChange(int,int,int,int)));
	connect(template_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClick(int,int)));
	
// 	connect(new_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(newTemplate(QAction*)));
	
	connect(new_button, SIGNAL(clicked(bool)), this, SLOT(openTemplate()));
	connect(delete_button, SIGNAL(clicked(bool)), this, SLOT(deleteTemplate()));
	connect(move_up_button, SIGNAL(clicked(bool)), this, SLOT(moveTemplateUp()));
	connect(move_down_button, SIGNAL(clicked(bool)), this, SLOT(moveTemplateDown()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	connect(move_by_hand_action, SIGNAL(triggered(bool)), this, SLOT(moveByHandClicked(bool)));
	connect(adjust_button, SIGNAL(clicked(bool)), this, SLOT(adjustClicked(bool)));
	connect(position_action, SIGNAL(triggered(bool)), this, SLOT(positionClicked(bool)));
	
	//connect(group_button, SIGNAL(clicked(bool)), this, SLOT(groupClicked()));
	//connect(more_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(moreActionClicked(QAction*)));
	
	connect(map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
	connect(controller, SIGNAL(templatePositionDockWidgetClosed(Template*)), this, SLOT(templatePositionDockWidgetClosed(Template*)));
}

TemplateWidget::~TemplateWidget()
{
	; // Nothing
}

QToolButton* TemplateWidget::newToolButton(const QIcon& icon, const QString& text)
{
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setIcon(icon);
	button->setText(text);
	button->setWhatsThis("<a href=\"templates.html#setup\">See more</a>");
	return button;
}

// slot
void TemplateWidget::setAllTemplatesHidden(bool value)
{
	all_hidden_check->setChecked(value);
	
	bool enabled = !value;
	template_table->setEnabled(enabled);
	list_buttons_group->setEnabled(enabled);
	updateButtons();
}

void TemplateWidget::addTemplateAt(Template* new_template, int pos)
{
	/*int row;
	if (pos >= 0)
		row = template_table->rowCount() - 1 - ((pos >= map->getFirstFrontTemplate()) ? (pos + 1) : pos);
	else
		row = template_table->rowCount() - 1 - map->getFirstFrontTemplate();*/
	
	if (pos < map->getFirstFrontTemplate())
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() + 1);
	if (pos < 0)
		pos = map->getFirstFrontTemplate() - 1;
	
	// Add template and make it visible in the currently active view; TODO: currently, it is made visible in the main view -> support multiple views
	map->addTemplate(new_template, pos, main_view);
	map->setTemplateAreaDirty(pos);
	
	map->setTemplatesDirty();
}

Template* TemplateWidget::showOpenTemplateDialog(QWidget* dialog_parent, MapEditorController* controller)
{
	QSettings settings;
	QString template_directory = settings.value("templateFileDirectory", QDir::homePath()).toString();
	
	QString path = QFileDialog::getOpenFileName(dialog_parent, tr("Open image, GPS track or DXF file"), template_directory, QString("%1 (*.omap *.xmap *.ocd *.bmp *.jpg *.jpeg *.gif *.png *.tif *.tiff *.gpx *.dxf *.osm);;%2 (*.*)").arg(tr("Template files")).arg(tr("All files")));
	path = QFileInfo(path).canonicalFilePath();
	if (path.isEmpty())
		return NULL;
	
	settings.setValue("templateFileDirectory", QFileInfo(path).canonicalPath());
	
	Template* new_temp = Template::templateForFile(path, controller->getMap());
	if (!new_temp)
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot open template:\n%1\n\nFile format not recognized.").arg(path));
		return NULL;
	}
	
	if (!new_temp->preLoadConfiguration(dialog_parent))
	{
		delete new_temp;
		return NULL;
	}
	
	if (!new_temp->loadTemplateFile(true))
	{
		QMessageBox::warning(dialog_parent, tr("Error"), tr("Cannot open template:\n%1\n\nFailed to load template. Does the file exist and is it valid?").arg(path));
		delete new_temp;
		return NULL;
	}
	
	bool center_in_view = true;
	if (!new_temp->postLoadConfiguration(dialog_parent, center_in_view))
	{
		delete new_temp;
		return NULL;
	}
	
	// If the template is not georeferenced, position it at the viewport midpoint
	if (!new_temp->isTemplateGeoreferenced() && center_in_view)
	{
		MapView* main_view = controller->getMainWidget()->getMapView();
		QPointF center = new_temp->calculateTemplateBoundingBox().center();
		new_temp->setTemplateX(main_view->getPositionX() - qRound64(1000 * center.x()));
		new_temp->setTemplateY(main_view->getPositionY() - qRound64(1000 * center.y()));
	}
	
	return new_temp;
}

bool TemplateWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == template_table)
	{
		if ( event->type() == QEvent::KeyPress &&
		     static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space )
		{
			int row = template_table->currentRow();
			if (row >= 0 && template_table->item(row, 1)->flags().testFlag(Qt::ItemIsEnabled))
			{
				bool is_checked = template_table->item(row, 0)->checkState() != Qt::Unchecked;
				template_table->item(row, 0)->setCheckState(is_checked ? Qt::Unchecked : Qt::Checked);
			}
			return true;
		}
		
		if ( event->type() == QEvent::KeyRelease &&
		     static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space )
		{
			return true;
		}
	}
	
	return false;
}

void TemplateWidget::newTemplate(QAction* action)
{
	if (action->text() == tr("Sketch"))
	{
		// TODO
	}
	else if (action->text() == tr("GPS"))
	{
		// TODO
	}
}

void TemplateWidget::openTemplate()
{
	Template* new_template = showOpenTemplateDialog(window(), controller);
	if (!new_template)
		return;
	
	int pos;
	int row = template_table->currentRow();
	if (row < 0)
		pos = -1;
	else
		pos = posFromRow(row);
	
	addTemplateAt(new_template, pos);
}

void TemplateWidget::deleteTemplate()
{
	int pos = posFromRow(template_table->currentRow());
	Q_ASSERT(pos >= 0);
	
	map->setTemplateAreaDirty(pos);
	
	if (Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool())
		map->closeTemplate(pos);
	else
		map->deleteTemplate(pos);
	
	{
		bool react_to_changes = this->react_to_changes;
		this->react_to_changes = false;
		template_table->removeRow(template_table->currentRow());
		this->react_to_changes = react_to_changes;
	}
		
	if (pos < map->getFirstFrontTemplate())
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() - 1);
	
	map->setTemplatesDirty();
	
	template_table->selectRow(template_table->currentRow());
}

void TemplateWidget::duplicateTemplate()
{
	int row = template_table->currentRow();
	Q_ASSERT(row >= 0);
	int pos = posFromRow(row);
	Q_ASSERT(pos >= 0);
	
	Template* new_template = map->getTemplate(pos)->duplicate();
	addTemplateAt(new_template, pos);
}

void TemplateWidget::moveTemplateUp()
{
	int row = template_table->currentRow();
	Q_ASSERT(row >= 1);
	if (!(row >= 1)) return; // in release mode
	
	int cur_pos = posFromRow(row);
	int above_pos = posFromRow(row - 1);
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(above_pos);
	
	if (cur_pos < 0)
	{
		// Moving the map layer up
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() + 1);
	}
	else if (above_pos < 0)
	{
		// Moving something above the map layer
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() - 1);
	}
	else
	{
		// Exchanging two templates
		Template* above_template = map->getTemplate(above_pos);
		Template* cur_template = map->getTemplate(cur_pos);
		map->setTemplate(cur_template, above_pos);
		map->setTemplate(above_template, cur_pos);
	}
	
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(above_pos);
	updateRow(row - 1);
	updateRow(row);
	
	{
		bool react_to_changes = this->react_to_changes;
		this->react_to_changes = false;
		template_table->setCurrentCell(row - 1, template_table->currentColumn());
		this->react_to_changes = react_to_changes;
	}
	updateButtons();
	map->setTemplatesDirty();
}

void TemplateWidget::moveTemplateDown()
{
	int row = template_table->currentRow();
	Q_ASSERT(row >= 0);
	if (!(row >= 0)) return; // in release mode
	Q_ASSERT(row < template_table->rowCount() - 1);
	if (!(row < template_table->rowCount() - 1)) return; // in release mode
	
	int cur_pos = posFromRow(row);
	int below_pos = posFromRow(row + 1);
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(below_pos);
	
	if (cur_pos < 0)
	{
		// Moving the map layer down
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() - 1);
	}
	else if (below_pos < 0)
	{
		// Moving something below the map layer
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() + 1);
	}
	else
	{
		// Exchanging two templates
		Template* below_template = map->getTemplate(below_pos);
		Template* cur_template = map->getTemplate(cur_pos);
		map->setTemplate(cur_template, below_pos);
		map->setTemplate(below_template, cur_pos);
	}
	
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(below_pos);
	updateRow(row + 1);
	updateRow(row);
	
	{
		bool react_to_changes = this->react_to_changes;
		this->react_to_changes = false;
		template_table->setCurrentCell(row + 1, template_table->currentColumn());
		this->react_to_changes = react_to_changes;
	}
	updateButtons();
	map->setTemplatesDirty();
}

void TemplateWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "templates.html", "setup");
}

void TemplateWidget::cellChange(int row, int column)
{
	if (!react_to_changes)
		return;
	
	int pos = posFromRow(row);
	if (pos >= 0)
	{
		Template* temp = (row >= 0) ? map->getTemplate(pos) : NULL;
		if (!temp)
			return;
		
		TemplateVisibility* vis = main_view->getTemplateVisibility(temp);
		QString text = template_table->item(row, column)->text().trimmed();
		
		react_to_changes = false;
		
		if (column == 0)
		{
			if (temp->getTemplateState() != Template::Invalid)
			{
				bool visible_new = template_table->item(row, column)->checkState() != Qt::Unchecked;
				if (!visible_new)
					map->setTemplateAreaDirty(pos);
				
				vis->visible = visible_new;
				
				if (visible_new)
					map->setTemplateAreaDirty(pos);
			}
			updateRow(row);
			updateButtons();
		}
		else if (column == 1)
		{
			float opacity = template_table->item(row, column)->data(Qt::DisplayRole).toFloat();
			if (opacity <= 0.0f)
			{
				map->setTemplateAreaDirty(pos);
				vis->opacity = qMax(0.0f, opacity);
			}
			else
			{
				vis->opacity = qMin(1.0f, opacity);
				map->setTemplateAreaDirty(pos);
			}
			
			if (temp->getTemplateState() != Template::Invalid)
			{
				template_table->item(row, 1)->setData(Qt::DecorationRole, QColor::fromCmykF(0.0f, 0.0f, 0.0f, vis->opacity));
			}
		}
		else if (column == 2)
		{
			bool ok = true;
			int ivalue = text.toInt(&ok);
			
			if (text.isEmpty())
			{
				temp->setTemplateGroup(-1);
			}
			else if (!ok)
			{
				QMessageBox::warning(window(), tr("Error"), tr("Please enter a valid integer number to set a group or leave the field empty to ungroup the template!"));
				template_table->item(row, column)->setText(QString::number(temp->getTemplateGroup()));
			}
			else
				temp->setTemplateGroup(ivalue);
		}
		
		react_to_changes = true;
		
	}
	else
	{
		TemplateVisibility* vis = main_view->getMapVisibility();
		QString text = template_table->item(row, column)->text().trimmed();
		QRectF map_bounds = map->calculateExtent(true, false, NULL);
		
		react_to_changes = false;
		if (column == 0)
		{
			bool visible_new = template_table->item(row, column)->checkState() == Qt::Checked;
			if (!visible_new)
				map->setObjectAreaDirty(map_bounds);
			
			vis->visible = visible_new;
			
			if (visible_new)
				map->setObjectAreaDirty(map_bounds);
			
			updateRow(row);
			updateButtons();
		}
		else if (column == 1)
		{
			float opacity = template_table->item(row, column)->data(Qt::DisplayRole).toFloat();
			if (opacity <= 0.0f)
			{
				map->setObjectAreaDirty(map_bounds);
				vis->opacity = qMax(0.0f, opacity);
			}
			else
			{
				vis->opacity = qMin(1.0f, opacity);
				map->setObjectAreaDirty(map_bounds);
			}
			
			template_table->item(row, 1)->setData(Qt::DecorationRole, QColor::fromCmykF(0.0f, 0.0f, 0.0f, vis->opacity));
		}
		react_to_changes = true;
	}
}

void TemplateWidget::updateButtons()
{
	if (!react_to_changes)
		return;
	
	bool map_row_selected = false;  // does the selection contain the map row?
	bool first_row_selected = false;
	bool last_row_selected = false;
	int num_rows_selected = 0;
	int visited_row = -1;
	for (int i = 1; i < template_table->selectedItems().size(); ++i)
	{
		const int row = template_table->selectedItems()[i]->row();
		if (row == visited_row)
			continue;
		
		visited_row = row;
		++num_rows_selected;
		
		if (posFromRow(row) < 0)
			map_row_selected = true;
		
		if (row == 0)
			first_row_selected = true;
		if (row == template_table->rowCount() - 1)
			last_row_selected = true;
	}
	bool single_row_selected = (num_rows_selected == 1);
	
	duplicate_action->setEnabled(single_row_selected && !map_row_selected);
	delete_button->setEnabled(single_row_selected && !map_row_selected);	// TODO: Make it possible to delete multiple templates at once
	move_up_button->setEnabled(single_row_selected && !first_row_selected);
	move_down_button->setEnabled(single_row_selected && !last_row_selected);
	edit_button->setEnabled(single_row_selected && !map_row_selected);
	
	bool georef_visible = false;
	bool georef_active  = false;
	bool custom_visible = false;
	bool custom_active  = false;
	bool import_active  = false;
	if (single_row_selected)
	{
		Template* temp = map_row_selected ? NULL : map->getTemplate(posFromRow(visited_row));
		if (map_row_selected)
		{
			georef_visible = true;
			georef_active = map->getGeoreferencing().isValid() && !map->getGeoreferencing().isLocal();
		}
		else if (temp && temp->isTemplateGeoreferenced())
		{
			georef_visible = true;
			georef_active  = true;
		}
		else
		{
			custom_visible = true;
			custom_active = template_table->item(visited_row, 0)->checkState() == Qt::Checked;
		}
		import_active = qobject_cast< TemplateMap* >(getCurrentTemplate());
		
	}
	georef_button->setVisible(georef_visible);
	georef_button->setChecked(georef_active);
	move_by_hand_button->setEnabled(custom_active);
	move_by_hand_button->setVisible(custom_visible);
	adjust_button->setEnabled(custom_active);
	adjust_button->setVisible(custom_visible);
	position_action->setEnabled(custom_active);
	position_action->setVisible(custom_visible);
	import_action->setVisible(import_active);
	
/*	if (enable_active_buttons)
	{
		// TODO: Implement and enable buttons again
		//group_button->setEnabled(false); //multiple_rows_selected || (!multiple_rows_selected && map->getTemplate(posFromRow(current_row))->getTemplateGroup() >= 0));
		//more_button->setEnabled(false); // !multiple_rows_selected);
	} */
	
// 	if (multiple_rows_selected)
// 	{
// 		adjust_button->setChecked(false);
// 		position_button->setChecked(false);
// 	}
// 	else
// 	{
// 		adjust_button->setChecked(temp && controller->getEditorActivity() && controller->getEditorActivity()->getActivityObject() == (void*)temp);
// 		position_button->setChecked(temp && controller->existsTemplatePositionDockWidget(temp));
// 	}
}

void TemplateWidget::currentCellChange(int current_row, int current_column, int previous_row, int previous_column)
{
	Q_UNUSED(previous_row);
	Q_UNUSED(previous_column);
	
	if (!react_to_changes)
		return;
	
	if (current_column == 3)
	{
		int pos = posFromRow(current_row);
		Template* temp = (current_row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
		if (!temp)
			return;
		
		if (temp->getTemplateState() == Template::Invalid)
		{
			QTimer::singleShot(0, this, SLOT(changeTemplateFile()));
		}
	}
}

void TemplateWidget::cellDoubleClick(int row, int column)
{
	int pos = posFromRow(row);
	Template* temp = (row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
	if (temp)
	{
		template_table->setCurrentCell(row, 0); // prerequisite for changeTemplateFile()
		if (column == 3 || temp->getTemplateState() == Template::Invalid)
		{
			QTimer::singleShot(0, this, SLOT(changeTemplateFile()));
		}
	}
}

void TemplateWidget::updateDeleteButtonText()
{
	bool keep_transformation_of_closed_templates = Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool();
	delete_button->setToolTip(keep_transformation_of_closed_templates ? tr("Close") : tr("Delete"));
}

void TemplateWidget::moveByHandClicked(bool checked)
{
	Template* temp = getCurrentTemplate();
	Q_ASSERT(temp);
	controller->setTool(checked ? new TemplateMoveTool(temp, controller, move_by_hand_action) : NULL);
}

void TemplateWidget::adjustClicked(bool checked)
{
	if (checked)
	{
		Template* temp = getCurrentTemplate();
		Q_ASSERT(temp);
		TemplateAdjustActivity* activity = new TemplateAdjustActivity(temp, controller);
		controller->setEditorActivity(activity);
		connect(activity->getDockWidget(), SIGNAL(closed()), this, SLOT(adjustWindowClosed()));
	}
	else
	{
		controller->setEditorActivity(NULL);	// TODO: default activity?!
	}
}

void TemplateWidget::adjustWindowClosed()
{
	Template* current_template = getCurrentTemplate();
	if (!current_template)
		return;
	
	if (controller->getEditorActivity() && controller->getEditorActivity()->getActivityObject() == (void*)current_template)
		adjust_button->setChecked(false);
}

/*void TemplateWidget::groupClicked()
{
	// TODO
}*/

void TemplateWidget::positionClicked(bool checked)
{
	Q_UNUSED(checked);
	
	Template* temp = getCurrentTemplate();
	if (!temp)
		return;
	
	if (controller->existsTemplatePositionDockWidget(temp))
		controller->removeTemplatePositionDockWidget(temp);
	else
		controller->addTemplatePositionDockWidget(temp);
}

void TemplateWidget::importClicked()
{
	Template* templ = qobject_cast< TemplateMap* >(getCurrentTemplate());
	QScopedPointer<Map> template_map(new Map());
	if (templ && template_map->loadFrom(templ->getTemplatePath(), this, NULL, false, true))
	{
		TemplateTransform transform;
		templ->getTransform(transform);
		template_map->operationOnAllObjects(ApplyTemplateTransform(transform));
		
		double nominal_scale = template_map->getScaleDenominator() / map->getScaleDenominator();
		double current_scale = 0.5 * (transform.template_scale_x + transform.template_scale_y);
		double scale = 1.0;
		bool ok = true;
		if (qAbs(nominal_scale - 1.0) > 0.01 || qAbs(current_scale - 1.0) > 0.01)
		{
			QStringList scale_options( QStringList() <<
			  tr("Don't scale") <<
			  tr("Scale by nominal map scale ratio (%1 %)").arg(locale().toString(nominal_scale * 100.0, 'f', 1)) <<
			  tr("Scale by current template scaling (%1 %)").arg(locale().toString(current_scale * 100.0, 'f', 1)) );
			QString option = QInputDialog::getItem( window(),
			  tr("Template import"),
			  tr("How shall the symbols of the imported template map be scaled?"),
			  scale_options, 0, false, &ok );
			if (option.isEmpty())
				return;
			else if (option == scale_options[1])
				scale = nominal_scale;
			else if (option == scale_options[2])
				scale = current_scale;
		}
		
		if (ok)
		{
			if (scale != 1.0)
				template_map->scaleAllSymbols(scale);
			
			// Symbols and objects are already adjusted. Merge as is.
			template_map->setGeoreferencing(map->getGeoreferencing());
			map->importMap(template_map.data(), Map::MinimalObjectImport, window());
			deleteTemplate();
		}
	}
}

void TemplateWidget::moreActionClicked(QAction* action)
{
	Q_UNUSED(action);
	// TODO
}

void TemplateWidget::templateAdded(int pos, Template* temp)
{
	Q_UNUSED(temp);
	int row = rowFromPos(pos);
	template_table->insertRow(row);
	addRow(row);
	template_table->setCurrentCell(row, 0);
}

void TemplateWidget::templatePositionDockWidgetClosed(Template* temp)
{
	Template* current_temp = getCurrentTemplate();
	if (current_temp == temp)
		position_action->setChecked(false);
}

void TemplateWidget::addRow(int row)
{
	react_to_changes = false;
	
	for (int i = 0; i < 4; ++i)
	{
		QTableWidgetItem* item = new QTableWidgetItem();
		template_table->setItem(row, i, item);
	}
	template_table->item(row, 0)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	template_table->item(row, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	template_table->item(row, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	updateRow(row);
	
	react_to_changes = true;
}

void TemplateWidget::updateRow(int row)
{
	int pos = posFromRow(row);
	
	react_to_changes = false;
	
	QPalette::ColorGroup color_group = template_table->isEnabled() ? QPalette::Active : QPalette::Disabled;
	TemplateVisibility* vis = NULL;
	int group = -1;
	QString name, path;
	bool valid = true;
	if (pos >= 0)
	{
		Template* temp = map->getTemplate(pos);
		// TODO: Get visibility values from the MapView of the active MapWidget (instead of always main_view)
		vis = main_view->getTemplateVisibility(temp);
		group = temp->getTemplateGroup();
		name = temp->getTemplateFilename();
		path = temp->getTemplatePath();
		valid = temp->getTemplateState() != Template::Invalid;
		Qt::ItemFlag editable = (valid && vis->visible) ? Qt::ItemIsEditable : Qt::NoItemFlags;
		QBrush active_background(QPalette().color(color_group, QPalette::Base));
		template_table->item(row, 0)->setBackground(active_background);
		template_table->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | editable);
		template_table->item(row, 1)->setBackground(active_background);
		template_table->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | editable);
		template_table->item(row, 2)->setBackground(active_background);
		template_table->item(row, 3)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		template_table->item(row, 3)->setBackground(active_background);
	}
	else
	{
		vis = main_view->getMapVisibility();
		name = tr("- Map -");
		QBrush map_row_background(QPalette().color(color_group, QPalette::AlternateBase));
		template_table->item(row, 0)->setBackground(map_row_background);
		template_table->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		template_table->item(row, 1)->setBackground(map_row_background);
		template_table->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		template_table->item(row, 2)->setBackground(map_row_background);
		template_table->item(row, 3)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		template_table->item(row, 3)->setBackground(map_row_background);
	}
	
	if (valid)
	{
		template_table->item(row, 0)->setCheckState(vis->visible ? Qt::Checked : Qt::Unchecked);
		template_table->item(row, 3)->setData(Qt::DecorationRole, QVariant());
	}
	else
	{
		template_table->item(row, 0)->setCheckState(vis->visible ? Qt::PartiallyChecked : Qt::Unchecked);
		template_table->item(row, 3)->setData(Qt::DecorationRole, QIcon(":/images/delete.png"));
	}
	template_table->item(row, 1)->setData(Qt::DisplayRole, vis->opacity);
	template_table->item(row, 2)->setText((group < 0) ? "" : QString::number(group));
	template_table->item(row, 3)->setText(name);
	template_table->item(row, 3)->setData(Qt::ToolTipRole, path);
	
	QColor decoration_color(Qt::white);
	QBrush brush(QColor::fromRgb(204, 0, 0));
	if (vis->visible)
	{
		if (valid)
		{
			decoration_color = QColor::fromCmykF(0.0f, 0.0f, 0.0f, vis->opacity);
			brush.setColor(QPalette().color(color_group, QPalette::Foreground));
		}
	}
	else if (valid)
	{
		decoration_color = QColor(Qt::transparent);
		brush.setColor(QPalette().color(QPalette::Disabled, QPalette::Foreground));
	}
	else
	{
		decoration_color = QColor(Qt::transparent);
		brush.setColor(brush.color().lighter());
	}
	template_table->item(row, 1)->setData(Qt::DecorationRole, decoration_color);
	template_table->item(row, 1)->setForeground(brush);
	template_table->item(row, 2)->setForeground(brush);
	template_table->item(row, 3)->setForeground(brush);
	
	react_to_changes = true;
}

int TemplateWidget::posFromRow(int row)
{
	int pos = template_table->rowCount() - 1 - row;
	
	if (pos == map->getFirstFrontTemplate())
		return -1;
	
	if (pos > map->getFirstFrontTemplate())
		return pos - 1;
	else
		return pos;
}

int TemplateWidget::rowFromPos(int pos)
{
	Q_ASSERT(pos >= 0);
	return map->getNumTemplates() - 1 - ((pos >= map->getFirstFrontTemplate()) ? pos : (pos - 1));
}

Template* TemplateWidget::getCurrentTemplate()
{
	int current_row = template_table->currentRow();
	if (current_row < 0)
		return NULL;
	int pos = posFromRow(current_row);
	if (pos < 0)
		return NULL;
	return map->getTemplate(pos);
}

void TemplateWidget::changeTemplateFile()
{
	int row = template_table->currentRow();
	int pos = posFromRow(row);
	Template* temp = (row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
	Q_ASSERT(temp);
	
	if (temp && temp->execSwitchTemplateFileDialog(this))
	{
		updateRow(row);
		updateButtons();
		temp->setTemplateAreaDirty();
		map->setTemplatesDirty();
	}
}
