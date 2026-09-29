// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "CarlaSettings.h"
#include "DynamicWeather.h"
#include "Settings/CameraDescription.h"
#include "Settings/LidarDescription.h"
#include "Util/IniFile.h"
#include "Package.h"
#include "CommandLine.h"
#include "UnrealMathUtility.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstance.h"

// INI file sections.
#define S_CARLA_SERVER					TEXT("CARLA/Server")
#define S_CARLA_LEVELSETTINGS			TEXT("CARLA/LevelSettings")
#define S_CARLA_SENSOR					TEXT("CARLA/Sensor")
#define S_CARLA_QUALITYSETTINGS			TEXT("CARLA/QualitySettings")
#define S_FMRI							TEXT("fMRI")
#define S_TRAFFIC						TEXT("Traffic")
#define S_NAVIGATION_EXPERIMENT			TEXT("NavigationExperiment")
#define S_TRACK_EXPERIMENT				TEXT("TrackExperiment")
#define S_CIRCUIT_EXPERIMENT			TEXT("CircuitExperiment")
#define S_HUD							TEXT("HUD")
#define S_FORAGING						TEXT("Foraging")
#define S_LEARNINGPRACTICE				TEXT("LearningPractice")
#define S_CONTROLS						TEXT("Controls")

// =============================================================================
// -- Static variables & constants ---------------------------------------------
// =============================================================================

const FName UCarlaSettings::CARLA_ROAD_TAG = FName("CARLA_ROAD");
const FName UCarlaSettings::CARLA_SKY_TAG = FName("CARLA_SKY");

// =============================================================================
// -- Static methods -----------------------------------------------------------
// =============================================================================

template<typename T>
static void ForEachSectionInName(const FString& SensorName, T&& Callback)
{
	TArray <FString> SubSections;
	SensorName.ParseIntoArray(SubSections, TEXT("/"), true);
	check(SubSections.Num() > 0);
	FString Section = S_CARLA_SENSOR;
	Callback(Section);
	for (FString& SubSection : SubSections)
	{
		Section += TEXT("/");
		Section += SubSection;
		Callback(Section);
	}
}

static FString GetSensorType(const FIniFile& ConfigFile, const FString& SensorName)
{
	FString SensorType;
	ForEachSectionInName(SensorName, [&](const auto& Section)
	{
		ConfigFile.GetString(*Section, TEXT("SensorType"), SensorType);
	});
	return SensorType;
}

static void LoadSensorFromConfig(const FIniFile& ConfigFile, USensorDescription& Sensor)
{
	ForEachSectionInName(Sensor.Name, [&](const auto& Section)
	{
		Sensor.Load(ConfigFile, Section);
	});
}

template<typename T>
static T* MakeSensor(UObject* Parent, const FString& Name, const FString& Type)
{
	auto* Sensor = NewObject<T>(Parent);
	Sensor->Name = Name;
	Sensor->Type = Type;
	return Sensor;
}

static USensorDescription* MakeSensor(const FIniFile& ConfigFile, UObject* Parent, const FString& SensorName)
{
	const auto SensorType = GetSensorType(ConfigFile, SensorName);
	if (SensorType == TEXT("CAMERA"))
	{
		return MakeSensor<UCameraDescription>(Parent, SensorName, SensorType);
	}
	else if (SensorType == TEXT("LIDAR_RAY_CAST"))
	{
		return MakeSensor<ULidarDescription>(Parent, SensorName, SensorType);
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Invalid sensor type '%s'"), *SensorType);
		return nullptr;
	}
}

