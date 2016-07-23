/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2016 Kai Pastor
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


#ifndef OPENORIENTEERING_TEXT_ALIGNMENT_WIDGET_H
#define OPENORIENTEERING_TEXT_ALIGNMENT_WIDGET_H

#include <QDockWidget>


QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

class TextObject;
class TextObjectEditorHelper;


/**
 * A widget for setting text alignment.
 * 
 * This widget is shown while the TextObjectEditorHelper is active.
 * 
 * \todo Use QDockWidget (if necessary at all), don't inherit from it.
 */
class TextObjectAlignmentDockWidget : public QDockWidget
{
Q_OBJECT
public:
	TextObjectAlignmentDockWidget(TextObject* object, int horz_default, int vert_default, TextObjectEditorHelper* text_editor, QWidget* parent);
	virtual QSize sizeHint() const {return QSize(10, 10);}
	
	virtual bool event(QEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	
signals:
	void alignmentChanged(int horz, int vert);
	
public slots:
	void horzClicked(int index);
	void vertClicked(int index);
	
private:
	void addHorzButton(int index, const QString& icon_path, int horz_default);
	void addVertButton(int index, const QString& icon_path, int vert_default);
	void emitAlignmentChanged();
	
	QPushButton* horz_buttons[3];
	QPushButton* vert_buttons[4];
	int horz_index;
	int vert_index;
	
	TextObject* object;
	TextObjectEditorHelper* text_editor;
};

#endif
