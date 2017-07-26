/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_DIALOG_REOPEN_H
#define OPENORIENTEERING_TEMPLATE_DIALOG_REOPEN_H

#include <QDialog>
#include <QListWidget>
#include <QObject>
#include <QString>

class QAbstractButton;
class QDropEvent;
class QPushButton;
class QWidget;

class Map;


/**
 * Dialog showing a list of closed templates.
 * 
 * Offers to reopen those templates by dragging them into the list of open
 * templates shown next to them.
 */
class ReopenTemplateDialog : public QDialog
{
Q_OBJECT
public:
	ReopenTemplateDialog(QWidget* parent, Map* map, const QString& map_directory);
	
private slots:
	void updateClosedTemplateList();
	void clearClicked();
	void doAccept(QAbstractButton* button);
	
private:
	class OpenTemplateList : public QListWidget
	{
	public:
		OpenTemplateList(ReopenTemplateDialog* dialog);
        virtual void dropEvent(QDropEvent* event);
	private:
		ReopenTemplateDialog* dialog;
	};
	
	QListWidget* closed_template_list;
	OpenTemplateList* open_template_list;
	QPushButton* clear_button;
	
	Map* map;
	QString map_directory;
};

#endif
