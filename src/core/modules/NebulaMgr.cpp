/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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

// class used to manage groups of Nebulas

#include "StelApp.hpp"
#include "NebulaList.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QDir>
#include <QMessageBox>

// Define version of valid Stellarium DSO Catalog
// This number must be incremented each time the content or file format of the stars catalogs change
const QString NebulaMgr::StellariumDSOCatalogVersion = QStringLiteral("3.21");

namespace
{

void addEnglishOrAliasName(const NebulaP& nebula, const QString& name)
{
	const QString &currentName = nebula->getEnglishName();
	if (currentName.isEmpty()) // Set native name of DSO
		nebula->setEnglishName(name);
	else if (currentName != name) // Add traditional (well-known?) name of DSO as alias
		nebula->addNameAlias(name);
}

}

void NebulaMgr::setLabelsColor(const Vec3f& c) {Nebula::labelColor = c; emit labelsColorChanged(c);}
const Vec3f NebulaMgr::getLabelsColor(void) const {return Nebula::labelColor;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebUnknown, c); emit circlesColorChanged(c); }
const Vec3f NebulaMgr::getCirclesColor(void) const {return Nebula::hintColorMap.value(Nebula::NebUnknown);}
void NebulaMgr::setRegionsColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebRegion, c); emit regionsColorChanged(c); }
const Vec3f NebulaMgr::getRegionsColor(void) const {return Nebula::hintColorMap.value(Nebula::NebRegion);}
void NebulaMgr::setGalaxyColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebGx, c); Nebula::hintColorMap.insert(Nebula::NebPartOfGx, c); emit galaxiesColorChanged(c); }
const Vec3f NebulaMgr::getGalaxyColor(void) const {return Nebula::hintColorMap.value(Nebula::NebGx);}
void NebulaMgr::setRadioGalaxyColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebRGx, c); emit radioGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getRadioGalaxyColor(void) const {return Nebula::hintColorMap.value(Nebula::NebRGx);}
void NebulaMgr::setActiveGalaxyColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebAGx, c); emit activeGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getActiveGalaxyColor(void) const {return Nebula::hintColorMap.value(Nebula::NebAGx);}
void NebulaMgr::setInteractingGalaxyColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebIGx, c); emit interactingGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getInteractingGalaxyColor(void) const {return Nebula::hintColorMap.value(Nebula::NebIGx);}
void NebulaMgr::setQuasarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebQSO, c); emit quasarsColorChanged(c); }
const Vec3f NebulaMgr::getQuasarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebQSO);}
void NebulaMgr::setNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebN, c); Nebula::hintColorMap.insert(Nebula::NebCn, c); emit nebulaeColorChanged(c); }
const Vec3f NebulaMgr::getNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebN);}
void NebulaMgr::setPlanetaryNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebPn, c); emit planetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getPlanetaryNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebPn);}
void NebulaMgr::setReflectionNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebRn, c); emit reflectionNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getReflectionNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebRn);}
void NebulaMgr::setBipolarNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebBn, c); emit bipolarNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getBipolarNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebBn);}
void NebulaMgr::setEmissionNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebEn, c); emit emissionNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getEmissionNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebEn);}
void NebulaMgr::setDarkNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebDn, c); emit darkNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getDarkNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebDn);}
void NebulaMgr::setHydrogenRegionColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebHII, c); emit hydrogenRegionsColorChanged(c);}
const Vec3f NebulaMgr::getHydrogenRegionColor(void) const {return Nebula::hintColorMap.value(Nebula::NebHII);}
void NebulaMgr::setSupernovaRemnantColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSNR, c); emit supernovaRemnantsColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaRemnantColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSNR);}
void NebulaMgr::setSupernovaCandidateColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSNC, c); emit supernovaCandidatesColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaCandidateColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSNC);}
void NebulaMgr::setSupernovaRemnantCandidateColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSNRC, c); emit supernovaRemnantCandidatesColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaRemnantCandidateColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSNRC);}
void NebulaMgr::setInterstellarMatterColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebISM, c); emit interstellarMatterColorChanged(c);}
const Vec3f NebulaMgr::getInterstellarMatterColor(void) const {return Nebula::hintColorMap.value(Nebula::NebISM);}
void NebulaMgr::setClusterColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebCl, c); emit clustersColorChanged(c);}
const Vec3f NebulaMgr::getClusterColor(void) const {return Nebula::hintColorMap.value(Nebula::NebCl);}
void NebulaMgr::setOpenClusterColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebOc, c); emit openClustersColorChanged(c);}
const Vec3f NebulaMgr::getOpenClusterColor(void) const {return Nebula::hintColorMap.value(Nebula::NebOc);}
void NebulaMgr::setGlobularClusterColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebGc, c); emit globularClustersColorChanged(c);}
const Vec3f NebulaMgr::getGlobularClusterColor(void) const {return Nebula::hintColorMap.value(Nebula::NebGc);}
void NebulaMgr::setStellarAssociationColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSA, c); emit stellarAssociationsColorChanged(c);}
const Vec3f NebulaMgr::getStellarAssociationColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSA);}
void NebulaMgr::setStarCloudColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSC, c); emit starCloudsColorChanged(c);}
const Vec3f NebulaMgr::getStarCloudColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSC);}
void NebulaMgr::setEmissionObjectColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebEMO, c); emit emissionObjectsColorChanged(c);}
const Vec3f NebulaMgr::getEmissionObjectColor(void) const {return Nebula::hintColorMap.value(Nebula::NebEMO);}
void NebulaMgr::setBlLacObjectColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebBLL, c); emit blLacObjectsColorChanged(c);}
const Vec3f NebulaMgr::getBlLacObjectColor(void) const {return Nebula::hintColorMap.value(Nebula::NebBLL);}
void NebulaMgr::setBlazarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebBLA, c); emit blazarsColorChanged(c);}
const Vec3f NebulaMgr::getBlazarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebBLA);}
void NebulaMgr::setMolecularCloudColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebMolCld, c); emit molecularCloudsColorChanged(c);}
const Vec3f NebulaMgr::getMolecularCloudColor(void) const {return Nebula::hintColorMap.value(Nebula::NebMolCld);}
void NebulaMgr::setYoungStellarObjectColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebYSO, c); emit youngStellarObjectsColorChanged(c);}
const Vec3f NebulaMgr::getYoungStellarObjectColor(void) const {return Nebula::hintColorMap.value(Nebula::NebYSO);}
void NebulaMgr::setPossibleQuasarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebPossQSO, c); emit possibleQuasarsColorChanged(c);}
const Vec3f NebulaMgr::getPossibleQuasarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebPossQSO);}
void NebulaMgr::setPossiblePlanetaryNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebPossPN, c); emit possiblePlanetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getPossiblePlanetaryNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebPossPN);}
void NebulaMgr::setProtoplanetaryNebulaColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebPPN, c); emit protoplanetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getProtoplanetaryNebulaColor(void) const {return Nebula::hintColorMap.value(Nebula::NebPPN);}
void NebulaMgr::setStarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebStar, c); emit starsColorChanged(c);}
const Vec3f NebulaMgr::getStarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebStar);}
void NebulaMgr::setSymbioticStarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebSymbioticStar, c); emit symbioticStarsColorChanged(c);}
const Vec3f NebulaMgr::getSymbioticStarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebSymbioticStar);}
void NebulaMgr::setEmissionLineStarColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebEmissionLineStar, c); emit emissionLineStarsColorChanged(c);}
const Vec3f NebulaMgr::getEmissionLineStarColor(void) const {return Nebula::hintColorMap.value(Nebula::NebEmissionLineStar);}
void NebulaMgr::setGalaxyClusterColor(const Vec3f& c) {Nebula::hintColorMap.insert(Nebula::NebGxCl, c); emit galaxyClustersColorChanged(c);}
const Vec3f NebulaMgr::getGalaxyClusterColor(void) const {return Nebula::hintColorMap.value(Nebula::NebGxCl);}

bool NebulaMgr::getHintsProportional(void) const {return Nebula::drawHintProportional;}
void NebulaMgr::setHintsProportional(const bool proportional)
{
	if(Nebula::drawHintProportional!=proportional)
	{
		Nebula::drawHintProportional=proportional;
		StelApp::immediateSave("astro/flag_nebula_hints_proportional", proportional);
		emit hintsProportionalChanged(proportional);
	}
}

bool NebulaMgr::getDesignationUsage(void) const {return Nebula::designationUsage; }
void NebulaMgr::setDesignationUsage(const bool flag)
{
	if(Nebula::designationUsage!=flag)
	{
		Nebula::designationUsage=flag;
		StelApp::immediateSave("astro/flag_dso_designation_usage", flag);
		emit designationUsageChanged(flag);
	}
}
bool NebulaMgr::getFlagAdditionalNames(void) const {return Nebula::flagShowAdditionalNames;}
void NebulaMgr::setFlagAdditionalNames(const bool flag)
{
	if(Nebula::flagShowAdditionalNames!=flag)
	{
		Nebula::flagShowAdditionalNames=flag;
		StelApp::immediateSave("astro/flag_dso_additional_names", flag);
		emit flagAdditionalNamesDisplayedChanged(flag);
	}
}

bool NebulaMgr::getFlagOutlines(void) const {return Nebula::flagUseOutlines;}
void NebulaMgr::setFlagOutlines(const bool flag)
{
	if(Nebula::flagUseOutlines!=flag)
	{
		Nebula::flagUseOutlines=flag;
		StelApp::immediateSave("astro/flag_dso_outlines_usage", flag);
		emit flagOutlinesDisplayedChanged(flag);
	}
}

bool NebulaMgr::getFlagShowOnlyNamedDSO(void) const {return Nebula::flagShowOnlyNamedDSO; }
void NebulaMgr::setFlagShowOnlyNamedDSO(const bool flag)
{
	if(Nebula::flagShowOnlyNamedDSO!=flag)
	{
		Nebula::flagShowOnlyNamedDSO=flag;
		StelApp::immediateSave("astro/flag_dso_show_only_named", flag);
		emit flagShowOnlyNamedDSOChanged(flag);
	}
}

NebulaMgr::NebulaMgr(void) : StelObjectModule()
	, nebGrid(200)
	, hintsAmount(0)
	, labelsAmount(0)
	, hintsBrightness(1.0)
	, labelsBrightness(1.0)
	, flagConverter(false)
	, flagDecimalCoordinates(true)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	Nebula::texRegion = StelTextureSP();
	Nebula::texPointElement = StelTextureSP();
	Nebula::texPlanetaryNebula = StelTextureSP();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MilkyWay")->getCallOrder(actionName)+10;
	return 0;
}

