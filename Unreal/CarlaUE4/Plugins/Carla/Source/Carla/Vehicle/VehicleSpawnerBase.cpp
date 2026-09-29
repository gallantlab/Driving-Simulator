// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "VehicleSpawnerBase.h"
#include "Game/CarlaGameInstance.h"

#include "Util/RandomEngine.h"
#include "Vehicle/CarlaWheeledVehicle.h"
#include "Vehicle/WheeledVehicleAIController.h"

#include "Engine/NetworkObjectList.h"


#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

int MAX_RECURSIONS = 512;	// maximum number of times the random playerstart picker can try

// =============================================================================
// -- Static local methods -----------------------------------------------------
// =============================================================================

static bool VehicleIsValid(const ACarlaWheeledVehicle* Vehicle)
{
	return ((Vehicle != nullptr) && !Vehicle->IsPendingKill());
}

static AWheeledVehicleAIController* GetController(ACarlaWheeledVehicle* Vehicle)
{
	return (VehicleIsValid(Vehicle) ? Cast<AWheeledVehicleAIController>(Vehicle->GetController()) : nullptr);
}

// =============================================================================
// -- AVehicleSpawnerBase ------------------------------------------------------
// =============================================================================

// Sets default values
AVehicleSpawnerBase::AVehicleSpawnerBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVehicleSpawnerBase::BeginPlay()
{
	Super::BeginPlay();

	const UCarlaSettings& settings = Cast<UCarlaGameInstance>(GetGameInstance())->GetCarlaSettings();
	TimeBetweenSpawnAttemptsAfterBegin = settings.respawnInterval;
	MinDistanceToPlayer = settings.minSpawnDistance * 100;
	MaxDistanceToPlayer = settings.maxSpawnDistance * 100;
	MinTimeFromPlayer = settings.minSpawnTimeDistance;
	numberToRespawn = settings.numberToRespawnInOneGo;
	float blah;
	FMath::SinCos(&blah, &MaxAngleToPlayer, settings.maxSpawnAngle * 3.1415926 / 180.0);
	UE_LOG(LogCarla, Log, TEXT("Max angle %0.1f cosine is %0.3f"), settings.maxSpawnAngle, MaxAngleToPlayer);
	UE_LOG(LogCarla, Log, TEXT("Will attempt to spawn every %.2f seconds"), TimeBetweenSpawnAttemptsAfterBegin);

	NumberOfVehicles = FMath::Max(0, NumberOfVehicles);

	isDebug = settings.ShowDebugInfo;

	// Allocate space for walkers.
	Vehicles.Reserve(NumberOfVehicles);

	FindSpawnPoints();

	if (SpawnPoints.Num() < NumberOfVehicles && SpawnPoints.Num() > 0)
	{
//		UE_LOG(LogCarla, Warning, TEXT("We don't have enough spawn points (PlayerStart) for vehicles!"));
		if (SpawnPoints.Num() == 0)
		{
			UE_LOG(LogCarla, Error, TEXT("At least one spawn point (PlayerStart) is needed to spawn vehicles!"));
		}
		else
		{
//			UE_LOG(LogCarla, Log, TEXT("To cover the %d vehicles to spawn after beginplay, it will spawn one new vehicle each %f seconds"), NumberOfVehicles - SpawnPoints.Num(), TimeBetweenSpawnAttemptsAfterBegin);
		}
	}

	if (NumberOfVehicles == 0 || SpawnPoints.Num() == 0)
		bSpawnVehicles = false;

	if (bSpawnVehicles)
	{
		GetRandomEngine()->Shuffle(SpawnPoints); //to get a random spawn point from the map
		const int32 MaximumNumberOfAttempts = SpawnPoints.Num();
		int32 NumberOfAttempts = 0;
		int32 SpawnIndexCount = 0;
		while ((NumberOfVehicles > Vehicles.Num()) && (NumberOfAttempts < MaximumNumberOfAttempts))
		{
			if (SpawnPoints.IsValidIndex(SpawnIndexCount))
			{
				if (SpawnVehicleAtSpawnPoint(*SpawnPoints[SpawnIndexCount]))
				{
					SpawnIndexCount++;
				}
			}
			NumberOfAttempts++;
		}
		bool bAllSpawned = false;
		if (NumberOfVehicles > SpawnIndexCount)
		{
//			UE_LOG(LogCarla, Warning, TEXT("Requested %d vehicles, but we were only able to spawn %d"), NumberOfVehicles, SpawnIndexCount);
		}
		else
		{
			if (SpawnIndexCount == NumberOfVehicles)
			{
				bAllSpawned = true;
			}
		}
		if (!bAllSpawned)
		{
//			UE_LOG(LogCarla, Log, TEXT("Starting the timer to spawn the other %d vehicles, one per %f seconds"), NumberOfVehicles - SpawnIndexCount, TimeBetweenSpawnAttemptsAfterBegin);
			GetWorld()->GetTimerManager().SetTimer(AttemptTimerHandle, this, &AVehicleSpawnerBase::SpawnVehicleAttempt, TimeBetweenSpawnAttemptsAfterBegin, false, -1);
		}
		else
		{
//			UE_LOG(LogCarla, Log, TEXT("Spawned all %d requested vehicles"), NumberOfVehicles);
		}
	}
}

void AVehicleSpawnerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AVehicleSpawnerBase::SetNumberOfVehicles(const int32 Count)
{
	bSpawnVehicles = Count > 0;
	NumberOfVehicles = Count >= 0 ? Count : 0;
	if (Count > 0)
	{
		if (Vehicles.Num() < NumberOfVehicles)
		{
			GetWorld()->GetTimerManager().SetTimer(AttemptTimerHandle, this, &AVehicleSpawnerBase::SpawnVehicleAttempt, TimeBetweenSpawnAttemptsAfterBegin, false, -1);
		}
	}
}

void AVehicleSpawnerBase::TryToSpawnRandomVehicle()
{
	auto SpawnPoint = GetRandomSpawnPoint();
	if (SpawnPoint != nullptr)
	{
		SpawnVehicleAtSpawnPoint(*SpawnPoint);
	}
	else
	{
		UE_LOG(LogCarla, Error, TEXT("Unable to find spawn point"));
	}
}

ACarlaWheeledVehicle* AVehicleSpawnerBase::SpawnVehicleAtSpawnPoint(const APlayerStart& SpawnPoint)
{
	ACarlaWheeledVehicle* Vehicle;
	SpawnVehicle(SpawnPoint.GetActorTransform(), Vehicle);
	AActor* player = Cast<AActor>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if ((Vehicle != nullptr) && !Vehicle->IsPendingKill())
	{
		Vehicle->AIControllerClass = AWheeledVehicleAIController::StaticClass();
		Vehicle->SpawnDefaultController();
		auto Controller = GetController(Vehicle);
		if (Controller != nullptr)
		{ // Sometimes fails...
			Controller->GetRandomEngine()->Seed(GetRandomEngine()->GenerateSeed());
			Controller->SetRoadMap(GetRoadMap());
			Controller->SetAutopilot(true);
//			Controller->MoveToActor(player, -1, false, true, false);	// move towards the player, but only available in AIController
			Vehicles.Add(Vehicle);
			if (GetWorld()->DemoNetDriver)
			{
				// Feb 3, 2020
				// Ok so basically if a controller gets spawned during a demo recording, it is by default a network controller
				// and its commands are not used by the local pawn. When the demo net driver goes away, there is no more network and the controller
				// becomes local and can affect the pawn.
				// SetAsLocalPlayerController() is supposed to be only called by the GameModeBase, but this does what I need for it to do...
				// I think this is fine, because multiplayer games do no expect two players on the same client and so that function
				// needs never be called except by the game mode base. However, in this case, the AI controllers are subclassed from
				// APlayerController, which makes it look as if there are multiple players on the same client.
				Controller->SetAsLocalPlayerController();
			}
		}
		else
		{
			UE_LOG(LogCarla, Error, TEXT("Something went wrong creating the controller for the new vehicle"));
			Vehicle->Destroy();
		}
	}
	UE_LOG(LogCarla, Log, TEXT("Vehicle is%s locally controlled"), Vehicle->IsLocallyControlled() ? TEXT("") : TEXT(" not"))
	UE_LOG(LogCarla, Log, TEXT("Vehicle is%s player controlled"), Vehicle->IsPlayerControlled() ? TEXT("") : TEXT(" not"))
	return Vehicle;
}

