// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CarlaSpectatorController.h"
#include "Sensor/Sensor.h"
#include "Game/CarlaPlayerState.h"
#include "Game/Tagger.h"
#include "Game/CarlaGameInstance.h"
#include "Game/DataRouter.h"
#include "Game/NavigationPlayerState.h"
#include "Game/TrackRunningPlayerState.h"
#include "Game/LearningTestPlayerState.h"
#include "Game/LearningPracticePlayerState.h"
#include "Game/LearningPracticeListPlayerState.h"
#include "Vehicle/CarlaWheeledVehicle.h"
#include "Agent/AgentComponent.h"
#include "Util/ExperimentState.h"
#include "Util/NavigationDestination.h"
#include "Util/NavigationLoggerComponent.h"
#include "Util/TrackRunningLoggerComponent.h"
#include "Util/LearningTestLoggerComponent.h"
#include "Util/LearningPracticeLoggerComponent.h"
#include "Util/LearningPracticeListLoggerComponent.h"
#include "Settings/CarlaSettings.h"
#include "Util/CircuitLoggerComponent.h"
#include "Game/ForagingPlayerState.h"
#include "Util/ForagingLoggerComponent.h"

#include <iostream>
#include <fstream>


ACarlaSpectatorController::ACarlaSpectatorController(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	Super::SetAutopilot(false);
	Super::EnableUserInput(false);

	// disable mouse freeview
	bEnableClickEvents = 0;
	bEnableMouseOverEvents = 0;
	// ticking when paused
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;

	UE_LOG(LogCarla, Log, TEXT("Spectator controller spawned"));

	sensors = new TArray<ASceneCaptureToDiskCamera*>();

	captureFrames = false;

	frameNumber = 0;

	playerAgentID = -1;

	destinations = objectInitializer.CreateDefaultSubobject<UDestinationParserComponent>(this, TEXT("DestinationParser"));
	
	taggerDelegate = CreateDefaultSubobject<UTaggerDelegate>(TEXT("SpectatorTaggerDelegate"));
};

ACarlaSpectatorController::~ACarlaSpectatorController() {};

// hard disable shit
void ACarlaSpectatorController::EnableUserInput(bool enable)
{
	Super::EnableUserInput(false);
}

void ACarlaSpectatorController::SetAutopilot(bool enable, bool ClearPlannedLocations)
{
	Super::SetAutopilot(false, true);
}

void ACarlaSpectatorController::PostNetInit()
{
	Super::PostNetInit();
//	UE_LOG(LogCarla, Log, TEXT("Post net init called"));
}

void ACarlaSpectatorController::PostActorCreated()
{
	Super::PostActorCreated();
	UE_LOG(LogCarla, Log, TEXT("Post actor created called"));

	UCarlaGameInstance* gameInstance = Cast<UCarlaGameInstance>(GetGameInstance());

	if (gameInstance->IsPlayingReplay())
	{
		TrySetViewTargetToPlayer();

		// check if the replay needs saving frames
		if (gameInstance->CaptureFrames())
		{
			SetFrameCaptureFolder(*(gameInstance->GetCaptureFolder()));
			fps = gameInstance->GetCaptureFPS();
			currentTime = -1.0;
			timeStep = 1.0 / fps;
			delay = 5.0;
			worldSettings = GetWorldSettings();

			FixTimeStep(true);

			if (!worldSettings)
			{
				UE_LOG(LogCarla, Log, TEXT("World settings is null pointer"));
				return;
			}
			if (!PlayerState)
			{
				UE_LOG(LogCarla, Log, TEXT("No player state. Using the reference pawn's."));
				if (!referencePawn->PlayerState)
				{
					UE_LOG(LogCarla, Log, TEXT("Reference pawn doesn't have one either. TTLs will not be read out"));
				}
				else
					PlayerState = referencePawn->PlayerState;

			}
			if (!worldSettings->Pauser)
				worldSettings->Pauser = PlayerState;
			demoNetDriver = GetWorld()->DemoNetDriver;

			ATagger::TagActorsInLevel(*GetWorld(), true);    // apply Carla semantic tags to existing objects
			// the tagger delegate then tags any entities that are spawned afterwards.
			taggerDelegate->RegisterSpawnHandler(GetWorld());
			taggerDelegate->SetSemanticSegmentationEnabled(true);
			captureFrames = true;
		}
	}
}

