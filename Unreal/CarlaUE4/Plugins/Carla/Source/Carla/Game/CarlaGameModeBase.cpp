// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "CarlaGameModeBase.h"

#include "Game/CarlaGameInstance.h"
#include "Game/CarlaHUD.h"
#include "Game/CarlaDriveHUD.h"

// player state includes
#include "Game/CarlaPlayerState.h"
#include "Game/CircuitPlayerState.h"
#include "Game/NavigationPlayerState.h"
#include "Game/TrackRunningPlayerState.h"
#include "Game/LearningTestPlayerState.h"
#include "Game/ForagingPlayerState.h"
#include "Game/LearningPracticePlayerState.h"
#include "Game/LearningPracticeListPlayerState.h"

#include "Game/Tagger.h"
#include "Game/TaggerDelegate.h"
#include "Sensor/Sensor.h"
#include "Sensor/SensorFactory.h"
#include "Settings/CarlaSettings.h"
#include "Settings/CarlaSettingsDelegate.h"
#include "Util/RandomEngine.h"

// Player controller includes
#include "Vehicle/CarlaVehicleController.h"
#include "Vehicle/CarlaSpectatorController.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Vehicle/TrackRunningController.h"
#include "Vehicle/CircuitPlayerController.h"
#include "Vehicle/TrackedLearningController.h"
#include "Vehicle/LearningTestController.h"
#include "Vehicle/ForagingController.h"
#include "Vehicle/LearningPracticeController.h"
#include "Vehicle/LearningPracticeListController.h"

#include "ConstructorHelpers.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "SceneViewport.h"
#include "../Settings/CarlaSettings.h"


ACarlaGameModeBase::ACarlaGameModeBase(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer),
		GameController(nullptr),
		PlayerController(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bAllowTickBeforeBeginPlay = false;

	PlayerControllerClass = ACarlaVehicleController::StaticClass();
	PlayerStateClass = ACarlaPlayerState::StaticClass();
	HUDClass = ACarlaDriveHUD::StaticClass();

	TaggerDelegate = CreateDefaultSubobject<UTaggerDelegate>(TEXT("TaggerDelegate"));
	CarlaSettingsDelegate = CreateDefaultSubobject<UCarlaSettingsDelegate>(TEXT("CarlaSettingsDelegate"));
}

