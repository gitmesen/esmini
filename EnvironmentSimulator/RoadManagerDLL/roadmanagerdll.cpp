/*
 * esmini - Environment Simulator Minimalistic
 * https://github.com/esmini/esmini
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) partners of Simulation Scenarios
 * https://sites.google.com/view/simulationscenarios
 */

#include "roadmanagerdll.hpp"
#include "RoadManager.hpp"
#include "CommonMini.hpp"

using namespace roadmanager;

static roadmanager::OpenDrive *odrManager = 0;
static std::vector<Position> position;

static int GetSteeringTarget(int index, float lookahead_distance, RM_SteeringTargetInfo *r_data, bool along_reference_lane)
{
	roadmanager::SteeringTargetInfo s_data;

	if (odrManager == 0)
	{
		return -1;
	}

	if (index >= position.size())
	{
		LOG("Object %d not available, only %d registered", index, position.size());
		return -1;
	}

	if (position[index].GetSteeringTargetInfo(lookahead_distance, &s_data, along_reference_lane) != 0)
	{
		return -1;
	}
	else
	{
		// Copy data
		r_data->local_pos[0] = (float)s_data.local_pos[0];
		r_data->local_pos[1] = (float)s_data.local_pos[1];
		r_data->local_pos[2] = (float)s_data.local_pos[2];
		r_data->global_pos[0] = (float)s_data.global_pos[0];
		r_data->global_pos[1] = (float)s_data.global_pos[1];
		r_data->global_pos[2] = (float)s_data.global_pos[2];
		r_data->angle = (float)s_data.angle;
		r_data->curvature = (float)s_data.curvature;
		r_data->road_heading = (float)s_data.road_heading;
		r_data->road_pitch = (float)s_data.road_pitch;
		r_data->road_roll = (float)s_data.road_roll;
		r_data->speed_limit = (float)s_data.speed_limit;

		return 0;
	}
}

static int GetRoadLaneInfo(int index, float lookahead_distance, RM_RoadLaneInfo *r_data)
{
	roadmanager::RoadLaneInfo s_data;

	if (odrManager == 0)
	{
		return -1;
	}

	if (index >= position.size())
	{
		LOG("Object %d not available, only %d registered", index, position.size());
		return -1;
	}

	roadmanager::Position pos = position[index];

	pos.GetRoadLaneInfo(lookahead_distance, &s_data);

	r_data->pos[0] = (float)pos.GetX();
	r_data->pos[1] = (float)pos.GetY();
	r_data->pos[2] = (float)pos.GetZ();
	r_data->heading = (float)s_data.heading;
	r_data->pitch = (float)s_data.pitch;
	r_data->roll = (float)s_data.roll;
	r_data->width = (float)s_data.width;
	r_data->curvature = (float)s_data.curvature;
	r_data->speed_limit = (float)s_data.speed_limit;

	return 0;
}

