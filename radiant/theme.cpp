/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

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

#include <QStyle>
#include <QApplication>
#include <QMenu>
#include <QActionGroup>
#include <QFile>
#include <QDir>

#include "preferences.h"
#include "mainframe.h"
#include "preferencesystem.h"
#include "stringio.h"
#include "stream/stringstream.h"
#include "gtkutil/image.h"


enum class ETheme{
	Default = 0,
	Fusion,
	Dark,
	Darker,
	GreyBlue2,
	BlenderDark,
	BlenderLight,
	GtkRadiant14,
};

static ETheme s_theme = ETheme::Dark;

QString load_qss( const char *filename ){
	if( QFile file( QString( AppPath_get() ) + "themes/" + filename ); file.open( QIODevice::OpenModeFlag::ReadOnly ) )
		return file.readAll();
	return {};
}

void set_icon_theme( bool light ){
	static auto init = ( Bitmaps_generateLight( AppPath_get(), SettingsPath_get() ),
	                     QIcon::setThemeSearchPaths( QIcon::themeSearchPaths() << AppPath_get() << SettingsPath_get() ), 1 );
	(void)init;

	BitmapsPath_set( light? StringStream( SettingsPath_get(), "bitmaps_light/" )
	                      : StringStream( AppPath_get(), "bitmaps/" ) );
	QIcon::setThemeName( light? "bitmaps_light" : "bitmaps" );
}

void theme_set( ETheme theme ){
	s_theme = theme;
#ifdef WIN32
//	QSettings settings( "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat );
//	if( settings.value( "AppsUseLightTheme" ) == 0 )
#endif
	static struct
	{
		bool is1stThemeApplication = true; // guard to not apply possibly wrong defaults while app is started with Default theme
		const QPalette palette = qApp->palette();
		const QString style = qApp->style()->objectName();
	}
	defaults;

	if( theme == ETheme::Default ){
		set_icon_theme( true );
		if( !defaults.is1stThemeApplication ){
			qApp->setPalette( defaults.palette );
			qApp->setStyleSheet( QString() );
			qApp->setStyle( defaults.style );
		}
	}
	else if( theme == ETheme::Fusion ){
		set_icon_theme( true );
		qApp->setPalette( defaults.palette );
		qApp->setStyleSheet( load_qss( "fusion.qss" ) ); //missing, stub to load custom qss
		qApp->setStyle( "Fusion" );
	}
	else if( theme == ETheme::Dark ){
		set_icon_theme( false );
		qApp->setStyle( "Fusion" );
		QPalette darkPalette;
		const QColor darkColor = QColor( 83, 84, 81 );
		const QColor disabledColor = QColor( 127, 127, 127 );
		const QColor baseColor( 46, 52, 54 );
		darkPalette.setColor( QPalette::Window, darkColor );
		darkPalette.setColor( QPalette::WindowText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::WindowText, disabledColor );
		darkPalette.setColor( QPalette::Base, baseColor );
		darkPalette.setColor( QPalette::AlternateBase, baseColor.darker( 130 ) );
		darkPalette.setColor( QPalette::ToolTipBase, Qt::white );
		darkPalette.setColor( QPalette::ToolTipText, Qt::white );
		darkPalette.setColor( QPalette::Text, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::Text, disabledColor );
		darkPalette.setColor( QPalette::Disabled, QPalette::Light, disabledColor ); // disabled menu text shadow
		darkPalette.setColor( QPalette::Button, darkColor.lighter( 130 ) ); //<>
		darkPalette.setColor( QPalette::ButtonText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::ButtonText, disabledColor.lighter( 130 ) ); //<>
		darkPalette.setColor( QPalette::BrightText, Qt::red );
		darkPalette.setColor( QPalette::Link, QColor( 42, 130, 218 ) );

		darkPalette.setColor( QPalette::Highlight, QColor( 250, 203, 129 ) ); //<>
		darkPalette.setColor( QPalette::Inactive, QPalette::Highlight, disabledColor );
		darkPalette.setColor( QPalette::HighlightedText, Qt::black );
		darkPalette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabledColor );

		qApp->setPalette( darkPalette );

		qApp->setStyleSheet( load_qss( "dark.qss" ) );
	}
	else if( theme == ETheme::Darker ){
		set_icon_theme( false );
		qApp->setStyle( "Fusion" );
		QPalette darkPalette;
		const QColor darkColor = QColor( 45, 45, 45 );
		const QColor disabledColor = QColor( 127, 127, 127 );
		const QColor baseColor( 18, 18, 18 );
		darkPalette.setColor( QPalette::Window, darkColor );
		darkPalette.setColor( QPalette::WindowText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::WindowText, disabledColor );
		darkPalette.setColor( QPalette::Base, baseColor );
		darkPalette.setColor( QPalette::AlternateBase, baseColor.darker( 130 ) );
		darkPalette.setColor( QPalette::ToolTipBase, Qt::white );
		darkPalette.setColor( QPalette::ToolTipText, Qt::white );
		darkPalette.setColor( QPalette::Text, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::Text, disabledColor );
		darkPalette.setColor( QPalette::Disabled, QPalette::Light, disabledColor ); // disabled menu text shadow
		darkPalette.setColor( QPalette::Button, darkColor );
		darkPalette.setColor( QPalette::ButtonText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::ButtonText, disabledColor );
		darkPalette.setColor( QPalette::BrightText, Qt::red );
		darkPalette.setColor( QPalette::Link, QColor( 42, 130, 218 ) );

		darkPalette.setColor( QPalette::Highlight, QColor( 42, 130, 218 ) );
		darkPalette.setColor( QPalette::Inactive, QPalette::Highlight, disabledColor );
		darkPalette.setColor( QPalette::HighlightedText, Qt::black );
		darkPalette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabledColor );

		qApp->setPalette( darkPalette );

		qApp->setStyleSheet( load_qss( "darker.qss" ) );
	}
	else if( theme == ETheme::GreyBlue2 ){
		set_icon_theme( false );
		qApp->setStyle( "Fusion" );
		qApp->setPalette( defaults.palette );
		QDir::addSearchPath( "julius", AppPath_get() ); // qss image paths prefix to avoid dependency on cwd
		qApp->setStyleSheet( load_qss( "greyblue2.qss" ) );
	}
	else if( theme == ETheme::BlenderDark ){
		set_icon_theme( false );
		qApp->setStyle( "Fusion" );
		qApp->setPalette( defaults.palette );
		QDir::addSearchPath( "julius", AppPath_get() ); // qss image paths prefix to avoid dependency on cwd
		qApp->setStyleSheet( load_qss( "blenderdark.qss" ) );
	}
	else if( theme == ETheme::BlenderLight ){
		set_icon_theme( true );
		qApp->setStyle( "Fusion" );
		qApp->setPalette( defaults.palette );
		QDir::addSearchPath( "julius", AppPath_get() ); // qss image paths prefix to avoid dependency on cwd
		qApp->setStyleSheet( load_qss( "blenderlight.qss" ) );
	}
	else if( theme == ETheme::GtkRadiant14 ){ // colors of the default GTK+ 2 engine, which GtkRadiant 1.3/1.4 used
		set_icon_theme( true );
		qApp->setStyle( "Fusion" );
		QPalette palette;
		const QColor normalColor( 0xd6, 0xd6, 0xd6 );
		const QColor prelightColor( 0xee, 0xee, 0xee );
		const QColor activeColor( 0xc3, 0xc3, 0xc3 );
		const QColor selectedColor( 0x4b, 0x69, 0x83 );
		const QColor disabledColor( 0x75, 0x75, 0x75 );
		palette.setColor( QPalette::Window, normalColor );
		palette.setColor( QPalette::WindowText, Qt::black );
		palette.setColor( QPalette::Base, Qt::white );
		palette.setColor( QPalette::AlternateBase, prelightColor );
		palette.setColor( QPalette::ToolTipBase, QColor( 0xff, 0xff, 0xc0 ) );
		palette.setColor( QPalette::ToolTipText, Qt::black );
		palette.setColor( QPalette::Text, Qt::black );
		palette.setColor( QPalette::Button, normalColor );
		palette.setColor( QPalette::ButtonText, Qt::black );
		palette.setColor( QPalette::BrightText, Qt::white );
		palette.setColor( QPalette::Link, QColor( 0, 0, 0xee ) );
		for( const auto role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText } )
			palette.setColor( QPalette::Disabled, role, disabledColor );

		palette.setColor( QPalette::Light, Qt::white ); // also disabled menu text shadow, as in GTK
		palette.setColor( QPalette::Midlight, prelightColor );
		palette.setColor( QPalette::Mid, activeColor );
		palette.setColor( QPalette::Dark, QColor( 0x96, 0x96, 0x96 ) );
		palette.setColor( QPalette::Shadow, Qt::black );

		palette.setColor( QPalette::Highlight, selectedColor );
		palette.setColor( QPalette::HighlightedText, Qt::white );
		palette.setColor( QPalette::Inactive, QPalette::Highlight, QColor( 0xc0, 0xc0, 0xc0 ) );
		palette.setColor( QPalette::Inactive, QPalette::HighlightedText, Qt::black );

		qApp->setPalette( palette );
		QDir::addSearchPath( "gtkr", AppPath_get() ); // qss image paths prefix to avoid dependency on cwd
		qApp->setStyleSheet( load_qss( "gtkradiant14.qss" ) );
	}

	defaults.is1stThemeApplication = false;
}

