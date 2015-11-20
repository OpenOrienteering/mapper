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

#ifndef _OPENORIENTEERING_AUTO_SAVE_H
#define _OPENORIENTEERING_AUTO_SAVE_H

#include <QScopedPointer>

class AutoSavePrivate;

/**
 * @brief Class AutoSave implements auto-saving behaviour.
 * 
 * Auto-saving means that data which has been modified is automatically saved
 * after some period of time if no regular saving (typically triggered by the
 * user) happens before.
 * 
 * Classes which wish to implement auto-saving may inherit from this class.
 * Inheriting classes must implement autoSave(). Furthermore, they must call
 * setAutoSaveNeeded() when changes need to be taken care of by AutoSave, or
 * when normal saving has terminated the need to perform auto-saving.
 * 
 * Auto-saving, as implemented by autoSave(), may succeed, fail temporary (e.g.
 * during editing), or fail permanent (e.g. for lack of disk space).
 * On success or permanent failure, autoSave() will be called again after the
 * regular auto-saving period.
 * On temporary failure, autoSave() will be called again after five seconds.
 * 
 * The auto-save period (in minutes) is taken from the setting
 * Settings::General_AutoSaveInterval.
 */
class AutoSave
{
public:
	/** @brief Possible results of auto-save attempts. */
	enum AutoSaveResult
	{
		Success,          ///< Auto-saving succeeded.
		PermanentFailure, ///< Auto-saving failed for some persistent reason.
		TemporaryFailure  ///< Auto-saving failed for some transient reason and shall be retried soon.
	};
	
	/** @brief Performs an auto-save, if possible. */
	virtual AutoSaveResult autoSave() = 0;
	
	/** @brief Informs AutoSave whether auto-saving is needed or not. */
	void setAutoSaveNeeded(bool);
	
protected:
	/** @brief Initializes the auto-save feature. */
	AutoSave();
	
	/** @brief Destructs the auto-save feature. */
	~AutoSave();
	
private:
	friend class AutoSavePrivate;
	
	QScopedPointer<AutoSavePrivate> auto_save_controller;
};

#endif
