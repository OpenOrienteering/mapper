/*
 *    Copyright 2024 Matthias Kühlewein
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

#ifndef OPENORIENTEERING_MAP_NOTES_H
#define OPENORIENTEERING_MAP_NOTES_H

#include <QDialog>
#include <QObject>

class QTextEdit;
class QWidget;


namespace OpenOrienteering {

class Map;

class MapNotesDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a new MapNotesDialog object.
	 */
	MapNotesDialog(QWidget* parent, Map& map);
	
	~MapNotesDialog() override;
	
private slots:
	void okClicked();
	
private:
	QTextEdit* text_edit;
	
	Map& map;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAP_NOTES_H
