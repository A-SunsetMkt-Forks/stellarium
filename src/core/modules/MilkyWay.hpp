/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef MILKYWAY_HPP
#define MILKYWAY_HPP

#include "StelModule.hpp"
#include "VecMath.hpp"
#include "StelCore.hpp"
#include "StelTextureTypes.hpp"

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
//! @class MilkyWay 
//! Manages the displaying of the Milky Way.
class MilkyWay : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagMilkyWayDisplayed
		   READ getFlagShow
		   WRITE setFlagShow
		   NOTIFY milkyWayDisplayedChanged)
	Q_PROPERTY(double intensity
		   READ getIntensity
		   WRITE setIntensity
		   NOTIFY intensityChanged)
	Q_PROPERTY(Vec3f color
		   READ getColor
		   WRITE setColor
		   NOTIFY colorChanged)
	Q_PROPERTY(double saturation
		   READ getSaturation
		   WRITE setSaturation
		   NOTIFY saturationChanged)
public:
	MilkyWay();
	~MilkyWay() override;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the class.  Here we load the texture for the Milky Way and 
	//! get the display settings from application settings, namely the flag which
	//! determines if the Milky Way is displayed or not, and the intensity setting.
	void init() override;

	//! Draw the Milky Way.
	void draw(StelCore* core) override;
	
	//! Update and time-dependent state.  Updates the fade level while the 
	//! Milky way rendering is being changed from on to off or off to on.
	void update(double deltaTime) override;
	
	//! actionDraw returns 1 (because this is background, very early drawing).
	//! Other actions return 0 for no action.
	double getCallOrder(StelModuleActionName actionName) const override;
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Get Milky Way intensity (brightness).
	double getIntensity() const {return intensity;}
	//! Set Milky Way intensity. Default value: 1.
	void setIntensity(double aintensity);

	//! Get Milky Way saturation (color strength).
	double getSaturation()const {return saturation;}
	//! Set Milky Way saturation (color strength).
	void setSaturation(double sat);

	//! Get the color used for rendering the Milky Way. It is modulated by intensity, light pollution and atmospheric extinction.
	Vec3f getColor() const {return color;}
	//! Sets the color to use for rendering the Milky Way
	//! @param c The color to use for rendering the Milky Way. Default (1.0, 1.0, 1.0)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(0.7, 1.0, 0.8);
	//! MilkyWay.setColor(c.toVec3f());
	//! @endcode
	void setColor(const Vec3f& c);
	
	//! Sets whether to show the Milky Way
	void setFlagShow(bool b);
	//! Gets whether the Milky Way is displayed
	bool getFlagShow(void) const;

signals:
	void milkyWayDisplayedChanged(const bool displayed);
	void intensityChanged(double intensity);
	void saturationChanged(double saturation);
	void colorChanged(Vec3f color);

private:
	void setupCurrentVAO();
	void bindVAO();
	void releaseVAO();

private:
	StelTextureSP mainTex;
	Vec3f color; // global color
	double intensity;
	double intensityFovScale; // like for constellations: reduce brightness when zooming in.
	double intensityMinFov;
	double intensityMaxFov;
	class LinearFader* fader;
	double saturation = 1.0;

	struct
	{
		int mainTex;
		int brightness;
		int saturation;
		int bortleIntensity;
		int extinctionEnabled;
		int projectionMatrixInverse;
	} shaderVars;

	std::unique_ptr<QOpenGLVertexArrayObject> vao;
	std::unique_ptr<QOpenGLBuffer> vbo;
	StelProjectorP prevProjector;
	std::unique_ptr<QOpenGLShaderProgram> renderProgram;
};

#endif // MILKYWAY_HPP
