/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_UTIL_TASK_DIALOG_H_
#define _OPENORIENTEERING_UTIL_TASK_DIALOG_H_

#include <QDialog>
#include <QDialogButtonBox>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QCommandLinkButton;
class QSignalMapper;
QT_END_NAMESPACE

class TaskDialog : public QDialog
{
Q_OBJECT
public:
	TaskDialog(QWidget* parent, const QString& title, const QString& text,
			   QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::NoButton);
	
	QCommandLinkButton* addCommandButton(const QString& text, const QString& description);
	
	inline QAbstractButton* clickedButton() const {return clicked_button;}
	inline QPushButton* button(QDialogButtonBox::StandardButton which) const {return button_box->button(which);}
	
private slots:
	void buttonClicked(QWidget* button);
	void buttonClicked(QAbstractButton* button);
	
private:
	QAbstractButton* clicked_button;
	QVBoxLayout* layout;
	QDialogButtonBox* button_box;
	QSignalMapper* signal_mapper;
};

#endif
