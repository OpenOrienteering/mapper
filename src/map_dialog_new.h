/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAP_DIALOG_NEW_H_
#define _OPENORIENTEERING_MAP_DIALOG_NEW_H_

#include <map>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

class NewMapDialog : public QDialog
{
Q_OBJECT
public:
	NewMapDialog(QWidget* parent = NULL);
	
	int getSelectedScale();
	QString getSelectedSymbolSetPath();
	
public slots:
	void createClicked();
    void scaleChanged(QString new_text);
    void symbolSetDoubleClicked(QListWidgetItem* item);
	
private:
	void loadSymbolSetMap();
	
	std::map<int, QStringList> symbol_set_map;	// scale to vector of symbol set names; TODO: store that globally / dir watcher
	
	QComboBox* scale_combo;
	QListWidget* symbol_set_list;
	QPushButton* create_button;
};

#endif
