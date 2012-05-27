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

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QToolButton;

class ColorDropDown;
class SymbolSettingDialog;
class PointSymbolEditorWidget;
class PathObject;
class PointObject;
class LineSymbol;
class PointSymbol;
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
	int minimum_area;	// in mm^2 // FIXME: unit (factor) wrong
	std::vector<FillPattern> patterns;
};

class AreaSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	AreaSymbolSettings(AreaSymbol* symbol, SymbolSettingDialog* dialog);
	
	virtual bool isResetSupported() { return true; }
	
	virtual void reset(Symbol* symbol);
	
	/**
	 * Updates the general area fields (not related to patterns)
	 */
	void updateAreaGeneral();
	
	void addPattern(AreaSymbol::FillPattern::Type type);
	
signals:
	void switchPatternEdits(int layer);
	
public slots:
	void selectPattern(int index);
	void addLinePattern();
	void addPointPattern();
	void deleteActivePattern();
	
protected:
	void clearPatterns();
	void loadPatterns();
	void updatePatternNames();
	void updatePatternWidgets();
	
protected slots:
	void colorChanged();
	void minimumSizeChanged(double value);
	void patternAngleChanged(double value);
	void patternRotatableClicked(bool checked);
	void patternSpacingChanged(double value);
	void patternLineOffsetChanged(double value);
	void patternOffsetAlongLineChanged(double value);
	void patternColorChanged();
	void patternLineWidthChanged(double value);
	void patternPointDistChanged(double value);
	
private:
	AreaSymbol* symbol;
	Map* map;
	MapEditorController* controller;
	std::vector<AreaSymbol::FillPattern>::iterator active_pattern;
	
	ColorDropDown*  color_edit;
	QDoubleSpinBox* minimum_size_edit;
	
	QListWidget*    pattern_list;
	QToolButton*    add_pattern_button;
	QPushButton*    del_pattern_button;
	
	QLabel*         pattern_name_edit;
	QDoubleSpinBox* pattern_angle_edit;
	QCheckBox*      pattern_rotatable_check;
	QDoubleSpinBox* pattern_spacing_edit;
	QDoubleSpinBox* pattern_line_offset_edit;
	QDoubleSpinBox* pattern_offset_along_line_edit;
	
	ColorDropDown*  pattern_color_edit;
	QDoubleSpinBox* pattern_linewidth_edit;
	QDoubleSpinBox* pattern_pointdist_edit;
};

#endif
