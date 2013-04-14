/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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


#ifndef _OPENORIENTEERING_GEOREFERENCING_DIALOG_H_
#define _OPENORIENTEERING_GEOREFERENCING_DIALOG_H_

#include <QDialog>
#include <QLocale>
#include <QString>
#include <QScopedPointer>

#include "gps_coordinates.h"
#include "tool.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QNetworkReply;
QT_END_NAMESPACE

class Georeferencing;
class Map;
class MapCoord;
class MapEditorController;
class GeoreferencingTool;
class ProjectedCRSSelector;
class CRSTemplate;

/**
 * A GeoreferencingDialog allows the user to adjust the georeferencing properties
 * of a map.
 */
class GeoreferencingDialog : public QDialog
{
Q_OBJECT
public:
	/**
	 * Constructs a new georeferencing dialog for the map handled by the given 
	 * controller. The optional parameter initial allows to override the current 
	 * properties of the map's georeferencing. The parameter
	 * allow_no_georeferencing determines if the okay button can
	 * be clicked while "- none -" is selected.
	 */
	GeoreferencingDialog(MapEditorController* controller, const Georeferencing* initial = NULL, bool allow_no_georeferencing = true);
	
	/**
	 * Constructs a new georeferencing dialog for the given map. The optional 
	 * parameter initial allows to override the current properties of the map's
	 * georeferencing. Since the dialog will not know a MapEditorController,
	 * it will not allow to select a new reference point from the map.
	 * The parameter allow_no_georeferencing determines if the okay button can
	 * be clicked while "- none -" is selected.
	 */
	GeoreferencingDialog(QWidget* parent, Map* map, const Georeferencing* initial = NULL, bool allow_no_georeferencing = true);
	
	/**
	 * Releases resources.
	 */
	virtual ~GeoreferencingDialog();
	
	/**
	 * Shows the dialog as a modal dialog, blocking until it is hidden.
	 * 
	 * If the GeoreferencingTool (for selecting the reference point) is active
	 * it will be destroyed before showing the dialog.
	 * 
	 * Note that this function will also return when the dialog is temporary 
	 * hidden for activating the GeoreferencingTool. The GeoreferencingTool
	 * takes care of reactivating exec().
	 */
	int exec();
	
	/**
	 * Sets all values in the dialog's widgets to the values from the
	 * given Georeferencing object.
	 */
	void setValuesFrom(Georeferencing* values);
	
	
public slots:
	/**
	 * Triggers an online request for the magnetic declination.
	 * 
	 * @param no_confirm If true, the user will not be asked for confirmation.
	 */
	void requestDeclination(bool no_confirm = false);
	
	/**
	 * Sets the map coordinates of the reference point
	 */
	void setMapRefPoint(MapCoord coords);
	
	/**
	 * Activates the "keep projected reference point coordinates on CRS changes" radio button.
	 */
	void setKeepProjectedRefCoords();
	
	/**
	 * Activates the "keep geographic reference point coordinates on CRS changes" radio button.
	 */
	void setKeepGeographicRefCoords();
	
	/**
	 * Notifies the dialog that the active GeoreferencingTool was deleted.
	 */
	void toolDeleted();
	
	/**
	 * Opens this dialog's help page.
	 */
	void showHelp();
	
	/** 
	 * Resets all input fields to the values in the map's Georeferencing.
	 * 
	 * This will also reset initial values passed to the constructor.
	 */
	void reset();
	
	/** 
	 * Pushes the changes from the dialog to the map's Georeferencing
	 * and closes the dialog. The dialog's result is set to QDialog::Accepted,
	 * and the active exec() function will return.
	 */
	void accept();
	
protected slots:
	
	/**
	 * Updates enabled / disabled states of all widgets.
	 */
	void updateWidgets();
	
	/**
	 * Updates enabled / disabled state and text of the declination query button.
	 */
	void updateDeclinationButton();
	
	/** 
	 * Notifies the dialog of a change in the CRS selector widget.
	 */
	void crsEdited(bool system_changed);
	
	/** Calls crsEdited(true). */
	void crsSystemEdited();
	
	/**
	 * Hides the dialog and activates a GeoreferencingTool for selecting
	 * the reference point on the map.
	 */
	void selectMapRefPoint();
	
	/**
	 * Notifies the dialog of a change in the map reference point fields.
	 */
	void mapRefChanged(double value);
	
	/**
	 * Notifies the dialog of a change in the easting / northing fields.
	 */
	void eastingNorthingChanged(double value = -1);
	
	/**
	 * Notifies the dialog of a change in the latitude / longitude fields.
	 * update_zone determines if updateZone() should also be called (which
	 * is necessary for UTM zone changes due to changed coordinates, but may be
	 * unwanted).
	 */
	void latLonChanged(bool update_zone);
	
	/** Variant of latLonChanged() for use with widget signal, assuming update_zone = true. */
	void latLonChanged(double value = -1);
	
	/** 
	 * Notifies the dialog of a change in the declination field.
	 */
	void declinationChanged(double value);
	
	/**
	 * Handles replies from the online declination service.
	 */
	void declinationReplyFinished(QNetworkReply* reply);
	
protected:
	/**
	 * Dialog initialization common to all constructors.
	 */
	void init(const Georeferencing* initial);
	
	// Helper methods for handling value changes
	
	/// Updates the zone field in the dialog from the underlying Georeferencing if
	/// UTM is used as coordinate reference system.
	void updateZone();
	
