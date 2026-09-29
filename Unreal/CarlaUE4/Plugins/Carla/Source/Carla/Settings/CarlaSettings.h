// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once
#include "Engine/StaticMesh.h"
#include "WeatherDescription.h"
#include "CarlaSettings.generated.h"

UENUM(BlueprintType)
enum class EQualitySettingsLevel : uint8
{
	None		UMETA(DisplayName = "Not set"),
	Low			UMETA(DisplayName = "Low"),
	Medium		UMETA(DisplayName = "Medium"),
	High		UMETA(DisplayName = "High"),
	Epic		UMETA(DisplayName = "Epic")
};


/// Experiment type enum
UENUM(BlueprintType)
enum class EExperimentType : uint8
{
	Navigation				UMETA(DisplayName = "Navigation"),
	TrackRunning			UMETA(DisplayName = "Track Running"),
	Learning				UMETA(DisplayName = "Learning"),
	TrackedLearning			UMETA(DisplayName = "Tracked Learning"),
	Circuit					UMETA(DisplayName = "Closed Loop Track"),
	LearningTest			UMETA(DisplayName = "Learning Test"),
	Foraging				UMETA(DisplayName = "Foraging"),
	TimedForaging			UMETA(DisplayName = "Timed Foraging"),
	LearningPractice		UMETA(DisplayName = "Learning Practice"),
	TimedLearningPractice	UMETA(DisplayName = "Timed Learning Practice"),
	LearningPracticeList	UMETA(DisplayName = "Learning Practice List"),
	Other					UMETA(DisplayName = "Other")
};


/// Neighborhoods/destination sets enum
UENUM(BlueprintType)
enum class EDestinationSet : uint8
{
	CircuitExp	UMETA(DisplayName = "Circuit"),
	Suburbs		UMETA(DisplayName = "Suburbs"),
	Rural		UMETA(DisplayName = "Rural"),
	Midtown		UMETA(DisplayName = "Midtown"),
	Downtown	UMETA(DisplayName = "Downtown"),
	Southtown	UMETA(DisplayName = "Southtown"),
	RiverSouth	UMETA(DisplayName = "River South"),
	WestSide	UMETA(DisplayName = "West Side"),
	EastSide	UMETA(DisplayName = "East Side"),
	None		UMETA(DisplayName = "None"),	// None indicates a destination not belonging to a neighborhood. Should not be used in practice.

	// catch-all for parsing unknown values from config file
	Unknown		UMETA(DisplayName = "Unknown value"),
};

/// Reticule Type
UENUM(BlueprintType)
enum class EReticuleType : uint8
{
	White		UMETA(DisplayName = "White"),
	Multiply	UMETA(DisplayName = "Multiply"),
	None		UMETA(DisplayName = "None"),
};

/// How to pick destinations
UENUM(BlueprintType)
enum class EDestinationPickingMode : uint8
{
	PureRandom		UMETA(DisplayName = "Pure Random"),		// randomly pick next destination
	BiasedRandom	UMETA(DisplayName = "Biased Random"),	// random, but biased by distance
	PurePermuted	UMETA(DisplayName = "Pure Permuted"),	// only pick unvisited destinations, random
	BiasedPermuted	UMETA(DisplayName = "Biased Permuted"),	// permuted but biased by distance
	Predetermined	UMETA(DisplayName = "Predetermined"),	// Use predetermined sequence
};


// For parsing quality settings to/from string
UCLASS(BlueprintType)
class CARLA_API UQualitySettings : public UObject
{
	GENERATED_BODY()

public:

 using uint_type = typename std::underlying_type<EQualitySettingsLevel>::type;

 UFUNCTION(BlueprintCallable)
 static EQualitySettingsLevel FromString(const FString &SQualitySettingsLevel);

 UFUNCTION(BlueprintCallable)
 static FString ToString(EQualitySettingsLevel QualitySettingsLevel);

 static constexpr uint_type ToUInt(EQualitySettingsLevel quality_settings_level)
 {
	 return static_cast<uint_type>(quality_settings_level);
 }
};


// For parsing experiment types to/from string
UCLASS(BlueprintType)
class CARLA_API UExperimentType : public UObject
{
	GENERATED_BODY()

public:

