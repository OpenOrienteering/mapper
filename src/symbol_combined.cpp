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


#include "symbol_combined.h"

#include <QtGui>
#include <QFile>

#include "symbol_setting_dialog.h"

CombinedSymbol::CombinedSymbol() : Symbol(Symbol::Combined)
{
	parts.resize(2);
	parts[0] = NULL;
	parts[1] = NULL;
}
CombinedSymbol::~CombinedSymbol()
{
}
Symbol* CombinedSymbol::duplicate()
{
	CombinedSymbol* new_symbol = new CombinedSymbol();
	new_symbol->duplicateImplCommon(this);
	new_symbol->parts = parts;
	return new_symbol;
}

void CombinedSymbol::createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output)
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i])
			parts[i]->createRenderables(object, flags, coords, path_closed, output);
	}
}
void CombinedSymbol::colorDeleted(Map* map, int pos, MapColor* color)
{
	// Do nothing. The parts are assumed to be ordinary symbols, so they will get this call independently
}
bool CombinedSymbol::containsColor(MapColor* color)
{
	// Do nothing. The parts are assumed to be ordinary symbols, so they will get this call independently
	return false;
}
bool CombinedSymbol::symbolChanged(Symbol* old_symbol, Symbol* new_symbol)
{
	bool have_symbol = false;
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i] == old_symbol)
		{
			have_symbol = true;
			parts[i] = new_symbol;
		}
	}
	return have_symbol;
}
bool CombinedSymbol::containsSymbol(Symbol* symbol)
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i] == symbol)
			return true;
		if (parts[i]->getType() == Symbol::Combined)	// TODO: see TODO in SymbolDropDown constructor.
		{
			CombinedSymbol* combined_symbol = reinterpret_cast<CombinedSymbol*>(parts[i]);
			if (combined_symbol->containsSymbol(symbol))
				return true;
		}
	}
	return false;
}
void CombinedSymbol::scale(double factor)
{
	// Do nothing. The parts are assumed to be ordinary symbols, so they will get this call independently
}
Symbol::Type CombinedSymbol::getContainedTypes()
{
	int type = (int)getType();
	
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
		type |= parts[i]->getContainedTypes();
	
	return (Type)type;
}

void CombinedSymbol::saveImpl(QFile* file, Map* map)
{
	int size = (int)parts.size();
	file->write((const char*)&size, sizeof(int));
	
	for (int i = 0; i < size; ++i)
	{
		int temp = (parts[i] == NULL) ? -1 : map->findSymbolIndex(parts[i]);
		file->write((const char*)&temp, sizeof(int));
	}
}
bool CombinedSymbol::loadImpl(QFile* file, int version, Map* map)
{
	int size;
	file->read((char*)&size, sizeof(int));
	parts.resize(size);
	
	for (int i = 0; i < size; ++i)
	{
		int temp;
		file->read((char*)&temp, sizeof(int));
		parts[i] = reinterpret_cast<Symbol*>(temp);
	}
	return true;
}
bool CombinedSymbol::loadFinished(Map* map)
{
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
		parts[i] = map->getSymbol(reinterpret_cast<int>(parts[i]));
	return true;
}

// ### CombinedSymbolSettings ###

const int CombinedSymbolSettings::max_count = 5;

CombinedSymbolSettings::CombinedSymbolSettings(CombinedSymbol* symbol, CombinedSymbol* in_map_symbol, Map* map, SymbolSettingDialog* parent): QGroupBox(tr("Combination settings"), parent), symbol(symbol), dialog(parent)
{
	react_to_changes = true;
	QGridLayout* layout = new QGridLayout();
	
	QLabel* number_label = new QLabel(tr("Symbol count:"));
	number_edit = new QComboBox();
	for (int i = 2; i <= max_count; ++i)
		number_edit->addItem(QString::number(i), QVariant(i));
	number_edit->setCurrentIndex(number_edit->findData(symbol->parts.size()));
	
	layout->addWidget(number_label, 0, 0);
	layout->addWidget(number_edit, 0, 1);
	connect(number_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(numberChanged(int)));
	
	symbol_labels = new QLabel*[max_count];
	symbol_edits = new SymbolDropDown*[max_count];
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i] = new QLabel(tr("Symbol %1:").arg(i+1));
		symbol_edits[i] = new SymbolDropDown(map, Symbol::Line | Symbol::Area | Symbol::Combined, ((int)symbol->parts.size() > i) ? symbol->parts[i] : NULL, in_map_symbol);
		
		layout->addWidget(symbol_labels[i], i+1, 0);
		layout->addWidget(symbol_edits[i], i+1, 1);
		connect(symbol_edits[i], SIGNAL(currentIndexChanged(int)), this, SLOT(symbolChanged(int)));
		
		if (i >= (int)symbol->parts.size())
		{
			symbol_labels[i]->hide();
			symbol_edits[i]->hide();
		}
	}
	
	setLayout(layout);
}
CombinedSymbolSettings::~CombinedSymbolSettings()
{
	delete[] symbol_labels;
	delete[] symbol_edits;
}

void CombinedSymbolSettings::numberChanged(int index)
{
	int old_num_items = (int)symbol->parts.size();
	int num_items = number_edit->itemData(index).toInt();
	symbol->parts.resize(num_items);
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i]->setVisible(i < num_items);
		symbol_edits[i]->setVisible(i < num_items);
		
		if (i >= old_num_items && i < num_items)
		{
			// This item appears now
			symbol->parts[i] = NULL;
			react_to_changes = false;
			symbol_edits[i]->setSymbol(NULL);
			react_to_changes = true;
		}
	}
	
	dialog->updatePreview();
}
void CombinedSymbolSettings::symbolChanged(int index)
{
	if (!react_to_changes)
		return;
	for (int i = 0; i < (int)symbol->parts.size(); ++i)
		symbol->parts[i] = symbol_edits[i]->symbol();
	dialog->updatePreview();
}

#include "symbol_combined.moc"