void ACarlaGameModeBase::InitGame(
		const FString& MapName,
		const FString& Options,
		FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	GameInstance = Cast<UCarlaGameInstance>(GetGameInstance());
	checkf(
			GameInstance != nullptr,
			TEXT("GameInstance is not a UCarlaGameInstance, did you forget to set it in the project settings?"));

	GameInstance->InitializeGameControllerIfNotPresent(MockGameControllerSettings);
	GameInstance->LoadSubjectStateFile();	// this needs the game controller to exist
	GameController = &GameInstance->GetGameController();
	auto& CarlaSettings = GameInstance->GetCarlaSettings();
	UWorld* world = GetWorld();
	{ // Load weather descriptions and initialize game controller.
#if WITH_EDITOR
    {
      // Hack to be able to test level-specific weather descriptions in editor.
      // When playing in editor the map name gets an extra prefix, here we
      // remove it.
      FString CorrectedMapName = MapName;
      constexpr auto PIEPrefix = TEXT("UEDPIE_0_");
      CorrectedMapName.RemoveFromStart(PIEPrefix);
      UE_LOG(LogCarla, Log, TEXT("Corrected map name from %s to %s"), *MapName, *CorrectedMapName);
      CarlaSettings.MapName = CorrectedMapName;
      CarlaSettings.LoadWeatherDescriptions();
    }
#else
    CarlaSettings.MapName = MapName;
    CarlaSettings.LoadWeatherDescriptions();
#endif // WITH_EDITOR
		GameController->Initialize(CarlaSettings);
		CarlaSettings.ValidateWeatherId();
		CarlaSettings.LogSettings();
	}

	// Set default pawn class.
	if (!CarlaSettings.PlayerVehicle.IsEmpty())
	{
		auto Class = FindObject<UClass>(ANY_PACKAGE, *CarlaSettings.PlayerVehicle);
		if (Class)
		{
			DefaultPawnClass = Class;
		}
		else
		{
			UE_LOG(LogCarla, Error, TEXT("Failed to load player pawn class \"%s\""), *CarlaSettings.PlayerVehicle)
		}
	}

	if (TaggerDelegate != nullptr)
	{
		TaggerDelegate->RegisterSpawnHandler(world);
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Missing TaggerDelegate!"));
	}

	if (CarlaSettingsDelegate != nullptr)
	{
		//apply quality settings
		CarlaSettingsDelegate->ApplyQualitySettingsLevelPostRestart();
		//assign settings delegate for every new actor from now on
		CarlaSettingsDelegate->RegisterSpawnHandler(world);

	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Missing CarlaSettingsDelegate!"));
	}


	for (TActorIterator<ADynamicWeather> WeatherIterator(world); WeatherIterator; ++WeatherIterator)
	{
		// there should only be one object whose name is "weather"
		if (WeatherIterator->GetName().StartsWith(FString("Weather")))
		{
			DynamicWeather = *WeatherIterator;
			break;
		}
	}

	// if dynamic weather is not found, spawn one.
	if (DynamicWeatherClass != nullptr && DynamicWeather == nullptr)
	{
		DynamicWeather = world->SpawnActor<ADynamicWeather>(DynamicWeatherClass);
	}

	if (VehicleSpawnerClass != nullptr)
	{
		VehicleSpawner = world->SpawnActor<AVehicleSpawnerBase>(VehicleSpawnerClass);
	}

	if (WalkerSpawnerClass != nullptr)
	{
		WalkerSpawner = world->SpawnActor<AWalkerSpawnerBase>(WalkerSpawnerClass);
	}

	// Find (if any) human start zones
	for (TActorIterator <AHumanStartZone> It(GetWorld()); It; ++It)
	{
		humanStartZones.Add(*It);
	}
	UE_LOG(LogCarla, Log, TEXT("CarlaGameModeBase found %d human start zones"), humanStartZones.Num());

	// Set controller and state classes according to experiment type
	experimentType = CarlaSettings.GetExperimentType();
	switch (experimentType)
	{
		case EExperimentType::Learning:
			UE_LOG(LogFMRI, Log, TEXT("Learning experiment"));
			// fallthrough
		case EExperimentType::Navigation:
			UE_LOG(LogFMRI, Log, TEXT("Navigation experiment"))
			PlayerControllerClass = ANavigationVehicleController::StaticClass();
			PlayerStateClass = ANavigationPlayerState::StaticClass();
			break;
		case EExperimentType::TrackRunning:
			UE_LOG(LogFMRI, Log, TEXT("Track running experiment"))
			PlayerControllerClass = ATrackRunningController::StaticClass();
			PlayerStateClass = ATrackRunningPlayerState::StaticClass();
			break;
		case EExperimentType::Circuit:
			UE_LOG(LogFMRI, Log, TEXT("Closed loop track running experiment"))
			PlayerControllerClass = ACircuitPlayerController::StaticClass();
			PlayerStateClass = ACircuitPlayerState::StaticClass();
			break;
		case EExperimentType::TrackedLearning:
			UE_LOG(LogFMRI, Log, TEXT("Learning experiment with record of where we went"))
			PlayerControllerClass = ATrackedLearningController::StaticClass();
			PlayerStateClass = ANavigationPlayerState::StaticClass();
			break;
		case EExperimentType::LearningTest:
			UE_LOG(LogFMRI, Log, TEXT("Learning experiment test session"))
			PlayerControllerClass = ALearningTestController::StaticClass();
			PlayerStateClass = ALearningTestPlayerState::StaticClass();
			break;
		case EExperimentType::Foraging:
			UE_LOG(LogFMRI, Log, TEXT("Foraging task"))
		// fall through since we use the same controller
		case EExperimentType::TimedForaging:
			UE_LOG(LogFMRI, Log, TEXT(" is timed"))
			PlayerControllerClass = AForagingController::StaticClass();
			PlayerStateClass = AForagingPlayerState::StaticClass();
			break;
		case EExperimentType::LearningPractice:
			UE_LOG(LogFMRI, Log, TEXT("Learning practice task"))
			PlayerControllerClass = ALearningPracticeController::StaticClass();
			PlayerStateClass = ALearningPracticePlayerState::StaticClass();
			break;
		case EExperimentType::LearningPracticeList:
			UE_LOG(LogFMRI, Log, TEXT("Learning practice list task"))
			PlayerControllerClass = ALearningPracticeListController::StaticClass();
			PlayerStateClass = ALearningPracticeListPlayerState::StaticClass();
			break;
		default:
			UE_LOG(LogFMRI, Log, TEXT("NNeither"))
			PlayerControllerClass = ACarlaVehicleController::StaticClass();
			PlayerStateClass = ACarlaPlayerState::StaticClass();
			break;
	}

}

