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


#ifndef OPENORIENTEERING_SEGMENTED_BUTTON_LAYOUT_H
#define OPENORIENTEERING_SEGMENTED_BUTTON_LAYOUT_H

#include <QtGlobal>
#include <QObject>
#include <QHBoxLayout>

class QWidget;

namespace OpenOrienteering {


/**
 * SegmentedButtonLayout is a horizontal box layout with no margin and no
 * spacing which will mark the contained widgets as being segments having a
 * left and/or right neighbor.
 * 
 * MapperProxyStyle uses this information to make buttons from a single
 * SegmentedButtonLayout appear as a single segmented button.
 */
class SegmentedButtonLayout : public QHBoxLayout
{
Q_OBJECT
public:
	/**
	 * Constructs a new SegmentedButtonLayout.
	 */
	SegmentedButtonLayout();
	
	/**
	 * Constructs a new SegmentedButtonLayout for the given parent.
	 */
	explicit SegmentedButtonLayout(QWidget* parent);
	
	/**
	 * Destroys the object.
	 */
	~SegmentedButtonLayout() override;
	
	/**
	 * Resets the information about neighboring segments and any other cached
	 * information about the layout.
	 */
	void invalidate() override;
	
	/**
	 * Types of segment neighborhood.
	 */
	enum Segment
	{
		NoNeighbors   = 0x00,
		RightNeighbor = 0x01,
		LeftNeighbor  = 0x02,
		BothNeighbors = RightNeighbor | LeftNeighbor
	};
	
private:
	Q_DISABLE_COPY(SegmentedButtonLayout)
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SEGMENTED_BUTTON_LAYOUT_H
