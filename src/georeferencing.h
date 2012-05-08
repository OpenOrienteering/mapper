/*
 *    Copyright 2012 Kai Pastor
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

#include "gps_coordinates.h"  // import LatLon, TODO: move to this file.
#include "map_coord.h"

class QDebug;

typedef void* projPJ;


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
friend QDebug operator<<(QDebug dbg, const Georeferencing& georef);

public:
	/** 
	 * Construct a local georeferencing for a particular map
	 */
	Georeferencing();
	
	/** 
	 * Construct a georeferencing which is a copy of an existing georeferencing 
	 */
	Georeferencing(const Georeferencing& georeferencing);
	
	/** 
	 * Cleanup memory allocated by the georeferencing 
	 */
	~Georeferencing();
	
	
	/**
	 * Assign the properties of another Georeferencing to this one.
	 */
	Georeferencing& operator=(const Georeferencing& other);
	
	
	/** 
	 * Returns true if this georeferencing is local.
	 * 
	 * A georeferencing is local if no (valid) coordinate system specification
	 * is given for the projected coordinates. Local georeferencing cannot 
	 * convert coordinates from and to geographic coordinate systems.
	 */
	inline bool isLocal() const { return projected_crs == NULL; }
	
	
	/**
	 * Get the map scale denominator
	 */
	inline int getScaleDenominator() const { return scale_denominator; }
	
	/**
	 * Set the map scale denominator
	 */
	void setScaleDenominator(int value);
	
	
	/**
	 * Get the grivation.
	 * 
	 * @see setGrivation()
	 */
	inline double getGrivation() const { return grivation; }
	
	/**
	 * Set the grivation.
	 * 
	 * Grivation is the angle between map north and grid north. In the context
	 * of OpenOrienteering Mapper, it is the angle between the y axes of map 
	 * coordinates and projected coordinates.
	 */
	void setGrivation(double grivation);
	
	
	/**
	 * Get the projected coordinates of the reference point
	 */
	inline QPointF getProjectedRefPoint() const { return projected_ref_point; };
	
	/**
	 * Define the projected coordinates of the reference point.
	 * 
	 * This will also update the geographic coordinates of the reference point
	 * if a conversion is possible.
	 */
	void setProjectedRefPoint(QPointF point);
	
	
	/** 
	 * Get the name (ID) of the coordinate reference system (CRS) of the projected coordinates
	 */
	QString getProjectedCRS() const { return projected_crs_id; }
	
	/** 
	 * Get the specification of the coordinate reference system (CRS) of the projected coordinates
	 * @return a PROJ.4 specification of the CRS
	 */
	QString getProjectedCRSSpec() const { return projected_crs_spec; }
	
	/** Set the coordinate reference system (CRS) of the projected coordinates 
	 * @param spec the PROJ.4 specification of the CRS
	 * @return true if the specification is valid, false otherwise 
	 */
	bool setProjectedCRS(const QString& id, const QString& spec);
	
	
	/**
	 * Get the geographic coordinates of the reference point
	 */
	inline LatLon getGeographicRefPoint() const { return geographic_ref_point; };
	
	/**
	 * Define the geographic coordinates of the reference point.
	 * 
	 * This will also update the projected coordinates of the reference point
	 * if a conversion is possible.
	 */
	void setGeographicRefPoint(LatLon lat_lon);
	
	
	/** 
	 * Transform map (paper) coordinates to projected coordinates 
	 */
	QPointF toProjectedCoords(const MapCoord& map_coords) const;
	
	/**
	 * Transform map (paper) coordinates to projected coordinates 
	 */
	QPointF toProjectedCoords(const MapCoordF& map_coords) const;
	
	/**
	 * Transform projected coordinates to map (paper) coordinates 
	 */
	MapCoord toMapCoords(const QPointF& projected_coords) const;
	
	/**
	 * Transform projected coordinates to map (paper) coordinates 
	 */
	MapCoordF toMapCoordF(const QPointF& projected_coords) const;
	
	
	/**
	 * Transform map (paper) coordinates to geographic coordinates (lat/lon) 
	 */
	LatLon toGeographicCoords(const MapCoordF& map_coords, bool* ok = 0) const;
	
	/**
	 * Transform CRS coordinates to geographic coordinates (lat/lon) 
	 */
	LatLon toGeographicCoords(const QPointF& projected_coords, bool* ok = 0) const;
	
	/**
	 * Transform geographic coordinates (lat/lon) to CRS coordinates 
	 */
	QPointF toProjectedCoords(const LatLon& lat_lon, bool* ok = 0) const;
	
	/**
	 * Transform geographic coordinates (lat/lon) to map coordinates 
	 */
	MapCoord toMapCoords(const LatLon& lat_lon, bool* ok = NULL) const;
	
	/**
	 * Transform geographic coordinates (lat/lon) to map coordinates 
	 */
	MapCoordF toMapCoordF(const LatLon& lat_lon, bool* ok = NULL) const;
	
	
	/**
	 * Convert a value from radians to degrees
	 */
	static double radToDeg(double val);
	
	/**
	 * Convert a value from radians to a D°M'S" string
	 */
	static QString radToDMS(double val);
	
	
signals:
	/**
	 * Indicates a change to the transformation rules between map coordinates
	 * and projected coordinates
	 */
	void transformationChanged();
	/**
	 * Indicates a change to the projection rules between geographic coordinates
	 * and projected coordinates. This signal is also emitted when the 
	 * georeferencing becomes local.
	 */
	void projectionChanged();
	
	
protected:
	/**
	 * Update the transformation parameters between map coordinates and 
	 * projected coordinates from the current projected reference point 
	 * coordinates, the grivation and the scale.
	 */
	void updateTransformation();
	
	
private:
	double scale_denominator;
	double grivation;
	
	QPointF projected_ref_point;
	
	QTransform from_projected;
	QTransform to_projected;
	
	QString projected_crs_id;
	QString projected_crs_spec;
	projPJ projected_crs;
	
	LatLon geographic_ref_point;
	
	projPJ geographic_crs;
	
	static const QString geographic_crs_spec;
};

/**
 * Dump a Georeferencing to the debug output
 * 
 * Note that this requires a *reference*, not a pointer.
 */
QDebug operator<<(QDebug dbg, const Georeferencing &georef);

#endif
