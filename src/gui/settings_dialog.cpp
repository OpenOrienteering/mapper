/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2016  Kai Pastor
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

#include "settings_dialog.h"

#include <QAbstractButton>
#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>
#include <QScroller>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "../util.h"
#include "../util/backports.h"

#include "main_window.h"
#include "widgets/editor_settings_page.h"
#include "widgets/general_settings_page.h"
#ifdef MAPPER_USE_GDAL
#  include "../gdal/gdal_settings_page.h"
#endif


SettingsDialog::SettingsDialog(QWidget* parent)
 : QDialog        { parent }
 , tab_bar_widget { nullptr }
{
	setWindowTitle(tr("Settings"));
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	
	pages_widget = new QStackedWidget();
	auto buttons = QDialogButtonBox::StandardButtons{ QDialogButtonBox::Ok };
	if (MainWindow::mobileMode())
	{
		if (parent)
			setGeometry(parent->geometry());
		
		auto menu_widget = new QToolBar();
		menu_widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		menu_widget->setOrientation(Qt::Vertical);
		pages_widget->addWidget(menu_widget);
	}
	else
	{
		buttons |=  QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Help;
		tab_bar_widget = new QTabBar();
		tab_bar_widget->setDocumentMode(true);
		connect(tab_bar_widget, &QTabBar::currentChanged, pages_widget, &QStackedWidget::setCurrentIndex);
		layout->addWidget(tab_bar_widget);
	}
	
	addPages();
	layout->addWidget(pages_widget, 1);
	
	button_box = new QDialogButtonBox(buttons, Qt::Horizontal);
	connect(button_box, &QDialogButtonBox::clicked, this, &SettingsDialog::buttonPressed);
	if (MainWindow::mobileMode())
	{
		int left, top, right, bottom;
		layout->getContentsMargins(&left, &top, &right, &bottom);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		
		QVBoxLayout* l = new QVBoxLayout();
		l->setContentsMargins(left, top, right, bottom);
		l->addWidget(button_box);
		layout->addLayout(l);
	}
	else
	{
		layout->addWidget(button_box);
	}
}

SettingsDialog::~SettingsDialog()
{
	// Nothing, not inlined.
}

void SettingsDialog::closeEvent(QCloseEvent* e)
{
	if (MainWindow::mobileMode())
		callOnAllPages(&SettingsPage::apply);
	QDialog::closeEvent(e);
}

void SettingsDialog::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Back:
	case Qt::Key_Escape:
		if (MainWindow::mobileMode() && pages_widget->currentIndex() > 0)
		{
			pages_widget->setCurrentIndex(0);
			auto buttons = button_box->standardButtons();
			button_box->setStandardButtons((buttons & ~QDialogButtonBox::Reset) | QDialogButtonBox::Help);
			return;
		}
		break;
	default:
		; // nothing
	}
	QDialog::keyPressEvent(event);
}

void SettingsDialog::addPages()
{
	addPage(new GeneralSettingsPage(this));
	addPage(new EditorSettingsPage(this));
#ifdef MAPPER_USE_GDAL
	addPage(new GdalSettingsPage(this));
#endif
}

void SettingsDialog::addPage(SettingsPage* page)
{
	if (MainWindow::mobileMode())
	{
		if (auto form_layout = qobject_cast<QFormLayout*>(page->layout()))
		{
			form_layout->setRowWrapPolicy(QFormLayout::WrapAllRows);
			auto labels = page->findChildren<QLabel*>();
			for (auto label : qAsConst(labels))
				label->setWordWrap(true);
		}
		page->setMaximumWidth(width());
		
		auto scrollarea = new QScrollArea();
		scrollarea->setFrameShape(QFrame::NoFrame);
		QScroller::grabGesture(scrollarea, QScroller::TouchGesture);
		scrollarea->setWidget(page);
		pages_widget->addWidget(scrollarea);
		
		auto menu_widget = qobject_cast<QToolBar*>(pages_widget->widget(0));
		auto action = menu_widget->addAction(page->title());
		connect(action, &QAction::triggered, [this, scrollarea]()
		{
			pages_widget->setCurrentWidget(scrollarea);
			button_box->setStandardButtons(button_box->standardButtons() | QDialogButtonBox::Reset);
		} );
	}
	else
	{
		pages_widget->addWidget(page);
		tab_bar_widget->addTab(page->title());
	}
}

void SettingsDialog::callOnAllPages(void (SettingsPage::*member)())
{
	auto pages = pages_widget->findChildren<SettingsPage*>();
	for (auto page : qAsConst(pages))
		(page->*member)();
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	switch (id)
	{
	case QDialogButtonBox::Ok:
		callOnAllPages(&SettingsPage::apply);
		if (MainWindow::mobileMode() && pages_widget->currentIndex() > 0)
		{
			pages_widget->setCurrentIndex(0);
			button_box->setStandardButtons(button_box->standardButtons() & ~QDialogButtonBox::Reset);
		}
		else
		{
			this->accept();
		}
		break;
		
	case QDialogButtonBox::Reset:
		callOnAllPages(&SettingsPage::reset);
		break;
		
	case QDialogButtonBox::Cancel:
		this->reject();
		break;
		
	case QDialogButtonBox::Help:
		Util::showHelp(this, QLatin1String("settings.html"));
		break;
		
	default:
		qDebug("%s: Unexpected button '0x%x'", Q_FUNC_INFO, id);
	}
}