void ACarlaGameModeBase::RestartPlayer(AController* NewPlayer)
{
	check(NewPlayer != nullptr);
	TArray < APlayerStart * > UnOccupiedStartPoints;
	APlayerStart* PlayFromHere = FindUnOccupiedStartPoints(NewPlayer, UnOccupiedStartPoints);
	if (PlayFromHere != nullptr)
	{
		RestartPlayerAtPlayerStart(NewPlayer, PlayFromHere);
		RegisterPlayer(*NewPlayer);
		return;
	}
	else if (UnOccupiedStartPoints.Num() > 0u)
	{
		check(GameController != nullptr);
		APlayerStart* StartSpot = GameController->ChoosePlayerStart(UnOccupiedStartPoints);
		if (StartSpot != nullptr)
		{
			RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
			RegisterPlayer(*NewPlayer);
			return;
		}
	}
	UE_LOG(LogCarla, Error, TEXT("No start spot found!"));
}

void ACarlaGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	const auto& CarlaSettings = GameInstance->GetCarlaSettings();

	// Setup semantic segmentation if necessary.
	if (CarlaSettings.bSemanticSegmentationEnabled)
	{
		TagActorsForSemanticSegmentation();
		TaggerDelegate->SetSemanticSegmentationEnabled();
	}

	// Change weather.
	if (DynamicWeather != nullptr)
	{
		const auto* Weather = CarlaSettings.GetActiveWeatherDescription();
		if (Weather != nullptr)
		{
			UE_LOG(LogCarla, Log, TEXT("Changing weather settings to \"%s\""), *Weather->Name);
			DynamicWeather->SetWeatherDescription(*Weather);
			DynamicWeather->RefreshWeather();
		}
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Missing dynamic weather actor!"));
	}

	// Find road map.
	TActorIterator <ACityMapGenerator> It(GetWorld());
	URoadMap* RoadMap = (It ? It->GetRoadMap() : nullptr);

	if (PlayerController != nullptr)
	{
		PlayerController->SetRoadMap(RoadMap);
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Player controller is not a AWheeledVehicleAIController!"));
	}

	// Setup other vehicles.
	if (VehicleSpawner != nullptr)
	{
		VehicleSpawner->SetNumberOfVehicles(CarlaSettings.NumberOfVehicles);
		VehicleSpawner->SetSeed(CarlaSettings.SeedVehicles);
		VehicleSpawner->SetRoadMap(RoadMap);
		if (PlayerController != nullptr)
		{
			PlayerController->GetRandomEngine()->Seed(
					VehicleSpawner->GetRandomEngine()->GenerateSeed());
		}
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Missing vehicle spawner actor!"));
	}

	// Setup walkers.
	if (WalkerSpawner != nullptr)
	{
		WalkerSpawner->SetNumberOfWalkers(CarlaSettings.NumberOfPedestrians);
		WalkerSpawner->SetSeed(CarlaSettings.SeedPedestrians);
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Missing walker spawner actor!"));
	}

	UE_LOG(LogCarla, Log, TEXT("CarlaGameModeBase Began Play"));
	GameController->BeginPlay();
}

void ACarlaGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (CarlaSettingsDelegate != nullptr && EndPlayReason != EEndPlayReason::EndPlayInEditor)
	{
		CarlaSettingsDelegate->Reset();
	}
}

void ACarlaGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GameController->Tick(DeltaSeconds);
}

void ACarlaGameModeBase::RegisterPlayer(AController& NewPlayer)
{
	check(GameController != nullptr);
	AddTickPrerequisiteActor(&NewPlayer);
	PlayerController = Cast<ACarlaVehicleController>(&NewPlayer);
	if (PlayerController != nullptr)
	{
		GetDataRouter().RegisterPlayer(*PlayerController);
		GameController->RegisterPlayer(*PlayerController);
		AttachSensorsToPlayer();
		ParseDestinations();
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("ACarlaGameModeBase: Player is not a ACarlaVehicleController"));
	}
}

void ACarlaGameModeBase::AttachSensorsToPlayer()
{
	check(PlayerController != nullptr);
	const auto& Settings = GameInstance->GetCarlaSettings();
	const auto* Weather = Settings.GetActiveWeatherDescription();

	// attach sensors only on replay configs
	if (Settings.SensorsOnlyOnReplay && !GameInstance->IsPlayingReplay())
		return;

	for (auto& Item : Settings.SensorDescriptions)
	{
		check(Item.Value != nullptr);
		auto& SensorDescription = *Item.Value;
		if (Weather != nullptr)
		{
			SensorDescription.AdjustToWeather(*Weather);
		}
		auto* Sensor = FSensorFactory::Make(SensorDescription, *GetWorld());
		check(Sensor != nullptr);
		Sensor->AttachToActor(PlayerController->GetVehiclePawn());
		GetDataRouter().RegisterSensor(*Sensor);
	}
}

