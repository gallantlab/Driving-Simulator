// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationLoggerComponent.h"
#include "NavigationExperimentState.h"

void UNavigationLoggerComponent::LogFrame()
{
	if (!navigationPlayerState)
	{
		playerState = spectatorController->GetReferencePlayerState();
		navigationPlayerState = Cast<ANavigationPlayerState>(playerState);
		if (!navigationPlayerState)
		{
			UE_LOG(LogFMRI, Error, TEXT("Player state is not a navigation player state!"));
			return;
		}
	}
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new NavigationExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																		  spectatorController->GetFrameNumber(),
																		  gameInstance->GetDataRouter().GetAgents(),
																		  navigationPlayerState));
}
