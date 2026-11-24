/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//
// Z Window
//

#include "zwindow.h"

#include <algorithm>
#include <limits>

#include "debugging/debugging.h"

#include "igl.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "irender.h"
#include "preferencesystem.h"
#include "stringio.h"

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QSplitter>
#include <QTimer>

#include "math/plane.h"
#include "stream/stringstream.h"
#include "texturelib.h"

#include "gtkutil/widget.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/cursor.h"
#include "brush.h"
#include "camwindow.h"
#include "mainframe.h"
#include "commands.h"
#include "grid.h"
#include "xywindow.h"


const int c_zwnd_minWidth = 24;
const int c_zwnd_defaultWidth = 96;

ToggleShown g_z_shown( false );

class ZGLWidget : public QOpenGLWidget
{
	ZWnd& m_zwnd;
	DeferredMotion m_deferred_motion;
	qreal m_scale;
	bool m_contextCreated{};
public:
	ZGLWidget( ZWnd& zwnd ) : QOpenGLWidget(), m_zwnd( zwnd ),
		m_deferred_motion( [this]( const QMouseEvent& event ){
				m_zwnd.Z_MouseMoved( event.x() * m_scale, event.y() * m_scale, buttons_for_state( event ) );
			} ),
		m_scale( 1 )
	{
		setMouseTracking( true );
		setMinimumWidth( c_zwnd_minWidth );
	}

	~ZGLWidget() override {
		if( m_contextCreated )
			glwidget_context_destroyed();
	}

protected:
	void initializeGL() override
	{
		glwidget_context_created( *this );
		m_contextCreated = true;
	}
	void resizeGL( int w, int h ) override
	{
		m_scale = devicePixelRatioF();
		m_zwnd.m_nWidth = float_to_integer( w * m_scale );
		m_zwnd.m_nHeight = float_to_integer( h * m_scale );
	}
	void paintGL() override
	{
		if ( Map_Valid( g_map ) && ScreenUpdates_Enabled() ) {
			GlobalOpenGL_debugAssertNoErrors();
			m_zwnd.Z_Draw();
			GlobalOpenGL_debugAssertNoErrors();
		}
	}

	void hideEvent( QHideEvent *event ) override {
		QOpenGLWidget::hideEvent( event );
		g_z_shown.update();
	}
	void showEvent( QShowEvent *event ) override {
		QOpenGLWidget::showEvent( event );
		// docked: the toggle connects before the main window shows the pane, so refresh its menu check here
		g_z_shown.update();
		// docked: a splitter restores a hidden pane with no width, so give it a usable one
		if( auto *splitter = qobject_cast<QSplitter*>( parentWidget() ) ){
			QTimer::singleShot( 0, this, [splitter, this](){
				const int index = splitter->indexOf( this );
				QList<int> sizes = splitter->sizes();
				if( index >= 0 && sizes.size() == 2 && isVisible() && sizes[index] <= c_zwnd_minWidth ){
					const int total = sizes[0] + sizes[1];
					sizes[index] = std::min( c_zwnd_defaultWidth, total / 2 );
					sizes[1 - index] = total - sizes[index];
					splitter->setSizes( sizes );
				}
			} );
		}
	}

	void mousePressEvent( QMouseEvent *event ) override {
		m_zwnd.Z_MouseDown( event->x() * m_scale, event->y() * m_scale, buttons_for_event_button( event ) );
	}
	void mouseMoveEvent( QMouseEvent *event ) override {
		m_deferred_motion.motion( event );
	}
	void mouseReleaseEvent( QMouseEvent *event ) override {
		m_zwnd.Z_MouseUp( event->x() * m_scale, event->y() * m_scale, buttons_for_event_button( event ) );
	}
	void wheelEvent( QWheelEvent *event ) override {
		if ( event->angleDelta().y() > 0 ) {
			m_zwnd.ZoomInWithMouse( event->position().y() * m_scale );
		}
		else if ( event->angleDelta().y() < 0 ) {
			m_zwnd.ZoomOutWithMouse( event->position().y() * m_scale );
		}
	}
};


