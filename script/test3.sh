#! /bin/bash
end=$((SECONDS+30))

cmd="-f./socket/mysock \
-W./TestFolder/test2/archivio1.zip \
-W./TestFolder/test2/archivio2.zip \
-W./TestFolder/test2/archivio3.zip \
-W./TestFolder/test2/archivio4.zip \
-W./TestFolder/test2/archivio5.zip \
-c./TestFolder/test2/archivio1.zip \
-c./TestFolder/test2/archivio2.zip \
-c./TestFolder/test2/archivio3.zip \
-c./TestFolder/test2/archivio4.zip \
-c./TestFolder/test2/archivio5.zip \
-D./TestFolder/backupdir"

PID=()
k=0
while [ $SECONDS -lt $end ]; do
    ./out/client ${cmd}
    PID[k]=$!
    ((k++))
done

for((i=0;i<k;++i)); do
    wait ${PID[i]}
done