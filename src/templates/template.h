/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_H
#define OPENORIENTEERING_TEMPLATE_H

#include <memory>

#include <QtGlobal>
#include <QObject>
#include <QPointF>
#include <QString>
#include <vector>

#include "core/map_coord.h"
#include "util/matrix.h"
#include "util/transformation.h"

class QByteArray;
class QColor;
class QDir;
class QPainter;
class QRectF;
class QTransform;
class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class MapView;


/**
 * Transformation parameters for non-georeferenced templates.
 * 
 * The parameters are to applied in the order
 * 1. translate
 * 2. rotate
 * 3. scale.
 * 
 * So this order is also chosen for the member variables, and
 * thus used in list initalization.
 * 
 * \see Template::applyTemplateTransform()
 */
struct TemplateTransform
{
	/// x position in 1/1000 mm
	qint32 template_x = 0;
	/// x position in 1/1000 mm
	qint32 template_y = 0;
	
	/// Rotation in radians
	double template_rotation = 0.0;
	
	/// Scaling in x direction (relative to 1 mm on map)
	double template_scale_x = 1.0;
	/// Scaling in y direction (relative to 1 mm on map)
	double template_scale_y = 1.0;
	
	/**
	 * Explicit implementation of aggregate initialization.
	 * 
	 * In C++11, there is no aggregate initialization when default initalizers are present.
	 * \todo Remove when we can use C++14 everywhere.
	 */
	TemplateTransform(qint32 x, qint32 y, double rotation, double scale_x, double scale_y) noexcept
	: template_x{x}, template_y{y}, template_rotation{rotation}, template_scale_x{scale_x}, template_scale_y{scale_y}
	{}
	
	/**
	 * Explicitly defaulted default constructor.
	 * 
	 * This is needed because of the other explicit constructor.
	 * \todo Remove when we can use C++14 everywhere.
	 */
	TemplateTransform() noexcept = default;
	
	static TemplateTransform fromQTransform(const QTransform& qt) noexcept;
	
	void save(QXmlStreamWriter& xml, const QString& role) const;
 	void load(QXmlStreamReader& xml);
};

bool operator==(const TemplateTransform& lhs, const TemplateTransform& rhs) noexcept;
bool operator!=(const TemplateTransform& lhs, const TemplateTransform& rhs) noexcept;



/**
 * Abstract base class for templates.
 */
class Template : public QObject
{
Q_OBJECT
public:
	/**
	 * States in the lifetime of a template.
	 */
	enum State
	{
		/// The template is loaded and ready to be displayed
		Loaded = 0,
		/// The template has been unloaded, but can be reloaded if needed
		Unloaded,
		/// A required resource cannot be found (e.g. missing image or font),
		/// so the template is invalid
		Invalid
	};
	
	/**
	 * Indicates arguments which must not be nullptr.
	 * \todo Use the Guideline Support Library
	 */
	template <typename T>
	using not_null = T;
	
	
protected:	
	/**
	 * Initializes the template as "invalid".
	 */
	Template(const QString& path, not_null<Map*> map);

public:	
	~Template() override;
	
	/**
	 * Creates a duplicate of the template
	 * 
	 * \todo Rewrite as virtual function, using protected copy constructor.
	 */
	Template* duplicate() const;
	
	/**
	 * Returns a string which should identify the type of the template uniquely:
	 * the class name. Very simple RTTI feature.
	 */
	virtual const char* getTemplateType() const = 0;
	
	
	/**
	 * Returns a description of the last error that occurred.
	 */
	QString errorString() const;
	
	/**
	 * Returns true if the template is raster graphics.
	 * 
	 * Raster graphics cannot be printed in the foreground in vector mode.
	 */
	virtual bool isRasterGraphics() const = 0;
	
	
	/**
	 * Saves template parameters.
	 * 
	 * This method saves common properties such as filename, transformation,
	 * adjustment, etc., and it calls saveTypeSpecificTemplateConfiguration for
	 * saving type-specific parameters (e.g. filtering mode for images).
	 * 
	 * If a target directory is given via `map_dir`, the relative template
	 * path is determined for this directory.
	 */
	void saveTemplateConfiguration(QXmlStreamWriter& xml, bool open, const QDir* map_dir = nullptr) const;
	
