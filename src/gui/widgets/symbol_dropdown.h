/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012, 2013, 2014, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_DROPDOWN_H
#define OPENORIENTEERING_SYMBOL_DROPDOWN_H

#include <QComboBox>
#include <QItemDelegate>
#include <QObject>
#include <QString>
// IWYU pragma: no_include <QStyleOptionViewItem>

class QAbstractItemModel;
class QModelIndex;
class QStyleOptionViewItem;
class QWidget;

namespace OpenOrienteering {

class Map;
class Symbol;


/**
 * Drop down combobox for selecting a symbol.
 */
class SymbolDropDown : public QComboBox
{
Q_OBJECT
public:
	/**
	 * Creates an empty SymbolDropDown.
	 */
	SymbolDropDown(QWidget* parent = nullptr);
	
	/**
	 * Creates a SymbolDropDown.
	 * @param map Map in which to choose a symbol.
	 * @param filter Bitwise-or combination of the allowed Symbol::Type types.
	 * @param initial_symbol Initial choice or nullptr for "- none -".
	 * @param excluded_symbol Symbol to exclude from the list or nullptr.
	 * @param parent QWidget parent.
	 */
	SymbolDropDown(const Map* map, int filter, const Symbol* initial_symbol = nullptr, const Symbol* excluded_symbol = nullptr, QWidget* parent = nullptr);
	
	~SymbolDropDown() override;
	
	
	/**
	 * Initializes an empty SymbolDropDown.
	 * @param map Map in which to choose a symbol.
	 * @param filter Bitwise-or combination of the allowed Symbol::Type types.
	 * @param initial_symbol Initial choice or nullptr for "- none -".
	 * @param excluded_symbol Symbol to exclude from the list or nullptr.
	 */
	void init(const Map* map, int filter, const Symbol* initial_symbol = nullptr, const Symbol* excluded_symbol = nullptr);
	
	
	/** Returns the selected symbol or nullptr if no symbol selected */
	const Symbol* symbol() const;
	
	/** Sets the selection to the given symbol  */
	void setSymbol(const Symbol* symbol);
	
	/** Adds a custom text item below the topmost "- none -" which
	 *  can be identified by the given id. */
	void addCustomItem(const QString& text, int id);
	
	/** Returns the id of the current item if it is a custom item,
	 *  or -1 otherwise */
	int customID() const;
	
	/** Sets the selection to the custom item with the given id */
	void setCustomItem(int id);
	
protected slots:
	// TODO: react to changes in the map (not important as long as that cannot
	// happen as long as a SymbolDropDown is shown, which is the case currently)
	
private:
	int num_custom_items = 0;
};



/**
 * Qt item delegate for SymbolDropDown.
 */
class SymbolDropDownDelegate : public QItemDelegate
{
Q_OBJECT
public:
	SymbolDropDownDelegate(int symbol_type_filter, QObject* parent = nullptr);
	~SymbolDropDownDelegate() override;
	
	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	
private slots:
	void emitCommitData();
	
private:
	const int symbol_type_filter;
};


}  // namespace OpenOrienteering

#endif
