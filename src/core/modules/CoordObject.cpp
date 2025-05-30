/*
 * Copyright (C) 2025 Georg Zotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "CoordObject.hpp"
#include "StelObject.hpp"

const QString CoordObject::COORDOBJECT_TYPE = QStringLiteral("CoordObject");

CoordObject::CoordObject(const QString& aName, const Vec3d& coordJ2000) : StelObject()
	, XYZ(coordJ2000)
	, name(aName)
{
	XYZ.normalize();
}

Vec3d CoordObject::getJ2000EquatorialPos(const StelCore* core) const
{
	Q_UNUSED(core)
	Q_ASSERT(qFuzzyCompare(XYZ.normSquared(), 1.0));
	return XYZ;
}
