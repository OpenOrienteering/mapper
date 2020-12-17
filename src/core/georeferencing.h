/*
 *    Copyright 2012-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_GEOREFERENCING_H
#define OPENORIENTEERING_GEOREFERENCING_H

#include <cmath>
#include <vector>

#include <QObject>
#include <QPointF>
#include <QString>
#include <QTransform>

#include "core/latlon.h"
#include "core/map_coord.h"

class QDebug;
class QXmlStreamReader;
class QXmlStreamWriter;
// IWYU pragma: no_forward_declare QPointF

#ifdef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
using ProjTransformData = void;
#else
using ProjTransformData = struct PJconsts;
#endif

namespace OpenOrienteering {


/**
 * A utility which encapsulates PROJ API variants and resource management.
 */
struct ProjTransform
{
	ProjTransform() noexcept = default;
	ProjTransform(const ProjTransform&) = delete;
	ProjTransform(ProjTransform&& other) noexcept;
	ProjTransform(const QString& crs_spec);
	~ProjTransform();
	
	ProjTransform& operator=(const ProjTransform& other) = delete;
	ProjTransform& operator=(ProjTransform&& other) noexcept;
	
	/// Create a PROJ CRS object.
	static ProjTransform crs(const QString& crs_spec);
	
	bool isValid() const noexcept;
	bool isGeographic() const;
	
	QPointF forward(const LatLon& lat_lon, bool* ok, double epoch = HUGE_VAL) const;
	LatLon inverse(const QPointF& projected, bool* ok) const;
	
	QString errorText() const;
	
private:
	ProjTransform(ProjTransformData* pj) noexcept;
	
	ProjTransformData* pj = nullptr;
	
};



/**
 * A Georeferencing defines a mapping between "map coordinates" (as measured on
 * paper) and coordinates in the real world. It provides functions for 
 * converting coordinates from one coordinate system to another.
 *
 * Conversions between map coordinates and "projected coordinates" (flat metric
 * coordinates in a projected coordinate reference system) are made as affine 
 * transformation based on the map scale (principal scale), a scale factor
 * combining grid scale and elevation, the grivation and a defined reference
 * point.
 * 
 * Conversions between projected coordinates and geographic coordinates (here:
 * latitude/longitude for the WGS84 datum) are made based on a specification
 * of the coordinate reference system of the projected coordinates. The actual
 * geographic transformation is done by the PROJ library for geographic 
 * projections. 
 * 
 * Conversions between "map coordinates" and "geographic coordinates" use the
 * projected coordinates as intermediate step.
 *
 * If no geospatial specification of the projected coordinate references system
 * is given, the projected coordinates are regarded as "local coordinates". The
 * georeferencing is "local".
 * 
 * If the georeferencing is local, or if the specification of the coordinate
 * reference system is invalid, coordinates cannot be converted to other
 * geospatial coordinate systems.
 */
class Georeferencing : public QObject  // clazy:exclude=copyable-polymorphic
{
Q_OBJECT

friend QDebug operator<<(QDebug dbg, const Georeferencing& georef);

public:
	/// Georeferencing state
	enum State
	{
		/// Only conversions between map and local projected coordinates are possible.
		Local,
		
		/// Only conversions between map and local projected coordinates are possible,
		/// because the given projected CRS is not usable.
		BrokenGeospatial,
		
		/// All coordinate conversions are possible.
		Geospatial,
	};
	
	
	/**
	 * A shared PROJ specification of a WGS84 geographic CRS.
	 */
	static const QString geographic_crs_spec;
	static const QString legacy_geographic_crs_spec;
	
	
	/**
	 * @brief Returns the precision of the grid scale factor.
	 * 
	 * The precision is given in number of decimal places,
	 * i.e. digits after the decimal point.
	 */
	static constexpr unsigned int scaleFactorPrecision();
	
