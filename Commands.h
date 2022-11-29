#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <ctime>
#include <unistd.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

typedef int jid_t;
using namespace std;

class Command {
  protected:
    const char* cmd_line;
  public:
    explicit Command(const char* cmd_line);
    virtual ~Command() = default;
    virtual void execute() = 0;
    const char* getCmdLine() {
      return cmd_line;
    }
    void setCmd(const char* cmd) {
        cmd_line = cmd;
    }
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {
      delete cmd_line;
  }
};

class TimeoutCommand : public BuiltInCommand {
    int time_out;
    string cmd;
public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() = default;
    void execute() override;
    void addAlarm(pid_t pid) const;
};

class ExternalCommand : public Command {
  public:
    bool is_alarm;
    ExternalCommand(const char* cmd_line, bool is_alarm);
    virtual ~ExternalCommand() = default;
    void execute() override;
    bool isCmdComplex(string cmd);
    void timeoutExecute(TimeoutCommand* cmd);
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 string command;
 string file_name;
 bool is_append;
  public:
    explicit RedirectionCommand(const char* cmd_line, bool append);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
  public:
    //char** p_previous_dir;
//    ChangeDirCommand(const char* cmd_line, char** p_previous_dir);
    ChangeDirCommand(const char* cmd_line);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
  public:
    explicit GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  explicit ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
public:
  explicit QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() = default;
  void execute() override;
};

class JobsList {
 public:
  class JobEntry {
   public:
   jid_t jobId;
   pid_t jobPid;
   time_t creation_time;
   string command;
   bool is_stopped;
   int duration;
   JobEntry(jid_t jobId, pid_t jobPid, time_t creation_time, string& command, bool is_stopped, int duration);
   time_t getElapsedTime() const;
  };
 public:
    vector<JobEntry> jobs_list;
    jid_t jobs_num;

    JobsList();
    ~JobsList() = default;
    void addJob(string cmd, pid_t pid, int duration = 0, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry* getLastJob(int* lastJobId);
    JobEntry* getLastStoppedJob(int *jobId);
};

class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    explicit ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
  explicit ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() = default;
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
    public:
        JobsList jobs_list;
        JobsList alarms_list;
        pid_t pid = getpid();
        string prompt = "smash";
        string previous_dir;

        pid_t current_process;
        jid_t current_job_id;
        jid_t fg_jid;
        pid_t curr_fg_pid;
        string current_cmd;
        string current_alarm_cmd;
        bool is_cmd_fg;
        bool is_fg_alarm;
        time_t current_duration;

        SmallShell();
        string getPrompt() {
            return prompt;
        }
        Command *CreateCommand(const char* cmd_line);
        SmallShell(SmallShell const&) = delete; // disable copy ctor
        void operator=(SmallShell const&)  = delete; // disable = operator
        static SmallShell& getInstance() // make SmallShell singleton
        {
            static SmallShell instance; // Guaranteed to be destroyed.
            // Instantiated on first use.
            return instance;
        }
        ~SmallShell();
        void executeCommand(const char* cmd_line);
        JobsList::JobEntry* getTimedOutJob();
        time_t getMostRecentAlarmTime();
        void addTimeoutToAlarm(const char* cmd, pid_t pid, int duration);

  // void setPrevDir (char* current_dir){
  //     previous_dir = current_dir;
  // }
//  static void setPrompt(string new_prompt) {
//      prompt = new_prompt;
//  }
//  static void setCmdIsFg(bool state) {
//      is_cmd_fg = state;
//  }
//  void setCurrProcess(pid_t pid) {
//    current_process = pid;
//  }
//  void setCurrCmd(string cmd) {
//    current_cmd = cmd;
//  }
//  void setFgJid(jid_t job_id) {
//    fg_jid = job_id;
//  }
//  void setIsFgAlarm(bool state) {
//    is_fg_alarm = state;
//  }
//    void setCurrDuration(time_t duration) {
//    current_duration = duration;
//  }
//    void setCurrAlarmCmd(string cmd) {
//    current_alarm_cmd = cmd;
//  }
//  pid_t getCurrProcess() {
//    return current_process;
//  }
//  string getCurrAlarmCmd() {
//    return current_alarm_cmd;
//  }
//  static string getPrompt () {
//      return prompt;
//  }
//  static bool getIsCmdFg () {
//      return is_cmd_fg;
//  }
//  static bool getIsFgAlarm () {
//      return is_fg_alarm;
//  }
//  static pid_t getPid () {
//      return pid;
//  }
//  jid_t getId () {
//      return current_job;
//  }
//  JobsList getJobsList() const {
//      return jobs_list;
//  }
//  JobsList getAlarmsList() const {
//      return alarms_list;
//  }
//  time_t getCurrDuration() {
//    return current_duration;
//  }
//  string getPreviousWD () {
//      return previous_dir;
//  }
//  void setPreviousWD (string newWD){
//      previous_dir = newWD;
//  }
};

#endif //SMASH_COMMAND_H_
