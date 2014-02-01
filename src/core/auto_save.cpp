/*
 *    Copyright 2014 Kai Pastor
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

#include "auto_save.h"
#include "auto_save_p.h"

#include "../settings.h"

AutoSavePrivate::AutoSavePrivate(AutoSave& document)
: document(document),
  auto_save_needed(false)
{
	auto_save_timer = new QTimer(this);
	auto_save_timer->setSingleShot(true);
	connect(auto_save_timer, SIGNAL(timeout()), this, SLOT(autoSave()));
	connect(&Settings::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
	settingsChanged();
}

AutoSavePrivate::~AutoSavePrivate()
{
	// nothing, not inlined
}

void AutoSavePrivate::settingsChanged()
{
	// Normally, the auto-save interval can be stored as an integer.
	// It is loaded as a double here to allow for faster unit testing.
	auto_save_interval = Settings::getInstance().getSetting(Settings::General_AutoSaveInterval).toDouble() * 60000;
	if (auto_save_interval < 1000)
	{
		// stop auto-save
		auto_save_interval = 0;
		auto_save_timer->stop();
	}
	else if (auto_save_needed && !auto_save_timer->isActive())
	{
		// start auto-save
		auto_save_timer->setInterval(auto_save_interval);
		auto_save_timer->start();
	}
}

bool AutoSavePrivate::autoSaveNeeded()
{
	return auto_save_needed;
}

void AutoSavePrivate::setAutoSaveNeeded(bool needed)
{
	auto_save_needed = needed;
	if (auto_save_interval)
	{
		// auto-saving enabled
		if (auto_save_needed && !auto_save_timer->isActive())
		{
			auto_save_timer->setInterval(auto_save_interval);
			auto_save_timer->start();
		}
		else if (!auto_save_needed && auto_save_timer->isActive())
		{
			auto_save_timer->stop();
		}
	}
}

void AutoSavePrivate::autoSave()
{
	AutoSave::AutoSaveResult result = document.autoSave();
	if (auto_save_interval)
	{
		switch (result)
		{
		case AutoSave::TemporaryFailure:
			auto_save_timer->setInterval(5000);
			auto_save_timer->start();
			break;
		case AutoSave::Success:
		case AutoSave::PermanentFailure:
			auto_save_timer->setInterval(auto_save_interval);
			auto_save_timer->start();
			break;
		default:
			Q_ASSERT(false && "Return value of autoSave() is not a valid AutoSave::AutoSaveResult.");
		}
	}
}



AutoSave::AutoSave()
: auto_save_controller(new AutoSavePrivate(*this))
{
	// Nothing, not inlined
}

AutoSave::~AutoSave()
{
	// Nothing, not inlined
}

void AutoSave::setAutoSaveNeeded(bool needed)
{
	auto_save_controller->setAutoSaveNeeded(needed);
}
