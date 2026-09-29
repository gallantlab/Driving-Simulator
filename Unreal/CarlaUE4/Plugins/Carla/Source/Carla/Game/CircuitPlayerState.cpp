// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitPlayerState.h"

ACircuitPlayerState::ACircuitPlayerState()
	: Super()
{
	currentCircuit = -1;
	nLapsCompleted = 0;
	nLapsCompleted = 0;
	isOnCourse = false;
}


void ACircuitPlayerState::Reset()
{
	Super::Reset();
	currentCircuit = -1;
	nLapsCompleted = 0;
	nLapsCompleted = 0;
	isOnCourse = 0;
}


void ACircuitPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	currentCircuit = -1;
	nLapsCompleted = 0;
	nLapsCompleted = 0;
	isOnCourse = 0;
}

void ACircuitPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		ACircuitPlayerState* Other = Cast<ACircuitPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			this->currentCircuit = Other->currentCircuit;
			this->nLapsDesired = Other->nLapsDesired;
			this->nLapsCompleted = Other->nLapsCompleted;
			this->isOnCourse = Other->isOnCourse;
		}
	}
}

void ACircuitPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACircuitPlayerState, currentCircuit);
	DOREPLIFETIME(ACircuitPlayerState, nLapsDesired);
	DOREPLIFETIME(ACircuitPlayerState, nLapsCompleted);
	DOREPLIFETIME(ACircuitPlayerState, isOnCourse);
}


