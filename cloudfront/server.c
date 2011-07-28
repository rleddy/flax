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


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "server.h"


//======================== MOVE TO SEPARATE FILE

pthread_mutex_t	feeder_mutex;
pthread_mutex_t	spent_socket_mutex;

pthread_cond_t	feeder_data_ready;
pthread_mutex_t	feeder_data_ready_mutex;




static st_spent_sockets				sg_socket_management;

static st_thread_data				sg_streamer;
static st_thread_data				sg_workers[MAX_THREADS];

static pthread_t					sg_threads[MAX_THREADS+1];
static int 							sg_thread_count = 0;

static st_streamer_thread_argument	sg_streamer_thread_parameter;




void
enter_feeder_mutex(void) {
	pthread_mutex_lock(&feeder_mutex);
}


void
exit_feeder_mutex(void) {
	pthread_mutex_unlock(&feeder_mutex);
}

///--------------------------------->>

void
enter_feeder_data_ready_mutex(void) {
	pthread_mutex_lock(&feeder_mutex);
}


void
exit_feeder_data_ready_mutex(void) {
	pthread_mutex_unlock(&feeder_mutex);
}

void
wait_feeder_data_ready(void) {
	pthread_cond_wait(&feeder_data_ready, &feeder_data_ready_mutex);
}


///--------------------------------->>

void
enter_spent_socket_mutex(void) {
	pthread_mutex_lock(&spent_socket_mutex);
}


void
exit_spent_socket_mutex(void) {
	pthread_mutex_unlock(&spent_socket_mutex);
}

///--------------------------------->>


extern void *stream_manager(void *threadarg);


void
initialize_stream_manager(void) {
	
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&feeder_mutex, &attr);
	pthread_mutex_init(&spent_socket_mutex, &attr);
	pthread_mutex_init(&feeder_data_ready_mutex, &attr);
	pthread_cond_init(&feeder_data_ready, NULL);

		///--------------------------->>>
	memset((void *)(&sg_streamer),0,sizeof(st_thread_data));
	memset((void *)(&sg_workers),0,sizeof(st_thread_data)*MAX_THREADS);
	memset((void *)(&sg_threads),0,sizeof(pthread_t)*(MAX_THREADS+1));
	memset((void *)(&sg_socket_management),0,sizeof(st_spent_sockets));
	memset((void *)(&sg_streamer_thread_parameter),0,sizeof(st_streamer_thread_argument));


	int status = 0;

	sg_streamer._thread_id = 0;
	sg_streamer._data  = (void *)(&sg_streamer_thread_parameter);		// data for operations.
	sg_streamer._sock_fd = 0;  		// Handles many sockets

	status = pthread_create(&sg_threads[sg_thread_count++], NULL, stream_manager, (void *)(&sg_streamer) );

	if ( status ) {
		printf("ERROR; return code from pthread_create() is %d\n", status);
		exit(EXIT_FAILURE);
	}

}





st_spent_sockets *
process_input_data(int *fds,int nfds) {  /// make socekts available to the stream manager....

	static st_spent_sockets				local_socket_updater;
	///---------------->>

	st_feeder_queue_el *fqel = (st_feeder_queue_el *)malloc(sizeof(st_feeder_queue_el));

	memcpy(fqel->_feeder_sockets,fds,MAX_EVENTS*sizeof(int));
	fqel->_count = nfds;

	enter_feeder_mutex();

	{		/// add to the head of the queue.
		fqel->_next = sg_streamer_thread_parameter._data_ready_elements.head;
		sg_streamer_thread_parameter._data_ready_elements.head = fqel;
		if ( sg_streamer_thread_parameter._data_ready_elements.last == NULL ) {
			sg_streamer_thread_parameter._data_ready_elements.last = fqel;
		}
	}

	exit_feeder_mutex();

	pthread_cond_broadcast(&feeder_data_ready);



	if ( sg_streamer_thread_parameter._has_spent_sockets ) {
			///----------------------------------->>
		enter_spent_socket_mutex();

		memcpy((void *)(&local_socket_updater),(void *)(&sg_streamer_thread_parameter._socket_management),sizeof(st_spent_sockets));
		sg_streamer_thread_parameter._has_spent_sockets = 0;
		memset((void *)(&sg_streamer_thread_parameter._socket_management),0,sizeof(st_spent_sockets));

		exit_spent_socket_mutex();
			///----------------------------------->>

		return(&local_socket_updater);
	}

	return(NULL);
}



