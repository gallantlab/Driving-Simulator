// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

// This file is included before any other file in every compile unit within the
// plugin.
#pragma once

#include "ModuleManager.h"
#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "Util/NonCopyable.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCarla, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCarlaServer, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogFMRI, Log, All);

// Options to compile with extra debug log.
#if WITH_EDITOR
// #define CARLA_AI_VEHICLES_EXTRA_LOG
// #define CARLA_AI_WALKERS_EXTRA_LOG
// #define CARLA_ROAD_GENERATOR_EXTRA_LOG
// #define CARLA_SERVER_EXTRA_LOG
// #define CARLA_TAGGER_EXTRA_LOG
#endif // WITH_EDITOR

class FCarlaModule : public IModuleInterface
{
	void RegisterSettings();
	void UnregisterSettings();
	bool HandleSettingsSaved();


public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};

// globally available tags
static FName PlayerTag = FName(TEXT("Player"));
static FName NPCTag = FName(TEXT("NPC"));


#define SHOW_POSITION	// show current location in HUD?
#define NAV				// navigation experiment stuff??
//#define VERBOSE_LOG
//#define NO_CAPTURE_FRAMES	// debugging for replay things - don't save frames