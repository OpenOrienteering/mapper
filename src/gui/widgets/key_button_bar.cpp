/*
 *    Copyright 2014 Thomas Schöps
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

#include "key_button_bar.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QVariant>
#include <QCoreApplication>
#include <QKeyEvent>

#include "tools/tool.h"
#include "gui/map/map_editor.h"
#include "gui/map/map_widget.h"


KeyButtonBar::KeyButtonBar(MapEditorTool* tool, MapWidget* map_widget, QWidget* parent)
 : QWidget(parent)
 , map_widget(map_widget)
 , tool(tool)
{
	active_modifiers = 0;
	
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(layout->spacing() / 2);
	setLayout(layout);
}

void KeyButtonBar::addPressKey(int key_code, QString text, QIcon icon)
{
	createButton(key_code, text, icon, false, 0);
}

void KeyButtonBar::addPressKeyWithModifier(int key_code, int modifier_code, QString text, QIcon icon)
{
	createButton(key_code, text, icon, false, modifier_code);
}

void KeyButtonBar::addModifierKey(int key_code, int modifier_code, QString text, QIcon icon)
{
	createButton(key_code, text, icon, true, modifier_code);
}

int KeyButtonBar::activeModifiers() const
{
	return active_modifiers;
}

void KeyButtonBar::buttonClicked(bool checked)
{
	QToolButton* button = reinterpret_cast<QToolButton*>(sender());
	ButtonInfo info = button_info.value(button);
	
	if (info.is_checkable)
	{
		if (checked)
			sendKeyPressEvent(info.key_code, info.modifier_code);
		else
			sendKeyReleaseEvent(info.key_code, info.modifier_code);
	}
	else
	{
		active_modifiers |= info.modifier_code;
		sendKeyPressEvent(info.key_code, 0);
		active_modifiers &= ~info.modifier_code;
	}
}

void KeyButtonBar::sendKeyPressEvent(int key_code, int modifier_code)
{
	QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, key_code, Qt::KeyboardModifiers(active_modifiers));
	QCoreApplication::postEvent(map_widget, event);
	//tool->keyPressEvent(&event);
	active_modifiers |= modifier_code;
}

void KeyButtonBar::sendKeyReleaseEvent(int key_code, int modifier_code)
{
	active_modifiers &= ~modifier_code;
	QKeyEvent* event = new QKeyEvent(QEvent::KeyRelease, key_code, Qt::KeyboardModifiers(active_modifiers));
	QCoreApplication::postEvent(map_widget, event);
	//tool->keyReleaseEvent(&event);
}

QToolButton* KeyButtonBar::createButton(int key_code, QString text, QIcon icon, bool is_checkable, int modifier_code)
{
	QToolButton* button = new QToolButton();
	button->setText(text);
	if (! icon.isNull())
		button->setIcon(icon);
	if (is_checkable)
		button->setCheckable(true);
	
	connect(button, SIGNAL(clicked(bool)), this, SLOT(buttonClicked(bool)));
	
	layout->addWidget(button);
	
	ButtonInfo info;
	info.key_code = key_code;
	info.modifier_code = modifier_code;
	info.is_checkable = is_checkable;
	button_info.insert(button, info);
	
	return button;
}
