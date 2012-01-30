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


#ifndef _OPENORIENTEERING_SYMBOL_AREA_H_
#define _OPENORIENTEERING_SYMBOL_AREA_H_

#include <QGroupBox>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QLabel;
QT_END_NAMESPACE

class ColorDropDown;
class SymbolSettingDialog;
class PointSymbolEditorWidget;
class PathObject;
class PointObject;

class AreaSymbol : public Symbol
{
friend class AreaSymbolSettings;
friend class PointSymbolEditorWidget;
public:
	struct FillPattern
	{
		enum Type
		{
			LinePattern = 1,
			PointPattern = 2
		};
		
		Type type;
		float angle;			// 0 to 2*M_PI
		int line_spacing;		// in 0.001mm
		
		MapColor* line_color;	// only if type == LinePattern
		int line_width;			// line width if type == LinePattern
		
		int point_distance;		// point distance if type == PointPattern
		PointSymbol* point;		// contained point symbol if type == PointPattern
		
		FillPattern();
		void save(QFile* file, Map* map);
		bool load(QFile* file, Map* map);
		void createRenderables(QRectF extent, RenderableVector& output);
		void createLine(MapCoordVectorF& coords, LineSymbol* line, PathObject* path, PointObject* point_object, RenderableVector& output);
	};
	
	AreaSymbol();
	virtual ~AreaSymbol();
    virtual Symbol* duplicate();
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
	
	// Getters / Setters
	inline MapColor* getColor() const {return color;}
	inline void setColor(MapColor* color) {this->color = color;}
	
	inline int getNumFillPatterns() const {return (int)patterns.size();}
	inline FillPattern& getFillPattern(int i) {return patterns[i];}
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, Map* map);
	
	MapColor* color;
	std::vector<FillPattern> patterns;
};

class AreaSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	AreaSymbolSettings(AreaSymbol* symbol, Map* map, SymbolSettingDialog* parent, PointSymbolEditorWidget* point_editor);
	
protected slots:
	void colorChanged();
	void addFillClicked();
	void deleteFillClicked();
	void fillNumberChanged(int index);
	void fillTypeChanged(int index);
	void fillAngleChanged(QString text);
	void fillSpacingChanged(QString text);
	void fillColorChanged();
	void fillLinewidthChanged(QString text);
	void fillPointdistChanged(QString text);
	
private:
	void updatePointSymbolNames();
	void updateFillWidgets(bool show);
	
	AreaSymbol* symbol;
	SymbolSettingDialog* dialog;
	PointSymbolEditorWidget* point_editor;
	
	ColorDropDown* color_edit;
	QPushButton* add_fill_button;
	QPushButton* delete_fill_button;
	
	QWidget* fill_pattern_widget;
	QComboBox* fill_number_combo;
	QComboBox* fill_type_combo;
	QLineEdit* fill_angle_edit;
	QLineEdit* fill_spacing_edit;
	
	QLabel* fill_color_label;
	ColorDropDown* fill_color_edit;
	QLabel* fill_linewidth_label;
	QLineEdit* fill_linewidth_edit;
	QLabel* fill_pointdist_label;
	QLineEdit* fill_pointdist_edit;
	
	bool react_to_changes;
};

#endif
