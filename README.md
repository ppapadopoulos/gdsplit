# gdsplit
Using gdrive split stdin to named files and uploads. Intended for uploading ZFS snapshots into google drive.

This uses gdrive from Petter Rasmussen's [gdrive](https://github.com/prasmussen/gdrive/blob/master/README.md)

* EXAMPLE USING ZFS SEND A SNAPSHOT TO GOOGLE DRIVE

```
zfs send tank/cseteaching@08MAR2017 | ./gdsplit -f cseteaching@08MAR2017 -b 512M -p 0B2VTJMbHpU8yT3ZXcDdMdzlkR2s
```
Example Output:
```
file chunk cseteaching@08MAR2017.0000 (536870912 bytes)
Executing command gdrive upload - --chunksize 33554432 --parent 0B2VTJMbHpU8yT3ZXcDdMdzlkR2s cseteaching@08MAR2017.0000  (536870912 bytes to write)
Uploading cseteaching@08MAR2017.0000
503.3 MB, Rate: 29.1 MB/swrote 536870912 bytes (0 remaining)
Uploaded 0B2VTJMbHpU8yYVFPTU1qZ3J4SXM at 27.4 MB/s, total 536.9 MB
file chunk cseteaching@08MAR2017.0001 (536870912 bytes)
Executing command gdrive upload - --chunksize 33554432 --parent 0B2VTJMbHpU8yT3ZXcDdMdzlkR2s cseteaching@08MAR2017.0001  (536870912 bytes to write)
Uploading cseteaching@08MAR2017.0001
503.3 MB, Rate: 31.0 MB/swrote 536870912 bytes (0 remaining)
Uploaded 0B2VTJMbHpU8ybzlsZzdRMzJNaWs at 29.0 MB/s, total 536.9 MB
file chunk cseteaching@08MAR2017.0002 (352900560 bytes)
Executing command gdrive upload - --chunksize 33554432 --parent 0B2VTJMbHpU8yT3ZXcDdMdzlkR2s cseteaching@08MAR2017.0002  (352900560 bytes to write)
Uploading cseteaching@08MAR2017.0002
335.5 MB, Rate: 30.1 MB/swrote 352900560 bytes (0 remaining)
Uploaded 0B2VTJMbHpU8ySnFEOEtsN2hKd0E at 27.0 MB/s, total 352.9 MB
```

* EXAMPLE RECEIVING SAVED SNAPSHOT
```
./gdcat.sh cseteaching@08MAR2017 | zfs recv pool1/cseteaching
```
Example output:
```
Retrieving ID 0B2VTJMbHpU8yYVFPTU1qZ3J4SXM [0/2]
Retrieving ID 0B2VTJMbHpU8ybzlsZzdRMzJNaWs [1/2]  
Retrieving ID 0B2VTJMbHpU8ySnFEOEtsN2hKd0E [2/2]  
[root@pragmadata-1-1 pool1]# zfs list             
NAME                    USED  AVAIL  REFER  MOUNTPOINT
pool1                   210G  49.7T   155G  /pool1
pool1/cseteaching      1.41G  49.7T  1.41G  /pool1/cseteaching
```