ZWnd::ZWnd() :
	m_gl_widget( new ZGLWidget( *this ) ),
	m_deferredDraw( WidgetQueueDrawCaller( *m_gl_widget ) ),
	m_parent( 0 ),
	m_nWidth( 0 ),
	m_nHeight( 0 ),
	m_probeFollowsCamera( true ),
	m_origin( 0 ),
	m_fScale( 1 ),
	m_probe( 0, 0, 0 ),
	m_move_started( false ),
	m_move_y( 0 )
{
}

ZWnd::~ZWnd(){
	delete m_gl_widget;
}

void ZWnd::onMapValidChanged(){
	DeferredDraw_onMapValidChanged( m_deferredDraw );
}

void ZWnd::SetOrigin( float z ){
	m_origin = std::clamp( z, -GetMaxGridCoord(), GetMaxGridCoord() );
	queueDraw();
}

void ZWnd::Scroll( int y ){
	SetOrigin( m_origin + y / m_fScale );
}

void ZWnd::SetScale( float f ){
	const float max_scale = 64.f;
	const float min_scale = std::max( m_nHeight, 1 ) / ( 2.2f * GetMaxGridCoord() );
	m_fScale = std::clamp( f, min_scale, max_scale );
	queueDraw();
}

void ZWnd::ZoomInWithMouse( int y ){
	const float z = ScreenToZ( y );
	SetScale( m_fScale * 5.0f / 4.0f );
	SetOrigin( z - ( m_nHeight * .5f - y ) / m_fScale );
}

void ZWnd::ZoomOutWithMouse( int y ){
	const float z = ScreenToZ( y );
	SetScale( m_fScale * 4.0f / 5.0f );
	SetOrigin( z - ( m_nHeight * .5f - y ) / m_fScale );
}

void ZWnd::SetProbePoint( const Vector3& point ){
	m_probe = point;
	queueDraw();
}

void ZWnd::updateProbePoint(){
	if ( m_probeFollowsCamera && g_pParentWnd != 0 && g_pParentWnd->GetCamWnd() != 0 ) {
		m_probe = Camera_getOrigin( *g_pParentWnd->GetCamWnd() );
	}
}


void ZWnd_PositionCamera( ZWnd& zwnd, int y ){
	if ( CamWnd* camwnd = g_pParentWnd->GetCamWnd() ) {
		Vector3 origin = Camera_getOrigin( *camwnd );
		origin.z() = float_snapped( zwnd.ScreenToZ( y ), GetSnapGridSize() );
		Camera_setOrigin( *camwnd, origin );
	}
}

void ZWnd::Z_MouseDown( int x, int y, unsigned int buttons ){
	if ( buttons == Move_buttons() ) {
		m_move_started = true;
		m_move_y = y;
	}
	else if ( buttons == MoveCamera_buttons() ) {
		ZWnd_PositionCamera( *this, y );
	}
}

void ZWnd::Z_MouseUp( int x, int y, unsigned int buttons ){
	m_move_started = false;
}

void ZWnd::Z_MouseMoved( int x, int y, unsigned int buttons ){
	if ( m_move_started ) {
		Scroll( y - m_move_y );
		m_move_y = y;
	}
	else if ( buttons == MoveCamera_buttons() ) {
		ZWnd_PositionCamera( *this, y );
	}
	else
	{
		g_pParentWnd->SetStatusText( c_status_position, StringStream<64>( "z: ", FloatFormat( ScreenToZ( y ), 6, 1 ) ) );
	}
}


