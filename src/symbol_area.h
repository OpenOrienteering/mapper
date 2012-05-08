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

#include "symbol.h"
#include "symbol_properties_widget.h"

class QPushButton;
class QLabel;
class QCheckBox;

class ColorDropDown;
class SymbolSettingDialog;
class PointSymbolEditorWidget;
class PathObject;
class PointObject;
class SymbolPropertiesWidget;
class SymbolSettingDialog;
class MapEditorController;

class AreaSymbol : public Symbol
{
friend class AreaSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
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
		bool rotatable;
		int line_spacing;		// as usual, in 0.001mm
		int line_offset;
		int offset_along_line;	// only if type == PointPattern
		
		MapColor* line_color;	// only if type == LinePattern
		int line_width;			// line width if type == LinePattern
		
		int point_distance;		// point distance if type == PointPattern
		PointSymbol* point;		// contained point symbol if type == PointPattern
		
		QString name;			// a display name (transient)
		
		FillPattern();
		void save(QFile* file, Map* map);
		bool load(QFile* file, int version, Map* map);
		void createRenderables(QRectF extent, RenderableVector& output);
		void createLine(MapCoordVectorF& coords, LineSymbol* line, PathObject* path, PointObject* point_object, RenderableVector& output);
		void scale(double factor);
	};
	
	AreaSymbol();
	virtual ~AreaSymbol();
    virtual Symbol* duplicate() const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
    virtual void scale(double factor);
	
	// Getters / Setters
	inline MapColor* getColor() const {return color;}
	inline void setColor(MapColor* color) {this->color = color;}
    inline int getMinimumArea() const {return minimum_area; }
	inline int getNumFillPatterns() const {return (int)patterns.size();}
	inline FillPattern& getFillPattern(int i) {return patterns[i];}
    inline const FillPattern& getFillPattern(int i) const {return patterns[i];}
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);

protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, int version, Map* map);
	
	MapColor* color;
	int minimum_area;	// in mm^2
	std::vector<FillPattern> patterns;
};

class AreaSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog);
	
protected slots:
	void colorChanged();
	void minimumDimensionsChanged(QString text);
	void addFillClicked();
	void deleteFillClicked();
	void fillNumberChanged(int index);
	void fillTypeChanged(int index);
	void fillAngleChanged(QString text);
	void fillRotatableClicked(bool checked);
	void fillSpacingChanged(QString text);
	void fillLineOffsetChanged(QString text);
	void fillOffsetAlongLineChanged(QString text);
	void fillColorChanged();
	void fillLinewidthChanged(QString text);
	void fillPointdistChanged(QString text);
	
private:
	void updatePatternNames(bool update_ui = true);
	void updateFillWidgets(bool show);
	
	AreaSymbol* symbol;
	Map* map;
	MapEditorController* controller;
	
	ColorDropDown* color_edit;
	QLineEdit* minimum_area_edit;
	QPushButton* add_fill_button;
	QPushButton* delete_fill_button;
	
	QWidget* fill_pattern_widget;
	QComboBox* fill_number_combo;
	QComboBox* fill_type_combo;
	QLineEdit* fill_angle_edit;
	QCheckBox* fill_rotatable_check;
	QLineEdit* fill_spacing_edit;
	QLineEdit* fill_line_offset_edit;
	QLabel* fill_offset_along_line_label;
	QLineEdit* fill_offset_along_line_edit;
	
	QLabel* fill_color_label;
	ColorDropDown* fill_color_edit;
	QLabel* fill_linewidth_label;
	QLineEdit* fill_linewidth_edit;
	QLabel* fill_pointdist_label;
	QLineEdit* fill_pointdist_edit;
	
	bool react_to_changes;
};

#endif