void ACarlaSpectatorController::TickAutopilotController(float deltaTime) {};

void ACarlaSpectatorController::TagPawn() {};

void ACarlaSpectatorController::SetReferencePawn(APawn* ref)
{
	referencePawn = ref;
//	SetViewTarget(referencePawn);
}

void ACarlaSpectatorController::Possess(APawn * inPawn)
{
	Super::Possess(inPawn);
	UE_LOG(LogCarla, Log, TEXT("Pawn %s possessed"), *inPawn->GetName());
}

void ACarlaSpectatorController::BeginPlayingState()
{
	// this function doesn't actually get called.
	Super::BeginPlayingState();
	UE_LOG(LogCarla, Log, TEXT("Begin playing state"));
	TrySetViewTargetToPlayer();
}

APawn* ACarlaSpectatorController::GetVehiclePawn()
{
	if (!referencePawn)
		referencePawn = FindTaggedPlayerPawn();
	return referencePawn;
}


APawn* ACarlaSpectatorController::FindTaggedPlayerPawn()
{
	UE_LOG(LogCarla, Log, TEXT("Spectator controller trying to find a tagged player pawn"));
	// iterate through all the actors, and find the one that has the player tag
	APawn *pawn = nullptr;
	int nPlayerTags = 0;
	for (TActorIterator<AWheeledVehicle> vehicle(GetWorld()); vehicle; ++vehicle)
	{
#ifdef VERBOSE_LOG
		UE_LOG(LogCarla, Log, TEXT("Pawn %s"), *vehicle->GetName());
		UE_LOG(LogCarla, Log, TEXT("Pawn has %d tags"), vehicle->Tags.Num());
		if (vehicle->Tags.Num() > 0)
			for (int i = 0; i < vehicle->Tags.Num(); i++)
				UE_LOG(LogCarla, Log, TEXT("Tag %s"), *vehicle->Tags[i].ToString());
#endif
		if (vehicle->ActorHasTag(PlayerTag))
		{
#ifdef VERBOSE_LOG
			UE_LOG(LogCarla, Log, TEXT("Player tag found, actor is %s"), *vehicle->GetName());
#endif
			pawn = *vehicle;
			break;
		}
		if (vehicle->ActorHasTag(NPCTag))
		{
//			UE_LOG(LogCarla, Log, TEXT("NPC tag found, actor is %s"), *vehicle->GetName());
		}
	}
#ifdef VERBOSE_LOG
	UE_LOG(LogCarla, Log, TEXT("%d player tags found"), nPlayerTags);
#endif
	return pawn;
}