// read from stream
void NebulaMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	nebulaFont.setPixelSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), SLOT(setFontSizeFromApp(int)));
    auto& texMan = StelApp::getInstance().getTextureManager();
	// Load dashed shape texture
	Nebula::texRegion		= texMan.createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_reg.png");
	// Load open cluster marker texture
	Nebula::texPointElement		= texMan.createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_point_elem.png");
	// Load planetary nebula marker texture
	Nebula::texPlanetaryNebula	= texMan.createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_pnb.png");
	// Load pointer texture
	texPointer = texMan.createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");

	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setHintsAmount(conf->value("astro/nebula_hints_amount", 3.0).toDouble());
	setLabelsAmount(conf->value("astro/nebula_labels_amount", 3.0).toDouble());
	setHintsProportional(conf->value("astro/flag_nebula_hints_proportional", false).toBool());
	setFlagOutlines(conf->value("astro/flag_dso_outlines_usage", false).toBool());
	setFlagAdditionalNames(conf->value("astro/flag_dso_additional_names",true).toBool());
	setDesignationUsage(conf->value("astro/flag_dso_designation_usage", false).toBool());
	setFlagSurfaceBrightnessUsage(conf->value("astro/flag_surface_brightness_usage", false).toBool());
	setFlagSurfaceBrightnessArcsecUsage(conf->value("gui/flag_surface_brightness_arcsec", false).toBool());
	setFlagSurfaceBrightnessShortNotationUsage(conf->value("gui/flag_surface_brightness_short", false).toBool());
	setFlagShowOnlyNamedDSO(conf->value("astro/flag_dso_show_only_named", false).toBool());

	setFlagSizeLimitsUsage(conf->value("astro/flag_size_limits_usage", false).toBool());
	setMinSizeLimit(conf->value("astro/size_limit_min", 1.0).toDouble());
	setMaxSizeLimit(conf->value("astro/size_limit_max", 600.0).toDouble());

	// Load colors from config file
	// Upgrade config keys
	if (conf->contains("color/nebula_label_color"))
	{
		conf->setValue("color/dso_label_color", conf->value("color/nebula_label_color", "0.4,0.3,0.5").toString());
		conf->remove("color/nebula_label_color");
	}
	if (conf->contains("color/nebula_circle_color"))
	{
		conf->setValue("color/dso_circle_color", conf->value("color/nebula_circle_color", "0.8,0.8,0.1").toString());
		conf->remove("color/nebula_circle_color");
	}
	if (conf->contains("color/nebula_galaxy_color"))
	{
		conf->setValue("color/dso_galaxy_color", conf->value("color/nebula_galaxy_color", "1.0,0.2,0.2").toString());
		conf->remove("color/nebula_galaxy_color");
	}
	if (conf->contains("color/nebula_radioglx_color"))
	{
		conf->setValue("color/dso_radio_galaxy_color", conf->value("color/nebula_radioglx_color", "0.3,0.3,0.3").toString());
		conf->remove("color/nebula_radioglx_color");
	}
	if (conf->contains("color/nebula_activeglx_color"))
	{
		conf->setValue("color/dso_active_galaxy_color", conf->value("color/nebula_activeglx_color", "1.0,0.5,0.2").toString());
		conf->remove("color/nebula_activeglx_color");
	}
	if (conf->contains("color/nebula_intglx_color"))
	{
		conf->setValue("color/dso_interacting_galaxy_color", conf->value("color/nebula_intglx_color", "0.2,0.5,1.0").toString());
		conf->remove("color/nebula_intglx_color");
	}
	if (conf->contains("color/nebula_brightneb_color"))
	{
		conf->setValue("color/dso_nebula_color", conf->value("color/nebula_brightneb_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_brightneb_color");
	}
	if (conf->contains("color/nebula_darkneb_color"))
	{
		conf->setValue("color/dso_dark_nebula_color", conf->value("color/nebula_darkneb_color", "0.3,0.3,0.3").toString());
		conf->remove("color/nebula_darkneb_color");
	}
	if (conf->contains("color/nebula_hregion_color"))
	{
		conf->setValue("color/dso_hydrogen_region_color", conf->value("color/nebula_hregion_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_hregion_color");
	}
	if (conf->contains("color/nebula_snr_color"))
	{
		conf->setValue("color/dso_supernova_remnant_color", conf->value("color/nebula_snr_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_snr_color");
	}
	if (conf->contains("color/nebula_cluster_color"))
	{
		conf->setValue("color/dso_cluster_color", conf->value("color/nebula_cluster_color", "0.8,0.8,0.1").toString());
		conf->remove("color/nebula_cluster_color");
	}

	// Set colors for markers
	setLabelsBrightness(conf->value("astro/nebula_labels_brightness", "1.0").toDouble());
	setHintsBrightness(conf->value("astro/nebula_hints_brightness", "1.0").toDouble());
	setLabelsColor(Vec3f(conf->value("color/dso_label_color", "0.2,0.6,0.7").toString()));
	setCirclesColor(Vec3f(conf->value("color/dso_circle_color", "1.0,0.7,0.2").toString()));
	setRegionsColor(Vec3f(conf->value("color/dso_region_color", "0.7,0.7,0.2").toString()));

	QString defaultGalaxyColor = conf->value("color/dso_galaxy_color", "1.0,0.2,0.2").toString();
	setGalaxyColor(           Vec3f(defaultGalaxyColor));
	setRadioGalaxyColor(      Vec3f(conf->value("color/dso_radio_galaxy_color", "0.3,0.3,0.3").toString()));
	setActiveGalaxyColor(     Vec3f(conf->value("color/dso_active_galaxy_color", "1.0,0.5,0.2").toString()));
	setInteractingGalaxyColor(Vec3f(conf->value("color/dso_interacting_galaxy_color", "0.2,0.5,1.0").toString()));
	setGalaxyClusterColor(    Vec3f(conf->value("color/dso_galaxy_cluster_color", "0.2,0.8,1.0").toString()));
	setQuasarColor(           Vec3f(conf->value("color/dso_quasar_color", defaultGalaxyColor).toString()));
	setPossibleQuasarColor(   Vec3f(conf->value("color/dso_possible_quasar_color", defaultGalaxyColor).toString()));
	setBlLacObjectColor(      Vec3f(conf->value("color/dso_bl_lac_color", defaultGalaxyColor).toString()));
	setBlazarColor(           Vec3f(conf->value("color/dso_blazar_color", defaultGalaxyColor).toString()));

	QString defaultNebulaColor = conf->value("color/dso_nebula_color", "0.1,1.0,0.1").toString();
	setNebulaColor(                   Vec3f(defaultNebulaColor));
	setPlanetaryNebulaColor(          Vec3f(conf->value("color/dso_planetary_nebula_color", defaultNebulaColor).toString()));
	setReflectionNebulaColor(         Vec3f(conf->value("color/dso_reflection_nebula_color", defaultNebulaColor).toString()));
	setBipolarNebulaColor(            Vec3f(conf->value("color/dso_bipolar_nebula_color", defaultNebulaColor).toString()));
	setEmissionNebulaColor(           Vec3f(conf->value("color/dso_emission_nebula_color", defaultNebulaColor).toString()));
	setDarkNebulaColor(               Vec3f(conf->value("color/dso_dark_nebula_color", "0.3,0.3,0.3").toString()));
	setHydrogenRegionColor(           Vec3f(conf->value("color/dso_hydrogen_region_color", defaultNebulaColor).toString()));
	setSupernovaRemnantColor(         Vec3f(conf->value("color/dso_supernova_remnant_color", defaultNebulaColor).toString()));
	setSupernovaCandidateColor(       Vec3f(conf->value("color/dso_supernova_candidate_color", defaultNebulaColor).toString()));
	setSupernovaRemnantCandidateColor(Vec3f(conf->value("color/dso_supernova_remnant_cand_color", defaultNebulaColor).toString()));
	setInterstellarMatterColor(       Vec3f(conf->value("color/dso_interstellar_matter_color", defaultNebulaColor).toString()));
	setMolecularCloudColor(           Vec3f(conf->value("color/dso_molecular_cloud_color", defaultNebulaColor).toString()));
	setPossiblePlanetaryNebulaColor(  Vec3f(conf->value("color/dso_possible_planetary_nebula_color", defaultNebulaColor).toString()));
	setProtoplanetaryNebulaColor(     Vec3f(conf->value("color/dso_protoplanetary_nebula_color", defaultNebulaColor).toString()));

	QString defaultClusterColor = conf->value("color/dso_cluster_color", "1.0,1.0,0.1").toString();
	setClusterColor(           Vec3f(defaultClusterColor));
	setOpenClusterColor(       Vec3f(conf->value("color/dso_open_cluster_color", defaultClusterColor).toString()));
	setGlobularClusterColor(   Vec3f(conf->value("color/dso_globular_cluster_color", defaultClusterColor).toString()));
	setStellarAssociationColor(Vec3f(conf->value("color/dso_stellar_association_color", defaultClusterColor).toString()));
	setStarCloudColor(         Vec3f(conf->value("color/dso_star_cloud_color", defaultClusterColor).toString()));

	QString defaultStellarColor = conf->value("color/dso_star_color", "1.0,0.7,0.2").toString();
	setStarColor(              Vec3f(defaultStellarColor));
	setSymbioticStarColor(     Vec3f(conf->value("color/dso_symbiotic_star_color", defaultStellarColor).toString()));
	setEmissionLineStarColor(  Vec3f(conf->value("color/dso_emission_star_color", defaultStellarColor).toString()));
	setEmissionObjectColor(    Vec3f(conf->value("color/dso_emission_object_color", defaultStellarColor).toString()));
	setYoungStellarObjectColor(Vec3f(conf->value("color/dso_young_stellar_object_color", defaultStellarColor).toString()));

	// Test that all nebula types have colors!
	QMetaEnum nType=QMetaEnum::fromType<Nebula::NebulaType>();
	for(int t=0; t<nType.keyCount(); t++)
	{
		if (!Nebula::hintColorMap.contains(static_cast<Nebula::NebulaType>(nType.value(t))))
				qDebug() << "No color assigned to Nebula Type " << nType.key(t) << ". Please report as bug!";
	}

	// for DSO converter (for developers!)
	flagConverter = conf->value("devel/convert_dso_catalog", false).toBool();
	flagDecimalCoordinates = conf->value("devel/convert_dso_decimal_coord", true).toBool();

	setFlagUseTypeFilters(conf->value("astro/flag_use_type_filter", false).toBool());

	loadCatalogFilters();

	Nebula::TypeGroup typeFilters = Nebula::TypeGroup(Nebula::TypeNone);

	conf->beginGroup("dso_type_filters");
	if (conf->value("flag_show_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeGalaxies;
	if (conf->value("flag_show_active_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeActiveGalaxies;
	if (conf->value("flag_show_interacting_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeInteractingGalaxies;
	if (conf->value("flag_show_open_clusters", true).toBool())
		typeFilters	|= Nebula::TypeOpenStarClusters;
	if (conf->value("flag_show_globular_clusters", true).toBool())
		typeFilters	|= Nebula::TypeGlobularStarClusters;
	if (conf->value("flag_show_bright_nebulae", true).toBool())
		typeFilters	|= Nebula::TypeBrightNebulae;
	if (conf->value("flag_show_dark_nebulae", true).toBool())
		typeFilters	|= Nebula::TypeDarkNebulae;
	if (conf->value("flag_show_planetary_nebulae", true).toBool())
		typeFilters	|= Nebula::TypePlanetaryNebulae;
	if (conf->value("flag_show_hydrogen_regions", true).toBool())
		typeFilters	|= Nebula::TypeHydrogenRegions;
	if (conf->value("flag_show_supernova_remnants", true).toBool())
		typeFilters	|= Nebula::TypeSupernovaRemnants;
	if (conf->value("flag_show_galaxy_clusters", true).toBool())
		typeFilters	|= Nebula::TypeGalaxyClusters;
	if (conf->value("flag_show_other", true).toBool())
		typeFilters	|= Nebula::TypeOther;
	conf->endGroup();

	setTypeFilters(typeFilters);

	// specify which sets get loaded at start time.
	const QString nebulaSetName=conf->value("astro/nebula_set", "default").toString();

	// Load the list of unwanted references.
	const QString allSetsUnwantedRefsRaw = conf->value("astro/nebula_exclude_references").toString(); // Later: read this from config.ini. Syntax: "setName:unrefA,unrefB,..."
	QString allUnwantedReferencesFromOneSetRaw;
	if (allSetsUnwantedRefsRaw.contains(';'))
	{
		// full specification: SetA:refA,refB;SetB:refC
#if (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
		QStringList allSetsUnwantedRefs=allSetsUnwantedRefsRaw.split(';', Qt::SkipEmptyParts);
#else
		QStringList allSetsUnwantedRefs=allSetsUnwantedRefsRaw.split(';', QString::SkipEmptyParts);
#endif
		foreach (const QString &entry, allSetsUnwantedRefs)
		{
			QStringList oneSetList=entry.split(':');
			if (oneSetList.length()==2 && oneSetList.at(0)==nebulaSetName)
			{
				allUnwantedReferencesFromOneSetRaw=oneSetList.at(1); // Now this has the packed list without nebulaSet name
				break;
			}
		}
		if (allUnwantedReferencesFromOneSetRaw.isEmpty())
		{
			qInfo() << "NebulaMgr: FYI: No references found to be excluded for set" << nebulaSetName;
		}
	}
	else if (allSetsUnwantedRefsRaw.contains(':'))
	{
		// Presumably only a packed list for default nebulaSet.
		allUnwantedReferencesFromOneSetRaw=allSetsUnwantedRefsRaw.section(':', 1, 1);
	}
	else
		allUnwantedReferencesFromOneSetRaw=allSetsUnwantedRefsRaw;
	// At this point allUnwantedReferencesFromOneSetRaw is assumed to be a comma-separated list for the current nebulaSet.
#if (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
	unwantedReferences=allUnwantedReferencesFromOneSetRaw.split(',', Qt::SkipEmptyParts);
#else
	unwantedReferences=allUnwantedReferencesFromOneSetRaw.split(',', QString::SkipEmptyParts);
#endif
	if (!unwantedReferences.isEmpty()) // Do not show unwanted references when they does not defined
		qInfo() << "NebulaMgr: Unwanted References:" << unwantedReferences;

	loadNebulaSet(nebulaSetName);

	updateI18n();

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), &StelSkyCultureMgr::currentSkyCultureChanged, this, &NebulaMgr::updateSkyCulture);
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	addAction("actionShow_Nebulas", N_("Display Options"), N_("Deep-sky objects"), "flagHintDisplayed", "D", "N");
	addAction("actionSet_Nebula_TypeFilterUsage", N_("Display Options"), N_("Toggle DSO type filter"), "flagTypeFiltersUsage");
}

void NebulaMgr::selectAllCatalogs()
{
	setCatalogFilters(Nebula::CatAll);
}

void NebulaMgr::selectStandardCatalogs()
{
	Nebula::CatalogGroup catalogs = Nebula::CatNone;
	catalogs |= Nebula::CatNGC;
	catalogs |= Nebula::CatIC;
	catalogs |= Nebula::CatM;
	setCatalogFilters(catalogs);
}

void NebulaMgr::selectNoneCatalogs()
{
	setCatalogFilters(Nebula::CatNone);
}

void NebulaMgr::loadCatalogFilters()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	Nebula::CatalogGroup catalogFilters = Nebula::CatalogGroup(Nebula::CatNone);

	conf->beginGroup("dso_catalog_filters");
	if (conf->value("flag_show_ngc", true).toBool())
		catalogFilters	|= Nebula::CatNGC;
	if (conf->value("flag_show_ic", true).toBool())
		catalogFilters	|= Nebula::CatIC;
	if (conf->value("flag_show_m", true).toBool())
		catalogFilters	|= Nebula::CatM;
	if (conf->value("flag_show_c", false).toBool())
		catalogFilters	|= Nebula::CatC;
	if (conf->value("flag_show_b", false).toBool())
		catalogFilters	|= Nebula::CatB;
	if (conf->value("flag_show_sh2", false).toBool())
		catalogFilters	|= Nebula::CatSh2;
	if (conf->value("flag_show_vdb", false).toBool())
		catalogFilters	|= Nebula::CatVdB;
	if (conf->value("flag_show_lbn", false).toBool())
		catalogFilters	|= Nebula::CatLBN;
	if (conf->value("flag_show_ldn", false).toBool())
		catalogFilters	|= Nebula::CatLDN;
	if (conf->value("flag_show_rcw", false).toBool())
		catalogFilters	|= Nebula::CatRCW;
	if (conf->value("flag_show_cr", false).toBool())
		catalogFilters	|= Nebula::CatCr;
	if (conf->value("flag_show_mel", false).toBool())
		catalogFilters	|= Nebula::CatMel;
	if (conf->value("flag_show_pgc", false).toBool())
		catalogFilters	|= Nebula::CatPGC;
	if (conf->value("flag_show_ced", false).toBool())
		catalogFilters	|= Nebula::CatCed;
	if (conf->value("flag_show_ugc", false).toBool())
		catalogFilters	|= Nebula::CatUGC;
	if (conf->value("flag_show_arp", false).toBool())
		catalogFilters	|= Nebula::CatArp;
	if (conf->value("flag_show_vv", false).toBool())
		catalogFilters	|= Nebula::CatVV;
	if (conf->value("flag_show_pk", false).toBool())
		catalogFilters	|= Nebula::CatPK;
	if (conf->value("flag_show_png", false).toBool())
		catalogFilters	|= Nebula::CatPNG;
	if (conf->value("flag_show_snrg", false).toBool())
		catalogFilters	|= Nebula::CatSNRG;
	if (conf->value("flag_show_aco", false).toBool())
		catalogFilters	|= Nebula::CatACO;
	if (conf->value("flag_show_hcg", false).toBool())
		catalogFilters	|= Nebula::CatHCG;
	if (conf->value("flag_show_eso", false).toBool())
		catalogFilters	|= Nebula::CatESO;
	if (conf->value("flag_show_vdbh", false).toBool())
		catalogFilters	|= Nebula::CatVdBH;
	if (conf->value("flag_show_dwb", false).toBool())
		catalogFilters	|= Nebula::CatDWB;
	if (conf->value("flag_show_tr", false).toBool())
		catalogFilters	|= Nebula::CatTr;
	if (conf->value("flag_show_st", false).toBool())
		catalogFilters	|= Nebula::CatSt;
	if (conf->value("flag_show_ru", false).toBool())
		catalogFilters	|= Nebula::CatRu;
	if (conf->value("flag_show_vdbha", false).toBool())
		catalogFilters	|= Nebula::CatVdBHa;
	if (conf->value("flag_show_other", true).toBool())
		catalogFilters	|= Nebula::CatOther;
	conf->endGroup();

	// NB: nebula set loaded inside setter of catalog filter
	setCatalogFilters(int(catalogFilters));
}

void NebulaMgr::storeCatalogFilters()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// view dialog / DSO tag settings
	const Nebula::CatalogGroup cflags = static_cast<Nebula::CatalogGroup>(getCatalogFilters());

	conf->beginGroup("dso_catalog_filters");
	conf->setValue("flag_show_ngc",		static_cast<bool>(cflags & Nebula::CatNGC));
	conf->setValue("flag_show_ic",		static_cast<bool>(cflags & Nebula::CatIC));
	conf->setValue("flag_show_m",		static_cast<bool>(cflags & Nebula::CatM));
	conf->setValue("flag_show_c",		static_cast<bool>(cflags & Nebula::CatC));
	conf->setValue("flag_show_b",		static_cast<bool>(cflags & Nebula::CatB));
	conf->setValue("flag_show_vdb",		static_cast<bool>(cflags & Nebula::CatVdB));
	conf->setValue("flag_show_sh2",		static_cast<bool>(cflags & Nebula::CatSh2));
	conf->setValue("flag_show_rcw",		static_cast<bool>(cflags & Nebula::CatRCW));
	conf->setValue("flag_show_lbn",		static_cast<bool>(cflags & Nebula::CatLBN));
	conf->setValue("flag_show_ldn",		static_cast<bool>(cflags & Nebula::CatLDN));
	conf->setValue("flag_show_cr",		static_cast<bool>(cflags & Nebula::CatCr));
	conf->setValue("flag_show_mel",		static_cast<bool>(cflags & Nebula::CatMel));
	conf->setValue("flag_show_ced",		static_cast<bool>(cflags & Nebula::CatCed));
	conf->setValue("flag_show_pgc",		static_cast<bool>(cflags & Nebula::CatPGC));
	conf->setValue("flag_show_ugc",		static_cast<bool>(cflags & Nebula::CatUGC));
	conf->setValue("flag_show_arp",		static_cast<bool>(cflags & Nebula::CatArp));
	conf->setValue("flag_show_vv",		static_cast<bool>(cflags & Nebula::CatVV));
	conf->setValue("flag_show_pk",		static_cast<bool>(cflags & Nebula::CatPK));
	conf->setValue("flag_show_png",		static_cast<bool>(cflags & Nebula::CatPNG));
	conf->setValue("flag_show_snrg",		static_cast<bool>(cflags & Nebula::CatSNRG));
	conf->setValue("flag_show_aco",		static_cast<bool>(cflags & Nebula::CatACO));
	conf->setValue("flag_show_hcg",		static_cast<bool>(cflags & Nebula::CatHCG));
	conf->setValue("flag_show_eso",		static_cast<bool>(cflags & Nebula::CatESO));
	conf->setValue("flag_show_vdbh",		static_cast<bool>(cflags & Nebula::CatVdBH));
	conf->setValue("flag_show_dwb",		static_cast<bool>(cflags & Nebula::CatDWB));
	conf->setValue("flag_show_tr",		static_cast<bool>(cflags & Nebula::CatTr));
	conf->setValue("flag_show_st",		static_cast<bool>(cflags & Nebula::CatSt));
	conf->setValue("flag_show_ru",		static_cast<bool>(cflags & Nebula::CatRu));
	conf->setValue("flag_show_vdbha",	static_cast<bool>(cflags & Nebula::CatVdBHa));
	conf->setValue("flag_show_other",		static_cast<bool>(cflags & Nebula::CatOther));
	conf->endGroup();
}

struct DrawNebulaFuncObject
{
	DrawNebulaFuncObject(float amaxMagHints, float amaxMagLabels, StelPainter* p, StelCore* aCore, bool acheckMaxMagHints)
		: maxMagHints(amaxMagHints)
		, maxMagLabels(amaxMagLabels)
		, sPainter(p)
		, core(aCore)
		, checkMaxMagHints(acheckMaxMagHints)
	{
		angularSizeLimit = 5.f/sPainter->getProjector()->getPixelPerRadAtCenter()*M_180_PIf;
	}
	void operator()(StelRegionObject* obj)
	{
		if (checkMaxMagHints)
			return;

		Nebula* n = static_cast<Nebula*>(obj);
		if (!n->objectInDisplayedCatalog())
			return;

		if (n->flagShowOnlyNamedDSO && n->getEnglishName().isEmpty() && n->culturalNames.isEmpty())
			return;

		if (!n->objectInAllowedSizeRangeLimits())
			return;

		float mag=n->getVisibilityLevelByMagnitude();
		StelSkyDrawer *drawer = core->getSkyDrawer();
		// filter out DSOs which are too dim to be seen (e.g. for bino observers)
		if ((drawer->getFlagNebulaMagnitudeLimit()) && (mag > static_cast<float>(drawer->getCustomNebulaMagnitudeLimit())))
			return;

		if (n->majorAxisSize>angularSizeLimit || n->majorAxisSize==0.f || mag <= maxMagHints)
		{
			sPainter->getProjector()->project(n->getJ2000EquatorialPos(core),n->XY);
			n->drawLabel(*sPainter, maxMagLabels);
			n->drawHints(*sPainter, maxMagHints, core);
			n->drawOutlines(*sPainter, maxMagHints);
		}
	}
	float maxMagHints;
	float maxMagLabels;
	StelPainter* sPainter;
	StelCore* core;
	float angularSizeLimit;
	bool checkMaxMagHints;
};

void NebulaMgr::setCatalogFilters(int cflags)
{
	if(cflags != static_cast<int>(Nebula::catalogFilters))
	{
		Nebula::catalogFilters = static_cast<Nebula::CatalogGroup>(cflags);
		emit catalogFiltersChanged(cflags);
	}
}

void NebulaMgr::setTypeFilters(int tflags)
{
	if(tflags != static_cast<int>(Nebula::typeFilters))
	{
		Nebula::typeFilters = static_cast<Nebula::TypeGroup>(tflags);
		emit typeFiltersChanged(tflags);
	}
}

void NebulaMgr::setFlagSurfaceBrightnessUsage(const bool usage)
{
	if (usage!=Nebula::surfaceBrightnessUsage)
	{
		Nebula::surfaceBrightnessUsage=usage;
		StelApp::immediateSave("astro/flag_surface_brightness_usage", Nebula::surfaceBrightnessUsage);
		emit flagSurfaceBrightnessUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessUsage(void) const
{
	return Nebula::surfaceBrightnessUsage;
}

void NebulaMgr::setFlagSurfaceBrightnessArcsecUsage(const bool usage)
{
	if (usage!=Nebula::flagUseArcsecSurfaceBrightness)
	{
		Nebula::flagUseArcsecSurfaceBrightness=usage;
		StelApp::immediateSave("gui/flag_surface_brightness_arcsec", Nebula::flagUseArcsecSurfaceBrightness);
		emit flagSurfaceBrightnessArcsecUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessArcsecUsage(void) const
{
	return Nebula::flagUseArcsecSurfaceBrightness;
}

void NebulaMgr::setFlagSurfaceBrightnessShortNotationUsage(const bool usage)
{
	if (usage!=Nebula::flagUseShortNotationSurfaceBrightness)
	{
		Nebula::flagUseShortNotationSurfaceBrightness=usage;
		StelApp::immediateSave("gui/flag_surface_brightness_short", Nebula::flagUseShortNotationSurfaceBrightness);
		emit flagSurfaceBrightnessShortNotationUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessShortNotationUsage(void) const
{
	return Nebula::flagUseShortNotationSurfaceBrightness;
}

void NebulaMgr::setFlagSizeLimitsUsage(const bool usage)
{
	if (usage!=Nebula::flagUseSizeLimits)
	{
		Nebula::flagUseSizeLimits=usage;
		StelApp::immediateSave("astro/flag_size_limits_usage", Nebula::flagUseSizeLimits);
		emit flagSizeLimitsUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSizeLimitsUsage(void) const
{
	return Nebula::flagUseSizeLimits;
}

void NebulaMgr::setFlagUseTypeFilters(const bool b)
{
	if (Nebula::flagUseTypeFilters!=b)
	{
		Nebula::flagUseTypeFilters=b;
		StelApp::immediateSave("astro/flag_use_type_filter", Nebula::flagUseTypeFilters);
		emit flagUseTypeFiltersChanged(b);
	}
}

bool NebulaMgr::getFlagUseTypeFilters(void) const
{
	return Nebula::flagUseTypeFilters;
}

void NebulaMgr::setLabelsAmount(double a)
{
	if((a-labelsAmount) != 0.)
	{
		labelsAmount=a;
		StelApp::immediateSave("astro/nebula_labels_amount", labelsAmount);
		emit labelsAmountChanged(a);
	}
}

double NebulaMgr::getLabelsAmount(void) const
{
	return labelsAmount;
}

void NebulaMgr::setHintsAmount(double f)
{
	if((hintsAmount-f) != 0.)
	{
		hintsAmount = f;
		StelApp::immediateSave("astro/nebula_hints_amount", hintsAmount);
		emit hintsAmountChanged(f);
	}
}

double NebulaMgr::getHintsAmount(void) const
{
	return hintsAmount;
}

void NebulaMgr::setLabelsBrightness(double b)
{
	if (b!=labelsBrightness)
	{
		labelsBrightness = b;
		StelApp::immediateSave("astro/nebula_labels_brightness", labelsBrightness);
		emit labelsBrightnessChanged(b);
	}
}

double NebulaMgr::getLabelsBrightness(void) const
{
	return labelsBrightness;
}

void NebulaMgr::setHintsBrightness(double b)
{
	if (b!=hintsBrightness)
	{
		hintsBrightness = b;
		StelApp::immediateSave("astro/nebula_hints_brightness", hintsBrightness);
		emit hintsBrightnessChanged(b);
	}
}

double NebulaMgr::getHintsBrightness(void) const
{
	return hintsBrightness;
}


void NebulaMgr::setMinSizeLimit(double s)
{
	if((Nebula::minSizeLimit-s) != 0.)
	{
		Nebula::minSizeLimit = s;
		StelApp::immediateSave("astro/size_limit_min", Nebula::minSizeLimit);
		emit minSizeLimitChanged(s);
	}
}

double NebulaMgr::getMinSizeLimit() const
{
	return Nebula::minSizeLimit;
}

void NebulaMgr::setMaxSizeLimit(double s)
{
	if((Nebula::maxSizeLimit-s) != 0.)
	{
		Nebula::maxSizeLimit = s;
		StelApp::immediateSave("astro/size_limit_max", Nebula::maxSizeLimit);
		emit maxSizeLimitChanged(s);
	}
}

double NebulaMgr::getMaxSizeLimit() const
{
	return Nebula::maxSizeLimit;
}

float NebulaMgr::computeMaxMagHint(const StelSkyDrawer* skyDrawer) const
{
	return skyDrawer->getLimitMagnitude()*1.2f-2.f+static_cast<float>(hintsAmount *1.)-2.f;
}

// Draw all the Nebulae and call drawing the pointer if needed
void NebulaMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(nebulaFont);

	if(hintsFader.getInterstate()>0.f)
	{
		static StelSkyDrawer* skyDrawer = core->getSkyDrawer();

		Nebula::hintsBrightness  = hintsFader.getInterstate()*flagShow.getInterstate()*static_cast<float>(hintsBrightness);
		Nebula::labelsBrightness = hintsFader.getInterstate()*flagShow.getInterstate()*static_cast<float>(labelsBrightness);

		// Use a 4 degree margin (esp. for wide outlines)
		const float margin = 4.f*M_PI_180f*prj->getPixelPerRadAtCenter();
		const SphericalRegionP& p = prj->getViewportConvexPolygon(margin, margin);

		// Print all the nebulae of all the selected zones
		float maxMagHints  = computeMaxMagHint(skyDrawer);
		float maxMagLabels = skyDrawer->getLimitMagnitude()-2.f+static_cast<float>(labelsAmount*1.2)-2.f;
		DrawNebulaFuncObject func(maxMagHints, maxMagLabels, &sPainter, core, hintsFader.getInterstate()<=0.f);
		nebGrid.processIntersectingPointInRegions(p.data(), func);
	}

	static StelObjectMgr *som=GETSTELMODULE(StelObjectMgr);
	if (som->getFlagSelectedObjectPointer())
		drawPointer(core, sPainter);
}

void NebulaMgr::drawPointer(const StelCore* core, StelPainter& sPainter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		// Compute 2D pos and return if outside screen
		if (!prj->projectInPlace(pos)) return;
		sPainter.setColor(0.4f,0.5f,0.8f);

		texPointer->bind();

		sPainter.setBlending(true);

		// Size on screen
		float screenRd = static_cast<float>(obj->getAngularRadius(core))*M_PI_180f*prj->getPixelPerRadAtCenter();
		if (screenRd>120.f) // avoid oversized marker
			screenRd = 120.f;

		if (Nebula::drawHintProportional)
			screenRd*=1.2f;
		screenRd+=20.f + 10.f*std::sin(3.f * static_cast<float>(StelApp::getInstance().getAnimationTime()));
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])-screenRd*0.5f, static_cast<float>(pos[1])-screenRd*0.5f, 10, 90);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])-screenRd*0.5f, static_cast<float>(pos[1])+screenRd*0.5f, 10, 0);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])+screenRd*0.5f, static_cast<float>(pos[1])+screenRd*0.5f, 10, -90);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])+screenRd*0.5f, static_cast<float>(pos[1])-screenRd*0.5f, 10, -180);
	}
}