void
handle_urgent_data(int fd) {
	
}





	/// This is very application specific....
int
check_for_close_command(char *buffer) {
	if ( strncmp(buffer,"{{END}}",7) == 0 ) {
		return(1);
	}
	return(0);
}



int
act_on_data(char *data_buffer,int sock_key) { /// use the socket as a key to writing process.
	int status = FALSE;
		/// do something useful here.
	return(status);
}



void *
stream_manager(void *threadarg) {

	if ( threadarg != NULL ) {

		/// Socket index
		st_thread_data *parameters = (st_thread_data *)threadarg;
		st_streamer_thread_argument *shared_data = (st_streamer_thread_argument *)parameters->_data;  ///  shared_data ... data for feeding the sockets through

		while ( FOREVER ) {

			enter_feeder_data_ready_mutex();
			{
				if ( ( shared_data->_data_ready_elements.head == NULL ) && ( shared_data->_in_process_count == 0 ) ) {
					/// wait ....
					wait_feeder_data_ready();
				}
			}
			exit_feeder_data_ready_mutex();

			st_feeder_queue_el *fqel = NULL;
			enter_feeder_mutex();
			{
					/// remove from the end of the queue. Assume queues are pretty short.
				st_feeder_queue_el *fqel_prev = shared_data->_data_ready_elements.head;
				fqel = shared_data->_data_ready_elements.last;

				if ( fqel_prev != NULL ) {

					if ( fqel_prev == fqel ) {
						shared_data->_data_ready_elements.head = NULL;
						shared_data->_data_ready_elements.last = NULL;
					} else {
						while ( fqel_prev->_next != fqel ) {
								///------------->>
							fqel_prev = fqel_prev->_next;
						}
						shared_data->_data_ready_elements.last = fqel_prev;
						fqel_prev->_next = NULL;
					}
				}
			}
			exit_feeder_mutex();

			if ( fqel != NULL ) {
				/// add sockets to processing sockets
				int n = fqel->_count;
				int i = 0;
				for ( ; i < n; i++ ) {
					if ( shared_data->_in_process_count < MAX_DESCRIPTORS_PROCESSING ) {
						int sock_fd = fqel->_feeder_sockets[i];
						shared_data->_in_process[shared_data->_in_process_count++] = sock_fd;
					} else break;
				}
			}

			{

				int n = shared_data->_in_process_count;
				int *sock_ptr = &(shared_data->_in_process[0]);
				int *sock_end = sock_ptr + n;
				while ( sock_ptr < sock_end ) {
					
					int sock_fd = *sock_ptr;
					int closing = 0;
					int result = EINTR;

					char data_buffer[MAX_READ_BUFFER];  // data buffer

					while ( result == EINTR ) {

					/// -----  result = read

					}

					if ( ( result == EAGAIN ) || ( result == EWOULDBLOCK ) ) {

						enter_spent_socket_mutex();

						*sock_ptr = -1;
						shared_data->_socket_management.ready_sockets[shared_data->_socket_management.n_ready++] = sock_fd;
						shared_data->_has_spent_sockets = 1;

						exit_spent_socket_mutex();

						/// write a coast is clear.....  assume it is in the data buffer

					} else {

						closing = ( result == ENOTCONN ) || ( result == ENOTSOCK );
										///------------------>>
						closing |= check_for_close_command(data_buffer);

						if ( closing ) {

							enter_spent_socket_mutex();

							*sock_ptr = -1;
							shared_data->_socket_management.closed_sockets[shared_data->_socket_management.n_closed++] = sock_fd;
							shared_data->_has_spent_sockets = 1;

							exit_spent_socket_mutex();

						} else {
							/// ---   ACTION ON DATA      --------
							if ( act_on_data(data_buffer,sock_fd) ) {

								/// write a response.....  assume it is in the data buffer
								/// this does not have to happen every read. But, let app decide

							}
							/// ---  END ACTION ON DATA      --------
						}
					}
				}

				enter_spent_socket_mutex();
				{
					int i = 0;

					n = shared_data->_in_process_count;
					for ( ; i< n; i++ ) {
						int marker = shared_data->_socket_management.ready_sockets[i];
						while ( marker == -1 ) {
							int mover = shared_data->_socket_management.ready_sockets[n-1];
							marker = mover;
							n--;
							if ( n <= i ) break;
						}
						if ( n > i ) {
							shared_data->_socket_management.ready_sockets[i] = marker;
						}
					}

					shared_data->_in_process_count = n;

				}
				exit_spent_socket_mutex();

				
			}
		}
	}


	pthread_exit(NULL);
	return(NULL);
}



