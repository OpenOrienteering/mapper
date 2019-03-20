/*
 *    Copyright 2014 Thomas Schöps
 *    Copyright 2017 Kai Pastor
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

#include <algorithm>
#include <iterator>

#include <QCoreApplication>
#include <QAbstractButton>
#include <QEvent>
#include <QFlags>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QToolButton>


namespace OpenOrienteering {

namespace {
	
	int modifierFromKeyCode(Qt::Key key_code)
	{
		switch(key_code)
		{
		case Qt::Key_Control:
			return Qt::ControlModifier;
			
		case Qt::Key_Shift:
			return Qt::ShiftModifier;
			
		case Qt::Key_Alt:
			return Qt::AltModifier;
			
		default:
			return Qt::NoModifier;
		}
		
		Q_UNREACHABLE();
	}
	
	
	int keyFromModifierCode(Qt::KeyboardModifier modifier)
	{
		switch(modifier)
		{
		case Qt::ControlModifier:
			return Qt::Key_Control;
			
		case Qt::ShiftModifier:
			return  Qt::Key_Shift;
			
		case Qt::AltModifier:
			return Qt::Key_Alt;
			
		default:
			return 0;
		}
		
		Q_UNREACHABLE();
	}
	
	
}  // namespace



KeyButtonBar::KeyButtonBar(QWidget* receiver, QWidget* parent, Qt::WindowFlags f)
: QWidget(parent, f)
, receiver(receiver)
, active_modifiers(QGuiApplication::keyboardModifiers())
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(layout->spacing() / 2);
	setLayout(layout);
	
	receiver->installEventFilter(this);
}


KeyButtonBar::~KeyButtonBar() = default;



QToolButton* KeyButtonBar::addKeyButton(Qt::Key key_code, const QString& text, const QIcon& icon)
{
	return addKeyButton(key_code, Qt::NoModifier, text, icon);
}


QToolButton* KeyButtonBar::addKeyButton(Qt::Key key_code, Qt::KeyboardModifier modifier_code, const QString& text, const QIcon& icon)
{
	Q_ASSERT(key_code);
	Q_ASSERT(!modifier_code || !modifierFromKeyCode(key_code));
	auto button = new QToolButton();
	button->setText(text);
	if (!icon.isNull())
		button->setIcon(icon);
	layout->addWidget(button);
	button_infos.push_back({button, key_code, modifier_code});
	connect(button, &QAbstractButton::clicked, this, &KeyButtonBar::sendKeyPressAndRelease);
	return button;
}


QToolButton* KeyButtonBar::addModifierButton(Qt::KeyboardModifier modifier_code, const QString& text, const QIcon& icon)
{
	Q_ASSERT(modifier_code);
	auto button = new QToolButton();
	button->setText(text);
	button->setCheckable(true);
	if (!icon.isNull())
		button->setIcon(icon);
	layout->addWidget(button);
	auto key_code = keyFromModifierCode(modifier_code);
	Q_ASSERT(key_code);
	button_infos.push_back({button, Qt::Key(key_code), modifier_code});
	connect(button, &QAbstractButton::clicked, this, &KeyButtonBar::sendModifierChange);
	return button;
}



bool KeyButtonBar::eventFilter(QObject* /*watched*/, QEvent* event)
{
	if (!active)
		return false;
	
	using std::begin;
	using std::end;
	auto find_info = [this](int key_code) -> ButtonInfo* {
		auto info = std::find_if(begin(button_infos), end(button_infos),
		                         [key_code](const auto& info) { return info.key == key_code; });
		return info == end(button_infos) ? nullptr : &*info;
	};
	
	switch (event->type())
	{
	case QEvent::KeyPress:
		{
			auto final_modifiers = active_modifiers;
			
			auto key_event = static_cast<QKeyEvent*>(event);
			const auto key = Qt::Key(key_event->key());
			if (auto modifier = modifierFromKeyCode(key))
			{
				if (active_modifiers & modifier)
					return true; // Modifier already active. Consume this event.
				
				final_modifiers |= Qt::KeyboardModifier(modifier);
				
				if (auto info = find_info(key))
					info->button->setChecked(true);
			}
			key_event->setModifiers(Qt::KeyboardModifiers(active_modifiers));
			active_modifiers = final_modifiers;
		}
		break;
		
	case QEvent::KeyRelease:
		{
			auto final_modifiers = active_modifiers;
			
			auto key_event = static_cast<QKeyEvent*>(event);
			const auto key = Qt::Key(key_event->key());
			if (auto modifier = modifierFromKeyCode(key))
			{
				if (!(active_modifiers & modifier))
					return true; // Modifier already inactive. Consume this event.
				
				final_modifiers &= ~Qt::KeyboardModifier(modifier);
				
				if (auto info = find_info(key))
					info->button->setChecked(false);
			}
			key_event->setModifiers(Qt::KeyboardModifiers(active_modifiers));
			active_modifiers = final_modifiers;
		}
		break;
		
	case QEvent::MouseMove:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	// Probably add more input events later.
		static_cast<QMouseEvent*>(event)->setModifiers(Qt::KeyboardModifiers(active_modifiers));
		break;
		
	default:
		;  // nothing
	}
	
	return false;
}



