#include "global.h"
#include "RageDisplay.h"
#include "RageTimer.h"
#include "RageLog.h"
#include "RageMath.h"
#include "RageUtil.h"
#include "RageFile.h"
#include "RageSurface_Save_BMP.h"
#include "RageSurface_Save_JPEG.h"
#include "RageSurface_Save_PNG.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageSurface.h"
#include "Preference.h"
#include "LocalizedString.h"
#include "DisplaySpec.h"
#include "arch/ArchHooks/ArchHooks.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>


// Statistics stuff
RageTimer	g_LastCheckTimer;
int		g_iNumVerts;
int		g_iFPS, g_iVPF, g_iCFPS;

int RageDisplay::GetFPS() const { return g_iFPS; }
int RageDisplay::GetVPF() const { return g_iVPF; }
int RageDisplay::GetCumFPS() const { return g_iCFPS; }

static int g_iFramesRenderedSinceLastCheck,
	   g_iFramesRenderedSinceLastReset,
	   g_iVertsRenderedSinceLastCheck,
	   g_iNumChecksSinceLastReset;
static RageTimer g_LastFrameEndedAt( RageZeroTimer );

struct Centering
{
	Centering( int iTranslateX = 0, int iTranslateY = 0, int iAddWidth = 0, int iAddHeight = 0 ):
		m_iTranslateX( iTranslateX ),
		m_iTranslateY( iTranslateY ),
		m_iAddWidth( iAddWidth ),
		m_iAddHeight( iAddHeight ) { }

	int m_iTranslateX, m_iTranslateY, m_iAddWidth, m_iAddHeight;
};

static std::vector<Centering> g_CenteringStack( 1, Centering(0, 0, 0, 0) );

RageDisplay*		DISPLAY	= nullptr; // global and accessible from anywhere in our program

Preference<bool>  LOG_FPS( "LogFPS", false );
Preference<float> g_fFrameLimitPercent( "FrameLimitPercent", 0.0f );

static const char *RagePixelFormatNames[] = {
	"RGBA8",
	"BGRA8",
	"RGBA4",
	"RGB5A1",
	"RGB5",
	"RGB8",
	"PAL",
	"BGR8",
	"A1BGR5",
	"X1RGB5",
};
XToString( RagePixelFormat );

/* bNeedReloadTextures is set to true if the device was re-created and we need
 * to reload textures.  On failure, an error message is returned.
 * XXX: the renderer itself should probably be the one to try fallback modes */
static LocalizedString SETVIDEOMODE_FAILED ( "RageDisplay", "SetVideoMode failed:" );
RString RageDisplay::SetVideoMode( VideoModeParams p, bool &bNeedReloadTextures )
{
	RString err;
	std::vector<RString> vs;

	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	LOG->Trace( "TryVideoMode failed: %s", err.c_str() );
	vs.push_back( err );

	// fall back to settings that will most likely work
	p.bpp = 16;
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	// "Intel(R) 82810E Graphics Controller" won't accept a 16 bpp surface if
	// the desktop is 32 bpp, so try 32 bpp as well.
	p.bpp = 32;
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	// Fall back on a known resolution good rather than 640 x 480.
	DisplaySpecs dr;
	this->GetDisplaySpecs(dr);
	if( dr.empty() )
	{
		vs.push_back( "No display resolutions" );
		return SETVIDEOMODE_FAILED.GetValue() + " " + join(";",vs);
	}

	DisplaySpec d =  *dr.begin();
	// Try to find DisplaySpec corresponding to requested display
	for (const auto &candidate: dr)
	{
		if (candidate.currentMode() != nullptr)
		{
			d = candidate;
			if (candidate.id() == p.sDisplayId)
			{
				break;
			}
		}
	}

	p.sDisplayId = d.id();
	const DisplayMode supported = d.currentMode() != nullptr ? *d.currentMode() : *d.supportedModes().begin();
	p.width = supported.width;
	p.height = supported.height;
	p.rate = std::round(supported.refreshRate);
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	return SETVIDEOMODE_FAILED.GetValue() + " " + join(";",vs);
}