class ZColumnCollector
{
	const Vector3& m_probe;
	std::vector<ZBrushColumn>& m_columns;
public:
	ZColumnCollector( const Vector3& probe, std::vector<ZBrushColumn>& columns ) : m_probe( probe ), m_columns( columns ){
	}
	void operator()( BrushInstance& brush ) const {
		const AABB& bounds = brush.worldAABB();
		if ( std::fabs( m_probe.x() - bounds.origin.x() ) > bounds.extents.x()
		  || std::fabs( m_probe.y() - bounds.origin.y() ) > bounds.extents.y() ) {
			return;
		}

		// face planes are in the brush's local space; outward normals, so the inside is dot( normal, point ) <= dist
		const Matrix4& localToWorld = brush.localToWorld();
		const bool transformed = !matrix4_affine_equal( localToWorld, g_matrix4_identity );
		double zmin = -std::numeric_limits<double>::infinity();
		double zmax = std::numeric_limits<double>::infinity();
		const Face* top = nullptr;
		bool outside = false;

		Brush_forEachFace( brush.getBrush(), [&]( Face& face ){
			if ( outside || !face.contributes() ) {
				return;
			}
			const Plane3 plane = transformed? plane3_transformed_affine_full( face.plane3(), localToWorld ) : face.plane3();
			// clip the line ( probe.x, probe.y, z ): normal.z * z <= dist - normal.x * probe.x - normal.y * probe.y
			const double rest = plane.dist() - plane.a * m_probe.x() - plane.b * m_probe.y();
			if ( plane.c > 1e-6 ) {
				const double z = rest / plane.c;
				if ( z < zmax ) {
					zmax = z;
					top = &face;
				}
			}
			else if ( plane.c < -1e-6 ) {
				zmin = std::max( zmin, rest / plane.c );
			}
			else if ( rest < 0 ) {
				outside = true;
			}
		} );

		if ( outside || top == nullptr || !( zmin < zmax ) || !std::isfinite( zmin ) ) {
			return;
		}

		Vector3 colour( .5f, .5f, .5f );
		if ( const Shader* state = top->getShader().state() ) {
			colour = state->getTexture().color;
		}
		m_columns.push_back( ZBrushColumn{ &brush, static_cast<float>( zmin ), static_cast<float>( zmax ), colour, brush.isSelected() || brush.parentSelected() } );
	}
};

void ZWnd::collectColumns( std::vector<ZBrushColumn>& columns ) const {
	Scene_forEachVisibleBrush( GlobalSceneGraph(), ZColumnCollector( m_probe, columns ) );
	// draw the largest first, so that the intervals inside it stay visible
	std::stable_sort( columns.begin(), columns.end(), []( const ZBrushColumn& a, const ZBrushColumn& b ){
		return ( a.zmax - a.zmin ) > ( b.zmax - b.zmin );
	} );
}


float ZWnd::columnCenter() const {
	return m_nWidth * .6f;
}

float ZWnd::columnHalfWidth() const {
	return std::clamp( m_nWidth * .25f, 4.f, 32.f );
}

