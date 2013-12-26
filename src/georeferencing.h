/*
 *    Copyright 2012, 2013 Kai Pastor
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


#ifndef _OPENORIENTEERING_GEOREFERENCING_H_
#define _OPENORIENTEERING_GEOREFERENCING_H_

#include <QPointF>
#include <QString>
#include <QTransform>

#include "map_coord.h"
#include "file_format.h"

class QDebug;
class QXmlStreamReader;
class QXmlStreamWriter;

typedef void* projPJ;

class GPSProjectionParameters;

/**
 * Specifies geographic coordinates by latitude and longitude.
 */
class LatLon
{
public:
	/** 
	 * Latitude is a geographic coordinate that specifies the north-south
	 * position (φ, phi). The unit is radiant.
	 */
	double latitude;
	
	/**
	 * Longitude is a geographic coordinate that specifies the east-west 
	 * position (λ, lambda). The unit is radiant.
	 */
	double longitude;
	
	/**
	 * Constructs a new LatLon with latitude and longitude set to zero.
	 */
	LatLon() : latitude(0.0), longitude(0.0) { };
	
	/**
	 * Constructs a new LatLon for the latitude and longitude.
	 * If given_in_degrees is true, the parameters' unit is degree, otherwise 
	 * the unit is radiant.
	 */
	LatLon(double latitude, double longitude, bool given_in_degrees = false)
	: latitude(latitude), longitude(longitude)
	{
		if (given_in_degrees)
		{
			this->latitude = latitude * M_PI / 180;
			this->longitude = longitude * M_PI / 180;
		}
	}
	
	/**
	 * Returns the latitude value in degrees.
	 */
	inline double getLatitudeInDegrees() const  { return latitude * 180.0 / M_PI; }
	
	/**
	 * Returns the longitude value in degrees.
	 */
	inline double getLongitudeInDegrees() const { return longitude * 180.0 / M_PI; }
	
	/**
	 * Returns true if this object has exactly the same latitude and longitude
	 * like another. FP precision issues are not taken into account.
	 */
	bool operator==(const LatLon& rhs) const
	{
		return (this->latitude == rhs.latitude) && (this->longitude == rhs.longitude);
	}
	
	/**
	 * Returns true if this object has not exactly the same latitude and longitude
	 * like another. FP precision issues are not taken into account.
	 */
	bool operator!=(const LatLon& rhs) const
	{
		return (this->latitude != rhs.latitude) || (this->longitude != rhs.longitude);
	}
	
private:
	/**
	 * @deprecated GPSProjectionParameters is no longer used.
	 * This constructor is left for historical reasons.
	 * Implementation in gps_coordinates.cpp.
	 */
	LatLon(MapCoordF map_coord, const GPSProjectionParameters& params);
	
	/**
	 * @deprecated GPSProjectionParameters is no longer used.
	 * This method is left for historical reasons.
	 * Implementation in gps_coordinates.cpp.
	 */
	MapCoordF toMapCoordF(const GPSProjectionParameters& params) const;
	/**
	 * @deprecated GPSProjectionParameters is no longer used.
	 * This method is left for historical reasons.
	 * Implementation in gps_coordinates.cpp.
	 */
	void toCartesianCoordinates(const GPSProjectionParameters& params, double height, double& x, double& y, double& z);
	
	/**
	 * @deprecated Not used at the moment
	 * Implementation in gps_coordinates.cpp.
	 */
	bool fromString(QString str);	// for example "53°48'33.82"N  2°07'46.38"E" or "N 48° 31.732 E 012° 08.422" or "48.52887 12.14037"
};

/**
 * Dumps a LatLon to the debug output.
 * 
 * Note that this requires a *reference*, not a pointer.
 */
QDebug operator<<(QDebug dbg, const LatLon &lat_lon);



