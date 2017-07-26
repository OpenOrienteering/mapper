/*
 *    Copyright 2017 Kai Pastor
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

#ifndef OPENORIENTEERING_MAP_FIND_FEATURE_H
#define OPENORIENTEERING_MAP_FIND_FEATURE_H

#include <memory>

#include <QObject>

class QAction;
class QDialog;
class QTextEdit;

class MainWindow;
class MapEditorController;

/**
 * Provides an interactive feature for finding objects in the map.
 * 
 * The search text from the provided dialog is trimmed and parsed as an 
 * ObjectQuery. If parsing fails, the trimmed text is taken as a ObjectQuery
 * search string. However, the search does not only use ObjectQuery but also
 * tries to match the text content of text objects.
 */
class MapFindFeature : public QObject
{
	Q_OBJECT
	
public:
	MapFindFeature(MainWindow& window, MapEditorController& controller);
	
	~MapFindFeature() override;
	
	void setEnabled(bool enabled);
	
	QAction* showAction();
	
	QAction* findNextAction();
	
private:
	void showDialog();
	
	void findNext();
	
	void findAll();
	
	void showHelp();
	
	MainWindow &window;
	MapEditorController& controller;
	std::unique_ptr<QAction> show_action;
	std::unique_ptr<QAction> find_next_action;
	std::unique_ptr<QDialog> find_dialog;
	QTextEdit* text_edit;
	
};

#endif
