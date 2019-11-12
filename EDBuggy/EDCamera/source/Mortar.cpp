#include <windows.h>

#include "gl.h"
#include "glu.h"
#include "glut.h"
#include "extgl.h"

#include "EDApp.h"
#include "EDCommon.h"

#include "Mortar.h"

unsigned int CMortar::m_uiRefCount = 0;
unsigned int CMortar::m_uiDrawList = 0;

void CMortar::Update( float fTime )
{
	m_fAge += fTime;
	m_fTrailTimer += fTime;
	vec3f offset = vec3f(0.0f, 5.0f, 0.0f);
	vec3f sPoint, ePoint;
	sPoint = EDApp::GetInstance().m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_pos;
	ePoint = m_Matrix.axis_pos;

	EDTriangle *tris;
	unsigned int size;

	EDApp::GetInstance().GetTriangles(&tris, &size, sPoint, ePoint);
	vec3f intersection;
	unsigned int index;

	// Update the mortar trail...
	if( m_fTrailTimer >= 0.0666f )
	{
		memmove( m_trail, &m_trail[1], sizeof(vec3f) * (TRAILLEN-1) );
		m_trail[TRAILLEN-1] = m_Matrix.axis_pos;
		m_fTrailTimer = 0.0f;
	}

	if (m_fAge > 60 || EDApp::GetInstance().LineSegment2Triangle(intersection, index, tris, size, sPoint, ePoint))
	{
		Kill();
	}

	m_Matrix.axis_pos += m_fVelocity * fTime;
	m_fVelocity.y -= 0.25f * fTime;

	for (unsigned int i = 0; i < 20; ++i)
	{
		CTarget &target = EDApp::GetInstance().m_Targets[i];
		vec3f cur = target.GetPosition() - m_Matrix.axis_pos;

		if (pow(m_fRadius + target.GetRadius(), 2) > dot_product(cur, cur))
		{
			Kill();
			target.Spin();
		}
	}
}

CMortar::CMortar(void)
{
	m_fRadius = 0.25f;
	m_fAge = 0.0f;
	m_fTrailTimer = 0.0f;

	if( m_uiRefCount == 0 )
	{
		m_uiDrawList = glGenLists(1);

		GLUquadric *pQuad = gluNewQuadric();
		glNewList( m_uiDrawList, GL_COMPILE );

			glPushMatrix();
				
				glPushMatrix();
					glTranslatef( 0.0f, 0.0f, -0.0625f );
					glColor3f( 0.75f, 0.75f, 0.0f );
					gluCylinder( pQuad, 0.0125f, 0.0125f, 0.125f, 15, 15 );
				glPopMatrix();
				
				glPushMatrix();
					glTranslatef( 0.0f, 0.0f, 0.0625f );
					glColor3f( 0.25f, 0.25f, 0.25f );
					gluSphere( pQuad, 0.0125f, 15, 15 );
				glPopMatrix();

				glPushMatrix();
					glTranslatef( 0.0f, 0.0f, -0.09375f );
					glColor3f( 0.75f, 0.75f, 0.0f );
					gluCylinder( pQuad, 0.00625f, 0.0125f, 0.03125f, 15, 15 );
				glPopMatrix();


				glPushMatrix();
					glTranslatef( 0.0f, 0.0f, -0.125f );
					gluCylinder( pQuad, 0.0125f, 0.00625f, 0.03125f, 15, 15 );

					glRotatef( 180.0f, 0.0f, 1.0f, 0.0f );
					glColor3f( 0.5f, 0.0f, 0.0f );
					gluDisk( pQuad, 0.0f, 0.0125f, 15, 15 );
				glPopMatrix();
				
			glPopMatrix();

		glEndList();
		gluDeleteQuadric(pQuad);
	}

	++m_uiRefCount;
	m_Matrix.make_identity();
}

CMortar::~CMortar()
{
	--m_uiRefCount;

	if( m_uiRefCount == 0 )
	{
		glDeleteLists( m_uiDrawList, 1 );
		m_uiDrawList = 0;
	}
}

void CMortar::Render(void)
{
	glPushMatrix();
		glMultMatrixf( m_Matrix.ma );
		glCallList( m_uiDrawList );
	glPopMatrix();

	glDisable( GL_LIGHTING );
	glDepthMask( 0 );

	glLineWidth( 2.0f );
	glBegin( GL_LINE_STRIP );
		for( unsigned int i = 0; i < TRAILLEN; ++i )
		{
			float fTemp = i/(float)TRAILLEN;

			glColor4f( 1.0f, fTemp*fTemp, fTemp*fTemp*fTemp*fTemp, fTemp );
			glVertex3fv( m_trail[i].v );
		}
	glEnd();

	glEnable( GL_LIGHTING );
	glDepthMask( 1 );
	glLineWidth( 1.0f );
}

void CMortar::Kill(void)
{
	for( unsigned int i = 0; i < 20; ++i )
	{
		if( !EDApp::GetInstance().m_Explosions[i].IsAlive() )
		{
			EDApp::GetInstance().m_Explosions[i].Explode( m_Matrix );
			break;
		}
	}

	m_fAge = 0.0f;
	m_fTrailTimer = 0.0f;
	m_bAliveFlag = false;
}

void CMortar::Fire( const matrix4f &attachMat )
{
	m_Matrix = attachMat;
	m_bAliveFlag = true;
	m_fVelocity = EDApp::GetInstance().m_Buggy.m_Frames[CBuggy::GUN].GetWorldMat().axis_z * 10;
	for( unsigned int i = 0; i < TRAILLEN; ++i )
		m_trail[i] = m_Matrix.axis_pos;
}