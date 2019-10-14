#!/bin/bash
sleep 1

(
for (( i=0; i<50; i++ )); do
	./os_client/client user$i 1 &>> testout.log &
done

wait

for (( i=0; i<30; i++ )); do
	./os_client/client user$i 2 &>> testout.log &
done


for (( i=30; i<50; i++ )); do
	./os_client/client user$i 3 &>> testout.log &
done

wait
)