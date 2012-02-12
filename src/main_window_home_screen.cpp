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


#include <QtGui>

#include "main_window_home_screen.h"
#include "global.h"

// ### HomeScreenController ###

HomeScreenController::HomeScreenController()
{

}
HomeScreenController::~HomeScreenController()
{

}

void HomeScreenController::attach(MainWindow* window)
{
	this->window = window;
	widget = new HomeScreenWidget(this);
	window->setCentralWidget(widget);
}
void HomeScreenController::detach()
{
	window->setCentralWidget(NULL);
	delete widget;
}

void HomeScreenController::recentFilesUpdated()
{
	widget->recentFilesUpdated();
}

// ### HomeScreenWidget ###

HomeScreenWidget::HomeScreenWidget(HomeScreenController* controller, QWidget* parent) : QWidget(parent), controller(controller)
{
	QLabel* title_label = new QLabel(QString("<img src=\":/images/title.png\"/>"));	// <br/>") + APP_VERSION
	title_label->setAlignment(Qt::AlignCenter);
	
	maps_widget = new DocumentSelectionWidget(tr("Maps"), tr("Create a new map ..."), tr("Open map ..."), tr("Recent maps"),
																	   tr("Open Map ..."), tr("Maps (*.omap *.ocd);;All files (*.*)"));
	HomeScreenTipOfTheDayWidget* tips_widget = new HomeScreenTipOfTheDayWidget(controller);
	HomeScreenOtherWidget* other_widget = new HomeScreenOtherWidget();
	
	QGridLayout* layout = new QGridLayout();
	layout->setSpacing(2 * layout->spacing());
	layout->addWidget(title_label, 0, 0, 1, 2);
	layout->addWidget(maps_widget, 1, 0, 2, 1);
	layout->addWidget(tips_widget, 1, 1);
	layout->addWidget(other_widget, 2, 1);
	layout->setRowStretch(1, 1);
	setLayout(layout);
	
	setAutoFillBackground(false);
	
	connect(maps_widget, SIGNAL(newClicked()), controller->getWindow(), SLOT(showNewMapWizard()));
	connect(maps_widget, SIGNAL(pathOpened(QString)), controller->getWindow(), SLOT(openPath(QString)));
	
	connect(other_widget, SIGNAL(settingsClicked()), controller->getWindow(), SLOT(showSettings()));
	connect(other_widget, SIGNAL(aboutClicked()), controller->getWindow(), SLOT(showAbout()));
	connect(other_widget, SIGNAL(helpClicked()), controller->getWindow(), SLOT(showHelp()));
}
HomeScreenWidget::~HomeScreenWidget()
{
}

void HomeScreenWidget::recentFilesUpdated()
{
	maps_widget->updateRecentFiles();
}

void HomeScreenWidget::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	
	// Background
	p.setPen(Qt::NoPen);
	p.setBrush(Qt::gray);
	p.drawRect(rect());
}

// ### DocumentSelectionWidget ###

DocumentSelectionWidget::DocumentSelectionWidget(const QString& title, const QString& new_text, const QString& open_text,
												 const QString& recent_text, const QString& open_title_text, const QString& open_filter_text, QWidget* parent)
 : QWidget(parent), timer(NULL), open_title_text(open_title_text), open_filter_text(open_filter_text)
{
	QLabel* title_label = new QLabel(title);
	QFont title_font = title_label->font();
	title_font.setPointSize(3 * title_font.pointSize());
	title_font.setBold(true);
	title_label->setFont(title_font);
	title_label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	
	QCommandLinkButton* new_button = new QCommandLinkButton(new_text);
	new_button->setIcon(QIcon(":/images/new.png"));
	QCommandLinkButton* open_button = new QCommandLinkButton(open_text);
	open_button->setIcon(QIcon(":/images/open.png"));
	
	QFont button_font = new_button->font();
	button_font.setPointSize((int)(1.5 * button_font.pointSize()));
	new_button->setFont(button_font);
	open_button->setFont(button_font);
	
	QLabel* recent_label = new QLabel(recent_text);
	recent_docs_list = new QListWidget();
	recent_docs_list->setCursor(Qt::PointingHandCursor);
	updateRecentFiles();
	
	QPushButton* clear_list_button = new QPushButton(tr("Clear list"));
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(title_label);
	layout->addSpacing(16);
	layout->addWidget(new_button);
	layout->addWidget(open_button);
	layout->addSpacing(16);
	layout->addWidget(recent_label);
	layout->addWidget(recent_docs_list);
	layout->addWidget(clear_list_button);
	setLayout(layout);
	
	setAutoFillBackground(false);
	
	connect(new_button, SIGNAL(clicked(bool)), this, SLOT(newDoc()));
	connect(open_button, SIGNAL(clicked(bool)), this, SLOT(openDoc()));
	connect(recent_docs_list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(recentFileClicked(QListWidgetItem*)));
	connect(clear_list_button, SIGNAL(clicked(bool)), this, SLOT(clearListClicked()));
}
DocumentSelectionWidget::~DocumentSelectionWidget()
{
	delete timer;
}

void DocumentSelectionWidget::refreshRecentDocs()
{
	// TODO
}

void DocumentSelectionWidget::newDoc()
{
	emit(newClicked());
}
void DocumentSelectionWidget::openDoc()
{
	// TODO: save directory
	QString path = QFileDialog::getOpenFileName(this, open_title_text, QString(), open_filter_text);
	path = QFileInfo(path).canonicalFilePath();
	
	if (path.isEmpty())
		return;
	
	emit(pathOpened(path));
}

