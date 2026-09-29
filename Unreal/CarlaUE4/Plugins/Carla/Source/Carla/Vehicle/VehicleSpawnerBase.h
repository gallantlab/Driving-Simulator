// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Util/ActorWithRandomEngine.h"
#include "Util/HumanStartZone.h"

#include "VehicleSpawnerBase.generated.h"

class ACarlaWheeledVehicle;
class APlayerStart;

UCLASS(Abstract)
class CARLA_API AVehicleSpawnerBase : public AActorWithRandomEngine
{
  GENERATED_BODY()

public:

  // Sets default values for this actor's properties
  AVehicleSpawnerBase(const FObjectInitializer& ObjectInitializer);

protected:

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  // Called when the actor is removed from the level
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  UFUNCTION(BlueprintImplementableEvent)
  void SpawnVehicle(const FTransform &SpawnTransform, ACarlaWheeledVehicle *&SpawnedCharacter);

  //UFUNCTION(BlueprintImplementableEvent)
  void TryToSpawnRandomVehicle();

public:

  void SetNumberOfVehicles(int32 Count);

  int32 GetNumberOfSpawnedVehicles() const
  {
    return Vehicles.Num();
  }

  const TArray<ACarlaWheeledVehicle *> &GetVehicles() const {
    return Vehicles;
  }

  void SetRoadMap(URoadMap *InRoadMap)
  {
    RoadMap = InRoadMap;
  }

  UFUNCTION(Category = "Road Map", BlueprintCallable)
  URoadMap *GetRoadMap()
  {
    return RoadMap;
  }
  /** Function called to spawn another vehicle when there is not enough spawn points in the beginplay */
  UFUNCTION(Category = "Vehicle Spawner", BlueprintCallable)
  void SpawnVehicleAttempt();

  void SetSpawnLimits(float minDistanceToPlayer = 5000, float maxDistanceToPlayer = 25000, float angle = 120.0);

  void SetNumberToRespawn(int number) {numberToRespawn = number;};

  /**
   * Fullscreen change causes the spawn points to go stale, so this got factored out to be called after initialization
   */
   UFUNCTION(Category = "Vehicle Spawner", BlueprintCallable)
  void FindSpawnPoints();

void SpawnVehicleAttemptMany();

  // respawns vehicles to be within a certain radius of two locations
//  void RespawnVehicles(FVector &pointA, FVector &pointB, double radiusA, double radiusB);

  // Removes a vehicle
  void RemoveVehicle(ACarlaWheeledVehicle* vehicleToRemove);

protected:

  APlayerStart* GetRandomSpawnPoint();

  /**
   * Gets a randome spawn point constrained around the pawn
   * @param pawn	generally the player vehicle pawn
   */
  APlayerStart* GetRandomSpawnPoint(APawn* pawn);

  ACarlaWheeledVehicle* SpawnVehicleAtSpawnPoint(const APlayerStart &SpawnPoint);

  UPROPERTY()
  URoadMap *RoadMap = nullptr;

  /** If false, no vehicles will be spawned. */
  UPROPERTY(Category = "Vehicle Spawner", EditAnywhere)
  bool bSpawnVehicles = true;

  // are we displaying debug info on the cars?
  bool isDebug;

  /** Number of walkers to be present within the volume. */
  UPROPERTY(Category = "Vehicle Spawner", EditAnywhere, meta = (EditCondition = bSpawnVehicles, ClampMin = "1"))
  int32 NumberOfVehicles = 10;

  UPROPERTY(Category = "Vehicle Spawner", VisibleAnywhere, AdvancedDisplay)
  TArray<APlayerStart *> SpawnPoints;

  // where do humans spawn, if this is more than zero, don't spawn AI cars there
	UPROPERTY(Category = "Vehicle Spawner", VisibleAnywhere, AdvancedDisplay)
	TArray<AHumanStartZone *> HumanStartZones;

  // for selectively respawning things only in certain regions
//  TArray<bool> spawnPointInUse;

  UPROPERTY(Category = "Vehicle Spawner", BlueprintReadOnly, VisibleAnywhere, AdvancedDisplay)
  TArray<ACarlaWheeledVehicle *> Vehicles;

  /** Time to spawn new vehicles after begin play if there was not enough spawn points at the moment */
  UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.1", ClampMax = "1000.0", UIMin = "0.1", UIMax = "1000.0"))
  float TimeBetweenSpawnAttemptsAfterBegin = 0.2f;

  /** Min Distance to the player vehicle to validate a spawn point location for the next vehicle spawn attempt */
  UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "10", ClampMax = "10000", UIMin = "10", UIMax = "10000"))
  float MinDistanceToPlayer = 5000;

	/** Max Distance to the player vehicle to validate a spawn point location for the next vehicle spawn attempt
	 *  Use 0 for no limit.
	 * */
	UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "10", ClampMax = "100000", UIMin = "50", UIMax = "1000"))
	float MaxDistanceToPlayer = 25000;

	/** Cosine of max angle to the player view vector to validate a spawn point location for the next vehicle spawn attempt */
	UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "-1", ClampMax = "1", UIMin = "-1", UIMax = "1"))
	float MaxAngleToPlayer = 0.0;

	/** Cosine of max angle to the player view vector to validate a spawn point location for the next vehicle spawn attempt */
	UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "-1", ClampMax = "1", UIMin = "-1", UIMax = "1"))
	int numberToRespawn = 10;

	/** Min time from the player at their current speed to validate a spawn point location for the next vehicle spawn attempt
	 * In other words, a spawn point is only valid if, given their current speed, the player would require at least this
	 * amount of time to reach it*/
	UPROPERTY(Category = "Vehicle Spawner", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "10", ClampMax = "10000", UIMin = "10", UIMax = "10000"))
	float MinTimeFromPlayer = 2.0;


private:
  
  /** Time handler to spawn more vehicles in the case we could not do it in the beginplay */
  FTimerHandle AttemptTimerHandle;

  /** Checks whether this play start is a valid point given the spatial constraints and positions of existing vehicles
   *  This is separate from checking against the player vehicle because the constrains are laxer (no max angle, max distance)
   *  It still follows the min distance and min time constrains for the player, and applies that to all the vehicles
   *  so the new vehicle does not collide/get spawned on other AI vehicles*/
  bool ValidateSpawnPointAgainstAllVehicles(int spawnIndex);
 
};
