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


#include "app_permissions.h"

#include <algorithm>
#include <QtAndroid>


namespace AppPermissions
{

namespace
{

/**
 * Returns a functor that tests if the passed permission is granted.
 */
auto granted()
{
	return [](const QString& permission)->bool {
		return QtAndroid::checkPermission(permission) == QtAndroid::PermissionResult::Granted;
	};
}

/**
 * Returns a functor that tests if the passed permissions are granted
 * in the actual permissions given at functor creation time.
 */
auto granted(const QtAndroid::PermissionResultMap& actual_permissions)
{
	return [&actual_permissions](const QString& permission)->bool {
		return actual_permissions.contains(permission)
		       && actual_permissions[permission] == QtAndroid::PermissionResult::Granted;
	};
}

}  // namespace



QStringList androidPermissions(AppPermission permission)
{
	QStringList android_permissions;
	android_permissions.reserve(2);
	
	switch (permission)
	{
	case StorageAccess:
		android_permissions << QStringLiteral("android.permission.READ_EXTERNAL_STORAGE")
		                    << QStringLiteral("android.permission.WRITE_EXTERNAL_STORAGE");
		break;
		
	case LocationAccess:
		android_permissions << QStringLiteral("android.permission.ACCESS_FINE_LOCATION");
		break;
	}
	
	return android_permissions;
}


bool permissionsGranted(const QStringList& requested_permissions, const QtAndroid::PermissionResultMap& actual_permissions)
{
	using std::begin; using std::end;
	if (std::all_of(begin(requested_permissions), end(requested_permissions), granted(actual_permissions)))
		return Granted;
	return Denied;
}


PermissionResult checkPermission(AppPermission permission)
{
	using std::begin; using std::end;
	auto const requested_permissions = androidPermissions(permission);
	if (std::all_of(begin(requested_permissions), end(requested_permissions), granted()))
		return Granted;
	return Denied;
}


PermissionResult requestPermissionSync(AppPermission permission)
{
	auto const requested_permissions = androidPermissions(permission);
	auto const actual_permissions = QtAndroid::requestPermissionsSync(requested_permissions);
	if (permissionsGranted(requested_permissions, actual_permissions))
		return Granted;
	return Denied;
}


}  // namespace AppPermissions
