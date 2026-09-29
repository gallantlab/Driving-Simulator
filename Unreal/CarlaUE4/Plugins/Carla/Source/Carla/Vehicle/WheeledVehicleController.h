// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Vehicle/WheeledVehicleAIController.h"

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "WheeledVehicleController.generated.h"

/// Wheeled vehicle controller with cameras and optional user input.
UCLASS()
class CARLA_API AWheeledVehicleController : public AWheeledVehicleAIController
{
	GENERATED_BODY()

	// ===========================================================================
	/// @name Constructor
	// ===========================================================================
	/// @{
public:

	AWheeledVehicleController(const FObjectInitializer& ObjectInitializer);

	~AWheeledVehicleController();

	/// @}
	// ===========================================================================
	/// @name AActor overrides
	// ===========================================================================
	/// @{
public:

	virtual void BeginPlay() override;

	/// @}
	// ===========================================================================
	/// @name APlayerController overrides
	// ===========================================================================
	/// @{
public:

	virtual void SetupInputComponent() override;

	virtual void Possess(APawn *aPawn) override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	/// @}
	// ===========================================================================
	/// @name User input
	// ===========================================================================
	/// @{
public:

	/// Enable keyboard control.
	UFUNCTION(Category = "Vehicle User Input", BlueprintCallable)
	virtual void EnableUserInput(bool Enable);

	// reattach cameras
	bool AttachCamerasTo(APawn*);

	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	virtual void SetAutopilot(bool Enable, bool ClearPlannedLocations = true) override;

	virtual void RestartLevel() override;

	virtual void ResumePlayerControl();

/// @}
	// ===========================================================================
	/// @name Camera movement
	// ===========================================================================
	/// @{
private:

	void ChangeCameraZoom(float Value);

	void ChangeCameraUp(float Value);

	void ChangeCameraRight(float Value);

	void EnableOnBoardCamera(bool bEnable = true, bool bForce = false);

	void ToggleCamera()
	{
		EnableOnBoardCamera(!bOnBoardCameraIsActive);
	}

	/// @}
	// ===========================================================================
	/// @name Vehicle movement
	// ===========================================================================
	/// @{
protected:
	virtual void SetSteeringInput(float Value);

	virtual void SetThrottleInput(float Value);

	virtual void SetBrakeInput(float Value);

	virtual void ToggleReverse();

	virtual void HoldHandbrake();

	virtual void ReleaseHandbrake();

	/// @}
	// ===========================================================================
	// -- Member variables -------------------------------------------------------
	// ===========================================================================
protected:

	UPROPERTY(EditAnywhere)
	USpringArmComponent *SpringArm;

	UPROPERTY(EditAnywhere)
	UCameraComponent *PlayerCamera;

	UPROPERTY(EditAnywhere)
	UCameraComponent *OnBoardCamera;

	UPROPERTY()
	bool bOnBoardCameraIsActive = true;

protected:
	UPROPERTY(Category = "Vehicle User Input", VisibleAnywhere)
	bool bAllowUserInput = false;

	UPROPERTY(Category = "Vehicle User Input", VisibleAnywhere)
	bool bGovernorActive = false;

};