static void LoadSettingsFromConfig(const FIniFile& ConfigFile, UCarlaSettings& Settings, const bool bLoadCarlaServerSection)
{
	// CarlaServer.
	if (bLoadCarlaServerSection) {
		ConfigFile.GetBool(S_CARLA_SERVER, TEXT("UseNetworking"), Settings.bUseNetworking);
		ConfigFile.GetInt(S_CARLA_SERVER, TEXT("WorldPort"), Settings.WorldPort);
		ConfigFile.GetInt(S_CARLA_SERVER, TEXT("ServerTimeOut"), Settings.ServerTimeOut);
	}
	ConfigFile.GetBool(S_CARLA_SERVER, TEXT("SynchronousMode"), Settings.bSynchronousMode);
	ConfigFile.GetBool(S_CARLA_SERVER, TEXT("SendNonPlayerAgentsInfo"), Settings.bSendNonPlayerAgentsInfo);
	// LevelSettings.
	ConfigFile.GetString(S_CARLA_LEVELSETTINGS, TEXT("PlayerVehicle"), Settings.PlayerVehicle);
	ConfigFile.GetInt(S_CARLA_LEVELSETTINGS, TEXT("NumberOfVehicles"), Settings.NumberOfVehicles);
	ConfigFile.GetInt(S_CARLA_LEVELSETTINGS, TEXT("NumberOfPedestrians"), Settings.NumberOfPedestrians);
	ConfigFile.GetInt(S_CARLA_LEVELSETTINGS, TEXT("WeatherId"), Settings.WeatherId);
	ConfigFile.GetInt(S_CARLA_LEVELSETTINGS, TEXT("SeedVehicles"), Settings.SeedVehicles);
	ConfigFile.GetInt(S_CARLA_LEVELSETTINGS, TEXT("SeedPedestrians"), Settings.SeedPedestrians);
	ConfigFile.GetBool(S_CARLA_LEVELSETTINGS, TEXT("DisableTwoWheeledVehicles"), Settings.bDisableTwoWheeledVehicles);

	// QualitySettings.
	FString sQualityLevel;
	ConfigFile.GetString(S_CARLA_QUALITYSETTINGS, TEXT("QualityLevel"), sQualityLevel);
	Settings.SetQualitySettingsLevel(UQualitySettings::FromString(sQualityLevel));
	ConfigFile.GetString(S_CARLA_QUALITYSETTINGS, TEXT("Resolution"), Settings.Resolution);

	// Sensors.
	FString Sensors;
	ConfigFile.GetString(S_CARLA_SENSOR, TEXT("Sensors"), Sensors);
	TArray <FString> SensorNames;
	Sensors.ParseIntoArray(SensorNames, TEXT(","), true);
	for (const FString& Name : SensorNames)
	{
		auto* Sensor = MakeSensor(ConfigFile, &Settings, Name);
		if (Sensor != nullptr)
		{
			LoadSensorFromConfig(ConfigFile, *Sensor);
			Sensor->Validate();
			Settings.bSemanticSegmentationEnabled |= Sensor->RequiresSemanticSegmentation();
			Settings.SensorDescriptions.Add(Name, Sensor);
		}
	}

	// General experiment settings
	FString sExperimentType;
	if (ConfigFile.GetString(S_FMRI, TEXT("ExperimentType"), sExperimentType))
		Settings.SetExperimentType(UExperimentType::FromString(sExperimentType));

	ConfigFile.GetBool(S_FMRI, TEXT("SensorsOnlyOnReplay"), Settings.SensorsOnlyOnReplay);
	ConfigFile.GetBool(S_FMRI, TEXT("AutoTriggerDemoRecording"), Settings.AutoTriggerDemoRecording);
	ConfigFile.GetFloat(S_FMRI, TEXT("SecondsBeforeAISteering"), Settings.SecondsBeforeAISteering);
	ConfigFile.GetBool(S_FMRI, TEXT("AutoEyetrackingCalibration"), Settings.AutoEyetrackingCalibration);
	ConfigFile.GetString(S_FMRI, TEXT("Subject"), Settings.Subject);
	ConfigFile.GetBool(S_FMRI, TEXT("RememberLocationBetweenSpawns"), Settings.RememberLocationBetweenSpawns);
	ConfigFile.GetBool(S_FMRI, TEXT("ButtonBoxControls"), Settings.ButtonBoxControls);
	ConfigFile.GetFloat(S_FMRI, TEXT("SecondsToDemoStop"), Settings.SecondsToDemoStop);
	ConfigFile.GetBool(S_FMRI, TEXT("governorOnPlayer"), Settings.bGovernorOnPlayer);

	// Navigation experiment settings
	ConfigFile.GetString(S_NAVIGATION_EXPERIMENT, TEXT("Destinations"), Settings.DestinationsFile);
	Settings.InitializeActiveDestinationSets(Settings.DestinationsFile);
	ConfigFile.GetFloat(S_NAVIGATION_EXPERIMENT, TEXT("Proximity"), Settings.Proximity);
	ConfigFile.GetFloat(S_NAVIGATION_EXPERIMENT, TEXT("ExclusionMin"), Settings.ExclusionMin);
	ConfigFile.GetFloat(S_NAVIGATION_EXPERIMENT, TEXT("ExclusionMaxStart"), Settings.ExclusionMaxStart);
	ConfigFile.GetFloat(S_NAVIGATION_EXPERIMENT, TEXT("ExclusionMaxEnd"), Settings.ExclusionMaxEnd);
	ConfigFile.GetFloat(S_NAVIGATION_EXPERIMENT, TEXT("ExclusionMinProbability"), Settings.ExclusionMinProbability);
	ConfigFile.GetString(S_NAVIGATION_EXPERIMENT, TEXT("NavigationSequenceFile"), Settings.NavigationSequenceFile);
	FString sDestinationPickingMode;
	if (ConfigFile.GetString(S_NAVIGATION_EXPERIMENT, TEXT("DestinationPickingMode"), sDestinationPickingMode))
		Settings.SetDestinationPickingMode(UDestinationPickingMode::FromString(sDestinationPickingMode));

	// traffic
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("maxSubjectBias"), Settings.maxSubjectBias);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("repulsionBias"), Settings.repulsionBias);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("inversionRadius"), Settings.inversionRadius);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("maxBiasRadius"), Settings.maxBiasRadius);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("minBiasRadius"), Settings.minBiasRadius);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("defaultSpeedLimit"), Settings.defaultSpeedLimit);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("reincarnation"), Settings.reincarnation);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("offScreenLiveTIme"), Settings.offScreenLiveTime);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("respawnInterval"), Settings.respawnInterval);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("minSpawnDistance"), Settings.minSpawnDistance);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("maxSpawnDistance"), Settings.maxSpawnDistance);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("minTimeFromPlayer"), Settings.minSpawnTimeDistance);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("maxSpawnAngle"), Settings.maxSpawnAngle);
	ConfigFile.GetInt(S_TRAFFIC, TEXT("numberToRespawnInOneGo"), Settings.numberToRespawnInOneGo);

	// low traffic config
	ConfigFile.GetInt(S_TRAFFIC, TEXT("lowTrafficNumberOfVehicles"), Settings.lowTrafficNumberOfVehicles);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("lowTrafficMinSpawnDistance"), Settings.lowTrafficMinSpawnDistance);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("lowTrafficMaxSpawnDistance"), Settings.lowTrafficMaxSpawnDistance);

	// high traffic config
	ConfigFile.GetInt(S_TRAFFIC, TEXT("highTrafficNumberOfVehicles"), Settings.highTrafficNumberOfVehicles);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("highTrafficMinSpawnDistance"), Settings.highTrafficMinSpawnDistance);
	ConfigFile.GetFloat(S_TRAFFIC, TEXT("highTrafficMaxSpawnDistance"), Settings.highTrafficMaxSpawnDistance);

	// circuit experiment parameters
	ConfigFile.GetFloat(S_CIRCUIT_EXPERIMENT, TEXT("MinDestinationFraction"), Settings.CircuitMinDestinationFraction);
	ConfigFile.GetFloat(S_CIRCUIT_EXPERIMENT, TEXT("MaxDestinationFraction"), Settings.CircuitMaxDestinationFraction);
	ConfigFile.GetBool(S_CIRCUIT_EXPERIMENT, TEXT("DirectionalCircuit"), Settings.bDirectionalCircuit);

	// HUD settings
	// the first 3 were moved from section fMRI, so we do this checking thing for account for older configs
	if (!ConfigFile.GetBool(S_HUD, TEXT("ShowFPS"), Settings.ShowFPS))
		ConfigFile.GetBool(S_FMRI, TEXT("ShowFPS"), Settings.ShowFPS);
	if (!ConfigFile.GetBool(S_HUD, TEXT("ShowNavigationInfo"), Settings.ShowNavigationInfo))
		ConfigFile.GetBool(S_FMRI, TEXT("ShowNavigationInfo"), Settings.ShowNavigationInfo);
	if (!ConfigFile.GetBool(S_HUD, TEXT("ShowDebugInfo"), Settings.ShowDebugInfo))
		ConfigFile.GetBool(S_FMRI, TEXT("ShowDebugInfo"), Settings.ShowDebugInfo);
	FString sReticuleType;
	if (ConfigFile.GetString(S_HUD, TEXT("ReticuleType"), sReticuleType))
		Settings.ReticuleType = UReticuleType::FromString(sReticuleType);
	ConfigFile.GetBool(S_HUD, TEXT("ShowPoints"), Settings.ShowPoints);

	// Foraging settings
	ConfigFile.GetInt(S_FORAGING, TEXT("MinNumberTargets"), Settings.MinNumberTargets);
	ConfigFile.GetInt(S_FORAGING, TEXT("MaxNumberTargets"), Settings.MaxNumberTargets);
	ConfigFile.GetFloat(S_FORAGING, TEXT("AvailableTargetMultiplier"), Settings.AvailableTargetMultiplier);
	ConfigFile.GetFloat(S_FORAGING, TEXT("ValueResetProbability"), Settings.ValueResetProbability);
	ConfigFile.GetInt(S_FORAGING, TEXT("MinStableTrials"), Settings.MinStableTrials);
	ConfigFile.GetInt(S_FORAGING, TEXT("LowValuePoints"), Settings.LowValuePoints);
	ConfigFile.GetInt(S_FORAGING, TEXT("MedValuePoints"), Settings.MedValuePoints);
	ConfigFile.GetInt(S_FORAGING, TEXT("HighValuePoints"), Settings.HighValuePoints);
	ConfigFile.GetInt(S_FORAGING, TEXT("BaseTrialTime"), Settings.BaseTrialTime);
	ConfigFile.GetInt(S_FORAGING, TEXT("MaxTrialTime"), Settings.MaxTrialTime);
	ConfigFile.GetInt(S_FORAGING, TEXT("AdditionalItemTime"), Settings.AdditionalItemTime);
	ConfigFile.GetFloat(S_FORAGING, TEXT("CollectionSpeed"), Settings.CollectionSpeed);

	// Learning practice settings
	ConfigFile.GetInt(S_LEARNINGPRACTICE, TEXT("NumberTargets"), Settings.NumberTargets);
	ConfigFile.GetBool(S_LEARNINGPRACTICE, TEXT("DestinationPoints"), Settings.DestinationPoints);
	// Controls settings
	ConfigFile.GetFloat(S_CONTROLS, TEXT("SteerSensitivity"), Settings.SteerSensitivity);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("ThrottleSensitivity"), Settings.ThrottleSensitivity);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("BrakeSensitivity"), Settings.BrakeSensitivity);

	ConfigFile.GetFloat(S_CONTROLS, TEXT("SteerQuartile"), Settings.SteerQuartile);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("ThrottleQuartile"), Settings.ThrottleQuartile);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("BrakeQuartile"), Settings.BrakeQuartile);

	ConfigFile.GetFloat(S_CONTROLS, TEXT("SteerMidpoint"), Settings.SteerMidpoint);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("ThrottleMidpoint"), Settings.ThrottleMidpoint);
	ConfigFile.GetFloat(S_CONTROLS, TEXT("BrakeMidpoint"), Settings.BrakeMidpoint);

	ConfigFile.GetBool(S_CONTROLS, TEXT("HelpButtonActive"), Settings.HelpButtonActive);
}

