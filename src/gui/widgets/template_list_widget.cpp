/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020, 2025 Kai Pastor
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

#include <utility>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QAbstractTableModel>
#include <QAction>
#include <QBoxLayout>
#include <QByteArray>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QScroller>
#include <QSettings>
#include <QSize>
#include <QSlider>
#include <QStringList>
#include <QStyle>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QVector>

#ifdef WITH_COVE
#include <app/coverunner.h>
#endif /* WITH_COVE */

#include "settings.h"
#include "core/georeferencing.h"
#include "core/map.h"
#include "fileformats/file_format_registry.h"
#include "fileformats/file_import_export.h"
#include "gui/file_dialog.h"
#include "gui/main_window.h"
#include "gui/util_gui.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"
#include "gui/widgets/segmented_button_layout.h"
#include "templates/template.h"
#include "templates/template_adjust.h"
#include "templates/template_image.h"
#include "templates/template_map.h"
#include "templates/template_position_dock_widget.h"
#include "templates/template_table_model.h"
#include "templates/template_tool_move.h"
#include "tools/tool.h"
#include "util/item_delegates.h"


#ifdef __clang_analyzer__
#define singleShot(A, B, C) singleShot(A, B, #C) // NOLINT 
#endif


namespace OpenOrienteering {

namespace {

QVariant makeCheckBoxDecorator(QStyle* style, const QSize& size)
{
	QCheckBox header_check;
	header_check.setChecked(true);
	header_check.setEnabled(false);
	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	QStyleOptionViewItem option_item;
	option_item.rect = { {0, 0}, size };
	style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &option_item, &painter, nullptr);
	painter.end();
	return pixmap;
}

/// Local wrapper for Util::ToolButton::create() adding the What's This bit.
QToolButton* createToolButton(const QIcon& icon, const QString& text)
{
	return Util::ToolButton::create(icon, text, "templates.html#setup");
}

}  // anonymous namespace


// ### TemplateListWidget ###

// Template grouping implementation is incomplete
#define NO_TEMPLATE_GROUP_SUPPORT

TemplateListWidget::~TemplateListWidget() = default;

TemplateListWidget::TemplateListWidget(Map& map, MapView& main_view, MapEditorController& controller, QWidget* parent)
: QWidget(parent)
, map(map)
, main_view(main_view)
, controller(controller)
, mobile_mode(controller.isInMobileMode())
{
	setWhatsThis(Util::makeWhatThis("templates.html#setup"));
	
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);
	
	// Wrap the checkbox in a widget and layout to force a margin.
	auto* top_bar_widget = new QWidget();
	auto* top_bar_layout = new QHBoxLayout(top_bar_widget);
	
	// Reuse the translation from MapEditorController action.
	all_hidden_check = new QCheckBox(::OpenOrienteering::MapEditorController::tr("Hide all templates"));
	top_bar_layout->addWidget(all_hidden_check);
	
	if (mobile_mode)
	{
		auto* close_action = new QAction(QIcon(QString::fromLatin1(":/images/close.png")), ::OpenOrienteering::MainWindow::tr("Close"), this);
		connect(close_action, &QAction::triggered, this, &TemplateListWidget::closeClicked );
		
		auto* close_button = new QToolButton();
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
	auto* template_model = new TemplateTableModel(map, main_view, this);
	template_table = new QTableView();
	template_table->setModel(template_model);
	
	QScroller::grabGesture(template_table->viewport(), QScroller::TouchGesture);
	template_table->installEventFilter(this);
	template_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	template_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	template_table->setSelectionMode(QAbstractItemView::SingleSelection);
	template_table->verticalHeader()->setVisible(false);
#ifdef NO_TEMPLATE_GROUP_SUPPORT
	// Template grouping is not yet implemented.
	template_table->hideColumn(TemplateTableModel::groupColumn());
#endif
	connect(template_model, &TemplateTableModel::rowsInserted, this, [this](const QModelIndex& /*unused*/, int first) {
		template_table->selectRow(first);
	});
	
	auto* percentage_delegate = new PercentageDelegate(this, 5);
	template_table->setItemDelegateForColumn(1, percentage_delegate);
	
	auto* header_view = template_table->horizontalHeader();
	if (mobile_mode)
	{
		template_model->setTouchMode(true);
		
		header_view->setSectionResizeMode(TemplateTableModel::visibilityColumn(), QHeaderView::Stretch);
		header_view->setSectionResizeMode(TemplateTableModel::opacityColumn(), QHeaderView::ResizeToContents);
		header_view->setSectionResizeMode(TemplateTableModel::groupColumn(), QHeaderView::ResizeToContents);
		header_view->setVisible(false);
		
		template_table->setShowGrid(false);
		template_table->hideColumn(TemplateTableModel::nameColumn());
	}
	else
	{
		header_view->setSectionResizeMode(TemplateTableModel::visibilityColumn(), QHeaderView::Fixed);
		header_view->setSectionResizeMode(TemplateTableModel::opacityColumn(), QHeaderView::ResizeToContents);
		header_view->setSectionResizeMode(TemplateTableModel::nameColumn(), QHeaderView::Stretch);
		header_view->setSectionResizeMode(TemplateTableModel::groupColumn(), QHeaderView::ResizeToContents);
		header_view->setSectionsClickable(false);
		
		QStyleOptionButton option;
		auto geometry = style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, nullptr);
		template_table->setColumnWidth(TemplateTableModel::visibilityColumn(), geometry.width() * 14 / 10);
		
		auto header_check_size = geometry.size();
		if (header_check_size.isValid())
			template_model->setCheckBoxDecorator(makeCheckBoxDecorator(style(), header_check_size));
	}
	
	all_templates_layout = new QVBoxLayout();
	all_templates_layout->setMargin(0);
	all_templates_layout->addWidget(top_bar_widget);
	all_templates_layout->addWidget(template_table, 1);
	
	auto* new_button_menu = new QMenu(this);
	if (!mobile_mode)
	{
		new_button_menu->addAction(QIcon(QString::fromLatin1(":/images/open.png")), tr("Open..."), this, &TemplateListWidget::openTemplate);
		new_button_menu->addAction(controller.getAction("reopentemplate"));
	}
	duplicate_action = new_button_menu->addAction(QIcon(QString::fromLatin1(":/images/tool-duplicate.png")), tr("Duplicate"), this, &TemplateListWidget::duplicateTemplate);
