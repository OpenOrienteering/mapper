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

#include <QGroupBox>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

class ColorDropDown;
class SymbolSettingDialog;
class PointSymbol;

class LineSymbol : public Symbol
{
friend class LineSymbolSettings;
friend class PointSymbolEditorWidget;
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
    virtual Symbol* duplicate();
	
	virtual void createRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
	
	/// Creates empty point symbols for contained NULL symbols with the given names
	void ensurePointSymbols(const QString& start_name, const QString& mid_name, const QString& end_name, const QString& dash_name);
	/// Deletes unused point symbols and sets them to NULL
	void cleanupPointSymbols();
	
	/// Returns the limit for miter joins in units of the line width.
	/// See the Qt docs for QPainter::setMiterJoin().
	/// TODO: Should that better be a line property?
	static const float miterLimit() {return 1;}
	
	// Getters / Setters
	inline int getLineWidth() const {return line_width;}
	inline void setLineWidth(double width) {line_width = qRound(1000 * width);}
	inline MapColor* getColor() const {return color;}
	inline void setColor(MapColor* color) {this->color = color;}
	inline CapStyle getCapStyle() const {return cap_style;}
	inline void setCapStyle(CapStyle style) {cap_style = style;}
	inline JoinStyle getJoinStyle() const {return join_style;}
	inline void setJoinStyle(JoinStyle style) {join_style = style;}
	inline PointSymbol* getStartSymbol() {return start_symbol;}
	inline PointSymbol* getMidSymbol() {return mid_symbol;}
	inline PointSymbol* getEndSymbol() {return end_symbol;}
	inline PointSymbol* getDashSymbol() {return dash_symbol;}
	
	// TODO: make configurable
	static const float bezier_error;
	
protected:
	struct LineCoord
	{
		MapCoordF pos;
		float clen;		// cumulative length since line part start
		int index;		// index into the MapCoordVector(F) to the first coordinate of the segment which contains this LineCoord
		float param;	// value of the curve parameter for this position
	};
	typedef std::vector<LineCoord> LineCoordVector;
	
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, Map* map);
	
	void processContinuousLine(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, const LineCoordVector& line_coords,
							   float start, float end, bool has_start, bool has_end, int& cur_line_coord,
							   MapCoordVector& processed_flags, MapCoordVectorF& processed_coords, bool include_first_point, bool set_mid_symbols, RenderableVector& output);
	void createPointedLineCap(const MapCoordVector& flags, const MapCoordVectorF& coords, const LineCoordVector& line_coords,
							  float start, float end, int& cur_line_coord, bool is_end, RenderableVector& output);
	MapCoordF calculateRightVector(const MapCoordVectorF& coords, int i, float* scaling);
	MapCoordF calculateTangent(const MapCoordVectorF& coords, int i, bool backward, bool second_try = false);
	void getCoordinatesForRange(const MapCoordVector& flags, const MapCoordVectorF& coords, const LineSymbol::LineCoordVector& line_coords,
								float start, float end, int& cur_line_coord, bool include_start_coord, MapCoordVector& out_flags, MapCoordVectorF& out_coords,
								std::vector<float>* out_lengths, bool set_mid_symbols, RenderableVector& output);
	void advanceCoordinateRangeTo(const MapCoordVector& flags, const MapCoordVectorF& coords, const LineCoordVector& line_coords, int& cur_line_coord, int& current_index, float cur_length,
								  int start_bezier_index, MapCoordVector& out_flags, MapCoordVectorF& out_coords, std::vector<float>* out_lengths, const MapCoordF& o3, const MapCoordF& o4);
	void processDashedLine(Object* object, const MapCoordVectorF& coords, MapCoordVector& out_flags, MapCoordVectorF& out_coords, RenderableVector& output);
	void createDashSymbolRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	void createDottedRenderables(Object* object, const MapCoordVectorF& coords, RenderableVector& output);
	bool getNextLinePart(const MapCoordVector& flags, const MapCoordVectorF& coords, int& part_start, int& part_end, LineCoordVector* line_coords, bool break_at_dash_points, bool append_line_coords);
	void curveToLineCoordRec(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, LineCoordVector* line_coords, float p0, float p1);
	void curveToLineCoord(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, int coord_index, float max_error, LineCoordVector* line_coords);
	void calcPositionAt(const MapCoordVector& flags, const MapCoordVectorF& coords, const LineCoordVector& line_coords, float length, int& line_coord_search_start, MapCoordF* out_pos, MapCoordF* out_right_vector);
	void splitBezierCurve(MapCoordF c0, MapCoordF c1, MapCoordF c2, MapCoordF c3, float p, MapCoordF& o0, MapCoordF& o1, MapCoordF& o2, MapCoordF& o3, MapCoordF& o4);
	
	// Base line
	int line_width;		// in 1/1000 mm
	MapColor* color;
	CapStyle cap_style;
	JoinStyle join_style;
	int pointed_cap_length;
	
	bool dashed;
	
	// Point symbols
	PointSymbol* start_symbol;
	PointSymbol* mid_symbol;
	PointSymbol* end_symbol;
	PointSymbol* dash_symbol;
	
	// Not dashed
	int segment_length;
	
	// Dashed
	int dash_length;
	int break_length;
	int dashes_in_group;
	int in_group_break_length;
	bool half_outer_dashes;
	
	// Border lines
	// TODO
};

class LineSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	LineSymbolSettings(LineSymbol* symbol, Map* map, SymbolSettingDialog* parent);
	
protected slots:
	void widthChanged(QString text);
	void colorChanged();
	void lineCapChanged(int index);
	void lineJoinChanged(int index);
	void pointedLineCapLengthChanged(QString text);
	void dashedChanged(bool checked);
	void segmentLengthChanged(QString text);
	void dashLengthChanged(QString text);
	void breakLengthChanged(QString text);
	void dashGroupsChanged(int index);
	void inGroupBreakLengthChanged(QString text);
	void halfOuterDashesChanged(bool checked);
	
private:
	void updateWidgets(bool show = true);
	
	LineSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	QLineEdit* width_edit;
	ColorDropDown* color_edit;
	
	// enabled if line_width > 0 && color != NULL
	QWidget* line_settings_widget;
	QComboBox* line_cap_combo;
	QComboBox* line_join_combo;
	QLabel* pointed_cap_length_label;
	QLineEdit* pointed_cap_length_edit;
	QCheckBox* dashed_check;
	
	// dashed == false
	QWidget* undashed_widget;
	QLineEdit* segment_length_edit;
	
	// dashed == true
	QWidget* dashed_widget;
	QLineEdit* dash_length_edit;
	QLineEdit* break_length_edit;
	QComboBox* dash_group_combo;
	QLabel* in_group_break_length_label;
	QLineEdit* in_group_break_length_edit;
	QCheckBox* half_outer_dashes_check;
	
	// enabled if line_width > 0
	// TODO: border line edits
};

#endif