void ACarlaSpectatorController::TrySetViewTargetToPlayer()
{
	UE_LOG(LogCarla, Log, TEXT("Trying to set view target"));
	if (GetWorld() != nullptr)	// extra layer of check, because GetGameInstance is just short for GetWorld()->GetGameInstance()
	{
		UGameInstance* gamesInstance = GetGameInstance();
		if (gamesInstance)
		{
			carlaGameInstance = Cast<UCarlaGameInstance>(gamesInstance);
			APawn* playerPawn = carlaGameInstance->GetPlayerPawn();
			if (playerPawn != nullptr)
			{
				UE_LOG(LogCarla, Log, TEXT("Player pawn is %s"), *playerPawn->GetName());
				this->AttachToActor(playerPawn, FAttachmentTransformRules::SnapToTargetIncludingScale);
				this->referencePawn = playerPawn;
				this->referencePlayerState = Cast<ANavigationPlayerState>(referencePawn->PlayerState);
				if (!referencePlayerState)
					UE_LOG(LogFMRI, Log, TEXT("Reference pawn player state is a null pointer"));
				if (this->AttachCamerasTo(playerPawn))
					UE_LOG(LogCarla, Log, TEXT("Cameras reattached"));
				UE_LOG(LogCarla, Log, TEXT("View target set to %s"), *playerPawn->GetName());
				needReference = false;

				if (carlaGameInstance->IsPlayingReplay())	// sensors need to be manually made and attached in the replay
				{
					// empty the sensors list
					if (sensors->Num() > 0)
						sensors->Empty();

					const auto& Settings = carlaGameInstance->GetCarlaSettings();
					const auto* Weather = Settings.GetActiveWeatherDescription();

					for (auto& Item : Settings.SensorDescriptions)
					{
						check(Item.Value != nullptr);
						auto& SensorDescription = *Item.Value;
						if (Weather != nullptr)
							SensorDescription.AdjustToWeather(*Weather);
						ASensor* sensor = FSensorFactory::Make(SensorDescription, *GetWorld());
						check(sensor != nullptr);
						sensor->AttachToActor(this->GetVehiclePawn());
						carlaGameInstance->GetDataRouter().RegisterSensor(*sensor);
						sensors->Add((ASceneCaptureToDiskCamera*)sensor);
					}

					ACarlaWheeledVehicle *playerVehiclePawn = Cast<ACarlaWheeledVehicle>(playerPawn);

					// check data router for agents
					FDataRouter& dataRouter = carlaGameInstance->GetDataRouter();
					TArray<const UAgentComponent *> agents = dataRouter.GetAgents();
					UE_LOG(LogCarla, Log, TEXT("Data router on replay got %d agents"), agents.Num());
					for (int i = 0; i < agents.Num(); i++)
					{
						UE_LOG(LogCarla, Log, TEXT("Agent %s, ID %d"), *(agents[i]->GetName()), agents[i]->GetId());
						if (agents[i]->GetId() == playerVehiclePawn->GetAgentComponentID())
						{
							UE_LOG(LogCarla, Log, TEXT("Agent is player"));
							playerAgentID = agents[i]->GetId();
						}
					}

					// init the frame-by-frame logging of actor locations
					frameStates = new TArray<ExperimentState*>();
				}

				TArray<AActor*>* attached = new TArray<AActor*>();
				playerPawn->GetAttachedActors(*attached);
				for (int i = 0; i < attached->Num(); i++)
					UE_LOG(LogCarla, Log, TEXT("Found attached actor %s"), *(*attached)[i]->GetName());

				CaptureSensors();
			}
			else
				UE_LOG(LogCarla, Log, TEXT("No Player pawn"));
		}
		else
			UE_LOG(LogCarla, Log, TEXT("Has world but not game instance"));
	}
	else
		needReference = true;
}

void ACarlaSpectatorController::CaptureSensors()
{
#ifdef VERBOSE_LOG
	UE_LOG(LogCarla, Log, TEXT("Capture %d sensors"), sensors->Num());
#endif
	if (sensors == nullptr)
		ClientMessage("Null sensors list");
	else
		for (int i = 0; i < sensors->Num(); i++)
		{
#ifdef VERBOSE_LOG
			UE_LOG(LogCarla, Log, TEXT("Sensor %s"), *(*sensors)[i]->GetName());
#endif
#ifndef NO_CAPTURE_FRAMES
			(*sensors)[i]->SaveNextFrame();
#endif
		}
}


void ACarlaSpectatorController::AddExperimentState(ExperimentState *state)
{
	frameStates->Add(state);
}


