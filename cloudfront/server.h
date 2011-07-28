// Copyright (C) 2011-2012 Copious Systems
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of FLAX DB
//
// The exception is that, if you link the FLAX library with other
// files to produce an executable, this does not by itself cause the
// resulting executable to be covered by the GNU General Public License.
// Your use of that executable is in no way restricted on account of
// linking the FLAX DB library code into it.
//
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.
//
// This exception applies only to the code released under the
// name Common C++.  If you copy code from other releases into a copy of
// Common C++, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for FLAX it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.



#ifndef FLAX_CLOUD_SERVER_H
#define FLAX_CLOUD_SERVER_H



#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <memory.h>
#include <errno.h>

#include <netinet/ip6.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


#include <netdb.h>
#include <unistd.h>
#include <string.h>



#define FALSE 0
#define TRUE 1



#define MAX_HOST_CHARS 		128

#define MAX_EVENTS 			10
#define IP_LISTEN_BACKLOG 	100

#define MAX_DESCRIPTORS_PROCESSING	1000

#define APP_EPOLL_WAIT_FOREVER  (-1)
#define APP_EPOLL_DONT_WAIT  (0)

#define MAX_THREADS			100

#define MAX_READ_BUFFER		2048


#define FOREVER				(1 == 1)


#define socket_errno errno

	/// an enum from commonC++
typedef enum {
		errSuccess = 0,
		errCreateFailed,
		errCopyFailed,
		errInput,
		errInputInterrupt,
		errResourceFailure,
		errOutput,
		errOutputInterrupt,
		errNotConnected,
		errConnectRefused,
		errConnectRejected,
		errConnectTimeout,
		errConnectFailed,
		errConnectInvalid,
		errConnectBusy,
		errConnectNoRoute,
		errBindingFailed,
		errBroadcastDenied,
		errRoutingDenied,
		errKeepaliveDenied,
		errServiceDenied,
		errServiceUnavailable,
		errMulticastDisabled,
		errTimeout,
		errNoDelay,
		errExtended,
		errLookupFail,
		errSearchErr,
		errInvalidValue
	} Sock_Error;


typedef struct {
	int				n_closed;
	int				closed_sockets[MAX_DESCRIPTORS_PROCESSING];
		///-------------------------->
	int				n_ready;
	int				ready_sockets[MAX_DESCRIPTORS_PROCESSING];
} st_spent_sockets;



typedef struct FQEL {
	int				_feeder_sockets[MAX_EVENTS];
	int				_count;
	struct FQEL		*_next;
} st_feeder_queue_el;


typedef struct st_feeder_queue_el {

	st_feeder_queue_el		*last;
	st_feeder_queue_el		*head;

} st_feeder_queue;

typedef struct {

	st_feeder_queue			_data_ready_elements;
	
	st_spent_sockets		_socket_management;
	int						_has_spent_sockets;
	
	int						_in_process[MAX_DESCRIPTORS_PROCESSING];
	int						_in_process_count;
} st_streamer_thread_argument;


typedef struct {

  int		_thread_id;	// which thread this is in application thread counting.
  void		*_data;		// data for operations.
 
  int		_sock_fd;  // if needed

} st_thread_data;


#endif