	/// Updates the grivation field from the underlying Georeferencing.
	void updateNorth();
	
	/// Sets map reference point widget values from the given Georeferencing object
	void setMapRefValuesFrom(Georeferencing* values);
	
	/// Sets easting / northing widget values from the given Georeferencing object
	void setEastingNorthingValuesFrom(Georeferencing* values);
	
	/// Sets latitude / longitude widget values from the given Georeferencing object
	void setLatLonValuesFrom(Georeferencing* values);
	
private:
	/* Internal state */
	MapEditorController* const controller;
	Map* const map;
	QScopedPointer<Georeferencing> georef;
	const Georeferencing* initial_georef;
	bool allow_no_georeferencing;
	bool tool_active;
	bool declination_query_in_progress;
	
	/* GUI elements */
	ProjectedCRSSelector* crs_edit;
	QLabel* crs_spec_label;
	QLineEdit* crs_spec_edit;
	QLabel* status_label;
	QLabel* status_display_label;
	
	QDoubleSpinBox* map_x_edit;
	QDoubleSpinBox* map_y_edit;
	QPushButton* ref_point_button;
	
	QLabel* projected_ref_label;
	QDoubleSpinBox* easting_edit;
	QDoubleSpinBox* northing_edit;
	
	QDoubleSpinBox* lat_edit;
	QDoubleSpinBox* lon_edit;
	QLabel* show_refpoint_label;
	QLabel* link_label;
	
	QRadioButton* keep_projected_radio;
	QRadioButton* keep_geographic_radio;
	
	QDoubleSpinBox* declination_edit;
	QPushButton* declination_button;
	QLabel* grivation_label;
	
	QDialogButtonBox* buttons_box;
	QPushButton* reset_button;
};



/** 
 * GeoreferencingTool is a helper to the GeoreferencingDialog which allows 
 * the user to select the position of the reference point on the map 
 * The GeoreferencingDialog hides when it activates this tool. The tool
 * takes care of reactivating the dialog.
 */
class GeoreferencingTool : public MapEditorTool
{
Q_OBJECT
public:
	/** 
	 * Constructs a new tool for the given dialog and controller.
	 */
	GeoreferencingTool(GeoreferencingDialog* dialog, MapEditorController* controller, QAction* action = NULL);
	
	/**
	 * Notifies the dialog that the tool is deleted.
	 */
	virtual ~GeoreferencingTool();
	
	/**
	 * Activates the tool.
	 */
	void init();
	
	/** 
	 * Reacts to the user activity by sending the reference point
	 * coordinates to the dialog and reactivating the dialog.
	 */
	bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	/**
	 * Returns the mouse cursor that will be shown when the tool is active.
	 */
	QCursor* getCursor() { return cursor; }
	
private:
	GeoreferencingDialog* const dialog;
	
	static QCursor* cursor;
};



/// Combobox for projected coordinate reference system (CRS) selection,
/// with a QLineEdit below to specify one parameter if necessary
class ProjectedCRSSelector : public QWidget
{
Q_OBJECT
public:
	ProjectedCRSSelector(QWidget* parent = NULL);
	
	/// Adds a custom text item at the top which can be identified by the given id.
	void addCustomItem(const QString& text, int id);
	
	
	/// Returns the selected CRS template,
	/// or NULL if a custom item is selected
	CRSTemplate* getSelectedCRSTemplate();
	
	/// Returns the selected CRS specification string,
	/// or an empty string if a custom item is selected
	QString getSelectedCRSSpec();
	
	/// Returns the id of the selected custom item,
	/// or -1 in case a normal item is selected
	int getSelectedCustomItemId();
	

	/// Selects the given item
	void selectItem(CRSTemplate* temp);
	
	/// Selects the given item
	void selectCustomItem(int id);
	
	
	/// Returns the number of parameters shown currently
	int getNumParams();
	
	/// Returns the i-th parameters' value (for storage,
	/// not for pasting into the crs specification!)
	QString getParam(int i);
	
	/// Sets the i-th parameters' value.
	/// Does not emit crsEdited().
	void setParam(int i, const QString& value);
	
signals:
	/// Called when the user edit the CRS.
	/// system_changed is true if the whole system was switched,
	/// if only a parameter was changed it is false.
	void crsEdited(bool system_changed);
	
private slots:
	void crsDropdownChanged(int index);
	void crsParamEdited(QString dont_use);
	
private:
	QComboBox* crs_dropdown;
	int num_custom_items;
	
	QLineEdit* param_edit;
	QSpinBox* param_int_spinbox;
	
	QFormLayout* layout;
};



/// Dialog to select a coordinate reference system (CRS)
class SelectCRSDialog : public QDialog
{
Q_OBJECT
public:
	SelectCRSDialog(Map* map, QWidget* parent, bool show_take_from_map,
					bool show_local, bool show_geographic, const QString& desc_text = QString());
	
	QString getCRSSpec() const;
	
private slots:
	void crsSpecEdited(QString text);
	void updateWidgets();
	
private:
	/* Internal state */
	Map* const map;
	
	/* GUI elements */
	QRadioButton* map_radio;
	QRadioButton* local_radio;
	QRadioButton* geographic_radio;
	QRadioButton* projected_radio;
	QRadioButton* spec_radio;
	ProjectedCRSSelector* crs_edit;
	QFormLayout* crs_spec_layout;
	QLineEdit* crs_spec_edit;
	QLabel* status_label;
	QDialogButtonBox* button_box;
};

#endif
