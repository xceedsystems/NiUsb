/***************************************************************************
                          lib/ibutil.c
                             -------------------

	copyright            : (C) 2001,2002 by Frank Mori Hess
    email                : fmhess@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE

//#include "ib_internal.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <pthread.h>
//#include <assert.h>
//#include "parse.h"

//ibConf_t *ibConfigs[ GPIB_CONFIGS_LENGTH ] = {NULL};
//ibConf_t ibFindConfigs[ FIND_CONFIGS_LENGTH ];
//
//void init_descriptor_settings( descriptor_settings_t *settings )
//{
//	settings->pad = -1;
//	settings->sad = -1;
//	settings->board = -1;
//	settings->usec_timeout = 3000000;
//	settings->spoll_usec_timeout = 1000000;
//	settings->ppoll_usec_timeout = 2;
//	settings->eos = 0;
//	settings->eos_flags = 0;
//	settings->ppoll_config = 0;
//	settings->send_eoi = 1;
//	settings->local_lockout = 0;
//	settings->readdr = 0;
//}
//
//void init_ibconf( ibConf_t *conf )
//{
//	conf->handle = -1;
//	memset(conf->name, 0, sizeof(conf->name));
//	init_descriptor_settings( &conf->defaults );
//	init_descriptor_settings( &conf->settings );
//	memset(conf->init_string, 0, sizeof(conf->init_string));
//	conf->flags = 0;
//	init_async_op( &conf->async );
//	conf->end = 0;
//	conf->is_interface = 0;
//	conf->board_is_open = 0;
//	conf->has_lock = 0;
//	conf->timed_out = 0;
//}



int ibstatus( ibConf_t *conf, int error, int clear_mask, int set_mask )
{
	int status = 0;
	int retval;

	retval = my_wait( conf, 0, clear_mask, set_mask, &status);
	if( retval < 0 ) error = 1;

	if( error ) status |= ERR;
	if( conf->timed_out )
		status |= TIMO;
	if( conf->end )
		status |= END;

	setIbsta( status );

	return status;
}

int exit_library( int ud, int error )
{
	return general_exit_library( ud, error, 0, 0, 0, 0, 0 );
}

int general_exit_library( int ud, int error, int no_sync_globals, int no_update_ibsta,
	int status_clear_mask, int status_set_mask, int no_unlock_board )
{
	ibConf_t *conf = ibConfigs[ ud ];
	ibBoard_t *board;
	int status;

	if( ibCheckDescriptor( ud ) < 0 )
	{
		setIbsta( ERR );
		if( no_sync_globals == 0 )
			sync_globals();
		return ERR;
	}

	board = interfaceBoard( conf );

	if( no_update_ibsta )
		status = ThreadIbsta();
	else
		status = ibstatus( conf, error, status_clear_mask, status_set_mask );

	if( no_unlock_board == 0 && conf->has_lock )
		conf_unlock_board( conf );

	if( no_sync_globals == 0 )
		sync_globals();

	return status;
}


int addressListIsValid( const Addr4882_t addressList[] )
{
	int i;

	if( addressList == NULL ) return 1;

	for( i = 0; addressList[ i ] != NOADDR; i++ )
	{
		if( addressIsValid( addressList[ i ] ) == 0 )
		{
			setIbcnt( i );
			return 0;
		}
	}

	return 1;
}

unsigned int numAddresses( const Addr4882_t addressList[] )
{
	unsigned int count;

	if( addressList == NULL )
		return 0;

	count = 0;
	while( addressList[ count ] != NOADDR )
	{
		count++;
	}

	return count;
}

int is_cic( const ibBoard_t *board )
{
	int retval;
	wait_ioctl_t cmd;

	cmd.usec_timeout = 0;
	cmd.wait_mask = 0;
	cmd.clear_mask = 0;
	cmd.set_mask = 0;
	cmd.pad = NOADDR;
	cmd.sad = NOADDR;
	cmd.handle = 0;
	cmd.ibsta = 0;
	retval = ioctl( board->fileno, IBWAIT, &cmd );
	if( retval < 0 )
	{
		setIberr( EDVR );
		setIbcnt( errno );
		fprintf( stderr, "libgpib: error in is_cic()!\n");
		return -1;
	}

	if( cmd.ibsta & CIC )
		return 1;

	return 0;
}

int is_system_controller( const ibBoard_t *board )
{
	int retval;
	board_info_ioctl_t info;

	retval = ioctl( board->fileno, IBBOARD_INFO, &info );
	if( retval < 0 )
	{
		fprintf( stderr, "libgpib: error in is_system_controller()!\n");
		return retval;
	}

	return info.is_system_controller;
}
