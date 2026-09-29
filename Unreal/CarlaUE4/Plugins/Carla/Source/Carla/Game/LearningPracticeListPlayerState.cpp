// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningPracticeListPlayerState.h"

ALearningPracticeListPlayerState::ALearningPracticeListPlayerState()
	: Super()
{
	currentTargets = TArray<int>();
	isTargetVisited = TArray<bool>();
	targetValues = TArray<int>();
	paused = false;
	bIsDestinationsListVisible = false;
}


void ALearningPracticeListPlayerState::Reset()
{
	Super::Reset();
	currentTargets.Empty();
	isTargetVisited.Empty();
	targetValues.Empty();
	paused = false;
	bIsDestinationsListVisible = false;
}


void ALearningPracticeListPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	currentTargets.Empty();
	isTargetVisited.Empty();
	targetValues.Empty();
	paused = false;
	bIsDestinationsListVisible = false;
}


void ALearningPracticeListPlayerState::CopyProperties(APlayerState *PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		ALearningPracticeListPlayerState* Other = Cast<ALearningPracticeListPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			this->NumTargetsToVisit = Other->NumTargetsToVisit;
			this->paused = Other->paused;
			this->currentTargets.Empty();
			this->isTargetVisited.Empty();
			this->targetValues.Empty();
			this->bIsDestinationsListVisible = Other->bIsDestinationsListVisible;
			for (int i = 0; i < Other->currentTargets.Num(); i++)
			{
				this->currentTargets.Add(Other->currentTargets[i]);
				this->isTargetVisited.Add(Other->isTargetVisited[i]);
				this->targetValues.Add(Other->targetValues[i]);
			}
		}
	}
}


void ALearningPracticeListPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALearningPracticeListPlayerState, currentTargets);
	DOREPLIFETIME(ALearningPracticeListPlayerState, isTargetVisited);
	DOREPLIFETIME(ALearningPracticeListPlayerState, NumTargetsToVisit);
	DOREPLIFETIME(ALearningPracticeListPlayerState, targetValues);
	DOREPLIFETIME(ALearningPracticeListPlayerState, paused);
	DOREPLIFETIME(ALearningPracticeListPlayerState, bIsDestinationsListVisible);
}
