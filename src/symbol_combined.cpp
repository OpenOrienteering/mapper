/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIODevice>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalMapper>
#include <QSpinBox>
#include <QXmlStreamWriter>

#include "core/map_color.h"
#include "gui/widgets/symbol_dropdown.h"
#include "map.h"
#include "symbol_setting_dialog.h"
#include "symbol_properties_widget.h"

CombinedSymbol::CombinedSymbol() : Symbol(Symbol::Combined)
{
	parts.resize(2);
	parts[0] = NULL;
	parts[1] = NULL;
	private_parts.assign(2, false);
}

CombinedSymbol::~CombinedSymbol()
{
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (private_parts[i])
			delete parts[i];
	}
}

Symbol* CombinedSymbol::duplicate(const MapColorMap* color_map) const
{
	CombinedSymbol* new_symbol = new CombinedSymbol();
	new_symbol->duplicateImplCommon(this);
	new_symbol->parts = parts;
	new_symbol->private_parts = private_parts;
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (private_parts[i])
			new_symbol->parts[i] = parts[i]->duplicate(color_map);
	}
	return new_symbol;
}

void CombinedSymbol::createRenderables(const Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output) const
{
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i])
			parts[i]->createRenderables(object, flags, coords, output);
	}
}

void CombinedSymbol::colorDeleted(const MapColor* color)
{
	if (containsColor(color))
		resetIcon();
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (private_parts[i])
			// Note on const_cast: private part is owned by this symbol.
			const_cast<Symbol*>(parts[i])->colorDeleted(color);
	}
}

bool CombinedSymbol::containsColor(const MapColor* color) const
{
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i] && parts[i]->containsColor(color))
		{
			return true;
		}
	}
	return false;
}

const MapColor* CombinedSymbol::getDominantColorGuess() const
{
	// Speculative heuristic. Prefers areas and non-white colors.
	const MapColor* dominant_color = NULL;
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i] && parts[i]->getContainedTypes() & Symbol::Area)
		{
			dominant_color = parts[i]->getDominantColorGuess();
			if (! dominant_color->isWhite())
				return dominant_color;
		}
	}
	
	if (dominant_color)
		return dominant_color;
	
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i] && !(parts[i]->getContainedTypes() & Symbol::Area))
		{
			dominant_color = parts[i]->getDominantColorGuess();
			if (dominant_color->isWhite())
				return dominant_color;
		}
	}
	
	return dominant_color;
}

bool CombinedSymbol::symbolChanged(const Symbol* old_symbol, const Symbol* new_symbol)
{
	bool have_symbol = false;
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i] == old_symbol)
		{
			have_symbol = true;
			parts[i] = new_symbol;
		}
	}
	
	// always invalidate the icon, since the parts might have changed.
	resetIcon();
	
	return have_symbol;
}

bool CombinedSymbol::containsSymbol(const Symbol* symbol) const
{
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (parts[i] == symbol)
			return true;
		else if (parts[i] == NULL)
			continue;
		if (parts[i]->getType() == Symbol::Combined)	// TODO: see TODO in SymbolDropDown constructor.
		{
			const CombinedSymbol* combined_symbol = reinterpret_cast<const CombinedSymbol*>(parts[i]);
			if (combined_symbol->containsSymbol(symbol))
				return true;
		}
	}
	return false;
}

void CombinedSymbol::scale(double factor)
{
	for (int i = 0, size = (int)parts.size(); i < size; ++i)
	{
		if (private_parts[i])
		{
			// Note on const_cast: private part is owned by this symbol.
			const_cast<Symbol*>(parts[i])->scale(factor);
		}
	}
	
	resetIcon();
}

Symbol::Type CombinedSymbol::getContainedTypes() const
{
	int type = (int)getType();
	
	int size = (int)parts.size();
	for (int i = 0; i < size; ++i)
	{
		if (parts[i])
			type |= parts[i]->getContainedTypes();
	}
	
	return (Type)type;
}

#ifndef NO_NATIVE_FILE_FORMAT

