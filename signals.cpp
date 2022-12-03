#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
	SmallShell &smash = SmallShell::getInstance();
    pid_t pid = smash.curr_fg_pid;
 //   jid_t jid = smash.fg_jid;
    cout << "smash: got ctrl-Z" << endl;
    if (pid == FAIL){ //there's no process running in fg
        return;
    }
    string command = smash.current_cmd;
    (smash.jobs_list).addJob(command,pid,0,true);
    if (kill(pid,SIGSTOP) == 0) { //success
        JobsList::JobEntry *job = (smash.jobs_list).getJobByPId(pid);
        if (job != nullptr){
            job->is_stopped = true;
        }
        else {
            (smash.jobs_list).addJob(command,pid,0,true);
        }
        cout << "smash: process " << pid << " was stopped" << endl;
        smash.curr_fg_pid = EMPTY;
        smash.fg_jid = EMPTY;
    }
    else {
        perror("smash error: kill failed");
    }
}

void ctrlCHandler(int sig_num) {
  SmallShell &smash = SmallShell::getInstance();
  pid_t pid = smash.curr_fg_pid;
  cout << "smash: got ctrl-C" << endl;
  if (pid == EMPTY){ //no process in fg
      return;
  }
  if (kill(pid,SIGKILL) != 0){
      perror("smash error: kill failed");
      return;
  }
  cout << "smash: process " << pid << " was killed" << endl;
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
    is_empty = !(SmallShell::getInstance().alarms_list.jobs_list.size() > 0);
    if(timeout > 0){
      alarm(timeout);
    } 
  }
}

