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

#pragma once

#include <vector>
#include "math/vector.h"
#include "map.h"

class QWidget;
class QObject;
class BrushInstance;
class Face;

/// \brief The vertical extent of one brush along the Z view's sample line.
struct ZBrushColumn
{
	BrushInstance* brush;
	float zmin, zmax;
	Vector3 colour; // texture colour of the top face
	bool selected;
	Face* top;    // the faces the sample line leaves and enters the brush through
	Face* bottom;
	scene::Instance* instance; // what the column selects: the brush, or a selected point entity ( then brush is null )
};

/// \brief Z view: shows the vertical extent of the brushes under the probe point, and the camera height.
/// Screen x is in device pixels from the left; screen y is in device pixels from the top.
class ZWnd
{
	QWidget* m_gl_widget;

	DeferredDraw m_deferredDraw;

public:
	QWidget* m_parent; // floating window, or 0 when docked
	ZWnd();
	~ZWnd();

	void queueDraw(){
		m_deferredDraw.draw();
	}
	void onMapValidChanged();
	QWidget* GetWidget(){
		return m_gl_widget;
	}

	int m_nWidth;
	int m_nHeight;

	/// \brief World Z at the vertical centre of the view.
	float GetOrigin() const {
		return m_origin;
	}
	void SetOrigin( float z );
	void Scroll( int y );

	/// \brief Pixels per world unit.
	float Scale() const {
		return m_fScale;
	}
	void SetScale( float f );
	void ZoomInWithMouse( int y );
	void ZoomOutWithMouse( int y );

	float ScreenToZ( float y ) const {
		return m_origin + ( m_nHeight * .5f - y ) / m_fScale;
	}
	float ZToScreen( float z ) const {
		return m_nHeight * .5f - ( z - m_origin ) * m_fScale;
	}

	/// \brief The vertical sample line passes through this point; only x and y are used.
	const Vector3& GetProbePoint() const {
		return m_probe;
	}
	/// \brief Places the probe point and stops it following the camera.
	void SetProbePoint( const Vector3& point );
	void updateProbePoint();

	/// \brief Collects the visible brushes the sample line passes through, largest interval first.
	void collectColumns( std::vector<ZBrushColumn>& columns ) const;

	void Z_Draw();

	void Z_MouseDown( int x, int y, unsigned int buttons );
	void Z_MouseUp( int x, int y, unsigned int buttons );
	void Z_MouseMoved( int x, int y, unsigned int buttons );
	/// \brief Restores the state from before the current drag, if any; the drag then does nothing until the button is released.
	void Z_CancelDrag();

private:
	float m_origin;
	float m_fScale;
	Vector3 m_probe;

	bool m_move_started;
	int m_move_y;

	enum EDrag
	{
		eDragNone,
		eDragPending,   // button pressed, mouse not moved yet: a click selects
		eDragMove,      // moving the selection vertically
		eDragFace,      // moving the top or bottom face of one brush
		eDragCancelled, // cancelled; waiting for the button release
	};
	enum EDragAction
	{
		eActionNone,
		eActionMove,
		eActionSelectMove,
		eActionFace,
	};
	EDrag m_drag;
	EDragAction m_drag_action;
	int m_drag_y;
	float m_drag_z;               // world z under the press
	float m_drag_offset;          // applied vertical offset, after snapping and clamping
	BrushInstance* m_drag_brush;  // face drag
	scene::Instance* m_drag_instance; // what to select for eActionSelectMove
	Face* m_drag_face;
	bool m_drag_top;
	QObject* m_drag_keyFilter;

	bool findEdge( const std::vector<ZBrushColumn>& columns, int y, const ZBrushColumn*& column, bool& top ) const;
	void dragBegin();
	void dragUpdate( int y );
	void dragEnd();
	void moveSelection( float offset );
	void moveFace( float offset );
	void selectClick( int y, unsigned int buttons );
	void Z_DrawDragInfo();

	void Z_DrawGrid( bool text );
	void Z_DrawColumns();
	void Z_DrawCameraIcon();
	float columnCenter() const;
	float columnHalfWidth() const;
};

inline void ZWnd_Update( ZWnd& zwnd ){
	zwnd.queueDraw();
}

void Z_Shown_Construct( QWidget* parent );

/// \brief Whether the probe point can be placed from the XY view: the Z view exists and is shown.
bool ZWnd_probeEditable();
void ZWnd_setProbePoint( const Vector3& point );
/// \brief Draws the probe marker in an XY (top) view, whose modelview is in world units; \p scale is its pixels per unit.
void ZWnd_drawProbeMarker( float scale );

void ZWindow_Construct();
