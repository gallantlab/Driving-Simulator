// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#ifdef _WIN32
// file not found on linux. it compiles without the explicit include
// but keeping it here because explicit includes are good
#include "HAL/PlatformFileManager.h"
#endif
#include "Misc/Paths.h"
#include "SettingsHUDBase.h"



USettingsHUDBase::USettingsHUDBase(const FObjectInitializer& initializer)
	: Super(initializer)
{

}

void USettingsHUDBase::UpdateActiveDestinationsInController()
{
	if (!navigationController)
		FindController();
	navigationController->ReloadActiveDestinations();
}

UDestinationParserComponent * USettingsHUDBase::GetDestinationParserComponent()
{
	if (!navigationController)
		FindController();
	return navigationController->GetDestinationParserComponent();
}

int USettingsHUDBase::GetCurrentDestination()
{
	if (!navigationController)
		FindController();
	return navigationController->GetCurrentDestinationID();
}

void USettingsHUDBase::ControllerRestartLevel()
{
	if (!navigationController)
		FindController();
	navigationController->RestartLevel();
}

void USettingsHUDBase::GenerateNextDestination()
{
	if (!navigationController)
		FindController();
	navigationController->OnSegmentEnd(0);
}

const TArray<FString>& USettingsHUDBase::GetSavedConfigs()
{
	if (bIsSavedConfigsStale)
	{
		// Get function will refresh list every time it is called
		SavedConfigs.Empty();

		IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
		FString SaveConfigDir = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Experiment Configs"));
		UE_LOG(LogFMRI, Log, TEXT("Looking in/for %s"), *FPaths::ConvertRelativePathToFull(SaveConfigDir));
		if (FileManager.DirectoryExists(*SaveConfigDir))
		{
			FileManager.FindFiles(SavedConfigs, *SaveConfigDir, TEXT("ini"));
			UE_LOG(LogFMRI, Log, TEXT("%d saved config files found"), SavedConfigs.Num());
			for (int i = 0; i < SavedConfigs.Num(); i++)
			{
				SavedConfigs[i] = FPaths::GetBaseFilename(SavedConfigs[i]);
				UE_LOG(LogFMRI, Log, TEXT("File %s"), *(SavedConfigs[i]));
			}
		}
		else
		{
			FileManager.CreateDirectory(*SaveConfigDir);
			UE_LOG(LogFMRI, Log, TEXT("Save directory not found and created."));
		}
		bIsSavedConfigsStale = false;
	}

	return SavedConfigs;
}

FString USettingsHUDBase::GetFullSavePath(const FString& configFileName)
{
	return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Experiment Configs"),
						   FString::Printf(TEXT("%s.ini"), *configFileName));
}