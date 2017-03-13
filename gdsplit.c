/* gdsplit -  split stdin into fixed sized chunks. name them and upload into
 * google drive using Petter Rasmussen's excellent gdrive client available at
 *              https://github.com/prasmussen/gdrive
 * 
 * Author: Philip Papadopoulos
 *         ppapadopoulos@ucsd.edu
 *
 *
 * Written to solve the problem of storing a ZFS snapshot into google drive for backup
 * example use: zfs send filesystem@snapshot | gdsplit -f filesystem@snapshot
 * Will store into gdrive  filesystem@snapshot.0000, filesystem@snapshot.0001 ....
 * use the -p (google's id of a folder) to store these into a particular folder
 *
 * gdcat.sh can be used to retrive the snaps
 * gdcat filesystem@snapshot | zfs recv ....
 *
 *
*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <openssl/md5.h>
#define TRUE 1
#define FALSE 0
#define STRMAX 256
#define KILO 1024
#define MEGA KILO*KILO
#define GIGA KILO*MEGA
#define NETBUF 32*MEGA
#define DEFBLKSIZE 128*MEGA
#define MAXCHUNKS 10000
#define PATTERN "%s.%04d"
#define RETRIES 3

/* Reporting flags */
#define SILENT 
#define INFO  1
#define PERF (INFO<<1)  
typedef struct gd {
	char * progexe;
	int chunksize;
	unsigned rptflag;
} GDRIVE;
#define DEFAULTPROG "gdrive"

int failedpipe = FALSE;
void sighandler(int sig)
{
	if (sig == SIGPIPE)
		failedpipe = TRUE;
	else
	 	exit(sig);
}	

int writegdrive(GDRIVE drive, char *buffer, int bytes, char *parent, char *dest)
{
	FILE *output;
	int error;
	int mybytes = bytes;
	char *mybuffer = buffer;
	int retries = RETRIES;
	char command[STRMAX + STRMAX ];
	char options[STRMAX + STRMAX ];
	options[0] = '\0';
	if (parent != NULL)
		sprintf(options,"--parent %s",parent);
	sprintf(command,"%s upload - --chunksize %d %s %s", drive.progexe,drive.chunksize, options, dest);

	/* register a signal handler if we get a PIPE error */
	failedpipe=FALSE;
	signal(SIGPIPE,sighandler);

	while (retries > 0)
	{
		mybytes = bytes;
		mybuffer = buffer;
		if (drive.rptflag & INFO)
			fprintf(stderr,"Executing command %s  (%d bytes to write)\n",command,mybytes);
		output = popen(command,"w");
		int byteswritten;
		while( !failedpipe && mybytes > 0 && 
			(byteswritten=fwrite(mybuffer,sizeof(char),mybytes,output)) > 0)
		{
			mybytes -= byteswritten;
			if (drive.rptflag & INFO)
				fprintf(stderr,"wrote %d bytes (%d remaining)\n",byteswritten,mybytes);
			mybuffer += byteswritten;
		}
		/* check if we completed without a PIPE error */
		if (pclose(output) == 0 && !failedpipe) break;
		failedpipe = FALSE;
		retries--;
		fprintf(stderr,"Warning: Pipe Error (%d retries remaining)\n",retries); 
	}
	signal(SIGPIPE,SIG_DFL);  /* reset to default hander */
	if (retries == 0) 
	{
		exit(ESTRPIPE);  /* exit the program, exhausted retries */ 
	}
	return bytes;
}


/** md5sum of buffer - use the external program md5sum to compute checksum of
    buffer
*/
void  md5sum(char *fname, void *buffer, unsigned long bytes)
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	char result[2*MD5_DIGEST_LENGTH+1];
	result[2*MD5_DIGEST_LENGTH] = '\0';
	MD5_CTX mdContext;
	MD5_Init(&mdContext);
	MD5_Update(&mdContext,buffer,bytes);
	MD5_Final(digest,&mdContext);
	int i;
	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
	{
		sprintf(result+(2*i),"%02x", digest[i]);
	}
	/* Print in same format gdrive info */
	fprintf(stderr,"Name: %s\n",fname);
	fprintf(stderr,"Md5sum: %s\n",result);
}