void DocumentSelectionWidget::updateRecentFiles()
{
	recent_docs_list->clear();
	
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	
	int num_recent_files = files.size();
	for (int i = 0; i < num_recent_files; ++i) {
		QString text = tr("%1").arg(QFileInfo(files[i]).fileName());
		
		QListWidgetItem* new_item = new QListWidgetItem(text);
		new_item->setData(Qt::UserRole, files[i]);
		recent_docs_list->addItem(new_item);
	}
}
void DocumentSelectionWidget::recentFileClicked(QListWidgetItem* item)
{
	if (timer)
		return;
	
	path = item->data(Qt::UserRole).toString();
	
	timer = new QTimer();
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(emitPathOpened()));
	timer->start(0);
}
void DocumentSelectionWidget::emitPathOpened()
{
	delete timer;
	timer = NULL;
	emit(pathOpened(path));
}
void DocumentSelectionWidget::clearListClicked()
{
	QSettings settings;
	settings.setValue("recentFileList", QStringList());
	
	foreach (QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* main_window = qobject_cast<MainWindow*>(widget);
		if (main_window)
			main_window->updateRecentFileActions(true);
	}
}

void DocumentSelectionWidget::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	
	// Background
	p.setPen(Qt::NoPen);
	p.setBrush(palette().window());
	p.drawRect(rect());
}

// ### HomeScreenTipOfTheDayWidget ###

HomeScreenTipOfTheDayWidget::HomeScreenTipOfTheDayWidget(HomeScreenController* controller, QWidget* parent) : QWidget(parent), controller(controller)
{
	QLabel* title_label = new QLabel(tr("Tip of the day"));
	QFont title_font = title_label->font();
	title_font.setPointSize(3 * title_font.pointSize());
	title_font.setBold(true);
	title_label->setFont(title_font);
	title_label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	
	tip_label = new QLabel(getNextTip());
	tip_label->setWordWrap(true);
	
	QPushButton* previous_button = new QPushButton(QIcon(":/images/arrow-left.png"), tr("Previous"));
	QPushButton* next_button = new QPushButton(QIcon(":/images/arrow-right.png"), tr("Next"));
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(previous_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(next_button);
	
	QBoxLayout* layout;
	//if (width() > height())
	//	layout = new QHBoxLayout();
	//else
		layout = new QVBoxLayout();
	layout->addWidget(title_label);
	layout->addStretch(1);
	layout->addWidget(tip_label);
	layout->addStretch(1);
	layout->addLayout(buttons_layout);
	setLayout(layout);

	setAutoFillBackground(true);
	
	connect(previous_button, SIGNAL(clicked(bool)), this, SLOT(previousClicked()));
	connect(next_button, SIGNAL(clicked(bool)), this, SLOT(nextClicked()));
	connect(tip_label, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));
}

void HomeScreenTipOfTheDayWidget::previousClicked()
{
	tip_label->setText(getNextTip(-1));
}
void HomeScreenTipOfTheDayWidget::nextClicked()
{
	tip_label->setText(getNextTip(1));
}
void HomeScreenTipOfTheDayWidget::linkClicked(QString link)
{
	if (link.compare("settings", Qt::CaseInsensitive) == 0)
		controller->getWindow()->showSettings();
	else if (link.compare("help", Qt::CaseInsensitive) == 0)
		controller->getWindow()->showHelp();
	else if (link.compare("about", Qt::CaseInsensitive) == 0)
		controller->getWindow()->showAbout();
	else
		QDesktopServices::openUrl(link);
}

QString HomeScreenTipOfTheDayWidget::getNextTip(int direction)
{
	QFile file(":/etc/tipoftheday.txt");
	if (!file.open(QIODevice::ReadOnly))
		return "";
	
	direction = (direction > 0) ? 1 : -1;
	
	QString tip_count_string = file.readLine();
	if (tip_count_string.endsWith('\n'))
		tip_count_string.chop(1);
	int tip_count = tip_count_string.toInt();
	
	QSettings settings;
	settings.beginGroup("TipOfTheDay");
	int number = settings.value("number", 0).toInt();
	number += direction;
	if (number > tip_count)
		number = 1;
	else if (number < 1)
		number = tip_count;
	settings.setValue("number", number);
	
	for (int i = 0; i < number - 1; ++i)
		file.readLine();
	
	QString tip = file.readLine();
	if (tip.endsWith('\n'))
		tip.chop(1);
	return tip;
}

// ### HomeScreenOtherWidget ###

HomeScreenOtherWidget::HomeScreenOtherWidget(QWidget* parent) : QWidget(parent)
{
	QCommandLinkButton* settings_button = new QCommandLinkButton(tr("Settings"));
	settings_button->setIcon(QIcon(":/images/settings.png"));
	QCommandLinkButton* about_button = new QCommandLinkButton(tr("About"));
	about_button->setIcon(QIcon(":/images/about.png"));
	QCommandLinkButton* help_button = new QCommandLinkButton(tr("Help"));
	help_button->setIcon(QIcon(":/images/help.png"));
	
	QFont button_font = settings_button->font();
	button_font.setPointSize((int)(1.5 * button_font.pointSize()));
	settings_button->setFont(button_font);
	about_button->setFont(button_font);
	help_button->setFont(button_font);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(settings_button);
	layout->addWidget(about_button);
	layout->addWidget(help_button);
	setLayout(layout);
	
	setAutoFillBackground(true);
	
	connect(settings_button, SIGNAL(clicked(bool)), this, SLOT(settingsSlot()));
	connect(about_button, SIGNAL(clicked(bool)), this, SLOT(aboutSlot()));
	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(helpSlot()));
}
void HomeScreenOtherWidget::aboutSlot()
{
	emit(aboutClicked());
}
void HomeScreenOtherWidget::settingsSlot()
{
	emit(settingsClicked());
}
void HomeScreenOtherWidget::helpSlot()
{
	emit(helpClicked());
}

#include "moc_main_window_home_screen.cpp"