void RageDisplay::ProcessStatsOnFlip()
{
	g_iFramesRenderedSinceLastCheck++;
	g_iFramesRenderedSinceLastReset++;

	if( g_LastCheckTimer.Ago() >= 1.0f )	// update stats every 1 sec.
	{
		float fActualTime = g_LastCheckTimer.GetDeltaTime();
		g_iNumChecksSinceLastReset++;
		g_iFPS = std::lrint( g_iFramesRenderedSinceLastCheck / fActualTime );
		g_iCFPS = g_iFramesRenderedSinceLastReset / g_iNumChecksSinceLastReset;
		g_iCFPS = std::lrint( g_iCFPS / fActualTime );
		g_iVPF = g_iVertsRenderedSinceLastCheck / g_iFramesRenderedSinceLastCheck;
		g_iFramesRenderedSinceLastCheck = g_iVertsRenderedSinceLastCheck = 0;
		if( LOG_FPS )
		{
			RString sStats = GetStats();
			Replace(sStats, "\n", ", ");
			LOG->Trace( "%s", sStats.c_str() );
		}
	}
}

void RageDisplay::ResetStats()
{
	g_iFPS = g_iVPF = 0;
	g_iFramesRenderedSinceLastCheck = g_iFramesRenderedSinceLastReset = 0;
	g_iNumChecksSinceLastReset = 0;
	g_iVertsRenderedSinceLastCheck = 0;
	g_LastCheckTimer.GetDeltaTime();
}

RString RageDisplay::GetStats() const
{
	RString s;
	// If FPS == 0, we don't have stats yet.
	if( !GetFPS() )
		s = "-- FPS\n-- av FPS\n-- VPF";

	s = ssprintf( "%i FPS\n%i av FPS\n%i VPF", GetFPS(), GetCumFPS(), GetVPF() );

//	#if defined(_WIN32)
	s += "\n"+this->GetApiDescription();
//	#endif

	return s;
}

bool RageDisplay::BeginFrame()
{
	this->SetDefaultRenderStates();

	return true;
}

void RageDisplay::EndFrame()
{
	ProcessStatsOnFlip();
}

void RageDisplay::BeginConcurrentRendering()
{
	this->SetDefaultRenderStates();
}

void RageDisplay::StatsAddVerts( int iNumVertsRendered ) { g_iVertsRenderedSinceLastCheck += iNumVertsRendered; }

/* Draw a line as a quad.  GL_LINES with SmoothLines off can draw line
 * ends at odd angles--they're forced to axis-alignment regardless of the
 * angle of the line. */
void RageDisplay::DrawPolyLine(const RageSpriteVertex &p1, const RageSpriteVertex &p2, float LineWidth )
{
	// soh cah toa strikes strikes again!
	float opp = p2.p.x - p1.p.x;
	float adj = p2.p.y - p1.p.y;
	float hyp = std::pow(opp*opp + adj*adj, 0.5f);

	float lsin = opp/hyp;
	float lcos = adj/hyp;

	RageSpriteVertex v[4];

	v[0] = v[1] = p1;
	v[2] = v[3] = p2;

	float ydist = lsin * LineWidth/2;
	float xdist = lcos * LineWidth/2;

	v[0].p.x += xdist;
	v[0].p.y -= ydist;
	v[1].p.x -= xdist;
	v[1].p.y += ydist;
	v[2].p.x -= xdist;
	v[2].p.y += ydist;
	v[3].p.x += xdist;
	v[3].p.y -= ydist;

	this->DrawQuad(v);
}


void RageDisplay::DrawLineStripInternal( const RageSpriteVertex v[], int iNumVerts, float LineWidth )
{
	ASSERT( iNumVerts >= 2 );

	/* Draw a line strip with rounded corners using polys. This is used on
	 * cards that have strange allergic reactions to antialiased points and
	 * lines. */
	for( int i = 0; i < iNumVerts-1; ++i )
		DrawPolyLine(v[i], v[i+1], LineWidth);

	// Join the lines with circles so we get rounded corners.
	for( int i = 0; i < iNumVerts; ++i )
		DrawCircle( v[i], LineWidth/2 );
}

