// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "DestinationsListHUD.h"

UDestinationsListHUD::UDestinationsListHUD(const FObjectInitializer& initializer)
    : Super(initializer)
{
}

TArray<FDestinationUIInfo> UDestinationsListHUD::GetDestinations()
{
    if (!learningPracticeListController)
    {
        FindController(); // from CarlaDriveHUDBase
    }

    if (learningPracticeListController)
    {
        return learningPracticeListController->GetCurrentDestinationsForUI();
    }

    // Return empty array if controller not found
    return TArray<FDestinationUIInfo>();
}

UTexture2D* UDestinationsListHUD::GetDestinationImage(FString DestinationName)
{
    if (!learningPracticeListController)
    {
        FindController();
    }

    if (learningPracticeListController)
    {
        return learningPracticeListController->FindDestinationImage(DestinationName);
    }

    return nullptr;
}

void UDestinationsListHUD::BindListDelegates()
{
    if (!learningPracticeListController)
    {
        FindController();
    }

    if (learningPracticeListController)
    {
        if (learningPracticeListController->OnDestinationsUpdated.IsBound())
        {
            learningPracticeListController->OnDestinationsUpdated.Unbind();
        }

        learningPracticeListController->OnDestinationsUpdated.BindUObject(this, &UDestinationsListHUD::OnDestinationsListUpdated);

        UE_LOG(LogTemp, Log, TEXT("DestinationsListHUD: Bound to List Updates only."));
    }
}