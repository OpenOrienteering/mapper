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

#include "autosave.h"
#include "autosave_p.h"

#include <QtGlobal>
#include <QLatin1String>
#include <QString>
#include <QTimer>
#include <QVariant>

#include "settings.h"


namespace OpenOrienteering {

AutosavePrivate::AutosavePrivate(Autosave& document)
: document(document)
, autosave_needed(false)
{
#ifdef QT_TESTLIB_LIB
	// The AutosaveTest uses a very short interval. By using a precise timer,
	// we try to avoid occasional AutosaveTest failures on macOS.
	autosave_timer.setTimerType(Qt::PreciseTimer);
#endif
	autosave_timer.setSingleShot(true);
	connect(&autosave_timer, &QTimer::timeout, this, &AutosavePrivate::autosave);
	connect(&Settings::getInstance(), &Settings::settingsChanged, this, &AutosavePrivate::settingsChanged);
	settingsChanged();
}

AutosavePrivate::~AutosavePrivate()
{
	// nothing, not inlined
}

void AutosavePrivate::settingsChanged()
{
	// Normally, the autosave interval can be stored as an integer.
	// It is loaded as a double here to allow for faster unit testing.
	autosave_interval = qRound(Settings::getInstance().getSetting(Settings::General_AutosaveInterval).toDouble() * 60000);
	if (autosave_interval < 1000)
	{
		// stop autosave
		autosave_interval = 0;
		autosave_timer.stop();
	}
	else if (autosave_needed && !autosave_timer.isActive())
	{
		// start autosave
		autosave_timer.setInterval(autosave_interval);
		autosave_timer.start();
	}
}

bool AutosavePrivate::autosaveNeeded()
{
	return autosave_needed;
}

void AutosavePrivate::setAutosaveNeeded(bool needed)
{
	autosave_needed = needed;
	if (autosave_interval)
	{
		// autosaving enabled
		if (autosave_needed && !autosave_timer.isActive())
		{
			autosave_timer.setInterval(autosave_interval);
			autosave_timer.start();
		}
		else if (!autosave_needed && autosave_timer.isActive())
		{
			autosave_timer.stop();
		}
	}
}

void AutosavePrivate::autosave()
{
	Autosave::AutosaveResult result = document.autosave();
	if (autosave_interval)
	{
		switch (result)
		{
		case Autosave::TemporaryFailure:
			autosave_timer.setInterval(5000);
			autosave_timer.start();
			return;
		case Autosave::Success:
		case Autosave::PermanentFailure:
			autosave_timer.setInterval(autosave_interval);
			autosave_timer.start();
			return;
		}
		Q_UNREACHABLE();
	}
}



// ### Autosave ###

Autosave::Autosave()
: autosave_controller(new AutosavePrivate(*this))
{
	// Nothing, not inlined
}

Autosave::~Autosave()
{
	// Nothing, not inlined
}

QString Autosave::autosavePath(const QString &path) const
{
	return path + QLatin1String(".autosave");
}

void Autosave::setAutosaveNeeded(bool needed)
{
	autosave_controller->setAutosaveNeeded(needed);
}

bool Autosave::autosaveNeeded() const
{
	return autosave_controller->autosaveNeeded();
}


}  // namespace OpenOrienteering