	/**
	 * @brief Rounds according to the defined precision of the grid scale factor.
	 * 
	 * @see scaleFactorPrecision();
	 */
	static double roundScaleFactor(double value);
	
	
	/**
	 * @brief Returns the precision of declination/grivation/convergence.
	 * 
	 * The precision is given in number of decimal places,
	 * i.e. digits after the decimal point.
	 * 
	 * All values set as declination or grivation will be rounded to this precisison.
	 */
	static constexpr unsigned int declinationPrecision();
	
	/**
	 * @brief Rounds according to the defined precision of declination/grivation/convergence.
	 * 
	 * @see declinationPrecision();
	 */
	static double roundDeclination(double);
	
	
	static double toEpoch(QDateTime datetime);
	
	
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
	Georeferencing(const Georeferencing& other);
	
	/** 
	 * Cleans up memory allocated by the georeferencing 
	 */
	~Georeferencing() override;
	
	
	/** 
	 * Saves the georeferencing to an XML stream.
	 */
	void save(QXmlStreamWriter& xml) const;
	
	/**
	 * Creates a georeferencing from an XML stream.
	 */
	void load(QXmlStreamReader& xml, bool load_scale_only);
	
	
	/**
	 * Assigns the properties of another Georeferencing to this one.
	 * 
	 * Having this method in a QObject is in contradiction to Qt conventions.
	 * But we really need to assign properties from one object to another,
	 * where each object maintains its own identity. 
	 */
	Georeferencing& operator=(const Georeferencing& other);
	
	
	/**
	 * Returns true if the "projected CRS" is actually geographic.
	 * 
	 * \see proj_angular_output(PJ *, enum PJ_DIRECTION) in PROJ
	 */
	bool isGeographic() const;
	
	
	/**
	 * Returns the georeferencing state.
	 */
	State getState() const noexcept { return state; }
	
	/**
	 * Switches the georeferencing to local state.
	 */
	void setLocalState() { setState(Georeferencing::Local); }
	
protected:
	/**
	 * Sets the georeferencing state.
	 */
	void setState(State value);
	
	
public:
	/**
	 * Returns the principal scale denominator.
	 * 
	 * The principal scale - or representative fraction - is the ratio between
	 * units on the printed map and units on ground.
	 */
	unsigned int getScaleDenominator() const;
	
	/**
	 * Sets the principal scale denominator.
	 */
	void setScaleDenominator(int value);
	
	
	/**
	 * Returns the combined scale factor.
	 *
	 * The combined factor is applied to ground distances to get grid plane
	 * distances. The combined scale factor is the ratio between a length
	 * in the grid and the length on the ground.
	 *
	 * The combined factor incoroprates the grid scale factor and the
	 * auxiliary scale factor.
	 */
	double getCombinedScaleFactor() const;
	
	/**
	 * Sets the combined scale factor.
	 *
	 * \see getCombinedScaleFactor()
	 */
	void setCombinedScaleFactor(double value);
	
	/**
	 * Returns the auxiliary scale factor.
	 * 
	 * The auxiliary scale factor is typically the elevation factor,
	 * i.e. the ratio between ground distance and distance on the ellipsoid.
	 * If the combined scale factor was set using an earlier Mapper
	 * that did not calculate the grid scale factor, then the auxiliary
	 * factor deals with the discrepancy between the distance on the
	 * map and distance on the ellipsoid.
	 * 
	 * Mapper doesn't explicitly deal with other factors (e.g. unit of
	 * measurement). Technically, this property can be used for purposes
	 * other than elevation.
	 */
	double getAuxiliaryScaleFactor() const;
	
	/**
	 * Sets the auxiliary scale factor.
	 * 
	 * \see getAuxiliaryScaleFactor()
	 */
	void setAuxiliaryScaleFactor(double value);
	
	
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
	void setDeclination(double value);
	
	
	/**
	 * Returns the grivation.
	 * 
	 * @see setGrivation()
	 */
	double getGrivation() const;
	
