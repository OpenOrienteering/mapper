/*
 *    Copyright 2012, 2013, 2014 Kai Pastor
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

#include "latlon.h"
#include "../map_coord.h"

class QDebug;
class QXmlStreamReader;
class QXmlStreamWriter;

typedef void* projPJ;

class FileFormatException;



#if defined(Q_OS_ANDROID)

/**
 * Registers a file finder function needed by Proj.4 on Android.
 */
extern "C" void registerProjFileHelper();

#endif



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
	 * @brief Returns the precision of declination/grivation/convergence.
	 * 
	 * The precision is given in number of decimal places,
	 * i.e. digits after the decimal point.
	 * 
	 * All values set as declination or grivation will be rounded to this precisison.
	 */
	static unsigned int declinationPrecision();
	
	/**
	 * @brief Rounds according to the defined precision of declination/grivation/convergence.
	 * 
	 * @see declinationPrecision();
	 */
	static double roundDeclination(double);
	
	
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
	bool isValid() const;
	
	/** 
	 * Returns true if this georeferencing is local.
	 * 
	 * A georeferencing is local if no (valid) coordinate system specification
	 * is given for the projected coordinates. A local georeferencing cannot 
	 * convert coordinates from and to geographic coordinate systems.
	 */
	bool isLocal() const;
	
	
	/**
	 * Returns the georeferencing state.
	 */
	State getState() const;
	
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
	unsigned int getScaleDenominator() const;
	
	/**
	 * Sets the map scale denominator.
	 */
	void setScaleDenominator(int value);
	
	
	/**
	 * Returns the magnetic declination (in degrees).
	 * 
	 * @see setDeclination()
	 */
	double getDeclination() const;
	
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
	double getGrivation() const;
	
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
	MapCoord getMapRefPoint() const;
	
	/**
	 * Defines the map coordinates of the reference point.
	 * 
	 * This will <b>not</b> update the map and geographic coordinates of the reference point.
	 */
	void setMapRefPoint(MapCoord point);
	
	
	/**
	 * Returns the projected coordinates of the reference point.
	 */
	QPointF getProjectedRefPoint() const;
	
	/**
	 * Defines the projected coordinates of the reference point.
	 * 
	 * This may trigger changes of the geographic coordinates of the reference
	 * point, the convergence, the grivation and the transformations.
	 */
	void setProjectedRefPoint(QPointF point, bool update_grivation = true);
	
	
	/**
	 * Returns the unique id of the projected CRS.
	 */
	QString getProjectedCRSId() const;
	
	/** 
	 * Returns the name of the coordinate reference system (CRS) of the
	 * projected coordinates.
	 */
	QString getProjectedCRSName() const;
	
	/**
	 * Returns the array of projected crs parameter values.
	 */
	const std::vector<QString>& getProjectedCRSParameters() const;
	
	/** 
	 * Returns the specification of the coordinate reference system (CRS) of the
	 * projected coordinates
	 * @return a PROJ.4 specification of the CRS
	 */
	QString getProjectedCRSSpec() const { return projected_crs_spec; }
	
	/**
	 * Sets the coordinate reference system (CRS) of the projected coordinates.
	 * 
	 * This will not touch any of the reference points, the declination, the
	 * grivation. It is up to the user to decide how to reestablish a valid
	 * configuration of geographic reference point, projected reference point,
	 * declination and grivation.
	 * 
	 * @param id  an identifier
	 * @param spec the PROJ.4 specification of the CRS
	 * @param params parameter values
	 * @return true if the specification is valid, false otherwise 
	 */
	bool setProjectedCRS(const QString& id, const QString& spec = "", std::vector< QString > params = std::vector< QString >());
	
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
	LatLon getGeographicRefPoint() const;
	
	/**
	 * Defines the geographic coordinates of the reference point.
	 * 
	 * This may trigger changes of the projected coordinates of the reference
	 * point, the convergence, the grivation and the transformations.
	 */
	void setGeographicRefPoint(LatLon lat_lon, bool update_grivation = true);
	
	
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
	 * Converts a value from degrees to radians.
	 */
	static double degToRad(double val);
	
	/**
	 * Converts a value from degrees to a D°M'S" string.
	 */
	static QString degToDMS(double val);
	
	
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
	 */
	void updateGrivation();
	
	/**
	 * Initializes the declination.
	 * 
	 * The new value is calculated from the grivation and the convergence.
	 * For a local georeferencing, the convergence is zero, and declination
	 * is set to the same value as grivation.
	 */
	void initDeclination();
	
	/**
	 * Sets the transformation matrix from map coordinates to projected
	 * coordinates directly. 
	 */
	void setTransformationDirectly(const QTransform& transform);
	
	
signals:
	/**
	 * Indicates a change of the state property.
	 */
	void stateChanged();
	
	/**
	 * Indicates a change to the transformation rules between map coordinates
	 * and projected coordinates.
	 */
	void transformationChanged();
	
	/**
	 * Indicates a change to the projection rules between geographic coordinates
	 * and projected coordinates.
	 */
	void projectionChanged();
	
	/**
	 * Indicates a change of the declination.
	 * 
	 * The declination has no direct influence on projection or transformation.
	 * That's why there is an independent signal.
	 */
	void declinationChanged();
	
	
private:
	void setDeclinationAndGrivation(double declination, double grivation);
	
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



//### Georeferencing inline code ###

inline
unsigned int Georeferencing::declinationPrecision()
{
	// This must match the implementation in declinationRound().
	return 2u;
}

inline
double Georeferencing::roundDeclination(double value)
{
	// This must match the implementation in declinationPrecision().
	return floor(value*100.0+0.5)/100.0;
}

inline
bool Georeferencing::isValid() const
{
	return state == Local || projected_crs != NULL;
}

inline
bool Georeferencing::isLocal() const
{
	return state == Local;
}

inline
Georeferencing::State Georeferencing::getState() const
{
	return state;
}

inline
unsigned int Georeferencing::getScaleDenominator() const
{
	return scale_denominator;
}

inline
double Georeferencing::getDeclination() const
{
	return declination;
}

inline
double Georeferencing::getGrivation() const
{
	return grivation;
}

inline
MapCoord Georeferencing::getMapRefPoint() const
{
	return map_ref_point;
}

inline
QPointF Georeferencing::getProjectedRefPoint() const
{
	return projected_ref_point;
}

inline
QString Georeferencing::getProjectedCRSId() const
{
	return projected_crs_id;
}

inline
const std::vector<QString>& Georeferencing::getProjectedCRSParameters() const
{
	return projected_crs_parameters;
}

inline
LatLon Georeferencing::getGeographicRefPoint() const
{
	return geographic_ref_point;
}

#endif
