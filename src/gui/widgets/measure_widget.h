/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2015, 2024 Kai Pastor
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


#ifndef OPENORIENTEERING_MEASURE_WIDGET_H
#define OPENORIENTEERING_MEASURE_WIDGET_H

#include <QObject>
#include <QString>
#include <QTextBrowser>

class QWidget;

namespace OpenOrienteering {

class Map;


/**
 * The widget which is shown in a dock widget when the measure tool is active.
 * Displays information about the currently selected objects.
 */
class MeasureWidget : public QTextBrowser
{
Q_OBJECT
public:
	/** Creates a new MeasureWidget for a given map. */
	MeasureWidget(Map* map, QWidget* parent = nullptr);

	/** Destroys the MeasureWidget. */
	~MeasureWidget() override;
	
protected slots:
	/**
	 * Is called when the object selection in the map changes.
	 * Updates the widget content.
	 */
	void objectSelectionChanged();
	
private:
	Map* map;
	QString warning_icon;
};


}  // namespace OpenOrienteering

#endif