	/**
	 * Returns the deviation of the grivation from the one given in pre-0.6 files.
	 * 
	 * Only valid immediately after loading a georeferencing from a file.
	 * Returns 0.0 in any other context.
	 * 
	 * Files from Mapper versions before 0.6 may have used any number of decimal
	 * places for grivation. Since version 0.6, grivation is rounded to the
	 * number of decimal places defined by declinationPrecision(). When this
	 * rounding takes place (i.e. only when opening a file which has not been
	 * saved by 0.6 or later), the difference between the original value and the
	 * rounded value is temporarily provided by this function. This value can be
	 * used to for correcting dependent data. Any changes to declination or
	 * grivation will invalidate this value.
	 * 
	 * @see getGrivation()
	 * @see declinationPrecision()
	 */
	double getGrivationError() const;
	
	/**
	 * Sets the grivation (in degrees).
	 * 
	 * Grivation is the angle between magnetic north and grid north. 
	 * In the context of OpenOrienteering Mapper, it is the angle between the y
	 * axes of map coordinates and projected coordinates.
	 * 
	 * This will also affect the declination value and the transformations.
	 */
	void setGrivation(double value);
	
	
	/**
	 * Returns the map coordinates of the reference point.
	 */
	MapCoord getMapRefPoint() const;
	
	/**
	 * Defines the map coordinates of the reference point.
	 * 
	 * This will <b>not</b> update the map and geographic coordinates of the reference point.
	 */
	void setMapRefPoint(const MapCoord& point);
	
	
	/**
	 * Returns the projected coordinates of the reference point.
	 */
	QPointF getProjectedRefPoint() const;
	
	/**
	 * Defines the projected coordinates of the reference point.
	 * 
	 * This may trigger changes of the geographic coordinates of the reference
	 * point, the convergence, the grivation, the transformations, and the
	 * scale factors.
	 */
	void setProjectedRefPoint(const QPointF& point, bool update_grivation = true, bool update_scale_factor = true);
	
	
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
	 * Returns the name of the projected coordinates.
	 */
	QString getProjectedCoordinatesName() const;
	
	/**
	 * Returns the array of projected crs parameter values.
	 */
	const std::vector<QString>& getProjectedCRSParameters() const;
	
	/** 
	 * Returns the specification of the coordinate reference system (CRS) of the
	 * projected coordinates
	 * @return a PROJ specification of the CRS
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
	 * If the spec is an empty (zero-length) string, the georeferencing will
	 * be in Local state when this function returns. In all other case, it will
	 * be either in Geospatial or BrokenGeospatial state.
	 * 
	 * @param id  an identifier
	 * @param spec the PROJ specification of the CRS
	 * @param params parameter values (ignore for empty spec)
	 * @return true if the specification is valid or empty, false otherwise
	 */
	bool setProjectedCRS(const QString& id, QString spec, std::vector< QString > params = std::vector<QString>());
	
	/**
	 * Calculates the convergence at the reference point.
	 * 
	 * The convergence is the angle between grid north and true north.
	 * In case of deformation, convergence varies with direction; this is an average.
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
	 * point, the convergence, the grivation, the transformations, and the
	 * scale factors.
	 */
	void setGeographicRefPoint(LatLon lat_lon, bool update_grivation = true, bool update_scale_factor = true);
	
	
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
	QPointF toProjectedCoords(const LatLon& lat_lon, bool* ok = 0, double epoch = HUGE_VAL) const;
	
	/**
	 * Transforms geographic coordinates (lat/lon) to map coordinates.
	 */
	MapCoord toMapCoords(const LatLon& lat_lon, bool* ok = nullptr, double epoch = HUGE_VAL) const;
	
	/**
	 * Transforms geographic coordinates (lat/lon) to map coordinates.
	 */
	MapCoordF toMapCoordF(const LatLon& lat_lon, bool* ok = nullptr, double epoch = HUGE_VAL) const;
	
	
	/**
	 * Transforms map coordinates from the other georeferencing to
	 * map coordinates of this georeferencing, if possible. 
	 */
	MapCoordF toMapCoordF(const Georeferencing* other, const MapCoordF& map_coords, bool* ok = nullptr) const;
	
	
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
	 * projected coordinates. This depends on the map and projected
	 * reference point coordinates, the grivation, the scale, and the
	 * combined scale factor.
	 */
	void updateTransformation();
	