	/**
	 * Creates and returns a template from the configuration in the XML stream.
	 * 
	 * Returns a null pointer in the case of error.
	 */
	static std::unique_ptr<Template> loadTemplateConfiguration(QXmlStreamReader& xml, Map& map, bool& open);
	
	
	/**
	 *  Saves the template itself, returns true if successful.
	 *
	 * This is called when saving the map if the template's hasUnsavedChanges()
	 * is true.
	 */
	virtual bool saveTemplateFile() const;
	
	/**
	 * Changes a template's file without changing the parameters.
	 * 
	 * Useful when a template file has been moved.
	 * If load_file is true, tries to load the given file.
	 */
	void switchTemplateFile(const QString& new_path, bool load_file);
	
	/**
	 * Shows the dialog to find a moved template.
	 * 
	 * If the user selects a new file, tries to switch to the selected template
	 * file using switchTemplateFile() and by trying to load the new file.
	 * Returns true if this succeeds; if not, reverts the switch and returns
	 * false. Also returns false if the dialog is aborted.
	 */
	bool execSwitchTemplateFileDialog(QWidget* dialog_parent);
	
	
	/**
	 * Does everything needed to load a template.
	 * 
	 * Calls preLoadConfiguration(), loadTemplateFile() and
	 * postLoadConfiguration(). Returns if the process was successful. 
	 * 
	 * The passed-in view is used to center the template if needed.
	 */
	bool configureAndLoad(QWidget* dialog_parent, MapView* view);
	
	/**
	 * Tries to find the template file non-interactively.
	 * 
	 * This function searches for the template in the following locations:
	 *  - saved relative position to map file, if available and map_directory is not empty
	 *  - absolute position of template file
	 *  - template filename in map_directory, if map_directory not empty
	 * 
	 * Returns true if successful.
	 * 
	 * If out_found_from_map_dir is given, it is set to true if the template file
	 * is found using the template filename in the map's directory (3rd alternative).
	 */
	bool tryToFindTemplateFile(QString map_directory, bool* out_found_from_map_dir = nullptr);
	
	/**
	 * Tries to find and load the template file non-interactively.
	 * 
	 * Returns true if the template was loaded successful.
	 * 
	 * If out_loaded_from_map_dir is given, it is set to true if the template file
	 * is found using the template filename in the map's directory.
	 * 
	 * \see tryToFindTemplateFile
	 */
	bool tryToFindAndReloadTemplateFile(QString map_directory, bool* out_loaded_from_map_dir = nullptr);
	
	
	/** 
	 * Does configuration before the actual template is loaded.
	 * 
	 * This function is called after the user chooses the template file, but
	 * before it is loaded. Derived classes can show dialogs here to get user
	 * input which is needed to interpret the template file.
	 * 
	 * If the implementation returns false, loading the template is aborted.
	 * 
	 * \note Derived classes should set is_georeferenced either here or in
	 *       postLoadConfiguration().
	 *       By default templates are loaded as non-georeferenced.
	 */
	virtual bool preLoadConfiguration(QWidget* dialog_parent);
	
	/**
	 * Loads the template file.
	 * 
	 * This function can be called if the template state is Invalid or Unloaded.
	 * It must not be called if the template file is already loaded.
	 * It returns true if the template is loaded successfully.
	 * 
	 * Set the configuring parameter to true if the template is currently being
	 * configured by the user (in contrast to the case where it is reloaded, e.g.
	 * when loaded while reopening an existing map file).
	 */
	bool loadTemplateFile(bool configuring);
	
	/**
	 * Does configuration after the actual template is loaded.
	 * 
	 * This function is called after the user chose the template file and after
	 * the chosen file was successfully loaded. Derived classes can show dialogs
	 * here to get user input which is needed to interpret the template file.
	 * 
	 * If the implementation returns false, loading the template is aborted.
	 * 
	 * By setting out_center_in_view, the implementation can decide if the
	 * template should be centered in the active view (only if it is not
	 * georeferenced.)
	 */
	virtual bool postLoadConfiguration(QWidget* dialog_parent, bool& out_center_in_view);
	
	/**
	 * Unloads the template file.
	 * 
	 * Can be called if the template state is Loaded.
	 * Must not be called if the template file is already unloaded, or invalid.
	 */
	void unloadTemplateFile();
	