bool CombinedSymbol::loadImpl(QIODevice* file, int version, Map* map)
{
	int size;
	file->read((char*)&size, sizeof(int));
	temp_part_indices.resize(size);
	parts.resize(size);
	
	for (int i = 0; i < size; ++i)
	{
		bool is_private = false;
		if (version >= 22)
			file->read((char*)&is_private, sizeof(bool));
		private_parts[i] = is_private;
		
		if (is_private)
		{
			// Note on const_cast: private part is owned by this symbol.
			if (!Symbol::loadSymbol(const_cast<Symbol*&>(parts[i]), file, version, map))
				return false;
			temp_part_indices[i] = -1;
		}
		else
		{
			int temp;
			file->read((char*)&temp, sizeof(int));
			temp_part_indices[i] = temp;
		}
	}
	return true;
}

#endif

void CombinedSymbol::saveImpl(QXmlStreamWriter& xml, const Map& map) const
{
	xml.writeStartElement("combined_symbol");
	int num_parts = (int)parts.size();
	xml.writeAttribute("parts", QString::number(num_parts));
	for (int i = 0; i < num_parts; ++i)
	{
		xml.writeStartElement("part");
		bool is_private = private_parts[i];
		if (is_private)
		{
			xml.writeAttribute("private", "true");
			parts[i]->save(xml, map);
		}
		else
		{
			int temp = (parts[i] == NULL) ? -1 : map.findSymbolIndex(parts[i]);
			xml.writeAttribute("symbol", QString::number(temp));
		}
		xml.writeEndElement(/*part*/);
	}
	xml.writeEndElement(/*combined_symbol*/);
}

