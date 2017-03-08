#!/bin/bash
GDRIVE=./gdrive
COLLECTION=$1
QUERY="name contains '$COLLECTION'"
## echo $QUERY
parts=($($GDRIVE list --no-header --max 1000 --query "$QUERY" | sort -k 2 | awk '{print $1}'))
let len=${#parts[*]}-1
segment=0
while [ $segment -le $len ]; do 
	id=${parts[$segment]}
	(>&2 echo "Retrieving ID $id [$segment/$len]")
	let segment=segment+1 
	$GDRIVE download --stdout $id
done