#if 0
	current_action = new_button_menu->addAction(tr("Sketch"));
	current_action->setDisabled(true);
	current_action = new_button_menu->addAction(tr("GPS"));
	current_action->setDisabled(true);
#endif
	
	auto* new_button = createToolButton(QIcon(QString::fromLatin1(":/images/plus.png")), tr("Add template..."));
	new_button->setPopupMode(QToolButton::InstantPopup);
	new_button->setMenu(new_button_menu);
	
	delete_button = createToolButton(QIcon(QString::fromLatin1(":/images/minus.png")), tr("Remove"));
	
	auto* add_remove_layout = new SegmentedButtonLayout();
	add_remove_layout->addWidget(new_button);
	add_remove_layout->addWidget(delete_button);
	
	move_up_button = createToolButton(QIcon(QString::fromLatin1(":/images/arrow-up.png")), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_down_button = createToolButton(QIcon(QString::fromLatin1(":/images/arrow-down.png")), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	
	auto* up_down_layout = new SegmentedButtonLayout();
	up_down_layout->addWidget(move_up_button);
	up_down_layout->addWidget(move_down_button);
	
	move_by_hand_action = new QAction(QIcon(QString::fromLatin1(":/images/move.png")), tr("Move by hand"), this);
	move_by_hand_action->setCheckable(true);
	move_by_hand_button = createToolButton(move_by_hand_action->icon(), move_by_hand_action->text());
	move_by_hand_button->setDefaultAction(move_by_hand_action);
	move_by_hand_button->setVisible(!mobile_mode);
	adjust_button = createToolButton(QIcon(QString::fromLatin1(":/images/georeferencing.png")), tr("Adjust..."));
	adjust_button->setCheckable(true);
	adjust_button->setVisible(!mobile_mode);
	
	auto* edit_menu = new QMenu(this);
	georef_action = edit_menu->addAction(tr("Georeferenced"), this, &TemplateListWidget::changeGeorefClicked);
	georef_action->setCheckable(true);
	position_action = edit_menu->addAction(tr("Positioning..."));
	position_action->setCheckable(true);
	edit_menu->addSeparator();
	custom_name_action = edit_menu->addAction(tr("Enter custom name..."), this, &TemplateListWidget::enterCustomname);
	show_name_action = edit_menu->addAction(tr("Show filename"), this, &TemplateListWidget::showCustomOrFilename);
	edit_menu->addSeparator();
	import_action =  edit_menu->addAction(tr("Import and remove"), this, &TemplateListWidget::importClicked);
	
#if WITH_COVE
	vectorize_action = edit_menu->addAction(tr("Vectorize lines"), this, &TemplateListWidget::vectorizeClicked);
#else
	vectorize_action = nullptr;
#endif /* WITH_COVE */

	edit_button = createToolButton(QIcon(QString::fromLatin1(":/images/settings.png")),
	                            ::OpenOrienteering::MapEditorController::tr("&Edit").remove(QLatin1Char('&')));
	edit_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	edit_button->setPopupMode(QToolButton::InstantPopup);
	edit_button->setMenu(edit_menu);
	edit_button->setVisible(!mobile_mode);
	
	// The buttons row layout
	auto* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(add_remove_layout);
	list_buttons_layout->addLayout(up_down_layout);
	list_buttons_layout->addWidget(adjust_button);
	list_buttons_layout->addWidget(move_by_hand_button);
	list_buttons_layout->addWidget(edit_button);
	
	list_buttons_group = new QWidget();
	list_buttons_group->setLayout(list_buttons_layout);
	
	auto* all_buttons_layout = new QHBoxLayout();
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
		auto help_button = createToolButton(QIcon(QString::fromLatin1(":/images/help.png")), tr("Help"));
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
	
	updateVisibility(MapView::MultipleFeatures, true);
	
	// Connections
	connect(all_hidden_check, &QAbstractButton::toggled, &controller, &MapEditorController::hideAllTemplates);
	
	connect(template_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &TemplateListWidget::setButtonsDirty);
	connect(template_table->model(), &QAbstractTableModel::rowsMoved, this, &TemplateListWidget::setButtonsDirty);
	connect(template_table->model(), &QAbstractTableModel::dataChanged, this, &TemplateListWidget::setButtonsDirty);
	connect(template_table, &QTableView::clicked, this, &TemplateListWidget::itemClicked, Qt::QueuedConnection);
	connect(template_table, &QTableView::doubleClicked, this, &TemplateListWidget::itemDoubleClicked, Qt::QueuedConnection);
	
	connect(delete_button, &QAbstractButton::clicked, this, &TemplateListWidget::deleteTemplate);
	connect(move_up_button, &QAbstractButton::clicked, this, &TemplateListWidget::moveTemplateUp);
	connect(move_down_button, &QAbstractButton::clicked, this, &TemplateListWidget::moveTemplateDown);
	
	connect(move_by_hand_action, &QAction::triggered, this, &TemplateListWidget::moveByHandClicked);
	connect(adjust_button, &QAbstractButton::clicked, this, &TemplateListWidget::adjustClicked);
	connect(position_action, &QAction::triggered, this, &TemplateListWidget::positionClicked);
	
	//connect(group_button, SIGNAL(clicked(bool)), this, &TemplateListWidget::groupClicked);
	//connect(more_button_menu, SIGNAL(triggered(QAction*)), this, SLOT(moreActionClicked(QAction*)));
	
	connect(&main_view, &MapView::visibilityChanged, this, &TemplateListWidget::updateVisibility);
	connect(&controller, &MapEditorController::templatePositionDockWidgetClosed, this, &TemplateListWidget::templatePositionDockWidgetClosed);
}



inline
TemplateTableModel* TemplateListWidget::model()
{
	return qobject_cast<TemplateTableModel*>(template_table->model());
}

QVariant TemplateListWidget::data(int row, int column, int role) const
{
	return template_table->model()->data(template_table->model()->index(row, column), role);
}

void TemplateListWidget::setData(int row, int column, QVariant value, int role)
{
	template_table->model()->setData(template_table->model()->index(row, column), value, role);
}

Qt::ItemFlags TemplateListWidget::flags(int row, int column) const
{
	return template_table->model()->flags(template_table->model()->index(row, column));
}



inline
int TemplateListWidget::currentRow() const
{
	return template_table->currentIndex().row();
}

inline
int TemplateListWidget::posFromRow(int row) const
{
	return model()->posFromRow(row);
}

inline
int TemplateListWidget::rowFromPos(int pos) const
{
	return model()->rowFromPos(pos);
}

Template* TemplateListWidget::currentTemplate()
{
	int current_row = currentRow();
	if (current_row < 0)
		return nullptr;
	int pos = posFromRow(current_row);
	if (pos < 0)
		return nullptr;
	return map.getTemplate(pos);
}



void TemplateListWidget::updateVisibility(MapView::VisibilityFeature feature, bool /*active*/, const Template* /*temp*/)
{
	switch (feature)
	{
	case MapView::MultipleFeatures:
	case MapView::AllTemplatesHidden:
		updateAllTemplatesHidden();
		break;
		
	default:
		;  // nothing
	}
}

void TemplateListWidget::updateAllTemplatesHidden()
{
	auto hidden = main_view.areAllTemplatesHidden();
	all_hidden_check->setChecked(hidden);
	template_table->setEnabled(!hidden);
	list_buttons_group->setEnabled(!hidden);
	updateButtons();
}

void TemplateListWidget::setButtonsDirty()
{
	if (!buttons_dirty)
	{
		buttons_dirty = true;
		QTimer::singleShot(0, this, &TemplateListWidget::updateButtons);
	}
}

void TemplateListWidget::updateButtons()
{
	buttons_dirty = false;
	
	auto const current_row = currentRow();
	move_up_button->setEnabled(current_row > 0);
	move_down_button->setEnabled(current_row >= 0 && current_row < model()->rowCount() - 1);
	
	auto* temp = currentTemplate();
	duplicate_action->setEnabled(bool(temp));
	delete_button->setEnabled(bool(temp));	/// \todo Make it possible to delete multiple templates at once
	
	if (!mobile_mode)
	{
		// Update and show other buttons
		
		bool is_georeferenced = false;
		bool edit_enabled   = false;
		bool georef_enabled = false;
		bool custom_enabled = false;
		bool import_enabled = false;
		bool vectorize_enabled  = false;
		if (bool(temp))
		{
			is_georeferenced = temp->isTemplateGeoreferenced();
			if (data(current_row, TemplateTableModel::visibilityColumn(), Qt::CheckStateRole) == Qt::Checked)
			{
				edit_enabled   = true;
				georef_enabled = temp->canChangeTemplateGeoreferenced();
				custom_enabled = !is_georeferenced;
				import_enabled = bool(qobject_cast<TemplateMap*>(temp));
				vectorize_enabled = qobject_cast<TemplateImage*>(temp)
									&& temp->getTemplateState() == Template::Loaded;
			}
			show_name_action->setText(temp->getCustomnamePreference() ? tr("Show filename") : tr("Show custom name"));
			show_name_action->setVisible(!temp->getTemplateCustomname().isEmpty());
			
		}
		else if (current_row >= 0)
		{
			// map row
			is_georeferenced = map.getGeoreferencing().getState() == Georeferencing::Geospatial;
		}
		
		edit_button->setEnabled(edit_enabled);
		georef_action->setChecked(is_georeferenced);
		georef_action->setEnabled(georef_enabled);
		move_by_hand_action->setEnabled(custom_enabled);
		adjust_button->setEnabled(custom_enabled);
		position_action->setEnabled(custom_enabled);
		import_action->setEnabled(import_enabled);
		if (vectorize_action)
			vectorize_action->setEnabled(vectorize_enabled);
	}
	
	// Not strictly related to buttons, but exactly the same triggers.
	if (current_row != last_row)
	{
		last_row = current_row;
		emit currentRowChanged(current_row);
	}
	if (temp != last_template)
	{
		last_template = temp;
		emit currentTemplateChanged(temp);
	}
}



void TemplateListWidget::itemClicked(const QModelIndex& index)
{
	auto const row = index.row();
	auto const pos = posFromRow(qMax(0, row));
	
	switch (index.column())
	{
	case TemplateTableModel::opacityColumn():
		if (mobile_mode
		    && row >= 0
		    && data(row, 0, Qt::CheckStateRole) == Qt::Checked)
		{
			showOpacitySlider(row);
		}
		break;
		
	case TemplateTableModel::nameColumn():
		if (!mobile_mode
		    && row >= 0 && pos >= 0
		    && map.getTemplate(pos)->getTemplateState() == Template::Invalid)
		{
			changeTemplateFile(pos);
		}
		break;
		
	default:
		break;
	}
}

void TemplateListWidget::itemDoubleClicked(const QModelIndex& index)
{
	auto const row = index.row();
	auto const pos = posFromRow(qMax(0, row));
	
	switch (index.column())
	{
	default:
		if (! (row >= 0 && pos >= 0
		       && map.getTemplate(pos)->getTemplateState() == Template::Invalid))
			break;
		// Invalid template:
		Q_FALLTHROUGH();
	case TemplateTableModel::nameColumn():
		if (!mobile_mode
		    && row >= 0 && pos >= 0)
		{
			changeTemplateFile(pos);
		}
	}
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
				int row = currentRow();
				if (row >= 0 && flags(row, 1).testFlag(Qt::ItemIsEnabled))
				{
					bool is_checked = data(row, 0, Qt::CheckStateRole) != Qt::Unchecked;
					auto check_state = is_checked ? Qt::Unchecked : Qt::Checked;
					setData(row, TemplateTableModel::visibilityColumn(), check_state, Qt::CheckStateRole);
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
				auto map_row = rowFromPos(map.getFirstFrontTemplate()) + 1;
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



std::unique_ptr<Template> TemplateListWidget::showOpenTemplateDialog(QWidget* dialog_parent, MapEditorController& controller)
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
	                                           QCoreApplication::translate("OpenOrienteering::MapEditorController",
	                                                                       "Open template..."),
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
	
	QString error;
	auto new_temp = Template::templateForPath(path, controller.getMap());
	if (!new_temp)
	{
		error = tr("File format not recognized.");
	}
	else if (!new_temp->setupAndLoad(dialog_parent, controller.getMainWidget()->getMapView()))
	{
		error = new_temp->errorString();
		if (new_temp->getTemplateState() == Template::Invalid && error.isEmpty())
			error = tr("Failed to load template. Does the file exist and is it valid?");
		new_temp.reset();
	}
	
	if (!error.isEmpty())
	{
		auto const error_template = tr("Cannot open template\n%1:\n%2");
		QMessageBox::warning(dialog_parent, tr("Error"), error_template.arg(path, error));
	}
	
	return new_temp;
}


#if 0
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
#endif

void TemplateListWidget::openTemplate()
{
	auto new_template = showOpenTemplateDialog(window(), controller);
	if (new_template)
	{
		int pos = -1;
		int row = currentRow();
		if (row >= 0)
			pos = posFromRow(row);
		
		map.addTemplate(pos, std::move(new_template));
	}
}

void TemplateListWidget::deleteTemplate()
{
	int pos = posFromRow(currentRow());
	Q_ASSERT(pos >= 0);
	
	if (Settings::getInstance().getSettingCached(Settings::Templates_KeepSettingsOfClosed).toBool())
		map.closeTemplate(pos);
	else
		map.deleteTemplate(pos);
}

void TemplateListWidget::duplicateTemplate()
{
	int row = currentRow();
	Q_ASSERT(row >= 0);
	int pos = posFromRow(row);
	Q_ASSERT(pos >= 0);
	
	const auto* prototype = map.getTemplate(pos);
	const auto visibility = main_view.getTemplateVisibility(prototype);
	
	auto new_template = prototype->duplicate();
	map.addTemplate(pos, std::unique_ptr<Template>{new_template});
	main_view.setTemplateVisibility(new_template, visibility);
}

void TemplateListWidget::moveTemplateUp()
{
	int row = currentRow();
	Q_ASSERT(row >= 1);
	if (!(row >= 1)) return; // in release mode
	
	int cur_pos = posFromRow(row);
	int above_pos = posFromRow(row - 1);
	
	if (cur_pos < 0)
	{
		// Moving the map layer up
		map.setFirstFrontTemplate(map.getFirstFrontTemplate() + 1);
	}
	else if (above_pos < 0)
	{
		// Moving something above the map layer
		map.setFirstFrontTemplate(map.getFirstFrontTemplate() - 1);
	}
	else
	{
		// Exchanging two templates
		map.moveTemplate(cur_pos, above_pos);
	}
	
	template_table->setCurrentIndex(template_table->model()->index(row - 1, template_table->selectionModel()->currentIndex().column()));
}

void TemplateListWidget::moveTemplateDown()
{
	int row = currentRow();
	Q_ASSERT(row >= 0);
	if (!(row >= 0)) return; // in release mode
	Q_ASSERT(row < template_table->model()->rowCount() - 1);
	if (!(row < template_table->model()->rowCount() - 1)) return; // in release mode
	
	int cur_pos = posFromRow(row);
	int below_pos = posFromRow(row + 1);
	
	if (cur_pos < 0)
	{
		// Moving the map layer down
		map.setFirstFrontTemplate(map.getFirstFrontTemplate() - 1);
	}
	else if (below_pos < 0)
	{
		// Moving something below the map layer
		map.setFirstFrontTemplate(map.getFirstFrontTemplate() + 1);
	}
	else
	{
		// Exchanging two templates
		map.moveTemplate(cur_pos, below_pos);
	}
	
	template_table->setCurrentIndex(template_table->model()->index(row + 1, template_table->selectionModel()->currentIndex().column()));
}

void TemplateListWidget::showHelp()
{
	Util::showHelp(controller.getWindow(), "templates.html", "setup");
}


void TemplateListWidget::moveByHandClicked(bool checked)
{
	MapEditorTool* tool = nullptr;
	if (checked)
	{
		tool = new TemplateMoveTool(currentTemplate(), &controller, move_by_hand_action);
		connect(this, &TemplateListWidget::currentRowChanged, tool, &TemplateMoveTool::deactivate);
	}
	controller.setTool(tool);
}

void TemplateListWidget::adjustClicked(bool checked)
{
	if (checked)
	{
		auto* activity = new TemplateAdjustActivity(currentTemplate(), &controller);
		controller.setEditorActivity(activity);
		connect(this, &TemplateListWidget::currentRowChanged,
		        activity, [this]() { controller.setEditorActivity(nullptr); });		
		connect(activity, &QObject::destroyed,
		        this, [this]() { adjust_button->setChecked(false); });
		
	}
	else
	{
		controller.setEditorActivity(nullptr);	// TODO: default activity?!
	}
}

#ifndef NO_TEMPLATE_GROUP_SUPPORT
void TemplateListWidget::groupClicked()
{
	// TODO
}
#endif

void TemplateListWidget::positionClicked(bool checked)
{
	if (checked)
	{
		auto* dock_widget = new TemplatePositionDockWidget(currentTemplate(), &controller, controller.getWindow());
		controller.addFloatingDockWidget(dock_widget);
		dock_widget->show();
		dock_widget->raise();
		if (dock_widget->isFloating())
			dock_widget->activateWindow();
		connect(&controller, &MapEditorController::destroyed, dock_widget, &TemplateAdjustDockWidget::close);
		connect(this, &TemplateListWidget::currentRowChanged, dock_widget, &TemplateAdjustDockWidget::close);
		connect(this, &TemplateListWidget::closePositionDockWidget, dock_widget, &TemplateAdjustDockWidget::close);
		connect(dock_widget, &TemplatePositionDockWidget::closed,
		        position_action, [this]() { position_action->setChecked(false); });
	}
	else
	{
		emit closePositionDockWidget();
	}
}

void TemplateListWidget::importClicked()
{
	auto* prototype = qobject_cast<const TemplateMap*>(currentTemplate());
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
			template_map.applyOnAllObjects(transform.makeObjectTransform());
			template_map.setGeoreferencing(map.getGeoreferencing());
		}
		auto template_scale = (transform.template_scale_x + transform.template_scale_y) / 2;
		template_scale *= double(prototype->templateMap()->getScaleDenominator()) / map.getScaleDenominator();
		if (!qFuzzyCompare(template_scale, 1))
		{
			template_map.scaleAllSymbols(template_scale);
		}
	}
	else if (qstrcmp(prototype->getTemplateType(), "TemplateMap") == 0)
	{
		auto importer = FileFormats.makeImporter(prototype->getTemplatePath(), template_map, nullptr);
		if (!importer)
		{
			QMessageBox::warning(this, tr("Error"), tr("Cannot load map file, aborting."));
			return;
		}
		if (!importer->doImport())
		{
			QMessageBox::warning(this, tr("Error"), importer->warnings().back());
			return;
		}
		if (!importer->warnings().empty())
		{
			MainWindow::showMessageBox(this, tr("Warning"), tr("The map import generated warnings."), importer->warnings());
		}
		
		if (!prototype->isTemplateGeoreferenced())
			template_map.applyOnAllObjects(transform.makeObjectTransform());
		
		auto nominal_scale = double(template_map.getScaleDenominator()) / map.getScaleDenominator();
		auto current_scale = 0.5 * (transform.template_scale_x + transform.template_scale_y);
		auto scale = 1.0;
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
		QMessageBox::warning(this, tr("Error"), tr("Cannot load map file, aborting."));
		return;
	}

	map.importMap(template_map, Map::MinimalObjectImport);
	deleteTemplate();
	
	if (main_view.isOverprintingSimulationEnabled()
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
			if (auto* action = controller.getAction("overprintsimulation"))
				action->trigger();
		}
	}
	
	
	auto map_visibility = main_view.getMapVisibility();
	if (!map_visibility.visible)
	{
		map_visibility.visible = true;
	}
}

