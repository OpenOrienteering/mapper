/*
 *    Copyright 2011 Thomas Schöps
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


#include "symbol_setting_dialog.h"

#include <QtGui>

#include "symbol.h"
#include "symbol_point.h"
#include "main_window.h"
#include "map.h"
#include "map_editor.h"
#include "object.h"
#include "map_widget.h"
#include "symbol_point_editor.h"

SymbolSettingDialog::SymbolSettingDialog(Symbol* symbol, Map* map, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	setSizeGripEnabled(true);
	this->symbol = symbol;
	
	QGroupBox* general_group = new QGroupBox(tr("General"));
	
	QLabel* number_label = new QLabel(tr("Number:"));
	number_edit = new QLineEdit*[Symbol::number_components];
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_edit[i] = new QLineEdit((symbol->getNumberComponent(i) < 0) ? "" : QString::number(symbol->getNumberComponent(i)));
		number_edit[i]->setMaximumWidth(60);
		number_edit[i]->setValidator(new QIntValidator(0, 99999, number_edit[i]));
	}
	QLabel* name_label = new QLabel(tr("Name:"));
	name_edit = new QLineEdit(symbol->getName());
	QLabel* description_label = new QLabel(tr("Description:"));
	description_edit = new QTextEdit(symbol->getDescription());
	helper_symbol_check = new QCheckBox(tr("Helper symbol (not shown in finished map)"));
	
	QWidget* type_specific_settings;
	Symbol::Type type = symbol->getType();
	if (type == Symbol::Point)
		type_specific_settings = new PointSymbolSettings(reinterpret_cast<PointSymbol*>(symbol), map, this);
	
	QPushButton* cancel_button = new QPushButton(tr("Cancel"));
	ok_button = new QPushButton(QIcon("images/arrow-right.png"), tr("Ok"));
	ok_button->setDefault(true);
	
	preview_map = new Map();
	preview_map->copyColorsFrom(map);
	
	//createPreviewMap();
	PointSymbolEditorWidget* point_sybol_editor = createPointSymbolEditor();
	
	preview_widget = new MainWindow(false);
	MapEditorController* controller = new MapEditorController((symbol->getType() == Symbol::Point) ? MapEditorController::PointSymbolEditor : MapEditorController::SymbolPreview, preview_map);
	preview_widget->setController(controller);
	MapView* map_view = controller->getMainWidget()->getMapView();
	map_view->setZoom(8 * map_view->getZoom());
	
	QHBoxLayout* number_layout = new QHBoxLayout();
	number_layout->addWidget(number_label);
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_layout->addWidget(number_edit[i]);
		if (i < Symbol::number_components - 1)
			number_layout->addWidget(new QLabel("."));
	}
	number_layout->addStretch(1);
	
	QHBoxLayout* name_layout = new QHBoxLayout();
	name_layout->addWidget(name_label);
	name_layout->addWidget(name_edit);
	name_layout->addStretch(1);
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch(1);
	buttons_layout->addWidget(ok_button);
	
	QVBoxLayout* general_layout = new QVBoxLayout();
	general_layout->addLayout(number_layout);
	general_layout->addLayout(name_layout);
	general_layout->addWidget(description_label);
	general_layout->addWidget(description_edit);
	general_layout->addWidget(helper_symbol_check);
	general_group->setLayout(general_layout);
	
	QVBoxLayout* left_layout = new QVBoxLayout();
	left_layout->addWidget(general_group);
	left_layout->addWidget(type_specific_settings);
	left_layout->addLayout(buttons_layout);
	
	QHBoxLayout* layout = new QHBoxLayout();
	layout->addLayout(left_layout);
	layout->addWidget(preview_widget, 1);
	layout->addWidget(point_sybol_editor);
	setLayout(layout);
	
	for (int i = 0; i < Symbol::number_components; ++i)
		connect(number_edit[i], SIGNAL(textEdited(QString)), this, SLOT(numberChanged(QString)));
	connect(name_edit, SIGNAL(textEdited(QString)), this, SLOT(nameChanged(QString)));
	connect(description_edit, SIGNAL(textChanged()), this, SLOT(descriptionChanged()));
	
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(ok_button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	
	updateNumberEdits();
	updateOkButton();
	updateWindowTitle();
}

void SymbolSettingDialog::updatePreview()
{
	for (int l = 0; l < preview_map->getNumLayers(); ++l)
		for (int i = 0; i < preview_map->getLayer(l)->getNumObjects(); ++i)
			preview_map->getLayer(l)->getObject(i)->update(true);
}

void SymbolSettingDialog::numberChanged(QString text)
{
	bool is_valid = true;
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		QString number_text = number_edit[i]->text();
		if (is_valid && !number_text.isEmpty())
			symbol->setNumberComponent(i, number_text.toInt());
		else
		{
			symbol->setNumberComponent(i, -1);
			is_valid = false;
		}
	}
	
	updateNumberEdits();
	updateOkButton();
}
void SymbolSettingDialog::nameChanged(QString text)
{
	symbol->setName(text);
	updateOkButton();
	updateWindowTitle();
}
void SymbolSettingDialog::descriptionChanged()
{
	symbol->setDescription(description_edit->toPlainText());
}
void SymbolSettingDialog::helperSymbolClicked(bool checked)
{
	symbol->setIsHelperSymbol(checked);
}

/*void SymbolSettingDialog::createPreviewMap()
{
	if (symbol->getType() == Symbol::Point)
		createPointSymbolEditor(reinterpret_cast<PointSymbol*>(symbol));
}
void SymbolSettingDialog::createPointSymbolEditor(PointSymbol* point)
{
	PointObject* object = new PointObject(preview_map, MapCoord(0, 0), point);
	preview_map->addObject(object);
}*/
PointSymbolEditorWidget* SymbolSettingDialog::createPointSymbolEditor()
{
	if (symbol->getType() == Symbol::Point)
	{
		PointSymbol* point = reinterpret_cast<PointSymbol*>(symbol);
		std::vector<PointSymbol*> point_vector;
		point_vector.push_back(point);
		return new PointSymbolEditorWidget(preview_map, point_vector);
	}
	
	assert(false);
	return NULL;
}

void SymbolSettingDialog::updateNumberEdits()
{
	bool enable = true;
	for (int i = 0; i < Symbol::number_components; ++i)
	{
		number_edit[i]->setEnabled(enable);
		if (enable && number_edit[i]->text().isEmpty())
			enable = false;
	}
}
void SymbolSettingDialog::updateOkButton()
{
	ok_button->setEnabled(!number_edit[0]->text().isEmpty() && !name_edit->text().isEmpty());
}
void SymbolSettingDialog::updateWindowTitle()
{
	if (symbol->getName().isEmpty())
		setWindowTitle(tr("Symbol settings"));
	else
		setWindowTitle(tr("Symbol settings for %1").arg(symbol->getName()));
}

#include "symbol_setting_dialog.moc"
