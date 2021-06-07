#!/bin/bash

sleep 2 #per dare tempo al Server di avviarsi

cmd1="-f./socket/mysock -W./TestFolder/test2/archivio1.zip -D./TestFolder/backupdir -p"
cmd2="-f./socket/mysock -W./TestFolder/test2/archivio2.zip -D./TestFolder/backupdir -p"
cmd3="-f./socket/mysock -W./TestFolder/test2/archivio3.zip -D./TestFolder/backupdir -p"
cmd4="-f./socket/mysock -W./TestFolder/test2/archivio4.zip -D./TestFolder/backupdir -p"
cmd5="-f./socket/mysock -W./TestFolder/test2/archivio5.zip -D./TestFolder/backupdir -p"
k=0

./out/client ${cmd1}
PID[k]=$!
((k++))

./out/client ${cmd2}
PID[k]=$!
((k++))

./out/client ${cmd3}
PID[k]=$!
((k++))

./out/client ${cmd4}
PID[k]=$!
((k++))

./out/client ${cmd5}
PID[k]=$!
((k++))

for((i=0;i<k;++i)); do
    wait ${PID[i]}
done