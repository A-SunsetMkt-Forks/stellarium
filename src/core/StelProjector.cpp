/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Matthew Gates
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

#include "StelTranslator.hpp"

#include "StelProjector.hpp"
#include "StelApp.hpp"

#include <QDebug>
#include <QString>
#include <QOpenGLShaderProgram>

StelProjector::Mat4dTransform::Mat4dTransform(const Mat4d& altAzToWorld, const Mat4d& vertexToAltAzPos)
    : transfoMat(altAzToWorld*vertexToAltAzPos)
    , transfoMatf(toMat4f(transfoMat))
	, vertexToAltAzPos(toMat4f(vertexToAltAzPos))
	, worldPosToAltAzPos(toMat4f(altAzToWorld.inverse()))
{
	Q_ASSERT(transfoMat[0]==transfoMat[0]); // prelude to assert later in Atmosphere rendering... still investigating
}

void StelProjector::Mat4dTransform::forward(Vec3d& v) const
{
	v.transfo4d(transfoMat);
}

void StelProjector::Mat4dTransform::backward(Vec3d& v) const
{
	// We need no matrix inversion because we always work with orthogonal matrices (where the transposed is the inverse).
	//v.transfo4d(inverseTransfoMat);
	const double x = v[0] - transfoMat.r[12];
	const double y = v[1] - transfoMat.r[13];
	const double z = v[2] - transfoMat.r[14];
	v[0] = transfoMat.r[0]*x + transfoMat.r[1]*y + transfoMat.r[2]*z;
	v[1] = transfoMat.r[4]*x + transfoMat.r[5]*y + transfoMat.r[6]*z;
	v[2] = transfoMat.r[8]*x + transfoMat.r[9]*y + transfoMat.r[10]*z;
}

void StelProjector::Mat4dTransform::forward(Vec3f& v) const
{
	v.transfo4d(transfoMatf);
}

void StelProjector::Mat4dTransform::backward(Vec3f& v) const
{
	// We need no matrix inversion because we always work with orthogonal matrices (where the transposed is the inverse).
	//v.transfo4d(inverseTransfoMat);
	const float x = v[0] - transfoMatf.r[12];
	const float y = v[1] - transfoMatf.r[13];
	const float z = v[2] - transfoMatf.r[14];
	v[0] = transfoMatf.r[0]*x + transfoMatf.r[1]*y + transfoMatf.r[2]*z;
	v[1] = transfoMatf.r[4]*x + transfoMatf.r[5]*y + transfoMatf.r[6]*z;
	v[2] = transfoMatf.r[8]*x + transfoMatf.r[9]*y + transfoMatf.r[10]*z;
}

void StelProjector::Mat4dTransform::combine(const Mat4d& m)
{
	transfoMat=transfoMat*m;
	transfoMatf=toMat4f(transfoMat);
}

Mat4d StelProjector::Mat4dTransform::getApproximateLinearTransfo() const
{
	return transfoMat;
}

StelProjector::ModelViewTranformP StelProjector::Mat4dTransform::clone() const
{
	return ModelViewTranformP(new Mat4dTransform(*this));
}

QByteArray StelProjector::Mat4dTransform::getForwardTransformShader() const
{
	return 1+R"(
#line 1 104
uniform mat4 PROJECTOR_vertexToAltAzPosMatrix;
uniform mat4 PROJECTOR_modelViewMatrix;
vec3 modelViewForwardTransform(vec3 v)
{
	return (PROJECTOR_modelViewMatrix * vec4(v,1)).xyz;
}
vec3 vertexToAltAzPos(vec3 v)
{
	return (PROJECTOR_vertexToAltAzPosMatrix * vec4(v,1)).xyz;
}
#line 1 0
)";
}

void StelProjector::Mat4dTransform::setForwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	program.setUniformValue("PROJECTOR_modelViewMatrix", transfoMatf.toQMatrix());
	program.setUniformValue("PROJECTOR_vertexToAltAzPosMatrix", vertexToAltAzPos.toQMatrix());
}

