// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "TrackRunningExperimentState.h"
#include "TrackRunningLoggerComponent.h"

UTrackRunningLoggerComponent::UTrackRunningLoggerComponent()
{
	UE_LOG(LogFMRI, Log, TEXT("Track running logger component spawned"))
}

void UTrackRunningLoggerComponent::LogFrame()
{
	if (!trackRunningPlayerState)
	{
		playerState = spectatorController->GetReferencePlayerState();
		trackRunningPlayerState = Cast<ATrackRunningPlayerState>(playerState);
		if (!trackRunningPlayerState)
		{
			UE_LOG(LogFMRI, Error, TEXT("Player state is not a track running player state!"));
			return;
		}
	}
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new TrackRunningExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																			spectatorController->GetFrameNumber(),
																			gameInstance->GetDataRouter().GetAgents(),
																			trackRunningPlayerState));
}