void TemplateListWidget::changeGeorefClicked()
{
	auto* templ = currentTemplate();
	if (templ && templ->canChangeTemplateGeoreferenced())
	{
		auto new_value = !templ->isTemplateGeoreferenced();
		if (new_value)
		{
			// Properly tear down positioning activities
			if (move_by_hand_action->isChecked())
				move_by_hand_action->trigger();
			if (adjust_button->isChecked())
				adjust_button->click();
			if (position_action->isChecked())
				position_action->trigger();
		}
		if (!templ->trySetTemplateGeoreferenced(new_value, this))
		{
			QMessageBox::warning(this, tr("Error"), tr("Cannot change the georeferencing state."));
			georef_action->setChecked(templ->isTemplateGeoreferenced());
		}
	}
}

void TemplateListWidget::vectorizeClicked()
{
#ifdef WITH_COVE
	cove::CoveRunner cr;
	auto* templ = qobject_cast<TemplateImage*>(currentTemplate());
	cr.run(controller.getWindow(), &map, templ);
#endif /* WITH_COVE */
}

void TemplateListWidget::moreActionClicked(QAction* action)
{
	Q_UNUSED(action);
	// TODO
}

void TemplateListWidget::templatePositionDockWidgetClosed(Template* temp)
{
	auto* current_temp = currentTemplate();
	if (current_temp == temp)
		position_action->setChecked(false);
}