void RageDisplay::DrawCircleInternal( const RageSpriteVertex &p, float radius )
{
	const int subdivisions = 32;
	RageSpriteVertex v[subdivisions+2];
	v[0] = p;

	for(int i = 0; i < subdivisions+1; ++i)
	{
		const float fRotation = float(i) / subdivisions * 2*PI;
		const float fX = std::cos(fRotation) * radius;
		const float fY = -std::sin(fRotation) * radius;
		v[1+i] = v[0];
		v[1+i].p.x += fX;
		v[1+i].p.y += fY;
	}

	this->DrawFan( v, subdivisions+2 );
}


void RageDisplay::SetDefaultRenderStates()
{
	SetLighting( false );
	SetCullMode( CULL_NONE );
	SetZWrite( false );
	SetZTestMode( ZTEST_OFF );
	SetAlphaTest( true );
	SetBlendMode( BLEND_NORMAL );
	SetTextureFiltering( TextureUnit_1, true );
	SetZBias( 0 );
	LoadMenuPerspective( 0, 640, 480, 320, 240 ); // 0 FOV = ortho
}


// Matrix stuff
class MatrixStack
{
	std::vector<RageMatrix> stack;
public:

	MatrixStack(): stack()
	{
		stack.resize(1);
		LoadIdentity();
	}

	// Pops the top of the stack.
	void Pop()
	{
		stack.pop_back();
		ASSERT( stack.size() > 0 ); // underflow
	}

	// Pushes the stack by one, duplicating the current matrix.
	void Push()
	{
		stack.push_back( stack.back() );
		ASSERT( stack.size() < 100 ); // overflow
	}

	// Loads identity in the current matrix.
	void LoadIdentity()
	{
		RageMatrixIdentity( &stack.back() );
	}

	// Loads the given matrix into the current matrix
	void LoadMatrix( const RageMatrix& m )
	{
		stack.back() = m;
	}

	// Right-Multiplies the given matrix to the current matrix.
	// (transformation is about the current world origin)
	void MultMatrix( const RageMatrix& m )
	{
		RageMatrixMultiply( &stack.back(), &m, &stack.back() );
	}

	// Left-Multiplies the given matrix to the current matrix
	// (transformation is about the local origin of the object)
	void MultMatrixLocal( const RageMatrix& m )
	{
		RageMatrixMultiply( &stack.back(), &stack.back(), &m );
	}

	// Right multiply the current matrix with the computed rotation
	// matrix, counterclockwise about the given axis with the given angle.
	// (rotation is about the current world origin)
	void RotateX( float degrees )
	{
		RageMatrix m;
		RageMatrixRotationX( &m, degrees );
		MultMatrix( m );
	}
	void RotateY( float degrees )
	{
		RageMatrix m;
		RageMatrixRotationY( &m, degrees );
		MultMatrix( m );
	}
	void RotateZ( float degrees )
	{
		RageMatrix m;
		RageMatrixRotationZ( &m, degrees );
		MultMatrix( m );
	}

	// Left multiply the current matrix with the computed rotation
	// matrix. All angles are counterclockwise. (rotation is about the
	// local origin of the object)
	void RotateXLocal( float degrees )
	{
		RageMatrix m;
		RageMatrixRotationX( &m, degrees );
		MultMatrixLocal( m );
	}
	void RotateYLocal( float degrees )
 	{
		RageMatrix m;
		RageMatrixRotationY( &m, degrees );
		MultMatrixLocal( m );
	}
	void RotateZLocal( float degrees )
	{
		RageMatrix m;
		RageMatrixRotationZ( &m, degrees );
		MultMatrixLocal( m );
	}

	// Right multiply the current matrix with the computed scale
	// matrix. (transformation is about the current world origin)
	void Scale( float x, float y, float z )
 	{
		RageMatrix m;
		RageMatrixScaling( &m, x, y, z );
		MultMatrix( m );
	}

	// Left multiply the current matrix with the computed scale
	// matrix. (transformation is about the local origin of the object)
	void ScaleLocal( float x, float y, float z )
 	{
		RageMatrix m;
		RageMatrixScaling( &m, x, y, z );
		MultMatrixLocal( m );
	}

	// Right multiply the current matrix with the computed translation
	// matrix. (transformation is about the current world origin)
	void Translate( float x, float y, float z )
 	{
		RageMatrix m;
		RageMatrixTranslation( &m, x, y, z );
		MultMatrix( m );
	}