	/**
	 * Updates the combined scale factor.
	 *
	 * The new value is calculated from the grid scale factor and
	 * auxiliary scale factor, overriding the present combined_scale_factor
	 * value.
	 */
	void updateCombinedScaleFactor();
	
	/**
	 * Initializes the auxiliary scale factor.
	 *
	 * The new value is calculated from the combined scale factor
	 * and grid scale factor, overriding the present auxiliary_scale_factor
	 * value.
	 */
	void initAuxiliaryScaleFactor();
	
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
	 * Updates convergence and grid scale factor.
	 *
	 * The new values are calculated from the CRS and the geographic reference
	 * point.
	 */
	void updateGridCompensation();
	
	/**
	 * Sets the transformation matrix from map coordinates to projected
	 * coordinates directly. 
	 */
	void setTransformationDirectly(const QTransform& transform);
	
	
	const QTransform& mapToProjected() const;
	
	const QTransform& projectedToMap() const;
	
	
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
	
	/**
	 * Indicates a change of the "auxiliary scale factor", as presented in
	 * the georeferencing dialog.
	 *
	 * The "auxiliary scale factor" has no direct influence on projection
	 * or transformation. That's why there is an independent signal.
	 */
	void auxiliaryScaleFactorChanged();
	
	
private:
	void setScaleFactors(double combined_scale_factor, double auxiliary_scale_factor);
	void setDeclinationAndGrivation(double declination, double grivation);
	
	State state;
	
	unsigned int scale_denominator;
	double combined_scale_factor;
	double auxiliary_scale_factor;
	
	/**
	 * This is the scale factor from true distance to grid distance.
	 * 
	 * In case of deformation, the scale factor varies with direction and this is an average.
	 * 
	 * \see updateGridCompensation()
	 */
	double grid_scale_factor;

	double declination;
	double grivation;
	double grivation_error;
	
	/**
	 * This is the angle between true azimuth and grid azimuth.
	 * 
	 * In case of deformation, the convergence varies with direction and this is an average.
	 * 
	 * \see updateGridCompensation()
	 */
	double convergence;
	
	MapCoord map_ref_point;
	
	QPointF projected_ref_point;
	
	QTransform from_projected;
	QTransform to_projected;
	
	/// Projected CRS id, used for lookup in the CRS registry.
	/// If the spec is directly entered as string, the id is empty.
	QString projected_crs_id;
	QString projected_crs_spec;
	std::vector< QString > projected_crs_parameters;
	
	ProjTransform proj_transform;
	
	LatLon geographic_ref_point;
	
};

/**
 * Dumps a Georeferencing to the debug output.
 * 
 * Note that this requires a *reference*, not a pointer.
 */
QDebug operator<<(QDebug dbg, const Georeferencing &georef);



//### Georeferencing inline code ###

inline
constexpr unsigned int Georeferencing::scaleFactorPrecision()
{
	return 6u;
}

inline
double Georeferencing::roundScaleFactor(double value)
{
	// This must match the implementation in scaleFactorPrecision().
	return floor(value*1000000.0+0.5)/1000000.0;
}

inline
constexpr unsigned int Georeferencing::declinationPrecision()
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
unsigned int Georeferencing::getScaleDenominator() const
{
	return scale_denominator;
}

inline
double Georeferencing::getCombinedScaleFactor() const
{
	return combined_scale_factor;
}

inline
double Georeferencing::getAuxiliaryScaleFactor() const
{
	return auxiliary_scale_factor;
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
double Georeferencing::getGrivationError() const
{
	return grivation_error;
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


}  // namespace OpenOrienteering

#endif