	using uint_type = typename std::underlying_type<EExperimentType>::type;

	UFUNCTION(BlueprintCallable)
	static EExperimentType FromString(const FString &experimentTypeString);

	UFUNCTION(BlueprintCallable)
	static FString ToString(EExperimentType experimentType);

	static constexpr uint_type ToUInt(EExperimentType experimentType)
	{
		return static_cast<uint_type>(experimentType);
	}
};


// Parsing neighborhoods to/from string
UCLASS(BlueprintType)
class CARLA_API UDestinationSet : public UObject
{
	GENERATED_BODY()

public:

	using uint_type = typename std::underlying_type<EDestinationSet>::type;

	UFUNCTION(BlueprintCallable)
	static EDestinationSet FromString(const FString &destinationSetString);

	UFUNCTION(BlueprintCallable)
	static FString ToString(EDestinationSet destinationSet);

	static constexpr uint_type ToUInt(EDestinationSet destinationSet)
	{
		return static_cast<uint_type>(destinationSet);
	}
};

// For parsing reticule types to/from string
UCLASS(BlueprintType)
class CARLA_API UReticuleType : public UObject
{
	GENERATED_BODY()

public:

	using uint_type = typename std::underlying_type<EReticuleType>::type;

	UFUNCTION(BlueprintCallable)
	static EReticuleType FromString(const FString &reticuleType);

	UFUNCTION(BlueprintCallable)
	static FString ToString(EReticuleType reticuleType);

	static constexpr uint_type ToUInt(EReticuleType reticuleType)
	{
		return static_cast<uint_type>(reticuleType);
	}
};


// For parsing destination picking types to/from string
UCLASS(BlueprintType)
class CARLA_API UDestinationPickingMode : public UObject
{
	GENERATED_BODY()

public:

	using uint_type = typename std::underlying_type<EDestinationPickingMode>::type;

	UFUNCTION(BlueprintCallable)
	static EDestinationPickingMode FromString(const FString &destinationPickingMode);

	UFUNCTION(BlueprintCallable)
	static FString ToString(EDestinationPickingMode destinationPickingMode);

	static constexpr uint_type ToUInt(EDestinationPickingMode destinationPickingMode)
	{
		return static_cast<uint_type>(destinationPickingMode);
	}
};


class USensorDescription;

/** Global settings for CARLA.
 * Setting object used to hold both config settings and editable ones in one place
 * To ensure the settings are saved to the specified config file make sure to add
 * props using the globalconfig or config meta.
 */
UCLASS(BlueprintType, Blueprintable, config = Game, defaultconfig)
class CARLA_API UCarlaSettings : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Sets the new quality settings level and make changes in the game related to it.
	 * @note This will not apply the quality settings. Use ApplyQualitySettings functions instead
	 * @param newQualityLevel Store the new quality
	 */
	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	void SetQualitySettingsLevel(EQualitySettingsLevel newQualityLevel);

	/** @return current quality settings level (could not be applied yet) */
	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	EQualitySettingsLevel GetQualitySettingsLevel() const { return QualitySettingsLevel; }

	/** Load the settings based on the command-line arguments and the INI file if provided. */
	void LoadSettings();
		
	/** Load settings from a specified file*/
	UFUNCTION(BlueprintCallable, Category = "CARLA Settings")
	void LoadSettingsFromFile(const FString &FilePath, bool bLogOnFailure = true);

	/** Load the settings from the given string (formatted as INI). CarlaServer section is ignored. */
	void LoadSettingsFromString(const FString &INIFileContents);

	/** Load weather description from config files. (There may be overrides for each map). */
	void LoadWeatherDescriptions();

	/** Check if requested weather id is present in WeatherDescriptions. */
	void ValidateWeatherId();

	/** Log settings values. */
	void LogSettings() const;

	/** Save methods*/
	UFUNCTION(BlueprintCallable, Category = "CARLA Settings")
	bool SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "CARLA Settings")
	bool SaveSettingsToFile(const FString &FilePath);

	const FWeatherDescription *GetActiveWeatherDescription() const
	{
		if ((WeatherId >= 0) && (WeatherId < WeatherDescriptions.Num())) {
			return &WeatherDescriptions[WeatherId];
		}
		return nullptr;
	}


	// Special overload for blueprints.
	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	void GetActiveWeatherDescription(
			bool &bWeatherWasChanged,
			FWeatherDescription &WeatherDescription) const;

	UFUNCTION(BlueprintCallable)
	const FWeatherDescription &GetWeatherDescriptionByIndex(int32 Index);

	///----------- constants ------------------
