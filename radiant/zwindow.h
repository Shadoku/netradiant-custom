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
class BrushInstance;

/// \brief The vertical extent of one brush along the Z view's sample line.
struct ZBrushColumn
{
	BrushInstance* brush;
	float zmin, zmax;
	Vector3 colour; // texture colour of the top face
	bool selected;
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
	void SetProbePoint( const Vector3& point );
	/// \brief Phase 1: the probe point follows the camera.
	bool m_probeFollowsCamera;
	void updateProbePoint();

	/// \brief Collects the visible brushes the sample line passes through, largest interval first.
	void collectColumns( std::vector<ZBrushColumn>& columns ) const;

	void Z_Draw();

	void Z_MouseDown( int x, int y, unsigned int buttons );
	void Z_MouseUp( int x, int y, unsigned int buttons );
	void Z_MouseMoved( int x, int y, unsigned int buttons );

private:
	float m_origin;
	float m_fScale;
	Vector3 m_probe;

	bool m_move_started;
	int m_move_y;

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

void ZWindow_Construct();