	// Left multiply the current matrix with the computed translation
	// matrix. (transformation is about the local origin of the object)
	void TranslateLocal( float x, float y, float z )
 	{
		RageMatrix m;
		RageMatrixTranslation( &m, x, y, z );
		MultMatrixLocal( m );
	}

	void SkewX( float fAmount )
	{
		RageMatrix m;
		RageMatrixSkewX( &m, fAmount );
		MultMatrixLocal( m );
	}

	void SkewY( float fAmount )
	{
		RageMatrix m;
		RageMatrixSkewY( &m, fAmount );
		MultMatrixLocal( m );
	}

	// Obtain the current matrix at the top of the stack
	const RageMatrix* GetTop() const { return &stack.back(); }
	void SetTop( const RageMatrix &m ) { stack.back() = m; }
};


static RageMatrix g_CenteringMatrix;
static MatrixStack g_ProjectionStack;
static MatrixStack g_ViewStack;
static MatrixStack g_WorldStack;
static MatrixStack g_TextureStack;


RageDisplay::RageDisplay()
{
	RageMatrixIdentity( &g_CenteringMatrix );
	g_ProjectionStack = MatrixStack();
	g_ViewStack = MatrixStack();
	g_WorldStack = MatrixStack();
	g_TextureStack = MatrixStack();

	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "DISPLAY" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}
}

RageDisplay::~RageDisplay()
{
	// Unregister with Lua.
	LUA->UnsetGlobal( "DISPLAY" );
}

const RageMatrix* RageDisplay::GetCentering() const
{
	return &g_CenteringMatrix;
}

const RageMatrix* RageDisplay::GetProjectionTop() const
{
	return g_ProjectionStack.GetTop();
}

const RageMatrix* RageDisplay::GetViewTop() const
{
	return g_ViewStack.GetTop();
}

const RageMatrix* RageDisplay::GetWorldTop() const
{
	return g_WorldStack.GetTop();
}

const RageMatrix* RageDisplay::GetTextureTop() const
{
	return g_TextureStack.GetTop();
}

void RageDisplay::PushMatrix()
{
	g_WorldStack.Push();
}

void RageDisplay::PopMatrix()
{
	g_WorldStack.Pop();
}

void RageDisplay::Translate( float x, float y, float z )
{
	g_WorldStack.TranslateLocal(x, y, z);
}

void RageDisplay::TranslateWorld( float x, float y, float z )
{
	g_WorldStack.Translate(x, y, z);
}

void RageDisplay::Scale( float x, float y, float z )
{
	g_WorldStack.ScaleLocal(x, y, z);
}

void RageDisplay::RotateX( float deg )
{
	g_WorldStack.RotateXLocal( deg );
}

void RageDisplay::RotateY( float deg )
{
	g_WorldStack.RotateYLocal( deg );
}

void RageDisplay::RotateZ( float deg )
{
	g_WorldStack.RotateZLocal( deg );
}

void RageDisplay::SkewX( float fAmount )
{
	g_WorldStack.SkewX( fAmount );
}

void RageDisplay::SkewY( float fAmount )
{
	g_WorldStack.SkewY( fAmount );
}

void RageDisplay::PostMultMatrix( const RageMatrix &m )
{
	g_WorldStack.MultMatrix( m );
}

void RageDisplay::PreMultMatrix( const RageMatrix &m )
{
	g_WorldStack.MultMatrixLocal( m );
}

void RageDisplay::LoadIdentity()
{
	g_WorldStack.LoadIdentity();
}


void RageDisplay::TexturePushMatrix()
{
	g_TextureStack.Push();
}

void RageDisplay::TexturePopMatrix()
{
	g_TextureStack.Pop();
}

void RageDisplay::TextureTranslate( float x, float y )
{
	g_TextureStack.TranslateLocal(x, y, 0);
}