void ACarlaGameModeBase::ParseDestinations()
{
	const auto& settings = GameInstance->GetCarlaSettings();
	if (settings.GetExperimentType() == EExperimentType::Navigation || settings.GetExperimentType() == EExperimentType::TrackRunning)
	{
		ANavigationVehicleController* controller = Cast<ANavigationVehicleController>(PlayerController);
	}
}

void ACarlaGameModeBase::CaptureSensors()
{
	if (PlayerController->IsA(ACarlaSpectatorController::StaticClass()))
		((ACarlaSpectatorController*)PlayerController)->CaptureSensors();
	else
		PlayerController->ClientMessage("Not a spectator controller");
}

void ACarlaGameModeBase::RenderDemoFrames(const FString& demoName, int fps = 30)
{
	GameInstance->DemoRenderFrames(demoName, fps);
}

void ACarlaGameModeBase::SetCaptureFolder(const FString& folder)
{
	GameInstance->SetCaptureFolder(folder);
}


void ACarlaGameModeBase::TagActorsForSemanticSegmentation()
{
	check(GetWorld() != nullptr);
	ATagger::TagActorsInLevel(*GetWorld(), true);
}

APlayerStart* ACarlaGameModeBase::FindUnOccupiedStartPoints(AController* Player, TArray<APlayerStart*>& UnOccupiedStartPoints)
{
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	bool isHumanStart = true;
	for (TActorIterator <APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			FoundPlayerStart = PlayerStart;
			break;
		}
		else
		{
			FVector ActorLocation = PlayerStart->GetActorLocation();
			const FRotator ActorRotation = PlayerStart->GetActorRotation();
			if (!GetWorld()->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{

				if (humanStartZones.Num() > 0 && bUseHumanStartZones)
				{
					isHumanStart = false;
					for (int i = 0; i < humanStartZones.Num(); i++)
					{
						if (humanStartZones[i]->IsOverlappingNonCollidingActor(PlayerStart))
						{
							isHumanStart = true;

							UE_LOG(LogCarla, Log, TEXT("Overlap in human zone found"));
						}
					}
				}
				if (isHumanStart) UnOccupiedStartPoints.Add(PlayerStart);
			}
#if WITH_EDITOR
			else if (GetWorld()->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation)) {
			  UE_LOG(
				  LogCarla,
				  Warning,
				  TEXT("Player start cannot be used, occupied location: %s"),
				  *PlayerStart->GetActorLocation().ToString());
			}
#endif // WITH_EDITOR
		}
	}
	return FoundPlayerStart;
}

void ACarlaGameModeBase::SetControllerEyetrackingEnded()
{
	UE_LOG(LogFMRI, Log, TEXT("Trying to tell controller that eyetracking has ended"))
	int width = 0, height = 0;
	PlayerController->GetViewportSize(width, height);
	UE_LOG(LogFMRI, Log, TEXT("Calibration resolution was done with width %d heigh %d"), width, height);
	ANavigationVehicleController* controller = Cast<ANavigationVehicleController>(PlayerController);
	if (controller)
	{
		controller->SetEyetrackingEnded();
		UE_LOG(LogFMRI, Log, TEXT("Successful"))
	}
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("Controller case to navigation vehicle controller failed"))
	}
}

void ACarlaGameModeBase::SaveSubjectState()
{
	GameInstance->SaveSubjectStateFile();
}

void ACarlaGameModeBase::LoadSubjectState()
{
	GameInstance->LoadSubjectStateFile();
}

void ACarlaGameModeBase::SetIndexInNavigationSequence(int newIndex)
{
	GameInstance->SetIndexInNavigationSequence(newIndex);
}

void ACarlaGameModeBase::GenerateNextDestination()
{
	UE_LOG(LogFMRI, Log, TEXT("Trying to tell controller that it has arrived. For debugging"));
	ANavigationVehicleController* controller = Cast<ANavigationVehicleController>(PlayerController);
	if (controller)
	{
		controller->OnSegmentEnd(0);
		UE_LOG(LogFMRI, Log, TEXT("Successful"))
	}
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("Controller case to navigation vehicle controller failed"))
	}
}

void ACarlaGameModeBase::VehicleSpawnerFindSpawnPoints()
{
	if (VehicleSpawner)
		VehicleSpawner->FindSpawnPoints();
}