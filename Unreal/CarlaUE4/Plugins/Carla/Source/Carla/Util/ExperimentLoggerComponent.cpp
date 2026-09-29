// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ExperimentLoggerComponent.h"


// Sets default values for this component's properties
UExperimentLoggerComponent::UExperimentLoggerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	UE_LOG(LogFMRI, Log, TEXT("Experiment logger component spawned"));
	// ...
}

void UExperimentLoggerComponent::SetParentController(ACarlaSpectatorController *parent)
{
	spectatorController = parent;
	gameInstance = parent->GetCarlaGameInstance();
}


void UExperimentLoggerComponent::LogFrame()
{
	if (!playerState)
		playerState = spectatorController->GetReferencePlayerState();
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new ExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																spectatorController->GetFrameNumber(),
																gameInstance->GetDataRouter().GetAgents(),
																playerState));
}