static bool GetSettingsFilePathFromCommandLine(FString& Value)
{
	if (FParse::Value(FCommandLine::Get(), TEXT("-carla-settings="), Value))
	{
		if (FPaths::IsRelative(Value))
		{
			// 12-23-2020: moving to all config files in Config\Experiment Configs
			// instead of in the root directory of the game
			// and drop the need to explicitly specify the *.ini file extension
			// this commented line is the old version
			//Value = FPaths::ConvertRelativePathToFull(FPaths::LaunchDir(), Value);
			Value = Value.EndsWith(TEXT(".ini"))	?
						FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Experiment Configs"),
										Value)		:
						FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Experiment Configs"),
										Value, TEXT(".ini"));
			return true;
		}
	}
	return false;
}

// =============================================================================
// -- UCarlaSettings -----------------------------------------------------------
// =============================================================================

EQualitySettingsLevel UQualitySettings::FromString(const FString& SQualitySettingsLevel)
{
	if (SQualitySettingsLevel.Equals("Low")) return EQualitySettingsLevel::Low;
	if (SQualitySettingsLevel.Equals("Medium")) return EQualitySettingsLevel::Medium;
	if (SQualitySettingsLevel.Equals("High")) return EQualitySettingsLevel::High;
	if (SQualitySettingsLevel.Equals("Epic")) return EQualitySettingsLevel::Epic;

	return EQualitySettingsLevel::None;
}

FString UQualitySettings::ToString(EQualitySettingsLevel QualitySettingsLevel)
{
	const UEnum* ptr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EQualitySettingsLevel"), true);
	if (!ptr)
		return FString("Invalid");
	return ptr->GetNameStringByIndex(static_cast<int32>(QualitySettingsLevel));
}

void UCarlaSettings::SetQualitySettingsLevel(EQualitySettingsLevel newQualityLevel)
{
	QualitySettingsLevel = newQualityLevel;
}

void UCarlaSettings::LoadSettings()
{
	CurrentFileName = TEXT("");
	// Load settings from project Config folder if present.
	LoadSettingsFromFile(FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("CarlaSettings.ini")), false);
	// Load settings given by command-line arg if provided.
	{
		FString FilePath;
		if (GetSettingsFilePathFromCommandLine(FilePath))
		{
			LoadSettingsFromFile(FilePath, true);
		}
	}
	// Command-line overrides text settings.
	{
		bUseNetworking = false;

#define GET_COMMANDLINE_ARG_OUT(arg, out) FParse::Value(FCommandLine::Get(), arg, out)
#define GET_COMMANDLINE_ARG(arg) FParse::Param(FCommandLine::Get(), arg)

		FString quality;
		if (FParse::Value(FCommandLine::Get(), TEXT("-quality="), quality))
		{
			QualitySettingsLevel = UQualitySettings::FromString(quality);
		}
		if (Resolution.StartsWith("f"))	// no resolution was specified, set with command line args if given
		{
			int resX, resY;
			bool windowed = GET_COMMANDLINE_ARG(TEXT("windowed"));
			if (GET_COMMANDLINE_ARG_OUT(TEXT("-ResX="), resX) && GET_COMMANDLINE_ARG_OUT(TEXT("-ResY="), resY))
			{
				SetResolution(resX, resY, !windowed);
			}
			else Resolution = FString(windowed ? "w" : "f");
		}
	}
	// Launcher will inform Game of build version and time, specificlly state when packaged and launched
	// with a fallback to the one stored in the source, which may be out of date
	{
		FParse::Value(FCommandLine::Get(), TEXT("-compilingMachine="), CompilingMachine);
		FParse::Value(FCommandLine::Get(), TEXT("-branch="), GitBranch);
		FParse::Value(FCommandLine::Get(), TEXT("-version="), Version);
		FParse::Value(FCommandLine::Get(), TEXT("-date="), BuildDate);
	}
	if (NavigationSequenceFile.Equals(FString("default")))
	{
		NavigationSequenceFile = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("sequence.txt"));
	}
	else
	{
		NavigationSequenceFile = FPaths::Combine(FPaths::ProjectConfigDir(), NavigationSequenceFile);
	}

	// the renderall command must be sent from the command line so we don't accidentally 
	// trigger it with a saved config
	int value;
	if (FParse::Value(FCommandLine::Get(), TEXT("-RenderAll="), value))
	{
		RenderAll = value > 0 ? value : 0;
	}
}

