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

#ifndef OPENORIENTEERING_AUTOSAVE_P_H
#define OPENORIENTEERING_AUTOSAVE_P_H

#include <QObject>
#include <QTimer>

#include "autosave.h"

namespace OpenOrienteering {


/**
 * @brief AutosavePrivate is a helper class of Autosave.
 * 
 * AutosavePrivate implements most of Autosave's behaviour.
 * Autosave is meant to be used through inheritance.
 * Due to the implementation of QObject and moc, only the first inherited class
 * may be derived from QObject. That is why Autosave itself is not derived from
 * QObject, but rather uses this helper class.
 */
class AutosavePrivate : public QObject
{
Q_OBJECT
public:	
	AutosavePrivate(Autosave& document);
	
	~AutosavePrivate() override;
	
	bool autosaveNeeded();
	
	void setAutosaveNeeded(bool);
	
public slots:
	void autosave();
	
	void settingsChanged();
	
private:
	Q_DISABLE_COPY(AutosavePrivate)
	
	Autosave& document;
	QTimer autosave_timer;
	bool autosave_needed;
	int  autosave_interval;
};


}  // namespace OpenOrienteering

#endif
