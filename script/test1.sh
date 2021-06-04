#!/bin/bash
sleep 2 #per dare tempo al server di avviarsi
sample1_path=$(realpath ./TestFolder/files/sample/sample1.txt)
doc_path=$(realpath ./TestFolder/files/sample/document.doc)
cmd="-f./socket/mysock -t200 -w./TestFolder/files/sample -W./TestFolder/files/document.doc -r${sample1_path}, ${doc_path} -R,2 -p"
./out/client ${cmd}