	/** 
	 * Draws the template using the given painter with the given opacity.
	 * 
	 * The painter transformation is set to use template coordinates.
	 * The clip rect is in template coordinates.
	 * The scale is the combined view & template scale. It can be used to give
	 * a minimum size to elements.
	 */
    virtual void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, float opacity) const = 0;
	
	
	/** 
	 * Calculates the template's bounding box in map coordinates.
	 */
	virtual QRectF calculateTemplateBoundingBox() const;
	
	/**
	 * Returns the extra extent of the template out of the bounding box.
	 * 
	 * While the bounding box is defined in map coordinates, this border is
	 * given in pixels. This is useful for elements which stay the same size
	 * regardless of the zoom level so that a bounding box in map coords
	 * cannot be calculated.
	 */
	virtual int getTemplateBoundingBoxPixelBorder() {return 0;}
	
	/** 
	 * Marks the whole area of the template as needing a redraw.
	 * 
	 * Use this before and after modifications to the template transformation.
	 * 
	 * The default implementation marks everything as "to be redrawn" for
	 * georeferenced templates and uses the reported extent otherwise.
	 */
	virtual void setTemplateAreaDirty();
	
	
	/** 
	 * Must return if freehand drawing onto the template is possible.
	 */
	virtual bool canBeDrawnOnto() const;
	
	/**
	 * Draws onto the template.
	 * 
	 * This only works for templates for which canBeDrawnOnto() returns true.
	 * 
	 * coords is an array of points with which the drawn line is defined and
	 * must contain at least 2 points. 
	 * 
	 * If map_bbox is an invalid rect, then the method will calculate it itself.
	 * 
	 * \todo Rewrite using a range of MapCoordF.
	 */
	void drawOntoTemplate(not_null<MapCoordF*> coords, int num_coords, QColor color, float width, QRectF map_bbox);
	
	/** 
	 * Triggers an undo or redo action for template freehand drawing.
	 * 
	 * This only works for templates for which canBeDrawnOnto() returns true.
	 */
	virtual void drawOntoTemplateUndo(bool redo);
	
	
	// Transformation related methods for non-georeferenced templates only
	
	/**
	 * Changes the painter's transformation so it can be used to draw in template coordinates.
	 * 
	 * The previous transformation of the painter must be the map transformation.
	 * 
	 * \note For non-georeferenced templates only, or if the template
	 *       transformation has been set by the template nevertheless.
	 */
	void applyTemplateTransform(QPainter* painter) const;
	
	/**
	 * Returns the extent of the template in template coordinates.
	 * 
	 * The default implementation returns a "very big" rectangle.
	 * 
	 * \note For non-georeferenced templates only!
	 */
	virtual QRectF getTemplateExtent() const;
	
	/** 
	 * Scales the template with the given scaling center.
	 * 
	 * \note For non-georeferenced templates only!
	 */
	void scale(double factor, const MapCoord& center);
	
	/** 
	 * Rotates the template around the given point.
	 * 
	 * \note For non-georeferenced templates only!
	 */
	void rotate(double rotation, const MapCoord& center);
	
	
	// Coordinate transformations between template coordinates and map coordinates
	
	inline MapCoordF mapToTemplate(const MapCoordF& coords) const
	{
		return MapCoordF(map_to_template.get(0, 0) * coords.x() + map_to_template.get(0, 1) * coords.y() + map_to_template.get(0, 2),
		                 map_to_template.get(1, 0) * coords.x() + map_to_template.get(1, 1) * coords.y() + map_to_template.get(1, 2));
	}
	inline MapCoordF mapToTemplateOther(const MapCoordF& coords) const	// normally not needed - this uses the other transformation parameters
	{
		Q_ASSERT(!is_georeferenced);
		// SLOW - cache this matrix if needed often
		Matrix map_to_template_other;
		template_to_map_other.invert(map_to_template_other);
		return MapCoordF(map_to_template_other.get(0, 0) * coords.x() + map_to_template_other.get(0, 1) * coords.y() + map_to_template_other.get(0, 2),
		                 map_to_template_other.get(1, 0) * coords.x() + map_to_template_other.get(1, 1) * coords.y() + map_to_template_other.get(1, 2));
	}
	inline MapCoordF templateToMap(const QPointF& coords) const
	{
		return MapCoordF(template_to_map.get(0, 0) * coords.x() + template_to_map.get(0, 1) * coords.y() + template_to_map.get(0, 2),
		                 template_to_map.get(1, 0) * coords.x() + template_to_map.get(1, 1) * coords.y() + template_to_map.get(1, 2));
	}
	inline MapCoordF templateToMapOther(const QPointF& coords) const	// normally not needed - this uses the other transformation parameters
	{
		Q_ASSERT(!is_georeferenced);
		return MapCoordF(template_to_map_other.get(0, 0) * coords.x() + template_to_map_other.get(0, 1) * coords.y() + template_to_map_other.get(0, 2),
		                 template_to_map_other.get(1, 0) * coords.x() + template_to_map_other.get(1, 1) * coords.y() + template_to_map_other.get(1, 2));
	}
	
	
	// Pass points & adjustment
	
	inline const PassPointList& getPassPointList() const {return passpoints;}
	inline PassPointList& getPassPointList() {return passpoints;}
	inline int getNumPassPoints() const {return passpoints.size();}
	inline PassPoint* getPassPoint(int i) {return &passpoints[i];}
	void addPassPoint(const PassPoint& point, int pos);
	void deletePassPoint(int pos);
	void clearPassPoints();
	
	/// Change from adjusted into original state or the other way round
	void switchTransforms();
	void getTransform(TemplateTransform& out) const;
	void setTransform(const TemplateTransform& transform);
	void getOtherTransform(TemplateTransform& out) const;
	void setOtherTransform(const TemplateTransform& transform);
	
	
	// Getters / Setters
	// General
	
	inline Map* getMap() const {return map;}
	
	inline const QString& getTemplateFilename() const {return template_file;}
	
	inline const QString& getTemplatePath() const {return template_path;}
	/// Changes the path and filename only. Does not do any reloading etc.
	void setTemplatePath(const QString& value);
	
	inline const QString& getTemplateRelativePath() const {return template_relative_path;}
	inline void setTemplateRelativePath(const QString& value) {template_relative_path = value;}
	
	inline State getTemplateState() const {return template_state;}
	inline void setTemplateState(State state) {template_state = state;}
	
	inline int getTemplateGroup() const {return template_group;}
	inline void setTemplateGroup(int value) {template_group = value;}
	
	inline bool hasUnsavedChanges() const {return has_unsaved_changes;}
	void setHasUnsavedChanges(bool value);
	
	inline bool isTemplateGeoreferenced() const {return is_georeferenced;}
	
	/**
	 * Returns if the template allows the georefencing state to be changed at all.
	 * 
	 * The default implementation returns false.
	 */
	virtual bool canChangeTemplateGeoreferenced();
	
	/**
	 * Tries to change the usage of georeferencing data.
	 * 
	 * If supported by the actual template, this function tries to switch the
	 * state between non-georeferenced and georeferenced. It returns the final
	 * state which may be the same as before if the change is not implemented
	 * or fails for other reasons.
	 * 
	 * The default implementation changes nothing, and it just returns the
	 * current state.
	 */
	virtual bool trySetTemplateGeoreferenced(bool value, QWidget* dialog_parent);
	
	
	// Transformation of non-georeferenced templates
	
	MapCoord templatePosition() const;
	void setTemplatePosition(const MapCoord& coord);
	
	MapCoord templatePositionOffset() const;
	void setTemplatePositionOffset(const MapCoord& offset);
	void applyTemplatePositionOffset();
	void resetTemplatePositionOffset();
	
	inline qint64 getTemplateX() const {return transform.template_x;}
	inline void setTemplateX(qint64 x) {transform.template_x = x; updateTransformationMatrices();}
	
	inline qint64 getTemplateY() const {return transform.template_y;}
	inline void setTemplateY(qint64 y) {transform.template_y = y; updateTransformationMatrices();}
	
	inline double getTemplateScaleX() const {return transform.template_scale_x;}
	inline void setTemplateScaleX(double scale_x) {transform.template_scale_x = scale_x; updateTransformationMatrices();}
	inline double getTemplateScaleY() const {return transform.template_scale_y;}
	inline void setTemplateScaleY(double scale_y) {transform.template_scale_y = scale_y; updateTransformationMatrices();}
	
	inline double getTemplateRotation() const {return transform.template_rotation;}
	inline void setTemplateRotation(double rotation) {transform.template_rotation = rotation; updateTransformationMatrices();}
	
	inline bool isAdjustmentApplied() const {return adjusted;}
	inline bool isAdjustmentDirty() const {return adjustment_dirty;}
	void setAdjustmentDirty(bool value);
	
	
	/**
	 * Returns true if the template has elements which are not opaque.
	 * 
	 * The default implementation returns true when the template is loaded.
	 */
	virtual bool hasAlpha() const;
	
	
	// Static
	/**
	 * Returns the filename extensions supported by known subclasses.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	/**
	 * Creates a Template instance for the given path.
	 * 
	 * This function tries to find a matching template subclass by looking at
	 * the file extension. It may return nullptr if no subclass supports the
	 * extension.
	 */
	static std::unique_ptr<Template> templateForFile(const QString& path, Map* map);
	
	/**
	 * A flag which disables the writing of absolute paths for template files.
	 * 
	 * By default, class Template saves absolute paths. This behavior can be
	 * changed by setting this flag to true. This allows to hide local (or
	 * private) directory names.
	 * 
	 * This flag defaults to false.
	 */
	static bool suppressAbsolutePaths;
	
	