/// Add an operating thread,,, theses are for jobs outside the streamer function.

int
add_thread(void * (* thread_function)(void *threadarg),void *data,int sockfd) {

	if ( (sg_thread_count + 1) < MAX_THREADS ) {

		int status = 0;

		st_thread_data *ct = &sg_workers[sg_thread_count++];

		ct->_thread_id = sg_thread_count;
		ct->_data  = (void *)data;		// data for operations.
		ct->_sock_fd = sockfd;  		// Handles many sockets

		status = pthread_create(&(sg_threads[sg_thread_count]), NULL, thread_function, (void *)(ct) );

		if ( status ) {
			printf("ERROR; return code from pthread_create() is %d\n", status);
			return(FALSE);
		}

		return(TRUE);

	}

	return(FALSE);
}



//========================


void
setnonblocking(int so) {
	/// FCNTL
	int fflags = fcntl(so, F_GETFL);

	fflags |= O_NONBLOCK;
	
	fcntl(so, F_SETFL, fflags);

}


void
setblocking(int so) {
	/// FCNTL
	int fflags = fcntl(so, F_GETFL);

	fflags &=~ O_NONBLOCK;
	fcntl(so, F_SETFL, fflags);
}





int
set_ip_address(const char *host,struct in6_addr *l_addr) {
	
	if ( !host ) {
		return FALSE;
	}

	int ok = inet_pton(AF_INET6, host, l_addr);

	if ( ok < 0 ) {
		perror("Invalid Internet FAMILY");
		exit(EXIT_FAILURE);
	}

	if ( !ok ) {
		return FALSE;
	}

	return TRUE;
}



struct addrinfo *
get_addresses(char *port) {

	struct addrinfo hints;
	struct addrinfo *result;
	int sfd, s, j;
	size_t len;
	ssize_t nread;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(NULL, port, &hints, &result);

	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	return(result);
}






