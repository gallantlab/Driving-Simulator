// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningTestLoggerComponent.h"
#include "LearningTestExperimentState.h"

ULearningTestLoggerComponent::ULearningTestLoggerComponent()
{
	UE_LOG(LogFMRI, Log, TEXT("Learning Test logger component spawned"))
}

void ULearningTestLoggerComponent::LogFrame()
{
	if (!learningTestPlayerState)
	{
		playerState = spectatorController->GetReferencePlayerState();
		learningTestPlayerState = Cast<ALearningTestPlayerState>(playerState);
		if (!learningTestPlayerState)
		{
			UE_LOG(LogFMRI, Error, TEXT("Player state is not a Learning Test player state!"));
			return;
		}
	}
	if (!gameInstance)
		gameInstance = spectatorController->GetCarlaGameInstance();
	spectatorController->AddExperimentState(new LearningTestExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
																	   spectatorController->GetFrameNumber(),
																	   gameInstance->GetDataRouter().GetAgents(),
																	   learningTestPlayerState));
}