signals:
	/** 
	 * Emitted whenever template_state was changed.
	 */
	void templateStateChanged();
	
	
protected:
	/**
	 * Sets the error description which will be returned by errorString().
	 */
	void setErrorString(const QString &text);
	
	
	/** 
	 * Derived classes must create a duplicate and transfer
	 * 
	 * type specific information over to the copy here.
	 * This includes the content of the template file if it is loaded.
	 * 
	 * \todo Rewrite together with duplicate().
	 */
	virtual Template* duplicateImpl() const = 0;
	
	
	/** 
	 * Hook for saving parameters needed by the actual template type.
	 * 
	 * The default implementation does nothing.
	 */
	virtual void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const;
	
	/**
	 * Hook for loading parameters needed by the actual template type.
	 * 
	 * \note The default implementation calls xml.skipCurrentElement().
	 *       Implementations must do the same if they do not parse it.
	 * 
	 * Returns true on success.
	 */
	virtual bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml);
	
	
	/**
	 * Hook for loading the actual template file non-interactively.
	 * 
	 * Returns true if successful.
	 * 
	 * If configuring is true, a call to postLoadConfiguration() will follow
	 * if this returns true.
	 */
	virtual bool loadTemplateFileImpl(bool configuring) = 0;
	
	/**
	 * Hook for unloading the template file.
	 */
	virtual void unloadTemplateFileImpl() = 0;
	
	
	/** 
	 * Hook for drawing on the template.
	 * 
	 * Draws the polyline given by the points onto the template.
	 * Required if canBeDrawnOnto() returns true.
	 */
	virtual void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, QColor color, float width);
	
	
	/**
	 * Must be called after direct changes to transform or other_transform.
	 */
	void updateTransformationMatrices();
	
	
	// General properties
	
	/// Map containing this template
	Map* map;
	
	/// The filename of the template file (e.g. "map.bmp")
	QString template_file;
	
	/// The complete path to the template file including the filename (e.g. "/home/me/map.bmp")
	QString template_path;
	
	/// The template path relative to the map file (e.g. "../me/map.bmp").
	/// Can be empty as long as the map file has not been saved yet.
	QString template_relative_path;
	
	/// The template lifetime state
	State template_state;
	
	/// The description of the last error
	QString error_string;
	
	/// Does the template itself (not its transformation) have unsaved changes (e.g. GPS track has changed, image has been painted on)
	bool has_unsaved_changes;
	
	/// Is the template in georeferenced mode?
	bool is_georeferenced;
	
private:	
	// Properties for non-georeferenced templates (invalid if is_georeferenced is true) 
	
	/// Bounds correction offset for map templates. Must be masked out when saving.
	MapCoord accounted_offset;
	
	/**
	 * This class reverts the template's accounted offset for its lifetime.
	 * 
	 * \todo Copy/restore the transformation matrices when this no longer needs
	 *       allocations.
	 */
	class ScopedOffsetReversal;
	
protected:
	/// Currently active transformation. NOTE: after direct changes here call updateTransformationMatrices()
	TemplateTransform transform;
	
	/// Currently inactive transformation (adjusted or original). NOTE: after direct changes here call updateTransformationMatrices()
	TemplateTransform other_transform;
	
	/// If true, transform is the adjusted transformation, otherwise it is the original one
	bool adjusted;
	
	/// If true, the adjusted transformation has to be recalculated
	bool adjustment_dirty;
	
	/// List of pass points for position adjustment
	PassPointList passpoints;
	
	/// Number of the template group. If the template is not grouped, this is set to -1.
	int template_group;
	
	// Transformation matrices calculated from cur_trans
	Matrix map_to_template;
	Matrix template_to_map;
	Matrix template_to_map_other;
};


}  // namespace OpenOrienteering

#endif
