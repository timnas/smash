#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

#define FAIL -1
using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
  time_t timeout = 0;
  bool is_empty = false;
  while(timeout <= 0 && !is_empty) {
    JobsList::JobEntry* job = SmallShell::getInstance().getTimedOutJob();
    cout << "smash: got an alarm" << endl;
    if (job != nullptr) {
        cout << "smash: " << job->command << " timed out!" << endl;
        if (kill(job->jobPid, SIGKILL) == FAIL) {
          perror("smash error: kill failed");
        }
        SmallShell::getInstance().alarms_list.removeJobById(job->jobId);
    }
    else {
      return;
    }
    timeout = SmallShell::getInstance().getMostRecentAlarmTime();
    is_empty = SmallShell::getInstance().alarms_list.jobs_list.size() > 0 ? false : true;
    if(timeout > 0){
      alarm(timeout);
    } 
  }
}