QByteArray StelProjector::Mat4dTransform::getBackwardTransformShader() const
{
	return 1+R"(
#line 1 105
uniform mat4 PROJECTOR_modelViewMatrixInverse;
uniform mat4 PROJECTOR_worldPosToAltAzPosMatrix;
vec3 modelViewBackwardTransform(vec3 worldPos)
{
	return (PROJECTOR_modelViewMatrixInverse * vec4(worldPos,1)).xyz;
}
vec3 worldPosToAltAzPos(vec3 worldPos)
{
	return (PROJECTOR_worldPosToAltAzPosMatrix * vec4(worldPos,1)).xyz;
}

#line 1 0
)";
}

void StelProjector::Mat4dTransform::setBackwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	program.setUniformValue("PROJECTOR_modelViewMatrixInverse", transfoMatf.toQMatrix().inverted());
	program.setUniformValue("PROJECTOR_worldPosToAltAzPosMatrix", worldPosToAltAzPos.toQMatrix());
}

const QString StelProjector::maskTypeToString(StelProjectorMaskType type)
{
	if (type == MaskDisk )
		return "disk";
	else
		return "none";
}

StelProjector::StelProjectorMaskType StelProjector::stringToMaskType(const QString &s)
{
	if (s=="disk")
		return MaskDisk;
	return MaskNone;
}

void StelProjector::init(const StelProjectorParams& params)
{
	devicePixelsPerPixel = params.devicePixelsPerPixel;
	maskType = params.maskType;
	zNear = params.zNear;
	oneOverZNearMinusZFar = 1./(zNear-params.zFar);
	viewportCenterOffset = params.viewportCenterOffset;
	viewportXywh = params.viewportXywh;
	viewportXywh[0] *= devicePixelsPerPixel;
	viewportXywh[1] *= devicePixelsPerPixel;
	viewportXywh[2] *= devicePixelsPerPixel;
	viewportXywh[3] *= devicePixelsPerPixel;
	viewportCenter = params.viewportCenter;
	viewportCenter *= devicePixelsPerPixel;
	gravityLabels = params.gravityLabels;
	defaultAngleForGravityText = params.defaultAngleForGravityText;
	flipHorz = params.flipHorz ? -1.f : 1.f;
	flipVert = params.flipVert ? -1.f : 1.f;
	viewportFovDiameter = params.viewportFovDiameter * devicePixelsPerPixel;
	pixelPerRad = 0.5f * static_cast<float>(viewportFovDiameter) / fovToViewScalingFactor(params.fov*(static_cast<float>(M_PI/360.)));
	widthStretch = params.widthStretch;
	computeBoundingCap();
}

QString StelProjector::getHtmlSummary() const
{
	return QString("<h3>%1</h3><p>%2</p><b>%3</b>%4°").arg(getNameI18(), getDescriptionI18(), q_("Maximum FOV: "), QString::number(static_cast<double>(getMaxFov())));
}

void StelProjector::setForwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	modelViewTransform->setForwardTransformUniforms(program);
	program.setUniformValue("PROJECTOR_FWD_widthStretch", GLfloat(widthStretch));
}

void StelProjector::setBackwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	modelViewTransform->setBackwardTransformUniforms(program);
	program.setUniformValue("PROJECTOR_FWD_widthStretch", GLfloat(widthStretch));
}

bool StelProjector::intersectViewportDiscontinuity(const Vec3d& p1, const Vec3d& p2) const
{
	if (hasDiscontinuity()==false)
		return false;
	Vec3d v1(p1);
	modelViewTransform->forward(v1);
	Vec3d v2(p2);
	modelViewTransform->forward(v2);
	return intersectViewportDiscontinuityInternal(v1, v2);
}

bool StelProjector::intersectViewportDiscontinuity(const SphericalCap& cap) const
{
	if (hasDiscontinuity()==false)
		return false;
	Vec3d v1(cap.n);
	modelViewTransform->forward(v1);
	return intersectViewportDiscontinuityInternal(v1, cap.d);
}

bool StelProjector::getFlagGravityLabels() const
{
	return gravityLabels;
}

const Vec4i& StelProjector::getViewport() const
{
	return viewportXywh;
}

Vector2<qreal> StelProjector::getViewportCenter() const
{
	return Vector2<qreal>(viewportCenter[0]-viewportXywh[0],viewportCenter[1]-viewportXywh[1]);
}

