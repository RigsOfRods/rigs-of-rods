/*
--------------------------------------------------------------------------------
This source file is part of SkyX.
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

#include "SCfgFileManager.h"
#include "SkyX.h"

namespace SkyX
{
	CfgFileManager::CfgFileManager(SkyX* s, BasicController *c, Ogre::Camera *d)
		: mSkyX(s), mController(c), mCamera(d)
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


		mSkyX->setTimeMultiplier(_getFloatValue(CfgFile, "TimeMultiplier"));
		mController->setTime(_getVector3Value(CfgFile, "Time"));
		mController->setMoonPhase(_getFloatValue(CfgFile, "MoonPhase"));

		mSkyX->getAtmosphereManager()->setOptions(
			AtmosphereManager::Options(
			_getFloatValue(CfgFile, "_InnerRadius"),
			_getFloatValue(CfgFile, "_OuterRadius"),
			_getFloatValue(CfgFile, "_HeightPosition"),
			_getFloatValue(CfgFile, "_RayleighMultiplier"),
			_getFloatValue(CfgFile, "_MieMultiplier"),
			_getFloatValue(CfgFile, "_SunIntensity"),
			_getVector3Value(CfgFile, "_WaveLength"),
			_getFloatValue(CfgFile, "_G"),
			_getFloatValue(CfgFile, "_Exposure"),
			_getFloatValue(CfgFile, "_NumerOfSamples")
			));

		// Layered clouds
		if (_getBoolValue(CfgFile, "layeredclouds") && !_getBoolValue(CfgFile, "volClouds"))
		{
			// Create layer cloud
			if (mSkyX->getCloudsManager()->getCloudLayers().empty())
			{
				mSkyX->getCloudsManager()->add(CloudLayer::Options(
					_getFloatValue(CfgFile, "lHeight"),
					_getFloatValue(CfgFile, "Scale"),
					_getVector2Value(CfgFile, "WindDirection"),
					_getFloatValue(CfgFile, "TimeMultiplier"),
					_getFloatValue(CfgFile, "DistanceAttenuation"),
					_getFloatValue(CfgFile, "DetailAttenuation"),
					_getFloatValue(CfgFile, "HeightVolume"),
					_getFloatValue(CfgFile, "VolumetricDisplacement")
					));
			}
		}
		else
		{
			mSkyX->getVCloudsManager()->setWindSpeed(_getFloatValue(CfgFile, "WindSpeed"));
			mSkyX->getVCloudsManager()->setAutoupdate(_getBoolValue(CfgFile, "AutoUpdate"));
			mSkyX->getVCloudsManager()->setHeight(_getVector2Value(CfgFile, "vHeight"));

			VClouds::VClouds* vclouds = mSkyX->getVCloudsManager()->getVClouds();

			vclouds->setWindDirection(_getDegreeValue(CfgFile, "WindDirection"));

			vclouds->setAmbientColor(_getVector3Value(CfgFile, "AmbientColor"));
			vclouds->setLightResponse(_getVector4Value(CfgFile, "LightResponse"));
			vclouds->setAmbientFactors(_getVector4Value(CfgFile, "AmbientFactors"));
			vclouds->setWheater(_getVector2Value(CfgFile, "Wheater").x, _getVector2Value(CfgFile, "Wheater").y, _getBoolValue(CfgFile, "DelayedResponse"));

			// Create VClouds
			if (!mSkyX->getVCloudsManager()->isCreated())
			{
				// SkyX::MeshManager::getSkydomeRadius(...) works for both finite and infinite(=0) camera far clip distances
				mSkyX->getVCloudsManager()->create(mSkyX->getMeshManager()->getSkydomeRadius(mCamera));
			}

			vclouds->getLightningManager()->setEnabled(_getBoolValue(CfgFile, "lightnings"));

			if (vclouds->getLightningManager()->isEnabled())
			{
				vclouds->getLightningManager()->setAverageLightningApparitionTime(_getFloatValue(CfgFile, "AverageLightningApparitionTime"));
				vclouds->getLightningManager()->setLightningColor(_getVector3Value(CfgFile, "LightningColor"));
				vclouds->getLightningManager()->setLightningTimeMultiplier(_getFloatValue(CfgFile, "LightningTimeMultiplier"));
			}
		}
		return true;
	}

	const bool CfgFileManager::save(const Ogre::String& File, const Ogre::String& Path) const
	{
		Ogre::String Data =
			"#SkyX cfg file.\n\n";

		Data += "#SkyX version field\n";
		Data += _getVersionCfgString();

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

		SkyXLOG(File + " saved in " + Path + " .");

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
			SkyXLOG("CfgFileManager::_getCfgFile(...): " + File + " not found in any resource group.");
			Result.first = false;
		}

		SkyXLOG(File + " loaded.");
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

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const Ogre::Vector4 &Value)
	{
		return "<vector4>" + Name + "=" + Ogre::StringConverter::toString(Value.x) + "x" + Ogre::StringConverter::toString(Value.y) + "x" + Ogre::StringConverter::toString(Value.z) + "x" + Ogre::StringConverter::toString(Value.w) + "\n";
	}

	Ogre::String CfgFileManager::_getCfgString(const Ogre::String &Name, const Ogre::Degree &Value)
	{
		return "<degree>" + Name + "=" + Ogre::StringConverter::toString(Value) + "\n";
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

	const Ogre::String CfgFileManager::_getVersionCfgString() const
	{
		return "SkyXVersion=" +
				// Major
				Ogre::StringConverter::toString(SKYX_VERSION_MAJOR)+"." +
				// Minor
				Ogre::StringConverter::toString(SKYX_VERSION_MINOR) + "." +
				// Patch
				Ogre::StringConverter::toString(SKYX_VERSION_PATCH) + "\n\n";
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

	Ogre::Vector4 CfgFileManager::_getVector4Value(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<vector4>" + Name);

		if (Value == "")
		{
			return Ogre::Vector4(0, 0, 0, 0);
		}
		else
		{
			return Ogre::Vector4(
				Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[0]),
				Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[1]),
				Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[2]),
				Ogre::StringConverter::parseReal(Ogre::StringUtil::split(Value, "x")[3])
				);
		}
	}

	Ogre::Degree CfgFileManager::_getDegreeValue(Ogre::ConfigFile& CfgFile, const Ogre::String Name)
	{
		Ogre::String Value = CfgFile.getSetting("<degree>" + Name);

		if (Value == "")
		{
			return Ogre::Degree(0);
		}
		else
		{
			return Ogre::Degree(Ogre::StringConverter::parseReal(Value));
		}
	}
}
