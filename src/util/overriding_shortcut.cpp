/*
 *    Copyright 2013-2019 Kai Pastor
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

#include "overriding_shortcut.h"

#include <QEvent>
#include <QKeyEvent>
#include <QShortcutEvent>
#include <QWidget>


namespace OpenOrienteering {

// ### OverridingShortcut ###

OverridingShortcut::OverridingShortcut(QWidget* parent)
 : OverridingShortcut(QKeySequence{}, parent, nullptr, nullptr, Qt::WindowShortcut)
{}

OverridingShortcut::OverridingShortcut(const QKeySequence& key, QWidget* parent, const char* member, const char* ambiguousMember, Qt::ShortcutContext context)
 : QShortcut(key, parent, member, ambiguousMember, context)
 , parent_or_self(parent)
{
	if (!parent_or_self)
		parent_or_self = this;
	parent_or_self->installEventFilter(this);
	updateEventFilters();
}


OverridingShortcut::~OverridingShortcut()
{
	parent_or_self->removeEventFilter(this);
	if (window_)
		window_->removeEventFilter(this);
}



bool OverridingShortcut::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::ParentChange
	    && (watched == parent_or_self || watched == window_) )
	{
		updateEventFilters();
	}
	else if (event->type() == QEvent::ShortcutOverride
	         && isEnabled() && key().count() == 1)
	{
		auto* key_event = static_cast<QKeyEvent*>(event);
		if ((key_event->key() | int(key_event->modifiers())) == key()[0])
		{
			QShortcutEvent se(key(), id());
			event->setAccepted(QShortcut::event(&se));
			return event->isAccepted();
		}
	}
	
	return false;
}


void OverridingShortcut::updateEventFilters()
{
	QObject* new_parent_or_self = parent() ? parent() : this; 
	if (parent_or_self != new_parent_or_self)
	{
		parent_or_self->removeEventFilter(this);
		parent_or_self = new_parent_or_self;
		parent_or_self->installEventFilter(this);
	}
	
	QWidget* parent_widget = qobject_cast<QWidget*>(parent());
	QWidget* new_window = parent_widget ? parent_widget->window() : nullptr;
	if (window_ != new_window)
	{
		if (window_)
			window_->removeEventFilter(this);
		window_ = new_window;
		if (window_)
			window_->installEventFilter(this);
	}
}


}  // namespace OpenOrienteering