extern "C"
{
	RM_DLL_API int RM_Init(const char *odrFilename)
	{
		if (odrManager)
		{
			RM_Close();
		}

		if (!roadmanager::Position::LoadOpenDrive(odrFilename))
		{
			printf("Failed to load ODR %s\n", odrFilename);
			return -1;
		}
		odrManager = roadmanager::Position::GetOpenDrive();

		return 0;
	}

	RM_DLL_API int RM_Close()
	{
		position.clear();

		return 0;
	}
	
	RM_DLL_API int RM_CreatePosition()
	{
		roadmanager::Position newPosition;
		position.push_back(newPosition);
		return (int)(position.size() - 1);  // return index of newly created 
	}

	RM_DLL_API int RM_GetNumberOfRoads()
	{
		if (odrManager)
		{
			return odrManager->GetNumOfRoads();
		}
		else
		{
			return 0;
		}
	}

	RM_DLL_API int RM_GetIdOfRoadFromIndex(int index)
	{
		if (odrManager)
		{
			return odrManager->GetRoadByIdx(index)->GetId();
		}
		else
		{
			return -1;
		}
	}

	RM_DLL_API float RM_GetRoadLength(int id)
	{
		if (odrManager)
		{
			return (float)(odrManager->GetRoadById(id)->GetLength());
		}
		else
		{
			return 0.0f;
		}
	}

	RM_DLL_API int RM_GetRoadNumberOfLanes(int roadId, float s)
	{
		int numberOfDrivableLanes = 0;

		if (odrManager)
		{
			roadmanager::Road *road = odrManager->GetRoadById(roadId);

			// Consider only drivable lanes
			roadmanager::LaneSection *laneSection = road->GetLaneSectionByS(s);
			for (size_t i = 0; i < laneSection->GetNumberOfLanes(); i++)
			{
				if (laneSection->GetLaneByIdx((int)i)->IsDriving())
				{
					numberOfDrivableLanes++;
				}
			}
		}

		return numberOfDrivableLanes;
	}

	RM_DLL_API int RM_GetLaneIdByIndex(int roadId, int laneIndex, float s)
	{
		int numberOfDrivableLanes = 0;

		if (odrManager)
		{
			roadmanager::Road *road = odrManager->GetRoadById(roadId);

			// Consider only drivable lanes
			roadmanager::LaneSection *laneSection = road->GetLaneSectionByS(s);
			for (size_t i = 0; i < laneSection->GetNumberOfLanes(); i++)
			{
				if (laneSection->GetLaneByIdx((int)i)->IsDriving())
				{
					if (numberOfDrivableLanes == laneIndex)
					{
						return laneSection->GetLaneByIdx((int)i)->GetId();
					}
					else
					{
						numberOfDrivableLanes++;
					}
				}
			}
		}
		return 0;
	}
		
	RM_DLL_API int RM_SetLanePosition(int handle, int roadId, int laneId, float laneOffset, float s, bool align)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			roadmanager::Position *pos = &position[handle];
			pos->SetLanePos(roadId, laneId, s, laneOffset);

			if (align)
			{
				if (laneId < 0)
				{
					pos->SetHeadingRelative(0);
				}
				else
				{
					pos->SetHeadingRelative(M_PI);
				}
			}
		}

		return 0;
	}

	RM_DLL_API int RM_SetWorldPosition(int handle, float x, float y, float z, float h, float p, float r)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			roadmanager::Position *pos = &position[handle];
			pos->SetInertiaPos(x, y, z, h, p, r);
		}

		return 0;
	}

	RM_DLL_API int RM_SetWorldXYZHPosition(int handle, float x, float y, float z, float h)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			roadmanager::Position *pos = &position[handle];
			pos->XYZH2TrackPos(x, y, z, h, true);
		}

		return 0;
	}

	RM_DLL_API int RM_SetS(int handle, float s)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			roadmanager::Position *pos = &position[handle];
			pos->SetLanePos(pos->GetTrackId(), pos->GetLaneId(), s, pos->GetOffset());
		}

		return 0;
	}

	RM_DLL_API int RM_PositionMoveForward(int handle, float dist, int strategy)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			roadmanager::Position *pos = &position[handle];
			
			return(pos->MoveAlongS(dist, 0.0, (Junction::JunctionStrategyType)strategy));
		}
	}

	RM_DLL_API int RM_GetPositionData(int handle, RM_PositionData *data)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}
		else
		{
			data->x = (float)position[handle].GetX();
			data->y = (float)position[handle].GetY();
			data->z = (float)position[handle].GetZ();
			data->h = (float)position[handle].GetH();
			data->p = (float)position[handle].GetP();
			data->r = (float)position[handle].GetR();
			data->hRelative = (float)position[handle].GetHRelative();
			data->roadId = position[handle].GetTrackId();
			data->laneId = position[handle].GetLaneId();
			data->laneOffset = (float)position[handle].GetOffset();
			data->s = (float)position[handle].GetS();
		}

		return 0;
	}

	RM_DLL_API int RM_GetLaneInfo(int handle, float lookahead_distance, RM_RoadLaneInfo *data)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}

		GetRoadLaneInfo(handle, lookahead_distance, data);

		// First find out curvature at this lateral position from reference lane
		double ref_curvature = position[handle].GetCurvature();
		double lat_offset = position[handle].GetT();
		double radius;
		Position *pos = &position[handle];
		
 		if (fabs(ref_curvature) > SMALL_NUMBER)
		{
			radius = 1.0 / ref_curvature;
			radius -= lat_offset; // curvature positive in left curves, lat_offset positive left of reference lane
			data->curvature = (float)(1.0 / radius);
		}
		else
		{
			// curvature close to zero (straight segment), radius infitite - curvature the same in all lanes
			data->curvature = (float)ref_curvature;
		}

		// Then find out the width of the lane at current s-value
		Road *road = pos->GetRoadById(pos->GetTrackId());
		data->width = (float)road->GetLaneWidthByS(pos->GetS(), pos->GetLaneId());

		data->speed_limit = (float)road->GetSpeedByS(pos->GetS());

		return 0;
	}

	RM_DLL_API float RM_GetSpeedLimit(int handle)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}

		return (float)position[handle].GetSpeedLimit();
	}

	RM_DLL_API int RM_GetSteeringTarget(int handle, float lookahead_distance, RM_SteeringTargetInfo * data, int along_reference_lane)
	{
		if (odrManager == 0 || handle >= position.size())
		{
			return -1;
		}

		if (GetSteeringTarget(handle, lookahead_distance, data, along_reference_lane) != 0)
		{
			return -1;
		}

		return 0;
	}

	RM_DLL_API bool RM_SubtractAFromB(int handleA, int handleB, RM_PositionDiff *pos_diff)
	{
		if (odrManager == 0 || handleA >= position.size() || handleB >= position.size())
		{
			return false;
		}

		double ds = 0, dt = 0;
		int dl = 0;
		bool result = position[handleA].Delta(position[handleB], ds, dt, dl);
		if (result == true)
		{
			pos_diff->ds = (float)ds;
			pos_diff->dt = (float)dt;
			pos_diff->dLaneId = dl;
		}

		return result;
	}
}
