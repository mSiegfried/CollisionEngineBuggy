#include <windows.h>

#include "gl.h"
#include "glu.h"
#include "glut.h"
#include "extgl.h"

#include "EDApp.h"
#include "EDCommon.h"
#include "Crosshair.h"

unsigned int CCrosshair::m_uiRefCount = 0;
unsigned int CCrosshair::m_uiDrawList = 0;

void CCrosshair::Update(float fTime)
{
	// Reset the selected target to NULL
	EDApp::GetInstance().m_pTarget = NULL;
	HardAttach(m_Matrix, EDApp::GetInstance().m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat(), vec3f(0, 0, 25));

	vec3f gun_axis_z = EDApp::GetInstance().m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_z;
	vec3f gun_pos = EDApp::GetInstance().m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_pos;
	
	vec3f offset = vec3f(0.0f, 5.0f, 0.0f);
	vec3f sPoint, ePoint;
	sPoint = gun_pos;
	ePoint = m_Matrix.axis_pos;

	EDTriangle *tris;
	unsigned int size;

	EDApp::GetInstance().GetTriangles(&tris, &size, sPoint, ePoint);
	vec3f intersection;
	unsigned int index;

	if (EDApp::GetInstance().LineSegment2Triangle(intersection, index, tris, size, sPoint, ePoint))
	{
		m_Matrix.axis_pos = intersection + m_fRadius;
	}
	for (unsigned int i = 0; i < 20; ++i)
	{
		m_fRatio = dot_product(gun_axis_z, EDApp::GetInstance().m_Targets[i].GetPosition() - gun_pos) / dot_product(gun_axis_z, gun_axis_z);

		// clamp
		if (m_fRatio < 0)
			m_fRatio = 0;

		m_closePoint = gun_pos + gun_axis_z * m_fRatio;
		m_distance = m_closePoint - EDApp::GetInstance().m_Targets[i].GetPosition();

		m_sqDistance = dot_product(m_distance, m_distance);
		m_sqRadius = pow(EDApp::GetInstance().m_Targets[i].GetRadius(), 2);

		if (m_sqDistance < m_sqRadius)
		{
			m_Matrix = EDApp::GetInstance().m_Targets[i].GetMatrix();
			EDApp::GetInstance().m_pTarget = &EDApp::GetInstance().m_Targets[i];
		}
	}
}

CCrosshair::CCrosshair(void)
{
	m_fRadius = 0.375f;

	if (m_uiRefCount == 0)
	{
		m_uiDrawList = glGenLists(1);

		GLUquadric *pQuad = gluNewQuadric();
		glNewList(m_uiDrawList, GL_COMPILE);
		// innner ring
		gluCylinder(pQuad, 0.125f, 0.125f, 0.125f, 15, 15);

		glPushMatrix();
		glTranslatef(0.0f, 0.0f, 0.125f);
		gluDisk(pQuad, 0.0f, 0.125f, 15, 15);
		glPopMatrix();
		glPushMatrix();
		glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
		gluDisk(pQuad, 0.0f, 0.125f, 15, 15);
		glPopMatrix();

		// outer ring
		gluCylinder(pQuad, 0.375f, 0.375f, 0.125f, 15, 15);
		gluCylinder(pQuad, 0.25f, 0.25f, 0.125f, 15, 15);

		glPushMatrix();
		glTranslatef(0.0f, 0.0f, 0.125f);
		gluDisk(pQuad, 0.25f, 0.375f, 15, 15);
		glPopMatrix();
		glPushMatrix();
		glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
		gluDisk(pQuad, 0.25f, 0.375f, 15, 15);
		glPopMatrix();
		glEndList();
		gluDeleteQuadric(pQuad);
	}

	++m_uiRefCount;
	m_Matrix.make_identity();
}

CCrosshair::~CCrosshair()
{
	--m_uiRefCount;

	if (m_uiRefCount == 0)
	{
		glDeleteLists(m_uiDrawList, 1);
		m_uiDrawList = 0;
	}
}

void CCrosshair::Render(void)
{
	glColor3f(0.5f, 0.5f, 0.5f);
	//glDisable( GL_LIGHTING );
	glDisable(GL_CULL_FACE);

	glPushMatrix();
	glMultMatrixf(m_Matrix.ma);
	glCallList(m_uiDrawList);
	glPopMatrix();
	//glEnable( GL_LIGHTING );
	glEnable(GL_CULL_FACE);
}

