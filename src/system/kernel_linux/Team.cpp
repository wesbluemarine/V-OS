/*
 *  Copyright 2019, Dario Casalinuovo. All rights reserved.
 *  Distributed under the terms of the LGPL License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "syscalls.h"

#define UID_LINE 8
#define GID_LINE 9
#define MAX_BYTES 64
#define MAX_LINE_LENGTH 1024
status_t
_kern_get_team_usage_info(team_id team, int32 who, team_usage_info *info, size_t size)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
_kern_kill_team(team_id team)
{
	int err = kill((pid_t) team, SIGKILL);
	if (err < 0)
		return B_BAD_TEAM_ID;

	return B_OK;
}


status_t
_kern_get_team_info(team_id id, team_info *info)
{
  	char proc_path[500];
    char buffer[MAX_LINE_LENGTH];
    char command_proc_path[500];
    sprintf(proc_path, "/proc/%d/status", id);
    sprintf(command_proc_path, "/proc/%d/cmdline", id);
  	FILE *fileid = fopen(proc_path, "r");
    int file_command_id = open(command_proc_path, O_RDONLY);
    if(fileid == NULL) {
		return B_ERROR;
	}
    int i=0;
    while(!feof(fileid)){
        fgets(buffer, MAX_LINE_LENGTH, fileid);
        if( i == UID_LINE) {
            sscanf(buffer, "Uid:\t%u\t", &info->uid);
        } else if( i == GID_LINE) {
            sscanf(buffer, "Gid:\t%u\t", &info->gid);
        } else {
            if(strncmp(buffer, "Threads:", sizeof("Threads:")) == 0){
                sscanf(buffer, "Threads:\t%d", &info->thread_count);
                break;
            }
        }
        i++;
    }
    int bytes_read = read(file_command_id, buffer, MAX_LINE_LENGTH);
    int argCount = 0;
    int j = 0;
    while(j < bytes_read){
        if(buffer[j] == '\0'){
            argCount++;
            buffer[j] = ' ';
        }
        j++;
    }
    buffer[j] = '\0';
    info->argc = argCount;
    strncpy(info->args, buffer, MAX_BYTES);
    fclose(fileid);
    close(file_command_id);
    return B_OK;
}


status_t
_kern_get_next_team_info(int32 *cookie, team_info *info)
{
	UNIMPLEMENTED();
	return B_ERROR;
}