void ACarlaSpectatorController::Tick(float deltaTime)
{
	if (captureFrames)
	{
		if (delay > 0)									// before recording, pause for a couple of seconds to allow everything to load fully
		{
			if (!worldSettings->Pauser)
				worldSettings->Pauser = PlayerState;
			delay -= deltaTime;

			// player state on the reference pawn may appear *after* the first couple of ticks.
			if (referencePawn->PlayerState)
			{
				UE_LOG(LogCarla, Log, TEXT("Player state found during initial ticks"));
				referencePlayerState = Cast<ACarlaPlayerState>(referencePawn->PlayerState);
			}
		}
		else
		{
			if (worldSettings->Pauser)					// unpause the recording
			{
				worldSettings->Pauser = nullptr;
				UE_LOG(LogCarla, Log, TEXT("Begin rendering frames"));
				frameNumber = 0;
			}

			if (referencePlayerState)
			{
				UE_LOG(LogCarla, Log, TEXT("Time %f, dt %f%s"), demoNetDriver->DemoCurrentTime, deltaTime, referencePlayerState->isTTL() ? TEXT(", TTL") : TEXT(""));
				UE_LOG(LogCarla, Log, TEXT("Speed %f, throttle %f, steering angle %f, brake %f, handbrake %s, gear %d"), referencePlayerState->GetForwardSpeed(),
																													 	 referencePlayerState->GetThrottle(),
																												 		 referencePlayerState->GetSteer(),
																														 referencePlayerState->GetBrake(),
																														 referencePlayerState->GetHandBrake() ? TEXT("yes") : TEXT("no"),
																														 referencePlayerState->GetCurrentGear());
			}
			else
			{
				UE_LOG(LogCarla, Log, TEXT("Time %f, dt %f%s"), demoNetDriver->DemoCurrentTime, deltaTime, TEXT("No player state available"));
			}

			// force tag all vehicles/pedestrians in world because the tagger delegate
			// isn't registering the AI vehicle spawns
			for (TActorIterator<ACarlaWheeledVehicle> iterator(GetWorld()); iterator; ++iterator)
			{
				if (!ATagger::IsActorTaggedWithTag(**iterator, ECityObjectLabel::Vehicles))
					ATagger::TagActor(**iterator, true);
			}
			for (TActorIterator<ACharacter> iterator(GetWorld()); iterator; ++iterator)
			{
				if (!ATagger::IsActorTaggedWithTag(**iterator, ECityObjectLabel::Pedestrians))
					ATagger::TagActor(**iterator, true);
			}
			
			CaptureSensors();
			if (referencePlayerState)
				LogExperimentState();
			if (currentTime == demoNetDriver->DemoCurrentTime)	// stop rendering frames at the end
			{
				captureFrames = false;
				FixTimeStep(false);
				if(referencePlayerState)
					SaveExperimentLog();
				UE_LOG(LogCarla, Log, TEXT("End replay and end rendering frames"));
				carlaGameInstance->isReplay = false;
				stopDemo = true;
			}
			else
				currentTime = demoNetDriver->DemoCurrentTime;

			frameNumber++;
		}
	}
	else if (stopDemo)	// wait one tick since end of demo
	{
		demoNetDriver->StopDemo();
		GetWorld()->DestroyDemoNetDriver();
//		UnPossess();
		RestartLevel();
	}

	Super::Tick(deltaTime);
}

void ACarlaSpectatorController::SaveExperimentLog()
{
//  write out agent locations to xml file

#define FORMAT_TO_CHAR_ARRAY(string, ...) TCHAR_TO_ANSI(*FString::Printf(TEXT(string), ##__VA_ARGS__))
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString fileName = PlatformFile.ConvertToAbsolutePathForExternalAppForWrite(*FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("%s-positions.xml"), **(carlaGameInstance->GetDemoName()))));

	ofstream logFile;
	logFile.open(TCHAR_TO_ANSI(*fileName));

	int indentation = 0;


#define WRITE_INDENTS(out, indents) for (int indent = 0; indent < indents; indent++) out << '\t';

