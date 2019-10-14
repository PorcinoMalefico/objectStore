#!/bin/bash

awk '
BEGIN{
	reg_tot=0;
	reg_ok=0;
	sto_tot=0;
	sto_ok=0;
	ret_tot=0;
	ret_ok=0;
	del_tot;
	del_ok;
	n_users=0
	n_clients=0
}

$4~/socket:/ && $5~/^[0-9]+$/{
	++sockets[$5]
}

/REGISTER/{
	++reg_tot ;
	if ( $6 ~ /name:/ )
	{
		user=$7
		++userlist[user]
	}
}
/STORE/{
	++sto_tot ;
	sto_ok+=($NF==1) ;
}
/RETRIEVE/{
	++ret_tot ;
	ret_ok+=($NF==1) ;
}
/DELETE/{
	++del_tot ;
	del_ok+=($NF==1) ;
}

END{
	printf( "# clients connected:   %d\n" , length(sockets) )
	printf( "# register operations: %d\n"    , reg_tot );
	printf( "# store operations:    %d/%d\n" , sto_tot , sto_ok );
	printf( "# retrieve operations: %d/%d\n" , ret_tot , ret_ok );
	printf( "# delete operations:   %d/%d\n" , del_tot , del_ok );
	printf( "# registered users:    %s\n"  , length(userlist) );
}' server.log
