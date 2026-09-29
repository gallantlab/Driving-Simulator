// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include <queue>

#include "GameFramework/PlayerController.h"

#include "Traffic/TrafficLightState.h"
#include "Vehicle/VehicleControl.h"
#include "Traffic/SpeedZoneBase.h"

#include "WheeledVehicleAIController.generated.h"

class ACarlaWheeledVehicle;
class URandomEngine;
class URoadMap;


/// Enum for logging despawn reasons
UENUM(BlueprintType)
enum class EDespawnReason : uint8
{
	OffScreen			UMETA(DisplayName = "Offscreen"),
	NotMoving			UMETA(DisplayName = "NotMoving"),
	DespawnTrigger		UMETA(DisplayName = "DespawnTrigger")
};



/// Wheeled vehicle controller with optional AI.
UCLASS()
class CARLA_API AWheeledVehicleAIController : public APlayerController
{
	GENERATED_BODY()

	// ===========================================================================
	/// @name Constructor and destructor
	// ===========================================================================
	/// @{
public:

	AWheeledVehicleAIController(const FObjectInitializer& ObjectInitializer);

	~AWheeledVehicleAIController();

	/// @}
	// ===========================================================================
	/// @name APlayerController overrides
	// ===========================================================================
	/// @{
public:

	virtual void Possess(APawn *aPawn) override;

	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

	/// @}
	// ===========================================================================
	/// @name Possessed vehicle
	// ===========================================================================
	/// @{
public:

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	bool IsPossessingAVehicle() const
	{
		return Vehicle != nullptr;
	}

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	ACarlaWheeledVehicle *GetPossessedVehicle()
	{
		return Vehicle;
	}

	const ACarlaWheeledVehicle *GetPossessedVehicle() const
	{
		return Vehicle;
	}

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	virtual bool IsPossessingThePlayer() const
	{
		return false;
	}

	/// @}
	// ===========================================================================
	/// @name Road map
	// ===========================================================================
	/// @{
public:

	void SetRoadMap(URoadMap *InRoadMap)
	{
		RoadMap = InRoadMap;
	}

	UFUNCTION(Category = "Road Map", BlueprintCallable)
	URoadMap *GetRoadMap()
	{
		return RoadMap;
	}

	/// @}
	// ===========================================================================
	/// @name Random engine
	// ===========================================================================
	/// @{
public:

	UFUNCTION(Category = "Random Engine", BlueprintCallable)
	URandomEngine *GetRandomEngine()
	{
		check(RandomEngine != nullptr);
		return RandomEngine;
	}

	/// @}
	// ===========================================================================
	/// @name Autopilot
	// ===========================================================================
	/// @{
public:

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	bool IsAutopilotEnabled() const
	{
		return bAutopilotEnabled;
	}

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	virtual void SetAutopilot(bool Enable, bool ClearPlannedLocations = true)
	{
		if (IsAutopilotEnabled() != Enable) {
			ConfigureAutopilot(Enable, ClearPlannedLocations);
		}
	}

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	void ToggleAutopilot()
	{
		ConfigureAutopilot(!bAutopilotEnabled);
	}

protected:

	void ConfigureAutopilot(bool Enable, bool ClearPlannedLocations = true);

	/// @}
	// ===========================================================================
	/// @name Traffic
	// ===========================================================================
	/// @{

private:
	bool IsVehicleInIntersection() const;

protected:
	bool bMoveSlow = false;	// do we want to move slowly as if we're in an intersection?

public:

	/// Get current allowable speed
	/// Is current speed limit unless in an intersection
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	float GetCurrentMaxAllowedSpeed() const
	{
		if (bMoveSlow) return 8.045;	// max 5 mph if move slow
		return IsVehicleInIntersection() ? 30 : CurrentSpeedLimit;
	}

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	float GetSpeedLimit() const
	{
		return CurrentSpeedLimit;
	}

