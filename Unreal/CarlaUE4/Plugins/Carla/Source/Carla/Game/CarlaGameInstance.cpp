// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"

#include "Runtime/NetworkReplayStreaming/NullNetworkReplayStreaming/Public/NullNetworkReplayStreaming.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "NetworkVersion.h"

#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

#include "CarlaGameInstance.h"

#include "Game/MockGameController.h"
#include "Server/ServerGameController.h"
#include "Vehicle/CarlaSpectatorController.h"

#include "Util/easywsclient.h"
#include "Util/NavigationParser.h"

#include "EngineUtils.h"
#include "MoviePlayer.h"

UCarlaGameInstance::UCarlaGameInstance()
{
	CarlaSettings = CreateDefaultSubobject<UCarlaSettings>(TEXT("CarlaSettings"));
	check(CarlaSettings != nullptr);
	CarlaSettings->LoadSettings();
	CarlaSettings->LogSettings();
	playerPawn = nullptr;
	playerPawnName = nullptr;
	UE_LOG(LogCarla, Log, TEXT("Game instance spawned"));
	demoStartTime = FDateTime::Now();
	navigationSequence = TArray<int>();
	if (CarlaSettings->GetDestinationPickingMode() == EDestinationPickingMode::Predetermined)
	{
		if (!NavigationParser::ParseNavigationSequence(CarlaSettings->NavigationSequenceFile, this->navigationSequence))
		{
			UE_LOG(LogFMRI, Error, TEXT("Error in parsing the navigation sequence file, reverting to random destinations"));
			CarlaSettings->SetDestinationPickingMode(EDestinationPickingMode::BiasedRandom);
		}
	}
	allDemos = TArray<FString>();

	if (CarlaSettings->ButtonBoxControls)
	{
		UInputSettings *inputSettings = const_cast<UInputSettings*>(GetDefault<UInputSettings>());
		if (!inputSettings)
		{
			UE_LOG(LogFMRI, Error, TEXT("Error getting input settings to remap to button box inputs"));
		}
		else
		{
			inputSettings->RemoveActionMapping(FInputActionKeyMapping(FName("ToggleReverse"), FKey("One")));
			inputSettings->AddAxisMapping(FInputAxisKeyMapping(FName("MoveForward"), FKey("One")));
			inputSettings->AddAxisMapping(FInputAxisKeyMapping(FName("Brake"), FKey("Two")));
			inputSettings->SaveKeyMappings();
			UE_LOG(LogFMRI, Log, TEXT("Game instance remapped to button box car controls"));
		}
	}
	bIsAutoRender = CarlaSettings->RenderAll > 0;
}

UCarlaGameInstance::~UCarlaGameInstance()
{}

void UCarlaGameInstance::InitializeGameControllerIfNotPresent(const FMockGameControllerSettings& MockControllerSettings)
{
	if (GameController == nullptr)
	{
		if (CarlaSettings->bUseNetworking)
			GameController = MakeUnique<FServerGameController>(DataRouter);
		else
		{
			GameController = MakeUnique<MockGameController>(DataRouter, MockControllerSettings);
			UE_LOG(LogCarla, Log, TEXT("Using mock CARLA controller"));
		}
	}
}

APawn* UCarlaGameInstance::GetPlayerPawn()
{
	// get the actual *player* pawn, not the spectator pawn in a replay
	APawn* out = GetPlayerPawnByName();
	out = out ? out : GetPlayerPawnByTag();
	return out;
}

APawn* UCarlaGameInstance::GetPlayerPawnByTag()
{
	// iterate through all the actors, and find the one that has the player tag
	APawn* out = nullptr;
	int nPlayerTags = 0;
	for (TActorIterator <APawn> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr->ActorHasTag(PlayerTag))
		{
			out = *ActorItr;
			playerPawnName = new FName(ActorItr->GetFName());
			nPlayerTags++;
		}
		if (ActorItr->ActorHasTag(NPCTag))
		{
//			UE_LOG(LogCarla, Log, TEXT("NPC tag found"));
		}
	}
	return out;
}

