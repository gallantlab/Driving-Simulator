// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningPracticeListLoggerComponent.h"

#include "LearningPracticeListExperimentState.h"

void ULearningPracticeListLoggerComponent::LogFrame()
{
    if (!learningPracticeListPlayerState)
    {
        playerState = spectatorController->GetReferencePlayerState();
        learningPracticeListPlayerState = Cast<ALearningPracticeListPlayerState>(playerState);
        if (!learningPracticeListPlayerState)
        {
            UE_LOG(LogFMRI, Error, TEXT("Player state is not a learning practice player state!"));
            return;
        }
    }
    if (!gameInstance)
        gameInstance = spectatorController->GetCarlaGameInstance();
    spectatorController->AddExperimentState(new LearningPracticeListExperimentState(spectatorController->GetDemoNetDriver()->DemoCurrentTime,
                                                                                spectatorController->GetFrameNumber(),
                                                                                gameInstance->GetDataRouter().GetAgents(),
                                                                                learningPracticeListPlayerState));
}