#define WRITE_ATTRIBUTE(attribute) '"' << attribute << '"'

	logFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	logFile << "<Driving-Experiment>" << endl;
	indentation++;

	WRITE_INDENTS(logFile, indentation)
	logFile << "<Experiment-Type>";
	switch (experimentType)
	{
		case EExperimentType::Foraging:
			logFile << "Foraging";
			break;
		case EExperimentType::Navigation:
			logFile << "Navigation";
			break;
		case EExperimentType::TrackRunning:
			logFile << "Track Running";
			break;
		case EExperimentType::Circuit:
			logFile << "Circuit Running";
			break;
		case EExperimentType::LearningTest:
			logFile << "Learning Test";
			break;
		case EExperimentType::LearningPractice:
			logFile << "Learning Practice";
			break;
		case EExperimentType::LearningPracticeList:
			logFile << "Learning Practice List";
			break;
		default:
			logFile << "Other";
			break;
	}
	logFile << "</Experiment-Type>" << endl;

	WRITE_INDENTS(logFile, indentation)
	logFile << "<Player-ID>" << playerAgentID << "</Player-ID>" << endl;

	WRITE_INDENTS(logFile, indentation)
	logFile << "<Capture-FPS>" << carlaGameInstance->GetCaptureFPS() << "</Capture-FPS>" << endl;

	WRITE_INDENTS(logFile, indentation)
	logFile << "<Seconds-To-Run-Start>" << referencePlayerState->GetSecondsToStartOfRun() << "</Seconds-To-Run-Start>" << endl;

//	save destinations if is navigation experiment

	WRITE_INDENTS(logFile, indentation)
	logFile << "<Destinations>" << endl;
	indentation++;

	NavigationDestination* destination;
	for (int i = 0; i < destinations->Num(); i++)
	{
		destination = &(destinations->At(i));
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Destination Name=" << WRITE_ATTRIBUTE(TCHAR_TO_ANSI(*(destination->GetName().Replace(TEXT("&"), TEXT("+"))))) << " ID=" << WRITE_ATTRIBUTE(i) << ">" << destination->GetDestination().X << ',' << destination->GetDestination().Y << "</Destination>" << endl;
	}
	indentation--;
	WRITE_INDENTS(logFile, indentation)
	logFile << "</Destinations>" << endl;

//	record static entities
	WRITE_INDENTS(logFile, indentation)
	logFile << "<Static-Entities>" << endl;
	indentation++;

	EntityState* entityState;
	for (int entity = 0; entity < (*frameStates)[0]->Num(); entity++)
	{
		entityState = &((*((*frameStates)[0]))[entity]);
		if (entityState->type != EntityType::RoadSign)
			continue;
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Entity Type=\"Road Sign\" ID=" << WRITE_ATTRIBUTE(entityState->ID) << ">" << endl;
		indentation++;
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Position>" << entityState->position.X << ',' << entityState->position.Y << ',' << entityState->position.Z << "</Position>" << endl;
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Rotation>" << entityState->rotation.Pitch << ',' << entityState->rotation.Roll << ',' << entityState->rotation.Yaw << "</Rotation>" << endl;
		indentation--;
		WRITE_INDENTS(logFile, indentation)
		logFile << "</Entity>" << endl;
	}
	indentation--;
	WRITE_INDENTS(logFile, indentation)
	logFile << "</Static-Entities>" << endl;

	// to clear the confusion, frameStates is an array of ExperimentStates
	// each ExperimentState is an array of EntityStates
	// save each frame's state
	UE_LOG(LogFMRI, Log, TEXT("%d frames to save, be patient"), frameStates->Num());
	int percent = frameStates->Num() / 100;
	percent = percent ? percent : 1;	// case where there's less than 100 frames, the percent will be 0 and modulo 0 is floating point error
	ExperimentState* frameState = nullptr;

	for (int frame = 0; frame < frameStates->Num(); frame++)
	{
		(*frameStates)[frame]->WriteToFile(logFile, indentation);

		if (frame % percent == 0)
			UE_LOG(LogFMRI, Log, TEXT("%d/%d frames written"), frame + 1, frameStates->Num() + 1);
	}
	indentation--;
	logFile << "</Driving-Experiment>" << endl;

	logFile.close();
	UE_LOG(LogFMRI, Log, TEXT("File saved to %s"), *fileName);


	// for reasons I can't understand, I cannot programmatically play multiple replays, even though
	// the human clicking calls the exact same code. In the second replay, the spectator controller
	// does not get spawned. So instead of having the game play all the replays, it quits every time
	// and the launcher re-starts the game to render the next replay. At some point this needs to be
	// debugged in Unreal, but for the time being, I really would prefer auto-replay-all to work, no
	// matter how jank it is. -tz 4/10/2021
	if (GetCarlaGameInstance()->IsAutoRender())
		FGenericPlatformMisc::RequestExit(false);
}


