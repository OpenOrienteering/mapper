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


#ifndef _OPENORIENTEERING_SYMBOL_LINE_H_
#define _OPENORIENTEERING_SYMBOL_LINE_H_

#include "symbol.h"
#include "path_coord.h"
#include "symbol_properties_widget.h"

class QLineEdit;
class QLabel;
class QCheckBox;
class QScrollArea;

class ColorDropDown;
class SymbolSettingDialog;
class PointSymbolEditorWidget;
class PointSymbol;

class LineSymbol : public Symbol
{
friend class LineSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	enum CapStyle
	{
		FlatCap = 0,
		RoundCap = 1,
		SquareCap = 2,
		PointedCap = 3
	};
	
	enum JoinStyle
	{
		BevelJoin = 0,
		MiterJoin = 1,
		RoundJoin = 2
	};
	
	/// Constructs an empty point symbol
	LineSymbol();
	virtual ~LineSymbol();
    virtual Symbol* duplicate() const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output);
	void createRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, PathCoordVector* path_coords, ObjectRenderables& output);
	virtual void colorDeleted(MapColor* color);
    virtual bool containsColor(MapColor* color);
    virtual void scale(double factor);
	
	/// Creates empty point symbols for contained NULL symbols with the given names
    void ensurePointSymbols(const QString& start_name, const QString& mid_name, const QString& end_name, const QString& dash_name);
	/// Deletes unused point symbols and sets them to NULL
	void cleanupPointSymbols();
	
	/// Returns the limit for miter joins in units of the line width.
	/// See the Qt docs for QPainter::setMiterJoin().
	/// TODO: Should that better be a line property?
	static const float miterLimit() {return 1;}
	
	// Getters / Setters TODO: complete
	inline int getLineWidth() const {return line_width;}
	inline void setLineWidth(double width) {line_width = qRound(1000 * width);}
	inline MapColor* getColor() const {return color;}
	inline void setColor(MapColor* color) {this->color = color;}
	inline CapStyle getCapStyle() const {return cap_style;}
	inline void setCapStyle(CapStyle style) {cap_style = style;}
	inline JoinStyle getJoinStyle() const {return join_style;}
	inline void setJoinStyle(JoinStyle style) {join_style = style;}
    inline PointSymbol* getStartSymbol() const {return start_symbol;}
    inline PointSymbol* getMidSymbol() const {return mid_symbol;}
    inline PointSymbol* getEndSymbol() const {return end_symbol;}
    inline PointSymbol* getDashSymbol() const {return dash_symbol;}
    inline bool hasBorder() const {return have_border_lines;}
	inline int getBorderLineWidth() const {return border_width;}
	inline MapColor* getBorderColor() const {return border_color;}
	inline int getBorderShift() const {return border_shift;}
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, int version, Map* map);
	
	void createBorderLines(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, ObjectRenderables& output);
	void shiftCoordinates(const MapCoordVector& flags, const MapCoordVectorF& coords, bool path_closed, float shift, MapCoordVector& out_flags, MapCoordVectorF& out_coords);
	void processContinuousLine(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
							   float start, float end, bool has_start, bool has_end, int& cur_line_coord,
							   MapCoordVector& processed_flags, MapCoordVectorF& processed_coords, bool include_first_point, bool set_mid_symbols, ObjectRenderables& output);
	void createPointedLineCap(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
							  float start, float end, int& cur_line_coord, bool is_end, ObjectRenderables& output);
	void processDashedLine(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, MapCoordVector& out_flags, MapCoordVectorF& out_coords, ObjectRenderables& output);
	void createDashSymbolRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output);
    void createDottedRenderables(Object* object, bool path_closed, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output);
	
	void calculateCoordinatesForRange(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords,
									  float start, float end, int& cur_line_coord, bool include_start_coord, MapCoordVector& out_flags, MapCoordVectorF& out_coords,
									  std::vector<float>* out_lengths, bool set_mid_symbols, ObjectRenderables& output);
	void advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const PathCoordVector& line_coords, int& cur_line_coord, int& current_index, float cur_length,
								  int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, std::vector<float>* out_lengths, const MapCoordF& o3, const MapCoordF& o4);
	
	// Base line
	int line_width;		// in 1/1000 mm
	MapColor* color;
	int minimum_length;
	CapStyle cap_style;
	JoinStyle join_style;
	int pointed_cap_length;
	
	bool dashed;
	
	// Point symbols
	PointSymbol* start_symbol;
	PointSymbol* mid_symbol;
	PointSymbol* end_symbol;
    PointSymbol* dash_symbol;

	int mid_symbols_per_spot;
	int mid_symbol_distance;
	
	// Not dashed
	int segment_length;
	int end_length;
	bool show_at_least_one_symbol;
	int minimum_mid_symbol_count;
	int minimum_mid_symbol_count_when_closed;
	
	// Dashed
	int dash_length;
	int break_length;
	int dashes_in_group;
	int in_group_break_length;
	bool half_outer_dashes;
	
	// Border lines
	bool have_border_lines;
	MapColor* border_color;
	int border_width;
	int border_shift;
	bool dashed_border;
	int border_dash_length;
	int border_break_length;
};



class LineSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	LineSymbolSettings(LineSymbol* symbol, SymbolSettingDialog* dialog);
	
	virtual ~LineSymbolSettings();
	
	virtual void reset(Symbol* symbol);
	
	/**
	 * Updates the content and state of all input fields.
	 */
	void updateContents();
	
protected:
	/** 
	 * Ensures that a particular widget is visible in the scoll area. 
	 */
	void ensureWidgetVisible(QWidget* widget);
	
	/** 
	 * Adjusts the visibility and enabled state of all UI parts.
	 * There is a large number of dependencies between different elements
	 * of the line settings. This method handles them all.
	 */
	void updateStates();
	
protected slots:
	/** 
	 * Notifies this settings widget that one of the symbols has been modified.
	 */
	void pointSymbolEdited();
	
	void widthChanged(QString text);
	void colorChanged();
	void minimumDimensionsEdited(QString text);
	void lineCapChanged(int index);
	void lineJoinChanged(int index);
	void pointedLineCapLengthChanged(QString text);
	void dashedChanged(bool checked);
	void segmentLengthChanged(QString text);
	void endLengthChanged(QString text);
	void showAtLeastOneSymbolChanged(bool checked);
	void dashLengthChanged(QString text);
	void breakLengthChanged(QString text);
	void dashGroupsChanged(int index);
	void inGroupBreakLengthChanged(QString text);
	void halfOuterDashesChanged(bool checked);
	void midSymbolsPerDashChanged(QString text);
	void midSymbolDistanceChanged(QString text);
	void borderCheckClicked(bool checked);
	void borderWidthEdited(QString text);
	void borderColorChanged();
	void borderShiftChanged(QString text);
	void borderDashedClicked(bool checked);
	void borderDashesChanged(QString text);
	
private slots:
	/** Ensure that a predetermined widget is visible in the scoll area.
	 *  The widget is set in advance by ensureWidgetVisible(QWidget* widget).
	 */
	void ensureWidgetVisible();
	
private:
	LineSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QLineEdit* width_edit;
	ColorDropDown* color_edit;
	QLineEdit* minimum_length_edit;
	
	// enabled if line_width > 0 && color != NULL
	QList<QWidget *> line_settings_list;
	QComboBox* line_cap_combo;
	QComboBox* line_join_combo;
	QLabel* pointed_cap_length_label;
	QLineEdit* pointed_cap_length_edit;
	QCheckBox* dashed_check;
	
	// dashed == false && mid_symbol
	QList<QWidget *> undashed_widget_list;
	QLineEdit* segment_length_edit;
	QLineEdit* end_length_edit;
	QCheckBox* show_at_least_one_symbol_check;
	QLineEdit* minimum_mid_symbol_count_edit;
	QLineEdit* minimum_mid_symbol_count_when_closed_edit;
	
	// dashed == true
	QList<QWidget *> dashed_widget_list;
	QLineEdit* dash_length_edit;
	QLineEdit* break_length_edit;
	QComboBox* dash_group_combo;
	QLabel* in_group_break_length_label;
	QLineEdit* in_group_break_length_edit;
	QCheckBox* half_outer_dashes_check;
	
	// mid_symbol
	QList<QWidget *> mid_symbol_widget_list;
	QLineEdit* mid_symbol_per_spot_edit;
	QLabel* mid_symbol_distance_label;
	QLineEdit* mid_symbol_distance_edit;
	
	// enabled if line_width > 0
	QList<QWidget *> border_widget_list;
	QCheckBox* border_check;
	QLineEdit* border_width_edit;
	ColorDropDown* border_color_edit;
	QLineEdit* border_shift_edit;
	QCheckBox* border_dashed_check;
	
	QList<QWidget *> border_dash_widget_list;
	QLineEdit* border_dash_length_edit;
	QLineEdit* border_break_length_edit;
	
	QScrollArea* scroll_area;
	QWidget* widget_to_ensure_visible;
};

#endif
