/*
 *    Copyright 2014 Thomas Schöps
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

#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QToolButton;
QT_END_NAMESPACE
class MapEditorController;
class MapEditorTool;
class MapWidget;


/** Shows a set of buttons for simulating key presses.
 *  This is used to simulate keys in Mapper's mobile GUI. */
class KeyButtonBar : public QWidget
{
Q_OBJECT
public:
	KeyButtonBar(MapEditorTool* tool, MapWidget* map_widget, QWidget* parent = 0);
	
	/** Adds a non-checkable button. */
	void addPressKey(int key_code, QString text, QIcon icon = QIcon());
	
	/** Adds a non-checkable button for which in addition a modifier key is pressed while the key is pressed. */
	void addPressKeyWithModifier(int key_code, int modifier_code, QString text, QIcon icon = QIcon());
	
	/** Adds a checkable button which may add a modifier code while checked. */
	void addModifierKey(int key_code, int modifier_code, QString text, QIcon icon = QIcon());
	
	/** Returns the active modifier flags. Is intended to be combined with the modifier flags returned by mouse events,
	 *  because the key simulation cannot insert modifiers there. */
	int activeModifiers() const;
	
public slots:
	void buttonClicked(bool checked);
	
private:
	struct ButtonInfo
	{
		int key_code;
		int modifier_code;
		bool is_checkable;
	};
	
	QToolButton* createButton(int key_code, QString text, QIcon icon, bool is_checkable, int modifier_code);
	void sendKeyPressEvent(int key_code, int modifier_code);
	void sendKeyReleaseEvent(int key_code, int modifier_code);
	
	int active_modifiers;
	QHash<QToolButton*, ButtonInfo> button_info;
	QHBoxLayout* layout;
	MapWidget* map_widget;
	MapEditorTool* tool;
};
