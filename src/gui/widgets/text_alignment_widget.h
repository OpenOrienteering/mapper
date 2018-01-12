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
#include <QObject>
#include <QString>

class QEvent;
class QKeyEvent;
class QPushButton;
class QWidget;

namespace OpenOrienteering {

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
	explicit TextObjectAlignmentDockWidget(TextObjectEditorHelper* text_editor, QWidget* parent = nullptr);
	~TextObjectAlignmentDockWidget() override;
	
signals:
	void alignmentChanged(int horizontal, int vertical);
	
protected:
	bool event(QEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	
	void horizontalClicked();
	void verticalClicked();
	
	QPushButton* makeButton(const QString& icon_path, const QString& text) const;
	
private:
	TextObjectEditorHelper* text_editor;
	QPushButton* horizontal_buttons[3];
	QPushButton* vertical_buttons[4];
	int horizontal_alignment;
	int vertical_alignment;
};


}  // namespace OpenOrienteering

#endif
