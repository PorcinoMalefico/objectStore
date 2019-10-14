#!/bin/bash

awk '
BEGIN{
        nstore=0;
        nret=0;
        ndel=0;
        numcstore=0;
        numcret=0;
        numcdel=0;
        storeok=0;
        retok=0;
        delok=0;
}
/store/{
        nstore += $4;
        storeok += $6
        ++numcstore;
}
/retrieve/{
        nret += $4;
        retok += $6
        ++numcret;
}
/delete/{
        ndel += $4;
        delok += $6
        ++numcdel;
}
END{
        printf("Test tipo 1: %d client hanno eseguito un totale di %d os_store delle quali %d con successo.\n", numcstore,nstore,storeok);
        printf("Test tipo 2: %d client hanno eseguito un totale di %d os_retrieve delle quali %d con successo.\n",numcret,nret,retok);
        printf("Test tipo 3: %d client hanno eseguito un totale di %d os_delete delle quali %d con successo.\n",numcdel,ndel,delok);
        printf("Operazioni totali: %d \n",nstore+nret+ndel);
        printf("Operazioni a buon fine: %d \n",storeok+retok+delok);
        printf("Operazioni fallite: %d \n",nstore+nret+ndel-storeok-retok-delok);
}' testout.log

killall -USR1 server
sleep 1
