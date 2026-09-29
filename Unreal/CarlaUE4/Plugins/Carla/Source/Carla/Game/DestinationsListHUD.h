// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaDriveHUDBase.h"
#include "Vehicle/LearningPracticeListController.h" // Needed for the Struct
#include "DestinationsListHUD.generated.h"

/**
 * C++ Backend for the Destinations List UI
 */
UCLASS()
class CARLA_API UDestinationsListHUD : public UCarlaDriveHUDBase
{
    GENERATED_BODY()

public:
    UDestinationsListHUD(const FObjectInitializer& initializer);

    UFUNCTION(BlueprintCallable, Category = "Destinations HUD")
    TArray<FDestinationUIInfo> GetDestinations();

    UFUNCTION(BlueprintCallable, Category = "Destinations HUD")
    UTexture2D* GetDestinationImage(FString DestinationName);

    UFUNCTION(BlueprintCallable, Category = "Destinations HUD")
    void BindListDelegates();
};