void KeyButtonBar::showEvent(QShowEvent* event)
{
	if (!event->spontaneous())
		active = true;
}


void KeyButtonBar::hideEvent(QHideEvent* event)
{
	if (!event->spontaneous())
		active = false;
}



void KeyButtonBar::sendKeyPressAndRelease()
{
	using std::begin;
	using std::end;
	auto button = reinterpret_cast<QToolButton*>(sender());
	auto info = std::find_if(begin(button_infos), end(button_infos),
	                         [button](const auto& info) { return info.button == button; });
	
	Q_ASSERT(info != end(button_infos));
	
	auto original_modifiers = active_modifiers;
	if (info->modifier && !active_modifiers.testFlag(info->modifier))
	{
		sendKeyPressEvent(Qt::Key(keyFromModifierCode(info->modifier)));
		Q_ASSERT(active_modifiers & info->modifier);
	}
	sendKeyPressEvent(info->key);
	sendKeyReleaseEvent(info->key);
	if (active_modifiers != original_modifiers)
	{
		sendKeyReleaseEvent(Qt::Key(keyFromModifierCode(info->modifier)));
		Q_ASSERT(!(active_modifiers & info->modifier));
	}
}


void KeyButtonBar::sendModifierChange(bool checked)
{
	using std::begin;
	using std::end;
	auto button = reinterpret_cast<QToolButton*>(sender());
	auto info = std::find_if(begin(button_infos), end(button_infos),
	                         [button](const auto& info) { return info.button == button; });
	
	Q_ASSERT(info != end(button_infos));
	Q_ASSERT(info->modifier);
	
	if (checked)
	{
		if (!active_modifiers.testFlag(info->modifier))
		{
			sendKeyPressEvent(info->key);
		}
		Q_ASSERT(active_modifiers & info->modifier);
	}
	else
	{
		if (active_modifiers.testFlag(info->modifier))
		{
			sendKeyReleaseEvent(info->key);
		}
	}
	
	Q_ASSERT(active_modifiers.testFlag(info->modifier) == checked);
}



void KeyButtonBar::sendKeyPressEvent(Qt::Key key_code)
{
	QKeyEvent event(QEvent::KeyPress, key_code, active_modifiers);
	QCoreApplication::sendEvent(receiver, &event);
}


void KeyButtonBar::sendKeyReleaseEvent(Qt::Key key_code)
{
	QKeyEvent event(QEvent::KeyRelease, key_code, active_modifiers);
	QCoreApplication::sendEvent(receiver, &event);
}


}  // namespace OpenOrienteering