void ZWnd::Z_DrawGrid( bool text ){
	float step, minor_step, stepy;
	step = minor_step = stepy = GetGridSize();

	int minor_power = Grid_getPower();
	while ( ( minor_step * m_fScale ) <= 4.0f ) { // make sure minor grid spacing is at least 4 pixels on the screen
		++minor_power;
		minor_step *= 2;
	}

	int power = minor_power;
	while ( ( power % 3 ) != 0 || ( step * m_fScale ) <= 32.0f ) { // make sure major grid spacing is at least 32 pixels on the screen
		++power;
		step = pow( 2.0f, power );
	}

	const int mask = ( 1 << ( power - minor_power ) ) - 1;

	while ( ( stepy * m_fScale ) <= 32.0f ) // text step y must be at least 32
		stepy *= 2;

	const float h = ( m_nHeight / 2 / m_fScale );
	const float yb = step * floor( std::max( m_origin - h, g_region_mins[2] ) / step );
	const float ye = step * ceil( std::min( m_origin + h, g_region_maxs[2] ) / step );

	if ( text ) {
		gl().glColor4fv( vector4_to_array( Vector4( g_xywindow_globals.color_gridtext, 1 ) ) );
		const float fontDescent = ( GlobalOpenGL().m_font->getPixelDescent() - 1 ) / m_fScale;
		char label[32];
		for ( float y = yb - fmod( yb, stepy ); y <= ye; y += stepy ) {
			gl().glRasterPos2f( 4, y - fontDescent );
			sprintf( label, "%g", y );
			GlobalOpenGL().drawString( label );
		}
		return;
	}

	const float a = ( ( GetSnapGridSize() > 0 ) ? 1.0f : 0.3f );
	if ( a < 1.0f ) {
		gl().glEnable( GL_BLEND );
	}

	// draw minor blocks
	if ( g_xywindow_globals.color_gridminor != g_xywindow_globals.color_gridback ) {
		gl().glColor4fv( vector4_to_array( Vector4( g_xywindow_globals.color_gridminor, a ) ) );

		gl().glBegin( GL_LINES );
		int i = 0;
		for ( float y = yb; y < ye; y += minor_step, ++i ) {
			if ( ( i & mask ) != 0 ) {
				gl().glVertex2f( 0, y );
				gl().glVertex2f( m_nWidth, y );
			}
		}
		gl().glEnd();
	}

	// draw major blocks
	if ( g_xywindow_globals.color_gridmajor != g_xywindow_globals.color_gridminor ) {
		gl().glColor4fv( vector4_to_array( Vector4( g_xywindow_globals.color_gridmajor, a ) ) );

		gl().glBegin( GL_LINES );
		for ( float y = yb; y <= ye; y += step ) {
			gl().glVertex2f( 0, y );
			gl().glVertex2f( m_nWidth, y );
		}
		gl().glEnd();
	}

	if ( a < 1.0f ) {
		gl().glDisable( GL_BLEND );
	}
}

void ZWnd::Z_DrawColumns(){
	std::vector<ZBrushColumn> columns;
	collectColumns( columns );

	const float x1 = columnCenter() - columnHalfWidth();
	const float x2 = columnCenter() + columnHalfWidth();

	for ( const ZBrushColumn& column : columns )
	{
		gl().glColor3fv( vector3_to_array( column.colour ) );
		gl().glBegin( GL_QUADS );
		gl().glVertex2f( x1, column.zmin );
		gl().glVertex2f( x2, column.zmin );
		gl().glVertex2f( x2, column.zmax );
		gl().glVertex2f( x1, column.zmax );
		gl().glEnd();

		gl().glColor3fv( vector3_to_array( g_xywindow_globals.color_brushes ) );
		gl().glBegin( GL_LINE_LOOP );
		gl().glVertex2f( x1, column.zmin );
		gl().glVertex2f( x2, column.zmin );
		gl().glVertex2f( x2, column.zmax );
		gl().glVertex2f( x1, column.zmax );
		gl().glEnd();
	}

	// selected outlines last, so neighbours do not cover them
	gl().glLineWidth( 2 );
	gl().glColor3fv( vector3_to_array( g_xywindow_globals.color_selbrushes ) );
	for ( const ZBrushColumn& column : columns )
	{
		if ( column.selected ) {
			gl().glBegin( GL_LINE_LOOP );
			gl().glVertex2f( x1, column.zmin );
			gl().glVertex2f( x2, column.zmin );
			gl().glVertex2f( x2, column.zmax );
			gl().glVertex2f( x1, column.zmax );
			gl().glEnd();
		}
	}
	gl().glLineWidth( 1 );
}

void ZWnd::Z_DrawCameraIcon(){
	if ( g_pParentWnd->GetCamWnd() == 0 ) {
		return;
	}
	const float x = columnCenter();
	const float z = Camera_getOrigin( *g_pParentWnd->GetCamWnd() ).z();
	const double a = degrees_to_radians( Camera_getAngles( *g_pParentWnd->GetCamWnd() )[CAMERA_PITCH] );
	// sizes in pixels; divide vertical ones by the scale, since y is in world units
	const float fov = 48;
	const float box = 16;

	gl().glColor3fv( vector3_to_array( g_xywindow_globals.color_camera ) );
	gl().glBegin( GL_LINE_STRIP );
	gl().glVertex2f( x - box, z );
	gl().glVertex2f( x      , z + ( box / 2 ) / m_fScale );
	gl().glVertex2f( x + box, z );
	gl().glVertex2f( x      , z - ( box / 2 ) / m_fScale );
	gl().glVertex2f( x - box, z );
	gl().glVertex2f( x + box, z );
	gl().glEnd();

	gl().glBegin( GL_LINE_STRIP );
	gl().glVertex2f( x + fov * cos( a + c_pi / 4 ), z + fov * sin( a + c_pi / 4 ) / m_fScale );
	gl().glVertex2f( x, z );
	gl().glVertex2f( x + fov * cos( a - c_pi / 4 ), z + fov * sin( a - c_pi / 4 ) / m_fScale );
	gl().glEnd();
}

