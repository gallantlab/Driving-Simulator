// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "TrackRunningPlayerState.h"

ATrackRunningPlayerState::ATrackRunningPlayerState()
	: Super()
{
	numberOfVehicles = 0;
	bIsRunningOnTrack = false;
	trackRunningDirection = 0;
}


void ATrackRunningPlayerState::Reset()
{
	Super::Reset();
	numberOfVehicles = 0;
	bIsRunningOnTrack = false;
	trackRunningDirection = 0;
}


void ATrackRunningPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
//	numberOfVehicles = 0;
	bIsRunningOnTrack = false;
//	trackRunningDirection = 0;
}

void ATrackRunningPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		ATrackRunningPlayerState* Other = Cast<ATrackRunningPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			this->numberOfVehicles = Other->numberOfVehicles;
			this->bIsRunningOnTrack = Other->bIsRunningOnTrack;
			this->trackRunningDirection = Other->trackRunningDirection;
		}
	}
}

void ATrackRunningPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrackRunningPlayerState, numberOfVehicles);
	DOREPLIFETIME(ATrackRunningPlayerState, bIsRunningOnTrack);
	DOREPLIFETIME(ATrackRunningPlayerState, trackRunningDirection);
}