public:
	/**
	 * CARLA_ROAD name to tag road mesh actors
	 */
	static const FName CARLA_ROAD_TAG;
	/**
	 * CARLA_SKY name to tag the sky sphere (BPS) actors in the scenes
	 */
	static const FName CARLA_SKY_TAG;

private:

	/***/
	void ResetSensorDescriptions();

	/** File name of the settings file used to load this settings. Empty if none used. */
	UPROPERTY(Category = "CARLA Settings|Debug", VisibleAnywhere)
	FString CurrentFileName;

public:
	UFUNCTION(BlueprintPure, Category = "CARLA Settings")
	const FString& GetFileName() const
	{
		return CurrentFileName;
	}


	// ===========================================================================
	/// @name CARLA Server
	// ===========================================================================
	/// @{
public:

	/** If active, wait for the client to connect and control the pawn. */
	UPROPERTY(Category = "CARLA Server", VisibleAnywhere)
	bool bUseNetworking = false;

	/** World port to listen for client connections. */
	UPROPERTY(Category = "CARLA Server", VisibleAnywhere, meta = (EditCondition = bUseNetworking))
	uint32 WorldPort = 2000u;

	/** Time-out in milliseconds for the networking operations. */
	UPROPERTY(Category = "CARLA Server", VisibleAnywhere, meta = (EditCondition = bUseNetworking))
	uint32 ServerTimeOut = 10000u;

	/** In synchronous mode, CARLA waits every tick until the control from the
		* client is received.
		*/
	UPROPERTY(Category = "CARLA Server", VisibleAnywhere, meta = (EditCondition = bUseNetworking))
	bool bSynchronousMode = true;

	/** Send info about every non-player agent in the scene every frame. */
	UPROPERTY(Category = "CARLA Server", VisibleAnywhere, meta = (EditCondition = bUseNetworking))
	bool bSendNonPlayerAgentsInfo = false;

	/// @}
	// ===========================================================================
	/// @name Level Settings
	// ===========================================================================
	/// @{
public:

	/** Display name of the current map. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere, BlueprintReadWrite)
	FString MapName;

	/** Path to the pawn class of the player. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere)
	FString PlayerVehicle;

	/** Number of NPC vehicles to be spawned into the level. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere, BlueprintReadWrite)
	int NumberOfVehicles = 5u;

	/** Number of NPC pedestrians to be spawned into the level. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere, BlueprintReadWrite)
	int NumberOfPedestrians = 15u;

	/** Index of the weather setting to use. If negative, weather won't be changed. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere)
	int32 WeatherId = 1;

	/** Available weather settings. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere)
	TArray<FWeatherDescription> WeatherDescriptions;

	/** Random seed for the pedestrian spawner. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere)
	int32 SeedPedestrians = 123456789;

	/** Random seed for the vehicle spawner and destination sequence. */
	UPROPERTY(Category = "Level Settings", VisibleAnywhere)
	int32 SeedVehicles = 123456789;

	/** Disable bikes and motorbikes. */
	UPROPERTY(Category = "Level Settings", BlueprintReadOnly, VisibleAnywhere)
	bool bDisableTwoWheeledVehicles = false;

	/// @}

	// ===========================================================================
	/// @name Quality Settings
	// ===========================================================================
	/// @{
private:

	/** Quality Settings level. */
	UPROPERTY(Category = "Quality Settings", VisibleAnywhere, meta =(AllowPrivateAccess="true"))
	EQualitySettingsLevel QualitySettingsLevel = EQualitySettingsLevel::Epic;

