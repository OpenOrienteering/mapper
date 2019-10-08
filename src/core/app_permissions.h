/*
 *    Copyright 2019 Kai Pastor
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


#ifndef OPENORIENTEERING_APP_PERMISSIONS_H
#define OPENORIENTEERING_APP_PERMISSIONS_H

#include <QtGlobal>

#ifdef Q_OS_ANDROID
#  include <functional>
#  include <type_traits>
#  include <QtAndroid>
#  include <QObject>
#  include <QPointer>
#  include <QStringList>
#  include <QTimer>
#endif


#ifdef __clang_analyzer__
#define singleShot(A, B, C) singleShot(A, B, #C) // NOLINT 
#endif


/**
 * A generic utility for requesting app permissions from the user.
 * 
 * This class is modeled after the patterns found in [Qt]Android,
 * but provides an abstraction from OS specific aspects.
 */
namespace AppPermissions 
{
	/// Permissions which are required for certain features of the application.
	enum AppPermission
	{
		LocationAccess,
		StorageAccess,
	};
	
	/// Possible results of requesting a permission.
	enum PermissionResult
	{
		Denied,
		Granted,
	};
	
	
	
	/**
	 * Checks if the permission was granted or not. 
	 */
	PermissionResult checkPermission(AppPermission permission);
	
	/**
	 * Asynchronously requests a new permission to be granted.
	 * 
	 * The given member function on the receiver will be called when the
	 * permission is actually granted.
	 * 
	 * This function must not be called while the requested permission is granted.
	 */
	template<class T>
	void requestPermission(AppPermission permission, T* object, void (T::* function)());
	
	/**
	 * Requests a permissions to be granted to the application.
	 * 
	 * This function must not be called while the requested permission is granted.
	 */
	PermissionResult requestPermissionSync(AppPermission permission);
	
	
	
#ifdef Q_OS_ANDROID
	
	// The Android implementation uses some helper functions.
	// Only the generic parts are defined in the header.
	
	/// Returns the list of Android permission for the given abstract permission.
	QStringList androidPermissions(AppPermission permission);
	
	/// Tests if all the requested Android permissions are granted.
	bool permissionsGranted(const QStringList& requested_permissions, const QtAndroid::PermissionResultMap& actual_permissions);
	
	/// A utility for safely doing callbacks after asynchronuous requests.
	template <class T>
	struct PermissionCallback
	{
		Q_STATIC_ASSERT((std::is_base_of<QObject, T>::value));
		
		using Function = void (T::*)();
		const QPointer<T> object;
		const Function function;
		const QStringList requested_permissions;
		
		PermissionCallback(T* object, Function function, const QStringList& android_permissions)
		: object{object}
		, function{function}
		, requested_permissions{android_permissions}
		{}
		
		void run(const QtAndroid::PermissionResultMap& actual_permissions)
		{
			if (object && permissionsGranted(requested_permissions, actual_permissions))
				QTimer::singleShot(10, object, function);
			delete this;
		}
		
	};
	
	template<class T>
	void requestPermission(AppPermission permission, T* object, void (T::* function)())
	{
		auto const android_permissions = androidPermissions(permission);
		auto* callback = new PermissionCallback<T>(object, function, android_permissions);
		QtAndroid::requestPermissions(android_permissions, [callback](const auto& result) { callback->run(result); });
	}
	
#else
	
	// The default implementation is fully inline,
	// in order to allow the compiler to optimize it out.
	
	inline PermissionResult checkPermission(AppPermission /*permission*/)
	{
		return Granted;
	}
	
	template<class T>
	void requestPermission(AppPermission /*permission*/, T* /*object*/, void (T::* /*function*/)())
	{
		// requestPermission() shouldn't be called because permissions are always granted.
		Q_UNREACHABLE();
	}
	
	inline PermissionResult requestPermissionSync(AppPermission /*permission*/)
	{
		// requestPermission() shouldn't be called because permissions are always granted.
		Q_UNREACHABLE();
		return Granted;
	}
	
#endif
	
}  // namespace AppPermissions


#ifdef __clang_analyzer__
#undef singleShot(A, B, C)
#endif


#endif  // OPENORIENTEERING_APP_PERMISSIONS_H
