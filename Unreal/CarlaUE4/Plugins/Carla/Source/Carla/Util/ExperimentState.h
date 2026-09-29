// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Agent/AgentComponent.h"
#include "Agent/AgentComponentVisitor.h"
#include "Game/CarlaPlayerState.h"
#include "Game/MRIPlayerState.h"

#include <iostream>
#include <fstream>


#ifndef WRITE_INDENTS
#define WRITE_INDENTS(out, indents) for (int indent = 0; indent < indents; indent++) out << '\t';
#endif

#ifndef WRITE_ATTRIBUTE
#define WRITE_ATTRIBUTE(attribute) '"' << attribute << '"'
#endif

// note this doesn't work with a ternary operator in the value
#define WRITE_ATTRIBUTE_KV(key, value) " " << key << "=\"" << value << '"'

#define WRITE_SINGLE_TAG(name, content) "<" << name << ">" << content << "</" << name << ">"

/**
 * A class to represent a snapshot of the state of the driving experiment at any time
 */

enum EntityType
{
	Vehicle,
	Pedestrian,
	RoadSign
};

struct EntityState
{
	EntityState(FVector position, FRotator rotation, EntityType type, uint32 ID)
	{
		this->position = position;
		this->rotation = rotation;
		this->type = type;
		this->ID = ID;
	}

	FVector position;
	FRotator rotation;
	EntityType type;
	uint32 ID;
};


class CARLA_API ExperimentState : IAgentComponentVisitor
{
public:
	ExperimentState(double time);

	ExperimentState(double time, int frame);

	ExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	ExperimentState(double time, int frame, TArray<const UAgentComponent*> agents, ACarlaPlayerState *playerState);

	void RecordStates(TArray<const UAgentComponent*> agents);

	double GetFrameTime() const
	{
		return time;
	}

	int GetFrame() const
	{
		return frame;
	}

	double GetSpeed() const
	{
		return speed;
	}

	double GetThrottle() const
	{
		return throttle;
	}

	double GetSteeringAngle() const
	{
		return steeringAngle;
	}

	double GetBrake() const
	{
		return brake;
	}

	bool GetHandbrake() const
	{
		return handbrake;
	}

	int GetGear() const
	{
		return gear;
	}

	int TotalTTLCount() const
	{
		return totalTTLs;
	}

	EDisplayedPromptType GetDisplayedPromptType() const
	{
		return displayedPromptType;
	}

	virtual void Visit(const UTrafficSignAgentComponent &) override;

	virtual void Visit(const UVehicleAgentComponent &) override;

	virtual void Visit(const UWalkerAgentComponent &) override;

	EntityState& operator[](int index)
	{
		return entityStates[index];
	}

	int Num() const
	{
		return entityStates.Num();
	}

	bool IsBeep() const
	{
		return isBeep;
	}

	int GetPoints() const
	{
		return points;
	}

	// writes a full self-contained tag to the out stream
	// calls WriteContents to write the insides
	// don't override
	void WriteToFile(std::ofstream& logFile, int &indentation);

	~ExperimentState();

	bool isTTL;


protected:
	// override this to add/append information that is written out
	virtual void WriteContents(std::ofstream &stream, int &indent);

private:
	double time;

	int frame;

	EntityType thisType;

	TArray<EntityState> entityStates;

	// player input information
	double speed;
	double throttle;
	double desiredThrottle;
	double steeringAngle;
	double brake;
	bool handbrake;
	int gear;
	bool autoSteer = false;
	bool underPlayerControl = true;
	int totalTTLs = 0;
	bool isBeep;
	int points = 0;

	EDisplayedPromptType displayedPromptType;
};
