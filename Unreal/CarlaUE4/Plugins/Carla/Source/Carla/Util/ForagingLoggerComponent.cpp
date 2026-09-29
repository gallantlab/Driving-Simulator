// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ForagingLoggerComponent.h"

#include "ForagingExperimentState.h"


  void UForagingLoggerComponent::LogFrame()
{
	if (!foragingPlayerState)
	{
		playerState = spectatorController->GetReferencePlayerState();
		foragingPlayerState = Cast<AForagingPlayerState>(playerState);
		if (!foragingPlayerState)
		{
			UE_LOG(LogFMRI, Error, TEXT("Player state is not a foraging player state!"));
			return;
		}
	}
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new ForagingExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																		spectatorController->GetFrameNumber(),
																		gameInstance->GetDataRouter().GetAgents(),
																		foragingPlayerState));
}


