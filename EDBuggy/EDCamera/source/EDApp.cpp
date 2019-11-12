#include "EDApp.h"

#include <windows.h>

#include "gl.h"
#include "glu.h"
#include "glut.h"
#include "extgl.h"

#include "EDDemo.h"
#include "BMPFile.h"

#include "EDCommon.h"

#ifndef FLT_EPSILON
#define FLT_EPSILON     1.192092896e-07F
#endif


void EDApp::Drive(float fTime)
{
	matrix4f &BuggyMat = m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat();
	
	///////// GROUND CLAMPING //////////////////////////
	vec3f offset = vec3f(0.0f, 5.0f, 0.0f);
	vec3f sPoint, ePoint;
	sPoint = m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_pos + offset;
	ePoint = m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_pos - offset;

	EDTriangle *tris;
	unsigned int size;
	
	GetTriangles(&tris, &size, sPoint, ePoint);
	vec3f intersection;
	unsigned int index;

	if (LineSegment2Triangle(intersection, index, tris, size, sPoint, ePoint))
	{
		m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_pos = intersection;
	}
	//////////// END GROUND CLAMP /////////////////////

	/////////// SMOOTH TERRAIN FOLLOWING /////////////////////
	EDTriangle *FLTris, *FRTris, *CTRTris;
	vec3f FLintersection, FRintersection, CTRintersection;
	unsigned int FLSize, FRSize, CTRSize;
	vec3f FLTireStart, FLTireEnd, FRTireStart, FRTireEnd, CTRTireStart, CTRTireEnd, RearTireAVG;

	// Create line segment for Front Left Tire
	FLTireStart = m_Buggy.m_Frames[CBuggy::FLWHEEL].GetWorldMat().axis_pos + offset;
	FLTireEnd = m_Buggy.m_Frames[CBuggy::FLWHEEL].GetWorldMat().axis_pos - offset;
	
	// Create line segment for Front Right Tire
	FRTireStart = m_Buggy.m_Frames[CBuggy::FRWHEEL].GetWorldMat().axis_pos + offset;
	FRTireEnd = m_Buggy.m_Frames[CBuggy::FRWHEEL].GetWorldMat().axis_pos - offset;

	// Create line segment for Average of Rear Tires
	RearTireAVG = (m_Buggy.m_Frames[CBuggy::BRWHEEL].GetWorldMat().axis_pos + m_Buggy.m_Frames[CBuggy::BLWHEEL].GetWorldMat().axis_pos) * 0.5f;
	CTRTireStart = RearTireAVG + offset;
	CTRTireEnd = RearTireAVG - offset;

	GetTriangles(&FLTris, &FLSize, FLTireStart, FLTireEnd);
	GetTriangles(&FRTris, &FRSize, FRTireStart, FRTireEnd);
	GetTriangles(&CTRTris, &CTRSize, CTRTireStart, CTRTireEnd);

	// check each wheel for collision
	LineSegment2Triangle(FLintersection, index, FLTris, FLSize, FLTireStart, FLTireEnd);
	LineSegment2Triangle(FRintersection, index, FRTris, FRSize, FRTireStart, FRTireEnd);
	LineSegment2Triangle(CTRintersection, index, CTRTris, CTRSize, CTRTireStart, CTRTireEnd);

	vec3f tempZ;
	vec3f newFL = m_Buggy.m_Frames[CBuggy::FLWHEEL].GetWorldMat().axis_pos + (FLintersection - m_Buggy.m_Frames[CBuggy::FLWHEEL].GetWorldMat().axis_pos) * 0.01f;
	vec3f newFR = m_Buggy.m_Frames[CBuggy::FRWHEEL].GetWorldMat().axis_pos + (FRintersection - m_Buggy.m_Frames[CBuggy::FRWHEEL].GetWorldMat().axis_pos) * 0.01f;
	vec3f newCTR = RearTireAVG + (CTRintersection - RearTireAVG) * 0.01f;

	m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_x = (newFL - newFR).normalize();
	tempZ = (newFR - newCTR).normalize();
	
	cross_product(m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_y, tempZ, m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_x);
	m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_y.normalize();
	
	cross_product(m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_z, m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_x, m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_y);
	m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_z.normalize();

	//////////// END TERRAIN FOLLOWING ///////////////////
	if (m_Camera.GetCameraMode() != EDCamera::MOUSE_LOOK && GetAsyncKeyState(VK_RBUTTON))
	{
		glutSetCursor(GLUT_CURSOR_NONE);
		MouseLook(m_Buggy.m_Frames[CBuggy::GUN].GetLocalMat(), fTime);
	}
	else
	{
		glutSetCursor(GLUT_CURSOR_INHERIT);
	}

	// Move the buggy forward along it's Z-Axis
	if (GetAsyncKeyState('W'))
	{
		BuggyMat.axis_pos += BuggyMat.axis_z * fTime;
		m_turnRate = dot_product(m_Buggy.m_Frames[CBuggy::BODY].GetWorldMat().axis_x, m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_z);
		m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().rotate_y_pre(m_turnRate * fTime);
		m_Buggy.m_Frames[CBuggy::GUN].GetLocalMat().rotate_y_pre(-m_turnRate * fTime);
	}

	// Move the buggy backward along it's Z-Axis
	if (GetAsyncKeyState('S'))
	{
		BuggyMat.axis_pos += BuggyMat.axis_z * -fTime;
		m_turnRate = dot_product(m_Buggy.m_Frames[CBuggy::BODY].GetWorldMat().axis_x, m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_z);
		m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().rotate_y_pre(m_turnRate * fTime);
		m_Buggy.m_Frames[CBuggy::GUN].GetLocalMat().rotate_y_pre(-m_turnRate * fTime);
	}

	// We moved the Buggy, so update it's frame.
	m_Buggy.m_Frames[CBuggy::BODY].Update();

	// Fire a mortar...
	if (GetAsyncKeyState(VK_SPACE) & 0x0001)
	{
		for (unsigned int i = 0; i < 20; ++i)
		{
			if (!m_Mortars[i].IsAlive())
			{
				matrix4f temp = m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat();
				temp.axis_pos += temp.axis_y * 0.075f;
				temp.axis_pos += temp.axis_z * 0.25f;

				m_Mortars[i].Fire(temp);
				break;
			}
		}
	}

	// Fire a bullet...
	if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
	{
		for (unsigned int i = 0; i < 20; ++i)
		{
			if (!m_Bullets[i].IsAlive())
			{
				matrix4f temp = m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat();
				temp.axis_pos += temp.axis_y * 0.075f;
				temp.axis_pos += temp.axis_z * 0.25f;

				m_Bullets[i].Fire(temp);
				break;
			}
		}
	}
}