void RageDisplay::LoadMenuPerspective( float fovDegrees, float fWidth, float fHeight, float fVanishPointX, float fVanishPointY )
{
	// fovDegrees == 0 gives ortho projection.
	if( fovDegrees == 0 )
	{
 		float left = 0, right = fWidth, bottom = fHeight, top = 0;
		g_ProjectionStack.LoadMatrix( GetOrthoMatrix(left, right, bottom, top, -1000, +1000) );
 		g_ViewStack.LoadIdentity();
	}
	else
	{
		CLAMP( fovDegrees, 0.1f, 179.9f );
		float fovRadians = fovDegrees / 180.f * PI;
		float theta = fovRadians/2;
		float fDistCameraFromImage = fWidth/2 / std::tan( theta );

		fVanishPointX = SCALE( fVanishPointX, 0, fWidth, fWidth, 0 );
		fVanishPointY = SCALE( fVanishPointY, 0, fHeight, fHeight, 0 );

		fVanishPointX -= fWidth/2;
		fVanishPointY -= fHeight/2;

		// It's the caller's responsibility to push first.
		g_ProjectionStack.LoadMatrix(
			GetFrustumMatrix(
			  (fVanishPointX-fWidth/2)/fDistCameraFromImage,
			  (fVanishPointX+fWidth/2)/fDistCameraFromImage,
			  (fVanishPointY+fHeight/2)/fDistCameraFromImage,
			  (fVanishPointY-fHeight/2)/fDistCameraFromImage,
			  1,
			  fDistCameraFromImage+1000	) );

		g_ViewStack.LoadMatrix(
			RageLookAt(
				-fVanishPointX+fWidth/2, -fVanishPointY+fHeight/2, fDistCameraFromImage,
				-fVanishPointX+fWidth/2, -fVanishPointY+fHeight/2, 0,
				0.0f, 1.0f, 0.0f) );
	}
}


void RageDisplay::CameraPushMatrix()
{
	g_ProjectionStack.Push();
	g_ViewStack.Push();
}

void RageDisplay::CameraPopMatrix()
{
	g_ProjectionStack.Pop();
	g_ViewStack.Pop();
}


/* gluLookAt. The result is pre-multiplied to the matrix (M = L * M) instead of
 * post-multiplied. */
void RageDisplay::LoadLookAt( float fFOV, const RageVector3 &Eye, const RageVector3 &At, const RageVector3 &Up )
{
	float fAspect = GetActualVideoModeParams().fDisplayAspectRatio;
	g_ProjectionStack.LoadMatrix( GetPerspectiveMatrix(fFOV, fAspect, 1, 1000) );

	// Flip the Y coordinate, so positive numbers go down.
	g_ProjectionStack.Scale( 1, -1, 1 );

	g_ViewStack.LoadMatrix( RageLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z) );
}


RageMatrix RageDisplay::GetPerspectiveMatrix(float fovy, float aspect, float zNear, float zFar)
{
	float ymax = zNear * std::tan(fovy * PI / 360.0f);
	float ymin = -ymax;
	float xmin = ymin * aspect;
	float xmax = ymax * aspect;

	return GetFrustumMatrix(xmin, xmax, ymin, ymax, zNear, zFar);
}

RageSurface *RageDisplay::CreateSurfaceFromPixfmt( RagePixelFormat pixfmt,
						void *pixels, int width, int height, int pitch )
{
	const RagePixelFormatDesc *tpf = GetPixelFormatDesc(pixfmt);

	RageSurface *surf = CreateSurfaceFrom(
		width, height, tpf->bpp,
		tpf->masks[0], tpf->masks[1], tpf->masks[2], tpf->masks[3],
		(uint8_t *) pixels, pitch );

	return surf;
}

RagePixelFormat RageDisplay::FindPixelFormat( int iBPP, unsigned iRmask, unsigned iGmask, unsigned iBmask, unsigned iAmask, bool bRealtime )
{
	RagePixelFormatDesc tmp = { iBPP, { iRmask, iGmask, iBmask, iAmask } };

	FOREACH_ENUM( RagePixelFormat, iPixFmt )
	{
		const RagePixelFormatDesc *pf = GetPixelFormatDesc( RagePixelFormat(iPixFmt) );
		if( !SupportsTextureFormat(RagePixelFormat(iPixFmt), bRealtime) )
			continue;

		if( memcmp(pf, &tmp, sizeof(tmp)) )
			continue;
		return iPixFmt;
	}

	return RagePixelFormat_Invalid;
}

/* These convert to OpenGL's coordinate system: -1,-1 is the bottom-left,
 * +1,+1 is the top-right, and Z goes from -1 (viewer) to +1 (distance).
 * It's a little odd, but very well-defined. */
