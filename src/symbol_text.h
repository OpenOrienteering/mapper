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

#include "symbol.h"

#include <QGroupBox>
#include <QDialog>

#include "symbol_properties_widget.h"

class QCheckBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;

class ColorDropDown;
class SymbolSettingDialog;

/// Symbol for text
/// NOTE: Uses a hack to disable hinting for fonts: Uses a big internal font size where hinting is neglegible and scales it down for output
/// TODO: A better solution would be to use FreeType directly
class TextSymbol : public Symbol
{
friend class TextSymbolSettings;
friend class DetermineFontSizeDialog;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
public:
	TextSymbol();
	virtual ~TextSymbol();
    virtual Symbol* duplicate() const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, RenderableVector& output);
	void createLineBelowRenderables(Object* object, RenderableVector& output);
	virtual void colorDeleted(Map* map, int pos, MapColor* color);
    virtual bool containsColor(MapColor* color);
    virtual void scale(double factor);
	
	void updateQFont();
	inline double calculateInternalScaling() const {return internal_point_size / (0.001 * font_size);}
	
	// Getters
	inline MapColor* getColor() const {return color;}
	inline const QString& getFontFamily() const {return font_family;}
	inline double getFontSize() const {return 0.001 * font_size;}
	inline bool isBold() const {return bold;}
	inline bool isItalic() const {return italic;}
	inline bool isUnderlined() const {return underline;}
	inline float getLineSpacing() const {return line_spacing;}
	inline double getParagraphSpacing() const {return 0.001 * paragraph_spacing;}
	inline double getCharacterSpacing() const {return 0.001 * character_spacing;}
	inline bool usesKerning() const {return kerning;}
	inline bool hasLineBelow() const {return line_below;}
	inline MapColor* getLineBelowColor() const {return line_below_color;}
	inline double getLineBelowWidth() const {return 0.001 * line_below_width;}
	inline double getLineBelowDistance() const {return 0.001 * line_below_distance;}
	inline int getNumCustomTabs() const {return (int)custom_tabs.size();}
	inline int getCustomTab(int index) const {return custom_tabs[index];}
	inline const QFont& getQFont() const {return qfont;}
	inline const QFontMetricsF& getFontMetrics() const { return metrics; }
	
	double getNextTab(double pos) const;
	
	static const float pt_in_mm;	// 1 pt in mm
	static const float internal_point_size;
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
protected:
	virtual void saveImpl(QFile* file, Map* map);
	virtual bool loadImpl(QFile* file, int version, Map* map);
	
	QFont qfont;
	QFontMetricsF metrics;
	
	MapColor* color;
	QString font_family;
	int font_size;				// this defines the font size in 1000 mm. How big the letters really are depends on the design of the font though
	bool bold;
	bool italic;
	bool underline;
	float line_spacing;			// as factor of original line spacing
	int paragraph_spacing;		// in mm
	float character_spacing;	// as a factor of the space character width
	bool kerning;
	
	// OCAD compatibility features
	bool line_below;
	MapColor* line_below_color;
	int line_below_width;
	int line_below_distance;
	std::vector<int> custom_tabs;
	
	double tab_interval;		/// default tab interval length in text coordinates
};

class TextSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	TextSymbolSettings(TextSymbol* symbol, SymbolSettingDialog* dialog);
    virtual ~TextSymbolSettings();
	
	virtual bool isResetSupported() { return true; }
	
	virtual void reset(Symbol* symbol);
	
	void updateGeneralContents();
	
	void updateCompatibilityCheckEnabled();
	
	void updateCompatibilityContents();
	
protected slots:
	void fontChanged(QFont font);
	void sizeChanged(double value);
	void determineSizeClicked();
	void colorChanged();
	void checkToggled(bool checked);
	void spacingChanged(double value);
	
	void ocadCompatibilityButtonClicked(bool checked);
	void lineBelowCheckClicked(bool checked);
	void lineBelowSettingChanged();
	void customTabRowChanged(int row);
	void addCustomTabClicked();
	void removeCustomTabClicked();
	
private:
	TextSymbol* symbol;
	SymbolSettingDialog* dialog;
	
	ColorDropDown*  color_edit;
	QFontComboBox*  font_edit;
	QDoubleSpinBox* size_edit;
	QPushButton*    size_determine_button;
	QCheckBox*      bold_check;
	QCheckBox*      italic_check;
	QCheckBox*      underline_check;
	QDoubleSpinBox* line_spacing_edit;
	QDoubleSpinBox* paragraph_spacing_edit;
	QDoubleSpinBox* character_spacing_edit;
	QCheckBox*      kerning_check;
	QCheckBox*      ocad_compat_check;
	
	QWidget*        ocad_compat_widget;
	QCheckBox*      line_below_check;
	QDoubleSpinBox* line_below_width_edit;
	ColorDropDown*  line_below_color_edit;
	QDoubleSpinBox* line_below_distance_edit;
	QListWidget*    custom_tab_list;
	QPushButton*    custom_tab_add;
	QPushButton*    custom_tab_remove;
	
	bool react_to_changes;
};

class DetermineFontSizeDialog : public QDialog
{
Q_OBJECT
public:
	DetermineFontSizeDialog(QWidget* parent, TextSymbol* symbol);
	
public slots:
	virtual void accept();
	void updateOkButton();
	
private:
	QLineEdit* character_edit;
	QDoubleSpinBox* size_edit;
	QPushButton* ok_button;
	
	TextSymbol* symbol;
};

#endif