void UCarlaSettings::LoadSettingsFromString(const FString& INIFileContents)
{
	UE_LOG(LogCarla, Log, TEXT("Loading CARLA settings from string"));
	ResetSensorDescriptions();
	FIniFile ConfigFile;
	ConfigFile.ProcessInputFileContents(INIFileContents);
	constexpr bool bLoadCarlaServerSection = false;
	LoadSettingsFromConfig(ConfigFile, *this, bLoadCarlaServerSection);
	CurrentFileName = TEXT("<string-provided-by-client>");
}

void UCarlaSettings::LoadWeatherDescriptions()
{
	WeatherDescriptions.Empty();
	ADynamicWeather::LoadWeatherDescriptionsFromFile(MapName, WeatherDescriptions);
	check(WeatherDescriptions.Num() > 0);
}

void UCarlaSettings::ValidateWeatherId()
{
	if (WeatherId >= WeatherDescriptions.Num())
	{
		UE_LOG(LogCarla, Error, TEXT("Provided weather id %d cannot be found"), WeatherId);
		WeatherId = -1;
	}
}

void UCarlaSettings::LogSettings() const
{
	auto EnabledDisabled = [](bool bValue) { return (bValue ? TEXT("Enabled") : TEXT("Disabled")); };
	UE_LOG(LogCarla, Log, TEXT("== CARLA Settings =============================================================="));
	UE_LOG(LogCarla, Log, TEXT("This version was built on %s on %s"), *BuildDate, *CompilingMachine);
	UE_LOG(LogCarla, Log, TEXT("Version %s"), *Version);
	UE_LOG(LogCarla, Log, TEXT("Git branch %s"), *GitBranch);
	UE_LOG(LogCarla, Log, TEXT("Last settings file loaded: %s"), *CurrentFileName);
	UE_LOG(LogCarla, Log, TEXT("[%s]"), S_CARLA_SERVER);
	UE_LOG(LogCarla, Log, TEXT("Networking = %s"), EnabledDisabled(bUseNetworking));
	UE_LOG(LogCarla, Log, TEXT("World Port = %d"), WorldPort);
	UE_LOG(LogCarla, Log, TEXT("Server Time-out = %d ms"), ServerTimeOut);
	UE_LOG(LogCarla, Log, TEXT("Synchronous Mode = %s"), EnabledDisabled(bSynchronousMode));
	UE_LOG(LogCarla, Log, TEXT("Send Non-Player Agents Info = %s"), EnabledDisabled(bSendNonPlayerAgentsInfo));
	UE_LOG(LogCarla, Log, TEXT("[%s]"), S_CARLA_LEVELSETTINGS);
	UE_LOG(LogCarla, Log, TEXT("Player Vehicle        = %s"), (PlayerVehicle.IsEmpty() ? TEXT("Default") : *PlayerVehicle));
	UE_LOG(LogCarla, Log, TEXT("Number Of Vehicles    = %d"), NumberOfVehicles);
	UE_LOG(LogCarla, Log, TEXT("Number Of Pedestrians = %d"), NumberOfPedestrians);
	UE_LOG(LogCarla, Log, TEXT("Weather Id = %d"), WeatherId);
	UE_LOG(LogCarla, Log, TEXT("Seed Vehicle Spawner = %d"), SeedVehicles);
	UE_LOG(LogCarla, Log, TEXT("Seed Pedestrian Spawner = %d"), SeedPedestrians);
	UE_LOG(LogCarla, Log, TEXT("Two-Wheeled Vehicles = %s"), EnabledDisabled(!bDisableTwoWheeledVehicles));
	UE_LOG(LogCarla, Log, TEXT("Found %d available weather settings."), WeatherDescriptions.Num());
	for (auto i = 0; i < WeatherDescriptions.Num(); ++i)
	{
		UE_LOG(LogCarla, Log, TEXT("  * %d - %s"), i, *WeatherDescriptions[i].Name);
	}
	UE_LOG(LogCarla, Log, TEXT("[%s]"), S_CARLA_QUALITYSETTINGS);
	UE_LOG(LogCarla, Log, TEXT("Quality Settings = %s"), *UQualitySettings::ToString(QualitySettingsLevel));

	UE_LOG(LogCarla, Log, TEXT("[%s]"), S_CARLA_SENSOR);
	UE_LOG(LogCarla, Log, TEXT("Added %d sensors."), SensorDescriptions.Num());
	UE_LOG(LogCarla, Log, TEXT("Semantic Segmentation = %s"), EnabledDisabled(bSemanticSegmentationEnabled));
	for (auto&& Sensor : SensorDescriptions)
	{
		check(Sensor.Value != nullptr);
		Sensor.Value->Log();
	}
	UE_LOG(LogFMRI, Log, TEXT("Experiment type %s"), *UExperimentType::ToString(ExperimentType));
	UE_LOG(LogFMRI, Log, TEXT("Using %s for controls"), ButtonBoxControls ? TEXT("button box") : TEXT("steering wheel & pedals"));
	UE_LOG(LogFMRI, Log, TEXT("Subject %s"), *Subject);
	UE_LOG(LogFMRI, Log, TEXT("Destinations file is %s"), *DestinationsFile);
	UE_LOG(LogFMRI, Log, TEXT("Proximity for arrival is %.2f"), Proximity);
	UE_LOG(LogFMRI, Log, TEXT("Exclusion distance (minimum) for next destination is %.2f"), ExclusionMin);
	UE_LOG(LogFMRI, Log, TEXT("Exclusion distance maximum start for next destination (p = 1) is %.2f"), ExclusionMaxStart);
	UE_LOG(LogFMRI, Log, TEXT("Exclusion distance maximum end for next destination (p = %.2f) is %.2f"), ExclusionMinProbability, ExclusionMaxEnd);
	UE_LOG(LogFMRI, Log, TEXT("Probability for destination linearly ramps down from 1 to 0 from start to end"));
	UE_LOG(LogFMRI, Log, TEXT("Deterministic destination sequence file at %s"), *NavigationSequenceFile);
	UE_LOG(LogFMRI, Log, TEXT("Destination picking mode %s"), *UDestinationPickingMode::ToString(DestinationPickingMode));

	UE_LOG(LogFMRI, Log, TEXT("Attach sensors only during replays: %s"), SensorsOnlyOnReplay ? TEXT("yes") : TEXT("no"));
	UE_LOG(LogFMRI, Log, TEXT("Remembering locations between spawns: %s"), RememberLocationBetweenSpawns ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogFMRI, Log, TEXT("Auto trigger demo recording by 1st TTL: %s"), AutoTriggerDemoRecording ? TEXT("Yes") : TEXT("No"));

	UE_LOG(LogFMRI, Log, TEXT("Seconds before auto steering: %:.2f"), SecondsBeforeAISteering);
	if (SecondsToDemoStop > 0)
	{
		UE_LOG(LogFMRI, Log, TEXT("Will auto stop demos after %.2f seconds without a TTL"), SecondsToDemoStop);
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Will not auto stop demos"));
	}

	UE_LOG(LogFMRI, Log, TEXT("Low traffic %d vehicles, min spawn %.0f m, max spawn %.0f"), lowTrafficNumberOfVehicles,
																							lowTrafficMinSpawnDistance,
																							lowTrafficMaxSpawnDistance);
	UE_LOG(LogFMRI, Log, TEXT("High traffic %d vehicles, min spawn %.0f m, max spawn %.0f"), highTrafficNumberOfVehicles,
																							 highTrafficMinSpawnDistance,
		  																					 highTrafficMaxSpawnDistance)

	UE_LOG(LogFMRI, Log, TEXT("Spawn location to be remembered: %s"), RememberLocationBetweenSpawns ? TEXT("true") : TEXT("false"));
	UE_LOG(LogCarla, Log, TEXT("Default speed limit %.2f"), defaultSpeedLimit);
	UE_LOG(LogCarla, Log, TEXT("Max bias towards subject %.2f"), maxSubjectBias);
	UE_LOG(LogCarla, Log, TEXT("Repulsion bias from subject %.2f"), repulsionBias);
	UE_LOG(LogCarla, Log, TEXT("Inversion radius %.2f m"), inversionRadius / 100);
	UE_LOG(LogCarla, Log, TEXT("Max bias radius %.2f m"), maxBiasRadius);
	UE_LOG(LogCarla, Log, TEXT("Min bias radius %.2f m"), minBiasRadius);
	UE_LOG(LogCarla, Log, TEXT("Subject speed will%s be limited to posted speed limits"), bGovernorOnPlayer ? TEXT("") : TEXT(" not"));
	UE_LOG(LogCarla, Log, TEXT("AI vehicles will reincarnate after %.2f seconds of non-movement"), reincarnation);
	UE_LOG(LogCarla, Log, TEXT("                             after %.2f seconds off screen"), offScreenLiveTime);
	UE_LOG(LogCarla, Log, TEXT("                             be spawned every %.2f seconds"), respawnInterval);
	UE_LOG(LogCarla, Log, TEXT("                             spawn at least %.2f meters away from subject"), minSpawnDistance);
	UE_LOG(LogCarla, Log, TEXT("                             at most %.2f meters away from subject"), maxSpawnDistance);
	UE_LOG(LogCarla, Log, TEXT("                             at most %.2f degrees away from subject view vector"), maxSpawnAngle);
	UE_LOG(LogCarla, Log, TEXT("                             at least %.2f seconds away from subject"), minSpawnTimeDistance);

	// experiment type-specific logs
	switch (ExperimentType)
	{
		case EExperimentType::Circuit :
			UE_LOG(LogCarla, Log, TEXT("==Circuit experiment=="));
			UE_LOG(LogCarla, Log, TEXT("Minimum next destination fraction %.2f"), CircuitMinDestinationFraction);
			UE_LOG(LogCarla, Log, TEXT("Maximum next destination fraction %.2f"), CircuitMaxDestinationFraction);
			UE_LOG(LogCarla, Log, TEXT("Circuit %s directional"), bDirectionalCircuit ? TEXT("is") : TEXT("isn't"));
			break;
		case EExperimentType::Foraging:
			UE_LOG(LogCarla, Log, TEXT("==Foraging experiment=="));
			UE_LOG(LogCarla, Log, TEXT("Minimum number of targets %d"), MinNumberTargets);
			UE_LOG(LogCarla, Log, TEXT("Maximum number of targets %d"), MaxNumberTargets);
			UE_LOG(LogCarla, Log, TEXT("Target availability multiplier %.2f"), AvailableTargetMultiplier);
			break;
		case EExperimentType::LearningPractice:
			UE_LOG(LogCarla, Log, TEXT("==Learning Practice experiment=="));
			UE_LOG(LogCarla, Log, TEXT("Number of targets %d"), NumberTargets);
			UE_LOG(LogCarla, Log, TEXT("Destination points %s"), DestinationPoints ? TEXT("enabled") : TEXT("disabled"));
			break;
		case EExperimentType::LearningPracticeList:
			UE_LOG(LogCarla, Log, TEXT("==Learning Practice List experiment=="));
			UE_LOG(LogCarla, Log, TEXT("Number of targets %d"), NumberTargets);
			UE_LOG(LogCarla, Log, TEXT("Destination points %s"), DestinationPoints ? TEXT("enabled") : TEXT("disabled"));
			break;
	}
	UE_LOG(LogCarla, Log, TEXT("================================================================================"));
}


