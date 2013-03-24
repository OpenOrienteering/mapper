/*
 *    Copyright 2012, 2013 Jan Dalheimer
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

#ifndef _OPENORIENTEERING_SETTINGS_DIALOG_H_
#define _OPENORIENTEERING_SETTINGS_DIALOG_H_

#include <QDialog>

class Object;

QT_BEGIN_NAMESPACE
class QTableWidget;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
QT_END_NAMESPACE

class TagsDialog : public QDialog
{
	Q_OBJECT
public:
	TagsDialog(Object* object, QWidget* parent = 0);

private slots:
	void addTagClicked();
	void removeTagClicked();
	void closeClicked();

private:
	QTableWidget* tags_table;
	QPushButton* add_tag_button;
	QPushButton* remove_tag_button;
	QPushButton* close_button;

	QHBoxLayout* buttons_layout;
	QVBoxLayout* layout;

	Object* object;

	bool doesKeyExist(const QString& key);
};

#endif