APawn* UCarlaGameInstance::GetPlayerPawnByName()
{
	if (!playerPawnName)
	{
		return nullptr;
	}
	FName* thisName = nullptr;
	for (TActorIterator <APawn> vehicle(GetWorld()); vehicle; ++vehicle)
	{
		thisName = new FName(vehicle->GetFName());
		if (*thisName == *playerPawnName)
		{
			return *vehicle;
		}
	}
	return nullptr;
}

void UCarlaGameInstance::PlayReplay(const FString& InName, UWorld* WorldOverride = nullptr, const TArray<FString>& AdditionalOptions = TArray<FString>())
{
	UE_LOG(LogCarla, Log, TEXT("Starting demofile replay"));
	isReplay = true;

	Super::PlayReplay(InName, WorldOverride, AdditionalOptions);
	UE_LOG(LogCarla, Log, TEXT("Superclass PlayReplay call has returned"));
//	HandlePreLoadMapDelegate = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UCarlaGameInstance::HandlePreLoadMap);
	playerPawn = nullptr;        // reset player reference
	demoFileName = new FString(InName);

	if (GetPlayerPawn() != nullptr)
		UE_LOG(LogCarla, Log, TEXT("Found a pawn tagged as player"));

	bIsDemosListStale = true;
}

void UCarlaGameInstance::SetCaptureFolder(const FString& folder)
{
	captureFolder = new FString(folder);
	UE_LOG(LogCarla, Log, TEXT("Capture folder set to %s"), captureFolder);
}

void UCarlaGameInstance::DemoRenderFrames(const FString& demoName, int fps)
{
	// chop off the "(R)" on an already rendered demo
	const FString actualDemoName = demoName.EndsWith(FString(" (R)")) ? demoName.LeftChop(4) : demoName;
	PlayReplay(actualDemoName);
	// spectator controller doesn't exist here yet, so we leave a flag for the controller to eventually find
	spectatorToCaptureFrames = true;
	captureFolder = new FString(actualDemoName);
	captureFPS = fps;

	// immediately pause the replay
	AWorldSettings *worldSettings = GetPlayerPawn()->GetWorldSettings();
	worldSettings->Pauser = GetPlayerPawn()->PlayerState;
}

void UCarlaGameInstance::Init()
{
	Super::Init();
	// create a ReplayStreamer for FindReplays() and DeleteReplay(..)
	EnumerateStreamsPtr = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();

	// Link FindReplays() delegate to function
	OnEnumerateStreamsCompleteDelegate = FOnEnumerateStreamsComplete::CreateUObject(this, &UCarlaGameInstance::OnEnumerateStreamsComplete);

	// Link DeleteReplay() delegate to function
	OnDeleteFinishedStreamCompleteDelegate = FOnDeleteFinishedStreamComplete::CreateUObject(this, &UCarlaGameInstance::OnDeleteFinishedStreamComplete);

	// loading screen stuff from https://wiki.unrealengine.com/Loading_Screen

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UCarlaGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCarlaGameInstance::EndLoadingScreen);

}

void UCarlaGameInstance::StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions)
{
	if (!InName.IsEmpty())	// name provided
		Super::StartRecordingReplay(InName, FriendlyName, AdditionalOptions);
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("No demo name given, using %s"), *(FDateTime::Now().ToString().Replace(TEXT("."), TEXT("-"))));
		Super::StartRecordingReplay(FDateTime::Now().ToString().Replace(TEXT("."), TEXT("-")), FriendlyName, AdditionalOptions);
	}
	UE_LOG(LogFMRI, Log, TEXT("Demo %s recording started %s"), *InName, *FDateTime::Now().ToString());
	isRecording = true;
	isReplay = false;
	demoStatus = 1;
	demoStartTime = FDateTime::Now();
	bIsDemosListStale = true;	// demos list will be need to be rebuilt because a new one has been made
	int OBSRequestResult = RequestOBSRecording(true);
	UE_LOG(LogFMRI, Log, TEXT("Websocket request to start recording %d bytes sent"), OBSRequestResult);
	// SetViewerOverride supposedly changes the viewpoint from which the demo is recorded?
