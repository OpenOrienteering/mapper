/*
 *    Copyright 2012, 2013 Jan Dalheimer
 *    Copyright 2012-2017  Kai Pastor
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

#include <Qt>
#include <QtGlobal>
#include <QAbstractButton> // IWYU pragma: keep
#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFlags>
#include <QFrame>
#include <QKeyEvent>
#include <QLabel>
#include <QLatin1String>
#include <QLayout>
#include <QScrollArea>
#include <QScroller>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "settings.h"
#include "gui/util_gui.h"
#include "gui/widgets/editor_settings_page.h"
#include "gui/widgets/general_settings_page.h"
#include "gui/widgets/settings_page.h"
#include "util/backports.h" // IWYU pragma: keep

#ifdef MAPPER_USE_GDAL
#  include "gdal/gdal_settings_page.h"
#endif


namespace OpenOrienteering {

SettingsDialog::SettingsDialog(QWidget* parent)
 : QDialog        { parent }
 , tab_widget     { nullptr }
 , stack_widget   { nullptr }
{
	setWindowTitle(tr("Settings"));
	
	auto layout = new QVBoxLayout(this);
	
	auto buttons = QDialogButtonBox::StandardButtons{ QDialogButtonBox::Ok };
	if (Settings::mobileModeEnforced())
	{
		if (parent)
			setGeometry(parent->geometry());
		
		stack_widget = new QStackedWidget();
		layout->addWidget(stack_widget, 1);
		
		auto menu_widget = new QToolBar();
		menu_widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		menu_widget->setOrientation(Qt::Vertical);
		stack_widget->addWidget(menu_widget);
	}
	else
	{
		buttons |=  QDialogButtonBox::Reset | QDialogButtonBox::Cancel | QDialogButtonBox::Help;
		
		tab_widget = new QTabWidget();
#ifndef Q_OS_MACOS
		tab_widget->setDocumentMode(true);
#endif
		layout->addWidget(tab_widget, 1);
	}
	
	button_box = new QDialogButtonBox(buttons, Qt::Horizontal);
	connect(button_box, &QDialogButtonBox::clicked, this, &SettingsDialog::buttonPressed);
	if (stack_widget)
	{
		int left, top, right, bottom;
		layout->getContentsMargins(&left, &top, &right, &bottom);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		
		auto l = new QVBoxLayout();
		l->setContentsMargins(left, top, right, bottom);
		l->addWidget(button_box);
		layout->addLayout(l);
	}
	else
	{
		layout->addWidget(button_box);
	}
	
	addPages();
}

SettingsDialog::~SettingsDialog() = default;



void SettingsDialog::closeEvent(QCloseEvent* event)
{
	if (stack_widget)
		callOnAllPages(&SettingsPage::apply);
	QDialog::closeEvent(event);
}

void SettingsDialog::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Back:
	case Qt::Key_Escape:
		if (stack_widget && stack_widget->currentIndex() > 0)
		{
			stack_widget->setCurrentIndex(0);
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
	if (stack_widget)
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
		stack_widget->addWidget(scrollarea);
		
		auto menu_widget = qobject_cast<QToolBar*>(stack_widget->widget(0));
		auto action = menu_widget->addAction(page->title());
		connect(action, &QAction::triggered, this, [this, scrollarea]()
		{
			stack_widget->setCurrentWidget(scrollarea);
			scrollarea->ensureVisible(0, 0);
			button_box->setStandardButtons(button_box->standardButtons() | QDialogButtonBox::Reset);
		} );
	}
	else
	{
		tab_widget->addTab(page, page->title());
	}
}

void SettingsDialog::callOnAllPages(void (SettingsPage::*member)())
{
	auto const pages = stack_widget ? stack_widget->findChildren<SettingsPage*>()
	                                : tab_widget->findChildren<SettingsPage*>();
	for (auto* page : pages)
		(page->*member)();
}

void SettingsDialog::buttonPressed(QAbstractButton* button)
{
	QDialogButtonBox::StandardButton id = button_box->standardButton(button);
	switch (id)
	{
	case QDialogButtonBox::Ok:
		callOnAllPages(&SettingsPage::apply);
		if (stack_widget && stack_widget->currentIndex() > 0)
		{
			stack_widget->setCurrentIndex(0);
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


}  // namespace OpenOrienteering
