// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CarlaUMGBase.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"



UCarlaUMGBase::UCarlaUMGBase(const FObjectInitializer& initializer)
		: Super(initializer)
{
	controller = nullptr;
	EyetrackingCalibrationOrder = TArray<int>();
	for (int i = 0; i < 70; i++)
		EyetrackingCalibrationOrder.Add(eyetrackingCalibrationOrder[i] - 1);	// b/c in the python one, img 0 is a blank screen
}

void UCarlaUMGBase::FindController()
{
	controller = Cast<ACarlaVehicleController>(GetOwningLocalPlayer() == nullptr ? nullptr : GetOwningLocalPlayer()->GetPlayerController(GetWorld()));
}

int32 UCarlaUMGBase::GetCurrentSpeed() const
{
	if (controller)
	{
		int speed = (int)FMath::RoundHalfToZero(controller->GetPossessedVehicle()->GetVehicleForwardSpeed() * 0.0223704f); // mph conversion
		speed = speed > 0 ? speed : speed * -1;
		return speed;
	}
	else return 0;
}


int32 UCarlaUMGBase::GetOnesSpeed() const
{
	return 0;
}

int32 UCarlaUMGBase::GetTensSpeed() const
{
	return 1;
}

bool UCarlaUMGBase::IsTTL() const
{
	if (controller)
		return controller->isTTL();
	else return false;
}


float UCarlaUMGBase::GetFPS() const
{
	if (controller)
		return 1.0 / controller->GetPlayerState().GetSimulationStepInSeconds();
	else return 0;
}


FVector UCarlaUMGBase::GetPlayerViewPosition()
{
	if (!controller) FindController();
#ifdef NAV
	return controller->GetVehiclePawn()->GetPawnViewLocation();
#else
	return controller->GetPawnOrSpectator()->GetPawnViewLocation();
#endif
}


APawn* UCarlaUMGBase::GetPlayerPawn()
{
	if (!controller) FindController();
	return controller->GetPawn();
}

ACarlaVehicleController* UCarlaUMGBase::GetController()
{
	if (!controller) FindController();
#ifdef NAV
	return (ACarlaVehicleController*)controller;
#else
	return controller;
#endif
}

void UCarlaUMGBase::LogToTerminal(const FString& logMessage)
{
	UE_LOG(LogFMRI, Log, TEXT("%s"), *logMessage);
}

float UCarlaUMGBase::DPIScaleFactor()
{
	if (CalculatedDPIScaleFactor > 0)
		return CalculatedDPIScaleFactor;

	int width, height;
	GetController()->GetViewportSize(width, height);
	CalculatedDPIScaleFactor = 1.0 / GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(width, height));
	UE_LOG(LogFMRI, Log, TEXT("DPI scale factor %f"), CalculatedDPIScaleFactor);

	return CalculatedDPIScaleFactor;
}

int UCarlaUMGBase::GetSpeedLimitMPH() const
{
	if (controller)
		return (int)FMath::RoundHalfToZero(controller->GetSpeedLimit() / 1.609);
	else
		return 0;
}

const int UCarlaUMGBase::eyetrackingCalibrationOrder[] = {14, 31, 18, 17, 3, 28, 2, 29, 26, 11, 27, 10, 15, 6, 35, 33,
														  32, 13, 9, 34, 19, 20, 4, 24, 30, 21, 8, 1, 5, 25, 23, 12, 16,
														  22, 7/*, 31, 7, 16, 34, 33, 11, 21, 1, 8, 20, 9, 22, 2, 12, 5,
														  25, 14, 19, 35, 27, 18, 15, 6, 28, 26, 29, 23, 32, 17, 30, 10,
														  24, 3, 4, 13*/};

void UCarlaUMGBase::PlayerControllerReloadSettings()
{
	if (!controller) FindController();
	if (controller)
	controller->ReloadSettings();
}