//	GetWorld()->DemoNetDriver->SetViewerOverride(Cast<APlayerController>(GetPlayerPawn()->Controller));
}

void UCarlaGameInstance::StopRecordingReplay()
{
	Super::StopRecordingReplay();
	isRecording = false;
	demoStatus = 2;
	int OBSRequestResults = RequestOBSRecording(false);
	bIsDemosListStale = true;	// demos list will be need to be rebuilt because a new one has been made
	UE_LOG(LogFMRI, Log, TEXT("Websocket request to stop recording %d bytes sent"), OBSRequestResults);
	UE_LOG(LogFMRI, Log, TEXT("Demostop called"));
}

// blueprint-to-code calls
void UCarlaGameInstance::StartRecordingReplayFromBP(FString ReplayName, FString FriendlyName)
{
	StartRecordingReplay(ReplayName, FriendlyName);
	// StartRecordingReplay probably has to tag the player pawn
}

void UCarlaGameInstance::StopRecordingReplayFromBP()
{
	StopRecordingReplay();
}

void UCarlaGameInstance::PlayReplayFromBP(FString ReplayName)
{
	PlayReplay(ReplayName);
}

void UCarlaGameInstance::FindReplays()
{	
	if (bIsDemosListStale && EnumerateStreamsPtr.Get())
	{
		EnumerateStreamsPtr.Get()->EnumerateStreams(FNetworkReplayVersion(), FString(), FString(), OnEnumerateStreamsCompleteDelegate);
	}
}

void UCarlaGameInstance::OnEnumerateStreamsComplete(const TArray <FNetworkReplayStreamInfo>& StreamInfos)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// logic for auto rendering
	bool hasUnrenderedReplays = false;
	replayToRender = FString();

	// clear demos because duplicates
	allDemos.Empty();
	FString savedFileName;	// name of file produced by demo rendering
	// iterate through streams found by EnumerateStreams
	for (FNetworkReplayStreamInfo StreamInfo : StreamInfos)
		if (!StreamInfo.bIsLive)
		{
			UE_LOG(LogFMRI, Log, TEXT("Replay %s"), *(StreamInfo.Name));
			savedFileName = PlatformFile.ConvertToAbsolutePathForExternalAppForWrite(*FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("%s-positions.xml"), *(StreamInfo.Name))));
			if (FPaths::FileExists(savedFileName))
			{
				allDemos.AddUnique(StreamInfo.Name.Append(FString(" (R)")));
			}
			else
			{
				allDemos.AddUnique(StreamInfo.Name);
				hasUnrenderedReplays = true;
				replayToRender = StreamInfo.Name;
			}
		}

	allDemos.Sort([](const FString& lhs, const FString& rhs) {return lhs.Compare(rhs) < 0;});

	UE_LOG(LogFMRI, Log, TEXT("%d replays found"), allDemos.Num());
	bIsDemosListStale = false;
}

void UCarlaGameInstance::RenameReplay(const FString& ReplayName, const FString& NewFriendlyReplayName)
{
	// Get File Info
	FNullReplayInfo Info;

	const FString DemoPath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/"));
	const FString StreamDirectory = FPaths::Combine(*DemoPath, *ReplayName);
	const FString StreamFullBaseFilename = FPaths::Combine(*StreamDirectory, *ReplayName);
	const FString InfoFilename = StreamFullBaseFilename + TEXT(".replayinfo");

	TUniquePtr <FArchive> InfoFileArchive(IFileManager::Get().CreateFileReader(*InfoFilename));

	if (InfoFileArchive.IsValid() && InfoFileArchive->TotalSize() != 0)
	{
		FString JsonString;
		*InfoFileArchive << JsonString;

		Info.FromJson(JsonString);
		Info.bIsValid = true;

		InfoFileArchive->Close();
	}

	// Set FriendlyName
	Info.FriendlyName = NewFriendlyReplayName;

	// Write File Info
	TUniquePtr <FArchive> ReplayInfoFileAr(IFileManager::Get().CreateFileWriter(*InfoFilename));

	if (ReplayInfoFileAr.IsValid())
	{
		FString JsonString = Info.ToJson();
		*ReplayInfoFileAr << JsonString;

		ReplayInfoFileAr->Close();
	}
}

