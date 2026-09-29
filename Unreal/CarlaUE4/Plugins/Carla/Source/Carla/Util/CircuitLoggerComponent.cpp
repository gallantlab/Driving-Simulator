// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitExperimentState.h"
#include "CircuitLoggerComponent.h"


UCircuitLoggerComponent::UCircuitLoggerComponent()
{
	UE_LOG(LogFMRI, Log, TEXT("Circuit logger component spawned"))
}

void UCircuitLoggerComponent::LogFrame()
{
	if (!circuitPlayerState)
	{
		playerState = spectatorController->GetReferencePlayerState();
		circuitPlayerState = Cast<ACircuitPlayerState>(playerState);
		if (!circuitPlayerState)
		{
			UE_LOG(LogFMRI, Error, TEXT("Player state is not a circuit player state!"));
			return;
		}
	}
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new CircuitExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																			spectatorController->GetFrameNumber(),
																			gameInstance->GetDataRouter().GetAgents(),
																			circuitPlayerState));
}


