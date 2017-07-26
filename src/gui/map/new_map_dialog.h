/*
 *    Copyright 2011-2013 Thomas Schöps
 *    Copyright 2012, 2013, 2015, 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_NEW_MAP_DIALOG_H
#define OPENORIENTEERING_NEW_MAP_DIALOG_H

#include <map>

#include <QDialog>
#include <QFileInfoList>
#include <QObject>
#include <QString>

class QCheckBox;
class QComboBox;
class QDir;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QWidget;

/**
 * Dialog for creating a new map.
 * Shows scale and symbol set selection.
 */
class NewMapDialog : public QDialog
{
Q_OBJECT
public:
	/** Constructs a dialog for scale and symbol set of a new map. */
	NewMapDialog(QWidget* parent = nullptr);
	
	~NewMapDialog() override;
	
	/** Get the denominator of the chosen map scale.
	 *  For a scale of 1:10000, this function returns 10000.
	 */
	unsigned int getSelectedScale() const;
	
	/** Get the full path of the selected symbol set.
	 *  Returns an empty string if the map shall be created with an empty symbol set. 
	 */
	QString getSelectedSymbolSetPath() const;
	
public slots:
	/** Updates the list of symbol sets for the chosen map scale [denominator]. */
	void updateSymbolSetList();
	
	/** Accepts a selected symbol set, or triggers the file dialog. */
	void symbolSetDoubleClicked(QListWidgetItem* item);
	
	/** Accepts the selected symbol set, or triggers the file dialog. */
	void createClicked();
	
	/** Hides the dialog and accepts the input. */
	void accept() override;
	
protected:
	struct SymbolSetKeyCompare
	{
		bool operator() (const QString& a, const QString& b) const
		{
			bool ok1, ok2;
			int a_int = a.toInt(&ok1);
			int b_int = b.toInt(&ok2);
			if (ok1)
			{
				if (ok2)
					return a_int < b_int;
				else
					return true;
			}
			else
			{
				if (ok2)
					return false;
				else
					return a.compare(b);
			}
		}
	};
	
	/** A type for mapping map scales to lists of symbol sets. */
	typedef std::map<QString, QFileInfoList, SymbolSetKeyCompare> SymbolSetMap;
	
	/** Loads all available symbol. */
	void loadSymbolSetMap();
	
	/** Adds the symbol sets from a particular base directory. */
	void loadSymbolSetDir(const QDir& symbol_set_dir);
	
	/** Open a dialog for loading a symbol set from a file. */
	void showFileDialog();
	
private:
	/** A mapping from map scales to lists of matching symbol set. */
	SymbolSetMap symbol_set_map;	// scale to vector of symbol set names; TODO: store that globally / dir watcher
	
	/** The map scale input widget. */
	QComboBox* scale_combo;
	
	/** The symbol set selection widget. */
	QListWidget* symbol_set_list;
	
	/** The list item for loading a symbol set from a file. */
	QListWidgetItem* load_from_file;
	
	/** This check box controls whether only matching or all symbol sets are displayed. */
	QCheckBox* symbol_set_matching;
	
	/** The button for accepting the selected map scale and symbol set. */
	QPushButton* create_button;
};

#endif
