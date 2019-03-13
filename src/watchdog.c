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
unsigned char daemonRunning = 1;

static int openWd(void);
static int pingWd(void);
static int closeWd(void);

void sigHandler(int signum, siginfo_t* info, void* ptr);
void catchSigterm(void);


int main(void) {

	pid_t pid, sid;								// Pid and sid for daemon fork
	FILE* pidFile = NULL;

	catchSigterm();								// Enable SIGTERM handler

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
	if(openWd())
		return 1;

	/* Write PID in file */
	pidFile = fopen("/var/run/watchdogd.pid","w");
	if(pidFile != NULL)
	{
		fprintf(pidFile,"%d",getpid());
		fclose(pidFile);
	}

	/* Daemon Loop */
	while (daemonRunning)
	{
		pingWd();
		sleep(10);
	}

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
static int openWd(void)
{
	struct watchdog_info id;

	fd = open("/dev/watchdog",O_WRONLY|O_CLOEXEC);
	if(fd < 0)
		goto EXIT_WITH_OPEN_FAILURE;

	if(ioctl(fd, WDIOC_GETSUPPORT, &id) >=0)
	{
			system("echo \"Watchdog daemon : started\" >> /dev/kmsg");
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
	if(fd<0)
		goto EXIT;

	fd = close(fd);

	EXIT:
	return 0;
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
