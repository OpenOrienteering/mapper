/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020, 2025 Kai Pastor
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

#include <functional>
#include <memory>
#include <vector>

#include <QtGlobal>
#include <QFlags>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringRef>

#include "core/map_coord.h"
#include "util/matrix.h"
#include "util/transformation.h"

class QByteArray;
class QColor;
class QDir;
class QFileInfo;
class QPainter;
class QPointF;
class QRectF;
class QTransform;
class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;
class MapView;
class Object;


/**
 * Transformation parameters for non-georeferenced templates,
 * transforming template coordinates to map coordinates.
 * 
 * The parameters are applied to painter in the order
 * 1. translate
 * 2. rotate
 * 3. scale.
 * 
 * So this order is also chosen for the member variables, and
 * thus used in list initialization.
 * 
 * \see Template::applyTemplateTransform()
 * 
 * Coordinate transformations use the opposite order for the
 * same effect.
 */
struct TemplateTransform
{
	/// x position in 1/1000 mm
	qint32 template_x = 0;
	/// x position in 1/1000 mm
	qint32 template_y = 0;
	
	/// Rotation in radians, a positive rotation is counter-clockwise.
	double template_rotation = 0.0;
	
	/// Scaling in x direction, smaller than 1 shrinks.
	double template_scale_x = 1.0;
	/// Scaling in y direction, smaller than 1 shrinks.
	double template_scale_y = 1.0;
	/// Adjustment to scaling if anisotropy is askew.
	double template_shear = 0.0;
	
	static TemplateTransform fromQTransform(const QTransform& qt) noexcept;
	
	std::function<void (Object*)> makeObjectTransform() const;
	
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
		/// The template data is loaded and ready to be displayed.
		Loaded = 0,
		/// The template data is not yet loaded or has been unloaded.
		/// It is assumed to be available, so it can be (re-)loaded if needed.
		Unloaded,
		/// The template's configuration/setup is not yet finished.
		/// This is the initial state after construction.
		Configuring,
		/// A required resource cannot be found (e.g. missing image or font),
		/// so the template data cannot be loaded.
		Invalid
	};
	
	/**
	 * The result of template file lookup attempts.
	 */
	enum LookupResult
	{
		NotFound       = 0,  ///< File not found at all.
		FoundInMapDir  = 1,  ///< File name found in the map's directory.
		FoundByRelPath = 2,  ///< File found by relative path from the map's directory.
		FoundByAbsPath = 3,  ///< File found by absolute path.
	};

	/**
	 * Modifier options for the scribble tool operation.
	 */
	enum ScribbleOption
	{
		NoScribbleOptions   = 0x00,
		FilledAreas         = 0x01,  ///< Fill the area defined by the scribble line.
		ComposeBackground   = 0x02,  ///< Draw lines or fill areas only in transparent areas.
		PatternFill         = 0x04,  ///< Fill areas with a dot pattern.
		ScribbleOptionsMask = 0x07,  ///< All bits used by ScribbleOption.
	};
	Q_DECLARE_FLAGS(ScribbleOptions, ScribbleOption)
	
	/**
	 * Indicates arguments which must not be nullptr.
	 * \todo Use the Guideline Support Library
	 */
	template <typename T>
	using not_null = T;
	
	
protected:	
	/**
	 * Initializes the template as "Unloaded".
	 */
	Template(const QString& path, not_null<Map*> map);
	
	/**
	 * Copy-construction as duplication helper.
	 */
	Template(const Template& proto);
	
