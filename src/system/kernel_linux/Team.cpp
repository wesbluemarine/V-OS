/*
 *  Copyright 2019, Dario Casalinuovo. All rights reserved.
 *  Distributed under the terms of the LGPL License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "syscalls.h"

#define UID_LINE 8
#define GID_LINE 9

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
    char buffer[1024];
    char command_proc_path[500];
    sprintf(proc_path, "/proc/%d/status", id);
    sprintf(command_proc_path, "/proc/%d/cmdline", id);
  	FILE *fileid = fopen(proc_path, "r");
    FILE *file_command_id = fopen(command_proc_path, "r");
    if(fileid == NULL) {
		return B_ERROR;
	}
    int i=0;
    while(!feof(fileid)){
        fgets(buffer, 1024, fileid);
        if( i == UID_LINE) {
            sscanf(buffer, "Uid:\t%u\t", &info->uid);
        } else if( i == GID_LINE) {
            sscanf(buffer, "Gid:\t%u\t", &info->gid);
        } else {
            if(strncmp(buffer, "Threads:", sizeof("Threads")) == 0){
                sscanf(buffer, "Threads:\t%d", &info->thread_count);
                break;
            }
        }
        i++;
        //fread(buffer, sizeof(char), 1025, fileid) > 0){
    }
    printf("---\n");
    fgets(buffer, 1024, file_command_id);
    char *token = strtok(buffer, " ");
    int argCount = 0;
    while(strtok(NULL, " ") != NULL){
        argCount++;
    }
    info->argc = argCount;
    strncpy(info->args, buffer, 64);
    fclose(fileid);
    fclose(file_command_id);
    return B_OK;
}


status_t
_kern_get_next_team_info(int32 *cookie, team_info *info)
{
	UNIMPLEMENTED();
	return B_ERROR;
}
