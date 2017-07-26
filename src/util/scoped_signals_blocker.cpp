/*
 *    Copyright 2012-2015 Kai Pastor
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


#include "scoped_signals_blocker.h"

#include <QtGlobal>
#include <QObject>


ScopedMultiSignalsBlocker::~ScopedMultiSignalsBlocker()
{
#ifdef MAPPER_DEVELOPMENT_BUILD
	// We reserve only 10 items on the stack.
	if (items.size() > 10)
		qWarning("More than 10 items in a ScopedMultiSignalsBlocker");
#endif
	while (!items.empty())
	{
		auto item = items.back();
		if (item.object)
			item.object->blockSignals(item.old_state);
		items.pop_back();
	}
}

void ScopedMultiSignalsBlocker::add(QObject* object)
{
	items.push_back( {object, object && object->blockSignals(true)} );
	Q_ASSERT(!object || object->signalsBlocked());
}