Vector2<qreal> StelProjector::getViewportCenterOffset() const
{
	return viewportCenterOffset;
}

int StelProjector::getViewportPosX() const
{
	return viewportXywh[0];
}

int StelProjector::getViewportPosY() const
{
	return viewportXywh[1];
}

int StelProjector::getViewportWidth() const
{
	return viewportXywh[2];
}

int StelProjector::getViewportHeight() const
{
	return viewportXywh[3];
}



/*************************************************************************
 Return a convex polygon on the sphere which includes the viewport in the
 current frame
*************************************************************************/
SphericalRegionP StelProjector::getViewportConvexPolygon(float marginX, float marginY) const
{
	Vec3d e0, e1, e2, e3;
	const Vec4i& vp = viewportXywh;
	bool ok = unProject(static_cast<double>(vp[0]-marginX),static_cast<double>(vp[1]-marginY),e0);
	ok &= unProject(static_cast<double>(vp[0]+vp[2]+marginX),static_cast<double>(vp[1]-marginY),e1);
	ok &= unProject(static_cast<double>(vp[0]+vp[2]+marginX),static_cast<double>(vp[1]+vp[3]+marginY),e2);
	ok &= unProject(static_cast<double>(vp[0]-marginX),static_cast<double>(vp[1]+vp[3]+marginY),e3);
	if (!ok)
	{
		// Special case for handling degenerated cases, use full sky.
		//qDebug() << "!ok";
		return SphericalRegionP(reinterpret_cast<SphericalRegion*>(new AllSkySphericalRegion()));
	}
	e0.normalize();
	e1.normalize();
	e2.normalize();
	e3.normalize();
	if (needGlFrontFaceCW())
	{
		Vec3d v = e0;
		e0 = e3;
		e3 = v;
		v = e1;
		e1 = e2;
		e2 = v;
	}
	const double d = e3*((e2-e3)^(e1-e3));
	if (d > 0)
	{
		SphericalConvexPolygon* res = new SphericalConvexPolygon(e0, e1, e2, e3);
		if (res->checkValid())
		{
			return SphericalRegionP(res);
		}
		//qDebug() << "!valid";
		delete res;
	}
	//return SphericalRegionP((SphericalRegion*)(new AllSkySphericalRegion()));
	const SphericalCap& hp = getBoundingCap();
	return SphericalRegionP(new SphericalCap(hp));
}

const SphericalCap& StelProjector::getBoundingCap() const
{
	return boundingCap;
}

float StelProjector::getPixelPerRadAtCenter() const
{
	return pixelPerRad;
}

float StelProjector::getScreenScale() const
{
	return StelApp::getInstance().getScreenScale();
}

//! Get the current FOV diameter in degrees
float StelProjector::getFov() const {
	return 360.f/static_cast<float>(M_PI)*viewScalingFactorToFov(0.5f*static_cast<float>(viewportFovDiameter)/pixelPerRad);
}

//! Get whether front faces need to be oriented in the clockwise direction
bool StelProjector::needGlFrontFaceCW() const
{
	return (flipHorz*flipVert < 0.f);
}

bool StelProjector::checkInViewport(const Vec3d& pos) const
{
	return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
	        pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));
}

//! Check to see if a 2d position is inside the viewport.
//! TODO Optimize by storing viewportXywh[1] + viewportXywh[3] and viewportXywh[0] + viewportXywh[2] already computed
bool StelProjector::checkInViewport(const Vec3f& pos) const
{
	return (pos[1]>=viewportXywh[1] && pos[0]>=viewportXywh[0] &&
	        pos[1]<=(viewportXywh[1] + viewportXywh[3]) && pos[0]<=(viewportXywh[0] + viewportXywh[2]));
}

//! Return the position where the 2 2D point p1 and p2 cross the viewport edge
//! P1 must be inside the viewport and P2 outside (check with checkInViewport() before calling this method)
Vec3d StelProjector::viewPortIntersect(const Vec3d& p1, const Vec3d& p2) const
{
	Vec3d v1=p1;
	Vec3d v2=p2;
	Vec3d v;
	for (int i=0;i<8;++i)
	{
		v=(v1+v2)*0.5;
		if (!checkInViewport(v))
			v2=v;
		else
			v1=v;
	}
	return v;
}

