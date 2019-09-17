/*
 *    Copyright 2013, 2015 Kai Pastor
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

#ifndef OPENORIENTEERING_MODIFIER_KEY_H
#define OPENORIENTEERING_MODIFIER_KEY_H

#include <Qt>
#include <QString>

namespace OpenOrienteering {


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
 * On Windows and Linux, the keys will be displayed as word ("Shift" etc.).
 * On OS X, the keys will be displayed as graphical symbols ("⇧", i.e.
 * Unicode character U+21E7, etc.).
 * 
 * Synopsis:
 * 
 *     QString text = tr("%1+Click to add a point.").arg(ModifierKey::control());
 *     QString more = tr("%1+Click to select a point.").arg(ModifierKey(Qt::ALT + Qt::ShiftModifier));
 * 
 *     // BUT:
 *     QString help = help_action.shortcut().toString(QKeySequence::NativeText);
 */
class ModifierKey
{
protected:
	/** Constructs a new ModifierKey for the given key. */
	explicit ModifierKey(int key);
	
public:
	/** Constructs a new ModifierKey for the given combination of KeyboardModifiers. */
	explicit ModifierKey(Qt::KeyboardModifiers keys);
	
	/** Constructs a new ModifierKey for the given key. */
	explicit ModifierKey(Qt::Key key);
	
	/** Returns a string representation for user interface purposes.
	 * 
	 * This operator is intended to be used for implicit type casts. */
	operator QString() const;
	
	/** Returns a shared Alt modifier key. */
	static const ModifierKey& alt();
	
	/** Returns a shared Control modifier key. */
	static const ModifierKey& control();
	
	/** Returns a shared Control+Shift modifier key. */
	static const ModifierKey& controlShift();
	
	/** Returns a shared Meta modifier key. */
	static const ModifierKey& meta();
	
	/** Returns a shared Shift modifier key. */
	static const ModifierKey& shift();
	
	/** Returns a shared Space key. */
	static const ModifierKey& space();
	
	/** Returns a shared Return key. */
	static const ModifierKey& return_key();
	
	/** Returns a shared Backspace key. */
	static const ModifierKey& backspace();
	
	/** Returns a shared Escape modifier key. */
	static const ModifierKey& escape();
	
private:
	/** The native text (localized, adapted to the system). */
	QString native_text;
};



// Inline implementation

inline
ModifierKey::operator QString() const
{
	return native_text;
}


}  // namespace OpenOrienteering

#endif
