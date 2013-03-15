/*
 *    Copyright 2013 Kai Pastor
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

#ifndef _OPENORIENTEERING_MODIFIER_KEY_H
#define _OPENORIENTEERING_MODIFIER_KEY_H

#include <QKeySequence>
#include <QString>

/**
 * A class that helps to deal efficiently with platform and localization issues
 * of modifier keys.
 * 
 * It is based on QKeySequence::toString(QKeySequence::NativeText) which provides
 * localization and deals with swapping Ctrl and Cmd on Mac OS X. In contrast
 * to QKeySequence, ModifierKey has an implicit operator for casting to QString,
 * and it removes the trailing '+' from pseudo key sequences which consist of
 * modifier keys only. Static methods provide efficient translations of the
 * pure modifier keys.
 * 
 * For true QKeySequences, call QKeySequence::toString(QKeySequence::NativeText)
 * directly.
 * 
 * Synopsis:
 * 
 *     QString text = tr("%1+Click to add a point.").arg(ModifierKey::ctrl());
 *     QString more = tr("%1+Click to select a point.").arg(ModifierKey(Qt::ALT + Qt::ShiftModifier));
 * 
 *     // BUT:
 *     QString help = help_action.shortcut().toString(QKeySequence::NativeText);
 */
class ModifierKey
{
public:
	/** Constructs a new ModifierKey for the given combination of KeyboardModifiers. */
	explicit ModifierKey(Qt::KeyboardModifiers keys);
	
	/** Constructs a new ModifierKey for the given combination of KeyboardModifiers. */
	explicit ModifierKey(int keys);
	
	/** Returns a string representation for user interface purposes.
	  * Will be used for implicit type casts. */
	operator const QString&() const;
	
	/** Returns a shared Alt modifier key. */
	static const ModifierKey alt();
	
	/** Returns a shared Control modifier key. */
	static const ModifierKey control();
	
	/** Returns a shared Control+Shift modifier key. */
	static const ModifierKey controlShift();
	
	/** Returns a shared Meta modifier key. */
	static const ModifierKey meta();
	
	/** Returns a shared Shift modifier key. */
	static const ModifierKey shift();
	
private:
	/** The native text (localized, adapted to the system). */
	QString native_text;
};



// Implementation

inline
ModifierKey::ModifierKey(Qt::KeyboardModifiers keys)
{
	native_text = QKeySequence((int)keys).toString(QKeySequence::NativeText);
	if (native_text.endsWith('+'))
	{
		native_text.chop(1);
	}
}

inline
ModifierKey::ModifierKey(int keys)
{
	native_text = QKeySequence(keys).toString(QKeySequence::NativeText);
	if (native_text.endsWith('+'))
	{
		native_text.chop(1);
	}
}

inline
ModifierKey::operator const QString&() const
{
	return native_text;
}


#endif