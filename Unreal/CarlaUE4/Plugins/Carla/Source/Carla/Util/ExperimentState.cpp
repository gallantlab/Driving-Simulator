// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ExperimentState.h"


ExperimentState::ExperimentState(double time)
{
	this->time = time;
	entityStates = TArray<EntityState>();
	thisType = EntityType::Pedestrian;
	frame = -1;
	isTTL = false;
	isBeep = false;
	points = 0;
}

ExperimentState::ExperimentState(double time, int frame)
{
	this->time = time;
	entityStates = TArray<EntityState>();
	thisType = EntityType::Pedestrian;
	this->frame = frame;
	isTTL = false;
	speed = 0;
	throttle = 0;
	steeringAngle = 0;
	brake = 0;
	handbrake = false;
	gear = 0;
	isBeep = false;
	points = 0;
}

ExperimentState::ExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: ExperimentState(time, frame)
{
	RecordStates(agents);
}

ExperimentState::ExperimentState(double time, int frame, TArray<const UAgentComponent*> agents, ACarlaPlayerState *playerState)
		: ExperimentState(time, frame, agents)
{
	steeringAngle = playerState->GetSteer();
	isTTL = playerState->isTTL();
	speed = playerState->GetForwardSpeed();
	throttle = playerState->GetThrottle();
	brake = playerState->GetBrake();
	handbrake = playerState->GetHandBrake();
	gear = playerState->GetCurrentGear();
	autoSteer = playerState->IsAutoSteerOn();
	underPlayerControl = playerState->IsUnderPlayerControl();
	totalTTLs = playerState->GetTotalTTLs();
	desiredThrottle = playerState->GetDesiredThrottle();
	displayedPromptType = Cast<AMRIPlayerState>(playerState)->GetDisplayedPromptType();
	isBeep = playerState->IsBeep();
	points = playerState->GetPoints();
}

ExperimentState::~ExperimentState()
{
}

void ExperimentState::RecordStates(TArray<const UAgentComponent*> agents)
{
	const UAgentComponent* agent;
	UE_LOG(LogFMRI, Log, TEXT("%d agents"), agents.Num());
	for (int i = 0; i < agents.Num(); i++)
	{
		agent = agents[i];
		agent->AcceptVisitor(*this);
		entityStates.Add(EntityState(agent->GetComponentLocation(), agent->GetComponentRotation(), thisType, agent->GetId()));
	}
}

void ExperimentState::Visit(const UTrafficSignAgentComponent & agent)
{
	thisType = EntityType::RoadSign;
}

void ExperimentState::Visit(const UVehicleAgentComponent & agent)
{
	thisType = EntityType::Vehicle;
}

void ExperimentState::Visit(const UWalkerAgentComponent& agent)
{
	thisType = EntityType::Pedestrian;
}

void ExperimentState::WriteToFile(std::ofstream& logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("Experiment state write frame"));
	WRITE_INDENTS(logFile, indentation)
	logFile << "<Frame Number=" << WRITE_ATTRIBUTE(frame) << " Time=" << WRITE_ATTRIBUTE(time)
			<< " TTL=" << WRITE_ATTRIBUTE(isTTL) << " TotalTTL=" << WRITE_ATTRIBUTE(totalTTLs)
			<< " Beep=" <<  WRITE_ATTRIBUTE(isBeep) << WRITE_ATTRIBUTE_KV("Points", points) << '>' << endl;
	indentation++;
	WriteContents(logFile, indentation);
	indentation--;
	WRITE_INDENTS(logFile, indentation)
	logFile << "</Frame>" << endl;
}

void ExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("Experiment state write content"));
	// log player vehicle information
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Player" << WRITE_ATTRIBUTE_KV("Speed", speed)
			<< WRITE_ATTRIBUTE_KV("Throttle", throttle)
			<< WRITE_ATTRIBUTE_KV("DesiredThrottle", desiredThrottle)
			<< WRITE_ATTRIBUTE_KV("Steering", steeringAngle)
			<< WRITE_ATTRIBUTE_KV("Brake", brake)
			<< WRITE_ATTRIBUTE_KV("Gear", gear)
			<< WRITE_ATTRIBUTE_KV("Handbrake", handbrake)
			<< WRITE_ATTRIBUTE_KV("AutoSteer", autoSteer)
			<< WRITE_ATTRIBUTE_KV("PlayerControl", underPlayerControl)
			<< WRITE_ATTRIBUTE_KV("Prompt", (int)displayedPromptType)
			<< "/>" << endl;

	EntityState* entityState;
	for (int entity = 0; entity < entityStates.Num(); entity++)
	{
		entityState = &(entityStates[entity]);
		if (entityState->type == EntityType::RoadSign)
			continue;

		WRITE_INDENTS(logFile, indentation)

		logFile << "<Entity Type=";
		switch (entityState->type)
		{
			case EntityType::Vehicle:
				logFile << WRITE_ATTRIBUTE("Vehicle");
				break;
			case EntityType::Pedestrian:
				logFile << WRITE_ATTRIBUTE("Pedestrian");
				break;
			default:
				logFile << WRITE_ATTRIBUTE("Unknown");
				break;
		}
		logFile << " ID=" << WRITE_ATTRIBUTE(entityState->ID) << '>' << endl;
		indentation++;
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Position>" << entityState->position.X << ',' << entityState->position.Y << ',' << entityState->position.Z << "</Position>" << endl;
		WRITE_INDENTS(logFile, indentation)
		logFile << "<Rotation>" << entityState->rotation.Pitch << ',' << entityState->rotation.Roll << ',' << entityState->rotation.Yaw << "</Rotation>" << endl;
		indentation--;
		WRITE_INDENTS(logFile, indentation)
		logFile << "</Entity>" << endl;
	}
}