RageMatrix RageDisplay::GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf )
{
	RageMatrix m(
		2/(r-l),      0,            0,           0,
		0,            2/(t-b),      0,           0,
		0,            0,            -2/(zf-zn),   0,
		-(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn),  1 );
	return m;
}

RageMatrix RageDisplay::GetFrustumMatrix( float l, float r, float b, float t, float zn, float zf )
{
	// glFrustum
	float A = (r+l) / (r-l);
	float B = (t+b) / (t-b);
	float C = -1 * (zf+zn) / (zf-zn);
	float D = -1 * (2*zf*zn) / (zf-zn);
	RageMatrix m(
		2*zn/(r-l), 0,          0,  0,
		0,          2*zn/(t-b), 0,  0,
		A,          B,          C,  -1,
		0,          0,          D,  0 );
	return m;
}

void RageDisplay::ResolutionChanged()
{
	// The centering matrix depends on the resolution.
	UpdateCentering();
}

void RageDisplay::CenteringPushMatrix()
{
	g_CenteringStack.push_back( g_CenteringStack.back() );
	ASSERT( g_CenteringStack.size() < 100 ); // overflow
}

void RageDisplay::CenteringPopMatrix()
{
	g_CenteringStack.pop_back();
	ASSERT( g_CenteringStack.size() > 0 ); // underflow
	UpdateCentering();
}

void RageDisplay::ChangeCentering( int iTranslateX, int iTranslateY, int iAddWidth, int iAddHeight )
{
	g_CenteringStack.back() = Centering( iTranslateX, iTranslateY, iAddWidth, iAddHeight );

	UpdateCentering();
}

RageMatrix RageDisplay::GetCenteringMatrix( float fTranslateX, float fTranslateY, float fAddWidth, float fAddHeight ) const
{
	// in screen space, left edge = -1, right edge = 1, bottom edge = -1. top edge = 1
	float fWidth = (float) GetActualVideoModeParams().windowWidth;
	float fHeight = (float) GetActualVideoModeParams().windowHeight;
	float fPercentShiftX = SCALE( fTranslateX, 0, fWidth, 0, +2.0f );
	float fPercentShiftY = SCALE( fTranslateY, 0, fHeight, 0, -2.0f );
	float fPercentScaleX = SCALE( fAddWidth, 0, fWidth, 1.0f, 2.0f );
	float fPercentScaleY = SCALE( fAddHeight, 0, fHeight, 1.0f, 2.0f );

	RageMatrix m1;
	RageMatrix m2;
	RageMatrixTranslation(
		&m1,
		fPercentShiftX,
		fPercentShiftY,
		0 );
	RageMatrixScaling(
		&m2,
		fPercentScaleX,
		fPercentScaleY,
		1 );
	RageMatrix mOut;
	RageMatrixMultiply( &mOut, &m1, &m2 );
	return mOut;
}

void RageDisplay::UpdateCentering()
{
	const Centering &p = g_CenteringStack.back();
	g_CenteringMatrix = GetCenteringMatrix(
		(float) p.m_iTranslateX, (float) p.m_iTranslateY, (float) p.m_iAddWidth, (float) p.m_iAddHeight );
}

