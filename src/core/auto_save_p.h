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

#ifndef _OPENORIENTEERING_AUTO_SAVE_P_H
#define _OPENORIENTEERING_AUTO_SAVE_P_H

#include <QObject>
#include <QTimer>

#include "auto_save.h"

/**
 * @brief AutoSavePrivate is a helper class of AutoSave.
 * 
 * AutoSavePrivate implements most of AutoSave's behaviour.
 * AutoSave is meant to be used through inheritance.
 * Due to the implemenation of QObject and moc, only the first inherited class
 * may be derived from QObject. That is why AutoSave itself is not derived from
 * QObject, but rather uses this helper class.
 */
class AutoSavePrivate : public QObject
{
Q_OBJECT
public:	
	AutoSavePrivate(AutoSave& document);
	
	virtual ~AutoSavePrivate();
	
	bool autoSaveNeeded();
	
	void setAutoSaveNeeded(bool);
	
public slots:
	void autoSave();
	
	void settingsChanged();
	
private:
	Q_DISABLE_COPY(AutoSavePrivate)
	
	AutoSave& document;
	QTimer* auto_save_timer;
	bool auto_save_needed;
	int  auto_save_interval;
};

#endif