void UCarlaSettings::GetActiveWeatherDescription(
		bool& bWeatherWasChanged,
		FWeatherDescription& WeatherDescription) const
{
	auto WeatherPtr = GetActiveWeatherDescription();
	if (WeatherPtr != nullptr)
	{
		WeatherDescription = *WeatherPtr;
		bWeatherWasChanged = true;
	}
	else
	{
		bWeatherWasChanged = false;
	}
}

const FWeatherDescription& UCarlaSettings::GetWeatherDescriptionByIndex(int32 Index)
{
	check(WeatherDescriptions.Num() > 0);
	FMath::Clamp(Index, 0, WeatherDescriptions.Num());
	return WeatherDescriptions[Index];
}

void UCarlaSettings::ResetSensorDescriptions()
{
	SensorDescriptions.Empty();
	bSemanticSegmentationEnabled = false;
}

void UCarlaSettings::LoadSettingsFromFile(const FString& FilePath, const bool bLogOnFailure)
{
	if (FPaths::FileExists(FilePath))
	{
		UE_LOG(LogCarla, Log, TEXT("Loading CARLA settings from \"%s\""), *FilePath);
		ResetSensorDescriptions();
		const FIniFile ConfigFile(FilePath);
		constexpr bool bLoadCarlaServerSection = true;
		LoadSettingsFromConfig(ConfigFile, *this, bLoadCarlaServerSection);
		CurrentFileName = FilePath;
	}
	else if (bLogOnFailure)
	{
		UE_LOG(LogCarla, Error, TEXT("Unable to find settings file \"%s\""), *FilePath);
	}
}

