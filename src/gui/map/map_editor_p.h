/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2013, 2014, 2015, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_EDITOR_P_H
#define OPENORIENTEERING_MAP_EDITOR_P_H

#include <QAction>
#include <QDockWidget>

class QEvent;
class QIcon;
class QObject;
class QResizeEvent;
class QSizeGrip; // IWYU pragma: keep
class QString;
class QWidget;

class MapEditorController;
class Template;


/**
 * Custom QDockWidget which unchecks the associated menu action when closed
 * and delivers a notification to its child
 */
class EditorDockWidget : public QDockWidget
{
Q_OBJECT
public:
	EditorDockWidget(const QString& title, QAction* action,
					 MapEditorController* editor, QWidget* parent = nullptr);
	
protected:
	virtual bool event(QEvent* event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	
private:
	QAction* action;
	MapEditorController* editor;
	
#ifdef Q_OS_ANDROID
	QSizeGrip* size_grip;
#endif
};



/**
 * Helper class which disallows deselecting the checkable action by the user
 */
class MapEditorToolAction : public QAction
{
Q_OBJECT
public:
	MapEditorToolAction(const QIcon& icon, const QString& text, QObject* parent);
	
signals:
	void activated();
	
private slots:
	void triggeredImpl(bool checked);
};

#endif
