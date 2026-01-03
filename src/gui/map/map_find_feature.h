/*
 *    Copyright 2017-2019, 2025 Kai Pastor
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

#ifndef OPENORIENTEERING_MAP_FIND_FEATURE_H
#define OPENORIENTEERING_MAP_FIND_FEATURE_H

#include <QtGlobal>
#include <QObject>
#include <QPointer>
#include <QTextEdit>

class QAction;
class QCheckBox;
class QContextMenuEvent;
class QDialog;
class QKeyEvent;
class QLabel;
class QPoint;
class QPushButton;
class QStackedLayout;
class QWidget;

namespace OpenOrienteering {

class MapEditorController;
class Object;
class ObjectQuery;
class TagSelectWidget;

/**
 * The context menu (right click or Ctrl+K) is extended by the possibility
 * to select and insert one of the keywords (e.g., SYMBOL, AND...)
 */
class MapFindTextEdit : public QTextEdit
{
	Q_OBJECT
	
private:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void showCustomContextMenu(const QPoint& globalPos);
	
private slots:
	void insertKeyword(QAction* action);
};


/**
 * Provides an interactive feature for finding objects in the map.
 * 
 * The search text from the provided dialog is trimmed and parsed as an 
 * ObjectQuery. If parsing fails, the trimmed text is used to find any matching
 * text in tag keys, tag values, text object content or symbol name.
 */
class MapFindFeature : public QObject
{
	Q_OBJECT
public:
	MapFindFeature(MapEditorController& controller);
	
	~MapFindFeature() override;
	
	void setEnabled(bool enabled);
	
	QAction* showDialogAction() { return show_action; }
	
	QAction* findNextAction() { return find_next_action; }
	
	static void findNextMatchingObject(MapEditorController& controller, const ObjectQuery& query, bool center_selection_visibility = false);
	
	static void findAllMatchingObjects(MapEditorController& controller, const ObjectQuery& query, bool center_selection_visibility = false);
	
private:
	void showDialog();
	
	ObjectQuery makeQuery() const;
	
	void findNext();
	
	void deleteAndFindNext();
	
	void findAll();
	
	void objectSelectionChanged();
	
	void centerView();
	
	void showHelp() const;
	
	void tagSelectorToggled(bool active);
	
	
	MapEditorController& controller;
	QPointer<QDialog> find_dialog;           // child of controller's window
	QStackedLayout* editor_stack = nullptr;  // child of find_dialog
	MapFindTextEdit* text_edit = nullptr;    // child of find_dialog
	TagSelectWidget* tag_selector = nullptr; // child of find_dialog
	QWidget* tag_selector_buttons = nullptr; // child of find_dialog
	QPushButton* delete_find_next = nullptr; // child of find_dialog
	QCheckBox* center_view = nullptr;        // child of find_dialog
	QLabel* selected_objects = nullptr;      // child of find_dialog
	QAction* show_action = nullptr;          // child of this
	QAction* find_next_action = nullptr;     // child of this
	
	Object* previous_object = nullptr;
	
	Q_DISABLE_COPY(MapFindFeature)
};

}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_MAP_FIND_FEATURE_H