public:
	UPROPERTY(Category = "Quality Settings", VisibleAnywhere, BlueprintReadWrite)
	FString Resolution = FString("1280x800w");

	UFUNCTION(Category = "Quality Settings", BlueprintCallable)
	void SetResolution(int width, int height, bool fullscreen = false);

	UFUNCTION(Category = "Quality Settings", BlueprintCallable)
	bool IsFullscreen() const
	{
		return !Resolution.EndsWith("w");
	}

	/** @TODO : Move Low quality vars to a generic map of structs with the quality level as key*/

	/** Low quality Road Materials.
	 * Uses slots name to set material for each part of the road for low quality
	 */
	UPROPERTY(Category = "Quality Settings/Low", BlueprintReadOnly, EditAnywhere, config, DisplayName="Road Materials List for Low Quality")
	TArray<FStaticMaterial> LowRoadMaterials;

	//distances
	/**
	 * Distance at which the light function should be completely faded to DisabledBrightness.
	 * This is useful for hiding aliasing from light functions applied in the distance.
	 */
	UPROPERTY(Category = "Quality Settings/Low", BlueprintReadOnly, EditAnywhere, config)
	float LowLightFadeDistance	= 1000.0f;

	/**
	 * Default low distance for all primitive components
	 */
	UPROPERTY(Category = "Quality Settings/Low", BlueprintReadOnly, EditAnywhere, config, meta = (ClampMin = "5000.0", ClampMax = "100000.0", UIMin = "5000.0", UIMax = "100000.0"))
	float LowStaticMeshMaxDrawDistance = 100000.0f;

	/**
	 * Default low distance for roads meshes
	 */
	UPROPERTY(Category = "Quality Settings/Low", BlueprintReadOnly, EditAnywhere, config, meta = (ClampMin = "5000.0", ClampMax = "25000.0", UIMin = "5000.0", UIMax = "25000.0"))
	float LowRoadPieceMeshMaxDrawDistance = 25000.0f;


	/** EPIC quality Road Materials.
	 * Uses slots name to set material for each part of the road for Epic quality
	 */
	UPROPERTY(Category = "Quality Settings/Epic", BlueprintReadOnly, EditAnywhere, config, DisplayName="Road Materials List for EPIC Quality")
	TArray<FStaticMaterial> EpicRoadMaterials;

	/// @}

	// ===========================================================================
	/// @name Sensors
	// ===========================================================================
	/// @{
public:

	/** Descriptions of the cameras to be attached to the player. */
	UPROPERTY(Category = "Sensors", BlueprintReadOnly, VisibleAnywhere)
	TMap<FString, USensorDescription *> SensorDescriptions;

	/** Whether semantic segmentation should be activated. The mechanisms for
		* semantic segmentation impose some performance penalties even if it is not
		* used, we only enable it if necessary.
		*/
	UPROPERTY(Category = "Sensors", BlueprintReadOnly, VisibleAnywhere)
	bool bSemanticSegmentationEnabled = false;

	/// @}