bool StelProjector::project(const Vec3d& v, Vec3d& win) const
{
	win = v;
	return projectInPlace(win);
}

bool StelProjector::project(const Vec3f& v, Vec3f& win) const
{
	win = v;
	return projectInPlace(win);
}

bool StelProjector::project(const Vec3d& v, Vec3f& win) const
{
	Vec3d wind = v;
	bool res= projectInPlace(wind);
	win = wind.toVec3f();
	return res;
}

void StelProjector::project(int n, const Vec3d* in, Vec3f* out)
{
	Vec3d v;
	for (int i = 0; i < n; ++i, ++out)
	{
		v = in[i];
		modelViewTransform->forward(v);
		out->set(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
		forward(*out);
		out->set(static_cast<float>(viewportCenter[0]) + flipHorz * pixelPerRad * (*out)[0],
		         static_cast<float>(viewportCenter[1]) + flipVert * pixelPerRad * (*out)[1],
		         static_cast<float>((static_cast<double>((*out)[2]) - zNear) * oneOverZNearMinusZFar));
	}
}

void StelProjector::project(int n, const Vec3f* in, Vec3f* out)
{
	for (int i = 0; i < n; ++i, ++out)
	{
		*out=in[i];
		modelViewTransform->forward(*out);
		forward(*out);
		out->set(static_cast<float>(viewportCenter[0]) + flipHorz * pixelPerRad * (*out)[0],
		         static_cast<float>(viewportCenter[1]) + flipVert * pixelPerRad * (*out)[1],
		         static_cast<float>((static_cast<double>((*out)[2]) - zNear) * oneOverZNearMinusZFar));
	}
}

bool StelProjector::projectInPlace(Vec3d& vd) const
{
	modelViewTransform->forward(vd);
	Vec3f v= vd.toVec3f();
	const bool rval = forward(v);
	// very important: even when the projected point comes from an
	// invisible region of the sky (rval=false), we must finish
	// reprojecting, so that OpenGL can successfully eliminate
	// polygons by culling.
	vd[0] = viewportCenter[0] + static_cast<double>(flipHorz * pixelPerRad * v[0]);
	vd[1] = viewportCenter[1] + static_cast<double>(flipVert * pixelPerRad * v[1]);
	vd[2] = (static_cast<double>(v[2]) - zNear) * oneOverZNearMinusZFar;
	return rval;
}

bool StelProjector::projectInPlace(Vec3f& v) const
{
	modelViewTransform->forward(v);
	const bool rval = forward(v);
	// very important: even when the projected point comes from an
	// invisible region of the sky (rval=false), we must finish
	// reprojecting, so that OpenGl can successfully eliminate
	// polygons by culling.
	v[0] = static_cast<float>(viewportCenter[0]) + flipHorz * pixelPerRad * v[0];
	v[1] = static_cast<float>(viewportCenter[1]) + flipVert * pixelPerRad * v[1];
	v[2] = static_cast<float>((static_cast<double>(v[2]) - zNear) * oneOverZNearMinusZFar);
	return rval;
}

//! Project the vector v from the current frame into the viewport.
//! @param v the direction vector in the current frame. Does not need to be normalized.
//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
//! @return true if the projected point is inside the viewport.
bool StelProjector::projectCheck(const Vec3d& v, Vec3d& win) const
{
	return (project(v, win) && checkInViewport(win));
}

//! Project the vector v from the current frame into the viewport.
//! @param v the direction vector in the current frame. Does not need to be normalized.
//! @param win the projected vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
//! @return true if the projected point is inside the viewport.
bool StelProjector::projectCheck(const Vec3f& v, Vec3f& win) const
{
	return (project(v, win) && checkInViewport(win));
}

//! Project the vector v from the viewport frame into the current frame.
//! @param win the vector in the viewport 2D frame. win[0] and win[1] are in screen pixels, win[2] is unused.
//! @param v the unprojected direction vector in the current frame.
//! @return true if the projected coordinate is valid.
bool StelProjector::unProject(const Vec3d& win, Vec3d& v) const
{
	return unProject(win[0], win[1], v);
}

QByteArray StelProjector::getProjectShader() const
{
	static const char*const projectSrc = 1+R"(
#line 1 106
uniform vec2 PROJECTOR_viewportCenter;
uniform vec2 PROJECTOR_flip;
uniform float PROJECTOR_pixelPerRad;
uniform float PROJECTOR_zNear;
uniform float PROJECTOR_oneOverZNearMinusZFar;
vec3 project(vec3 modelSpacePoint)
{
	float flipHorz = PROJECTOR_flip.x, flipVert = PROJECTOR_flip.y;
	vec2  viewportCenter = PROJECTOR_viewportCenter;
	float pixelPerRad = PROJECTOR_pixelPerRad;
	float zNear = PROJECTOR_zNear;
	float oneOverZNearMinusZFar = PROJECTOR_oneOverZNearMinusZFar;

	vec3 worldSpacePoint = modelViewForwardTransform(modelSpacePoint);
	vec3 v = projectorForwardTransform(worldSpacePoint);
	return vec3(viewportCenter[0] + flipHorz * pixelPerRad * v[0],
	            viewportCenter[1] + flipVert * pixelPerRad * v[1],
	            (v[2] - zNear) * oneOverZNearMinusZFar);
}
#line 1 0
)";
	return getForwardTransformShader() + projectSrc;
}

