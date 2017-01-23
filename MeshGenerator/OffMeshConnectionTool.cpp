//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "OffMeshConnectionTool.h"
#include "InputGeom.h"
#include "Sample.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"

#include "../NavMeshData.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include "imgui.h"
#include "imgui_custom/imgui_user.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

OffMeshConnectionTool::OffMeshConnectionTool()
{
}

OffMeshConnectionTool::~OffMeshConnectionTool()
{
	if (m_sample)
	{
		m_sample->setNavMeshDrawFlags(m_oldFlags);
	}
}

void OffMeshConnectionTool::init(Sample* sample)
{
	if (m_sample != sample)
	{
		m_sample = sample;
		m_oldFlags = m_sample->getNavMeshDrawFlags();
		m_sample->setNavMeshDrawFlags(m_oldFlags & ~DU_DRAWNAVMESH_OFFMESHCONS);
	}
}

void OffMeshConnectionTool::reset()
{
	m_hitPosSet = false;
}

void OffMeshConnectionTool::handleMenu()
{
	if (ImGui::RadioButton("One Way", !m_bidir))
		m_bidir = false;
	if (ImGui::RadioButton("Bidirectional", m_bidir))
		m_bidir = true;
}

void OffMeshConnectionTool::handleClick(const glm::vec3& s, const glm::vec3& p, bool shift)
{
	if (!m_sample) return;
	InputGeom* geom = m_sample->getInputGeom();
	if (!geom) return;

	if (shift)
	{
		// Delete
		// Find nearest link end-point
		float nearestDist = FLT_MAX;
		int nearestIndex = -1;
		const float* verts = geom->getOffMeshConnectionVerts();
		for (int i = 0; i < geom->getOffMeshConnectionCount()*2; ++i)
		{
			const float* v = &verts[i*3];
			float d = rcVdistSqr(glm::value_ptr(p), v);
			if (d < nearestDist)
			{
				nearestDist = d;
				nearestIndex = i/2; // Each link has two vertices.
			}
		}
		// If end point close enough, delete it.
		if (nearestIndex != -1 &&
			sqrtf(nearestDist) < m_sample->getAgentRadius())
		{
			geom->deleteOffMeshConnection(nearestIndex);
		}
	}
	else
	{
		// Create
		if (!m_hitPosSet)
		{
			m_hitPos = p;
			m_hitPosSet = true;
		}
		else
		{
			const uint8_t area = PolyArea::Jump;
			const uint16_t flags = PolyFlags::Jump;
			geom->addOffMeshConnection(glm::value_ptr(m_hitPos), glm::value_ptr(p),
				m_sample->getAgentRadius(), m_bidir ? 1 : 0, area, flags);
			m_hitPosSet = false;
		}
	}
}

void OffMeshConnectionTool::handleToggle()
{
}

void OffMeshConnectionTool::handleStep()
{
}

void OffMeshConnectionTool::handleUpdate(float /*dt*/)
{
}

void OffMeshConnectionTool::handleRender()
{
	DebugDrawGL dd;
	const float s = m_sample->getAgentRadius();
	
	if (m_hitPosSet)
		duDebugDrawCross(&dd, m_hitPos[0],m_hitPos[1]+0.1f,m_hitPos[2], s, duRGBA(0,0,0,128), 2.0f);

	InputGeom* geom = m_sample->getInputGeom();
	if (geom)
		geom->drawOffMeshConnections(&dd, true);
}

void OffMeshConnectionTool::handleRenderOverlay(const glm::mat4& proj,
	const glm::mat4& model, const glm::ivec4& view)
{
	// Draw start and end point labels
	if (m_hitPosSet)
	{
		glm::vec3 pos = glm::project(m_hitPos, model, proj, view);

		ImGui::RenderText((int)pos.x + 5, -((int)pos.y - 5), ImVec4(0, 0, 0, 220), "Start");
	}
	
	// Tool help
	if (!m_hitPosSet)
	{
		ImGui::RenderTextRight(-330, -(view[3] - 40), ImVec4(255, 255, 255, 192),
			"LMB: Create new connection.  SHIFT+LMB: Delete existing connection, click close to start or end point.");
	}
	else
	{
		ImGui::RenderTextRight(-330, -(view[3] - 40), ImVec4(255, 255, 255, 192),
			"LMB: Set connection end point and finish.");
	}
}