void AVehicleSpawnerBase::SpawnVehicleAttempt()
{
	if (Vehicles.Num() >= NumberOfVehicles)
	{
//		UE_LOG(LogCarla, Log, TEXT("All vehicles spawned correctly"));
		return;
	}

	APawn* playerpawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	APlayerStart* spawnpoint = GetRandomSpawnPoint(playerpawn);

	float NextTime = TimeBetweenSpawnAttemptsAfterBegin;
	if (spawnpoint && SpawnVehicleAtSpawnPoint(*spawnpoint) != nullptr)
	{
		UE_LOG(LogCarla, Log, TEXT("Vehicle %d/%d spawned"), Vehicles.Num(), NumberOfVehicles);
	}
	else
	{
		NextTime /= 2.0f;
	}
	if (Vehicles.Num() < NumberOfVehicles)
	{
		auto& timemanager = GetWorld()->GetTimerManager();
		if (AttemptTimerHandle.IsValid())
			timemanager.ClearTimer(AttemptTimerHandle);
		timemanager.SetTimer(AttemptTimerHandle, this, &AVehicleSpawnerBase::SpawnVehicleAttempt, NextTime, false, -1);
	}
	else
	{
//		UE_LOG(LogCarla, Log, TEXT("All vehicles spawned correctly"));
	}

}

void AVehicleSpawnerBase::SpawnVehicleAttemptMany()
{
	for (int i = 0; i < numberToRespawn; i++)
		SpawnVehicleAttempt();
}

APlayerStart* AVehicleSpawnerBase::GetRandomSpawnPoint()
{
	if (SpawnPoints.Num() < 1)
		return nullptr;

	int index = GetRandomEngine()->GetUniformIntInRange(0, SpawnPoints.Num() - 1);

	return SpawnPoints[index];
}

// Random spawn point proximal to the pawn
APlayerStart* AVehicleSpawnerBase::GetRandomSpawnPoint(APawn* pawn)
{
	if (!pawn)
		return GetRandomSpawnPoint();

	if (SpawnPoints.Num() < 1)
		return nullptr;

	int i;
	int index;
	float dotProduct, distance, time;
	FVector viewVector = pawn->GetRootComponent()->GetComponentRotation().Vector();
	viewVector.Normalize();
	for (i = 0; i < MAX_RECURSIONS; i++)
	{
		index = GetRandomEngine()->GetUniformIntInRange(0, SpawnPoints.Num() - 1);

		FVector playerToPawn = SpawnPoints[index]->GetActorLocation() - pawn->GetActorLocation();

		// so apparently the player vehicle isn't in Vehicles. Which makes sense, since that holds the "other cars"
		// for the AI vehicles, and therefore we need to specifically also check against the player vehicle
		distance = playerToPawn.Size();
		if ((MaxDistanceToPlayer > 0) && (distance > MaxDistanceToPlayer))
			continue;
		time = playerToPawn.Size() / pawn->GetVelocity().Size();	// time it takes to cover this distance at the pawn's current velocity

		playerToPawn.Normalize();
		dotProduct = FVector::DotProduct(viewVector, playerToPawn);

		if (((distance < MinDistanceToPlayer) || (time < MinTimeFromPlayer)) && (dotProduct > 0))
			continue;
		
		if (dotProduct < MaxAngleToPlayer)    // want only spawn points in front of subject
			continue;

		// the rest of the position validation is equally applies to all vehicles so
		if (!ValidateSpawnPointAgainstAllVehicles(index))
			continue;

//		UE_LOG(LogCarla, Log, TEXT("Spawn point got on iteration %d"), i + 1);
		return SpawnPoints[index];
	}
	return nullptr;
}