void StelProjector::setProjectUniforms(QOpenGLShaderProgram& program) const
{
	setForwardTransformUniforms(program);
	program.setUniformValue("PROJECTOR_viewportCenter", QVector2D(viewportCenter[0], viewportCenter[1]));
	program.setUniformValue("PROJECTOR_flip", QVector2D(flipHorz, flipVert));
	program.setUniformValue("PROJECTOR_pixelPerRad", GLfloat(pixelPerRad));
	program.setUniformValue("PROJECTOR_zNear", GLfloat(zNear));
	program.setUniformValue("PROJECTOR_oneOverZNearMinusZFar", GLfloat(oneOverZNearMinusZFar));
}

QByteArray StelProjector::getUnProjectShader() const
{
	static const char*const projectSrc = 1+R"(
#line 1 107
uniform vec2 PROJECTOR_viewportCenter;
uniform vec2 PROJECTOR_flip;
uniform float PROJECTOR_pixelPerRad;
vec3 winPosToWorldPos(float x, float y, out bool ok)
{
	float flipHorz = PROJECTOR_flip.x, flipVert = PROJECTOR_flip.y;
	vec2  viewportCenter = PROJECTOR_viewportCenter;
	float pixelPerRad = PROJECTOR_pixelPerRad;

	vec3 v;
	v[0] = flipHorz * (x - viewportCenter[0]) / pixelPerRad;
	v[1] = flipVert * (y - viewportCenter[1]) / pixelPerRad;
	v[2] = 0.;
	return projectorBackwardTransform(v, ok);
}

vec3 unProject(float x, float y, out bool ok)
{
	vec3 worldPos = winPosToWorldPos(x, y, ok);
	// Even when the reprojected point comes from a region of the screen,
	// where nothing is projected to (rval=false), we finish reprojecting.
	// This looks good for atmosphere rendering, and it helps avoiding
	// discontinuities when dragging around with the mouse.

	return modelViewBackwardTransform(worldPos);
}
#line 1 0
)";
	return getBackwardTransformShader() + projectSrc;
}

void StelProjector::setUnProjectUniforms(QOpenGLShaderProgram& program) const
{
	setBackwardTransformUniforms(program);
	program.setUniformValue("PROJECTOR_viewportCenter", QVector2D(viewportCenter[0], viewportCenter[1]));
	program.setUniformValue("PROJECTOR_flip", QVector2D(flipHorz, flipVert));
	program.setUniformValue("PROJECTOR_pixelPerRad", GLfloat(pixelPerRad));
}

