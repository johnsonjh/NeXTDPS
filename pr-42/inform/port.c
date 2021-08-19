#include <stdio.h>

#include <mach.h>
#include <mach_error.h>
#include <sys/message.h>


#include <sys/types.h>
#include <sys/dir.h>

port_t
CreatePort( char *name )
{
  port_t result;

  if( port_allocate( task_self(), &result) != KERN_SUCCESS )
    return( 0 );

  if( name )
    if(netname_check_in(name_server_port,name,PORT_NULL,result)!= KERN_SUCCESS)
      return( 0 );

  return( result );
}


port_t
FindPort( char *host, char *name )
{
  port_t result;

  if( !host )
    host = "";
  if( netname_look_up( name_server_port, host,name, &result ) != KERN_SUCCESS )
    return( 0 );

  return( result );
}


void
FreePort( port_t port )
{
  port_deallocate( task_self(), port );
}
