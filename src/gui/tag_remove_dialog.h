/*
 *    Copyright 2026 Matthias Kühlewein
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

#include <set>
#include <utility>

#include <QDialog>
#include <QObject>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
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
	
	static int findMatchingTags(const Map *map, const QString& pattern, int op, std::set<QString>& matching_keys);
	
	static void removeMatchingTags(Map *map, const QString& pattern, int op, bool add_undo);
	
private slots:
	void textChanged(const QString& text);
	void comboBoxChanged();
	void findClicked();
	void removeClicked();
	
private:
	void reset();
	std::pair<QString, int> getPatternAndOperation() const;
	
	QPushButton* find_button;
	QPushButton* remove_button;
	QCheckBox* undo_check;
	QComboBox* compare_op;
	QLabel* number_matching_objects;
	QLabel* number_matching_keys;
	QLineEdit* pattern_edit;
	QComboBox* pattern_select;
	QStackedWidget* pattern_choice;
	QPlainTextEdit* matching_keys_details;
	
	Map *map;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_TAG_REMOVE_DIALOG_H