void StelProjector::computeBoundingCap()
{
	bool ok = unProject(static_cast<double>(viewportXywh[0]+0.5f*viewportXywh[2]), static_cast<double>(viewportXywh[1]+0.5f*viewportXywh[3]), boundingCap.n);
	// The central point should be at a valid position by definition.
	// When center is offset, this assumption may not hold however.
	Q_ASSERT(ok || (viewportCenterOffset.normSquared()>0) );
	const bool needNormalization = fabs(boundingCap.n.normSquared()-1.)>0.00000001;

	// Now need to determine the aperture
	Vec3d e0,e1,e2,e3,e4,e5;
	const Vec4i& vp = viewportXywh;
	// Saemundsson's inversion formula for atmospheric refraction is not exact, so need some padding in terms of arcseconds
	const double margin = 30000. * MAS2RAD * getPixelPerRadAtCenter();  // 0.5 arcmin

	ok &= unProject(vp[0]-margin, vp[1]-margin, e0);             // e0: bottom left
	ok &= unProject(vp[0]+vp[2]+margin, vp[1]-margin,e1);        // e1: bottom right
	ok &= unProject(vp[0]+vp[2]+margin, vp[1]+vp[3]+margin, e2); // e2: top right
	ok &= unProject(vp[0]-margin, vp[1]+vp[3]+margin, e3);       // e3: top left
	ok &= unProject(vp[0]-margin,vp[1]+vp[3]/2+margin,e4);       // e4: left center
	ok &= unProject(vp[0]+vp[2]+margin,vp[1]+vp[3]/2+margin,e5); // e5: right center
	if (!ok)
	{
		// Some points were in invalid positions, use full sky.
		boundingCap.d = -1.;
		boundingCap.n.set(1,0,0);
		return;
	}
	if (needNormalization)
	{
		boundingCap.n.normalize();
		e0.normalize();
		e1.normalize();
		e2.normalize();
		e3.normalize();
		e4.normalize();
		e5.normalize();
	}
	boundingCap.d = boundingCap.n*e0;
	double h = boundingCap.n*e1;
	if (boundingCap.d > h)
		boundingCap.d=h;
	h = boundingCap.n*e2;
	if (boundingCap.d > h)
		boundingCap.d=h;
	h = boundingCap.n*e3;
	if (boundingCap.d > h)
		boundingCap.d=h;
	h = boundingCap.n*e4;
	if (boundingCap.d > h)
		boundingCap.d=h;
	h = boundingCap.n*e5;
	if (boundingCap.d > h)
		boundingCap.d=h;
}

/*************************************************************************
 Project the vector v from the viewport frame into the current frame
*************************************************************************/
bool StelProjector::unProject(double x, double y, Vec3d &v) const
{
	v[0] = static_cast<double>(flipHorz * static_cast<float>(x - viewportCenter[0]) / pixelPerRad);
	v[1] = static_cast<double>(flipVert * static_cast<float>(y - viewportCenter[1]) / pixelPerRad);
	v[2] = 0;
	const bool rval = backward(v);
	// Even when the reprojected point comes from a region of the screen,
	// where nothing is projected to (rval=false), we finish reprojecting.
	// This looks good for atmosphere rendering, and it helps avoiding
	// discontinuities when dragging around with the mouse.

	modelViewTransform->backward(v);
	return rval;
}

bool StelProjector::projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const

{
	return project(v1, win1) && project(v2, win2) && (checkInViewport(win1) || checkInViewport(win2));
}

//! Get the current model view matrix.
StelProjector::ModelViewTranformP StelProjector::getModelViewTransform() const
{
	return modelViewTransform;
}

//! Get the current projection matrix.
Mat4f StelProjector::getProjectionMatrix() const
{
	return Mat4f(2.f/viewportXywh[2], 0, 0, 0, 0, 2.f/viewportXywh[3], 0, 0, 0, 0, -1., 0., -(2.f*viewportXywh[0] + viewportXywh[2])/viewportXywh[2], -(2.f*viewportXywh[1] + viewportXywh[3])/viewportXywh[3], 0, 1);
}

StelProjector::StelProjectorMaskType StelProjector::getMaskType(void) const
{
	return maskType;
}

bool StelProjector::isSameProjection(const StelProjector& other) const
{
	// *this defines the projection type, modelViewTransform defines how refraction is handled.
	return typeid(*this) == typeid(other) &&
	       typeid(*modelViewTransform) == typeid(*other.modelViewTransform);
}