/**
 * A Georeferencing defines a mapping between "map coordinates" (as measured on
 * paper) and coordinates in the real world. It provides functions for 
 * converting coordinates from one coordinate system to another.
 *
 * Conversions between map coordinates and "projected coordinates" (flat metric
 * coordinates in a projected coordinate reference system) are made as affine 
 * transformation based on the map scale, the grivation and a defined 
 * reference point.
 * 
 * Conversions between projected coordinates and geographic coordinates (here:
 * latitude/longitude for the WGS84 datum) are made based on a specification
 * of the coordinate reference system of the projected coordinates. The actual
 * geographic transformation is done by the PROJ.4 library for geographic 
 * projections. 
 *
 * If no (valid) specification is given, the projected coordinates are regarded
 * as local coordinates. Local coordinates cannot be converted to other 
 * geographic coordinate systems. The georeferencing is "local".
 * 
 * Conversions between "map coordinates" and "geographic coordinates" use the
 * projected coordinates as intermediate step.
 */
class Georeferencing : public QObject
{
Q_OBJECT

friend class NativeFileImport;
friend QDebug operator<<(QDebug dbg, const Georeferencing& georef);

public:
	/// Georeferencing state
	enum State
	{
		/// The settings have not been set yet. Only the scale denominator is valid.
		ScaleOnly = 0,
		
		/// Only conversions between map and projected coordinates are possible.
		Local,
		
		/// All coordinate conversions are possible (if there is no error in the
		/// crs specification).
		Normal
	};
	
	/** 
	 * Constructs a scale-only georeferencing.
	 */
	Georeferencing();
	
	/** 
	 * Constructs a georeferencing which is a copy of an existing georeferencing.
	 * 
	 * Note: Since QObjects may not be copied, this is better understood as
	 * creating a new object with the same settings.
	 */
	Georeferencing(const Georeferencing& georeferencing);
	
	/** 
	 * Cleans up memory allocated by the georeferencing 
	 */
	~Georeferencing();
	
	
	/** 
	 * Saves the georeferencing to an XML stream.
	 */
	void save(QXmlStreamWriter& xml) const;
	
	/**
	 * Creates a georeferencing from an XML stream.
	 */
	void load(QXmlStreamReader& xml, bool load_scale_only) throw (FileFormatException);
	
	
	/**
	 * Assigns the properties of another Georeferencing to this one.
	 * 
	 * Having this method in a QObject is in contradiction to Qt conventions.
	 * But we really need to assign properties from one object to another,
	 * where each object maintains its own identity. 
	 */
	Georeferencing& operator=(const Georeferencing& other);
	
	
	/**
	 * Returns if the georeferencing settings are valid.
	 * 
	 * This means that coordinates can be converted between map and projected
	 * coordinates. isLocal() can be checked to determine if a conversion
	 * to geographic coordinates is also possible.
	 */
	inline bool isValid() const {return state == Local || projected_crs != NULL;}
	
	/** 
	 * Returns true if this georeferencing is local.
	 * 
	 * A georeferencing is local if no (valid) coordinate system specification
	 * is given for the projected coordinates. A local georeferencing cannot 
	 * convert coordinates from and to geographic coordinate systems.
	 */
	inline bool isLocal() const { return state == Local; }
	
	
	/**
	 * Returns the georeferencing state.
	 */
	inline State getState() const {return state;}
	
	/**
	 * Sets the georeferencing state.
	 * 
	 * This is only neccessary to decrease the state to Local or ScaleOnly,
	 * as otherwise it will be automatically changed when setting the respective values.
	 */
	void setState(State value);
	
	
	/**
	 * Returns the map scale denominator.
	 */
	inline unsigned int getScaleDenominator() const { return scale_denominator; }
	
	/**
	 * Sets the map scale denominator.
	 */
	void setScaleDenominator(int value);
	
	
	/**
	 * Returns the magnetic declination (in degrees).
	 * 
	 * @see setDeclination()
	 */
	inline double getDeclination() const { return declination; }
	
