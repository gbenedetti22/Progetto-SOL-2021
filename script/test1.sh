#!/bin/bash
sleep 2 #per dare tempo al server di avviarsi

sample_path=$(realpath ./TestFolder/files)    #scrivo tutta la cartella files (-w)
pdf_path=$(realpath ./progettosol-20_21.pdf)  #scrivo il teso del progetto (-W)

backup_dir=$(realpath ./TestFolder/backupdir) #In caso di capacity misses (-D)
download_dir=$(realpath ./TestFolder/downloadDir)

read_file="${sample_path}/sample/sample1.txt"        #leggo sample1.txt e lo salvo in download_dir

args="-f./socket/mysock -t200 -w${sample_path} -W${pdf_path} -r${read_file}, ${pdf_path} -D${backup_dir} -d${download_dir} -R,2 -p"
./out/client ${args}