/*
 *    Copyright 2012 Kai Pastor
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


#ifndef _UTIL_SCOPED_SIGNALS_BLOCKER_H_
#define _UTIL_SCOPED_SIGNALS_BLOCKER_H_

#include <utility>
#include <vector>

#include <QObject>

/**
 * A ScopedSignalsBlocker disables the signals of a QObject for the scope
 * of a particular block.
 * 
 * It sets a QObject's signalsBlocked property to true during construction, 
 * and resets its to its previous state during destruction.
 * 
 * Synopsis:
 * 
 *     {
 *         ScopedSignalsBlocker block(my_widget);
 *         my_widget->setSomeProperty("Hello World");
 *     }
 */
class ScopedSignalsBlocker
{
public:
	inline ScopedSignalsBlocker(QObject* obj)
	: obj(obj), prev_state(obj->signalsBlocked())
	{
		obj->blockSignals(true);
	}
	
	inline ~ScopedSignalsBlocker()
	{
		obj->blockSignals(prev_state);
	}
	
private:
	QObject* obj;
	bool prev_state;
};

/**
 * A ScopedMultiSignalsBlocker allows to disable the signals of multiple
 * QObjects for the scope of a particular block.
 * 
 * It sets the QObject's signalsBlocked property to true when they are added
 * (by operator <<), and resets its to its previous state during destruction.
 * 
 * Synopsis:
 * 
 *     {
 *         ScopedMultiSignalsBlocker block;
 *         block << my_widget << my_other_widget;
 *         my_widget->setSomeProperty("Hello World");
 *     }
 */
class ScopedMultiSignalsBlocker
{
private:
	typedef std::pair<QObject*,bool> item_type;
	
public:
	inline ScopedMultiSignalsBlocker()
	{ }
	
	inline ScopedMultiSignalsBlocker& operator<<(QObject* obj)
	{
		objects.push_back(std::make_pair(obj, obj->signalsBlocked()));
		obj->blockSignals(true);
		return *this;
	}
	
	inline ~ScopedMultiSignalsBlocker()
	{
		Q_FOREACH(item_type item, objects)
			item.first->blockSignals(item.second);
	}
	
private:
	std::vector<item_type> objects;
};

#endif