	/**
	 * Sets the magnetic declination (in degrees).
	 * 
	 * Magnetic declination is the angle between magnetic north and true north.
	 * In the context of OpenOrienteering Mapper, it is the angle between the 
	 * y axis of map coordinates and the latitude axis of geographic 
	 * coordinates.
	 * 
	 * This will also affect the grivation value and the transformations.
	 */
	void setDeclination(double declination);
	
	
	/**
	 * Returns the grivation.
	 * 
	 * @see setGrivation()
	 */
	inline double getGrivation() const { return grivation; }
	
	/**
	 * Sets the grivation (in degrees).
	 * 
	 * Grivation is the angle between magnetic north and grid north. 
	 * In the context of OpenOrienteering Mapper, it is the angle between the y
	 * axes of map coordinates and projected coordinates.
	 * 
	 * This will also affect the declination value and the transformations.
	 */
	void setGrivation(double grivation);
	
	
	/**
	 * Returns the map coordinates of the reference point.
	 */
	inline MapCoord getMapRefPoint() const { return map_ref_point; };
	
	/**
	 * Defines the map coordinates of the reference point.
	 * 
	 * This will <b>not</b> update the map and geographic coordinates of the reference point.
	 */
	void setMapRefPoint(MapCoord point);
	
	
	/**
	 * Returns the projected coordinates of the reference point.
	 */
	inline QPointF getProjectedRefPoint() const { return projected_ref_point; };
	
	/**
	 * Defines the projected coordinates of the reference point.
	 * 
	 * This may trigger changes of the geographic coordinates of the reference
	 * point, the convergence, the grivation and the transformations.
	 */
	void setProjectedRefPoint(QPointF point);
	
	
	/**
	 * Returns the unique id of the projected CRS.
	 */
	QString getProjectedCRSId() const { return projected_crs_id; }
	
	/** 
	 * Returns the name of the coordinate reference system (CRS) of the
	 * projected coordinates.
	 */
	QString getProjectedCRSName() const;
	
	/**
	 * Returns the array of projected crs parameter values.
	 */
	std::vector< QString > getProjectedCRSParameters() const { return projected_crs_parameters; }
	
	/**
	 * Sets the array of projected crs parameter values.
	 */
	void setProjectedCRSParameters(std::vector< QString > values) { projected_crs_parameters = values; }
	
	/** 
	 * Returns the specification of the coordinate reference system (CRS) of the
	 * projected coordinates
	 * @return a PROJ.4 specification of the CRS
	 */
	QString getProjectedCRSSpec() const { return projected_crs_spec; }
	
	/** Sets the coordinate reference system (CRS) of the projected coordinates.
	 * 
	 * This may trigger changes of the projected coordinates of the reference
	 * point, the convergence, the grivation and the transformations.
	 * 
	 * @param spec the PROJ.4 specification of the CRS
	 * @return true if the specification is valid, false otherwise 
	 */
	bool setProjectedCRS(const QString& id, const QString& spec);
	
	/**
	 * Calculates the meridian convergence at the reference point.
	 * 
	 * The meridian convergence is the angle between grid north and true north.
	 * 
	 * @return zero for a local georeferencing, or a calculated approximation
	 */
	double getConvergence() const;
	
	
	/**
	 * Returns the geographic coordinates of the reference point.
	 */
	inline LatLon getGeographicRefPoint() const { return geographic_ref_point; };
	
	/**
	 * Defines the geographic coordinates of the reference point.
	 * 
	 * This may trigger changes of the projected coordinates of the reference
	 * point, the convergence, the grivation and the transformations.
	 */
	void setGeographicRefPoint(LatLon lat_lon);
	
	
	/** 
	 * Transforms map (paper) coordinates to projected coordinates.
	 */
	QPointF toProjectedCoords(const MapCoord& map_coords) const;
	
	/**
	 * Transforms map (paper) coordinates to projected coordinates.
	 */
	QPointF toProjectedCoords(const MapCoordF& map_coords) const;
	
	/**
	 * Transforms projected coordinates to map (paper) coordinates.
	 */
	MapCoord toMapCoords(const QPointF& projected_coords) const;
	
