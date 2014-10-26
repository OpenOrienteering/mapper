/*
 *    Copyright 2012, 2013, 2014 Kai Pastor
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
 * @brief A safe and scoped wrapper around QObject::blockSignals().
 * 
 * A ScopedSignalsBlocker disables the signals of a QObject for the scope of a
 * particular block. It sets a QObject's signalsBlocked property to true during
 * construction, and resets its to its previous state during destruction.
 * 
 * Synopsis:
 * 
 *     {
 *         const ScopedSignalsBlocker block(my_widget);
 *         my_widget->setSomeProperty("Hello World");
 *     }
 * 
 * Since Qt 5.3, a better implementation is available from Qt as QSignalBlocker.
 */
class ScopedSignalsBlocker
{
public:
	inline explicit ScopedSignalsBlocker(QObject* obj)
	: obj(obj)
	, prev_state(obj && obj->blockSignals(true))
	{
		Q_ASSERT(!obj || obj->signalsBlocked());
	}
	
	inline ~ScopedSignalsBlocker()
	{
		if (obj)
			obj->blockSignals(prev_state);
	}
	
private:
	Q_DISABLE_COPY(ScopedSignalsBlocker)
	QObject* obj;
	const bool prev_state;
};

#if QT_VERSION < 0x050300

// We export our class under the name of the class introduced in Qt 5.3.
// Note: Our class does not support the full API of QSignalBlocker.
// However, this is normal for lower API versions anyway.
typedef ScopedSignalsBlocker QSignalBlocker;

#endif

/**
 * @brief A safe and scoped wrapper around QObject::blockSignals() of multiple objects.
 * 
 * A ScopedMultiSignalsBlocker allows to disable the signals of multiple
 * QObjects for the scope of a particular block. 
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
		objects.push_back(std::make_pair(obj, obj && obj->blockSignals(true)));
		Q_ASSERT(!obj || obj->signalsBlocked());
		return *this;
	}
	
	inline ~ScopedMultiSignalsBlocker()
	{
		for (std::vector<item_type>::iterator item = objects.begin(); item != objects.end(); ++item)
			if (item->first)
				item->first->blockSignals(item->second);
	}
	
private:
	Q_DISABLE_COPY(ScopedMultiSignalsBlocker)
	std::vector<item_type> objects;
};

#endif
