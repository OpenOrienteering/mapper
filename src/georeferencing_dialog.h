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
	 * properties of the map's georeferencing.
	 */
	GeoreferencingDialog(MapEditorController* controller, const Georeferencing* initial = NULL);
	
	/**
	 * Constructs a new georeferencing dialog for the given map. The optional 
	 * parameter initial allows to override the current properties of the map's
	 * georeferencing. Since the dialog will not know a MapEditorController,
	 * it will not allow to select a new reference point from the map.
	 */
	GeoreferencingDialog(QWidget* parent, Map* map, const Georeferencing* initial = NULL);
	
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
	void setRefPoint(MapCoord coords);
	
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
	 * Handles replies from the online declination service.
	 */
	void declinationReplyFinished(QNetworkReply* reply);
	
	/** 
	 * Notifies the dialog of a change in the declination field.
	 */
	void declinationChanged(double value);
	
	/** 
	 * Notifies the dialog of a change in the grivation field.
	 */
	void grivationChanged(double value);
	
	/**
	 * Hides the dialog and activates a GeoreferencingTool for selecting
	 * the reference point on the map.
	 */
	void selectRefPoint();
	
	/**
	 * Notifies the dialog of a change in the crs or zone field.
	 */
	void crsChanged();
	
	/** 
	 * Notifies the dialog of a change in the easting or northing field
	 * (projected coordinates).
	 */
	void eastingNorthingChanged();
	
	/**
	 * Notifies the dialog of a change in the latitude or longitude field
	 */
	void latLonChanged();
	
protected:
	/**
	 * Dialog initialization common to all constructors.
	 */
	void init(const Georeferencing* initial);
	
	/** 
	 * Updates the general field group in the dialog from the underlying Georeferencing. 
	 */
	void updateGeneral();
	
	/** 
	 * Updates the CRS field in the dialog from the underlying Georeferencing. 
	 */
	void updateCRS();
	
	/** 
	 * Updates the zone field in the dialog from the underlying Georeferencing.  
	 * 
	 * This will also show, hide and clear the zone field as neccessary, and 
	 * try to determine a default value if empty.
	 */
	void updateZone();
	
	/**
	 * Updates the error status field.
	 */
	void updateStatus();
	
	/** 
	 * Updates the easting and northing fields (projected coordinates) in the 
	 * dialog from the underlying Georeferencing.
	 */
	void updateEastingNorthing();
	
	/**
	 * Updates the magnetic convergence field from the underlying Georeferencing.
	 */
	void updateNorth();
	
	/** 
	 * Updates the latitude and longitude fields (geographic coordinates) in the
	 * dialog from the underlying Georeferencing. 
	 */
	void updateLatLon();
	
private:
	/* Internal state */
	MapEditorController* const controller;
	Map* const map;
	QScopedPointer<Georeferencing> georef;
	QString crs_spec_template;
	bool tool_active;
	
	/* GUI elements */
	QLabel* scale_edit;
	QDoubleSpinBox* declination_edit;
	QPushButton* declination_button;
	QDoubleSpinBox* grivation_edit;
	QLabel* ref_point_edit;
	
	QComboBox* crs_edit;
	QLineEdit* zone_edit;
	QLabel* status_label;
	QDoubleSpinBox* easting_edit;
	QDoubleSpinBox* northing_edit;
	QLabel* convergence_edit;
	
	QComboBox* ll_datum_edit;
	QDoubleSpinBox* lat_edit;
	QDoubleSpinBox* lon_edit;
	
	QLabel* link_label;
	
	QPushButton* reset_button;
	
	enum ChangeType
	{
		NONE,
		PROJECTED,
		GEOGRAPHIC
	};
	ChangeType changed_coords;
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



/// A template for a coordinate reference system specification string,
/// which may contain one or more parameters described by the Param struct.
/// For each param, spec_template must contain a free parameter for QString::arg(),
/// e.g. "%1" for the first parameter.
class CRSTemplate
{
public:
	struct Param
	{
		Param(const QString& desc);
		virtual ~Param() {}
		virtual QWidget* createEditWidget(QObject* edit_receiver) const = 0;
		virtual QString getValue(QWidget* edit_widget) const = 0;
		
		QString desc;
	};
	
	struct ZoneParam : public Param
	{
		ZoneParam(const QString& desc);
		virtual QWidget* createEditWidget(QObject* edit_receiver) const;
		virtual QString getValue(QWidget* edit_widget) const;
	};
	
	struct IntRangeParam : public Param
	{
		IntRangeParam(const QString& desc, int min_value, int max_value, int apply_factor = 1);
		virtual QWidget* createEditWidget(QObject* edit_receiver) const;
		virtual QString getValue(QWidget* edit_widget) const;
		
		int min_value;
		int max_value;
		int apply_factor;
	};
	
	/// Creates a new CRS template with the given id (name) and spec template
	CRSTemplate(const QString& id, const QString& spec_template);
	~CRSTemplate();
	
	void addParam(Param* param);
	
	inline const QString& getId() const {return id;}
	inline const QString& getSpecTemplate() const {return spec_template;}
	inline int getNumParams() const {return (int)params.size();}
	inline const Param& getParam(int index) const {return *params[index];}
	
	// CRS Registry
	
	/// Returns the number of CRS templates which are registered
	static int getNumCRSTemplates();
	
	/// Returns a registered CRS template by index
	static CRSTemplate& getCRSTemplate(int index);
	
	/// Registers a CRS template
	static void registerCRSTemplate(CRSTemplate* temp);
	
private:
	QString id;
	QString spec_template;
	std::vector<Param*> params;
	
	// CRS Registry
	static std::vector<CRSTemplate*> crs_templates;
};



/// Combobox for projected coordinate reference system (CRS) selection,
/// with a QLineEdit below to specify one parameter if necessary
class ProjectedCRSSelector : public QWidget
{
Q_OBJECT
public:
	ProjectedCRSSelector(QWidget* parent = NULL);
	
	QString getSelectedCRSSpec();
	
signals:
	void crsEdited();
	
private slots:
	void crsDropdownChanged(int index);
	void crsParamEdited(QString dont_use);
	
private:
	QComboBox* crs_dropdown;
	
	QLineEdit* param_edit;
	QSpinBox* param_int_spinbox;
	
	QFormLayout* layout;
};



/// Dialog to select a coordinate reference system (CRS)
class SelectCRSDialog : public QDialog
{
Q_OBJECT
public:
	SelectCRSDialog(Map* map, QWidget* parent, const QString& desc_text = QString());
	
	QString getCRSSpec() const;
	
private slots:
	void crsSpecEdited(QString text);
	void updateWidgets();
	
private:
	/* Internal state */
	Map* const map;
	
	/* GUI elements */
	QRadioButton* map_radio;
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
