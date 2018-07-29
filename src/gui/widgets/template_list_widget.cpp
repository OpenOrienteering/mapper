/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#include "template_list_widget.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QScroller>
#include <QSettings>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QToolButton>
#include <QToolTip>

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "core/objects/object.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/segmented_button_layout.h"
#include "templates/template.h"
#include "templates/template_adjust.h"
#include "templates/template_map.h"
#include "templates/template_tool_move.h"
#include "util/item_delegates.h"


namespace OpenOrienteering {

// ### ApplyTemplateTransform ###

/**
 * A local functor which is used when importing map templates into the current map.
 */
struct ApplyTemplateTransform
{
	const TemplateTransform& transform;
	
	void operator()(Object* object) const
	{ 
		object->rotate(transform.template_rotation);
		object->scale(transform.template_scale_x, transform.template_scale_y);
		object->move(transform.template_x, transform.template_y);
	}
};



// ### TemplateListWidget ###

// Template grouping implementation is incomplete
#define NO_TEMPLATE_GROUP_SUPPORT

TemplateListWidget::TemplateListWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget(parent)
, map(map)
, main_view(main_view)
, controller(controller)
, mobile_mode(controller->isInMobileMode())
, name_column(3)
{
	Q_ASSERT(main_view);
	Q_ASSERT(controller);
	
	setWhatsThis(Util::makeWhatThis("templates.html#setup"));
	
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	
	// Wrap the checkbox in a widget and layout to force a margin.
	auto top_bar_widget = new QWidget();
	auto top_bar_layout = new QHBoxLayout(top_bar_widget);
	
	// Reuse the translation from MapEditorController action.
	all_hidden_check = new QCheckBox(::OpenOrienteering::MapEditorController::tr("Hide all templates"));
	top_bar_layout->addWidget(all_hidden_check);
	
	if (mobile_mode)
	{
		auto close_action = new QAction(QIcon(QString::fromLatin1(":/images/close.png")), ::OpenOrienteering::MainWindow::tr("Close"), this);
		connect(close_action, &QAction::triggered, this, &TemplateListWidget::closeClicked );
		
		auto close_button = new QToolButton();
		close_button->setDefaultAction(close_action);
		close_button->setAutoRaise(true);
		
		top_bar_layout->addWidget(close_button);
	}
	
	top_bar_layout->setContentsMargins(
	            style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
	            style()->pixelMetric(QStyle::PM_LayoutTopMargin, &style_option) / 2,
	            style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
	            0 // Covered by the main layout's spacing.
	);
	
	// Template table
	template_table = new QTableWidget(map->getNumTemplates() + 1, 4);
	QScroller::grabGesture(template_table->viewport(), QScroller::TouchGesture);
	template_table->installEventFilter(this);
	template_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	template_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	template_table->setSelectionMode(QAbstractItemView::SingleSelection);
	template_table->verticalHeader()->setVisible(false);
#ifdef NO_TEMPLATE_GROUP_SUPPORT
	// Template grouping is not yet implemented.
	template_table->hideColumn(2);
#endif
	
	auto header_view = template_table->horizontalHeader();
	if (mobile_mode)
	{
		header_view->setVisible(false);
		template_table->setShowGrid(false);
		template_table->hideColumn(3);
		name_column = 0;
	}
	else
	{
		template_table->setHorizontalHeaderLabels(QStringList() << QString{} << tr("Opacity") << tr("Group") << tr("Filename"));
		template_table->horizontalHeaderItem(0)->setData(Qt::ToolTipRole, tr("Show"));
		
		header_view->setSectionResizeMode(0, QHeaderView::Fixed);
		
		QStyleOptionButton option;
		auto geometry = style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, nullptr);
		template_table->setColumnWidth(0, geometry.width() * 14 / 10);
		
		auto header_check_size = geometry.size();
		if (header_check_size.isValid())
		{
			QCheckBox header_check;
			header_check.setChecked(true);
			header_check.setEnabled(false);
			QPixmap pixmap(header_check_size);
			pixmap.fill(Qt::transparent);
			QPainter painter(&pixmap);
			QStyleOptionViewItem option;
			option.rect = { {0, 0}, geometry.size() };
			style()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &option, &painter, nullptr);
			painter.end();
			template_table->horizontalHeaderItem(0)->setData(Qt::DecorationRole, pixmap);
		}
	}
		
	auto percentage_delegate = new PercentageDelegate(this, 5);
	template_table->setItemDelegateForColumn(1, percentage_delegate);
	
	for (int i = 1; i < 3; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(name_column, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	for (int i = 0; i < map->getNumTemplates() + 1; ++i)
		addRowItems(i);
	
	all_templates_layout = new QVBoxLayout();
	all_templates_layout->setMargin(0);
	all_templates_layout->addWidget(top_bar_widget);
	all_templates_layout->addWidget(template_table, 1);
	
	auto new_button_menu = new QMenu(this);
	if (!mobile_mode)
	{
		new_button_menu->addAction(QIcon(QString::fromLatin1(":/images/open.png")), tr("Open..."), this, SLOT(openTemplate()));
		new_button_menu->addAction(controller->getAction("reopentemplate"));
	}
	duplicate_action = new_button_menu->addAction(QIcon(QString::fromLatin1(":/images/tool-duplicate.png")), tr("Duplicate"), this, SLOT(duplicateTemplate()));
#if 0
	current_action = new_button_menu->addAction(tr("Sketch"));
	current_action->setDisabled(true);
	current_action = new_button_menu->addAction(tr("GPS"));
	current_action->setDisabled(true);
#endif
	
	auto new_button = newToolButton(QIcon(QString::fromLatin1(":/images/plus.png")), tr("Add template..."));
	new_button->setPopupMode(QToolButton::InstantPopup);
	new_button->setMenu(new_button_menu);
	
	delete_button = newToolButton(QIcon(QString::fromLatin1(":/images/minus.png")), (tr("Remove"), tr("Close"))); /// \todo Use "Remove instead of "Close"
	
	auto add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(new_button);
	add_remove_layout->addWidget(delete_button);
	
	move_up_button = newToolButton(QIcon(QString::fromLatin1(":/images/arrow-up.png")), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_down_button = newToolButton(QIcon(QString::fromLatin1(":/images/arrow-down.png")), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	
	auto up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);
	
	// TODO: Fix string
	georef_button = newToolButton(QIcon(QString::fromLatin1(":/images/grid.png")), tr("Georeferenced: %1").remove(QLatin1String(": %1")));
	georef_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	georef_button->setCheckable(true);
	georef_button->setChecked(true);
	georef_button->setEnabled(false); // TODO
	move_by_hand_action = new QAction(QIcon(QString::fromLatin1(":/images/move.png")), tr("Move by hand"), this);
	move_by_hand_action->setCheckable(true);
	move_by_hand_button = newToolButton(move_by_hand_action->icon(), move_by_hand_action->text());
	move_by_hand_button->setDefaultAction(move_by_hand_action);
	adjust_button = newToolButton(QIcon(QString::fromLatin1(":/images/georeferencing.png")), tr("Adjust..."));
	adjust_button->setCheckable(true);
	
	auto edit_menu = new QMenu(this);
	position_action = edit_menu->addAction(tr("Positioning..."));
	position_action->setCheckable(true);
	import_action =  edit_menu->addAction(tr("Import and remove"), this, SLOT(importClicked()));
	
	edit_button = newToolButton(QIcon(QString::fromLatin1(":/images/settings.png")),
	                            ::OpenOrienteering::MapEditorController::tr("&Edit").remove(QLatin1Char('&')));
	edit_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	edit_button->setPopupMode(QToolButton::InstantPopup);
	edit_button->setMenu(edit_menu);
	
	// The buttons row layout
	auto list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	list_buttons_layout->addWidget(adjust_button);
	list_buttons_layout->addWidget(move_by_hand_button);
	list_buttons_layout->addWidget(georef_button);
	list_buttons_layout->addWidget(edit_button);
	
	list_buttons_group = new QWidget();
	list_buttons_group->setLayout(list_buttons_layout);
	
	auto all_buttons_layout = new QHBoxLayout();
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		0, // Covered by the main layout's spacing.
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(list_buttons_group);
	all_buttons_layout->addWidget(new QLabel(QString::fromLatin1("   ")), 1);
	
	if (!mobile_mode)
	{
		auto help_button = newToolButton(QIcon(QString::fromLatin1(":/images/help.png")), tr("Help"));
		help_button->setAutoRaise(true);
		all_buttons_layout->addWidget(help_button);
		connect(help_button, &QAbstractButton::clicked, this, &TemplateListWidget::showHelp);
	}
	
	all_templates_layout->addLayout(all_buttons_layout);
	
	setLayout(all_templates_layout);
	
	//group_button = new QPushButton(QIcon(QString::fromLatin1(":/images/group.png")), tr("(Un)group"));
	/*more_button = new QToolButton();
	more_button->setText(tr("More..."));
	more_button->setPopupMode(QToolButton::InstantPopup);
	more_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	QMenu* more_button_menu = new QMenu(more_button);
	more_button_menu->addAction(QIcon(QString::fromLatin1(":/images/window-new.png")), tr("Numeric transformation window"));
	more_button_menu->addAction(tr("Set transparent color..."));
	more_button_menu->addAction(tr("Trace lines..."));
	more_button->setMenu(more_button_menu);*/
	
	updateButtons();
	
	setAllTemplatesHidden(main_view->areAllTemplatesHidden());
	
	// Connections
	connect(all_hidden_check, &QAbstractButton::toggled, controller, &MapEditorController::hideAllTemplates);
	
	connect(template_table, &QTableWidget::cellChanged, this, &TemplateListWidget::cellChange);
	connect(template_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TemplateListWidget::updateButtons);
	connect(template_table, &QTableWidget::cellClicked, this, &TemplateListWidget::cellClicked, Qt::QueuedConnection);
	connect(template_table, &QTableWidget::cellDoubleClicked, this, &TemplateListWidget::cellDoubleClicked, Qt::QueuedConnection);
	
	connect(delete_button, &QAbstractButton::clicked, this, &TemplateListWidget::deleteTemplate);
	connect(move_up_button, &QAbstractButton::clicked, this, &TemplateListWidget::moveTemplateUp);
	connect(move_down_button, &QAbstractButton::clicked, this, &TemplateListWidget::moveTemplateDown);
	
	connect(move_by_hand_action, &QAction::triggered, this, &TemplateListWidget::moveByHandClicked);
	connect(adjust_button, &QAbstractButton::clicked, this, &TemplateListWidget::adjustClicked);
	connect(position_action, &QAction::triggered, this, &TemplateListWidget::positionClicked);
	
	//connect(group_button, SIGNAL(clicked(bool)), this, SLOT(groupClicked()));
	//connect(more_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(moreActionClicked(QAction*)));
	
	connect(main_view, &MapView::visibilityChanged, this, &TemplateListWidget::updateVisibility);
	connect(controller, &MapEditorController::templatePositionDockWidgetClosed, this, &TemplateListWidget::templatePositionDockWidgetClosed);
}

TemplateListWidget::~TemplateListWidget() = default;



QToolButton* TemplateListWidget::newToolButton(const QIcon& icon, const QString& text)
{
	auto button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setIcon(icon);
	button->setText(text);
	button->setWhatsThis(Util::makeWhatThis("templates.html#setup"));
	return button;
}

// slot
void TemplateListWidget::setAllTemplatesHidden(bool value)
{
	all_hidden_check->setChecked(value);
	
	bool enabled = !value;
	template_table->setEnabled(enabled);
	list_buttons_group->setEnabled(enabled);
	updateButtons();
}

void TemplateListWidget::addTemplateAt(Template* new_template, int pos)
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
	
	map->addTemplate(new_template, pos);
	map->setTemplateAreaDirty(pos);
	
	map->setTemplatesDirty();
}

std::unique_ptr<Template> TemplateListWidget::showOpenTemplateDialog(QWidget* dialog_parent, MapEditorController* controller)
{
	QSettings settings;
	QString template_directory = settings.value(QString::fromLatin1("templateFileDirectory"), QDir::homePath()).toString();
	
	QString pattern;
	for (const auto& extension : Template::supportedExtensions())
	{
		pattern.append(QLatin1String(" *."));
		pattern.append(QLatin1String(extension));
	}
	pattern.remove(0, 1);
	QString path = FileDialog::getOpenFileName(dialog_parent,
	                                           tr("Open image, GPS track or DXF file"),
	                                           template_directory,
	                                           QString::fromLatin1("%1 (%2);;%3 (*.*)").arg(
	                                               tr("Template files"), pattern, tr("All files")));
	auto canonical_path = QFileInfo(path).canonicalFilePath();
	if (!canonical_path.isEmpty())
	{
		path = canonical_path;
		settings.setValue(QString::fromLatin1("templateFileDirectory"), QFileInfo(path).canonicalPath());
	}
	else if (path.isEmpty())
	{
		return {};
	}
	
	bool center_in_view = true;
	QString error = tr("Cannot open template\n%1:\n%2").arg(path);
	auto new_temp = Template::templateForFile(path, controller->getMap());
	if (!new_temp)
	{
		QMessageBox::warning(dialog_parent, tr("Error"), error.arg(tr("File format not recognized.")));
	}
	else if (!new_temp->preLoadConfiguration(dialog_parent))
	{
		new_temp.reset();
	}
	else if (!new_temp->loadTemplateFile(true))
	{
		QString error_detail = new_temp->errorString();
		if (error_detail.isEmpty())
			error_detail = tr("Failed to load template. Does the file exist and is it valid?");
		QMessageBox::warning(dialog_parent, tr("Error"), error.arg(error_detail));
		new_temp.reset();
	}
	else if (!new_temp->postLoadConfiguration(dialog_parent, center_in_view))
	{
		new_temp.reset();
	}
	// If the template is not georeferenced, position it at the viewport midpoint
	else if (!new_temp->isTemplateGeoreferenced() && center_in_view)
	{
		auto main_view = controller->getMainWidget()->getMapView();
		auto view_pos = main_view->center();
		auto offset = MapCoord { new_temp->calculateTemplateBoundingBox().center() };
		new_temp->setTemplatePosition(view_pos - offset);
	}
	
	return new_temp;
}

bool TemplateListWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == template_table)
	{
		switch (event->type())
		{
		case QEvent::KeyPress:
			if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space)
			{
				int row = template_table->currentRow();
				if (row >= 0 && template_table->item(row, 1)->flags().testFlag(Qt::ItemIsEnabled))
				{
					bool is_checked = template_table->item(row, 0)->checkState() != Qt::Unchecked;
					template_table->item(row, 0)->setCheckState(is_checked ? Qt::Unchecked : Qt::Checked);
				}
				return true;
			}
			break;
			
		case QEvent::KeyRelease:
			if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space)
				return true;
			break;
			