void usage() 
{
	fprintf(stderr,"gdsplit [-b blksize] [-c chunksize] [-f filebase] [-g gdrive] [-p parent] [-s start] [-5]\n");
	fprintf(stderr,"\tSplit a stream into blksize files and store in google drive\n");
	fprintf(stderr,"\tfiles names <filebase>.0000, <filebase>.0001, ....\n");
	fprintf(stderr,"Flags:\n");
	fprintf(stderr,"\t-b blksize   Size of each filesplit [e.g., 8K,1M,1G] (default: 128M)\n");
	fprintf(stderr,"\t-c chunksize Parameter to gdrive for upload performance tuning [e.g., 8M] (default:32M)\n");
	fprintf(stderr,"\t-f filebase  Basename of file for storing in gdrive (default: Ztemp)\n");
	fprintf(stderr,"\t-g gdrive    Path to gdrive executable (default:gdrive)\n");
	fprintf(stderr,"\t-p parent    Parent id (folder to store files).\n"); 
	fprintf(stderr,"\t-s start     Start uploading at starting chunk (default:0)\n");
	fprintf(stderr,"\t-5           Compute the MD5 sum of each chunk.\n");
	fprintf(stderr,"\t             Can be used with -s to locally recompute checksums without uploading\n"); 
}
  
int str2int(char *str, int dflt)
{
	/* Take in str that is <nnnn>[kKmMgG] and return an int*/
	if (str == NULL) return dflt;
	int size, mult=1;
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		if (!isdigit(str[i]))
		{
			switch (str[i])
			{
				case 'k':
				case 'K': mult=KILO;
					  break;
				case 'm':
				case 'M': mult=MEGA;
					  break;
				case 'g':
				case 'G': mult=GIGA;
					  break;
			}
			str[i] = '\0';
			break;
		}
	}
	if (strlen(str) <= 0) return dflt;
	size = ((int) strtol(str, (char **)NULL, 10))*mult;	
	return size;
}
int main (int argc, char **argv)
{
	char *blkstr,*chunkstr,*filebase;
	char *parent = NULL;
	chunkstr = NULL;

	blkstr = NULL;
	filebase= "Ztemp";
	int c;
	opterr = 0;
	int startchunk = 0;
	GDRIVE drive;
	drive.progexe = DEFAULTPROG;
	drive.rptflag = INFO;
	int md5 = 0;

	while ((c = getopt (argc, argv, "b:c:f:g:p:s:5")) != -1)
	{
		switch (c)
		{
			case 'b':
				blkstr = optarg;
				break;
			case 'c':
				chunkstr = optarg;
				break;
			case 'f':
			        filebase = optarg;	
				break;
			case 'g':
			        drive.progexe = optarg;	
				break;
			case 'p':
				parent = optarg;
				break;
			case 's':
				startchunk = atoi(optarg);
				break;
			case '5': md5 = 1;
				break;
			case '?':
			 	usage();
				return(0);	
			default:
				abort ();
		}
	}

	
	int blksize = str2int(blkstr,DEFBLKSIZE);
	drive.chunksize = str2int(chunkstr,NETBUF);
	
	char fname[STRMAX+STRMAX];
	void *buffer = malloc(blksize*sizeof(char));
	if (buffer == NULL) {
		fprintf(stderr,"Could not allocate %d memory buffer. Exiting\n",blksize);
		abort();
	}

	void *curloc = buffer;
	int byteswant = blksize;
	int bytesread;
	int buflen = 0;
	int segment=0;
	/* Read data from stdin, attempt to fill our in-memory buffer */
	while ((bytesread = read(STDIN_FILENO,curloc,byteswant)) > 0)  
	{
		byteswant -= bytesread;
		curloc += bytesread;
		buflen += bytesread;
		// have we filled our buffer
		if (byteswant == 0)
		{
			sprintf(fname,PATTERN,filebase,segment);
			if (drive.rptflag & INFO) fprintf(stderr,"file chunk %s (%d bytes)\n",fname,blksize);
			if (segment >= startchunk)
			{
				writegdrive(drive,buffer,blksize,parent,fname);
			}
			else
			{
				if (drive.rptflag & INFO) fprintf(stderr,"skipping upload of %s\n",fname);
			}
			if (md5) {
				md5sum(fname, buffer,(unsigned long) blksize);
			}
			segment++;
			byteswant =  blksize;
			curloc = buffer; /* re-use the buffer memory */
			buflen = 0;	
		}
	}
	/* pick up any straggling bits */
	if (buflen > 0)
	{
		sprintf(fname,PATTERN,filebase,segment);
		if (segment >= startchunk)
		{
			if (drive.rptflag & INFO) fprintf(stderr,"file chunk %s (%d bytes)\n",fname,buflen);
			writegdrive(drive, buffer,buflen,parent,fname);
		}
		else if (drive.rptflag & INFO) 
			fprintf(stderr,"skipping upload of %s\n",fname);
		if (md5)
			md5sum(fname,buffer,(unsigned long) buflen);

	}
	free(buffer);

}
// vim: ts=4:sw=4