// Search by name
NebulaP NebulaMgr::searchForCommonName(const QString& name)
{
	QString uname = name.toUpper();

	if (const auto it = commonNameIndex.find(uname); it != commonNameIndex.end())
		return (it->nebula);

	return searchByDesignation(uname);
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	QString srcCatalogPath	= StelFileMgr::findFile("nebulae/" + setName + "/catalog.txt");
	QString dsoCatalogPath	= StelFileMgr::findFile("nebulae/" + setName + "/catalog-" + StellariumDSOCatalogVersion + ".dat");
	if (dsoCatalogPath.isEmpty()) // Extended edition is not exist, let's try find standard edition
		dsoCatalogPath	= StelFileMgr::findFile("nebulae/" + setName + "/catalog.dat");
	QString dsoOutlinesPath	= StelFileMgr::findFile("nebulae/" + setName + "/outlines.dat");
	QString dsoDiscoveryPath = StelFileMgr::findFile("nebulae/" + setName + "/discovery.dat");
	QString dsoNamesPath = StelFileMgr::findFile("nebulae/" + setName + "/names.dat");

	dsoArray.clear();
	dsoIndex.clear();
	nebGrid.clear();

	if (flagConverter)
	{
		if (!srcCatalogPath.isEmpty())
			convertDSOCatalog(srcCatalogPath, StelFileMgr::findFile("nebulae/" + setName + "/catalog.pack", StelFileMgr::New), flagDecimalCoordinates);
		else
			qWarning() << "ERROR converting catalogue, because source data set does not exist for " << setName;
	}

	if (dsoCatalogPath.isEmpty())
	{
		qWarning() << "ERROR while loading deep-sky catalog data set " << setName;
		return;
	}

	loadDSOCatalog(dsoCatalogPath);

	if (dsoNamesPath.isEmpty())
	{
		qWarning().noquote() << "ERROR while loading deep-sky names data set" << setName;
	}
	else
	{
		loadDSONames(dsoNamesPath);
	}

	if (!dsoOutlinesPath.isEmpty())
		loadDSOOutlines(dsoOutlinesPath);

	if (!dsoDiscoveryPath.isEmpty())
		loadDSODiscoveryData(dsoDiscoveryPath);
}

