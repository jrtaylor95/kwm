#ifndef AXLIB_SHAREDWORKSPACE_H
#define AXLIB_SHAREDWORKSPACE_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>

#include "application.h"

void SharedWorkspaceInitialize();
std::map<pid_t, std::string> SharedWorkspaceRunningApplications();

void SharedWorkspaceActivateApplication(pid_t PID);
bool SharedWorkspaceIsApplicationActive(pid_t PID);
bool SharedWorkspaceIsApplicationHidden(pid_t PID);

#endif