/// =================================================================
/// fMRI configs, distance units are unreal units, which in this project are CM
/// =================================================================
	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	void SetExperimentType(EExperimentType experimentType);

	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	EExperimentType GetExperimentType() const { return ExperimentType; }

	// repurposed to a list of ints of active destinations
	// the ints are the ID hashes of each location
	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	FString DestinationsFile = TEXT("Circuit,Rural,Suburbs,Midtown");

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	float Proximity = 500;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	float ExclusionMin = 750;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	float ExclusionMaxStart = 50000;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	float ExclusionMaxEnd = 100000;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	float ExclusionMinProbability = 0.0;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	bool SensorsOnlyOnReplay = true;

	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	bool AutoEyetrackingCalibration = false;

	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	FString Subject;

	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	void SetDestinationPickingMode(EDestinationPickingMode destinationPickingMode);

	UFUNCTION(BlueprintCallable, Category="CARLA Settings")
	EDestinationPickingMode GetDestinationPickingMode() const { return DestinationPickingMode; }

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	FString NavigationSequenceFile;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	bool RememberLocationBetweenSpawns = true;

	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	bool AutoTriggerDemoRecording = true;

	// Number of seconds of no user steering before AI roadmap steering takes over
	// Use 0 for never
	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	float SecondsBeforeAISteering = 0;

	// use 1 for go and 2 for stop instead?
	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	bool ButtonBoxControls = false;

	// auto stop demo after n seconds of no TR input
	// use 0 to disable
	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	float SecondsToDemoStop = 0;

	// Active neighborhoods
	UPROPERTY(Category = "fMRI", BlueprintReadWrite, VisibleAnywhere)
	TMap<EDestinationSet, bool> ActiveDestinationSets;

	// Convenience function to check whether any neighborhood is active
	UFUNCTION(Category = "fMRI", BlueprintPure)
	bool IsAnyDestinationSetActive() const;

	// Convenience function to check if a particular neighborhood is active
	UFUNCTION(Category = "fMRI", BlueprintCallable)
	bool IsDestinationSetActive(const EDestinationSet neighborhood) const;

	UPROPERTY(Category = "fMRI", BlueprintReadOnly, VisibleAnywhere)
	int RenderAll = 0;	// jank, 0 = do not, >0 is framerate to render at

	/**
	 * Initializes the TMap to be false for all
	 */
	void InitializeActiveDestinationSets();

	/**
	 * Initializes with the active set specified by the string
	 * @param activeSets
	 */
	void InitializeActiveDestinationSets(const FString &activeSets);

/// =================================================================
/// traffic configs
/// =================================================================
	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float maxSubjectBias = 1.0f;	// max probability that vehicles should be sent in direct towards the subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float repulsionBias = 0.65;		// amount to direct away from subject in inversion

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float inversionRadius = 10000;	// distance from subject at which cars begin to be directed directed _away_ from subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float maxBiasRadius = 40000;	// distance from subject at which max attraction begins

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float minBiasRadius = 5000;		// distance from subject at which max repulsion is reached

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float defaultSpeedLimit = 35.0f;// default speed limit in mph

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float reincarnation = 30.0f;	// Time to wait in seconds until auto-despawn

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float offScreenLiveTime = 30.0f;// Time in seconds that an AI vehicle can exist without being rendered

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float respawnInterval = 0.15f;	// Duration between respawns of vehicles

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float minSpawnDistance = 50.0f;	// Min distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float maxSpawnDistance = 250.0f;// Max distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float maxSpawnAngle= 90.0f;		// Max angle in degrees from view vector to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	float minSpawnTimeDistance = 2.0f;		// Minimum distance in time from player to spawn vehicle

	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	int numberToRespawnInOneGo = 10;// Number of vehicles to respawn in one go

	// Normal traffic is parsed from the above values
	// low traffic
	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	int lowTrafficNumberOfVehicles = 5;			// Min distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float lowTrafficMinSpawnDistance = 75.0f;	// Min distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float lowTrafficMaxSpawnDistance = 500.0f;	// Max distance in meters to spawn vehicle from subject

	// high traffic
	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	int highTrafficNumberOfVehicles = 40;		// Min distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float highTrafficMinSpawnDistance = 30.0f;	// Min distance in meters to spawn vehicle from subject

	UPROPERTY(Category = "Traffic", BlueprintReadOnly, VisibleAnywhere)
	float highTrafficMaxSpawnDistance = 200.0f;	// Max distance in meters to spawn vehicle from subject

	// Limit how fast the player can go?
	UPROPERTY(Category = "Traffic", BlueprintReadWrite, VisibleAnywhere)
	bool bGovernorOnPlayer = false;


private:
	/** Experiment type Settings level. */
	UPROPERTY(Category = "fMRI", VisibleAnywhere, meta =(AllowPrivateAccess="true"))
	EExperimentType ExperimentType = EExperimentType::Navigation;

	/** Destination picking mode*/
	UPROPERTY(Category = "fMRI", VisibleAnywhere, meta =(AllowPrivateAccess="true"))
	EDestinationPickingMode DestinationPickingMode = EDestinationPickingMode::BiasedRandom;

