/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#include <CfgFileManager.h>
#include <Hydrax.h>

namespace Hydrax
{
	CfgFileManager::CfgFileManager(Hydrax* h)
		: mHydrax(h)
	{
	}

	CfgFileManager::~CfgFileManager()
	{
	}

	const bool CfgFileManager::load(const Ogre::String& File) const
	{
		std::pair<bool, Ogre::ConfigFile> CfgFileResult;
		_loadCfgFile(File, CfgFileResult);

		if (!CfgFileResult.first)
		{
			return false;
		}

		Ogre::ConfigFile &CfgFile = CfgFileResult.second;

		if (!_checkVersion(CfgFile))
		{
			return false;
		}

		// Load main options
		// RIGS OF RODS NOTE: ShaderMode is determined automatically based on renderer
		mHydrax->setPosition(_getVector3Value(CfgFile,"Position"));
		mHydrax->setPlanesError(_getFloatValue(CfgFile,"PlanesError"));
		mHydrax->setFullReflectionDistance(_getFloatValue(CfgFile,"FullReflectionDistance"));
		mHydrax->setGlobalTransparency(_getFloatValue(CfgFile,"GlobalTransparency"));
		mHydrax->setNormalDistortion(_getFloatValue(CfgFile,"NormalDistortion"));
		mHydrax->setWaterColor(_getVector3Value(CfgFile,"WaterColor"));

		// Load components settings
		_loadComponentsSettings(CfgFile);

		// Load rtt settings
		_loadRttSettings(CfgFile);

		// Load module and noise settings
		if (mHydrax->getModule())
		{
			mHydrax->getModule()->loadCfg(CfgFile);

			if (mHydrax->getModule()->getNoise())
			{
				mHydrax->getModule()->getNoise()->loadCfg(CfgFile);
			}
		}

		return true;
	}

	const bool CfgFileManager::save(const Ogre::String& File, const Ogre::String& Path) const
	{
		Ogre::String Data =
			"#Hydrax cfg file.\n\n";

		Data += "#Hydrax version field\n";
		Data += _getVersionCfgString();

		Data += "#Main options field\n";
		Data += _getCfgString("Position",               mHydrax->getPosition());
		Data += _getCfgString("PlanesError",            mHydrax->getPlanesError());
		// RIGS OF RODS NOTE: ShaderMode is determined automatically based on renderer
		Data += _getCfgString("FullReflectionDistance", mHydrax->getFullReflectionDistance());
		Data += _getCfgString("GlobalTransparency",     mHydrax->getGlobalTransparency());
		Data += _getCfgString("NormalDistortion",       mHydrax->getNormalDistortion());
		Data += _getCfgString("WaterColor",             mHydrax->getWaterColor()); Data += "\n";

		Data += "#Components field\n";
		Data += _getComponentsCfgString();

		Data += "#Rtt quality field(0x0 = Auto)\n";
		Data += _getRttCfgString(); Data += "\n";

		if (mHydrax->getModule())
		{
			mHydrax->getModule()->saveCfg(Data);

			if (mHydrax->getModule()->getNoise())
			{
				mHydrax->getModule()->getNoise()->saveCfg(Data);
			}
		}

		return _saveToFile(Data, File, Path);
	}

	const bool CfgFileManager::_saveToFile(const Ogre::String& Data, const Ogre::String& File, const Ogre::String& Path) const
	{
		FILE *DestinationFile = fopen((Path+"/"+File).c_str(), "w");

		if (!DestinationFile)
		{
			return false;
		}

		fprintf(DestinationFile, "%s", Data.c_str());
		fclose(DestinationFile);

		HydraxLOG(File + " saved in " + Path + " .");

		return true;
	}

	const void CfgFileManager::_loadCfgFile(const Ogre::String& File, std::pair<bool,Ogre::ConfigFile> &Result) const
	{
		try
		{
			Result.second.load(Ogre::ResourceGroupManager::getSingleton().openResource(File, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));
			Result.first = true;

		} catch(...)
		{
			HydraxLOG("CfgFileManager::_getCfgFile(...): " + File + " not found in any resource group.");
			Result.first = false;
		}

		HydraxLOG(File + " loaded.");
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const int &Value)
	{
		return "<int>" + Name + "=" + Ogre::StringConverter::toString(Value) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const Ogre::Real &Value)
	{
		return "<float>" + Name + "=" + Ogre::StringConverter::toString(Value) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const bool &Value)
	{
		return "<bool>" + Name + "=" + Ogre::StringConverter::toString(Value) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const Ogre::Vector2 &Value)
	{
		return "<vector2>" + Name + "=" + Ogre::StringConverter::toString(Value.x) + "x" + Ogre::StringConverter::toString(Value.y) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const Ogre::Vector3 &Value)
	{
		return "<vector3>" + Name + "=" + Ogre::StringConverter::toString(Value.x) + "x" + Ogre::StringConverter::toString(Value.y) + "x" + Ogre::StringConverter::toString(Value.z) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String& Name, const Size& Value)
	{
		return "<size>" + Name + "=" + Ogre::StringConverter::toString(Value.Width) + "x" + Ogre::StringConverter::toString(Value.Height) + "\n";
	}