bool CombinedSymbol::loadImpl(QXmlStreamReader& xml, const Map& map, SymbolDictionary& symbol_dict)
{
	if (xml.name() != "combined_symbol")
		return false;
	
	int num_parts = xml.attributes().value("parts").toString().toInt();
	temp_part_indices.reserve(num_parts % 10); // 10 is not the limit
	private_parts.clear();
	private_parts.reserve(num_parts % 10);
	parts.clear();
	parts.reserve(num_parts % 10);
	while (xml.readNextStartElement())
	{
		if (xml.name() == "part")
		{
			bool is_private = (xml.attributes().value("private") == "true");
			private_parts.push_back(is_private);
			if (is_private)
			{
				xml.readNextStartElement();
				parts.push_back(Symbol::load(xml, map, symbol_dict));
				temp_part_indices.push_back(-1);
			}
			else
			{
				int temp = xml.attributes().value("symbol").toString().toInt();
				temp_part_indices.push_back(temp);
				parts.push_back(NULL);
			}
			xml.skipCurrentElement();
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	return true;
}

bool CombinedSymbol::equalsImpl(const Symbol* other, Qt::CaseSensitivity case_sensitivity) const
{
	const CombinedSymbol* combination = static_cast<const CombinedSymbol*>(other);
	if (parts.size() != combination->parts.size())
		return false;
	// TODO: parts are only compared in order
	for (size_t i = 0, end = parts.size(); i < end; ++i)
	{
		if (private_parts[i] != combination->private_parts[i])
			return false;
		if ((parts[i] == NULL && combination->parts[i] != NULL) ||
			(parts[i] != NULL && combination->parts[i] == NULL))
			return false;
		if (parts[i] && !parts[i]->equals(combination->parts[i], case_sensitivity))
			return false;
	}
	return true;
}

bool CombinedSymbol::loadFinished(Map* map)
{
	int size = (int)temp_part_indices.size();
	if (size == 0)
		return true;
	for (int i = 0; i < size; ++i)
	{
		int index = temp_part_indices[i];
		if (index < 0)
			continue;	// part is private and has already been loaded
		if (index >= map->getNumSymbols())
			return false;
		parts[i] = map->getSymbol(index);
	}
	temp_part_indices.clear();
	return true;
}

float CombinedSymbol::calculateLargestLineExtent(Map* map) const
{
	float result = 0;
	for (size_t i = 0, end = parts.size(); i < end; ++i)
	{
		if (parts[i])
			result = qMax(result, parts[i]->calculateLargestLineExtent(map));
	}
	return result;
}

void CombinedSymbol::setPart(int i, const Symbol* symbol, bool is_private)
{
	if (private_parts[i])
		delete parts[i];
	
	parts[i] = symbol;
	private_parts[i] = (symbol == NULL) ? false : is_private;
}

SymbolPropertiesWidget* CombinedSymbol::createPropertiesWidget(SymbolSettingDialog* dialog)
{
	return new CombinedSymbolSettings(this, dialog);
}

// ### CombinedSymbolSettings ###

const int CombinedSymbolSettings::max_count = 5;

CombinedSymbolSettings::CombinedSymbolSettings(CombinedSymbol* symbol, SymbolSettingDialog* dialog) 
 : SymbolPropertiesWidget(symbol, dialog), symbol(symbol)
{
	const CombinedSymbol* source_symbol = static_cast<const CombinedSymbol*>(dialog->getUnmodifiedSymbol());
	Map* source_map = dialog->getSourceMap();
	
	QFormLayout* layout = new QFormLayout();
	
	number_edit = new QSpinBox();
	number_edit->setRange(2, qMax<int>(max_count, symbol->getNumParts()));
	number_edit->setValue(symbol->getNumParts());
	connect(number_edit, SIGNAL(valueChanged(int)), this, SLOT(numberChanged(int)));
	layout->addRow(tr("&Number of parts:"), number_edit);
	
	QSignalMapper* button_signal_mapper = new QSignalMapper(this);
	connect(button_signal_mapper, SIGNAL(mapped(int)), this, SLOT(editClicked(int)));
	QSignalMapper* symbol_signal_mapper = new QSignalMapper(this);
	connect(symbol_signal_mapper, SIGNAL(mapped(int)), this, SLOT(symbolChanged(int)));
	
	symbol_labels = new QLabel*[max_count];
	symbol_edits = new SymbolDropDown*[max_count];
	edit_buttons = new QPushButton*[max_count];
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i] = new QLabel(tr("Symbol %1:").arg(i+1));
		
		symbol_edits[i] = new SymbolDropDown(source_map, Symbol::Line | Symbol::Area | Symbol::Combined, ((int)symbol->parts.size() > i) ? symbol->parts[i] : NULL, source_symbol);
		symbol_edits[i]->addCustomItem(tr("- Private line symbol -"), 1);
		symbol_edits[i]->addCustomItem(tr("- Private area symbol -"), 2);
		if (((int)symbol->parts.size() > i) && symbol->isPartPrivate(i))
			symbol_edits[i]->setCustomItem((symbol->getPart(i)->getType() == Symbol::Line) ? 1 : 2);
		connect(symbol_edits[i], SIGNAL(currentIndexChanged(int)), symbol_signal_mapper, SLOT(map()));
		symbol_signal_mapper->setMapping(symbol_edits[i], i);
		
		edit_buttons[i] = new QPushButton(tr("Edit private symbol..."));
		edit_buttons[i]->setEnabled((int)symbol->parts.size() > i && symbol->private_parts[i]);
		connect(edit_buttons[i], SIGNAL(clicked()), button_signal_mapper, SLOT(map()));
		button_signal_mapper->setMapping(edit_buttons[i], i);
		
		QHBoxLayout* row_layout = new QHBoxLayout();
		row_layout->addWidget(symbol_edits[i]);
		row_layout->addWidget(edit_buttons[i]);
		layout->addRow(symbol_labels[i], row_layout);
		
		if (i >= symbol->getNumParts())
		{
			symbol_labels[i]->hide();
			symbol_edits[i]->hide();
			edit_buttons[i]->hide();
		}
	}
	
	QWidget* widget = new QWidget;
	widget->setLayout(layout);
	addPropertiesGroup(tr("Combination settings"), widget);
}

CombinedSymbolSettings::~CombinedSymbolSettings()
{
	delete[] symbol_labels;
	delete[] symbol_edits;
	delete[] edit_buttons;
}