void EDApp::Update(void)
{
	// Get the elapsed seconds since the last frame
	float fTimeElapsed = m_Timer.GetElapsedTime() * 4.0f;
	m_Timer.Reset();

	// Update/Drive the buggy
	Drive(fTimeElapsed);

	// Clear our screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Save the identity
	glPushMatrix();

	// Update our camera
	m_Camera.Update(fTimeElapsed);
	m_Camera.ApplyCameraTransform();

	// Set LIGHT0's position
	float fPos[4] = { 0.707f, 0.707f, 0.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, fPos);

	// Render the buggy
	glBindTexture(GL_TEXTURE_2D, m_Buggy.m_uiTexID);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	m_Buggy.Render();
	glDisable(GL_TEXTURE_2D);

#if 1 // Render the terrain or don't render the terrain
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, m_TerTex);
	glColor3f(1.0f, 1.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);

	glVertexPointer(3, GL_FLOAT, 0, m_verts[0].v);
	glNormalPointer(GL_FLOAT, 0, m_norms[0].v);
	glTexCoordPointer(2, GL_FLOAT, 0, &m_uvs[0].s);

	glDrawArrays(GL_TRIANGLES, 0, m_verts.size());
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
#endif

	// UPDATE TARGETS
	for (unsigned int i = 0; i < 20; ++i)
	{
		m_Targets[i].Update(fTimeElapsed);
		m_Targets[i].Render();
	}

	// Draw the world's coordinate axes
	glDisable(GL_LIGHTING);
	glDepthMask(0);
	glColor3f(0.0f, 0.5f, 0.0f);
	DrawGround();
	glDepthMask(1);
	DrawAxes();
	glEnable(GL_LIGHTING);

	// UPDATE THE CROSSHAIR
	m_Crosshair.Update(fTimeElapsed);
	m_Crosshair.Render();

	// UPDATE ACTIVE MORTARS
	for (unsigned int i = 0; i < 20; ++i)
	{
		if (m_Mortars[i].IsAlive())
		{
			m_Mortars[i].Update(fTimeElapsed);
			m_Mortars[i].Render();
		}
	}

	// UPDATE ACTIVE EXPLOSIONS
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDepthMask(0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_BullTex);

	for (unsigned int i = 0; i < 20; ++i)
	{
		if (m_Explosions[i].IsAlive())
		{
			m_Explosions[i].Update(fTimeElapsed);
			m_Explosions[i].Render();
		}
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
	glDepthMask(1);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDepthMask(0);

	// UPDATE ACTIVE BULLETS
	glBindTexture(GL_TEXTURE_2D, m_BullTex);
	for (unsigned int i = 0; i < 20; ++i)
	{
		if (m_Bullets[i].IsAlive())
		{
			m_Bullets[i].Update(fTimeElapsed);
			m_Bullets[i].Render();
		}
	}
	glEnable(GL_LIGHTING);
	glDepthMask(1);
	glDisable(GL_TEXTURE_2D);

	// Restore the identity
	glPopMatrix();

	// Swap the buffer
	glutSwapBuffers();

	// Tell glut to render again
	glutPostRedisplay();
}

