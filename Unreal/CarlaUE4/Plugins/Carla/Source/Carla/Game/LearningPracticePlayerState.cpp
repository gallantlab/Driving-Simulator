// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningPracticePlayerState.h"

ALearningPracticePlayerState::ALearningPracticePlayerState()
    : Super()
{
    currentTargets = TArray<int>();
    isTargetVisited = TArray<bool>();
    targetValues = TArray<int>();
    paused = false;
}


void ALearningPracticePlayerState::Reset()
{
    Super::Reset();
    currentTargets.Empty();
    isTargetVisited.Empty();
    targetValues.Empty();
    paused = false;
}


void ALearningPracticePlayerState::ResetExperimentState()
{
    Super::ResetExperimentState();
    currentTargets.Empty();
    isTargetVisited.Empty();
    targetValues.Empty();
    paused = false;
}


void ALearningPracticePlayerState::CopyProperties(APlayerState *PlayerState)
{
    Super::CopyProperties(PlayerState);
    if ((PlayerState != nullptr) && (this != PlayerState))
    {
        ALearningPracticePlayerState* Other = Cast<ALearningPracticePlayerState>(PlayerState);
        if (Other != nullptr)
        {
            this->NumTargetsToCollect = Other->NumTargetsToCollect;
            this->paused = Other->paused;
            this->currentTargets.Empty();
            this->isTargetVisited.Empty();
            this->targetValues.Empty();
            for (int i = 0; i < Other->currentTargets.Num(); i++)
            {
                this->currentTargets.Add(Other->currentTargets[i]);
                this->isTargetVisited.Add(Other->isTargetVisited[i]);
                this->targetValues.Add(Other->targetValues[i]);
            }
        }
    }
}


void ALearningPracticePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALearningPracticePlayerState, currentTargets);
    DOREPLIFETIME(ALearningPracticePlayerState, isTargetVisited);
    DOREPLIFETIME(ALearningPracticePlayerState, NumTargetsToCollect);
    DOREPLIFETIME(ALearningPracticePlayerState, targetValues);
    DOREPLIFETIME(ALearningPracticePlayerState, paused);
}


int ALearningPracticePlayerState::GetCurrentDestination() const
{
    if (currentTargets.Num() < 1)
        return -1;

    // return the first one that the subject has not collected
    for (int i = 0; i < currentTargets.Num(); i++)
        if (!isTargetVisited[i])
            return currentTargets[i];

    // should not get here
    return -1;
}


