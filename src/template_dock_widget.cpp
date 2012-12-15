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


#include "template_dock_widget.h"

#include <cassert>

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "map.h"
#include "template.h"
#include "map_editor.h"
#include "template_adjust.h"
#include "template_tool_move.h"
#include "template_position_dock_widget.h"
#include "georeferencing.h"
#include "settings.h"

/** Parses a user-entered opacity value. Values must be strings of the form "F%" where F is any decimal number between 0 and
 *  100 (inclusive). Leading and trailing whitespace is trimmed. If the value is invalid, the arguments are unchanged and the method returns false.
 *  If the value is valid, the method updates both the text (to a canonical form) and the float value, and returns true.
 */
static bool parseOpacityEntry(QString &text, float &fvalue)
{
    bool ok = true;
    QString str = text.trimmed();
    float value;
    if (str.endsWith('%'))
    {
        str.chop(1);
        str = str.trimmed();
    }
	value = str.toFloat(&ok) / 100.0f;

    if (!ok || value < 0 || value > 1)
        return false;

    text = QString("%1%").arg(100.0f * value);
    fvalue = value;
    return true;
}

TemplateWidget::TemplateWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget(parent), 
  map(map), 
  main_view(main_view), 
  controller(controller)
{
	this->setWhatsThis("<a href=\"template_menu.html\">See more</a>");
	react_to_changes = true;
	
	// Template table
	template_table = new QTableWidget(map->getNumTemplates() + 1, 4);
	template_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	template_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	template_table->setHorizontalHeaderLabels(QStringList() << tr("Show") << tr("Opacity") << tr("Group") << tr("Filename"));
	template_table->verticalHeader()->setVisible(false);
	
	QHeaderView* header_view = template_table->horizontalHeader();
	for (int i = 0; i < 3; ++i)
		header_view->setResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setResizeMode(3, QHeaderView::Stretch);
	header_view->setClickable(false);
	
	for (int i = 0; i < map->getNumTemplates() + 1; ++i)
		addRow(i);
	
	// List Buttons
	list_buttons_group = new QWidget();
	
	QToolButton* new_button = new QToolButton();
	new_button->setText(tr("Create..."));
	new_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	new_button->setPopupMode(QToolButton::InstantPopup);
	new_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	QMenu* new_button_menu = new QMenu(new_button);
	new_button_menu->addAction(tr("Sketch"));
	new_button_menu->addAction(tr("GPS"));
	new_button->setMenu(new_button_menu);
	new_button->setEnabled(false);	// TODO!
	
	QPushButton* open_button = new QPushButton(QIcon(":/images/open.png"), tr("Open..."));
	delete_button = new QPushButton(QIcon(":/images/minus.png"), "");
	updateDeleteButtonText();
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(updateDeleteButtonText()));
	duplicate_button = new QPushButton(QIcon(":/images/copy.png"), tr("Duplicate"));
	move_up_button = new QPushButton(QIcon(":/images/arrow-up.png"), tr("Move Up"));
	move_down_button = new QPushButton(QIcon(":/images/arrow-down.png"), tr("Move Down"));
	QPushButton* help_button = new QPushButton(QIcon(":/images/help.png"), tr("Help"));

	QGridLayout* list_buttons_group_layout = new QGridLayout();
	list_buttons_group_layout->setMargin(0);
	list_buttons_group_layout->addWidget(new_button, 0, 1);
	list_buttons_group_layout->addWidget(open_button, 0, 0);
	list_buttons_group_layout->addWidget(duplicate_button, 1, 1);
	list_buttons_group_layout->addWidget(delete_button, 2, 1);
	list_buttons_group_layout->addWidget(move_up_button, 1, 0);
	list_buttons_group_layout->addWidget(move_down_button, 2, 0);
	list_buttons_group_layout->setRowStretch(3, 1);
	list_buttons_group_layout->addWidget(help_button, 4, 0, 1, 2);
	list_buttons_group->setLayout(list_buttons_group_layout);
	
	// Active group
	active_buttons_group = new QGroupBox();
	
	georef_button = new QPushButton();
	georef_button->setCheckable(true);
	move_by_hand_action = new QAction(QIcon(":/images/move.png"), tr("Move by hand"), this);
	move_by_hand_action->setCheckable(true);
	move_by_hand_button = new QToolButton();
	move_by_hand_button->setDefaultAction(move_by_hand_action);
	move_by_hand_button->setToolButtonStyle(Qt::ToolButtonTextOnly);
	move_by_hand_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	adjust_button = new QPushButton(QIcon(":/images/georeferencing.png"), tr("Adjust..."));
	adjust_button->setCheckable(true);
	//group_button = new QPushButton(QIcon(":/images/group.png"), tr("(Un)group"));
	position_button = new QPushButton(tr("Positioning..."));
	position_button->setCheckable(true);
	
	/*more_button = new QToolButton();
	more_button->setText(tr("More..."));
	more_button->setPopupMode(QToolButton::InstantPopup);
	more_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	QMenu* more_button_menu = new QMenu(more_button);
	more_button_menu->addAction(QIcon(":/images/window-new.png"), tr("Numeric transformation window"));
	more_button_menu->addAction(tr("Set transparent color..."));
	more_button_menu->addAction(tr("Trace lines..."));
	more_button->setMenu(more_button_menu);*/
	
	int row = 0;
	QGridLayout* active_buttons_group_layout = new QGridLayout();
	active_buttons_group_layout->setMargin(0);
	active_buttons_group_layout->addWidget(georef_button, row, 0);
	active_buttons_group_layout->addWidget(adjust_button, row++, 1);
	active_buttons_group_layout->addWidget(move_by_hand_button, row, 0);
	active_buttons_group_layout->addWidget(position_button, row++, 1);
	//active_buttons_group_layout->addWidget(group_button, row++, 0);
	//active_buttons_group_layout->addWidget(more_button, row++, 1);
	active_buttons_group->setLayout(active_buttons_group_layout);
	
	selectionChanged(QItemSelection(), QItemSelection()); // enable / disable buttons
	//currentCellChange(template_table->currentRow(), 0, 0, 0);	// enable / disable buttons
	
	// Load settings
	QSettings settings;
	settings.beginGroup("TemplateWidget");
	QSize preferred_size = settings.value("size", QSize(200, 500)).toSize();
	settings.endGroup();
	
	// Create main layout
	wide_layout = false;
	layout = NULL;
	QResizeEvent event(preferred_size, preferred_size);
	resizeEvent(&event);
	
	// Connections
	connect(template_table, SIGNAL(cellChanged(int,int)), this, SLOT(cellChange(int,int)));
	connect(template_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	connect(template_table, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentCellChange(int,int,int,int)));
	connect(template_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClick(int,int)));
	
	connect(new_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(newTemplate(QAction*)));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(openTemplate()));
	connect(delete_button, SIGNAL(clicked(bool)), this, SLOT(deleteTemplate()));
	connect(duplicate_button, SIGNAL(clicked(bool)), this, SLOT(duplicateTemplate()));
	connect(move_up_button, SIGNAL(clicked(bool)), this, SLOT(moveTemplateUp()));
	connect(move_down_button, SIGNAL(clicked(bool)), this, SLOT(moveTemplateDown()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
	
	connect(move_by_hand_action, SIGNAL(triggered(bool)), this, SLOT(moveByHandClicked(bool)));
	connect(adjust_button, SIGNAL(clicked(bool)), this, SLOT(adjustClicked(bool)));
	//connect(group_button, SIGNAL(clicked(bool)), this, SLOT(groupClicked()));
	connect(position_button, SIGNAL(clicked(bool)), this, SLOT(positionClicked(bool)));
	//connect(more_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(moreActionClicked(QAction*)));
	
	connect(map, SIGNAL(templateAdded(int,Template*)), this, SLOT(templateAdded(int,Template*)));
	connect(controller, SIGNAL(templatePositionDockWidgetClosed(Template*)), this, SLOT(templatePositionDockWidgetClosed(Template*)));
}
TemplateWidget::~TemplateWidget()
{
	// Save settings
	QSettings settings;
	settings.beginGroup("TemplateWidget");
	settings.setValue("size", size());
	settings.endGroup();	
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
Template* TemplateWidget::showOpenTemplateDialog(QWidget* dialog_parent, MapView* main_view)
{
	QSettings settings;
	QString template_directory = settings.value("templateFileDirectory", QDir::homePath()).toString();
	
	QString path = QFileDialog::getOpenFileName(dialog_parent, tr("Open image, GPS track or DXF file"), template_directory, QString("%1 (*.omap *.xmap *.ocd *.bmp *.jpg *.jpeg *.gif *.png *.tif *.tiff *.gpx *.dxf *.osm);;%2 (*.*)").arg(tr("Template files")).arg(tr("All files")));
	path = QFileInfo(path).canonicalFilePath();
	if (path.isEmpty())
		return NULL;
	
	settings.setValue("templateFileDirectory", QFileInfo(path).canonicalPath());
	
	Template* new_temp = Template::templateForFile(path, main_view->getMap());
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
		QPointF center = new_temp->calculateTemplateBoundingBox().center();
		new_temp->setTemplateX(main_view->getPositionX() - qRound64(1000 * center.x()));
		new_temp->setTemplateY(main_view->getPositionY() - qRound64(1000 * center.y()));
	}
	
	return new_temp;
}

void TemplateWidget::resizeEvent(QResizeEvent* event)
{
	int width = event->size().width();
	int height = event->size().height();
	
	// NOTE: layout direction adaption deactivated for better usability
	
	bool change = (layout == NULL);
	//if ((width >= height && !wide_layout) || (width < height && wide_layout))
	//	change = true;
	
	if (change)
	{
		if (layout)
		{
			for (int i = layout->count(); i >= 0; --i)
				layout->removeItem(layout->itemAt(i));
			delete layout;
		}
		
		//if (width >= height)
		//	layout = new QHBoxLayout();
		//else if (width < height)
			layout = new QVBoxLayout();
		
		layout->setMargin(0);
		layout->addWidget(template_table, 1);
		layout->addWidget(list_buttons_group);
		layout->addWidget(active_buttons_group);
		setLayout(layout);
		updateGeometry();
		
		wide_layout = width > height;
	}
	
	event->accept();
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
	Template* new_template = showOpenTemplateDialog(window(), main_view);
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
	assert(pos >= 0);
	
	map->setTemplateAreaDirty(pos);
	
	if (Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool())
		map->closeTemplate(pos);
	else
		map->deleteTemplate(pos);
	
	template_table->removeRow(template_table->currentRow());
	if (pos < map->getFirstFrontTemplate())
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() - 1);
	
	map->setTemplatesDirty();
}
void TemplateWidget::duplicateTemplate()
{
	int row = template_table->currentRow();
	assert(row >= 0);
	int pos = posFromRow(row);
	assert(pos >= 0);
	
	Template* new_template = map->getTemplate(pos)->duplicate();
	addTemplateAt(new_template, pos);
}
void TemplateWidget::moveTemplateUp()
{
	int row = template_table->currentRow();
	assert(row >= 1);
	
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
	
	template_table->setCurrentCell(row - 1, template_table->currentColumn());
	map->setTemplatesDirty();
}
void TemplateWidget::moveTemplateDown()
{
	int row = template_table->currentRow();
	assert(row < template_table->rowCount() - 1);
	
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
	
	template_table->setCurrentCell(row + 1, template_table->currentColumn());
	map->setTemplatesDirty();
}
void TemplateWidget::showHelp()
{
	controller->getWindow()->showHelp("template_menu.html");
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
            bool visible_new = template_table->item(row, column)->checkState() == Qt::Checked;
            if (!visible_new)
                map->setTemplateAreaDirty(pos);

            vis->visible = visible_new;

            if (visible_new)
                map->setTemplateAreaDirty(pos);
        }
        else if (column == 1)
        {
            float fvalue;
            if (!parseOpacityEntry(text, fvalue))
            {
                QMessageBox::warning(window(), tr("Error"), tr("Please enter a percentage from 0 to 100!"));
                template_table->item(row, column)->setText(QString::number(vis->opacity * 100) + "%");
            }
            else
            {
                template_table->item(row, column)->setText(text);
                if (fvalue <= 0)
                    map->setTemplateAreaDirty(pos);

                vis->opacity = fvalue;

                if (fvalue > 0)
                    map->setTemplateAreaDirty(pos);
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
        }
        else if (column == 1)
        {
            float fvalue;
            if (!parseOpacityEntry(text, fvalue))
            {
                QMessageBox::warning(window(), tr("Error"), tr("Please enter a valid number from 0 to 1, or specify a percentage from 0 to 100!"));
                template_table->item(row, column)->setText(QString::number(vis->opacity * 100) + "%");
            }
            else
            {
                template_table->item(row, column)->setText(text);
                if (fvalue <= 0)
                    map->setObjectAreaDirty(map_bounds);

                vis->opacity = fvalue;

                if (fvalue > 0)
                    map->setObjectAreaDirty(map_bounds);
            }
        }
        react_to_changes = true;
    }
}

void TemplateWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	if (!react_to_changes)
		return;
	
	int current_row = template_table->currentRow();
	int pos = posFromRow(current_row);
	bool map_row = pos < 0;	// does the selection contain the map row?
	Template* temp = (current_row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
	bool multiple_rows_selected = false;		// is more than one row selected?
	
	int first_selected_row = (template_table->selectedItems().size() > 0) ? template_table->selectedItems()[0]->row() : 0;
	for (int i = 1; i < template_table->selectedItems().size(); ++i)
	{
		int row = template_table->selectedItems()[i]->row();
		if (row != first_selected_row)
			multiple_rows_selected = true;
		if (posFromRow(row) < 0)
			map_row = true;
	}
	if (current_row != first_selected_row)
		multiple_rows_selected = true;
	
	duplicate_button->setEnabled(current_row >= 0 && !map_row && !multiple_rows_selected);
	delete_button->setEnabled(current_row >= 0 && !map_row && !multiple_rows_selected);	// TODO: Make it possible to delete multiple templates at once
	move_up_button->setEnabled(current_row >= 1 && !multiple_rows_selected);
	move_down_button->setEnabled(current_row < template_table->rowCount() - 1 && current_row != -1 && !multiple_rows_selected);
	
	QString active_group_title;
	QString georef_text;
	bool georef_active;
	if (current_row < 0)
	{
		active_group_title = tr("No selection");
		georef_text = "-";
		georef_active = false;
	}
	else if (multiple_rows_selected)
	{
		active_group_title = tr("Multiple templates selected");
		georef_text = "-";
		georef_active = false;
	}
	else if (map_row)
	{
		active_group_title = tr("- Map -");
		georef_active = !map->getGeoreferencing().isLocal();
		georef_text = georef_active ? tr("yes") : tr("no");
	}
	else
	{
		active_group_title = temp->getTemplateFilename();
		georef_active = temp->isTemplateGeoreferenced();
		georef_text = georef_active ? tr("yes") : tr("no");
	}
	active_buttons_group->setTitle(active_group_title);
	georef_button->setText(tr("Georeferenced: %1").arg(georef_text));
	georef_button->setChecked(georef_active);
	
	bool enable_active_buttons = current_row >= 0 && !map_row && temp && !multiple_rows_selected;
	active_buttons_group->setEnabled(enable_active_buttons);
	if (enable_active_buttons)
	{
		// TODO: enable changing georeferencing state
		georef_button->setEnabled(false);
		
		move_by_hand_button->setEnabled(!temp->isTemplateGeoreferenced());
		adjust_button->setEnabled(!temp->isTemplateGeoreferenced());
		position_button->setEnabled(!temp->isTemplateGeoreferenced());
		
		// TODO: Implement and enable buttons again
		//group_button->setEnabled(false); //multiple_rows_selected || (!multiple_rows_selected && map->getTemplate(posFromRow(current_row))->getTemplateGroup() >= 0));
		//more_button->setEnabled(false); // !multiple_rows_selected);
	}
	
	if (multiple_rows_selected)
	{
		adjust_button->setChecked(false);
		position_button->setChecked(false);
	}
	else
	{
		adjust_button->setChecked(temp && controller->getEditorActivity() && controller->getEditorActivity()->getActivityObject() == (void*)temp);
		position_button->setChecked(temp && controller->existsTemplatePositionDockWidget(temp));
	}
}
void TemplateWidget::currentCellChange(int current_row, int current_column, int previous_row, int previous_column)
{
	if (current_column == 3)
	{
        int pos = posFromRow(current_row);
        Template* temp = (current_row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
        if (!temp)
            return;

        if (temp->getTemplateState() == Template::Invalid)
            changeTemplateFile(current_row);
    }
}
void TemplateWidget::cellDoubleClick(int row, int column)
{
	if (column == 3)
	{
		int pos = posFromRow(row);
		Template* temp = (row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
		if (!temp)
			return;
		
		changeTemplateFile(row);
	}
}

void TemplateWidget::updateDeleteButtonText()
{
	bool keep_transformation_of_closed_templates = Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool();
	delete_button->setText(keep_transformation_of_closed_templates ? tr("Close") : tr("Delete"));
}

void TemplateWidget::moveByHandClicked(bool checked)
{
	Template* temp = getCurrentTemplate();
	assert(temp);
	controller->setTool(checked ? new TemplateMoveTool(temp, controller, move_by_hand_action) : NULL);
}
void TemplateWidget::adjustClicked(bool checked)
{
	if (checked)
	{
		Template* temp = getCurrentTemplate();
		assert(temp);
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
	Template* temp = getCurrentTemplate();
	if (!temp)
		return;
	
	if (controller->existsTemplatePositionDockWidget(temp))
		controller->removeTemplatePositionDockWidget(temp);
	else
		controller->addTemplatePositionDockWidget(temp);
}
void TemplateWidget::moreActionClicked(QAction* action)
{
	// TODO
}

void TemplateWidget::templateAdded(int pos, Template* temp)
{
	int row = rowFromPos(pos);
	template_table->insertRow(row);
	addRow(row);
	template_table->setCurrentCell(row, 3);
}

void TemplateWidget::templatePositionDockWidgetClosed(Template* temp)
{
	Template* current_temp = getCurrentTemplate();
	if (current_temp == temp)
		position_button->setChecked(false);
}

void TemplateWidget::addRow(int row)
{
	react_to_changes = false;
	
	int pos = posFromRow(row);
	if (pos < 0)
	{
		// Insert "map" row
		QTableWidgetItem* item = new QTableWidgetItem();
		template_table->setItem(row, 0, item);
		
		item = new QTableWidgetItem();
		template_table->setItem(row, 1, item);
		
		item = new QTableWidgetItem();
		template_table->setItem(row, 2, item);
		
		item = new QTableWidgetItem();
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		template_table->setItem(row, 3, item);
	}
	else
	{
		QTableWidgetItem* item = new QTableWidgetItem();
		template_table->setItem(row, 0, item);
		
		item = new QTableWidgetItem();
		template_table->setItem(row, 1, item);
		
		item = new QTableWidgetItem();
		template_table->setItem(row, 2, item);
		
		item = new QTableWidgetItem();
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		template_table->setItem(row, 3, item);
	}
	
	updateRow(row);
	
	react_to_changes = true;
}
void TemplateWidget::updateRow(int row)
{
	int pos = posFromRow(row);
	
	react_to_changes = false;

    template_table->item(row, 0)->setBackgroundColor(Qt::white);	// TODO: might be better to load this from some palette ...
    template_table->item(row, 0)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    template_table->item(row, 1)->setBackgroundColor(Qt::white);
    template_table->item(row, 1)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    template_table->item(row, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    template_table->item(row, 2)->setBackgroundColor(Qt::white);
    template_table->item(row, 2)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    template_table->item(row, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    TemplateVisibility* vis = NULL;
    int group = -1;
    QString name;
    bool valid = true;
    if (pos >= 0)
    {
        Template* temp = map->getTemplate(pos);
        // TODO: Get visibility values from the MapView of the active MapWidget (instead of always main_view)
        vis = main_view->getTemplateVisibility(temp);
        group = temp->getTemplateGroup();
        name = temp->getTemplateFilename();
        valid = temp->getTemplateState() != Template::Invalid;
    }
    else
    {
        vis = main_view->getMapVisibility();
        name = tr("- Map -");
        template_table->item(row, 2)->setBackgroundColor(qRgb(180, 180, 180));
        template_table->item(row, 2)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }

    template_table->item(row, 0)->setCheckState(vis->visible ? Qt::Checked : Qt::Unchecked);
    template_table->item(row, 1)->setText(QString::number(vis->opacity * 100) + "%");
    template_table->item(row, 2)->setText((group < 0) ? "" : QString::number(group));
    template_table->item(row, 3)->setText(name);
    template_table->item(row, 3)->setTextColor(valid ? QPalette().color(QPalette::Text) : qRgb(204, 0, 0));

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
	assert(pos >= 0);
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

void TemplateWidget::changeTemplateFile(int row)
{
	int pos = posFromRow(row);
	Template* temp = (row >= 0 && pos >= 0) ? map->getTemplate(pos) : NULL;
	assert(temp);
	
	if (temp->execSwitchTemplateFileDialog(this))
	{
		updateRow(row);
		temp->setTemplateAreaDirty();
		map->setTemplatesDirty();
	}
}
