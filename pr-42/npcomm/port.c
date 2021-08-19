#include <stdio.h>

#include <mach.h>
#include <mach_error.h>
#include <sys/message.h>

#include <sys/types.h>
#include <sys/dir.h>


port_t
CreatePort( )
{
  port_t result;

  if( port_allocate( task_self(), &result) != KERN_SUCCESS )
    return( 0 );

  return( result );
}


port_t
FindPort( name )
char *name;
{
  port_t result;

  if( netname_look_up( name_server_port, "",name, &result ) != KERN_SUCCESS )
    return( 0 );

  return( result );
}


DeletePort( port )
port_t port;
{
  port_deallocate( task_self(), port );
}