bool RageDisplay::SaveScreenshot( const RString &sPath, GraphicsFileFormat format )
{
	RageSurface *surface = this->CreateScreenshot();
	
	if (nullptr == surface)
	{
		LOG->Warn("SaveScreenshot: failed to create a screenshot surface");
		return false;
	}

	/* Unless we're in lossless, resize the image to 640x480.  If we're saving lossy,
	 * there's no sense in saving 1280x960 screenshots, and we don't want to output
	 * screenshots in a strange (non-1) sample aspect ratio. */
	if( format != SAVE_LOSSLESS && format != SAVE_LOSSLESS_SENSIBLE )
	{
		// Maintain the DAR.
		ASSERT( GetActualVideoModeParams().fDisplayAspectRatio > 0 );
		int iHeight = 480;
		// This used to be lrint. However, lrint causes odd resolutions like
		// 639x480 (4:3) and 853x480 (16:9). ceil gives correct values. -aj
		int iWidth = std::ceil( iHeight * GetActualVideoModeParams().fDisplayAspectRatio );
		RageSurfaceUtils::Zoom( surface, iWidth, iHeight );
	}

	RageFile out;
	if( !out.Open( sPath, RageFile::WRITE ) )
	{
		LOG->Warn("SaveScreenshot: Failed to open %s for writing: %s", sPath.c_str(), out.GetError().c_str() );
		RageUtil::SafeDelete( surface );
		return false;
	}

	bool bSuccess = false;
	RString strError = "";
	switch( format )
	{
	case SAVE_LOSSLESS:
		bSuccess = RageSurfaceUtils::SaveBMP( surface, out );
		break;
	case SAVE_LOSSLESS_SENSIBLE:
		bSuccess = RageSurfaceUtils::SavePNG( surface, out, strError );
		break;
	case SAVE_LOSSY_LOW_QUAL:
		bSuccess = RageSurfaceUtils::SaveJPEG( surface, out, false );
		break;
	case SAVE_LOSSY_HIGH_QUAL:
		bSuccess = RageSurfaceUtils::SaveJPEG( surface, out, true );
		break;
	default:
		LOG->Warn("SaveScreenshot: Invalid graphics file format requested %d", format);
		bSuccess = false;
		break;
	}

	RageUtil::SafeDelete( surface );

	if( !bSuccess )
	{
		LOG->Warn("SaveScreenshot: Failed to save screenshot to %s: %s", sPath.c_str(), out.GetError().c_str() );
	}

	return bSuccess;
}