int
main(int argc, char **argv) {

	if ( argc == 1 ) {
		printf("A commmand line parameter is required: port \n");
		exit(EXIT_FAILURE);
	} 

	char *port = argv[1];


	struct addrinfo *rp = NULL, *results = get_addresses(port);
	
	if ( results == NULL ) {
		perror("server address");
		exit(EXIT_FAILURE);

	}

	int listen_sock = -1;

		/// Walk through the list of results untili one of them successfully binds.
		
	for ( rp = results; rp != NULL; rp = rp->ai_next ) {

		listen_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if ( listen_sock == -1 ) continue;

		int opt = 1;
		setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, (socklen_t)sizeof(opt));

		if ( bind(listen_sock, rp->ai_addr, rp->ai_addrlen) == 0 ) { break; } /* Success */

		close(listen_sock);
 
	}

	if ( rp == NULL ) {               /* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(results);           /* No longer needed */


	if ( listen_sock == -1 ) {

		perror("create server socket");
		exit(EXIT_FAILURE);

	}


	{
		//setSegmentSize(mss);

		if ( listen(listen_sock, IP_LISTEN_BACKLOG) ) {
			close(listen_sock);
			error(EXIT_FAILURE,errBindingFailed,(char *)"Could not listen on socket",socket_errno);
		}

	}

	/// OK READY TO GO... SET UP A THREAD FOR PROCESSING DATAs

	initialize_stream_manager();


	{
		struct epoll_event ev, events[MAX_EVENTS];  /// An event in the sense that data has arrived
		int epoll_fd;


		epoll_fd = epoll_create1(0);

		if ( epoll_fd == -1 ) {
			perror("epoll_create");
			exit(EXIT_FAILURE);
		}

		ev.events = EPOLLIN;
		ev.data.fd = listen_sock;

		if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1 ) {
			perror("epoll_ctl: listen_sock");
			exit(EXIT_FAILURE);
		}

		while ( FOREVER ) {

			int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, APP_EPOLL_WAIT_FOREVER);

			if ( nfds == -1 ) {
				perror("epoll_pwait");
				exit(EXIT_FAILURE);
			}

			{

				int count_ready_data = 0;;
				int ready_data_descriptors[MAX_EVENTS];

				struct epoll_event *evs = &events[0];
				struct epoll_event *evs_end = evs + nfds;

				memset((void *)ready_data_descriptors,0,(MAX_EVENTS)*sizeof(int));


				while ( evs < evs_end ) {

					if ( evs->data.fd == listen_sock ) {  /// The socket found is server listner

						struct sockaddr local;
						socklen_t addrlen;

						int conn_sock;

						conn_sock = accept(listen_sock, &local, &addrlen);	/// connect with the client, creating client socket

						if ( conn_sock == -1 ) {
							perror("accept");
							exit(EXIT_FAILURE);
						}

						setnonblocking(conn_sock);
						ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;   // edge triggered for these non blocking connections, as may be used by streaming.
						ev.data.fd = conn_sock;

						if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1 ) {

							perror("epoll_ctl: conn_sock EPOLL_CTL_ADD");
							exit(EXIT_FAILURE);

						}

					} else {

						/// save up the descriptors in order to pass it on to a worker thread.
						/// Do this unless there are conditions ending communication.

						if ( ( evs->events & EPOLLRDHUP ) || ( evs->events & EPOLLERR ) || ( evs->events & EPOLLHUP ) ) {  /// hang up on peer

							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs->data.fd, NULL);
							close(evs->data.fd);

						} else if ( evs->events & EPOLLPRI ) {

							/// priority data ... process it separately...
							handle_urgent_data(evs->data.fd);

						} else { 
																	///--<<
							ready_data_descriptors[count_ready_data] = evs->data.fd;
						}

					}

					evs++;

				}

				if ( count_ready_data > 0 ) {

					st_spent_sockets *spent_descriptors = process_input_data(ready_data_descriptors,count_ready_data);
																	///------------------------->>
					if ( spent_descriptors != NULL ) {
								///------------------------->>
						{
							int *fds = &(spent_descriptors->closed_sockets[0]);
							int *end_fds = fds + spent_descriptors->n_closed;

							while ( fds < end_fds ) {
								
								int c_socket = *fds++;

								if ( epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c_socket, NULL) == -1 ) {  /// keep theses out of the consideration until ready for more data.

									perror("epoll_ctl: conn_sock EPOLLONESHOT");
									exit(EXIT_FAILURE);

								}

								close(c_socket);

							}
						}

								///------------------------->>
						{
							int *fds = &(spent_descriptors->ready_sockets[0]);
							int *end_fds = fds + spent_descriptors->n_ready;

							while ( fds < end_fds ) {
								
								int c_socket = *fds++;

								ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;   // edge triggered for these non blocking connections, as may be used by streaming.
								ev.data.fd = c_socket;

								if ( epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c_socket, &ev) == -1 ) {

									perror("epoll_ctl: conn_sock EPOLL_CTL_ADD");
									exit(EXIT_FAILURE);

								}

								close(c_socket);

							}
						}
					}
				}
			}
		}
	}
}