void UCarlaGameInstance::DeleteReplay(const FString& ReplayName)
{
	if (EnumerateStreamsPtr.Get())
	{
		EnumerateStreamsPtr.Get()->DeleteFinishedStream(ReplayName, OnDeleteFinishedStreamCompleteDelegate);
	}
}

void UCarlaGameInstance::OnDeleteFinishedStreamComplete(const bool bDeleteSucceeded)
{
	FindReplays();
}

int UCarlaGameInstance::RequestOBSRecording(bool startStop)
{
	using easywsclient::WebSocket;
	WebSocket::pointer client = WebSocket::from_url("ws://localhost:4444");
	if (client == NULL)
	{
		UE_LOG(LogFMRI, Error, TEXT("No websocket connection to OBS"));
		return -1;
	}
	if (startStop)
		client->send("{\"request-type\": \"StartRecording\", \"message-id\": 0}");
	else
		client->send("{\"request-type\": \"StopRecording\", \"message-id\": 0}");
	client->poll();
	client->close();
	delete client;
	return 0;
}

int UCarlaGameInstance::GetCurrentNavigationDestination() const
{
	int index = indexInNavigationSequence % navigationSequence.Num();
	if (index != indexInNavigationSequence)
	{
		UE_LOG(LogFMRI, Log, TEXT("Current navigation destination sequence index has wrapped around %d times"), indexInNavigationSequence / navigationSequence.Num());
	}
	return navigationSequence[index];
}

void UCarlaGameInstance::LoadSubjectStateFile()
{
	FString fileName = FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("%s-save.txt"), *CarlaSettings->Subject));
	if (!FPaths::FileExists(fileName))
	{
		UE_LOG(LogFMRI, Error, TEXT("Save file for subject %s does not exists at %s, starting from scratch"), *CarlaSettings->Subject, *fileName);
		return;
	}

	UE_LOG(LogFMRI, Log, TEXT("Loading file %s"), *fileName);
	TArray<FString> lines = TArray<FString>();
	FFileHelper::LoadFileToStringArray(lines, *fileName);
	if (lines.Num() != 7)
	{
		UE_LOG(LogFMRI, Error, TEXT("Incorrect number of lines in the file: %d instead of 7"), lines.Num());
		return;
	}
	if (!lines[1].Equals(CarlaSettings->NavigationSequenceFile, ESearchCase::Type::IgnoreCase))
	{
		UE_LOG(LogFMRI, Error, TEXT("Nav sequence file in saved file is different than the one in settings\n\tSaved: %s\n\tSettings: %s"), *lines[1], *CarlaSettings->NavigationSequenceFile);
		return;
	}
	bool savedRandomState = FCString::Atoi(*lines[3]);
	if (savedRandomState != (CarlaSettings->GetDestinationPickingMode() == EDestinationPickingMode::Predetermined))
	{
		UE_LOG(LogFMRI, Log, TEXT("Random destinations bool value is different between saved file and this session"));
	}
	indexInNavigationSequence = FCString::Atoi(*lines[2]);
	UE_LOG(LogFMRI, Log, TEXT("Index in nav sequence: %d"), indexInNavigationSequence);


	TArray<FString> tokens = TArray<FString>();
	lines[4].ParseIntoArray(tokens, TEXT(","));
	FVector location = FVector(FCString::Atof(*tokens[0]), FCString::Atof(*tokens[1]), FCString::Atof(*tokens[2]));
	int indexToUse = GetIndexOfClosestPlayerStart(location);
	if (CarlaSettings->RememberLocationBetweenSpawns)
	{
		UE_LOG(LogFMRI, Log, TEXT("Using start index %d and overriding loaded settings"), indexToUse);
		GameController->SetPlayerStartIndex(indexToUse);
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Using random start index"));
	}
	UE_LOG(LogFMRI, Log, TEXT("Subject save state loaded"));
}