#ifdef Q_OS_ANDROID
		case QEvent::Show:
			{
				auto map_row = rowFromPos(map->getFirstFrontTemplate()) + 1;
				template_table->resizeRowToContents(map_row);
				template_table->verticalHeader()->setDefaultSectionSize(template_table->verticalHeader()->sectionSize(map_row));
			}
			break;
#endif
			
		default:
			; //nothing
		}
	}
	
	return false;
}

void TemplateListWidget::newTemplate(QAction* action)
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

void TemplateListWidget::openTemplate()
{
	auto new_template = showOpenTemplateDialog(window(), controller);
	if (new_template)
	{
		int pos = -1;
		int row = template_table->currentRow();
		if (row >= 0)
			pos = posFromRow(row);
		
		addTemplateAt(new_template.release(), pos);
	}
}

void TemplateListWidget::deleteTemplate()
{
	int pos = posFromRow(template_table->currentRow());
	Q_ASSERT(pos >= 0);
	
	map->setTemplateAreaDirty(pos);
	
	if (Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool())
		map->closeTemplate(pos);
	else
		map->deleteTemplate(pos);
	
	{
		QSignalBlocker block(template_table);
		template_table->removeRow(template_table->currentRow());
	}
		
	if (pos < map->getFirstFrontTemplate())
		map->setFirstFrontTemplate(map->getFirstFrontTemplate() - 1);
	
	map->setTemplatesDirty();
	
	// Do a change of selection to trigger a button update
	int current_row = template_table->currentRow();
	template_table->clearSelection();
	template_table->selectRow(current_row);
}