void RageDisplay::DrawQuads( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( (iNumVerts%4) == 0 );

	if(!iNumVerts)
		return;

	this->DrawQuadsInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawQuadStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( (iNumVerts%2) == 0 );

	if(iNumVerts < 4)
		return;

	this->DrawQuadStripInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawFan( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	this->DrawFanInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	this->DrawStripInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawTriangles( const RageSpriteVertex v[], int iNumVerts )
{
	if( iNumVerts == 0 )
		return;

	ASSERT( iNumVerts >= 3 );

	this->DrawTrianglesInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawCompiledGeometry( const RageCompiledGeometry *p, int iMeshIndex, const std::vector<msMesh> &vMeshes )
{
	this->DrawCompiledGeometryInternal( p, iMeshIndex );

	StatsAddVerts( vMeshes[iMeshIndex].Triangles.size() );
}

void RageDisplay::DrawLineStrip( const RageSpriteVertex v[], int iNumVerts, float LineWidth )
{
	ASSERT( iNumVerts >= 2 );

	this->DrawLineStripInternal( v, iNumVerts, LineWidth );
}

/*
 * Draw a strip of:
 *
 * 0..1..2
 * . /.\ .
 * ./ . \.
 * 3..4..5
 * . /.\ .
 * ./ . \.
 * 6..7..8
 */

void RageDisplay::DrawSymmetricQuadStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	if( iNumVerts < 6 )
		return;

	this->DrawSymmetricQuadStripInternal( v, iNumVerts );

	StatsAddVerts( iNumVerts );
}

void RageDisplay::DrawCircle( const RageSpriteVertex &v, float radius )
{
	this->DrawCircleInternal( v, radius );
}

void RageDisplay::FrameLimitBeforeVsync( int iFPS )
{
	ASSERT( iFPS != 0 );

	int iDelayMicroseconds = 0;
	if( g_fFrameLimitPercent.Get() > 0.0f && !g_LastFrameEndedAt.IsZero() )
	{
		float fFrameTime = g_LastFrameEndedAt.GetDeltaTime();
		float fExpectedTime = 1.0f / iFPS;

		/* This is typically used to turn some of the delay that would normally
		 * be waiting for vsync and turn it into a usleep, to make sure we give
		 * up the CPU.  If we overshoot the sleep, then we'll miss the vsync,
		 * so allow tweaking the amount of time we expect a frame to take.
		 * Frame limiting is disabled by setting this to 0. */
		fExpectedTime *= g_fFrameLimitPercent.Get();
		float fExtraTime = fExpectedTime - fFrameTime;

		iDelayMicroseconds = int(fExtraTime * 1000000);
	}

	if( !HOOKS->AppHasFocus() )
		iDelayMicroseconds = std::max( iDelayMicroseconds, 10000 ); // give some time to other processes and threads

	if( iDelayMicroseconds > 0 )
		usleep( iDelayMicroseconds );
}

void RageDisplay::FrameLimitAfterVsync()
{
	if( g_fFrameLimitPercent.Get() == 0.0f )
		return;

	g_LastFrameEndedAt.Touch();
}


RageCompiledGeometry::~RageCompiledGeometry()
{
	m_bNeedsNormals = false;
}

void RageCompiledGeometry::Set( const std::vector<msMesh> &vMeshes, bool bNeedsNormals )
{
	m_bNeedsNormals = bNeedsNormals;

	size_t totalVerts = 0;
	size_t totalTriangles = 0;

	m_bAnyNeedsTextureMatrixScale = false;

	m_vMeshInfo.resize( vMeshes.size() );
	for( unsigned i=0; i<vMeshes.size(); i++ )
	{
		const msMesh& mesh = vMeshes[i];
		const std::vector<RageModelVertex> &Vertices = mesh.Vertices;
		const std::vector<msTriangle> &Triangles = mesh.Triangles;

		MeshInfo& meshInfo = m_vMeshInfo[i];
		meshInfo.m_bNeedsTextureMatrixScale = false;

		meshInfo.iVertexStart = totalVerts;
		meshInfo.iVertexCount = Vertices.size();
		meshInfo.iTriangleStart = totalTriangles;
		meshInfo.iTriangleCount = Triangles.size();

		totalVerts += Vertices.size();
		totalTriangles += Triangles.size();

		for( unsigned j = 0; j < Vertices.size(); ++j )
		{
			if( Vertices[j].TextureMatrixScale.x != 1.0f || Vertices[j].TextureMatrixScale.y != 1.0f )
			{
				meshInfo.m_bNeedsTextureMatrixScale = true;
				m_bAnyNeedsTextureMatrixScale = true;
			}
		}
	}

	this->Allocate( vMeshes );

	Change( vMeshes );
}

// lua start
#include "LuaBinding.h"

// Register with Lua.
static void register_REFRESH_DEFAULT(lua_State *L)
{
	lua_pushstring( L, "REFRESH_DEFAULT" );
	lua_pushinteger( L, REFRESH_DEFAULT );
	lua_settable( L, LUA_GLOBALSINDEX);
}
REGISTER_WITH_LUA_FUNCTION( register_REFRESH_DEFAULT );


/** @brief Allow Lua to have access to the RageDisplay. */
class LunaRageDisplay: public Luna<RageDisplay>
{
public:
	static int GetDisplayWidth( T* p, lua_State *L )
	{
		VideoModeParams params = p->GetActualVideoModeParams();
		LuaHelpers::Push( L, params.width );
		return 1;
	}

	static int GetDisplayHeight( T* p, lua_State *L )
	{
		VideoModeParams params = p->GetActualVideoModeParams();
		LuaHelpers::Push( L, params.height );
		return 1;
	}

	static int GetFPS( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetFPS());
		return 1;
	}

	static int GetVPF( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetVPF());
		return 1;
	}

	static int GetCumFPS( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetCumFPS());
		return 1;
	}

	static int GetDisplaySpecs( T* p, lua_State *L )
	{
		DisplaySpecs s;
		p->GetDisplaySpecs(s);
		pushDisplaySpecs(L, s);
		return 1;
	}

	static int SupportsRenderToTexture( T* p, lua_State *L )
	{
		lua_pushboolean(L, p->SupportsRenderToTexture());
		return 1;
	}

	static int SupportsFullscreenBorderlessWindow( T* p, lua_State *L )
	{
		lua_pushboolean(L, p->SupportsFullscreenBorderlessWindow());
		return 1;
	}

	LunaRageDisplay()
	{
		ADD_METHOD( GetDisplayWidth );
		ADD_METHOD( GetDisplayHeight );
		ADD_METHOD( GetFPS );
		ADD_METHOD( GetVPF );
		ADD_METHOD( GetCumFPS );
		ADD_METHOD( GetDisplaySpecs );
		ADD_METHOD( SupportsRenderToTexture );
		ADD_METHOD( SupportsFullscreenBorderlessWindow );
	}
};

LUA_REGISTER_CLASS( RageDisplay )
// lua end

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

