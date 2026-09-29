// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/PlayerState.h"
#include "Traffic/TrafficLightState.h"
#include "CarlaPlayerState.generated.h"

/// Current state of the player, updated every frame by ACarlaVehicleController.
///
/// This class matches the reward that it is sent to the client over the
/// network.
UCLASS()
class CARLA_API ACarlaPlayerState : public APlayerState
{
	GENERATED_BODY()

	// ===========================================================================
	// -- APlayerState -----------------------------------------------------------
	// ===========================================================================
public:

	ACarlaPlayerState();

	virtual void Reset() override;

	virtual void CopyProperties(APlayerState *PlayerState) override;

	// does less things than Reset() since that is called on level restart
	virtual void ResetExperimentState();

	// ===========================================================================
	// -- Getters ----------------------------------------------------------------
	// ===========================================================================
public:

	// ===========================================================================
	/// @name Timing
	// ===========================================================================
	/// @{

	uint64 GetFrameNumber() const
	{
	return FrameNumber;
	}

	UFUNCTION(BlueprintCallable)
	float GetSimulationStepInSeconds() const
	{
	return SimulationStepInSeconds;
	}

	UFUNCTION(BlueprintCallable)
	int32 GetPlatformTimeStamp() const
	{
	return PlatformTimeStamp;
	}

	UFUNCTION(BlueprintCallable)
	int32 GetGameTimeStamp() const
	{
	return GameTimeStamp;
	}

	bool isTTL() const
	{
	return TTL;
	}

	bool IsBeep() const
	{
		return PlayBeep;
	}


	/// @}
	// ===========================================================================
	/// @name Transform and dynamics
	// ===========================================================================
	/// @{

	UFUNCTION(BlueprintCallable)
	const FTransform &GetTransform() const
	{
	return Transform;
	}

	UFUNCTION(BlueprintCallable)
	FVector GetLocation() const
	{
	return Transform.GetLocation();
	}

	UFUNCTION(BlueprintCallable)
	FVector GetOrientation() const
	{
	return Transform.GetRotation().GetForwardVector();
	}

	UFUNCTION(BlueprintCallable)
	FTransform GetBoundingBoxTransform() const
	{
	return BoundingBoxTransform;
	}

	UFUNCTION(BlueprintCallable)
	FVector GetBoundingBoxExtent() const
	{
	return BoundingBoxExtent;
	}

	UFUNCTION(BlueprintCallable)
	float GetForwardSpeed() const
	{
	return ForwardSpeed;
	}

	UFUNCTION(BlueprintCallable)
	const FVector &GetAcceleration() const
	{
	return Acceleration;
	}

	/// @}
	// ===========================================================================
	/// @name Vehicle control
	// ===========================================================================
	/// @{

	UFUNCTION(BlueprintCallable)
	float GetThrottle() const
	{
	return Throttle;
	}

	UFUNCTION(BlueprintCallable)
	float GetSteer() const
	{
	return Steer;
	}

	UFUNCTION(BlueprintCallable)
	float GetBrake() const
	{
	return Brake;
	}

	UFUNCTION(BlueprintCallable)
	bool GetHandBrake() const
	{
	return bHandBrake;
	}

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentGear() const
	{
	return CurrentGear;
	}

	UFUNCTION(BlueprintCallable)
	float GetSpeedLimit() const
	{
	return SpeedLimit;
	}

	UFUNCTION(BlueprintCallable)
	ETrafficLightState GetTrafficLightState() const
	{
	return TrafficLightState;
	}

	UFUNCTION(BlueprintCallable)
	bool IsUnderPlayerControl() const
	{
		return bIsUnderPlayerControl;
	}

	UFUNCTION(BlueprintCallable)
	bool IsAutoSteerOn() const
	{
		return bIsAutoSteerOn;
	}

	UFUNCTION(BlueprintCallable)
	int GetTotalTTLs() const
	{
  		return TotalTTLs;
	}

	/// @}
	// ===========================================================================
	/// @name Collision
	// ===========================================================================
	/// @{

	UFUNCTION(BlueprintCallable)
	float GetCollisionIntensityCars() const
	{
	return CollisionIntensityCars;
	}