void UCarlaGameInstance::SetStartIndex(int indexToUse)
{
	GameController->SetPlayerStartIndex(indexToUse);
}

void UCarlaGameInstance::SaveSubjectStateFile()
{
	TArray<FString> lines = TArray<FString>();													// save:
	lines.Add(CarlaSettings->Subject);															// subject
	lines.Add(CarlaSettings->NavigationSequenceFile);											// nav sequence file in config file
	lines.Add(FString::FromInt(indexInNavigationSequence));										// current index in the nav sequence
	lines.Add(CarlaSettings->GetDestinationPickingMode() == EDestinationPickingMode::Predetermined ? FString::FromInt(0) : FString::FromInt(1));	// whether we are using a random sequence

	FVector playerPosition;
	FRotator playerRotation;
	GetPlayerPawn()->GetActorEyesViewPoint(playerPosition, playerRotation);
	lines.Add(FString::Printf(TEXT("%f,%f,%f"), playerPosition.X, playerPosition.Y, playerPosition.Z));				// player location
	lines.Add(FString::Printf(TEXT("%f,%f,%f"), playerRotation.Roll, playerRotation.Pitch, playerRotation.Yaw));	// player rotation

	UE_LOG(LogFMRI, Log, TEXT("Finding closest spawn point"));

	lines.Add(FString::FromInt(GetIndexOfClosestPlayerStart(playerPosition)));		// spawn index to use

	FString fileName = FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("%s-save.txt"), *CarlaSettings->Subject));
	UE_LOG(LogFMRI, Log, TEXT("Saving file to %s"), *fileName);
	FFileHelper::SaveStringArrayToFile(lines, *fileName);
}

int UCarlaGameInstance::GetIndexOfClosestPlayerStart(const FVector& location)
{
	// iterate through and find closest
	int indexToUse = 0;
	int index = 0;
	double closestDistance = 100 * 10000.0;
	double thisDistance = 0;
	for (TActorIterator <APlayerStart> playerStartIterator(GetWorld()); playerStartIterator; ++playerStartIterator)
	{
		APlayerStart* PlayerStart = *playerStartIterator;
		thisDistance = (PlayerStart->GetActorLocation() - location).Size();
		if (thisDistance < closestDistance)
		{
			closestDistance = thisDistance;
			indexToUse = index;
		}
		index++;
	}

	return indexToUse;
}

// respawn does not happen immediately
void UCarlaGameInstance::AddOneVehicleToRespawnQueue(ACarlaWheeledVehicle *vehicleToReincarnate)
{
	nVehiclesToSpawn++;
	if (!vehicleSpawner)
		FindVehicleSpawner();
	if (!vehicleSpawner)
	{
		UE_LOG(LogFMRI, Error, TEXT("Cannot find the vehicle spawner to destroy a vehicle"));
		return;
	}
	else
	{
		vehicleSpawner->RemoveVehicle(vehicleToReincarnate);
	}
}

void UCarlaGameInstance::FindVehicleSpawner()
{
	TActorIterator<AVehicleSpawnerBase> spawnerIterator = TActorIterator<AVehicleSpawnerBase>(GetWorld());
	vehicleSpawner = *spawnerIterator;
}

