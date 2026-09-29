// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once
#include "Game/CarlaPlayerState.h"
#include "Vehicle/WheeledVehicleController.h"

#include "CarlaVehicleController.generated.h"

struct FCameraPostProcessParameters;
class ACarlaHUD;
class ACarlaPlayerState;
class ALidar;
class ASceneCaptureCamera;
class UCameraDescription;
class ULidarDescription;
struct FCameraPostProcessParameters;

/// The CARLA player controller.
UCLASS()
class CARLA_API ACarlaVehicleController : public AWheeledVehicleController
{
	GENERATED_BODY()

	// ===========================================================================
	/// @name Constructor and destructor
	// ===========================================================================
	/// @{
public:

	ACarlaVehicleController(const FObjectInitializer& ObjectInitializer);

	~ACarlaVehicleController();

	// return the player vehicle pawn, needed for the spectator class
	virtual APawn* GetVehiclePawn();

	/// @}
	// ===========================================================================
	/// @name APlayerController overrides
	// ===========================================================================
	/// @{
public:

	virtual void Possess(APawn *aPawn) override;

	virtual void BeginPlay() override;

	/// @}
	// ===========================================================================
	/// @name AActor overrides
	// ===========================================================================
	/// @{
public:

	virtual void Tick(float DeltaTime) override;

	/// @}
	// ===========================================================================
	/// @name AWheeledVehicleAIController overrides
	// ===========================================================================
	/// @{
public:

	virtual bool IsPossessingThePlayer() const final
	{
		return true;
	}

	virtual void SetFixedRoute(const TArray<FVector> &Locations, bool bOverwriteCurrent,
		int newDirection) override;

	virtual void OnFixedRouteFinished() override;

	virtual void ResumePlayerControl() override;

protected:
	virtual void SetSteeringInput(float value) override;

	virtual void SetThrottleInput(float value) override;

	virtual void SetBrakeInput(float Value) override;

	virtual void HoldHandbrake() override;

	virtual void ReleaseHandbrake() override;

	/// @}
	// ===========================================================================
	/// @name Player state
	// ===========================================================================
	/// @{
public:

	const ACarlaPlayerState &GetPlayerState() const
	{
		return *CarlaPlayerState;
	}

	/// @}
	// ===========================================================================
	/// @name Events
	// ===========================================================================
	/// @{

  /// TTL things
public:
	bool isTTL() const;

protected:
	// to be overidden by classes that need this
	virtual void OnPlayerRegainControl() {};

	virtual void TTLdown();

	virtual void TTLup();

	virtual void SetupInputComponent() override;
	   
	void PlayBeep();

	void PlayPickup();

	UFUNCTION(BlueprintCallable)
	void SetSecondsToStartOfRun(float seconds);

	bool isFirstTTLInRun = true;	// used for recording the first TTL in the run

	FTimerHandle beepTimer;

	void OnBeepEnd()
	{
		CarlaPlayerState->PlayBeep = false;
	}

	// Points methods
	virtual void UpdatePoints(int increment = 1)
	{
		CarlaPlayerState->Points += increment;
	}

	void ResetPoints()
	{
		CarlaPlayerState->Points = 0;
	}

public:
	virtual void ReloadSettings();

private:
	// stuff for playing a beep
	USoundCue *beepCue;

	// Stuff for playing a coin noise
	USoundCue *pickupCue;

	UFUNCTION()
	void OnCollisionEvent(
		AActor* Actor,
		AActor* OtherActor,
		FVector NormalImpulse,
		const FHitResult& Hit);

	/// @}
	// ===========================================================================
	/// @name Private methods
	// ===========================================================================
	/// @{
private:

	void IntersectPlayerWithRoadMap();

	/// @}
	// ===========================================================================
	// -- Member variables -------------------------------------------------------
	// ===========================================================================
protected:

	// Cast for quick access to the custom player state.
	UPROPERTY()
		ACarlaPlayerState *CarlaPlayerState;

	// Cast for quick access to the custom HUD.
	UPROPERTY()
		ACarlaHUD *CarlaHUD;

	void RelinquishPlayerControl();

	bool bRelinquishControlOnStop = false;

private:
	// used for holding brakes until user input
	float holdBrakes = 0;
	float secondsBeforeAISteering = 0;
	float secondsWithNoSteeringInput = 0;
	float lastSteering = 0.0f;

	// control sensitivity multipliers
	// when the logistic function is zeroed out, these are the max values
	// when the logistic function is in use, these are the carrying capacities
	float steerSensitivity = 1.0f;
	float throttleSensitivity = 1.0f;
	float brakeSensitivity = 1.0f;

	// control logistic function values
	// in which the current speed affects the sensitivity of the controller
	// brake shouldn't really be modified, but included here for completeness

	// the exponent specifies how flat the curve is
	float steerExponent = 0.0f;
	float throttleExponent = 0.0f;
	float brakeExponent = 0.0f;

	// the midpoint specifies at what speed the sensitivity drops to half
	// in units of unreal speed
	float steerMidpoint = 0.0f;
	float throttleMidpoint = 0.0f;
	float brakeMidpoint = 0.0f;

protected:
	void SetSecondsBeforeAISteering(float seconds)
	{
		secondsBeforeAISteering = seconds;
	}

public:
	UFUNCTION(BlueprintPure)
	float SteeringValue() const;
};
