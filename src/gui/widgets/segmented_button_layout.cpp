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

#include "segmented_button_layout.h"

#include <QLayoutItem>
#include <QVariant>  // IWYU pragma: keep
#include <QWidget>


SegmentedButtonLayout::SegmentedButtonLayout()
 : QHBoxLayout()
{
	setContentsMargins(0, 0, 0, 0);
	setSpacing(0);
}

SegmentedButtonLayout::SegmentedButtonLayout(QWidget* parent)
 : QHBoxLayout(parent)
{
	setContentsMargins(0, 0, 0, 0);
	setSpacing(0);
}

SegmentedButtonLayout::~SegmentedButtonLayout()
{
	; // Nothing, not inlined
}

void SegmentedButtonLayout::invalidate()
{
	int num_items = count();
	QWidget* first_widget = nullptr;
	QWidget* widget = nullptr;
	for (int i = 0; i < num_items; ++i)
	{
		widget = itemAt(i)->widget();
		if (!widget)
			continue;
		
// 		widget->setStyle(segmented_style);
		
		if (!first_widget)
		{
			first_widget = widget;
			widget->setProperty("segment", RightNeighbor);
		}
		else
		{
			widget->setProperty("segment", BothNeighbors);
		}
	}
	
	if (widget)
		widget->setProperty("segment", (widget == first_widget) ? NoNeighbors : LeftNeighbor);
	
	QHBoxLayout::invalidate();
}