void EDApp::Initialize(void)
{
	BMPFile buggyTex("jeepbmp.BMP");
	glGenTextures(1, &m_Buggy.m_uiTexID);
	glBindTexture(GL_TEXTURE_2D, m_Buggy.m_uiTexID);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, (GLsizei)buggyTex.GetWidth(), (GLsizei)buggyTex.GetHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, buggyTex.GetPixelData());

	BMPFile terrainTex("tempgrass2.bmp");
	glGenTextures(1, &m_TerTex);
	glBindTexture(GL_TEXTURE_2D, m_TerTex);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, (GLsizei)terrainTex.GetWidth(), (GLsizei)terrainTex.GetHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, terrainTex.GetPixelData());

	BMPFile bullTex("plasma2.bmp");
	glGenTextures(1, &m_BullTex);
	glBindTexture(GL_TEXTURE_2D, m_BullTex);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, (GLsizei)bullTex.GetWidth(), (GLsizei)bullTex.GetHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, bullTex.GetPixelData());

	size_t rowCount;

	FILE *pTerrainFile = fopen("terrain.bin", "rb");

	if (pTerrainFile)
	{
		fread(&rowCount, sizeof(size_t), 1, pTerrainFile);

		m_verts.resize(rowCount);
		m_norms.resize(rowCount);
		m_uvs.resize(rowCount);

		fread(m_verts[0].v, sizeof(vec3f), rowCount, pTerrainFile);
		fread(m_norms[0].v, sizeof(vec3f), rowCount, pTerrainFile);
		fread(&m_uvs[0].s, sizeof(uv), rowCount, pTerrainFile);

		fclose(pTerrainFile);
	}

	vec3f highest = m_verts[0];
	for (unsigned int i = 0; i < m_verts.size(); ++i)
	{
		if (m_verts[i].x > highest.x)
			highest.x = m_verts[i].x;
		if (m_verts[i].y > highest.y)
			highest.y = m_verts[i].y;
		if (m_verts[i].z > highest.z)
			highest.z = m_verts[i].z;
	}

	highest *= 0.5f;
	for (unsigned int i = 0; i < m_verts.size(); ++i)
	{
		m_verts[i] -= highest;
	}


	m_Buggy.m_Frames[CBuggy::BODY].GetLocalMat().axis_pos = vec3f(-1.5f, 1.5f, -1.5f);

	m_Buggy.m_Frames[CBuggy::BODY].Update();


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	srand(GetTickCount());

	for (unsigned int i = 0; i < 20; ++i)
	{
		size_t posIndex = rand() % m_verts.size();

		m_Targets[i].m_Matrix.axis_pos = m_verts[posIndex];
		m_Targets[i].m_Matrix.axis_pos.y += 2.5f;
	}

	m_Timer.Reset();

	m_pBVTree = new EDBVTree(&m_verts[0], (unsigned int)m_verts.size(), 10);
}