	/**
	 * Transforms projected coordinates to map (paper) coordinates.
	 */
	MapCoordF toMapCoordF(const QPointF& projected_coords) const;
	
	
	/**
	 * Transforms map (paper) coordinates to geographic coordinates (lat/lon).
	 */
	LatLon toGeographicCoords(const MapCoordF& map_coords, bool* ok = 0) const;
	
	/**
	 * Transforms CRS coordinates to geographic coordinates (lat/lon).
	 */
	LatLon toGeographicCoords(const QPointF& projected_coords, bool* ok = 0) const;
	
	/**
	 * Transforms geographic coordinates (lat/lon) to CRS coordinates.
	 */
	QPointF toProjectedCoords(const LatLon& lat_lon, bool* ok = 0) const;
	
	/**
	 * Transforms geographic coordinates (lat/lon) to map coordinates.
	 */
	MapCoord toMapCoords(const LatLon& lat_lon, bool* ok = NULL) const;
	
	/**
	 * Transforms geographic coordinates (lat/lon) to map coordinates.
	 */
	MapCoordF toMapCoordF(const LatLon& lat_lon, bool* ok = NULL) const;
	
	
	/**
	 * Transforms map coordinates from the other georeferencing to
	 * map coordinates of this georeferencing, if possible. 
	 */
	MapCoordF toMapCoordF(Georeferencing* other, const MapCoordF& map_coords, bool* ok = NULL) const;
	
	
	/**
	 * Returns the current error text.
	 */
	QString getErrorText() const;
	
	/**
	 * Converts a value from radians to degrees.
	 */
	static double radToDeg(double val);
	
	/**
	 * Converts a value from radians to a D°M'S" string.
	 */
	static QString radToDMS(double val);
	
	
	/**
	 * Updates the transformation parameters between map coordinates and 
	 * projected coordinates from the current projected reference point 
	 * coordinates, the grivation and the scale.
	 */
	void updateTransformation();
	
	/**
	 * Updates the grivation. 
	 * 
	 * The new value is calculated from the declination and the convergence.
	 * For a local georeferencing, the convergence is zero, and grivation
	 * is set to the same value as declination.
	 * 
	 * If the grivation changes, it is neccessary to call updateTransformation().
	 * 
	 * @return true if grivation changed, false otherwise.
	 */
	bool updateGrivation();
	
	/**
	 * Initializes the declination.
	 * 
	 * The new value is calculated from the grivation and the convergence.
	 * For a local georeferencing, the convergence is zero, and declination
	 * is set to the same value as grivation.
	 * 
	 * This method intended for import of Version 17 OMAP files. It tries to
	 * initialize the internal projection first if it is not yet defined.
	 */
	void initDeclination();
	
	/**
	 * Sets the transformation matrix from map coordinates to projected
	 * coordinates directly. 
	 */
	void setTransformationDirectly(const QTransform& transform);
	
	
signals:
	/**
	 * Indicates a change to the transformation rules between map coordinates
	 * and projected coordinates.
	 */
	void transformationChanged();
	
	/**
	 * Indicates a change to the projection rules between geographic coordinates
	 * and projected coordinates. This signal is also emitted when the 
	 * georeferencing becomes local.
	 */
	void projectionChanged();
	
	
private:
	State state;
	
	unsigned int scale_denominator;
	double declination;
	double grivation;
	MapCoord map_ref_point;
	
	QPointF projected_ref_point;
	
	QTransform from_projected;
	QTransform to_projected;
	
	/// Projected CRS id, used for lookup in the CRS registry.
	/// If the spec is directly entered as string, the id is empty.
	QString projected_crs_id;
	QString projected_crs_spec;
	std::vector< QString > projected_crs_parameters;
	projPJ projected_crs;
	
	LatLon geographic_ref_point;
	
	projPJ geographic_crs;
	
	static const QString geographic_crs_spec;
};

/**
 * Dumps a Georeferencing to the debug output.
 * 
 * Note that this requires a *reference*, not a pointer.
 */