/// Experiment types
EExperimentType UExperimentType::FromString(const FString &experimentTypeString)
{
	if (experimentTypeString.Equals("Navigation")) 
		return EExperimentType::Navigation;
	if (experimentTypeString.Equals("Learning")) 
		return EExperimentType::Navigation;
	if (experimentTypeString.Equals("TrackRunning") || experimentTypeString.Equals("Track Running")) 
		return EExperimentType::TrackRunning;
	if (experimentTypeString.Equals("Circuit") || experimentTypeString.Equals("Closed Loop Track"))
		return EExperimentType::Circuit;
	if (experimentTypeString.Equals("TrackedLearning") || experimentTypeString.Equals("Tracked Learning")) 
		return EExperimentType::TrackedLearning;
	if (experimentTypeString.Equals("LearningTest") || experimentTypeString.Equals("Learning Test"))
		return EExperimentType::LearningTest;
	if (experimentTypeString.Equals("Foraging"))
		return EExperimentType::Foraging;
	if (experimentTypeString.Equals("TimedForaging") || experimentTypeString.Equals("Timed Foraging"))
		return EExperimentType::TimedForaging;
	if (experimentTypeString.Equals("LearningPractice") || experimentTypeString.Equals("Learning Practice"))
		return EExperimentType::LearningPractice;
	if (experimentTypeString.Equals("LearningPracticeList") || experimentTypeString.Equals("Learning Practice List"))
		return EExperimentType::LearningPracticeList;


	return EExperimentType::Other;
}

FString UExperimentType::ToString(EExperimentType experimentType)
{
	switch (experimentType)
	{
	case EExperimentType::Navigation:		return FString("Navigation");
	case EExperimentType::TrackRunning:		return FString("Track Running");
	case EExperimentType::Learning:			return FString("Learning");
	case EExperimentType::TrackedLearning:	return FString("Tracked Learning");
	case EExperimentType::Circuit:			return FString("Closed Loop Track");
	case EExperimentType::LearningTest:		return FString("Learning Test");
	case EExperimentType::Foraging:			return FString("Foraging");
	case EExperimentType::TimedForaging:	return FString("Timed Foraging");
	case EExperimentType::LearningPractice:	return FString("Learning Practice");
	case EExperimentType::LearningPracticeList: return FString("Learning Practice List");
	case EExperimentType::Other:
	default: return FString("Other");
	}
}

void UCarlaSettings::SetResolution(int width, int height, bool fullscreen)
{
	Resolution = FString::Printf(TEXT("%dx%d%c"), width, height, fullscreen ? 'f' : 'w');
}

void UCarlaSettings::SetExperimentType(EExperimentType newExperimentType)
{
	this->ExperimentType = newExperimentType;
}


/// Reticule types
EReticuleType UReticuleType::FromString(const FString &reticuleType)
{
	if (reticuleType.Equals("White"))
		return EReticuleType::White;
	if (reticuleType.Equals("None"))
		return EReticuleType::None;
	return EReticuleType::Multiply;
}

FString UReticuleType::ToString(EReticuleType reticuleType)
{
	switch (reticuleType)
	{
	case EReticuleType::White:
		return FString("White");
	case EReticuleType::None:
		return FString("None");
	default:
		return FString("Multiply");
	}
}

void UCarlaSettings::SetDestinationPickingMode(EDestinationPickingMode destinationPickingMode)
{
	this->DestinationPickingMode = destinationPickingMode;
}

// Destination sets
EDestinationSet UDestinationSet::FromString(const FString &destinationSetString)
{
	if (destinationSetString.Equals("Circuit")) return   	EDestinationSet::CircuitExp;
	if (destinationSetString.Equals("Suburbs")) return		EDestinationSet::Suburbs;
	if (destinationSetString.Equals("Rural")) return 		EDestinationSet::Rural;
	if (destinationSetString.Equals("Midtown")) return 	EDestinationSet::Midtown;
	if (destinationSetString.Equals("Downtown")) return   	EDestinationSet::Downtown;
	if (destinationSetString.Equals("Southtown")) return	EDestinationSet::Southtown;
	if (destinationSetString.Equals("River South")) return	EDestinationSet::RiverSouth;
	if (destinationSetString.Equals("West Side")) return 	EDestinationSet::WestSide;
	if (destinationSetString.Equals("East Side")) return 	EDestinationSet::EastSide;
	if (destinationSetString.Equals("None")) return 		EDestinationSet::None;
	return EDestinationSet::Unknown;
}

FString UDestinationSet::ToString(EDestinationSet destinationSet)
{
	switch (destinationSet)
	{
	case EDestinationSet::CircuitExp:	return FString("Circuit");
	case EDestinationSet::Suburbs:		return FString("Suburbs");
	case EDestinationSet::Rural:		return FString("Rural");
	case EDestinationSet::Midtown:		return FString("Midtown");
	case EDestinationSet::Downtown:		return FString("Downtown");
	case EDestinationSet::Southtown:	return FString("Southtown");
	case EDestinationSet::RiverSouth:	return FString("River South");
	case EDestinationSet::WestSide:		return FString("West Side");
	case EDestinationSet::EastSide:		return FString("East Side");
	case EDestinationSet::None:			return FString("None");
	case EDestinationSet::Unknown:		
	default: return FString("Unknown Value");;
	}
}


/// Destination picking modes
EDestinationPickingMode UDestinationPickingMode::FromString(const FString &DestinationPickingMode)
{
	if (DestinationPickingMode.Equals("Pure Random") || DestinationPickingMode.Equals("PureRandom"))
		return EDestinationPickingMode::PureRandom;
	if (DestinationPickingMode.Equals("Pure Permuted") || DestinationPickingMode.Equals("PurePermuted"))
		return EDestinationPickingMode::PurePermuted;
	if (DestinationPickingMode.Equals("Biased Permuted") || DestinationPickingMode.Equals("BiasedPermuted"))
		return EDestinationPickingMode::BiasedPermuted;
	if (DestinationPickingMode.Equals("Predetermined"))
		return EDestinationPickingMode::Predetermined;

	// default to biased random
	return EDestinationPickingMode::BiasedRandom;
}

FString UDestinationPickingMode::ToString(EDestinationPickingMode DestinationPickingMode)
{
	switch (DestinationPickingMode)
	{
		case EDestinationPickingMode::PureRandom:
			return FString("Pure Random");
		case EDestinationPickingMode::PurePermuted:
			return FString("Pure Permuted");
		case EDestinationPickingMode::BiasedPermuted:
			return FString("Biased Permuted");
		case EDestinationPickingMode::Predetermined:
			return FString("Predetermined");
		default:
			return FString("Biased Random");
	}
}



