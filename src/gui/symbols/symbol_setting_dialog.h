/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_SYMBOL_SETTING_DIALOG_H
#define OPENORIENTEERING_SYMBOL_SETTING_DIALOG_H

#include <memory>
#include <vector>

#include <QDialog>
#include <QObject>

class QLabel;
class QPushButton;
class QToolButton;
class QWidget;

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapEditorController;
class MapView;
class Object;
class Symbol;
class SymbolPropertiesWidget;


/** 
 * A dialog for editing symbol properties.
 *
 * The specific symbol properties must be edited in tabs provided by the symbol. 
 */
class SymbolSettingDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Constructs a new dialog for a given symbol and map. 
	 */
	SymbolSettingDialog(const Symbol* source_symbol, Map* source_map, QWidget* parent);
	
	/**
	 * Destructs the dialog and cleans up temporary objects. 
	 */
	~SymbolSettingDialog() override;
	
	/**
	 * Returns a copy of the currently edited symbol. 
	 */
	std::unique_ptr<Symbol> getNewSymbol() const;
	
	/**
	 * Returns a pointer to the unmodified symbol which is currently edited.
	 * Use getUnmodifiedSymbolCopy() instead if you need to access the object!
	 */
	inline const Symbol* getUnmodifiedSymbol() const { return source_symbol; }
	
	/**
	 * Returns a pointer to a copy of the unmodified symbol which is currently edited.
	 */
	inline const Symbol* getUnmodifiedSymbolCopy() const { return &*source_symbol_copy; }
	
	/**
	 * Returns true if the edited symbol has modifications.
	 */
	inline bool isSymbolModified() const { return symbol_modified; }
	
	/**
	 * Returns the Map of the unmodified symbol.
	 */
	inline Map* getSourceMap() { return source_map; }
	
	/**
	 * Returns the preview map of this dialog.
	 */
	inline Map* getPreviewMap() { return preview_map; }
	
	/**
	 * Return the MapEditorController which handles this dialog's preview map.
	 */
	inline MapEditorController* getPreviewController() { return preview_controller; }
	
public slots:
	/**
	 * Shows this dialog's help page.
	 */
	void showHelp();
	
	/**
	 * Resets the edited symbol to the state of the unmodified symbol from the map.
	 */
	void reset();
	
	/**
	 * Sets the modification status of the dialog, and triggers a screen update.
	 */
	void setSymbolModified(bool modified = true);
	
	/** 
	 * Updates the label which is the headline to the dialog.
	 */
	void updateSymbolLabel();
	
	/** 
	 * Updates the preview from the current symbol settings.
	 */
	void updatePreview();
	
	/**
	 * Updates the state (enabled/disabled) of the dialog buttons.
	 */
	void updateButtons();
	
protected slots:
	/**
	 * Opens a dialog for loading a template to the preview map.
	 */
	void loadTemplateClicked();
	
	/**
	 * Centers the preview map template relative to its bounding box.
	 */
	void centerTemplateBBox();
	
	/**
	 * Centers the preview map template relative to its center of gravity,
	 * leaving out a selected background color.
	 */
	void centerTemplateGravity();
	
protected:
	/**
	 * Populates the preview map for the symbol.
	 */
	void createPreviewMap();
	
private:
	Map* source_map;
	const Symbol* source_symbol;
	std::unique_ptr<const Symbol> source_symbol_copy;
	
	// properties_widget must be deleted before symbol.
	std::unique_ptr<Symbol> symbol;
	std::unique_ptr<SymbolPropertiesWidget> properties_widget;
	
	Map* preview_map;
	MainWindow* preview_widget;
	MapView* preview_map_view;
	MapEditorController* preview_controller;
	std::vector<Object*> preview_objects;
	
	QLabel* template_file_label;
	QToolButton* center_template_button;
	
	QPushButton* ok_button;
	QPushButton* reset_button;
	
	QLabel* symbol_icon_label;
	QLabel* symbol_text_label;
	
	bool symbol_modified;
};


}  // namespace OpenOrienteering

#endif
