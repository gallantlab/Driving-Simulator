// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationPlayerState.h"


ANavigationPlayerState::ANavigationPlayerState()
	: Super()
{
	currentDestination = -1;
}

void ANavigationPlayerState::Reset()
{
	Super::Reset();
	currentDestination = -1;
	secondsBeforeFirstDestination = 0.0;
	isLost = false;
	showingHelp = false;
}

void ANavigationPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	currentDestination = -1;
	secondsBeforeFirstDestination = 0.0;
	isLost = false;
	showingHelp = false;
}

void ANavigationPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		ANavigationPlayerState* Other = Cast<ANavigationPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			currentDestination = Other->currentDestination;
			isLost = Other->isLost;
			showingHelp = Other->showingHelp;
		}
	}
}

void ANavigationPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANavigationPlayerState, currentDestination);
	DOREPLIFETIME(ANavigationPlayerState, secondsBeforeFirstDestination);
	DOREPLIFETIME(ANavigationPlayerState, isLost);
	DOREPLIFETIME(ANavigationPlayerState, showingHelp);
}