public:
	// Circuit experiment parameters
	/**
	 * Minimum fraction in _total index_ of the next destination relative to the current one
	 */
	UPROPERTY(Category = "Circuit", VisibleAnywhere, BlueprintReadOnly)
	float CircuitMinDestinationFraction = 0.25;

	/**
	 * Maximum fraction in _total index_ of the next destination relative to the current one
	 */
	UPROPERTY(Category = "Circuit", VisibleAnywhere, BlueprintReadOnly)
	float CircuitMaxDestinationFraction = 0.75;

	/**
	 * Is the track one-way, i.e. display "wrong direction" text?
	 */
	UPROPERTY(Category = "Circuit", VisibleAnywhere, BlueprintReadOnly)
	bool bDirectionalCircuit = true;


	// == build information ==
	UPROPERTY(Category = "Version Info", VisibleAnywhere, BlueprintReadOnly)
FString CompilingMachine = FString("crystal");

	UPROPERTY(Category = "Version Info", VisibleAnywhere, BlueprintReadOnly)
FString GitBranch = FString("learning-task");

	// Version is updated by git tag
	UPROPERTY(Category = "Version Info", VisibleAnywhere, BlueprintReadOnly)
	FString Version = TEXT("1.7");

	UPROPERTY(Category = "Version Info", VisibleAnywhere, BlueprintReadOnly)
	FString BuildDate = TEXT("Feb 2025");

/// Display configs
public:
	UPROPERTY(Category = "HUD Display", BlueprintReadWrite, VisibleAnywhere)
	bool ShowFPS = false;

	UPROPERTY(Category = "HUD Display", BlueprintReadWrite, VisibleAnywhere)
	bool ShowNavigationInfo = false;

	UPROPERTY(Category = "HUD Display", BlueprintReadWrite, VisibleAnywhere)
	bool ShowDebugInfo = false;

	UPROPERTY(Category = "HUD Display", BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	EReticuleType ReticuleType = EReticuleType::Multiply;

	UPROPERTY(Category = "HUD Display", BlueprintReadWrite, VisibleAnywhere)
	bool ShowPoints = false;


/// Foraging Configs
	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int MinNumberTargets = 3;	// min number to ask subjects to collect on a trial

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int MaxNumberTargets = 6;	// max number to ask subjects to collect on a trial

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	float AvailableTargetMultiplier = 2.0;	// how many potential targets to make available per target to collect

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	float ValueResetProbability = 0.25;		// probability of target values changing on next trial

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int MinStableTrials = 3;				// minimum number of trials before values could reset

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int LowValuePoints = 10;				// points for low value/copper item

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int MedValuePoints = 20;				// points for med value/silver item

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int HighValuePoints = 40;				// points for high value/gold item

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int BaseTrialTime = 90;					// trial time limit for min targets

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int MaxTrialTime = 180;					// max possible time for trials

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	int AdditionalItemTime = 15;			// additional time to give for each target beyond the min number

	UPROPERTY(Category = "Foraging", BlueprintReadWrite, VisibleAnywhere)
	float CollectionSpeed = 5;				// Max speed in MPH subjects have to be to collect an item

	/// Learning Practice Configs
	UPROPERTY(Category = "LearningPractice", BlueprintReadWrite, VisibleAnywhere)
	int NumberTargets = 55;		// Number of targets to visit in a trial

	UPROPERTY(Category = "LearningPractice", BlueprintReadWrite, VisibleAnywhere)
	bool DestinationPoints = false; // Whether to have different points for destinations (inherited from foraging)

	/// Controls configs
	/// Sensitivity multipliers for steer, throttle, and brake
	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float SteerSensitivity = 1.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float ThrottleSensitivity = 1.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float BrakeSensitivity = 1.0f;

	// logistic response curve depending on speed parameters
	// zero these out to disable logistic response
	// Here, these are in units of MPH
	// The Midpoint values specify the speed at which sensitivity drops to half
	// The Quartile values specify the speed difference from the midpoints at which
	// sensitivity are at 75% / 25%
	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float SteerQuartile = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float ThrottleQuartile = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float BrakeQuartile = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float SteerMidpoint = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float ThrottleMidpoint = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	float BrakeMidpoint = 0.0f;

	UPROPERTY(Category = "Controls", BlueprintReadWrite, VisibleAnywhere)
	bool HelpButtonActive = true;
};