public:	
	~Template() override;
	
	/**
	 * Creates a duplicate of the template
	 */
	virtual Template* duplicate() const = 0;
	
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
	 * The template will be in Unloaded state after this function.
	 * 
	 * Returns a null pointer in the case of error.
	 */
	static std::unique_ptr<Template> loadTemplateConfiguration(QXmlStreamReader& xml, Map& map, bool& open);
	
	
	/**
	 * Saves the template contents and returns true if successful.
	 *
	 * This is called when saving the map if the template's hasUnsavedChanges()
	 * is true. After successful saving, hasUnsavedChanges() shall return false.
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
	 * Interactively changes the template file.
	 * 
	 * This functions shows a file dialog for selecting a new template file.
	 * Upon selection, it tries to load the template file. When loading fails,
	 * the original path and state is restored.
	 * 
	 * When this function is called on a TemplatePlaceholder object, an object
	 * of an actual template implementation class is created in order to load
	 * the data file. After successful loading, this new object replaces the
	 * original object in the map's list of active templates, destroying the
	 * original object. Thus the current object may be no longer in the map's
	 * active list, and it will be destroyed when control returns to the event
	 * loop.
	 * 
	 * Returns true if a file is selected and the loading succeeds,
	 * false otherwise.
	 */
	bool execSwitchTemplateFileDialog(QWidget* dialog_parent);
	
	
	/**
	 * Does everything needed to load a template.
	 * 
	 * This function can be called only if the template state is Configuring.
	 * Calls preLoadSetup(), loadTemplateFile() and postLoadSetup().
	 * Returns true if the process was successful.
	 * 
	 * The passed-in view is used to center the template if needed.
	 */
	bool setupAndLoad(QWidget* dialog_parent, const MapView* view);
	
	/**
	 * Tries to find the template file non-interactively.
	 * 
	 * This function updates the path and name variables, and the template state.
	 * If successful, changes the state from Invalid to Unloaded if necessary,
	 * and returns true. Otherwise, changes the state from Unloaded to Invalid
	 * if necessary, and returns false. (If the state is Loaded, it is left
	 * unchanged.) It returns an indication of its success.
	 * 
	 * The default implementation searches for the template in the following
	 * locations:
	 * 
	 *  1. The relative path with regard to the map directory, if both are valid.
	 *     This alternative has precedence because it should always work,
	 *     especially after copying or moving a whole working directory on the
	 *     same computer or to another one.
	 * 
	 *  2. The absolute path of the template.
	 *     This is the most explicit alternative. It works on the same computer
	 *     when the map file is copied or moved to another location.
	 * 
	 *  3. The map directory, if valid, for the filename of the template.
	 *     This is a fallback for use cases where a map and selected templates
	 *     are moved to the same flat folder, e.g. when receiving them via
	 *     individual e-mail attachments.
	 * 
	 * \param map_path  Either the full filepath of the map, or an arbitrary
	 *                  directory which shall be regarded as the map directory.
	 */
	virtual LookupResult tryToFindTemplateFile(const QString& map_path);
	
	/**
	 * A check if the template file exists.
	 * 
	 * The default implementation checks if the file exists in the file system.
	 * This maybe overridden to handle virtual file systems such as KMZ files.
	 * If it returns false, template loading will fail with "No such file.".
	 */
	virtual bool fileExists() const;
	
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
	bool tryToFindAndReloadTemplateFile(const QString& map_path);
	
	
	/** 
	 * Setup event before the template is loaded for the first time.
	 * 
	 * This function is called after the user chooses the template file, but
	 * before regular loading. Derived classes can show dialogs here to get user
	 * input which is needed to interpret the template file.
	 * 
	 * If the implementation returns false, loading the template is aborted.
	 * 
	 * \note Derived classes should set is_georeferenced either here or in
	 *       postLoadSetup().
	 *       By default templates are loaded as non-georeferenced.
	 */
	virtual bool preLoadSetup(QWidget* dialog_parent);
	
	/**
	 * Loads the template file.
	 * 
	 * This function can be called if the template state is Configuring, Invalid or Unloaded.
	 * It must not be called if the template file is already loaded.
	 * It returns true if the template is loaded successfully.
	 */
	bool loadTemplateFile();
	
	/**
	 * Setup event after the template is loaded for the first time.
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
	virtual bool postLoadSetup(QWidget* dialog_parent, bool& out_center_in_view);
	
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
    virtual void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, qreal opacity) const = 0;
	
	
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
	virtual int getTemplateBoundingBoxPixelBorder() const {return 0;}
	
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
	void drawOntoTemplate(not_null<MapCoordF*> coords, int num_coords, const QColor& color, qreal width, QRectF map_bbox, ScribbleOptions mode);
	
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
	 * Returns the bounding rectangle of the template in map coordinates.
	 * 
	 * The default implementation relies on getTemplateExtent().
	 */
	virtual QRectF boundingRect() const;
	
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
	
	inline const QString& getTemplateCustomname() const {return template_custom_name;}
	inline void setTemplateCustomname(const QString& template_custom_name) {this->template_custom_name = template_custom_name;}
	
	inline bool getCustomnamePreference() const {return custom_name_preference;}
	inline void setCustomnamePreference(bool custom_name_preference) {this->custom_name_preference = custom_name_preference;}
	
	/// Changes the path and filename only. Does not do any reloading etc.
	void setTemplateFileInfo(const QFileInfo& file_info);
	
	inline const QString& getTemplatePath() const {return template_path;}
	/// Changes the path and filename only. Does not do any reloading etc.
	void setTemplatePath(const QString& value);
	
	/**
	 * Returns an updated relative path.
	 * 
	 * If the template is in a valid state and map_dir is given, this function
	 * returns the corresponding relative path to the template file.
	 * 
	 * Otherwise it returns the original relative path if it is not empty, or
	 * just the template file name.
	 */
	QString getTemplateRelativePath(const QDir* map_dir) const;
	
	/// Returns the relative path which was set when template was configured from XML.
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
	virtual bool canChangeTemplateGeoreferenced() const;
	
	/**
	 * Tries to change the usage of georeferencing data.
	 * 
	 * If supported by the actual template, this function tries to switch the
	 * state between non-georeferenced and georeferenced. It returns false when
	 * an error occurred, and true if the change was successful or if it was
	 * explicitly cancelled by the user.
	 * 
	 * The default implementation returns true iff the state matches the value.
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
	inline double getTemplateShear() const {return transform.template_shear;}
	inline void setTemplateShear(double shear) {transform.template_shear = shear; updateTransformationMatrices();}
	
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
	 * the file extension and/or the content available at the given path.
	 * It may return nullptr if no subclass supports the extension or content.
	 */
	static std::unique_ptr<Template> templateForPath(const QString& path, Map* map);
	
	/**
	 * Creates a Template instance for the given type.
	 * 
	 * This function may return nullptr if the given type is unknown.
	 * 
	 * In addition, this function respects the user setting for assigning
	 * some TemplateTrack extensions (GPX) explicitly to GDAL (OgrTemplate).
	 */
	static std::unique_ptr<Template> templateForType(const QStringRef& type, const QString& path, Map* map);
	
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
	 * Hook which is called at the end of template configuration loading.
	 * 
	 * Derived classes may override this function if they need to do any post-
	 * processing on the configuration. Unlike loadTypeSpecificTemplateConfiguration(),
	 * this function is always called, even when there is no type-specific data.
	 * 
	 * The implementation must not do expensive calculations because it is
	 * called also for hidden and closed templates. It cannot reliably access
	 * the template data (or sidecar files) because it is called before the
	 * validation/updating of the template path.
	 * 
	 * Returns true on success.
	 */
	virtual bool finishTypeSpecificTemplateConfiguration();
	
	
	/**
	 * Hook for loading the actual template file non-interactively.
	 * 
	 * Returns true if successful.
	 * 
	 * If configuring is true, a call to postLoadSetup() will follow
	 * if this returns true.
	 */
	virtual bool loadTemplateFileImpl() = 0;
	
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
	virtual void drawOntoTemplateImpl(MapCoordF* coords, int num_coords, const QColor& color, qreal width, ScribbleOptions mode);
	
	
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
	
	/// The custom name of the template set by the user
	QString template_custom_name;
	
	/// User preference to show custom name instead of filename
	bool custom_name_preference = false;
	
	/// The template lifetime state
	State template_state = Configuring;
	
	/// The description of the last error
	QString error_string;
	
	/// Does the template itself (not its transformation) have unsaved changes (e.g. GPS track has changed, image has been painted on)
	bool has_unsaved_changes = false;
	
	/// Is the template in georeferenced mode?
	bool is_georeferenced = false;
	
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
	bool adjusted = false;
	
	/// If true, the adjusted transformation has to be recalculated
	bool adjustment_dirty = true;
	
	/// List of pass points for position adjustment
	PassPointList passpoints;
	
	/// Number of the template group. If the template is not grouped, this is set to -1.
	/// \todo Switch to initialization with -1. ATM 0 is kept for compatibility.
	int template_group = 0;
	
	// Transformation matrices calculated from cur_trans
	Matrix map_to_template;
	Matrix template_to_map;
	Matrix template_to_map_other;
};


}  // namespace OpenOrienteering

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::Template::ScribbleOptions)


#endif // OPENORIENTEERING_TEMPLATE_H