void UCarlaGameInstance::FindPedestrianSpawner()
{
	TActorIterator<AWalkerSpawnerBase> spawnerIterator = TActorIterator<AWalkerSpawnerBase>(GetWorld());
	pedestrianSpawner = *spawnerIterator;
}


void UCarlaGameInstance::ClearVehicleSpawnerPointer()
{
	vehicleSpawner = nullptr;
}

void UCarlaGameInstance::ClearPedestrianSpawnerPointer()
{
	pedestrianSpawner = nullptr;
}

void UCarlaGameInstance::Shutdown()
{
	// this means it's an actual session and the state should be saved on exit
	if (!isReplay)
		SaveSubjectStateFile();
}


void UCarlaGameInstance::BeginLoadingScreen(const FString& InMapName)
{
	if (!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
		LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

void UCarlaGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{

}

int UCarlaGameInstance::SetTrafficDensity(ETrafficDensity newDensity)
{
	currentTrafficDensity = newDensity;
	int nCars;
	float minSpawn, maxSpawn;
	switch (currentTrafficDensity)
	{
		case ETrafficDensity::Low:
			nCars = CarlaSettings->lowTrafficNumberOfVehicles;
			minSpawn = CarlaSettings->lowTrafficMinSpawnDistance;
			maxSpawn = CarlaSettings->lowTrafficMaxSpawnDistance;
			UE_LOG(LogCarla, Log, TEXT("Traffic density set to low"));
			break;
		case ETrafficDensity::High:
			nCars = CarlaSettings->highTrafficNumberOfVehicles;
			minSpawn = CarlaSettings->highTrafficMinSpawnDistance;
			maxSpawn = CarlaSettings->highTrafficMaxSpawnDistance;
			UE_LOG(LogCarla, Log, TEXT("Traffic density set to high"));
			break;
		default:
			nCars = CarlaSettings->NumberOfVehicles;
			minSpawn = CarlaSettings->minSpawnDistance;
			maxSpawn = CarlaSettings->maxSpawnDistance;
			UE_LOG(LogCarla, Log, TEXT("Traffic density set to normal"));
			break;
	}
	SetTraffic(nCars, minSpawn, maxSpawn);
	return nCars;
}

void UCarlaGameInstance::SetTraffic(int numberOfCars, float minSpawnDistance, float maxSpawnDistance, float maxSpawnAngle, int numberToSpawn)
{
	if (!vehicleSpawner) FindVehicleSpawner();

	if (numberOfCars < 0) numberOfCars = CarlaSettings->NumberOfVehicles;
	if (minSpawnDistance < 0) minSpawnDistance = CarlaSettings->minSpawnDistance;
	if (maxSpawnDistance < 0) maxSpawnDistance = CarlaSettings->maxSpawnDistance;
	if (maxSpawnAngle < 1) maxSpawnAngle = CarlaSettings->maxSpawnAngle;
	if (!numberToSpawn) numberToSpawn = CarlaSettings->numberToRespawnInOneGo;

	vehicleSpawner->SetNumberOfVehicles(numberOfCars);
	vehicleSpawner->SetSpawnLimits(minSpawnDistance * 100, maxSpawnDistance * 100, maxSpawnAngle);
	vehicleSpawner->SetNumberToRespawn(numberToSpawn);
	UE_LOG(LogCarla, Log, TEXT("Traffic params set to %d cars, spawn distance [%.2f, %.2f] meters, max spawn angle %.0f, spawning %d each time"),
								numberOfCars, minSpawnDistance, maxSpawnDistance, maxSpawnAngle, numberToSpawn);
}

void UCarlaGameInstance::ReloadSettings()
{
	SetTraffic();
	if (!pedestrianSpawner)
		FindPedestrianSpawner();
	pedestrianSpawner->SetNumberOfWalkers(CarlaSettings->NumberOfPedestrians);
}

const TArray<FString>& UCarlaGameInstance::GetDemosList()
{
	if (bIsDemosListStale)
	{
		FindReplays();
	}
	return allDemos;
}