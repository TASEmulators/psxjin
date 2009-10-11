#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "Config.h"
#include "cdriso.h"

void ExecCfg(char *arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgCdrIso");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		system(cfg); return;
	}

	strcpy(cfg, "./cfg/cfgCdrIso");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		system(cfg); return;
	}

	sprintf(cfg, "%s/cfgCdrIso", getenv("HOME"));
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		system(cfg); return;
	}

	printf("cfgCdrIso file not found!\n");
}

long CDRconfigure() {
	ExecCfg("configure");

	return 0;
}

void CDRabout() {
	ExecCfg("about");
}

void CfgOpenFile() {
	ExecCfg("open");
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[256];
	char cmd[256];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	sprintf(cmd, "message \"%s\"", tmp);
	ExecCfg(cmd);
}

