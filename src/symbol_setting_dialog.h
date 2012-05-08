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

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QLabel;
class QToolButton;
class QTabWidget;

class Map;
class MapView;
class Symbol;
class MainWindow;
class PointSymbol;
class PointSymbolEditorWidget;
class SymbolPropertiesWidget;
class MapEditorController;
class Object;

/** A dialog for editing symbol properties 
 */
class SymbolSettingDialog : public QDialog
{
Q_OBJECT
public:
	/** Construct a new dialog for a given symbol and map. */
	SymbolSettingDialog(Symbol* map_symbol, Map* map, QWidget* parent);
	
	/** Destruct the dialog and cleanup temporary objects. */
	virtual ~SymbolSettingDialog();
	
	/** Get the edited object. The caller can duplicate it for later user. */
	inline const Symbol* getEditedSymbol() const { return symbol; }
	
	inline const Symbol* getSourceSymbol() const { return source_symbol; }
	
	inline Map* getSourceMap() { return source_map; }
	
	inline Map* getPreviewMap() { return preview_map; }
	
	inline MapEditorController* getPreviewController() { return preview_controller; }
	
public slots:
	void generalModified();
	
	/** Update the symbol preview */
	void updatePreview();
	
protected slots:
	void loadTemplateClicked();
	void centerTemplateBBox();
	void centerTemplateGravity();
	
private:
	void createPreviewMap();
	
	void updateOkButton();
	void updateSymbolLabel();
	
	Map* source_map;
	Symbol* source_symbol;

	Symbol* symbol;
	Map* preview_map;
	MainWindow* preview_widget;
	MapView* preview_map_view;
	MapEditorController* preview_controller;
	std::vector<Object*> preview_objects;
	
	QLabel* template_file_label;
	QToolButton* center_template_button;
	
	QPushButton* ok_button;
	
	QLabel* symbol_icon_label;
	QLabel* symbol_text_label;
	
	SymbolPropertiesWidget* properties_widget;
};

#endif
