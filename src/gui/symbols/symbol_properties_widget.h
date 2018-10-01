/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2014, 2016, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_PROPERTIES_WIDGET_H
#define OPENORIENTEERING_SYMBOL_PROPERTIES_WIDGET_H


#include <vector>

#include <QObject>
#include <QString>
#include <QTabWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QWidget;

namespace OpenOrienteering {

class IconPropertiesWidget;
class Symbol;
class SymbolSettingDialog;


/**
 *  A widget for editing groups of properties of a symbol.
 */
class SymbolPropertiesWidget : public QTabWidget
{
Q_OBJECT
public:
	/** Construct a new widget containing only a general properties group. 
	  * @param symbol the symbol to be customized 
	  */
	SymbolPropertiesWidget(Symbol* symbol, SymbolSettingDialog* dialog);
	
	~SymbolPropertiesWidget() override;
	
	/** Add a widget as a named group of properties */
	void addPropertiesGroup(const QString& name, QWidget* widget);
	
	void insertPropertiesGroup(int index, const QString& name, QWidget* widget);
	
	void removePropertiesGroup(int index);
	
	void removePropertiesGroup(const QString& name);
	
	void renamePropertiesGroup(int index, const QString& new_name);
	
	void renamePropertiesGroup(const QString& old_name, const QString& new_name);
	
	int indexOfPropertiesGroup(const QString& name) const;
	
	/** Get the edited symbol */
	inline Symbol* getSymbol() { return symbol; }
	
	QString getHelpSection() const { return QString(); }
	
	/**
	 * Changes the edited symbol and resets the input values.
	 * When overriding this method, make sure to call SymbolPropertiesWidget::reset().
	 */
	virtual void reset(Symbol* symbol);
	
signals:
	void propertiesModified();
	
protected slots:
	void numberChanged(const QString& text);
	void languageChanged();
	void editClicked();
	void nameChanged(const QString& text);
	void descriptionChanged();
	void helperSymbolChanged(bool checked);
	
protected:
	void updateTextEdits();
	
	Symbol* symbol;
	SymbolSettingDialog* const dialog;
	
	std::vector<QLineEdit*> number_editors;
	QComboBox* language_combo;
	QPushButton* edit_button;
	QLineEdit* name_edit;
	QTextEdit* description_edit;
	QCheckBox* helper_symbol_check;
	
	IconPropertiesWidget* icon_widget;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SYMBOL_PROPERTIES_WIDGET_H
