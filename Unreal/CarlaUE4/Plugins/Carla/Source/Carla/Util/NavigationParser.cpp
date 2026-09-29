// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationParser.h"

bool NavigationParser::ParseFile(const FString& destinationFile, TArray<NavigationDestination>& destinations)
{
	TArray<FString> fileTextContents = TArray<FString>();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*destinationFile))
		FFileHelper::LoadFileToStringArray(fileTextContents, *destinationFile);
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("Destination file %s does not exist"), *destinationFile);
		return false;
	}

	if (fileTextContents.Num() < 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Empty file"));
		return false;
	}
	else
	{
		if (destinations.Num() > 0)
		{
			UE_LOG(LogFMRI, Warning, TEXT("Non-empty destinations array will be overwritten"));
			destinations.Empty();
		}
		TArray<FString> theseTokens = TArray<FString>();
		for (int i = 0; i < fileTextContents.Num(); i++)
		{
			// line format is [id],[name],[x position],[y position]
			fileTextContents[i].ParseIntoArray(theseTokens, TEXT(","));
			destinations.Add(NavigationDestination(theseTokens[1], FCString::Atoi(*theseTokens[0]), FCString::Atof(*theseTokens[2]), FCString::Atof(*theseTokens[3])));
			theseTokens.Empty();
		}
		destinations.Sort();
		UE_LOG(LogFMRI, Log, TEXT("%d location entries"), destinations.Num());
		for (int i = 0; i < destinations.Num(); i++)
			UE_LOG(LogFMRI, Log, TEXT("Destination %d: %s"), i, *(destinations[i].GetName()));
		return true;
	}
}

bool NavigationParser::WriteFile(const FString& destinationFile, TArray<NavigationDestination>& destinations)
{
	UE_LOG(LogFMRI, Log, TEXT("Parser write locations"));
	if (destinations.Num() < 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Empty array of destinations"));
		return  false;
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> lines = TArray<FString>();
	NavigationDestination *thisDest = nullptr;

	for (int i = 0; i < destinations.Num(); i++)
	{
		thisDest = &destinations[i];
		lines.Add(FString::Printf(TEXT("%d,%s,%f,%f"), thisDest->GetID(), *thisDest->GetName(), thisDest->GetDestination().X, thisDest->GetDestination().Y));
	}

	FString savePath = FString("/D/") + destinationFile;

	return FFileHelper::SaveStringArrayToFile(lines, *savePath);
}

bool NavigationParser::ParseNavigationSequence(const FString& navigationSequenceFile, TArray<int>& sequence)
{
	TArray<FString> fileTextContents = TArray<FString>();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*navigationSequenceFile))
		FFileHelper::LoadFileToStringArray(fileTextContents, *navigationSequenceFile);
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("Destination sequence file %s does not exist"), *navigationSequenceFile);
		return false;
	}

	UE_LOG(LogFMRI, Log, TEXT("Parsing destination sequence file %s"), *navigationSequenceFile);
	if (fileTextContents.Num() < 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Empty file"));
		return false;
	}
	else
	{
		if (sequence.Num() > 0)
		{
			UE_LOG(LogFMRI, Warning, TEXT("Non-empty navigation sequence array will be overwritten"));
			sequence.Empty();
		}
		TArray<FString> theseTokens = TArray<FString>();
		for (int i = 0; i < fileTextContents.Num(); i++)
		{
			// line format is [id],[name],[x position],[y position]
			fileTextContents[i].ParseIntoArray(theseTokens, TEXT(","));
			for (int j = 0; j < theseTokens.Num(); j++)
				sequence.Add(FCString::Atoi(*theseTokens[j]));
			//destinations.Add(NavigationDestination(theseTokens[1], FCString::Atoi(*theseTokens[0]), FCString::Atof(*theseTokens[2]), FCString::Atof(*theseTokens[3])));
			theseTokens.Empty();
		}
		UE_LOG(LogFMRI, Log, TEXT("%d in sequence"), sequence.Num());
		for (int i = 0; i < sequence.Num(); i++)
			UE_LOG(LogFMRI, Log, TEXT("Destination %d: %d"), i, sequence[i]);
		return true;
	}
}


bool NavigationParser::ParseActiveDestinations(const FString &activeDestinationsFile, TArray<int> &activeDestinations)
{
	return NavigationParser::ParseActiveDestinations(activeDestinationsFile, activeDestinations, true);
}