	const Ogre::String CfgFileManager::_getComponentsCfgString() const
	{
		Ogre::String Cmpnts = "Components=";

		std::pair<bool, Ogre::String> Cmp[8] = {
			std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_SUN),                    "Sun"),
			std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_FOAM),                   "Foam"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_DEPTH),                  "Depth"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_SMOOTH),                 "Smooth"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_CAUSTICS),               "Caustics"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER),             "Underwater"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS), "UnderwaterReflections"),
		    std::pair<bool, Ogre::String>(mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER_GODRAYS),     "UnderwaterGodRays"),};

		for (int k = 0; k < 8; k++)
		{
			if (Cmp[k].first)
			{
				Cmpnts += Cmp[k].second;

				bool isLast = true;

			    for (int u = k+1; u < 8; u++)
			    {
				    if (Cmp[u].first)
				    {
				    	isLast = false;
				    }
			    }

			    if (!isLast)
			    {
				    Cmpnts += "|";
			    }
			    else
			    {
			    	Cmpnts += "\n\n";
			    }
			}
		}

		// Sun parameters
		if (Cmp[0].first)
		{
			Cmpnts += "#Sun parameters\n";
			Cmpnts += _getCfgString("SunPosition", mHydrax->getSunPosition());
			Cmpnts += _getCfgString("SunStrength", mHydrax->getSunStrength());
			Cmpnts += _getCfgString("SunArea",     mHydrax->getSunArea());
			Cmpnts += _getCfgString("SunColor",    mHydrax->getSunColor()); Cmpnts += "\n";
		}
		// Foam parameters
		if (Cmp[1].first)
		{
			Cmpnts += "#Foam parameters\n";
			Cmpnts += _getCfgString("FoamMaxDistance",  mHydrax->getFoamMaxDistance());
			Cmpnts += _getCfgString("FoamScale",        mHydrax->getFoamScale());
			Cmpnts += _getCfgString("FoamStart",        mHydrax->getFoamStart());
			Cmpnts += _getCfgString("FoamTransparency", mHydrax->getFoamTransparency()); Cmpnts += "\n";
		}
		// Depth parameters
		if (Cmp[2].first)
		{
			Cmpnts += "#Depth parameters\n";
			Cmpnts += _getCfgString("DepthLimit", mHydrax->getDepthLimit()); Cmpnts += "\n";
		}
		// Smooth transitions parameters
		if (Cmp[3].first)
		{
			Cmpnts += "#Smooth transitions parameters\n";
			Cmpnts += _getCfgString("SmoothPower", mHydrax->getSmoothPower()); Cmpnts += "\n";
		}
		// Caustics parameters
		if (Cmp[4].first)
		{
			Cmpnts += "#Caustics parameters\n";
			Cmpnts += _getCfgString("CausticsScale", mHydrax->getCausticsScale());
			Cmpnts += _getCfgString("CausticsPower", mHydrax->getCausticsPower());
			Cmpnts += _getCfgString("CausticsEnd",   mHydrax->getCausticsEnd()); Cmpnts += "\n";
		}
		// God rays parameters
		if (Cmp[7].first)
		{
			Cmpnts += "#God rays parameters\n";
			Cmpnts += _getCfgString("GodRaysExposure",      mHydrax->getGodRaysExposure());
			Cmpnts += _getCfgString("GodRaysIntensity",     mHydrax->getGodRaysIntensity());
			Cmpnts += _getCfgString("GodRaysSpeed",         mHydrax->getGodRaysManager()->getSimulationSpeed());
			Cmpnts += _getCfgString("GodRaysNumberOfRays",  mHydrax->getGodRaysManager()->getNumberOfRays());
			Cmpnts += _getCfgString("GodRaysRaysSize",      mHydrax->getGodRaysManager()->getRaysSize());
			Cmpnts += _getCfgString("GodRaysIntersections", mHydrax->getGodRaysManager()->areObjectsIntersectionsEnabled()); Cmpnts += "\n";
		}

		return Cmpnts;
	}

	bool CfgFileManager::_isStringInList(const Ogre::StringVector &List, const Ogre::String &Find)
	{
		for (unsigned int k = 0; k < List.size(); k++)
		{
			if (List[k] == Find)
			{
				return true;
			}
		}

		return false;
	}

	const void CfgFileManager::_loadComponentsSettings(Ogre::ConfigFile& CfgFile) const
	{
		Ogre::StringVector Cmpnts = Ogre::StringUtil::split(CfgFile.getSetting("Components"), "|");

		HydraxComponent ComponentsToLoad[8] = {
			HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE,
			HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE,HYDRAX_COMPONENTS_NONE};

		if (_isStringInList(Cmpnts, "Sun"))
		{
			ComponentsToLoad[0] = HYDRAX_COMPONENT_SUN;
		}
		if (_isStringInList(Cmpnts, "Foam"))
		{
			ComponentsToLoad[1] = HYDRAX_COMPONENT_FOAM;
		}
		if (_isStringInList(Cmpnts, "Depth"))
		{
			ComponentsToLoad[2] = HYDRAX_COMPONENT_DEPTH;
		}
		if (_isStringInList(Cmpnts, "Smooth"))
		{
			ComponentsToLoad[3] = HYDRAX_COMPONENT_SMOOTH;
		}
		if (_isStringInList(Cmpnts, "Caustics"))
		{
			ComponentsToLoad[4] = HYDRAX_COMPONENT_CAUSTICS;
		}
		if (_isStringInList(Cmpnts, "Underwater"))
		{
			ComponentsToLoad[5] = HYDRAX_COMPONENT_UNDERWATER;
		}
		if (_isStringInList(Cmpnts, "UnderwaterReflections"))
		{
			ComponentsToLoad[6] = HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS;
		}
		if (_isStringInList(Cmpnts, "UnderwaterGodRays"))
		{
			ComponentsToLoad[7] = HYDRAX_COMPONENT_UNDERWATER_GODRAYS;
		}

		mHydrax->setComponents(static_cast<HydraxComponent>(
			ComponentsToLoad[0] | ComponentsToLoad[1] | ComponentsToLoad[2] | ComponentsToLoad[3] |
			ComponentsToLoad[4] | ComponentsToLoad[5] | ComponentsToLoad[6] | ComponentsToLoad[7]));

		if (_isStringInList(Cmpnts, "Sun"))
		{
			mHydrax->setSunPosition(_getVector3Value(CfgFile,"SunPosition"));
			mHydrax->setSunStrength(_getFloatValue(CfgFile,"SunStrength"));
			mHydrax->setSunArea(_getFloatValue(CfgFile,"SunArea"));
			mHydrax->setSunColor(_getVector3Value(CfgFile,"SunColor"));
		}

		if (_isStringInList(Cmpnts, "Foam"))
		{
			mHydrax->setFoamMaxDistance(_getFloatValue(CfgFile,"FoamMaxDistance"));
			mHydrax->setFoamScale(_getFloatValue(CfgFile,"FoamScale"));
			mHydrax->setFoamStart(_getFloatValue(CfgFile,"FoamStart"));
			mHydrax->setFoamTransparency(_getFloatValue(CfgFile,"FoamTransparency"));
		}

		if (_isStringInList(Cmpnts, "Depth"))
		{
			mHydrax->setDepthLimit(_getFloatValue(CfgFile,"DepthLimit"));
			mHydrax->setDistLimit(_getFloatValue(CfgFile,"DistLimit"));
		}

		if (_isStringInList(Cmpnts, "Smooth"))
		{
			mHydrax->setSmoothPower(_getFloatValue(CfgFile,"SmoothPower"));
		}

		if (_isStringInList(Cmpnts, "Caustics"))
		{
			mHydrax->setCausticsScale(_getFloatValue(CfgFile,"CausticsScale"));
			mHydrax->setCausticsPower(_getFloatValue(CfgFile,"CausticsPower"));
			mHydrax->setCausticsEnd(_getFloatValue(CfgFile,"CausticsEnd"));
		}

		if (_isStringInList(Cmpnts, "UnderwaterGodRays"))
		{
			mHydrax->setGodRaysExposure(_getVector3Value(CfgFile,"GodRaysExposure"));
			mHydrax->setGodRaysIntensity(_getFloatValue(CfgFile,"GodRaysIntensity"));
			mHydrax->getGodRaysManager()->SetSimulationSpeed(_getFloatValue(CfgFile,"GodRaysSpeed"));
			mHydrax->getGodRaysManager()->setNumberOfRays(_getIntValue(CfgFile,"GodRaysNumberOfRays"));
			mHydrax->getGodRaysManager()->setRaysSize(_getFloatValue(CfgFile,"GodRaysRaysSize"));
			mHydrax->getGodRaysManager()->setObjectIntersectionsEnabled(_getBoolValue(CfgFile,"GodRaysIntersections"));
		}
	}

	const Ogre::String CfgFileManager::_getRttCfgString() const
	{
		return
			_getCfgString("Rtt_Quality_Reflection",   mHydrax->getRttManager()->getTextureSize(RttManager::RTT_REFLECTION)) +
			_getCfgString("Rtt_Quality_Refraction",   mHydrax->getRttManager()->getTextureSize(RttManager::RTT_REFRACTION)) +
			_getCfgString("Rtt_Quality_Depth",        mHydrax->getRttManager()->getTextureSize(RttManager::RTT_DEPTH)) +
			_getCfgString("Rtt_Quality_URDepth",      mHydrax->getRttManager()->getTextureSize(RttManager::RTT_DEPTH_REFLECTION)) +
			_getCfgString("Rtt_Quality_GPUNormalMap", mHydrax->getRttManager()->getTextureSize(RttManager::RTT_GPU_NORMAL_MAP));
	}

	const void CfgFileManager::_loadRttSettings(Ogre::ConfigFile& CfgFile) const
	{
		mHydrax->getRttManager()->setTextureSize(RttManager::RTT_REFLECTION,_getSizeValue(CfgFile,"Rtt_Quality_Reflection"));
		mHydrax->getRttManager()->setTextureSize(RttManager::RTT_REFRACTION,_getSizeValue(CfgFile,"Rtt_Quality_Refraction"));
		mHydrax->getRttManager()->setTextureSize(RttManager::RTT_DEPTH,_getSizeValue(CfgFile,"Rtt_Quality_Depth"));
		mHydrax->getRttManager()->setTextureSize(RttManager::RTT_DEPTH_REFLECTION,_getSizeValue(CfgFile,"Rtt_Quality_URDepth"));
		mHydrax->getRttManager()->setTextureSize(RttManager::RTT_GPU_NORMAL_MAP,_getSizeValue(CfgFile,"Rtt_Quality_GPUNormalMap"));
	}

	const Ogre::String CfgFileManager::_getVersionCfgString() const
	{
		return "HydraxVersion=" +
				// Major
				Ogre::StringConverter::toString(HYDRAX_VERSION_MAJOR)+"." +
				// Minor
				Ogre::StringConverter::toString(HYDRAX_VERSION_MINOR)+"." +
				// Patch
				Ogre::StringConverter::toString(HYDRAX_VERSION_PATCH)+"\n\n";
	}

	const bool CfgFileManager::_checkVersion(Ogre::ConfigFile& CfgFile) const
	{
		// accept any
#if 0
		if(CfgFile.getSetting("HydraxVersion") != (
			    // Major
				Ogre::StringConverter::toString(HYDRAX_VERSION_MAJOR)+"."+
				// Minor
				Ogre::StringConverter::toString(HYDRAX_VERSION_MINOR)+"."+
				// Patch
				Ogre::StringConverter::toString(HYDRAX_VERSION_PATCH)))
		{
			HydraxLOG("Config file version doesn't correspond with Hydrax version.");

			return false;
		}
#endif // 0
		return true;
	}

	int CfgFileManager::_getIntValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<int>" + Name);

		if (Value == "")
		{
			return 0;
		}
		else
		{
			return Ogre::StringConverter::parseInt(Value);
		}
	}

	Ogre::Real CfgFileManager::_getFloatValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<float>" + Name);

		if (Value == "")
		{
			return 0;
		}
		else
		{
			return Ogre::StringConverter::parseReal(Value);
		}
	}

	bool CfgFileManager::_getBoolValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<bool>" + Name);

		if (Value == "")
		{
			return false;
		}
		else
		{
			return Ogre::StringConverter::parseBool(Value);
		}
	}

	Ogre::Vector2 CfgFileManager::_getVector2Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<vector2>" + Name);

		if (Value == "")
		{
			return Ogre::Vector2(0,0);
		}
		else
		{
			return Ogre::Vector2(Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[0]),
				                 Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[1]));
		}
	}

	Ogre::Vector3 CfgFileManager::_getVector3Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<vector3>" + Name);

		if (Value == "")
		{
			return Ogre::Vector3(0,0,0);
		}
		else
		{
			return Ogre::Vector3(Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[0]),
				                 Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[1]),
								 Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[2]));
		}
	}

	Size CfgFileManager::_getSizeValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<size>" + Name);

		if (Value == "")
		{
			return Size(0);
		}
		else
		{
			return Size(Ogre::StringConverter::parseInt(Ogre::StringUtil::split(Value, "x")[0]),
				        Ogre::StringConverter::parseInt(Ogre::StringUtil::split(Value, "x")[1]));
		}
	}
}