void UCarlaSettings::InitializeActiveDestinationSets()
{
	for (uint8 i = 0; i <= (uint8)EDestinationSet::None; i++)
		ActiveDestinationSets.Add((EDestinationSet)i, false);
}

void UCarlaSettings::InitializeActiveDestinationSets(const FString &activeSets)
{
	InitializeActiveDestinationSets();
	if (activeSets.Equals(TEXT("all"),ESearchCase::IgnoreCase))
	{
		for (uint8 i = 0; i <= (uint8)EDestinationSet::None; i++)
			ActiveDestinationSets[(EDestinationSet)i] = true;
	}
	else
	{
		TArray<FString> activeSetArray = TArray<FString>();
		activeSets.ParseIntoArray(activeSetArray, TEXT(","), true);
		EDestinationSet set;
		for (int i  = 0; i < activeSetArray.Num(); i++)
		{
			set = UDestinationSet::FromString(activeSetArray[i]);
			if (set != EDestinationSet::Unknown)
				ActiveDestinationSets[set] = true;
			else
			{
				UE_LOG(LogFMRI, Log, TEXT("Unknown destination set %s"), *activeSetArray[i])
			}
		}
	}
}

bool UCarlaSettings::IsAnyDestinationSetActive() const
{
	if (ActiveDestinationSets.Num() < 1) return false;
	for (auto& element : ActiveDestinationSets)
		if (element.Value) return true;
	return false;
}

bool UCarlaSettings::IsDestinationSetActive(const EDestinationSet neighborhood) const
{
	const bool *isActive = ActiveDestinationSets.Find(neighborhood);
	if (isActive == nullptr)
		return false;
	return *isActive;
}