void ACarlaSpectatorController::SetFrameCaptureFolder(const FString& folder)
{
	if (sensors == nullptr)
	{
		UE_LOG(LogCarla, Error, TEXT("Tried to set frame capture folder but the list of sensors is null"));
		return;
	}
	for (int i = 0; i < sensors->Num(); i++)
		(*sensors)[i]->SetSaveFolder(folder);
}

void ACarlaSpectatorController::FixTimeStep(bool fix)
{
	FApp::SetFixedDeltaTime(fix ? timeStep : 0);
	FApp::SetBenchmarking(fix);
	FApp::SetUseFixedTimeStep(fix);
}


void ACarlaSpectatorController::SpawnLoggerComponent()
{
	// use recorded player state to determine
	// which type to spawn because it seems like the GameInstance keeps thinking it's a nav experiment
	// The more base the class is, i.e. NavigationPlayerState, the further down it should be!
	if (Cast<ALearningPracticeListPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Learning Practice List logger component spawned"));
		experimentType = EExperimentType::LearningPracticeList;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, ULearningPracticeListLoggerComponent::StaticClass());
	}
	else if (Cast<ALearningPracticePlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Learning Practice logger component spawned"));
		experimentType = EExperimentType::LearningPractice;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, ULearningPracticeLoggerComponent::StaticClass());
	}
	else if (Cast<ALearningTestPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Learning Test logger component spawned"));
		experimentType = EExperimentType::LearningTest;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, ULearningTestLoggerComponent::StaticClass());
	}
	else if (Cast<AForagingPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Foraging logger component spawned"));
		experimentType = EExperimentType::Foraging;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, UForagingLoggerComponent::StaticClass());
	}
	else if (Cast<ATrackRunningPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Track running logger component spawned"));
		experimentType = EExperimentType::TrackRunning;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, UTrackRunningLoggerComponent::StaticClass());

	}
	else if (Cast<ACircuitPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Circuit logger component spawned"));
		experimentType = EExperimentType::Circuit;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, UCircuitLoggerComponent::StaticClass());
	}
	else if (Cast<ALearningTestPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Learning test logger component spawned"));
		experimentType = EExperimentType::LearningTest;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, ULearningTestLoggerComponent::StaticClass());
	}
	else if (Cast<ANavigationPlayerState>(referencePlayerState) != nullptr)
	{
		UE_LOG(LogFMRI, Log, TEXT("Navigation logger component spawned"));
		experimentType = EExperimentType::Navigation;
		loggerComponent = NewObject<UExperimentLoggerComponent>(this, UNavigationLoggerComponent::StaticClass());
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Experiment logger component spawned"));
		loggerComponent = NewObject<UExperimentLoggerComponent>(this);
	}
	loggerComponent->SetParentController(this);
}


void ACarlaSpectatorController::LogExperimentState()
{
	// lazy spawn logger component
	if (!loggerComponent) SpawnLoggerComponent();
	loggerComponent->LogFrame();
}

void ACarlaSpectatorController::SetSteeringInput(float value) {}

void ACarlaSpectatorController::SetThrottleInput(float value) {}

void ACarlaSpectatorController::SetBrakeInput(float value) {}

void ACarlaSpectatorController::HoldHandbrake() {}

void ACarlaSpectatorController::ReleaseHandbrake() {}