void TemplateListWidget::duplicateTemplate()
{
	int row = template_table->currentRow();
	Q_ASSERT(row >= 0);
	int pos = posFromRow(row);
	Q_ASSERT(pos >= 0);
	
	const auto prototype = map->getTemplate(pos);
	const auto visibility = main_view->getTemplateVisibility(prototype);
	
	auto new_template = prototype->duplicate();
	addTemplateAt(new_template, pos);
	main_view->setTemplateVisibility(new_template, visibility);
	updateRow(row+1);
}

void TemplateListWidget::moveTemplateUp()
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
		auto above_template = map->getTemplate(above_pos);
		auto cur_template = map->getTemplate(cur_pos);
		map->setTemplate(cur_template, above_pos);
		map->setTemplate(above_template, cur_pos);
	}
	
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(above_pos);
	updateRow(row - 1);
	updateRow(row);
	
	{
		QSignalBlocker block(template_table);
		template_table->setCurrentCell(row - 1, template_table->currentColumn());
	}
	//updateButtons();
	map->setTemplatesDirty();
}

void TemplateListWidget::moveTemplateDown()
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
		auto below_template = map->getTemplate(below_pos);
		auto cur_template = map->getTemplate(cur_pos);
		map->setTemplate(cur_template, below_pos);
		map->setTemplate(below_template, cur_pos);
	}
	
	map->setTemplateAreaDirty(cur_pos);
	map->setTemplateAreaDirty(below_pos);
	updateRow(row + 1);
	updateRow(row);
	
	{
		QSignalBlocker block(template_table);
		template_table->setCurrentCell(row + 1, template_table->currentColumn());
	}
	updateButtons();
	map->setTemplatesDirty();
}

void TemplateListWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "templates.html", "setup");
}

void TemplateListWidget::cellChange(int row, int column)
{
	int pos = posFromRow(row);
	
	Template* temp = nullptr;
	auto state = Template::Loaded;
	auto visibility = main_view->getMapVisibility();
	if (pos >= 0)
	{
		// Template row, not map row
		temp = map->getTemplate(pos);
		state = temp->getTemplateState();
		visibility = main_view->getTemplateVisibility(temp);
	}
	
	auto updateVisibility = [this](Template* temp, TemplateVisibility vis)
	{
		if (temp)
			main_view->setTemplateVisibility(temp, vis);
		else
			main_view->setMapVisibility(vis);
	};
	
	if (state != Template::Invalid)
	{
		auto setAreaDirty = [this, pos, temp]()
		{ 
			if (pos >= 0)
			{
				map->setTemplateAreaDirty(pos);
			}
			else
			{
				//QRectF map_bounds = map->calculateExtent(true, false, nullptr);
				//map->setObjectAreaDirty(map_bounds);
				main_view->updateAllMapWidgets();  // Map change - doesn't need to update the map cache
			}
		};
		
		switch (column)
		{
		case 0:  // Visibility checkbox
			{
				bool visible = template_table->item(row, column)->checkState() == Qt::Checked;
				if (visibility.visible != visible)
				{
					if (!visible)
					{
						setAreaDirty();
						visibility.visible = false;
						updateVisibility(temp, visibility);
					}
					else
					{
						if (state != Template::Loaded)
						{
							// Ensure feedback before slow loading/drawing
							QSignalBlocker block(template_table);
							template_table->item(row, 0)->setCheckState(Qt::PartiallyChecked);
							auto item_rect = template_table->visualItemRect(template_table->item(row, 1));
							QToolTip::showText(template_table->mapToGlobal(item_rect.bottomLeft()),
							                   qApp->translate("OpenOrienteering::MainWindow", "Opening %1").arg(temp->getTemplateFilename()) );
							// QToolTip seems to need to event loop runs.
							qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
							qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
						}
						visibility.visible = true;
						updateVisibility(temp, visibility);
						setAreaDirty();
						if (state != Template::Loaded)
						{
							QToolTip::hideText();
							if (temp->getTemplateState() != Template::Loaded)
							{
								QMessageBox::warning(this,
								                     qApp->translate("OpenOrienteering::MainWindow", "Error"),
								                     qApp->translate("OpenOrienteering::Importer", "Failed to load template '%1', reason: %2")
								                     .arg(temp->getTemplateFilename(), temp->errorString()) );
							}
						}
					}
					updateRow(row);
					updateButtons();
				}
			}
			break;
			
		case 1:  // Opacity spinbox or slider
			{
				float opacity = template_table->item(row, column)->data(Qt::DisplayRole).toFloat();
				if (!qFuzzyCompare(1.0f+opacity, 1.0f+visibility.opacity))
				{
					visibility.opacity = qBound(0.0f, opacity, 1.0f);
					updateVisibility(temp, visibility);
					setAreaDirty();
					template_table->item(row, 1)->setData(Qt::DecorationRole, QColor::fromCmykF(0.0f, 0.0f, 0.0f, visibility.opacity));
				}
			}
			break;
			
#ifndef NO_TEMPLATE_GROUP_SUPPORT
		case 2:
			{
				QString text = template_table->item(row, column)->text().trimmed();
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
#endif
		default:
			; // nothing
		}
	}
}

void TemplateListWidget::updateButtons()
{
	bool map_row_selected = false;  // does the selection contain the map row?
	bool first_row_selected = false;
	bool last_row_selected = false;
	int num_rows_selected = 0;
	int visited_row = -1;
	for (auto&& item : template_table->selectedItems())
	{
		const int row = item->row();
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
	
	auto single_template_selected = single_row_selected && !map_row_selected;
	duplicate_action->setEnabled(single_template_selected);
	delete_button->setEnabled(single_template_selected);	/// \todo Make it possible to delete multiple templates at once
	move_up_button->setEnabled(single_row_selected && !first_row_selected);
	move_down_button->setEnabled(single_row_selected && !last_row_selected);
	edit_button->setEnabled(single_template_selected);
	
	bool georef_visible = false;
	bool georef_active  = false;
	bool custom_visible = false;
	bool custom_active  = false;
	bool import_active  = false;
	if (mobile_mode)
	{
		// Leave most buttons invisible
		edit_button->setVisible(false);
	}
	else if (single_template_selected)
	{
		auto temp = map->getTemplate(posFromRow(visited_row));
		if (temp->isTemplateGeoreferenced())
		{
			georef_visible = true;
			georef_active  = true;
		}
		else
		{
			custom_visible = true;
			custom_active = template_table->item(visited_row, 0)->checkState() == Qt::Checked;
		}
		import_active = qobject_cast<TemplateMap*>(getCurrentTemplate());
	}
	else if (single_row_selected)
	{
		georef_visible = true;
		georef_active = map->getGeoreferencing().isValid() && !map->getGeoreferencing().isLocal();
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

void TemplateListWidget::cellClicked(int row, int column)
{
	auto pos = posFromRow(qMax(0, row));
	
	switch (column)
	{
	case 1:
		if (mobile_mode
		    && row >= 0
		    && template_table->item(row, 0)->checkState() == Qt::Checked)
		{
			showOpacitySlider(row);
		}
		break;
		
	case 3:
		if (!mobile_mode
		    && row >= 0 && pos >= 0
		    && map->getTemplate(pos)->getTemplateState() == Template::Invalid)
		{
			changeTemplateFile(pos);
		}
		break;
		
	default:
		break;
	}
}

void TemplateListWidget::cellDoubleClicked(int row, int column)
{
	auto pos = posFromRow(qMax(0, row));
	
	switch (column)
	{
	default:
		if (! (row >= 0 && pos >= 0
		       && map->getTemplate(pos)->getTemplateState() == Template::Invalid))
			break;
		// Invalid template: fall through
	case 3:
		if (!mobile_mode
		    && row >= 0 && pos >= 0)
		{
			changeTemplateFile(pos);
		}
	}
}

void TemplateListWidget::moveByHandClicked(bool checked)
{
	auto temp = getCurrentTemplate();
	Q_ASSERT(temp);
	controller->setTool(checked ? new TemplateMoveTool(temp, controller, move_by_hand_action) : nullptr);
}

void TemplateListWidget::adjustClicked(bool checked)
{
	if (checked)
	{
		auto temp = getCurrentTemplate();
		Q_ASSERT(temp);
		auto activity = new TemplateAdjustActivity(temp, controller);
		controller->setEditorActivity(activity);
		connect(activity->getDockWidget(), &TemplateAdjustDockWidget::closed, this, &TemplateListWidget::adjustWindowClosed);
	}
	else
	{
		controller->setEditorActivity(nullptr);	// TODO: default activity?!
	}
}

void TemplateListWidget::adjustWindowClosed()
{
	auto current_template = getCurrentTemplate();
	if (!current_template)
		return;
	
	if (controller->getEditorActivity() && controller->getEditorActivity()->getActivityObject() == (void*)current_template)
		adjust_button->setChecked(false);
}

#ifndef NO_TEMPLATE_GROUP_SUPPORT
void TemplateListWidget::groupClicked()
{
	// TODO
}
#endif

void TemplateListWidget::positionClicked(bool checked)
{
	Q_UNUSED(checked);
	
	auto temp = getCurrentTemplate();
	if (!temp)
		return;
	
	if (controller->existsTemplatePositionDockWidget(temp))
		controller->removeTemplatePositionDockWidget(temp);
	else
		controller->addTemplatePositionDockWidget(temp);
}

void TemplateListWidget::importClicked()
{
	auto prototype = qobject_cast<const TemplateMap*>(getCurrentTemplate());
	if (!prototype)
		return;
	
	TemplateTransform transform;
	if (!prototype->isTemplateGeoreferenced())
		prototype->getTransform(transform);
	
	Map template_map;
	
	bool ok = true;
	if (qstrcmp(prototype->getTemplateType(), "OgrTemplate") == 0)
	{
		template_map.importMap(*prototype->templateMap(), Map::MinimalObjectImport);
		if (!prototype->isTemplateGeoreferenced())
		{
			template_map.applyOnAllObjects(ApplyTemplateTransform{transform});
			template_map.setGeoreferencing(map->getGeoreferencing());
		}
		auto template_scale = (transform.template_scale_x + transform.template_scale_y) / 2;
		template_scale *= double(prototype->templateMap()->getScaleDenominator()) / map->getScaleDenominator();
		if (!qFuzzyCompare(template_scale, 1))
		{
			template_map.scaleAllSymbols(template_scale);
		}
	}
	else if (qstrcmp(prototype->getTemplateType(), "TemplateMap") == 0
	         && template_map.loadFrom(prototype->getTemplatePath(), this, nullptr, false, true))
	{
		if (!prototype->isTemplateGeoreferenced())
			template_map.applyOnAllObjects(ApplyTemplateTransform{transform});
		
		double nominal_scale = (double)template_map.getScaleDenominator() / (double)map->getScaleDenominator();
		double current_scale = 0.5 * (transform.template_scale_x + transform.template_scale_y);
		double scale = 1.0;
		QStringList scale_options;
		if (qAbs(nominal_scale - 1.0) > 0.009)
			scale_options.append(tr("Scale by nominal map scale ratio (%1 %)").arg(locale().toString(nominal_scale * 100.0, 'f', 1)));
		if (qAbs(current_scale - 1.0) > 0.009 && qAbs(current_scale - nominal_scale) > 0.009)
			scale_options.append(tr("Scale by current template scaling (%1 %)").arg(locale().toString(current_scale * 100.0, 'f', 1)));
		if (!scale_options.isEmpty())
		{
			scale_options.prepend(tr("Don't scale"));
			QString option = QInputDialog::getItem( window(),
			  tr("Template import"),
			  tr("How shall the symbols of the imported template map be scaled?"),
			  scale_options, 0, false, &ok );
			if (option.isEmpty())
				return;
			else if (option == scale_options[0])
				Q_ASSERT(scale == 1.0);
			else if (option == scale_options[1])
				scale = nominal_scale;
			else // This option may have been omitted.
				scale = current_scale;
		}
		
		if (ok && scale != 1.0)
				template_map.scaleAllSymbols(scale);
	}
	else
	{
		Q_UNREACHABLE();
		ok = false;
	}

	if (ok)
	{
		map->importMap(template_map, Map::MinimalObjectImport);
		deleteTemplate();
		
		if (main_view->isOverprintingSimulationEnabled()
		    && !template_map.hasSpotColors())
		{
			auto answer = QMessageBox::question(
			                  window(),
			                  tr("Template import"),
			                  tr("The template will be invisible in the overprinting simulation. "
			                     "Switch to normal view?"),
			                  QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No), 
			                  QMessageBox::Yes );
			if (answer == QMessageBox::Yes)
			{
				if (auto action = controller->getAction("overprintsimulation"))
					action->trigger();
			}
		}
		
		
		auto map_visibility = main_view->getMapVisibility();
		if (!map_visibility.visible)
		{
			map_visibility.visible = true;
			updateRow(map->getNumTemplates() - map->getFirstFrontTemplate());
		}
	}
}

void TemplateListWidget::moreActionClicked(QAction* action)
{
	Q_UNUSED(action);
	// TODO
}

void TemplateListWidget::templateAdded(int pos, const Template* temp)
{
	Q_UNUSED(temp);
	int row = rowFromPos(pos);
	template_table->insertRow(row);
	addRowItems(row);
	template_table->setCurrentCell(row, 0);
}

void TemplateListWidget::templatePositionDockWidgetClosed(Template* temp)
{
	auto current_temp = getCurrentTemplate();
	if (current_temp == temp)
		position_action->setChecked(false);
}

void TemplateListWidget::updateVisibility(MapView::VisibilityFeature feature, bool active, const Template* temp)
{
	switch (feature)
	{
	case MapView::AllTemplatesHidden:
		setAllTemplatesHidden(active);
		break;
		
	case MapView::MapVisible:
		updateRow(map->getFirstFrontTemplate());
		break;
		
	case MapView::TemplateVisible:
		if (map->getNumTemplates() == template_table->rowCount()+1)
		{
			auto row = map->findTemplateIndex(temp);
			if (row >= 0)
				updateRow(posFromRow(row));
			break;
		}
		// fallthrough
	case MapView::MultipleFeatures:
		updateAll();
		break;
		
	default:
		;  // nothing
	}
}

void TemplateListWidget::updateAll()
{
	auto templates_hidden = main_view->areAllTemplatesHidden();
	template_table->setEnabled(!templates_hidden); // Color scheme depends on state
	
	auto old_size = template_table->rowCount();
	template_table->setRowCount(map->getNumTemplates() + 1);
	for (auto i = 0; i < old_size; ++i)
		updateRow(i);
	for (auto i = old_size; i < template_table->rowCount(); ++i)
		addRowItems(i);
	
	setAllTemplatesHidden(templates_hidden);  // implicit updateButtons
}

void TemplateListWidget::addRowItems(int row)
{
	QSignalBlocker block(template_table);
	
	for (int i = 0; i < 4; ++i)
	{
		auto item = new QTableWidgetItem();
		template_table->setItem(row, i, item);
	}
	template_table->item(row, 0)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled /* | Qt::ItemIsSelectable*/);
	template_table->item(row, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	template_table->item(row, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	
	updateRow(row);
}

void TemplateListWidget::updateRow(int row)
{
	int pos = posFromRow(row);
	int group = -1;
	QString name;
	QString path;
	bool valid = true;
	
	TemplateVisibility vis;
	
	QPalette::ColorGroup color_group = template_table->isEnabled() ? QPalette::Active : QPalette::Disabled;
#ifdef Q_OS_ANDROID
	auto background_color = QPalette().color(color_group, QPalette::Background);
#else
	auto background_color = QPalette().color(color_group, QPalette::Base);
#endif
	
	if (pos >= 0)
	{
		auto temp = map->getTemplate(pos);
		group = temp->getTemplateGroup();
		name = temp->getTemplateFilename();
		path = temp->getTemplatePath();
		valid = temp->getTemplateState() != Template::Invalid;
		/// @todo Get visibility values from the MapView of the active MapWidget (instead of always main_view)
		vis = main_view->getTemplateVisibility(temp);
	}
	else
	{
		name = tr("- Map -");
		vis = main_view->getMapVisibility();
#ifdef Q_OS_ANDROID
		auto r = (128 + 5 * background_color.red()) / 6;
		auto g = (128 + 5 * background_color.green()) / 6;
		auto b = (128 + 5 * background_color.blue()) / 6;
		background_color = QColor(r, g, b);
#else
		background_color = QPalette().color(color_group, QPalette::AlternateBase);
#endif
	}
	
	// Cheep defaults, mostly for !vis.visible
	auto check_state    = Qt::Unchecked;
	auto opacity_color  = QColor{ Qt::transparent };   
	auto text_color     = QColor::fromRgb(255, 51, 51); 
	auto decoration     = QVariant{ };
	auto checkable      = Qt::ItemIsUserCheckable;
	auto editable       = Qt::NoItemFlags;
	auto group_editable = Qt::NoItemFlags;
	
	if (valid)
	{
		if (vis.visible)
		{
			check_state   = Qt::Checked;
			opacity_color = QColor::fromCmykF(0.0f, 0.0f, 0.0f, vis.opacity);
			text_color    = QPalette().color(color_group, QPalette::Foreground);
			if (!mobile_mode)
			{
				editable = Qt::ItemIsEditable;
				if (pos >= 0)
				{
					group_editable = Qt::ItemIsEditable;
				}
			}
		}
		else
		{
			text_color = QPalette().color(QPalette::Disabled, QPalette::Foreground);
		}
		decoration = QVariant{ opacity_color };
	}
	else
	{
		if (vis.visible)
		{
			check_state = Qt::PartiallyChecked;
			text_color = text_color.darker();
		}
		decoration = QIcon::fromTheme(QLatin1String("image-missing"), QIcon{QLatin1String(":/images/close.png")});
		checkable  = Qt::NoItemFlags;
		editable   = Qt::NoItemFlags;
	}
	
	auto foreground = QBrush(text_color);
	auto background = QBrush(background_color);
	
	QSignalBlocker block(template_table);
	{
		auto item0 = template_table->item(row, 0);
		item0->setBackground(background);
		item0->setCheckState(check_state);
		item0->setFlags(checkable | Qt::ItemIsEnabled);
#ifdef Q_OS_ANDROID
		// Some combinations not working well in Android style
		if (!valid)
		{
		//	item0->setData(Qt::CheckStateRole, {});
		//	item0->setFlags(enabled);
		}
#endif
	}
	{
		auto item1 = template_table->item(row, 1);
		item1->setBackground(background);
		item1->setForeground(foreground);
		item1->setData(Qt::DisplayRole, vis.opacity);
		item1->setData(Qt::DecorationRole, decoration);
		item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | editable);
	}
#ifndef NO_TEMPLATE_GROUP_SUPPORT
	{
		auto item2 = template_table->item(row, 2);
		item->setBackground(background);
		item->setForeground(foreground);
		item->setText((group < 0) ? "" : QString::number(group));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | groupable);
	}
#endif
	{
		auto name_item = template_table->item(row, name_column);
		name_item->setBackground(background);
		name_item->setForeground(foreground);
		name_item->setText(name);
		name_item->setData(Qt::ToolTipRole, path);
		name_item->setData(Qt::DecorationRole, {});
		auto prev_checkable = name_item->flags() & Qt::ItemIsUserCheckable;
		name_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | prev_checkable);
	}
}

int TemplateListWidget::posFromRow(int row)
{
	int pos = template_table->rowCount() - 1 - row;
	
	if (pos == map->getFirstFrontTemplate())
		pos = -1; // the map row
	else if (pos > map->getFirstFrontTemplate())
		pos = pos - 1; // before the map row 
	
	return pos;
}

int TemplateListWidget::rowFromPos(int pos)
{
	Q_ASSERT(pos >= 0);
	return map->getNumTemplates() - 1 - ((pos >= map->getFirstFrontTemplate()) ? pos : (pos - 1));
}

Template* TemplateListWidget::getCurrentTemplate()
{
	int current_row = template_table->currentRow();
	if (current_row < 0)
		return nullptr;
	int pos = posFromRow(current_row);
	if (pos < 0)
		return nullptr;
	return map->getTemplate(pos);
}

void TemplateListWidget::changeTemplateFile(int pos)
{
	auto temp = map->getTemplate(pos);
	Q_ASSERT(temp);
	temp->execSwitchTemplateFileDialog(this);
	updateRow(rowFromPos(pos));
	updateButtons();
	temp->setTemplateAreaDirty();
	map->setTemplatesDirty();
}

void TemplateListWidget::showOpacitySlider(int row)
{
	auto geometry = template_table->visualItemRect(template_table->item(row, name_column));
	geometry.translate(0, geometry.height());
	
	QDialog dialog(this);
	dialog.move(template_table->viewport()->mapToGlobal(geometry.topLeft()));
	
	auto slider = new QSlider();
	slider->setOrientation(Qt::Horizontal);
	slider->setRange(0, 20);
	slider->setMinimumWidth(geometry.width());
	
	auto opacity_item = template_table->item(row, 1);
	slider->setValue(opacity_item->data(Qt::DisplayRole).toFloat() * 20);
	connect(slider, &QSlider::valueChanged, [opacity_item](int value) {
		opacity_item->setData(Qt::DisplayRole, 0.05f * value);
	} );
	
	auto close_button = new QToolButton();
	close_button->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, 0, this));
	close_button->setAutoRaise(true);
	connect(close_button, &QToolButton::clicked, &dialog, &QDialog::accept);
	
	auto layout = new QHBoxLayout(&dialog);
	layout->addWidget(slider);
	layout->addWidget(close_button);
	
	dialog.exec();
}


}  // namespace OpenOrienteering