	/// Set vehicle's speed limit in km/h.
	/// Is the default speed limit
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	void SetSpeedLimit(float InSpeedLimit)
	{
		UE_LOG(LogCarla, Log, TEXT("Speed limit set %.2f"), InSpeedLimit);
		DefaultSpeedLimit = InSpeedLimit;
		if (!IsVehicleInIntersection())
			CurrentSpeedLimit = DefaultSpeedLimit;
	}

	/// Set the speed for the current speed zone
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	void SetSpeedZoneSpeedLimit(float speedLimit)
	{
		UE_LOG(LogCarla, Log, TEXT("Speed zone limit set %.2f"), speedLimit);
		CurrentSpeedLimit = speedLimit;
	}

	/// Exiting a speed zone and return to default speed limit
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	void ReturnToDefaultSpeedLimit()
	{
		UE_LOG(LogCarla, Log, TEXT("Returned to default speed limit %.2f"), DefaultSpeedLimit);
		CurrentSpeedLimit = DefaultSpeedLimit;
	}


	/// Get traffic light state currently affecting this vehicle.
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	ETrafficLightState GetTrafficLightState() const
	{
		return TrafficLightState;
	}

	/// Set traffic light state currently affecting this vehicle.
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	void SetTrafficLightState(ETrafficLightState InTrafficLightState)
	{
		TrafficLightState = InTrafficLightState;
	}

	/// Set a fixed route to follow if autopilot is enabled.
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	/**
	 * Set a fixed route to follow if autopilot is enabled
	 * @param Locations 			control points in route to follow
	 * @param bOverwriteCurrent 	if this controller already has a fixed route, should it be overwritted?
	 * @param newDirection 			direction that the subject will be facing at the end of this route, for track/circuit running - should be 0 for everything else
	 */
	virtual void SetFixedRoute(const TArray<FVector> &Locations, bool bOverwriteCurrent = true, int newDirection = 0);

	/// Queue up for despawning
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	/**
	 * Despawns this vehicle
	 * @param reason 	why is this vehicle being despawned
	 */
	virtual void DespawnVehicle(EDespawnReason reason);

protected:
	// Stuff for despawning
	float impatience = 30.0;

	float secondsAtRest = 0;

	/// @}
	// ===========================================================================
	/// @name AI
	// ===========================================================================
	/// @{
protected:

	const FVehicleControl &GetAutopilotControl() const
	{
		return AutopilotControl;
	}

	virtual void TickAutopilotController(float deltaTime);

	virtual void TagPawn();

	/// Returns steering value for following a fixed route from a route planner
	float GoToNextTargetLocation(FVector &Direction);

	/// Used in child classes for handing control back to the player
	virtual void OnFixedRouteFinished() { return;};

	/// Returns steering value for staying on the road
	float CalcSteeringValue(FVector &Direction);

	/// Returns throttle value.
	float Stop(float Speed);

	/// Returns throttle value.
	float Move(float Speed);

	/// @}
	// ===========================================================================
	// -- Member variables -------------------------------------------------------
	// ===========================================================================
private:

	UPROPERTY()
	ACarlaWheeledVehicle *Vehicle = nullptr;

	UPROPERTY()
	URoadMap *RoadMap = nullptr;

	UPROPERTY()
	URandomEngine *RandomEngine = nullptr;

	UPROPERTY(VisibleAnywhere)
	bool bAutopilotEnabled = false;

	UPROPERTY(VisibleAnywhere)
	float DefaultSpeedLimit = 35.0f * 1.609;	// Default speed limit if not in a speed zone

	UPROPERTY(VisibleAnywhere)
	float CurrentSpeedLimit = -1;	// Current speed limit determined by speed zone

	UPROPERTY(VisibleAnywhere)
	ETrafficLightState TrafficLightState = ETrafficLightState::Green;

	UPROPERTY(VisibleAnywhere)
	float MaximumSteerAngle = -1.0f;

	FVehicleControl AutopilotControl;

	std::queue<FVector> TargetLocations;

	float liveTime = 0.0f;

	float maxOffScreenTime = 0.0f;

	TArray<ASpeedZoneBase*> intersections;
};