	UFUNCTION(BlueprintCallable)
	float GetCollisionIntensityPedestrians() const
	{
	return CollisionIntensityPedestrians;
	}

	UFUNCTION(BlueprintCallable)
	float GetCollisionIntensityOther() const
	{
	return CollisionIntensityOther;
	}

	/// @}
	// ===========================================================================
	/// @name Road intersection
	// ===========================================================================
	/// @{

	UFUNCTION(BlueprintCallable)
	float GetOtherLaneIntersectionFactor() const
	{
	return OtherLaneIntersectionFactor;
	}

	UFUNCTION(BlueprintCallable)
	float GetOffRoadIntersectionFactor() const
	{
	return OffRoadIntersectionFactor;
	}

	UFUNCTION(BlueprintCallable)
	float GetSecondsToStartOfRun() const
	{
		return secondsToStartOfRun;
	}

	UFUNCTION(BlueprintCallable)
	float GetDesiredThrottle() const
	{
  		return DesiredThrottle;
	}

/// Misc
	UFUNCTION(BlueprintCallable)
	int GetPoints() const
	{
		return Points;
	}

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

  /// @}
  // ===========================================================================
  // -- Modifiers --------------------------------------------------------------
  // ===========================================================================
	private:

	void RegisterCollision(
	  AActor *Actor,
	  AActor *OtherActor,
	  const FVector &NormalImpulse,
	  const FHitResult &Hit);

	void UpdateTimeStamp(float DeltaSeconds);

	// ===========================================================================
	// -- Private members --------------------------------------------------------
	// ===========================================================================
private:

	friend class ACarlaVehicleController;

	// If you add another variable here, don't forget to copy it inside
	// CopyProperties if necessary.

	UPROPERTY(VisibleAnywhere)
	uint64 FrameNumber;

	UPROPERTY(VisibleAnywhere)
	float SimulationStepInSeconds;

	UPROPERTY(VisibleAnywhere)
	int32 PlatformTimeStamp;

	UPROPERTY(VisibleAnywhere)
	int32 GameTimeStamp = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	FTransform Transform;

	UPROPERTY(VisibleAnywhere, Replicated)
	FTransform BoundingBoxTransform;

	UPROPERTY(VisibleAnywhere, Replicated)
	FVector BoundingBoxExtent;

	UPROPERTY(VisibleAnywhere, Replicated)
	float ForwardSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	FVector Acceleration;

	UPROPERTY(VisibleAnywhere, Replicated)
	float Throttle = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	float Steer = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	float Brake = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool bHandBrake = false;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 CurrentGear;

	UPROPERTY(VisibleAnywhere)
	float SpeedLimit = -1.0f;

	UPROPERTY(VisibleAnywhere)
	ETrafficLightState TrafficLightState = ETrafficLightState::Green;

	UPROPERTY(VisibleAnywhere)
	float CollisionIntensityCars = 0.0f;

	UPROPERTY(VisibleAnywhere)
	float CollisionIntensityPedestrians = 0.0f;

	UPROPERTY(VisibleAnywhere)
	float CollisionIntensityOther = 0.0f;

	UPROPERTY(VisibleAnywhere)
	float OtherLaneIntersectionFactor = 0.0f;

	UPROPERTY(VisibleAnywhere)
	float OffRoadIntersectionFactor = 0.0f;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool TTL = false;

	UPROPERTY(VisibleAnywhere, Replicated)
	int TotalTTLs = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool PlayBeep = false;

	UPROPERTY(VisibleAnywhere, Replicated)
	float secondsToStartOfRun = 0.0;			// number of seconds between the beginning of the demo and start of the run, i.e. first TTL

	// driver assist-related stuff
	UPROPERTY(VisibleAnywhere, Replicated)
	bool bIsUnderPlayerControl = true;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool bIsAutoSteerOn = false;

	// because of the governor, the actual throttle may not be the desired throttle
	UPROPERTY(VisibleAnywhere, Replicated)
	float DesiredThrottle = 0.0;

	// a score thing to help motivate people
	UPROPERTY(VisibleAnywhere, Replicated)
	int Points = 0;
};