void theme_construct_menu( class QMenu *menu ){
	auto *m = menu->addMenu( "GUI Theme" );
	m->setTearOffEnabled( g_Layout_enableDetachableMenus.m_value );
	auto *group = new QActionGroup( m );

	for( const auto *name : { "Default", "Fusion", "Dark", "Darker", "GreyBlue2", "BlenderDark", "BlenderLight", "GtkRadiant 1.4" } )
	{
		auto *a = m->addAction( name );
		a->setCheckable( true );
		group->addAction( a );
	}
	// init radio
	if( const int value = static_cast<int>( s_theme ); 0 <= value && value < group->actions().size() )
		group->actions().at( value )->setChecked( true );

	QObject::connect( group, &QActionGroup::triggered, []( QAction *action ){
		theme_set( static_cast<ETheme>( action->actionGroup()->actions().indexOf( action ) ) );
	} );
}

void ThemeImport( int value ){
	s_theme = static_cast<ETheme>( value );
}
typedef FreeCaller<void(int), ThemeImport> ThemeImportCaller;

void ThemeExport( const IntImportCallback& importer ){
	importer( static_cast<int>( s_theme ) );
}
typedef FreeCaller<void(const IntImportCallback&), ThemeExport> ThemeExportCaller;


void theme_construct(){
	theme_set( s_theme ); // set theme here, not in importer, so it's set on the very 1st start too (when there is no preference to load)
}

void theme_registerGlobalPreference( class PreferenceSystem& preferences ){
	preferences.registerPreference( "GUITheme", makeIntStringImportCallback( ThemeImportCaller() ), makeIntStringExportCallback( ThemeExportCaller() ) );
}
