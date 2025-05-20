/*
 *    Copyright 2025 Matthias Kühlewein
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

#ifndef OPENORIENTEERING_TAG_REMOVE_DIALOG_H
#define OPENORIENTEERING_TAG_REMOVE_DIALOG_H

#include <QDialog>
#include <QObject>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QString;
class QWidget;


namespace OpenOrienteering {

class Map;

class TagRemoveDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Creates a new TagRemoveDialog object.
	 */
	TagRemoveDialog(QWidget* parent, Map* map);
	
	~TagRemoveDialog() override;
	
private slots:
	void textChanged(const QString& text);
	void okClicked();
	
private:
	QPushButton* ok_button;
	QCheckBox* undo_check;
	QComboBox* compare_op;
	QLineEdit* pattern_edit;
	
	Map *map;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_TAG_REMOVE_DIALOG_H
