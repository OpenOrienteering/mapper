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


#ifndef _OPENORIENTEERING_SYMBOL_SETTING_DIALOG_H_
#define _OPENORIENTEERING_SYMBOL_SETTING_DIALOG_H_

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QCheckBox;
class QLabel;
class QToolButton;
QT_END_NAMESPACE

class Map;
class MapView;
class Symbol;
class MainWindow;
class PointSymbol;
class PointSymbolEditorWidget;
class MapEditorController;
class Object;

class SymbolSettingDialog : public QDialog
{
Q_OBJECT
public:
	SymbolSettingDialog(Symbol* symbol, Symbol* in_map_symbol, Map* map, QWidget* parent);
    virtual ~SymbolSettingDialog();
	
	void updatePreview();
	
protected slots:
	void createPreviewMap();
	
	void numberChanged(QString text);
	void nameChanged(QString text);
	void descriptionChanged();
	void helperSymbolClicked(bool checked);
	
	void loadTemplateClicked();
	void centerTemplateBBox();
	void centerTemplateGravity();
	
	void okClicked();
	
private:
	PointSymbolEditorWidget* createPointSymbolEditor(MapEditorController* controller);
	
	void updateNumberEdits();
	void updateOkButton();
	void updateWindowTitle();
	
	Symbol* symbol;
	
	QLineEdit** number_edit;
	QLineEdit* name_edit;
	QTextEdit* description_edit;
	QCheckBox* helper_symbol_check;
	
	MainWindow* preview_widget;
	Map* preview_map;
	MapView* preview_map_view;
	std::vector<Object*> preview_objects;
	
	QLabel* template_file_label;
	QToolButton* center_template_button;
	
	QPushButton* ok_button;
};

#endif
