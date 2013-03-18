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

#include "modifier_key.h"


const ModifierKey ModifierKey::alt()
{
	static const ModifierKey key(Qt::AltModifier);
	return key;
}

const ModifierKey ModifierKey::control()
{
	static const ModifierKey key(Qt::ControlModifier);
	return key;
}

const ModifierKey ModifierKey::controlShift()
{
	static const ModifierKey key(Qt::ControlModifier | Qt::ShiftModifier);
	return key;
}

const ModifierKey ModifierKey::meta()
{
	static const ModifierKey key(Qt::MetaModifier);
	return key;
}

const ModifierKey ModifierKey::shift()
{
	static const ModifierKey key(Qt::ShiftModifier);
	return key;
}

const ModifierKey ModifierKey::space()
{
	static const ModifierKey key(Qt::Key_Space);
	return key;
}

const ModifierKey ModifierKey::return_key()
{
	static const ModifierKey key(Qt::Key_Return);
	return key;
}

const ModifierKey ModifierKey::backspace()
{
	static const ModifierKey key(Qt::Key_Backspace);
	return key;
}

const ModifierKey ModifierKey::escape()
{
	static const ModifierKey key(Qt::Key_Escape);
	return key;
}