void AVehicleSpawnerBase::RemoveVehicle(ACarlaWheeledVehicle* vehicleToRemove)
{
//	UE_LOG(LogFMRI, Log, TEXT("Removing vehicle %d from vehicles"), vehicleToRemove->GetUniqueID());
//	UE_LOG(LogFMRI, Log, TEXT("There are currently %d vehicles"), Vehicles.Num());
	if (Vehicles.Remove(vehicleToRemove) != 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Error removing a vehicle"));
	}
	else
	{
//		UE_LOG(LogFMRI, Log, TEXT("Vehicle removed and destroyed"));
		UE_LOG(LogFMRI, Log, TEXT("There are currently %d vehicles"), Vehicles.Num());
		vehicleToRemove->Destroy();
		if (AttemptTimerHandle.IsValid())	// clear timer if needed be
			GetWorld()->GetTimerManager().ClearTimer(AttemptTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(AttemptTimerHandle, this, &AVehicleSpawnerBase::SpawnVehicleAttemptMany, TimeBetweenSpawnAttemptsAfterBegin, false, -1);
//		UE_LOG(LogFMRI, Log, TEXT("Timer set to spawn more vehicles"));
	}
}

void AVehicleSpawnerBase::SetSpawnLimits(float minDistanceToPlayer, float maxDistanceToPlayer, float angle)
{
	MinDistanceToPlayer = minDistanceToPlayer;
	MaxDistanceToPlayer = maxDistanceToPlayer;
	float blah;
	FMath::SinCos(&blah, &MaxAngleToPlayer, angle * 3.1415926 / 180.0);
}

bool AVehicleSpawnerBase::ValidateSpawnPointAgainstAllVehicles(int spawnIndex)
{
	float directionDotProduct, distance, time;
	FVector spawnLocation = SpawnPoints[spawnIndex]->GetActorLocation();
	FVector spawnDirection = SpawnPoints[spawnIndex]->GetRootComponent()->GetComponentRotation().Vector();
	for (int i = 0; i < Vehicles.Num(); i++)
	{
		directionDotProduct = FVector::DotProduct(spawnDirection, Vehicles[i]->GetRootComponent()->GetComponentRotation().Vector());
		distance = FVector::Distance(Vehicles[i]->GetActorLocation(), spawnLocation);
		time = distance / Vehicles[i]->GetVelocity().Size();
		if (((distance < MinDistanceToPlayer) || (time < MinTimeFromPlayer)) && (directionDotProduct > 0))
			return false;
	}
	return true;
}

void AVehicleSpawnerBase::FindSpawnPoints()
{
	// clear existing pointers, which are likely stale
	SpawnPoints.Empty();
	HumanStartZones.Empty();

	// Find (if any) human start zones
	for (TActorIterator <AHumanStartZone> It(GetWorld()); It; ++It)
	{
		HumanStartZones.Add(*It);
	}
	UE_LOG(LogCarla, Log, TEXT("Found %d human spawn zones"), HumanStartZones.Num());

	// Find spawn points present in level.
	bool isHumanSpawnPoint;
	int count = 0;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		count++;
		isHumanSpawnPoint = false;
		for (int i = 0; i < HumanStartZones.Num(); i++)
		{
			if (HumanStartZones[i]->IsOverlappingNonCollidingActor(*It))
			{
				isHumanSpawnPoint = true;
				break;
			}
		}
		if (!isHumanSpawnPoint) SpawnPoints.Add(*It);
	}

	UE_LOG(LogCarla, Log, TEXT("Found %d PlayerStart positions, of which %d are for spawning AI vehicles"), count, SpawnPoints.Num());
}