// Look for a nebula by XYZ coords
NebulaP NebulaMgr::search(const Vec3d& apos)
{
	Vec3d pos(apos);
	pos.normalize();
	NebulaP plusProche;
	double anglePlusProche=0.0;
	for (const auto& n : std::as_const(dsoArray))
	{
		if (n->XYZ*pos>anglePlusProche)
		{
			anglePlusProche=n->XYZ*pos;
			plusProche=n;
		}
	}
	if (anglePlusProche>0.999) // object within ~2.5 degrees
	{
		return plusProche;
	}
	else return NebulaP();
}


QList<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;
	if (!getFlagShow())
		return result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;
	for (const auto& n : std::as_const(dsoArray))
	{
		equPos = n->XYZ;
		equPos.normalize();
		if (equPos*v >= cosLimFov)
		{
			result.push_back(qSharedPointerCast<StelObject>(n));
		}
	}
	return result;
}

NebulaP NebulaMgr::searchDSO(unsigned int DSO) const
{
	if (dsoIndex.contains(DSO))
		return dsoIndex[DSO];
	return NebulaP();
}


NebulaP NebulaMgr::searchM(unsigned int M) const
{
	for (const auto& n : dsoArray)
		if (n->M_nb == M)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchNGC(unsigned int NGC) const
{
	for (const auto& n : dsoArray)
		if (n->NGC_nb == NGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchIC(unsigned int IC) const
{
	for (const auto& n : dsoArray)
		if (n->IC_nb == IC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchC(unsigned int C) const
{
	for (const auto& n : dsoArray)
		if (n->C_nb == C)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchB(unsigned int B) const
{
	for (const auto& n : dsoArray)
		if (n->B_nb == B)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSh2(unsigned int Sh2) const
{
	for (const auto& n : dsoArray)
		if (n->Sh2_nb == Sh2)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdB(unsigned int VdB) const
{
	for (const auto& n : dsoArray)
		if (n->VdB_nb == VdB)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdBHa(unsigned int VdBHa) const
{
	for (const auto& n : dsoArray)
		if (n->VdBHa_nb == VdBHa)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchRCW(unsigned int RCW) const
{
	for (const auto& n : dsoArray)
		if (n->RCW_nb == RCW)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLDN(unsigned int LDN) const
{
	for (const auto& n : dsoArray)
		if (n->LDN_nb == LDN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLBN(unsigned int LBN) const
{
	for (const auto& n : dsoArray)
		if (n->LBN_nb == LBN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchCr(unsigned int Cr) const
{
	for (const auto& n : dsoArray)
		if (n->Cr_nb == Cr)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchMel(unsigned int Mel) const
{
	for (const auto& n : dsoArray)
		if (n->Mel_nb == Mel)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPGC(unsigned int PGC) const
{
	for (const auto& n : dsoArray)
		if (n->PGC_nb == PGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchUGC(unsigned int UGC) const
{
	for (const auto& n : dsoArray)
		if (n->UGC_nb == UGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchCed(QString Ced) const
{
	Ced = Ced.trimmed();
	for (const auto& n : dsoArray)
		if (n->Ced_nb.compare(Ced, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchArp(unsigned int Arp) const
{
	for (const auto& n : dsoArray)
		if (n->Arp_nb == Arp)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVV(unsigned int VV) const
{
	for (const auto& n : dsoArray)
		if (n->VV_nb == VV)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPK(QString PK) const
{
	PK = PK.trimmed();
	for (const auto& n : dsoArray)
		if (n->PK_nb.compare(PK, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPNG(QString PNG) const
{
	PNG = PNG.trimmed();
	for (const auto& n : dsoArray)
		if (n->PNG_nb.compare(PNG, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSNRG(QString SNRG) const
{
	SNRG = SNRG.trimmed();
	for (const auto& n : dsoArray)
		if (n->SNRG_nb.compare(SNRG, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchACO(QString ACO) const
{
	ACO = ACO.trimmed();
	for (const auto& n : dsoArray)
		if (n->ACO_nb.compare(ACO, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchHCG(QString HCG) const
{
	HCG = HCG.trimmed();
	for (const auto& n : dsoArray)
		if (n->HCG_nb.compare(HCG, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchESO(QString ESO) const
{
	ESO = ESO.trimmed();
	for (const auto& n : dsoArray)
		if (n->ESO_nb.compare(ESO, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdBH(QString VdBH) const
{
	VdBH = VdBH.trimmed();
	for (const auto& n : dsoArray)
		if (n->VdBH_nb.compare(VdBH, Qt::CaseInsensitive) == 0)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchDWB(unsigned int DWB) const
{
	for (const auto& n : dsoArray)
		if (n->DWB_nb == DWB)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchTr(unsigned int Tr) const
{
	for (const auto& n : dsoArray)
		if (n->Tr_nb == Tr)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSt(unsigned int St) const
{
	for (const auto& n : dsoArray)
		if (n->St_nb == St)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchRu(unsigned int Ru) const
{
	for (const auto& n : dsoArray)
		if (n->Ru_nb == Ru)
			return n;
	return NebulaP();
}

QString NebulaMgr::getLatestSelectedDSODesignation() const
{
	QString result = "";

	const QList<StelObjectP> selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!selected.empty())
	{
		for (const auto& n : dsoArray)
			if (n==selected[0])
				result = n->getDSODesignation(); // Get designation for latest selected DSO
	}

	return result;
}

QString NebulaMgr::getLatestSelectedDSODesignationWIC() const
{
	QString result = "";

	const QList<StelObjectP> selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!selected.empty())
	{
		for (const auto& n : dsoArray)
			if (n==selected[0])
				result = n->getDSODesignationWIC(); // Get designation for latest selected DSO
	}

	return result;
}

void NebulaMgr::convertDSOCatalog(const QString &in, const QString &out, bool decimal=false)
{
	QFile dsoIn(in);
	if (!dsoIn.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QFile dsoOut(out);
	if (!dsoOut.open(QIODevice::WriteOnly))
	{
		qDebug() << "Error converting DSO data! Cannot open file" << QDir::toNativeSeparators(out);
		return;
	}

	int totalRecords=0;
	QString record;
	while (!dsoIn.atEnd())
	{
		dsoIn.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	dsoIn.seek(0);

	QDataStream dsoOutStream(&dsoOut);
	dsoOutStream.setVersion(QDataStream::Qt_5_2);

	int readOk = 0;				// how many records were read without problems
	while (!dsoIn.atEnd())
	{
		record = QString::fromUtf8(dsoIn.readLine());

		static const QRegularExpression version("ersion\\s+([\\d\\.]+)\\s+(\\w+)");
		QRegularExpressionMatch versionMatch;
		int vp = record.indexOf(version, 0, &versionMatch);
		if (vp!=-1) // Version of catalog, a first line!
			dsoOutStream << versionMatch.captured(1).trimmed() << versionMatch.captured(2).trimmed();

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
		{
			--totalRecords;
			continue;
		}

		if (!record.isEmpty())
		{
			#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
			QStringList list=record.split("\t", Qt::KeepEmptyParts);
			#else
			QStringList list=record.split("\t", QString::KeepEmptyParts);
			#endif

			int id                 = list.at(0).toInt();    // ID (inner identification number)
			QString ra             = list.at(1).trimmed();
			QString dec            = list.at(2).trimmed();
			float bMag             = list.at(3).toFloat();  // B magnitude
			float vMag             = list.at(4).toFloat();  // V magnitude
			QString oType          = list.at(5).trimmed();  // Object type
			QString mType          = list.at(6).trimmed();  // Morphological type of object
			float majorAxisSize    = list.at(7).toFloat();  // major axis size (arcmin)
			float minorAxisSize    = list.at(8).toFloat();  // minor axis size (arcmin)
			int orientationAngle   = list.at(9).toInt();    // orientation angle (degrees)
			float z                = list.at(10).toFloat(); // redshift
			float zErr             = list.at(11).toFloat(); // error of redshift
			float plx              = list.at(12).toFloat(); // parallax (mas)
			float plxErr           = list.at(13).toFloat(); // error of parallax (mas)
			float dist             = list.at(14).toFloat(); // distance (Mpc for galaxies, kpc for other objects)
			float distErr          = list.at(15).toFloat(); // distance error (Mpc for galaxies, kpc for other objects)
			// -----------------------------------------------
			// cross-identification data
			// -----------------------------------------------
			int NGC                = list.at(16).toInt();   // NGC number
			int IC                 = list.at(17).toInt();   // IC number
			int M                  = list.at(18).toInt();   // M number
			int C                  = list.at(19).toInt();   // C number
			int B                  = list.at(20).toInt();   // B number
			int Sh2                = list.at(21).toInt();   // Sh2 number
			int VdB                = list.at(22).toInt();   // VdB number
			int RCW                = list.at(23).toInt();   // RCW number
			int LDN                = list.at(24).toInt();   // LDN number
			int LBN                = list.at(25).toInt();   // LBN number
			int Cr                 = list.at(26).toInt();   // Cr number (alias: Col)
			int Mel                = list.at(27).toInt();   // Mel number
			int PGC                = list.at(28).toInt();   // PGC number (subset)
			int UGC                = list.at(29).toInt();   // UGC number (subset)
			QString Ced            = list.at(30).trimmed();	// Ced number
			int Arp                = list.at(31).toInt();   // Arp number
			int VV                 = list.at(32).toInt();   // VV number
			QString PK             = list.at(33).trimmed(); // PK number
			QString PNG            = list.at(34).trimmed(); // PN G number
			QString SNRG           = list.at(35).trimmed(); // SNR G number
			QString ACO            = list.at(36).trimmed(); // ACO number
			QString HCG            = list.at(37).trimmed(); // HCG number
			QString ESO            = list.at(38).trimmed(); // ESO number
			QString VdBH           = list.at(39).trimmed(); // VdBH number
			int DWB                = list.at(40).toInt();   // DWB number
			int Tr                 = list.at(41).toInt();   // Tr number
			int St                 = list.at(42).toInt();   // St number
			int Ru                 = list.at(43).toInt();   // Ru number
			int VdBHa              = list.at(44).toInt();   // VdB-Ha number

			float raRad, decRad;
			if (decimal)
			{
				// Convert from deg to rad
				raRad	= ra.toFloat() *M_PI_180f;
				decRad	= dec.toFloat()*M_PI_180f;
			}
			else
			{
				QStringList raLst;
				if (ra.contains(":"))
					raLst	= ra.split(":");
				else
					raLst	= ra.split(" ");

				QStringList decLst;
				if (dec.contains(":"))
					decLst = dec.split(":");
				else
					decLst = dec.split(" ");

				raRad	= raLst.at(0).toFloat() + raLst.at(1).toFloat()/60.f + raLst.at(2).toFloat()/3600.f;
				decRad	= qAbs(decLst.at(0).toFloat()) + decLst.at(1).toFloat()/60.f + decLst.at(2).toFloat()/3600.f;
				if (dec.startsWith("-")) decRad *= -1.f;

				raRad  *= M_PIf/12.f;	// Convert from hours to rad
				decRad *= M_PIf/180.f;    // Convert from deg to rad
			}

			majorAxisSize /= 60.f;	// Convert from arcmin to degrees
			minorAxisSize /= 60.f;	// Convert from arcmin to degrees

			// Warning: Hyades and LMC has visual magnitude less than 1.0 (0.5^m and 0.9^m)
			if (bMag <= 0.f) bMag = 99.f;
			if (vMag <= 0.f) vMag = 99.f;
			static const QHash<QString, Nebula::NebulaType> oTypesHash({
				{ "G"   , Nebula::NebGx  },
				{ "GX"  , Nebula::NebGx  },
				{ "GC"  , Nebula::NebGc  },
				{ "OC"  , Nebula::NebOc  },
				{ "NB"  , Nebula::NebN   },
				{ "PN"  , Nebula::NebPn  },
				{ "DN"  , Nebula::NebDn  },
				{ "RN"  , Nebula::NebRn  },
				{ "C+N" , Nebula::NebCn  },
				{ "RNE" , Nebula::NebRn  },
				{ "HII" , Nebula::NebHII },
				{ "SNR" , Nebula::NebSNR },
				{ "BN"  , Nebula::NebBn  },
				{ "EN"  , Nebula::NebEn  },
				{ "SA"  , Nebula::NebSA  },
				{ "SC"  , Nebula::NebSC  },
				{ "CL"  , Nebula::NebCl  },
				{ "IG"  , Nebula::NebIGx },
				{ "RG"  , Nebula::NebRGx },
				{ "AGX" , Nebula::NebAGx },
				{ "QSO" , Nebula::NebQSO },
				{ "ISM" , Nebula::NebISM },
				{ "EMO" , Nebula::NebEMO },
				{ "GNE" , Nebula::NebHII },
				{ "RAD" , Nebula::NebISM },
				{ "LIN" , Nebula::NebAGx },// LINER-type active galaxies
				{ "BLL" , Nebula::NebBLL },
				{ "BLA" , Nebula::NebBLA },
				{ "MOC" , Nebula::NebMolCld },
				{ "YSO" , Nebula::NebYSO },
				{ "Q?"  , Nebula::NebPossQSO },
				{ "PN?" , Nebula::NebPossPN },
				{ "*"   , Nebula::NebStar},
				{ "SFR" , Nebula::NebMolCld },
				{ "IR"  , Nebula::NebDn  },
				{ "**"  , Nebula::NebStar},
				{ "MUL" , Nebula::NebStar},
				{ "PPN" , Nebula::NebPPN },
				{ "GIG" , Nebula::NebIGx },
				{ "OPC" , Nebula::NebOc  },
				{ "MGR" , Nebula::NebSA  },
				{ "IG2" , Nebula::NebIGx },
				{ "IG3" , Nebula::NebIGx },
				{ "SY*" , Nebula::NebSymbioticStar},
				{ "PA*" , Nebula::NebPPN },
				{ "CV*" , Nebula::NebStar},
				{ "Y*?" , Nebula::NebYSO },
				{ "CGB" , Nebula::NebISM },
				{ "SNRG", Nebula::NebSNR },
				{ "Y*O" , Nebula::NebYSO },
				{ "SR*" , Nebula::NebStar},
				{ "EM*" , Nebula::NebEmissionLineStar },
				{ "AB*" , Nebula::NebStar },
				{ "MI*" , Nebula::NebStar },
				{ "MI?" , Nebula::NebStar },
				{ "TT*" , Nebula::NebStar },
				{ "WR*" , Nebula::NebStar },
				{ "C*"  , Nebula::NebEmissionLineStar },
				{ "WD*" , Nebula::NebStar },
				{ "EL*" , Nebula::NebStar },
				{ "NL*" , Nebula::NebStar },
				{ "NO*" , Nebula::NebStar },
				{ "HS*" , Nebula::NebStar },
				{ "LP*" , Nebula::NebStar },
				{ "OH*" , Nebula::NebStar },
				{ "S?R" , Nebula::NebStar },
				{ "IR*" , Nebula::NebStar },
				{ "POC" , Nebula::NebMolCld },
				{ "PNB" , Nebula::NebPn   },
				{ "GXCL", Nebula::NebGxCl },
				{ "AL*" , Nebula::NebStar },
				{ "PR*" , Nebula::NebStar },
				{ "RS*" , Nebula::NebStar },
				{ "S*B" , Nebula::NebStar },
				{ "SN?" , Nebula::NebSNC  },
				{ "SR?" , Nebula::NebSNRC },
				{ "DNE" , Nebula::NebDn   },
				{ "RG*" , Nebula::NebStar },
				{ "PSR" , Nebula::NebSNR  },
				{ "HH"  , Nebula::NebISM  },
				{ "V*"  , Nebula::NebStar },
				{ "*IN" , Nebula::NebCn   },
				{ "SN*" , Nebula::NebStar },
				{ "PA?" , Nebula::NebPPN  },
				{ "BUB" , Nebula::NebISM  },
				{ "CLG" , Nebula::NebGxCl },
				{ "POG" , Nebula::NebPartOfGx },
				{ "CGG" , Nebula::NebGxCl },
				{ "SCG" , Nebula::NebGxCl },
				{ "REG" , Nebula::NebRegion },
				{ "?" , Nebula::NebUnknown }
			});

			Nebula::NebulaType nType=oTypesHash.value(oType.toUpper(), Nebula::NebUnknown);
			if (nType == Nebula::NebUnknown)
				qDebug() << "Record with ID" << id <<"has unknown type of object:" << oType;

			++readOk;

			dsoOutStream << id << raRad << decRad << bMag << vMag << static_cast<unsigned int>(nType) << mType << majorAxisSize << minorAxisSize
				     << orientationAngle << z << zErr << plx << plxErr << dist  << distErr << NGC << IC << M << C
				     << B << Sh2 << VdB << RCW  << LDN << LBN << Cr << Mel << PGC << UGC << Ced << Arp << VV << PK
				     << PNG << SNRG << ACO << HCG << ESO << VdBH << DWB << Tr << St << Ru << VdBHa;
		}
	}
	dsoIn.close();
	dsoOut.flush();
	dsoOut.close();
	qInfo().noquote() << "Converted" << readOk << "/" << totalRecords << "DSO records";
	qInfo().noquote() << "[...] Please use 'gzip -nc catalog.pack > catalog.dat' to pack the catalog.";
}

bool NebulaMgr::loadDSOCatalog(const QString &filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly))
		return false;

	qInfo().noquote() << "Loading DSO data ...";

	// Let's begin use gzipped data
	QDataStream ins(StelUtils::uncompress(in.readAll()));
	ins.setVersion(QDataStream::Qt_5_2);

	QString version = "", edition= "";
	int totalRecords=0;
	while (!ins.atEnd())
	{
		if (totalRecords==0) // Read the version of catalog
		{
			ins >> version >> edition;
			if (version.isEmpty())
				version = "3.1"; // The first version of extended edition of the catalog
			if (edition.isEmpty())
				edition = "unknown";
			qInfo().noquote() << "[...]" << QString("Stellarium DSO Catalog, version %1 (%2 edition)").arg(version, edition);
			if (StelUtils::compareVersions(version, StellariumDSOCatalogVersion)!=0)
			{
				++totalRecords;
				qWarning().noquote() << "Mismatch of DSO catalog version (" << version << ")! The expected version is" << StellariumDSOCatalogVersion;
				qWarning().noquote() << "See section 5.5 of the User Guide and install the right version of the catalog!";
				QMessageBox::warning(&StelMainView::getInstance(), q_("Attention!"),
				                     QString("%1. %2: %3 - %4: %5. %6").arg(q_("DSO catalog version mismatch"),
				                                                            q_("Found"),
				                                                            version,
				                                                            q_("Expected"),
				                                                            StellariumDSOCatalogVersion,
				                                                            q_("See Logfile for instructions.")), QMessageBox::Ok);
				break;
			}
		}
		else
		{
			// Create a new Nebula record
			NebulaP e = NebulaP(new Nebula);
			e->readDSO(ins);

			dsoArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			if (e->DSO_nb!=0)
				dsoIndex.insert(e->DSO_nb, e);
		}
		++totalRecords;
	}
	in.close();
	qInfo().noquote() << "Loaded" << --totalRecords << "DSO records";
	return true;
}

bool NebulaMgr::loadDSONames(const QString &filename)
{
	qInfo() << "Loading DSO name data ...";

	commonNameMap.clear();

	QFile dsoNameFile(filename);
	if (!dsoNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "DSO name data file" << QDir::toNativeSeparators(filename) << "not found.";
		return false;
	}

	// Read the names of the deep-sky objects
	QString name, record, ref, cdes;
	QStringList nodata;
	nodata.clear();
	int totalRecords=0;
	int readOk=0;
	unsigned int nb;
	NebulaP e;
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	while (!dsoNameFile.atEnd())
	{
		record = QString::fromUtf8(dsoNameFile.readLine());
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		// bytes 1 - 5, designator for catalogue (prefix)
		ref  = record.left(5).trimmed();
		// bytes 6 -20, identificator for object in the catalog
		cdes = record.mid(5, 15).trimmed().toUpper();
		// bytes 21-80, proper name of the object (translatable)
		name = record.mid(21).trimmed(); // Let gets the name with trimmed whitespaces

		nb = cdes.toUInt();

		static const QStringList catalogs = {
			"IC",    "M",   "C",  "CR",  "MEL",   "B", "SH2", "VDB", "RCW",  "LDN",
			"LBN", "NGC", "PGC", "UGC",  "CED", "ARP",  "VV",  "PK", "PNG", "SNRG",
			"ACO", "HCG", "ESO", "VDBH", "DWB", "TR", "ST", "RU", "DBHA"};

		switch (catalogs.indexOf(ref.toUpper()))
		{
			case 0:
				e = searchIC(nb);
				break;
			case 1:
				e = searchM(nb);
				break;
			case 2:
				e = searchC(nb);
				break;
			case 3:
				e = searchCr(nb);
				break;
			case 4:
				e = searchMel(nb);
				break;
			case 5:
				e = searchB(nb);
				break;
			case 6:
				e = searchSh2(nb);
				break;
			case 7:
				e = searchVdB(nb);
				break;
			case 8:
				e = searchRCW(nb);
				break;
			case 9:
				e = searchLDN(nb);
				break;
			case 10:
				e = searchLBN(nb);
				break;
			case 11:
				e = searchNGC(nb);
				break;
			case 12:
				e = searchPGC(nb);
				break;
			case 13:
				e = searchUGC(nb);
				break;
			case 14:
				e = searchCed(cdes);
				break;
			case 15:
				e = searchArp(nb);
				break;
			case 16:
				e = searchVV(nb);
				break;
			case 17:
				e = searchPK(cdes);
				break;
			case 18:
				e = searchPNG(cdes);
				break;
			case 19:
				e = searchSNRG(cdes);
				break;
			case 20:
				e = searchACO(cdes);
				break;
			case 21:
				e = searchHCG(cdes);
				break;
			case 22:
				e = searchESO(cdes);
				break;
			case 23:
				e = searchVdBH(cdes);
				break;
			case 24:
				e = searchDWB(nb);
				break;
			case 25:
				e = searchTr(nb);
				break;
			case 26:
				e = searchSt(nb);
				break;
			case 27:
				e = searchRu(nb);
				break;
			case 28:
				e = searchVdBHa(nb);
				break;
			default:
				e = searchDSO(nb);
				break;
		}

		if (!e.isNull())
		{
			static const QRegularExpression transRx("_[(]\"(.*)\"[)](\\s*#.*)?"); // optional comments after name.
			QRegularExpressionMatch transMatch=transRx.match(name);
			if (transMatch.hasMatch())
			{
				QString propName = transMatch.captured(1).trimmed();
				// TODO: Check if name is in the unwanted list and then don't include.
				QString refs = transMatch.captured(2).trimmed(); // a null string should not complain.
				QStringList refList;
				bool accept=refs.isNull(); // if we don't have a reference, we must accept it.
				if (!accept)
				{
					refs=refs.right(refs.length()-1); // Chop off # sign. Use slice after switch to Qt6!
					refList=refs.split(",");
					// trim all sub-strings
					for (int i=0; i<refList.length(); ++i)
					{
						refList[i]=refList.at(i).trimmed();
					}

					//qDebug() << "References for object " << e->getDSODesignation() << "as" << propName << ":" << refList;
					accept=refList.isEmpty(); // if we don't have a reference list, we must accept it.
				}
				// Now refList contains the DSO's reference strings.
				// Compare with the list of unwantedReferences and accept only "allowed" ones.
				if (!accept)
				{
					//refList.removeAll()

					foreach(QString str, refList)
					{
						if (unwantedReferences.contains(str))
						{
							//qDebug() << "Unwanted reference" << str << "detected for DSO name" << propName;
							refList.removeOne(str);
						}
					}
					// Now it's inverse: if there are still references, we use the name.
					accept=!refList.isEmpty();
				}

				if (accept)
				{
					commonNameMap[e].push_back(propName);
					NebulaWithReferences nr = {e, refList };
					commonNameIndex[propName.toUpper()] = nr;  // add with reduced refList.
				}
			}
			readOk++;
		}
		else
			nodata.append(QString("%1 %2").arg(ref.trimmed(), cdes.trimmed()));
	}
	dsoNameFile.close();
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "DSO name records successfully";

	int err = nodata.size();
	if (err>0)
		qWarning().noquote() << "No position data for" << err << "objects:" << nodata.join(", ");

	return true;
}

bool NebulaMgr::loadDSODiscoveryData(const QString &filename)
{
	qInfo() << "Loading DSO discovery data ...";
	QFile dsoDiscoveryFile(filename);
	if (!dsoDiscoveryFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "DSO discovery data file" << QDir::toNativeSeparators(filename) << "not found.";
		return false;
	}

	int readOk = 0;
	int totalRecords = 0;
	QString record, dso, dYear, dName;
	NebulaP e;
	while (!dsoDiscoveryFile.atEnd())
	{
		record = QString::fromUtf8(dsoDiscoveryFile.readLine());
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;

		totalRecords++;
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		QStringList list=record.split("\t", Qt::KeepEmptyParts);
		#else
		QStringList list=record.split("\t", QString::KeepEmptyParts);
		#endif

		dso	= list.at(0).trimmed();
		dYear	= list.at(1).trimmed();
		dName	= list.at(2).trimmed();

		e = searchForCommonName(dso);
		if (e.isNull()) // maybe this is inner number of DSO
			e = searchDSO(dso.toUInt());

		if (!e.isNull())
		{
			e->setDiscoveryData(dName, dYear);
			readOk++;
		}
	}
	dsoDiscoveryFile.close();
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "DSO discovery records successfully";
	return true;
}

bool NebulaMgr::loadDSOOutlines(const QString &filename)
{
	qInfo() << "Loading DSO outline data ...";
	QFile dsoOutlineFile(filename);
	if (!dsoOutlineFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "DSO outline data file" << QDir::toNativeSeparators(filename) << "not found.";
		return false;
	}

	double RA, DE;
	int i, readOk = 0;
	Vec3d XYZ;
	std::vector<Vec3d> *points = Q_NULLPTR;
	typedef QPair<double, double> coords;
	coords point, fpoint;
	QVector<coords> outline;
	QString record, command, dso;
	NebulaP e;
	// Read the outlines data of the DSO
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	while (!dsoOutlineFile.atEnd())
	{
		record = QString::fromUtf8(dsoOutlineFile.readLine());
		if (commentRx.match(record).hasMatch())
			continue;

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		// bytes 1 - 8, RA
		RA = record.first(8).toDouble();
		// bytes 9 -18, DE
		DE = record.sliced(9, 10).toDouble();
#else
		// bytes 1 - 8, RA
		RA = record.leftRef(8).toDouble();
		// bytes 9 -18, DE
		DE = record.midRef(9, 10).toDouble();
#endif
		// bytes 19-25, command
		command = record.mid(19, 7).trimmed();
		// bytes 26, designation of DSO
		dso = record.mid(26).trimmed();

		RA*=M_PI/12.;     // Convert from hours to rad
		DE*=M_PI/180.;    // Convert from deg to rad

		if (command.contains("start", Qt::CaseInsensitive))
		{
			outline.clear();
			e = searchForCommonName(dso);
			if (e.isNull()) // maybe this is inner number of DSO
				e = searchDSO(dso.toUInt());

			point.first  = RA;
			point.second = DE;
			outline.append(point);
			fpoint = point;
		}

		if (command.contains("vertex", Qt::CaseInsensitive))
		{
			point.first  = RA;
			point.second = DE;
			outline.append(point);
		}

		if (command.contains("end", Qt::CaseInsensitive))
		{
			point.first  = RA;
			point.second = DE;
			outline.append(point);
			outline.append(fpoint);

			if (!e.isNull())
			{
				points = new std::vector<Vec3d>;
				for (i = 0; i < outline.size(); i++)
				{
					// Calc the Cartesian coord with RA and DE
					point = outline.at(i);
					StelUtils::spheToRect(point.first, point.second, XYZ);
					points->push_back(XYZ);
				}

				e->outlineSegments.push_back(points);
			}
			readOk++;
		}
	}
	dsoOutlineFile.close();
	qInfo().noquote() << "Loaded" << readOk << "DSO outline records successfully";
	return true;
}

void NebulaMgr::updateSkyCulture(const StelSkyCulture& skyCulture)
{
	for (const auto& n : std::as_const(dsoArray))
		n->removeAllNames();


	int numLoaded = 0;
	if (!skyCulture.names.isEmpty())
		numLoaded = loadCultureSpecificNames(skyCulture.names);

	if (numLoaded)
	{
		qInfo().noquote() << "Loaded" << numLoaded << "culture-specific DSO names";
	}

	// Shall we still use common names if a skycultures has no own DSO names?
	if (numLoaded==0 || skyCulture.fallbackToInternationalNames)
	{
		for (auto it = commonNameMap.cbegin(); it != commonNameMap.cend(); ++it)
			for (const auto& name : it.value())
				addEnglishOrAliasName(it.key(), name);
	}

	updateI18n();
}

int NebulaMgr::loadCultureSpecificNames(const QJsonObject& data)
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	for (const auto& n : std::as_const(dsoArray))
		n->culturalNames.clear();

	int loadedTotal = 0;
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		const auto key = it.key();
		const auto specificNames = it->toArray();

		if (key.startsWith("HIP "))
		{
			// skip since it's a star
		}
		else if (key.startsWith("NAME "))
		{
			// FIXME: this is a kludge for the discrepancy between stellarium-skycultures database
			// and Stellarium's internal database. Maybe we should make use of the former and drop
			// the latter instead of doing this name translation.
			auto name = key.mid(5);
			static const QHash<QString, QString> renamer{
				{"Beehive Cluster" , "Beehive"           },
				{"Carina Nebula"   , "Eta Carinae Nebula"},
				{u8"ω Cen Cluster" , "Omega Centauri"    },
			};
			if (const auto it = renamer.find(name); it != renamer.end())
				name = it.value();
			loadCultureSpecificNameForNamedObject(it.value().toArray(), name);
		}
		else if (const NebulaP n = searchByDesignation(key))
		{
			for (const auto& entry : specificNames)
			{
				QJsonObject json=entry.toObject();
				StelObject::CulturalName cName;
				cName.native          = json["native"].toString().trimmed();
				cName.pronounce       = json["pronounce"].toString().trimmed();
				cName.pronounceI18n   = trans.qtranslate(json["pronounce"].toString(), json["context"].toString());
				cName.transliteration = json["transliteration"].toString().trimmed();
				cName.translated      = json["english"].toString().trimmed();
				cName.translatedI18n  = trans.qtranslate(json["english"].toString(), json["context"].toString());
				cName.IPA             = json["IPA"].toString().trimmed();

				n->addCulturalName(cName);
				++loadedTotal;
			}
		}
	}
	return loadedTotal;
}

// Add names from data to the object locatable by commonname
void NebulaMgr::loadCultureSpecificNameForNamedObject(const QJsonArray& data, const QString& commonName)
{
	const auto commonNameIndexIt = commonNameIndex.find(commonName.toUpper());
	if (commonNameIndexIt == commonNameIndex.end())
	{
		// This may actually not even be a nebula, so we shouldn't emit any warning, just return
		return;
	}

	for (const auto& entry : data)
	{
		//// TODO: Filter away unwanted sources per-skyculture!
		//// For now, just accept as before.
		//setName(commonNameIndexIt.value().nebula, specificName);

		QJsonObject json=entry.toObject();
		StelObject::CulturalName cName;
		cName.native          = json["native"].toString().trimmed();
		cName.pronounce       = json["pronounce"].toString().trimmed();
		cName.pronounceI18n   = qc_(json["pronounce"].toString(), json["context"].toString()); // TODO: Clarify if context is available
		cName.transliteration = json["transliteration"].toString().trimmed();
		cName.translated      = json["english"].toString().trimmed();
		cName.translatedI18n  = qc_(json["english"].toString(), json["context"].toString()); // TODO: Clarify if context is available
		cName.IPA             = json["IPA"].toString().trimmed();

		commonNameIndexIt.value().nebula->addCulturalName(cName);
	}
}

void NebulaMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (const auto& n : std::as_const(dsoArray))
		n->translateName(trans);
}


//! Return the matching Nebula object's pointer if exists or an "empty" StelObjectP
StelObjectP NebulaMgr::searchByNameI18n(const QString& nameI18n) const
{
	const QString nameI18nUpper = nameI18n.toUpper();

	// Search by common names
	for (const auto& n : std::as_const(dsoArray))
	{
		if (n->nameI18.toUpper() == nameI18nUpper)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by aliases of common names
	for (const auto& n : std::as_const(dsoArray))
	{
		for (auto &nameI18Alias : n->nameI18Aliases)
		{
			if (nameI18Alias.toUpper()==nameI18nUpper)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by designation
	NebulaP n = searchByDesignation(nameI18nUpper);
	return qSharedPointerCast<StelObject>(n);
}


//! Return the matching Nebula object's pointer if exists or an "empty" StelObjectP
StelObjectP NebulaMgr::searchByName(const QString& name) const
{
	const QString nameUpper = name.toUpper();

	// Search by common names
	for (const auto& n : dsoArray)
	{
		if (n->englishName.toUpper()==nameUpper)
			return qSharedPointerCast<StelObject>(n);
	}

	if (getFlagAdditionalNames())
	{
		// Search by aliases of common names
		for (const auto& n : dsoArray)
		{
			for (auto &englishAlias : n->englishAliases)
			{
				if (englishAlias.toUpper()==nameUpper)
					return qSharedPointerCast<StelObject>(n);
			}
		}
	}

	// Search by designation
	NebulaP n = searchByDesignation(nameUpper);
	return qSharedPointerCast<StelObject>(n);
}

//! Return the matching Nebula object's pointer if exists or Q_NULLPTR
//! TODO Decide whether empty StelObjectP or Q_NULLPTR is the better return type and select the same for both.
NebulaP NebulaMgr::searchByDesignation(const QString &designation) const
{
	NebulaP n;
	QString designationUpper = designation.toUpper();
	// If no match found, try search by catalog reference
	static const QRegularExpression catNumRx("^(M|NGC|IC|C|B|VDB|RCW|LDN|LBN|CR|MEL|PGC|UGC|ARP|VV|DWB|TR|TRUMPLER|ST|STOCK|RU|RUPRECHT|VDB-HA)\\s*(\\d+)$");
	QRegularExpressionMatch catNumMatch=catNumRx.match(designationUpper);
	if (catNumMatch.hasMatch())
	{
		QString cat = catNumMatch.captured(1);
		unsigned int num = catNumMatch.captured(2).toUInt();
		if (cat == "M") n = searchM(num);
		if (cat == "NGC") n = searchNGC(num);
		if (cat == "IC") n = searchIC(num);
		if (cat == "C") n = searchC(num);
		if (cat == "B") n = searchB(num);
		if (cat == "VDB-HA") n = searchVdBHa(num);
		if (cat == "VDB") n = searchVdB(num);
		if (cat == "RCW") n = searchRCW(num);
		if (cat == "LDN") n = searchLDN(num);
		if (cat == "LBN") n = searchLBN(num);
		if (cat == "CR") n = searchCr(num);
		if (cat == "MEL") n = searchMel(num);
		if (cat == "PGC") n = searchPGC(num);
		if (cat == "UGC") n = searchUGC(num);
		if (cat == "ARP") n = searchArp(num);
		if (cat == "VV") n = searchVV(num);
		if (cat == "DWB") n = searchDWB(num);
		if (cat == "TR" || cat == "TRUMPLER") n = searchTr(num);
		if (cat == "ST" || cat == "STOCK") n = searchSt(num);
		if (cat == "RU" || cat == "RUPRECHT") n = searchRu(num);
	}
	static const QRegularExpression dCatNumRx("^(SH)\\s*\\d-\\s*(\\d+)$");
	QRegularExpressionMatch dCatNumMatch=dCatNumRx.match(designationUpper);
	if (dCatNumMatch.hasMatch())
	{
		QString dcat = dCatNumMatch.captured(1);
		unsigned int dnum = dCatNumMatch.captured(2).toUInt();

		if (dcat == "SH") n = searchSh2(dnum);
	}
	static const QRegularExpression sCatNumRx("^(CED|PK|ACO|ABELL|HCG|ESO|VDBH)\\s*(.+)$");
	QRegularExpressionMatch sCatNumMatch=sCatNumRx.match(designationUpper);
	if (sCatNumMatch.hasMatch())
	{
		QString cat = sCatNumMatch.captured(1);
		QString num = sCatNumMatch.captured(2).trimmed();

		if (cat == "CED") n = searchCed(num);
		if (cat == "PK") n = searchPK(num);
		if (cat == "ACO" || cat == "ABELL") n = searchACO(num);
		if (cat == "HCG") n = searchHCG(num);
		if (cat == "ESO") n = searchESO(num);
		if (cat == "VDBH") n = searchVdBH(num);
	}
	static const QRegularExpression gCatNumRx("^(PN|SNR)\\s*G(.+)$");
	QRegularExpressionMatch gCatNumMatch=gCatNumRx.match(designationUpper);
	if (gCatNumMatch.hasMatch())
	{
		QString cat = gCatNumMatch.captured(1);
		QString num = gCatNumMatch.captured(2).trimmed();

		if (cat == "PN") n = searchPNG(num);
		if (cat == "SNR") n = searchSNRG(num);
	}

	return n;
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object name
QStringList NebulaMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem <= 0)
		return result;

	QString objUpper = objPrefix.toUpper();

	// Search by Messier objects number (possible formats are "M31" or "M 31")
	if (objUpper.size()>=1 && objUpper.at(0)=='M' && objUpper.left(3)!="MEL")
	{
		for (const auto& n : dsoArray)
		{
			if (n->M_nb==0) continue;
			QString constw = QString("M%1").arg(n->M_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg(n->M_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Melotte objects number (possible formats are "Mel31" or "Mel 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="MEL")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Mel_nb==0) continue;
			QString constw = QString("Mel%1").arg(n->Mel_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("Melotte%1").arg(n->Mel_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Mel %1").arg(n->Mel_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("Melotte %1").arg(n->Mel_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by IC objects number (possible formats are "IC466" or "IC 466")
	if (objUpper.size()>=1 && objUpper.left(2)=="IC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->IC_nb==0) continue;
			QString constw = QString("IC%1").arg(n->IC_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("IC %1").arg(n->IC_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	for (const auto& n : dsoArray)
	{
		if (n->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg(n->NGC_nb);
		QString constws = constw.mid(0, objUpper.size());
		if (constws.toUpper()==objUpper)
		{
			result << constws;
			continue;
		}
		constw = QString("NGC %1").arg(n->NGC_nb);
		constws = constw.mid(0, objUpper.size());
		if (constws.toUpper()==objUpper)
			result << constw;
	}

	// Search by PGC object numbers (possible formats are "PGC31" or "PGC 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="PGC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PGC_nb==0) continue;
			QString constw = QString("PGC%1").arg(n->PGC_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;	// Prevent adding both forms for name
				continue;
			}
			constw = QString("PGC %1").arg(n->PGC_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by UGC object numbers (possible formats are "UGC31" or "UGC 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="UGC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->UGC_nb==0) continue;
			QString constw = QString("UGC%1").arg(n->UGC_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("UGC %1").arg(n->UGC_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Caldwell objects number (possible formats are "C31" or "C 31")
	if (objUpper.size()>=1 && objUpper.at(0)=='C' && objUpper.left(2)!="CR" && objUpper.left(2)!="CE")
	{
		for (const auto& n : dsoArray)
		{
			if (n->C_nb==0) continue;
			QString constw = QString("C%1").arg(n->C_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("C %1").arg(n->C_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Collinder objects number (possible formats are "Cr31" or "Cr 31")
	if (objUpper.size()>=1 && (objUpper.left(2)=="CR" || objUpper.left(9)=="COLLINDER"))
	{
		for (const auto& n : dsoArray)
		{
			if (n->Cr_nb==0) continue;
			QString constw = QString("Cr%1").arg(n->Cr_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("Collinder%1").arg(n->Cr_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Cr %1").arg(n->Cr_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("Collinder %1").arg(n->Cr_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Ced objects number (possible formats are "Ced31" or "Ced 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="CED")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Ced_nb.isEmpty()) continue;
			QString constw = QString("Ced%1").arg(n->Ced_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Ced %1").arg(n->Ced_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Barnard objects number (possible formats are "B31" or "B 31")
	if (objUpper.size()>=1 && objUpper.at(0)=='B')
	{
		for (const auto& n : dsoArray)
		{
			if (n->B_nb==0) continue;
			QString constw = QString("B%1").arg(n->B_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("B %1").arg(n->B_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Sharpless objects number (possible formats are "Sh2-31" or "Sh 2-31")
	if (objUpper.size()>=1 && objUpper.left(2)=="SH")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Sh2_nb==0) continue;
			QString constw = QString("SH2-%1").arg(n->Sh2_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SH 2-%1").arg(n->Sh2_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by van den Bergh objects number (possible formats are "vdB31" or "vdB 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="VDB" && objUpper.left(6)!="VDB-HA" && objUpper.left(4)!="VDBH")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VdB_nb==0) continue;
			QString constw = QString("vdB%1").arg(n->VdB_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("vdB %1").arg(n->VdB_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by RCW objects number (possible formats are "RCW31" or "RCW 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="RCW")
	{
		for (const auto& n : dsoArray)
		{
			if (n->RCW_nb==0) continue;
			QString constw = QString("RCW%1").arg(n->RCW_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("RCW %1").arg(n->RCW_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by LDN objects number (possible formats are "LDN31" or "LDN 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="LDN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->LDN_nb==0) continue;
			QString constw = QString("LDN%1").arg(n->LDN_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LDN %1").arg(n->LDN_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by LBN objects number (possible formats are "LBN31" or "LBN 31")
	if (objUpper.size()>=1 && objUpper.left(3)=="LBN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->LBN_nb==0) continue;
			QString constw = QString("LBN%1").arg(n->LBN_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LBN %1").arg(n->LBN_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Arp objects number
	if (objUpper.size()>=1 && objUpper.left(3)=="ARP")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Arp_nb==0) continue;
			QString constw = QString("Arp%1").arg(n->Arp_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Arp %1").arg(n->Arp_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by VV objects number
	if (objUpper.size()>=1 && objUpper.left(2)=="VV")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VV_nb==0) continue;
			QString constw = QString("VV%1").arg(n->VV_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VV %1").arg(n->VV_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by PK objects number
	if (objUpper.size()>=1 && objUpper.left(2)=="PK")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PK_nb.isEmpty()) continue;
			QString constw = QString("PK%1").arg(n->PK_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("PK %1").arg(n->PK_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by PN G objects number
	if (objUpper.size()>=1 && objUpper.left(2)=="PN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PNG_nb.isEmpty()) continue;
			QString constw = QString("PNG%1").arg(n->PNG_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("PN G%1").arg(n->PNG_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by SNR G objects number
	if (objUpper.size()>=1 && objUpper.left(3)=="SNR")
	{
		for (const auto& n : dsoArray)
		{
			if (n->SNRG_nb.isEmpty()) continue;
			QString constw = QString("SNRG%1").arg(n->SNRG_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SNR G%1").arg(n->SNRG_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by ACO (Abell) objects number
	if (objUpper.size()>=1 && (objUpper.left(5)=="ABELL" || objUpper.left(3)=="ACO"))
	{
		for (const auto& n : dsoArray)
		{
			if (n->ACO_nb.isEmpty()) continue;
			QString constw = QString("Abell%1").arg(n->ACO_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("ACO%1").arg(n->ACO_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Abell %1").arg(n->ACO_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("ACO %1").arg(n->ACO_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by HCG objects number
	if (objUpper.size()>=1 && objUpper.left(3)=="HCG")
	{
		for (const auto& n : dsoArray)
		{
			if (n->HCG_nb.isEmpty()) continue;
			QString constw = QString("HCG%1").arg(n->HCG_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("HCG %1").arg(n->HCG_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by ESO objects number
	if (objUpper.size()>=1 && objUpper.left(3)=="ESO")
	{
		for (const auto& n : dsoArray)
		{
			if (n->ESO_nb.isEmpty()) continue;
			QString constw = QString("ESO%1").arg(n->ESO_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("ESO %1").arg(n->ESO_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by VdBH objects number
	if (objUpper.size()>=1 && objUpper.left(4)=="VDBH")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VdBH_nb.isEmpty()) continue;
			QString constw = QString("vdBH%1").arg(n->VdBH_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("vdBH %1").arg(n->VdBH_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by DWB objects number
	if (objUpper.size()>=1 && objUpper.left(3)=="DWB")
	{
		for (const auto& n : dsoArray)
		{
			if (n->DWB_nb==0) continue;
			QString constw = QString("DWB%1").arg(n->DWB_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("DWB %1").arg(n->DWB_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Tr (Trumpler) objects number
	if (objUpper.size()>=1 && (objUpper.left(8)=="TRUMPLER" || objUpper.left(2)=="TR"))
	{
		for (const auto& n : dsoArray)
		{
			if (n->Tr_nb==0) continue;
			QString constw = QString("Tr%1").arg(n->Tr_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("Trumpler%1").arg(n->Tr_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Tr %1").arg(n->Tr_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("Trumpler %1").arg(n->Tr_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by St (Stock) objects number
	if (objUpper.size()>=1 && (objUpper.left(5)=="STOCK" || objUpper.left(2)=="ST"))
	{
		for (const auto& n : dsoArray)
		{
			if (n->St_nb==0) continue;
			QString constw = QString("St%1").arg(n->St_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("Stock%1").arg(n->St_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("St %1").arg(n->St_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("Stock %1").arg(n->St_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by Ru (Ruprecht) objects number
	if (objUpper.size()>=1 && (objUpper.left(8)=="RUPRECHT" || objUpper.left(2)=="RU"))
	{
		for (const auto& n : dsoArray)
		{
			if (n->Ru_nb==0) continue;
			QString constw = QString("Ru%1").arg(n->Ru_nb);
			QString constws = constw.mid(0, objUpper.size());
			QString constws2 = QString("Ruprecht%1").arg(n->Ru_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Ru %1").arg(n->Ru_nb);
			constws = constw.mid(0, objUpper.size());
			constws2 = QString("Ruprecht %1").arg(n->Ru_nb).mid(0, objUpper.size());
			if (constws.toUpper()==objUpper || constws2.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by van den Bergh-Hagen Catalogue objects number
	if (objUpper.size()>=1 && objUpper.left(6)=="VDB-HA")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VdBHa_nb==0) continue;
			QString constw = QString("vdB-Ha%1").arg(n->VdBHa_nb);
			QString constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("vdB-Ha %1").arg(n->VdBHa_nb);
			constws = constw.mid(0, objUpper.size());
			if (constws.toUpper()==objUpper)
				result << constw;
		}
	}

	// Search by common names and aliases
	QStringList names;
	for (const auto& n : dsoArray)
	{
		names.append(n->nameI18);
		names.append(n->englishName);
		if (getFlagAdditionalNames())
		{
			names.append(n->nameI18Aliases);
			names.append(n->englishAliases);
		}
	}

	QString fullMatch = "";
	for (const auto& name : std::as_const(names))
	{
		if (!matchObjectName(name, objPrefix, useStartOfWords))
			continue;

		if (name==objPrefix)
			fullMatch = name;
		else
			result.append(name);
	}

	result.sort();
	if (!fullMatch.isEmpty())
		result.prepend(fullMatch);

	if (result.size() > maxNbItem)
		result.erase(result.begin() + maxNbItem, result.end());

	return result;
}

QStringList NebulaMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	for (const auto& n : dsoArray)
	{
		if (!n->getEnglishName().isEmpty())
		{
			if (inEnglish)
				result << n->getEnglishName();
			else
				result << n->getNameI18n();
		}
	}
	return result;
}

QStringList NebulaMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;
	int type = objType.toInt();
	switch (type)
	{
		case 0: // Bright galaxies?
			for (const auto& n : dsoArray)
			{
				if (n->nType==type && qMin(n->vMag, n->bMag)<=10.f)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else
						result << n->getDSODesignationWIC();
				}
			}
			break;
		case 100: // Messier Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("M%1").arg(n->M_nb);
			break;
		case 101: // Caldwell Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("C%1").arg(n->C_nb);
			break;
		case 102: // Barnard Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("B %1").arg(n->B_nb);
			break;
		case 103: // Sharpless Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("SH 2-%1").arg(n->Sh2_nb);
			break;
		case 104: // van den Bergh Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("vdB %1").arg(n->VdB_nb);
			break;
		case 105: // RCW Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("RCW %1").arg(n->RCW_nb);
			break;
		case 106: // Collinder Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Cr %1").arg(n->Cr_nb);
			break;
		case 107: // Melotte Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Mel %1").arg(n->Mel_nb);
			break;
		case 108: // New General Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("NGC %1").arg(n->NGC_nb);
			break;
		case 109: // Index Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("IC %1").arg(n->IC_nb);
			break;
		case 110: // Lynds' Catalogue of Bright Nebulae
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("LBN %1").arg(n->LBN_nb);
			break;
		case 111: // Lynds' Catalogue of Dark Nebulae
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("LDN %1").arg(n->LDN_nb);
			break;
		case 114: // Cederblad Catalog
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Ced %1").arg(n->Ced_nb);
			break;
		case 115: // Atlas of Peculiar Galaxies (Arp)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Arp %1").arg(n->Arp_nb);
			break;
		case 116: // The Catalogue of Interacting Galaxies by Vorontsov-Velyaminov (VV)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("VV %1").arg(n->VV_nb);
			break;
		case 117: // Catalogue of Galactic Planetary Nebulae (PK)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("PK %1").arg(n->PK_nb);
			break;
		case 118: // Strasbourg-ESO Catalogue of Galactic Planetary Nebulae by Acker et. al. (PN G)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("PN G%1").arg(n->PNG_nb);
			break;
		case 119: // A catalogue of Galactic supernova remnants by Green (SNR G)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("SNR G%1").arg(n->SNRG_nb);
			break;
		case 120: // A Catalog of Rich Clusters of Galaxies by Abell et. al. (Abell (ACO))
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Abell %1").arg(n->ACO_nb);
			break;
		case 121: // Hickson Compact Group by Hickson et. al. (HCG)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("HCG %1").arg(n->HCG_nb);
			break;
		case 122: // ESO/Uppsala Survey of the ESO(B) Atlas (ESO)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("ESO %1").arg(n->ESO_nb);
			break;
		case 123: // Catalogue of southern stars embedded in nebulosity (vdBH)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("vdBH %1").arg(n->VdBH_nb);
			break;
		case 124: // Catalogue and distances of optically visible H II regions (DWB)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("DWB %1").arg(n->DWB_nb);
			break;
		case 125: // Trumpler Catalogue (Tr)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Tr %1").arg(n->Tr_nb);
			break;
		case 126: // Stock Catalogue (St)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("St %1").arg(n->St_nb);
			break;
		case 127: // Ruprecht Catalogue (Ru)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Ru %1").arg(n->Ru_nb);
			break;
		case 128: // van den Bergh-Hagen Catalogue (VdB-Ha)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("vdB-Ha %1").arg(n->VdBHa_nb);
			break;
		case 150: // Dwarf galaxies [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(DWARF_GALAXIES) / sizeof(DWARF_GALAXIES[0]); i++)
				result << QString("PGC %1").arg(DWARF_GALAXIES[i]);
			break;
		}
		case 151: // Herschel 400 Catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(H400_LIST) / sizeof(H400_LIST[0]); i++)
				result << QString("NGC %1").arg(H400_LIST[i]);
			break;
		}
		case 152: // Jack Bennett's deep sky catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(BENNETT_LIST) / sizeof(BENNETT_LIST[0]); i++)
				result << QString("NGC %1").arg(BENNETT_LIST[i]);
			result << "Mel 105" << "IC 1459";
			break;
		}
		case 153: // James Dunlop's deep sky catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(DUNLOP_LIST) / sizeof(DUNLOP_LIST[0]); i++)
				result << QString("NGC %1").arg(DUNLOP_LIST[i]);
			break;
		}
		default:
		{
			for (const auto& n : dsoArray)
			{
				if (n->nType==type)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else
						result << n->getDSODesignationWIC();
				}
			}
			break;
		}
	}

	result.removeDuplicates();
	return result;
}

const QList<NebulaP> NebulaMgr::getDeepSkyObjectsByType(const QString &objType) const
{
	QList<NebulaP> dso;
	int type = objType.toInt();
	switch (type)
	{
		case 100: // Messier Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->M_nb>0)
					dso.append(n);
			}
			break;
		case 101: // Caldwell Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->C_nb>0)
					dso.append(n);
			}
			break;
		case 102: // Barnard Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->B_nb>0)
					dso.append(n);
			}
			break;
		case 103: // Sharpless Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->Sh2_nb>0)
					dso.append(n);
			}
			break;
		case 104: // van den Bergh Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->VdB_nb>0)
					dso.append(n);
			}
			break;
		case 105: // RCW Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->RCW_nb>0)
					dso.append(n);
			}
			break;
		case 106: // Collinder Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->Cr_nb>0)
					dso.append(n);
			}
			break;
		case 107: // Melotte Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->Mel_nb>0)
					dso.append(n);
			}
			break;
		case 108: // New General Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->NGC_nb>0)
					dso.append(n);
			}
			break;
		case 109: // Index Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->IC_nb>0)
					dso.append(n);
			}
			break;
		case 110: // Lynds' Catalogue of Bright Nebulae
			for (const auto& n : dsoArray)
			{
				if (n->LBN_nb>0)
					dso.append(n);
			}
			break;
		case 111: // Lynds' Catalogue of Dark Nebulae
			for (const auto& n : dsoArray)
			{
				if (n->LDN_nb>0)
					dso.append(n);
			}
			break;
		case 112: // Principal Galaxy Catalog
			for (const auto& n : dsoArray)
			{
				if (n->PGC_nb>0)
					dso.append(n);
			}
			break;
		case 113: // The Uppsala General Catalogue of Galaxies
			for (const auto& n : dsoArray)
			{
				if (n->UGC_nb>0)
					dso.append(n);
			}
			break;
		case 114: // Cederblad Catalog
			for (const auto& n : dsoArray)
			{
				if (!n->Ced_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 115: // Atlas of Peculiar Galaxies (Arp)
			for (const auto& n : dsoArray)
			{
				if (n->Arp_nb>0)
					dso.append(n);
			}
			break;
		case 116: // The Catalogue of Interacting Galaxies by Vorontsov-Velyaminov (VV)
			for (const auto& n : dsoArray)
			{
				if (n->VV_nb>0)
					dso.append(n);
			}
			break;
		case 117: // Catalogue of Galactic Planetary Nebulae (PK)
			for (const auto& n : dsoArray)
			{
				if (!n->PK_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 118: // Strasbourg-ESO Catalogue of Galactic Planetary Nebulae by Acker et. al. (PN G)
			for (const auto& n : dsoArray)
			{
				if (!n->PNG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 119: // A catalogue of Galactic supernova remnants by Green (SNR G)
			for (const auto& n : dsoArray)
			{
				if (!n->SNRG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 120: // A Catalog of Rich Clusters of Galaxies by Abell et. al. (ACO)
			for (const auto& n : dsoArray)
			{
				if (!n->ACO_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 121: // Hickson Compact Group by Hickson et. al. (HCG)
			for (const auto& n : dsoArray)
			{
				if (!n->HCG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 122: // ESO/Uppsala Survey of the ESO(B) Atlas (ESO)
			for (const auto& n : dsoArray)
			{
				if (!n->ESO_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 123: // Catalogue of southern stars embedded in nebulosity (VdBH)
			for (const auto& n : dsoArray)
			{
				if (!n->VdBH_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 124: // Catalogue and distances of optically visible H II regions (DWB)
			for (const auto& n : dsoArray)
			{
				if (n->DWB_nb > 0)
					dso.append(n);
			}
			break;
		case 125: // Trumpler Catalogue (Tr)
			for (const auto& n : dsoArray)
			{
				if (n->Tr_nb > 0)
					dso.append(n);
			}
			break;
		case 126: // Stock Catalogue (St)
			for (const auto& n : dsoArray)
			{
				if (n->St_nb > 0)
					dso.append(n);
			}
			break;
		case 127: // Ruprecht Catalogue (Ru)
			for (const auto& n : dsoArray)
			{
				if (n->Ru_nb > 0)
					dso.append(n);
			}
			break;
		case 128: // van den Bergh-Hagen Catalogue (VdB-Ha)
			for (const auto& n : dsoArray)
			{
				if (n->VdBHa_nb > 0)
					dso.append(n);
			}
			break;
		case 150: // Dwarf galaxies [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(DWARF_GALAXIES) / sizeof(DWARF_GALAXIES[0]); i++)
			{
				ds = searchPGC(DWARF_GALAXIES[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		case 151: // Herschel 400 Catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(H400_LIST) / sizeof(H400_LIST[0]); i++)
			{
				ds = searchNGC(H400_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		case 152: // Jack Bennett's deep sky catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(BENNETT_LIST) / sizeof(BENNETT_LIST[0]); i++)
			{
				ds = searchNGC(BENNETT_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			ds = searchMel(105);
			if (!ds.isNull())
				dso.append(ds);
			ds = searchIC(1459);
			if (!ds.isNull())
				dso.append(ds);
			break;
		}
		case 153: // James Dunlop's deep sky catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(DUNLOP_LIST) / sizeof(DUNLOP_LIST[0]); i++)
			{
				ds = searchNGC(DUNLOP_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		default:
		{
			for (const auto& n : dsoArray)
			{
				if (n->nType==type)
					dso.append(n);
			}
			break;
		}
	}

	return dso;
}
