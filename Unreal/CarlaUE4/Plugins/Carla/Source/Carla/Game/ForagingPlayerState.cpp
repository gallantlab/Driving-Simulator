// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ForagingPlayerState.h"

AForagingPlayerState::AForagingPlayerState()
	: Super()
{
	currentForageTargets = TArray<int>();
	isTargetForaged = TArray<bool>();
	targetValues = TArray<int>();
	paused = false;
}


void AForagingPlayerState::Reset()
{
	Super::Reset();
	currentForageTargets.Empty();
	isTargetForaged.Empty();
	targetValues.Empty();
	paused = false;
}


void AForagingPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	currentForageTargets.Empty();
	isTargetForaged.Empty();
	targetValues.Empty();
	paused = false;
}


void AForagingPlayerState::CopyProperties(APlayerState *PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		AForagingPlayerState* Other = Cast<AForagingPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			this->NumTargetsToCollect = Other->NumTargetsToCollect;
			this->paused = Other->paused;
			this->currentForageTargets.Empty();
			this->isTargetForaged.Empty();
			this->targetValues.Empty();
			for (int i = 0; i < Other->currentForageTargets.Num(); i++)
			{
				this->currentForageTargets.Add(Other->currentForageTargets[i]);
				this->isTargetForaged.Add(Other->isTargetForaged[i]);
				this->targetValues.Add(Other->targetValues[i]);
			}
		}
	}
}


void AForagingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AForagingPlayerState, currentForageTargets);
	DOREPLIFETIME(AForagingPlayerState, isTargetForaged);
	DOREPLIFETIME(AForagingPlayerState, NumTargetsToCollect);
	DOREPLIFETIME(AForagingPlayerState, targetValues);
	DOREPLIFETIME(AForagingPlayerState, paused);
	DOREPLIFETIME(AForagingPlayerState, trialDuration);
}


int AForagingPlayerState::GetCurrentDestination() const
{
	if (currentForageTargets.Num() < 1)
		return -1;

	// return the first one that the subject has not collected
	for (int i = 0; i < currentForageTargets.Num(); i++)
		if (!isTargetForaged[i])
			return currentForageTargets[i];

	// should not get here
	return -1;
}

