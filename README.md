# gdsplit
Using gdrive split stdin to named files and uploads. Intended for uploading ZFS snapshots into google drive.

This uses gdrive from Petter Rasmussen's [gdrive](https://github.com/prasmussen/gdrive/blob/master/README.md)

```
gdsplit [-b blksize] [-c chunksize] [-f filebase] [-g gdrive] [-p parent] [-s start] [-5]
	Split a stream into blksize files and store in google drive
	files names <filebase>.0000, <filebase>.0001, ....
Flags:
	-b blksize   Size of each filesplit [e.g., 8K,1M,1G] (default: 128M)
	-c chunksize Parameter to gdrive for upload performance tuning [e.g., 8M] (default:32M)
	-f filebase  Basename of file for storing in gdrive (default: Ztemp)
	-g gdrive    Path to gdrive executable (default:gdrive)
	-p parent    Parent id (folder to store files).
	-s start     Start uploading at starting chunk (default:0)
	-5           Compute the MD5 sum of each chunk.
	             Can be used with -s to locally recompute checksums without uploading
```

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

* Validation
Each uploaded chunk can be optionally checksummed (md5) with its checksum
printed to stderr during upload.  The format of the checksum is identical to
gdrive info. Used in conjunction with the -s flag, checksums can be locally recomputed and verified against info from gdrive

Example of generating the checksums locally without uploading (-s skips past the first 1000 chunks, chunksize is 512M, -5 computes the md5sum)

```
zfs send tank/cseteaching@08MAR2017 | ./gdsplit -f cseteaching@08MAR2017 -s 1000 -b 512M -5 2>&1 | grep -e Name: -e Md5sum
```

Example output from above
```
Name: cseteaching@08MAR2017.0000
Md5sum: 225846bb06b14290635ed4558fbe5a6a
Name: cseteaching@08MAR2017.0001
Md5sum: b3cb2010602429bdebc6a5c227bee722
Name: cseteaching@08MAR2017.0002
Md5sum: 8ebedeb00e2f0c3f2dfaa46e036de05c
```

Now, we want to validate the copies in gdrive
```
ids=($(gdrive list --query "name contains 'cseteaching'" | sort -k 2 | grep cseteaching@08MAR2017 | awk '{print $1}'))
for gid in ${ids[@]}; do gdrive info $gid | grep -e Name: -e Md5sum:; done
```

And this has example output identical to the local copy
```
Name: cseteaching@08MAR2017.0000
Md5sum: 225846bb06b14290635ed4558fbe5a6a
Name: cseteaching@08MAR2017.0001
Md5sum: b3cb2010602429bdebc6a5c227bee722
Name: cseteaching@08MAR2017.0002
Md5sum: 8ebedeb00e2f0c3f2dfaa46e036de05c
```