void TemplateListWidget::changeTemplateFile(int pos)
{
	auto* temp = map.getTemplate(pos);
	Q_ASSERT(temp);
	temp->execSwitchTemplateFileDialog(this);
}

void TemplateListWidget::showOpacitySlider(int row)
{
	auto geometry = template_table->visualRect(template_table->model()->index(row, 0));
	geometry.translate(0, geometry.height());
	
	QDialog dialog(nullptr, Qt::FramelessWindowHint);
	dialog.move(template_table->viewport()->mapToGlobal(geometry.topLeft()));
	
	auto* slider = new QSlider();
	slider->setOrientation(Qt::Horizontal);
	slider->setRange(0, 20);
	slider->setMinimumWidth(geometry.width());
	
	slider->setValue(qRound(data(row, TemplateTableModel::opacityColumn(), Qt::EditRole).toDouble() * 20));
	connect(slider, &QSlider::valueChanged, this, [this, row](int value) {
		setData(row, TemplateTableModel::opacityColumn(), 0.05 * value, Qt::EditRole);
	} );
	
	auto* close_button = new QToolButton();
	close_button->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, nullptr, this));
	close_button->setAutoRaise(true);
	connect(close_button, &QToolButton::clicked, &dialog, &QDialog::accept);
	
	auto* layout = new QHBoxLayout(&dialog);
	layout->addWidget(slider);
	layout->addWidget(close_button);
	
	dialog.exec();
}

void TemplateListWidget::enterCustomname()
{
	auto* templ = currentTemplate();
	if (templ)
	{
		bool ok;
		const auto previous_customname = templ->getTemplateCustomname();
		const auto text = QInputDialog::getText(this, tr("Enter custom name"), tr("Custom name:"), QLineEdit::Normal, previous_customname, &ok);
		if (ok)
		{
			templ->setTemplateCustomname(text);
			templ->setCustomnamePreference(!text.isEmpty());
			if (previous_customname != text)
			{
				map.setTemplatesDirty();	// only mark as dirty if custom name has changed
				updateButtons();
			}
		}
	}
}

void TemplateListWidget::showCustomOrFilename()
{
	auto* templ = currentTemplate();
	if (templ)
	{
		templ->setCustomnamePreference(!templ->getCustomnamePreference());	// don't mark as dirty
		updateButtons();
	}
}


}  // namespace OpenOrienteering