void CombinedSymbolSettings::numberChanged(int value)
{
	int old_num_items = symbol->getNumParts();
	if (old_num_items == value)
		return;
	
	int num_items = value;
	symbol->setNumParts(num_items);
	for (int i = 0; i < max_count; ++i)
	{
		symbol_labels[i]->setVisible(i < num_items);
		symbol_edits[i]->setVisible(i < num_items);
		edit_buttons[i]->setVisible(i < num_items);
		
		if (i >= old_num_items && i < num_items)
		{
			// This item appears now
			symbol->setPart(i, NULL, false);
			symbol_edits[i]->blockSignals(true);
			symbol_edits[i]->setSymbol(NULL);
			symbol_edits[i]->blockSignals(false);
			edit_buttons[i]->setEnabled(symbol->isPartPrivate(i));
		}
	}
	emit propertiesModified();
}

void CombinedSymbolSettings::symbolChanged(int index)
{
	if (symbol_edits[index]->symbol() != NULL)
		symbol->setPart(index, symbol_edits[index]->symbol(), false);
	else if (symbol_edits[index]->customID() > 0)
	{
		// Changing to a private symbol
		Symbol::Type new_symbol_type;
		if (symbol_edits[index]->customID() == 1)
			new_symbol_type = Symbol::Line;
		else // if (symbol_edits[index]->customID() == 2)
			new_symbol_type = Symbol::Area;
		
		Symbol* new_symbol = NULL;
		if (symbol->getPart(index) != NULL && new_symbol_type == symbol->getPart(index)->getType())
		{
			if (QMessageBox::question(this, tr("Change from public to private symbol"),
				tr("Take the old symbol as template for the private symbol?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				new_symbol = symbol->getPart(index)->duplicate();
			}
		}
		
		if (new_symbol == NULL)
			new_symbol = Symbol::getSymbolForType(new_symbol_type);
		
		symbol->setPart(index, new_symbol, true);
		editClicked(index);
	}
	else
		symbol->setPart(index, NULL, false);
	
	edit_buttons[index]->setEnabled(symbol->isPartPrivate(index));
	emit propertiesModified();
}

void CombinedSymbolSettings::editClicked(int index)
{
	Q_ASSERT(symbol->isPartPrivate(index));
	if (!symbol->isPartPrivate(index))
		return;
	
	QScopedPointer<Symbol> part(symbol->getPart(index)->duplicate());
	SymbolSettingDialog sub_dialog(part.data(), dialog->getSourceMap(), this);
	sub_dialog.setWindowModality(Qt::WindowModal);
	if (sub_dialog.exec() == QDialog::Accepted)
	{
		symbol->setPart(index, sub_dialog.getNewSymbol(), true);
		emit propertiesModified();
	}
}

void CombinedSymbolSettings::reset(Symbol* symbol)
{
	Q_ASSERT(symbol->getType() == Symbol::Combined);
	
	SymbolPropertiesWidget::reset(symbol);
	
	this->symbol = symbol->asCombined();
	updateContents();
}

void CombinedSymbolSettings::updateContents()
{
	int num_parts = symbol->getNumParts();
	for (int i = 0; i < max_count; ++i)
	{
		symbol_edits[i]->blockSignals(true);
		if (i < num_parts)
		{
			if (symbol->isPartPrivate(i))
				symbol_edits[i]->setCustomItem((symbol->getPart(i)->getType() == Symbol::Line) ? 1 : 2);
			else
				symbol_edits[i]->setSymbol(symbol->parts[i]);
			symbol_edits[i]->show();
			symbol_labels[i]->show();
			edit_buttons[i]->setEnabled(symbol->isPartPrivate(i));
			edit_buttons[i]->show();
		}
		else
		{
			symbol_edits[i]->setSymbol(NULL);
			symbol_edits[i]->hide();
			symbol_labels[i]->hide();
			edit_buttons[i]->hide();
		}
		symbol_edits[i]->blockSignals(false);
	}
	
	number_edit->blockSignals(true);
	number_edit->setValue(num_parts);
	number_edit->blockSignals(false);
}
