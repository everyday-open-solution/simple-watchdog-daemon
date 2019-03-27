/* Simple watchdog daemon
 *
 * Copyright (C) 2019 Antoine BRAUT <antoine.braut@ac6.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <linux/watchdog.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>



static int fd = -1;								// Posix file descriptor
static unsigned char daemonRunning = 1;
static unsigned char watchdogDisarm = 0;

static int openWd(char*);
static int pingWd(void);
static int closeWd(void);

void sigHandler(int signum, siginfo_t* info, void* ptr);
void catchSigterm(void);


int main(int argc, char** argv) {

	pid_t pid, sid;								// Pid and sid for daemon fork
	FILE* pidFile = NULL;

	/* Configurable option */
	char* pidFileName = "/var/run/watchdogd.pid";
	char* watchdogDriver = "/dev/watchdog";
	int watchdogTimeOut = 10;

	int opt;											// Argument buffer

	catchSigterm();								// Enable SIGTERM handler

	/* Parse argument and configure watchdog */
	while ((opt = getopt (argc, argv, "dD:t:p:")) != -1)
	switch (opt)
		{
		case 'd':
			watchdogDisarm = 1;
			break;
		case 'D':
			watchdogDriver = optarg;
			break;
		case 't':
			watchdogTimeOut = atoi(optarg);
			break;
		case 'p':
			pidFileName = optarg;
			break;
		case '?':
			syslog(LOG_WARNING,"Bad Argument %c while be ignored",optopt);
		}

	/* Fork Parent Process */
	pid=fork();
	if(pid <0)
		goto EXIT_WITH_BAD_FORK;
	if(pid >0) /* Parent Exit */
		exit(EXIT_SUCCESS);

	/* Change file mode mask */
	umask(0);

	/* Get unique Session ID */
	sid = setsid();
	if(sid <0)
		goto EXIT_WITH_SID_FAILURE;

	/* change working directory to a safe place */
	if((chdir("/")) <0)
		goto EXIT_WITH_CD_FAILURE;

	/* Close standard I/O */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Opend Harware Watchdog Driver */
	if(openWd(watchdogDriver))
		return 1;

	/* Write PID in file */
	pidFile = fopen(pidFileName,"w");
	if(pidFile != NULL)
	{
		fprintf(pidFile,"%d",getpid());
		fclose(pidFile);
	}
	else
		syslog(LOG_WARNING,"Warning failed to write pid in %s",pidFileName);

	/* Daemon Loop */
	while (daemonRunning)
	{
		pingWd();
		sleep(watchdogTimeOut);
	}

	if(remove(pidFileName))
		syslog(LOG_WARNING,"Warning failed to remove pid file : %s",pidFileName);

	return 0;
	EXIT_WITH_BAD_FORK:
	syslog(LOG_ERR,"Error while forking the daemon");
	return 2;
	EXIT_WITH_SID_FAILURE:
	syslog(LOG_ERR,"Error while getting SID");
	return 3;
	EXIT_WITH_CD_FAILURE:
	syslog(LOG_ERR,"Error while changing working directory;");
	return 4;
}
/*
 * ==============================================
 * Name 		: openWd
 * Return		: 0 on succes, 1 on failure
 * Brief		: Open /dev/watchdog with posix file descriptor and log the driver description
 * ==============================================
 */
static int openWd(char* watchdogDriver)
{
	struct watchdog_info id;
	FILE* dmesgFile = NULL;

	fd = open(watchdogDriver,O_WRONLY|O_CLOEXEC);
	if(fd < 0)
		goto EXIT_WITH_OPEN_FAILURE;

	if(ioctl(fd, WDIOC_GETSUPPORT, &id) >=0)
	{
			//system("echo \"Watchdog daemon : started\" >> /dev/kmsg");
			dmesgFile = fopen("/dev/kmsg","a");
			if(dmesgFile != NULL)
			{
				fprintf(dmesgFile,"Watchdog daemon : started");
				fclose(dmesgFile);
			}
			syslog(LOG_INFO,"Watchdog daemon : started with '%s' driver, version %x", id.identity, id.firmware_version);
	}

	return 0;
	EXIT_WITH_OPEN_FAILURE:
	syslog(LOG_ERR,"Error while opening /dev/watchdog");
	return 1;
}
/*
 * ==============================================
 * Name 		: pingWd
 * Return		: 0 on succes, 1 on failure
 * Brief		: use syscall to kick the watchdog
 * ==============================================
 */
static int pingWd(void)
{

	if(ioctl(fd, WDIOC_KEEPALIVE, 0) < 0)
		goto EXIT_WITH_PING_FAILURE;

	return 0;
	EXIT_WITH_PING_FAILURE:
	syslog(LOG_ERR,"Error while pinging /dev/watchdog");
	return 1;
}
/*
 * ==============================================
 * Name 		: closeWd
 * Return		: 0
 * Brief		: close /dev/watchdog
 * ==============================================
 */
static int closeWd(void)
{
	int disarmFlag = WDIOS_DISABLECARD;
	const char magicVValue = 'V';

	int exitValue = 0;

	if(fd<0)
		goto EXIT;

	if(watchdogDisarm)
	{
		if(ioctl(fd, WDIOC_SETOPTIONS, &disarmFlag))
		{
			syslog(LOG_ERR, "Something went wrong while calling system to disarm watchdog");
			exitValue = 1;
		}
		/* In addition to the syscall we use the "magic value" to ensure watchdog disarmement */
		if(write(fd,&magicVValue,1) <1)
		{
			syslog(LOG_ERR, "Error while disarming the watchdog. It might still be active");
			exitValue = 1;
		}
		else
			exitValue = 0;
	}
	fd = close(fd);

	EXIT:
	return exitValue;
}
/*
 * ==============================================
 * Name 		: sigHandler
 * Return		: void
 * Brief		: Catch SIGTERM and stop daemon
 * ==============================================
 */
void sigHandler(int signum, siginfo_t* info, void* ptr)
{
	syslog(LOG_INFO,"Deamon stopped by SIGTERM");
	closeWd();
	daemonRunning=0;
}
/*
 * ==============================================
 * Name 		: catchSigterm
 * Return		: void
 * Brief		: Enable the catch of SIGTERM
 * ==============================================
 */
void catchSigterm(void)
{
	static struct sigaction _sigact;

	memset(&_sigact, 0, sizeof(_sigact));
	_sigact.sa_sigaction=sigHandler;
	_sigact.sa_flags=SA_SIGINFO;

	sigaction(SIGTERM, &_sigact, NULL);
}