QDebug operator<<(QDebug dbg, const Georeferencing &georef);



/**
 * A template for a coordinate reference system specification string,
 * which may contain one or more parameters described by the Param struct.
 * For each param, spec_template must contain a free parameter for QString::arg(),
 * e.g. "%1" for the first parameter.
 */
class CRSTemplate
{
public:
	/** Abstract base class for parameters in CRSTemplates. */
	struct Param
	{
		Param(const QString& desc);
		virtual ~Param() {}
		/** Must create a widget which can be used to edit the value. */
		virtual QWidget* createEditWidget(QObject* edit_receiver) const = 0;
		/** Must return the widget's value in a form so it can be pasted into
		 *  the CRS specification
		 */
		virtual QString getSpecValue(QWidget* edit_widget) const = 0;
		/** Must return the widget's value in a form so it can be stored */
		virtual QString getValue(QWidget* edit_widget) const = 0;
		/** Must set the stored value in the widget */
		virtual void setValue(QWidget* edit_widget, const QString& value) = 0;
		
		QString desc;
	};
	
	/** CRSTemplate parameter specifying a zone. */
	struct ZoneParam : public Param
	{
		ZoneParam(const QString& desc);
		virtual QWidget* createEditWidget(QObject* edit_receiver) const;
		virtual QString getSpecValue(QWidget* edit_widget) const;
		virtual QString getValue(QWidget* edit_widget) const;
		virtual void setValue(QWidget* edit_widget, const QString& value);
	};
	
	/** CRSTemplate integer parameter, with values from an integer range. */
	struct IntRangeParam : public Param
	{
		IntRangeParam(const QString& desc, int min_value, int max_value, int apply_factor = 1);
		virtual QWidget* createEditWidget(QObject* edit_receiver) const;
		virtual QString getSpecValue(QWidget* edit_widget) const;
		virtual QString getValue(QWidget* edit_widget) const;
		virtual void setValue(QWidget* edit_widget, const QString& value);
		
		int min_value;
		int max_value;
		int apply_factor;
	};
	
	/**
	 * Creates a new CRS template.
	 * The id must be unique and different from "Local".
	 */
	CRSTemplate(const QString& id, const QString& name,
				const QString& coordinates_name, const QString& spec_template);
	~CRSTemplate();
	
	/**
	 * Adds a parameter to this template.
	 * A corresponding "%x" (%0, %1, ...) entry must exist in the spec template,
	 * where the parameter value will be pasted using QString.arg() when
	 * applying the CRSTemplate.
	 */
	void addParam(Param* param);
	
	/** Returns the unique ID of this template. */
	inline const QString& getId() const {return id;}
	/** Returns the user-visible name of this template. */
	inline const QString& getName() const {return name;}
	/**
	 * Returns the name for the coordinates of this template, e.g.
	 * "UTM coordinates".
	 */
	inline const QString& getCoordinatesName() const {return coordinates_name;}
	/** Returns the specification string template in Proj.4 format. */
	inline const QString& getSpecTemplate() const {return spec_template;}
	/** Returns the number of free parameters in this template. */
	inline int getNumParams() const {return (int)params.size();}
	/** Returns a reference to the i-th parameter. */
	inline Param& getParam(int index) {return *params[index];}
	
	// CRS Registry
	
	/** Returns the number of CRS templates which are registered */
	static int getNumCRSTemplates();
	
	/** Returns a registered CRS template by index */
	static CRSTemplate& getCRSTemplate(int index);
	
	/**
	 * Returns a registered CRS template by id,
	 * or NULL if the given id does not exist
	 */
	static CRSTemplate* getCRSTemplate(const QString& id);
	
	/** Registers a CRS template */
	static void registerCRSTemplate(CRSTemplate* temp);
	
private:
	QString id;
	QString name;
	QString coordinates_name;
	QString spec_template;
	std::vector<Param*> params;
	
	// CRS Registry
	static std::vector<CRSTemplate*> crs_templates;
};

#endif
