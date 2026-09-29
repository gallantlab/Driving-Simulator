// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CarlaTrackRunningHUDBase.h"

const FString LOW_TRAFFIC = FString("low traffic");
const FString MEDIUM_TRAFFIC = FString("medium traffic");
const FString HIGH_TRAFFIC = FString("high traffic");
const FString END_OF_TRACK = FString("end of trial");


UCarlaTrackRunningHUDBase::UCarlaTrackRunningHUDBase(const FObjectInitializer &objectInitializer)
	: Super(objectInitializer)
{
	trackRunningController = nullptr;
}


FString UCarlaTrackRunningHUDBase::GetDisplayText() const
{
	if (trackRunningController)
	{
//		if (trackRunningController->ShowInstructions())
//		{
//			switch (trackRunningController->GetTrafficDensity())
//			{
//				case ETrafficDensity::Low :
//					return LOW_TRAFFIC;
//				case ETrafficDensity::Normal :
//					return MEDIUM_TRAFFIC;
//				case ETrafficDensity::High :
//					return HIGH_TRAFFIC;
//			}
//		}
//		else if (trackRunningController->ShowArrival())
//		{
//			return END_OF_TRACK;
//		}
	}
	return FString("");
}


void UCarlaTrackRunningHUDBase::FindController()
{
	Super::FindController();
	trackRunningController = Cast<ATrackRunningController>(controller);
}