bool UCarlaSettings::SaveSettingsToFile(const FString &FilePath)
{
	UE_LOG(LogFMRI, Log, TEXT("Saving to %s"), *FilePath);

	FIniFile ConfigFile(FilePath);
	// set args are (const TCHAR* Section, const TCHAR* Key, const T Value)
	
	// we just set everything available in the CarlaSettings object
	// == Carla/Server ==
	ConfigFile.SetBool(S_CARLA_SERVER,	TEXT("UseNetworking"),				bUseNetworking);
	ConfigFile.SetInt(S_CARLA_SERVER,	TEXT("WorldPort"),					WorldPort);
	ConfigFile.SetInt(S_CARLA_SERVER,	TEXT("ServerTimeOut"),				ServerTimeOut);
	ConfigFile.SetBool(S_CARLA_SERVER,	TEXT("SynchronousMode"),			bSynchronousMode);
	ConfigFile.SetBool(S_CARLA_SERVER,	TEXT("SendNonPlayerAgentsInfo"),	bSendNonPlayerAgentsInfo);

	// == Carla/LevelSettings ==
	ConfigFile.SetString(S_CARLA_LEVELSETTINGS,	TEXT("PlayerVehicle"),				PlayerVehicle);
	ConfigFile.SetInt(S_CARLA_LEVELSETTINGS,	TEXT("NumberOfVehicles"),			NumberOfVehicles);
	ConfigFile.SetInt(S_CARLA_LEVELSETTINGS,	TEXT("NumberOfPedestrians"),		NumberOfPedestrians);
	ConfigFile.SetInt(S_CARLA_LEVELSETTINGS,	TEXT("WeatherID"),					WeatherId);
	ConfigFile.SetInt(S_CARLA_LEVELSETTINGS,	TEXT("SeedVehicles"),				SeedVehicles);
	ConfigFile.SetInt(S_CARLA_LEVELSETTINGS,	TEXT("SeedPedestrians"),			SeedPedestrians);
	ConfigFile.SetBool(S_CARLA_LEVELSETTINGS,	TEXT("DisableTwoWheeledVehicles"),	bDisableTwoWheeledVehicles);

	// == Carla/QualitySettings ==
	ConfigFile.SetString(S_CARLA_QUALITYSETTINGS, TEXT("QualityLevel"), QualitySettingsLevel == EQualitySettingsLevel::Epic ? TEXT("Epic") : TEXT("Low"));
	ConfigFile.SetString(S_CARLA_QUALITYSETTINGS, TEXT("Resolution"), Resolution);

	// We skip sensors because they are not modifiable in-game so this is guaranteed to be unchanged

	// == fMRI == general experiment settings
	ConfigFile.SetString(S_FMRI,	TEXT("ExperimentType"),					UExperimentType::ToString(ExperimentType));
	ConfigFile.SetBool(S_FMRI,		TEXT("SensorsOnlyOnReplay"),			SensorsOnlyOnReplay);
	ConfigFile.SetBool(S_FMRI,		TEXT("AutoTriggerDemoRecording"),		AutoTriggerDemoRecording);
	ConfigFile.SetFloat(S_FMRI,		TEXT("SecondsBeforeAISteering"),		SecondsBeforeAISteering);
	ConfigFile.SetBool(S_FMRI,		TEXT("AutoEyetrackingCalibration"),		AutoEyetrackingCalibration);
	ConfigFile.SetString(S_FMRI,		TEXT("Subject"),						Subject);
	ConfigFile.SetBool(S_FMRI,		TEXT("RememberLocationBetweenSpawns"),	RememberLocationBetweenSpawns);
	ConfigFile.SetBool(S_FMRI,		TEXT("ButtonBoxControls"),				ButtonBoxControls);
	ConfigFile.SetFloat(S_FMRI,		TEXT("SecondsToDemoStop"),				SecondsToDemoStop);
	ConfigFile.SetBool(S_FMRI,		TEXT("governorOnPlayer"),				bGovernorOnPlayer);

	// == NavigationExperiment ==	navigation settings
	FString activeDestinations = FString("");
	int nActive = 0;
	for (int i = 0; i < (int)EDestinationSet::None; i++)
	{
		if (ActiveDestinationSets[(EDestinationSet)i])
		{
			nActive++;
			if (activeDestinations.Len() > 0)
				activeDestinations.Append(",");
			activeDestinations.Append(UDestinationSet::ToString((EDestinationSet)i));
		}
	}
	if ((nActive == (int)EDestinationSet::None) || (nActive == 0))
	{
		activeDestinations.Empty();
		activeDestinations = FString("all");
	}

	ConfigFile.SetString(S_NAVIGATION_EXPERIMENT,	TEXT("Destinations"),				activeDestinations);
	ConfigFile.SetInt(S_NAVIGATION_EXPERIMENT,		TEXT("Proximity"),					Proximity);
	ConfigFile.SetInt(S_NAVIGATION_EXPERIMENT,		TEXT("ExclusionMin"),				ExclusionMin);
	ConfigFile.SetInt(S_NAVIGATION_EXPERIMENT,		TEXT("ExclusionMaxStart"),			ExclusionMaxStart);
	ConfigFile.SetInt(S_NAVIGATION_EXPERIMENT,		TEXT("ExclusionMaxEnd"),			ExclusionMaxEnd);
	ConfigFile.SetFloat(S_NAVIGATION_EXPERIMENT,	TEXT("ExclusionMinProbability"),	ExclusionMinProbability);
	ConfigFile.SetString(S_NAVIGATION_EXPERIMENT,	TEXT("NavigationSequenceFile"),		FPaths::GetCleanFilename(NavigationSequenceFile));
	ConfigFile.SetString(S_NAVIGATION_EXPERIMENT,	TEXT("DestinationPickingMode"),		UDestinationPickingMode::ToString(DestinationPickingMode));

	// == Traffic ==
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("maxSubjectBias"),			maxSubjectBias);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("repulsionBias"),			repulsionBias);
	ConfigFile.SetInt(S_TRAFFIC,	TEXT("inversionRadius"),		inversionRadius);
	ConfigFile.SetInt(S_TRAFFIC,	TEXT("maxBiasRadius"),			maxBiasRadius);
	ConfigFile.SetInt(S_TRAFFIC,	TEXT("minBiasRadius"),			minBiasRadius);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("defaultSpeedLimit"),		defaultSpeedLimit);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("reincarnation"),			reincarnation);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("offScreenLiveTIme"),		offScreenLiveTime);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("respawnInterval"),		respawnInterval);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("minSpawnDistance"),		minSpawnDistance);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("maxSpawnDistance"),		maxSpawnDistance);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("minTimeFromPlayer"),		minSpawnTimeDistance);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("maxSpawnAngle"),			maxSpawnAngle);
	ConfigFile.SetInt(S_TRAFFIC,	TEXT("numberToRespawnInOneGo"),	numberToRespawnInOneGo);

	ConfigFile.SetInt(S_TRAFFIC,	TEXT("lowTrafficNumberOfVehicles"), lowTrafficNumberOfVehicles);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("lowTrafficMinSpawnDistance"), lowTrafficMinSpawnDistance);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("lowTrafficMaxSpawnDistance"), lowTrafficMaxSpawnDistance);

	ConfigFile.SetInt(S_TRAFFIC,	TEXT("highTrafficNumberOfVehicles"), highTrafficNumberOfVehicles);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("highTrafficMinSpawnDistance"), highTrafficMinSpawnDistance);
	ConfigFile.SetFloat(S_TRAFFIC,	TEXT("highTrafficMaxSpawnDistance"), highTrafficMaxSpawnDistance);

	// == CircuitExperiment ==
	ConfigFile.SetFloat(S_CIRCUIT_EXPERIMENT,	TEXT("MinDestinationFraction"), CircuitMinDestinationFraction);
	ConfigFile.SetFloat(S_CIRCUIT_EXPERIMENT,	TEXT("MaxDestinationFraction"), CircuitMaxDestinationFraction);
	ConfigFile.SetBool(S_CIRCUIT_EXPERIMENT,	TEXT("DirectionalCircuit"),		bDirectionalCircuit);

	// == HUD settings ==
	ConfigFile.SetBool(S_HUD, TEXT("ShowFPS"),				ShowFPS);
	ConfigFile.SetBool(S_HUD, TEXT("ShowNavigationInfo"),		ShowNavigationInfo);
	ConfigFile.SetBool(S_HUD, TEXT("ShowDebugInfo"),			ShowDebugInfo);
	ConfigFile.SetString(S_HUD, TEXT("ReticuleType"), UReticuleType::ToString(ReticuleType));
	ConfigFile.SetBool(S_HUD, TEXT("ShowPoints"), ShowPoints);

	// == Foraging settings ==
	ConfigFile.SetInt(S_FORAGING, TEXT("MinNumberTargets"), MinNumberTargets);
	ConfigFile.SetInt(S_FORAGING, TEXT("MaxNumberTargets"), MaxNumberTargets);
	ConfigFile.SetFloat(S_FORAGING, TEXT("AvailableTargetMultiplier"), AvailableTargetMultiplier);
	ConfigFile.SetFloat(S_FORAGING, TEXT("ValueResetProbability"), ValueResetProbability);
	ConfigFile.SetInt(S_FORAGING, TEXT("MinStableTrials"), MinStableTrials);
	ConfigFile.SetInt(S_FORAGING, TEXT("LowValuePoints"), LowValuePoints);
	ConfigFile.SetInt(S_FORAGING, TEXT("MedValuePoints"), MedValuePoints);
	ConfigFile.SetInt(S_FORAGING, TEXT("HighValuePoints"), HighValuePoints);
	ConfigFile.SetInt(S_FORAGING, TEXT("BaseTrialTime"), BaseTrialTime);
	ConfigFile.SetInt(S_FORAGING, TEXT("MaxTrialTime"), MaxTrialTime);
	ConfigFile.SetInt(S_FORAGING, TEXT("AdditionalItemTime"), AdditionalItemTime);
	ConfigFile.SetFloat(S_FORAGING, TEXT("CollectionSpeed"), CollectionSpeed);

	// == Learning practice settings ==
	ConfigFile.SetInt(S_LEARNINGPRACTICE, TEXT("NumberTargets"), NumberTargets);
	ConfigFile.SetBool(S_LEARNINGPRACTICE, TEXT("DestinationPoints"), DestinationPoints);
	// == Controls settings ==
	ConfigFile.SetFloat(S_CONTROLS, TEXT("SteerSensitivity"), SteerSensitivity);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("ThrottleSensitivity"), ThrottleSensitivity);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("BrakeSensitivity"), BrakeSensitivity);

	ConfigFile.SetFloat(S_CONTROLS, TEXT("SteerQuartile"), SteerQuartile);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("ThrottleQuartile"), ThrottleQuartile);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("BrakeQuartile"), BrakeQuartile);

	ConfigFile.SetFloat(S_CONTROLS, TEXT("SteerMidpoint"), SteerMidpoint);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("ThrottleMidpoint"), ThrottleMidpoint);
	ConfigFile.SetFloat(S_CONTROLS, TEXT("BrakeMidpoint"), BrakeMidpoint);

	ConfigFile.SetBool(S_CONTROLS, TEXT("HelpButtonActive"), HelpButtonActive);

	bool success = ConfigFile.Write(FilePath);
	if (success)
		CurrentFileName = FilePath;
	return success;
}

bool UCarlaSettings::SaveSettings()
{
	return SaveSettingsToFile(CurrentFileName);
}

#undef S_CARLA_SERVER
#undef S_CARLA_LEVELSETTINGS
#undef S_CARLA_SENSOR