bool NavigationParser::ParseActiveDestinations(const TArray<FString> &activeDestinationsFiles, TArray<int> &activeDestinations)
{
	UE_LOG(LogFMRI, Log, TEXT("Parsing %d files"), activeDestinationsFiles.Num());
	for (int i = 0; i < activeDestinationsFiles.Num(); i++)
	{
		if (!ParseActiveDestinations(activeDestinationsFiles[i], activeDestinations, false))
			return false;
	}
	return true;
}


bool NavigationParser::ParseActiveDestinations(const FString &activeDestinationsFile, TArray<int> &activeDestinations, bool overwrite)
{
	// if multiple files
	TArray<FString> activeDestinationsFiles = TArray<FString>();
	if (activeDestinationsFile.ParseIntoArray(activeDestinationsFiles, TEXT(",")) > 1)
	{
		// extra overwrite check at beginning because of the delimiter check for multiple files
		if (overwrite)
		{
			UE_LOG(LogFMRI, Warning, TEXT("Non-empty active destinations array will be overwritten"));
			activeDestinations.Empty();
		}
		return ParseActiveDestinations(activeDestinationsFiles, activeDestinations);
	}

	// parse single file
	TArray<FString> fileTextContents = TArray<FString>();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString absolutePath = FPaths::Combine(FPaths::ProjectConfigDir(), activeDestinationsFile);
	if (PlatformFile.FileExists(*absolutePath))
		FFileHelper::LoadFileToStringArray(fileTextContents, *absolutePath);
	else
	{
		UE_LOG(LogFMRI, Error, TEXT("Active destinations file %s does not exist"), *activeDestinationsFile);
		return false;
	}
	UE_LOG(LogFMRI, Log, TEXT("Parsing active destinations sequence file %s"), *activeDestinationsFile);
	if (fileTextContents.Num() < 1)
	{
		UE_LOG(LogFMRI, Error, TEXT("Empty file"));
		return false;
	}
	else
	{
		if (activeDestinations.Num() > 0)
		{
			if (overwrite)
			{
				UE_LOG(LogFMRI, Warning, TEXT("Non-empty active destinations array will be overwritten"));
				activeDestinations.Empty();
			}
			else
			{
				UE_LOG(LogFMRI, Log, TEXT("Appending to array with %d entries"), activeDestinations.Num())
			}
		}
		TArray<FString> theseTokens = TArray<FString>();
		for (int i = 0; i < fileTextContents.Num(); i++)
		{
			// comma separate IDs
			// can be any number on a line
			fileTextContents[i].ParseIntoArray(theseTokens, TEXT(","));
			for (int j = 0; j < theseTokens.Num(); j++)
				activeDestinations.Add(FCString::Atoi(*theseTokens[j]));
			theseTokens.Empty();
		}
		UE_LOG(LogFMRI, Log, TEXT("%d active destinations"), activeDestinations.Num());
		for (int i = 0; i < activeDestinations.Num(); i++)
			UE_LOG(LogFMRI, Log, TEXT("Active destination id %d"), i, activeDestinations[i]);
		return true;
	}
}

bool NavigationParser::ParseActiveDestinations(const TArray<NavigationDestination> &destinations, const TMap<EDestinationSet, bool> &activeSets, TArray<int> &activeDestinations, bool overwrite)
{
	if (overwrite)
	{
		UE_LOG(LogFMRI, Warning, TEXT("Non-empty active destinations array will be overwritten"));
		activeDestinations.Empty();
	}
	// check if no set is marked as active
	// in that case, all are active
	bool areAllSetsInactive = true;
	if (activeSets.Num() > 0)
		for (auto& element : activeSets)
			if (element.Value)
			{
				areAllSetsInactive = false;
				break;
			}
	for (int i = 0; i < destinations.Num(); i++)
	{
		if (areAllSetsInactive)	// all inactive is the same as all active
			activeDestinations.Add(destinations[i].GetID());
		else
		{
			if (destinations[i].GetDestinationSets()->Num() > 0)
				for (EDestinationSet destinationSet : *destinations[i].GetDestinationSets())
					if (activeSets[destinationSet])	// if any of this destination's destination sets is marked active
					{
						activeDestinations.Add(destinations[i].GetID());
						break;
					}
		}
	}
	UE_LOG(LogFMRI, Log, TEXT("%d active destinations"), activeDestinations.Num());
	for (int i = 0; i < activeDestinations.Num(); i++)
		UE_LOG(LogFMRI, Log, TEXT("Active destination id %d"), i, activeDestinations[i]);

	return true;
}

bool NavigationParser::ParseActiveDestinations(const TArray<NavigationDestination> &destinations, const TMap<EDestinationSet, bool> &activeSets, TArray<int> &activeDestinations)
{
	return ParseActiveDestinations(destinations, activeSets, activeDestinations, true);
}