void ZWnd::Z_Draw(){
	updateProbePoint();

	gl().glViewport( 0, 0, m_nWidth, m_nHeight );
	gl().glClearColor( g_xywindow_globals.color_gridback[0],
	                   g_xywindow_globals.color_gridback[1],
	                   g_xywindow_globals.color_gridback[2], 0 );
	gl().glClear( GL_COLOR_BUFFER_BIT );

	// x in pixels from the left, y in world z
	const float h = ( m_nHeight / 2 / m_fScale );
	gl().glMatrixMode( GL_PROJECTION );
	gl().glLoadIdentity();
	gl().glOrtho( 0, m_nWidth, m_origin - h, m_origin + h, -1, 1 );

	gl().glMatrixMode( GL_MODELVIEW );
	gl().glLoadIdentity();

	gl().glDisable( GL_LINE_STIPPLE );
	gl().glLineWidth( 1 );
	gl().glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	gl().glDisableClientState( GL_NORMAL_ARRAY );
	gl().glDisableClientState( GL_COLOR_ARRAY );
	gl().glDisable( GL_TEXTURE_2D );
	gl().glDisable( GL_TEXTURE_1D );
	gl().glDisable( GL_LIGHTING );
	gl().glDisable( GL_COLOR_MATERIAL );
	gl().glDisable( GL_DEPTH_TEST );
	gl().glDisable( GL_BLEND );

	Z_DrawGrid( false );
	Z_DrawColumns();
	Z_DrawCameraIcon();
	Z_DrawGrid( true );
}


void Z_Shown_Construct( QWidget* parent ){
	g_z_shown.connect( parent );
}

void ZView_toggleShown(){
	if ( g_pParentWnd != 0 && g_pParentWnd->GetZWnd() != 0 ) {
		g_z_shown.toggle();
	}
}

void ZWnd_updateAll(){
	if ( g_pParentWnd != 0 && g_pParentWnd->GetZWnd() != 0 ) {
		ZWnd_Update( *g_pParentWnd->GetZWnd() );
	}
}

void ZWnd_selectionChanged( const Selectable& selectable ){
	ZWnd_updateAll();
}

void ZWnd_mapValidChanged(){
	if ( g_pParentWnd != 0 && g_pParentWnd->GetZWnd() != 0 ) {
		g_pParentWnd->GetZWnd()->onMapValidChanged();
	}
}

void ZWindow_Construct(){
	GlobalToggles_insert( "ToggleZView", makeCallbackF( ZView_toggleShown ), ToggleItem::AddCallbackCaller( g_z_shown.m_item ) );

	GlobalPreferenceSystem().registerPreference( "ZVIS", makeBoolStringImportCallback( ToggleShownImportBoolCaller( g_z_shown ) ), makeBoolStringExportCallback( ToggleShownExportBoolCaller( g_z_shown ) ) );

	// the callbacks look the view up, so that nothing refers to it after MainFrame::Shutdown
	AddSceneChangeCallback( FreeCaller<void(), ZWnd_updateAll>() );
	AddCameraMovedCallback( FreeCaller<void(), ZWnd_updateAll>() );
	GlobalSelectionSystem().addSelectionChangeCallback( FreeCaller<void(const Selectable&), ZWnd_selectionChanged>() );
	Map_addValidCallback( g_map, FreeCaller<void(), ZWnd_mapValidChanged>() );
}
