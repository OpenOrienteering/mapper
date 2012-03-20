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


#ifndef _OPENORIENTEERING_SYMBOL_TEXT_H_
#define _OPENORIENTEERING_SYMBOL_TEXT_H_

#include <QGroupBox>

#include "symbol.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QFontComboBox;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class ColorDropDown;
class SymbolSettingDialog;

/// Symbol for text
/// NOTE: Uses a hack to disable hinting for fonts: Uses a big internal font size where hinting is neglegible and scales it down for output
/// TODO: A better solution would be to use FreeType directly
class TextSymbol : public Symbol
{
friend class TextSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	TextSymbol();
	virtual ~TextSymbol();
    virtual Symbol* duplicate();
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
    virtual void scale(double factor);
	
	void updateQFont();
	inline double calculateInternalScaling() const {return internal_point_size / (0.001 * ascent_size);}
	
	// Getters
	inline MapColor* getColor() const {return color;}
	inline const QString& getFontFamily() const {return font_family;}
	inline double getFontSize() const {return 0.001 * ascent_size;}
	inline bool isBold() const {return bold;}
	inline bool isItalic() const {return italic;}
	inline bool isUnderlined() const {return underline;}
	inline float getLineSpacing() const {return line_spacing;}
	inline bool usesKerning() const {return kerning;}
	inline const QFont& getQFont() const {return qfont;}
	inline const QFontMetricsF& getFontMetrics() const { return metrics; }
	
	double getNextTab(double pos) const;
	
	static const float pt_in_mm;	// 1 pt in mm
	static const float internal_point_size;
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, int version, Map* map);
	
	QFont qfont;
	QFontMetricsF metrics;
	
	MapColor* color;
	QString font_family;
	int ascent_size;		// Font size is defined by ascent height in 1000 * mm
	bool bold;
	bool italic;
	bool underline;
	float line_spacing;		// as factor of original line spacing
	bool kerning;
	
	double tab_interval;		/// default tab interval length in text coordinates
};

class TextSymbolSettings : public QGroupBox
{
Q_OBJECT
public:
	TextSymbolSettings(TextSymbol* symbol, Map* map, SymbolSettingDialog* parent);
	
protected slots:
	void fontChanged(QFont font);
	void sizeChanged(QString text);
	void sizeOptionMMClicked(bool checked);
	void sizeOptionPTClicked(bool checked);
	void colorChanged();
	void checkToggled(bool checked);
	void lineSpacingChanged(QString text);
	
private:
	TextSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	ColorDropDown* color_edit;
	QFontComboBox* font_edit;
	QLineEdit* size_edit;
	QPushButton* size_option_mm;
	QPushButton* size_option_pt;
	QCheckBox* bold_check;
	QCheckBox* italic_check;
	QCheckBox* underline_check;
	QLineEdit* line_spacing_edit;
	QCheckBox* kerning_check;
};

#endif