bool AABB2LineSegment(const EDAABB &box, const vec3f& startPoint, const vec3f& endPoint)
{
	vec3f c = (box.GetMin() + box.GetMax()) * 0.5f;
	vec3f e = box.GetMax() - box.GetMin();
	vec3f d = endPoint - startPoint;
	vec3f m = startPoint + endPoint - box.GetMin() - box.GetMax();

	float adx = abs(d.x);
	if (abs(m.x) > e.x + adx) return false;

	float ady = abs(d.y);
	if (abs(m.y) > e.y + ady) return false;

	float adz = abs(d.z);
	if (abs(m.z) > e.z + adz) return false;

	adx += FLT_EPSILON;
	ady += FLT_EPSILON;
	adz += FLT_EPSILON;

	if (abs(m.y * d.z - m.z * d.y) > e.y * adz + e.z * ady) return false;
	if (abs(m.z * d.x - m.x * d.z) > e.x * adz + e.z * adx) return false;
	if (abs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) return false;

	return true;
}

bool AABB2LineSegmentTraverse(EDAABB *pBV, void *pVoid)
{
	vec3f* pPoints = (vec3f*)pVoid;

	if (AABB2LineSegment(*pBV, pPoints[0], pPoints[1]))
		return true;

	return false;
}

void EDApp::GetTriangles(EDTriangle **pTris, unsigned int *pTriCount, const vec3f &startPoint, const vec3f &endPoint)
{
	vec3f points[2];
	points[0] = startPoint;
	points[1] = endPoint;

	EDApp::GetInstance().m_pBVTree->Traverse(AABB2LineSegmentTraverse, points, pTris, pTriCount);
}

void EDApp::Shutdown(void)
{
	glDeleteTextures(1, &m_Buggy.m_uiTexID);

	delete m_pBVTree;
}

bool EDApp::LineSegment2Triangle(vec3f &vOut, unsigned int &uiTriIndex, EDTriangle *pTris, unsigned int uiTriCount, const vec3f &vStart, const vec3f &vEnd)
{
	/*Reminder: Find the NEAREST interesecting triangle*/
	vec3f end = vEnd;
	bool col = false;
	for (unsigned int i = 0; i < uiTriCount; ++i)
	{
		vec3f line = end - vStart;
		vec3f point = pTris[i].m_Vertices[1];

		// Check to see if the line is below the plane. If the line is completely below the plane then return no collision
		if (dot_product(vStart, pTris[i].m_Normal) - dot_product(point, pTris[i].m_Normal) < 0 &&
			dot_product(end, pTris[i].m_Normal) - dot_product(point, pTris[i].m_Normal) < 0)
		{
			continue;
		}

		// Check to see if the line is above the plane. If line is completely above the plane then return no collision
		if (dot_product(vStart, pTris[i].m_Normal) - dot_product(point, pTris[i].m_Normal) > 0 &&
			dot_product(end, pTris[i].m_Normal) - dot_product(point, pTris[i].m_Normal) > 0)
		{
			continue;
		}

		float distance = -(dot_product(pTris[i].m_Normal, vStart) - dot_product(pTris[i].m_Normal, point)) / dot_product(pTris[i].m_Normal, line);

		vec3f colPT;
		colPT.x = distance * line.x;
		colPT.y = distance * line.y;
		colPT.z = distance * line.z;

		colPT = vStart + colPT;

		vec3f edge[3];
		vec3f norm[3];
		float results[3];

		edge[0] = pTris[i].m_Vertices[1] - pTris[i].m_Vertices[0];
		edge[1] = pTris[i].m_Vertices[2] - pTris[i].m_Vertices[1];
		edge[2] = pTris[i].m_Vertices[0] - pTris[i].m_Vertices[2];

		for (unsigned int j = 0; j < 3; ++j)
		{
			cross_product(norm[j], edge[j], pTris[i].m_Normal);
			vec3f tempVert = colPT - pTris[i].m_Vertices[j];
			// check to see if the collision point is outside of the triangle. If so, return no collision
			results[j] = dot_product(norm[j], tempVert);
		}

		if (results[0] > 0 || results[1] > 0 || results[2] > 0)
		{
			continue;
		}
		end = vOut = colPT;
		uiTriIndex = i;
		col = true;
	}
	return col;
}