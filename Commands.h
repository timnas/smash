#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <ctime>

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
  const char* getCmdLine(){
      return cmd_line;
  }
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand(){
      delete cmd_line;
  }
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
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
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  char** p_previous_dir;
  ChangeDirCommand(const char* cmd_line, char** p_previous_dir);
  virtual ~ChangeDirCommand() = default;
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  explicit GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
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
  explicit QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() = default;
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   jid_t jobId;
   pid_t jobPid;
   time_t creation_time;
   string command;
   bool isStopped;

   JobEntry(jid_t jobId, pid_t jobPid, time_t creation_time, string& command, bool stopped);

  public:
      jid_t getJobId () const;
      bool isJobStopped() const;
      string getCommand () const;
      pid_t getJobPid () const;
      time_t getCreationTime () const;
      void setJobStatus (bool stopped_status);
  };
 // TODO: Add your data members
 public:
    vector<JobEntry> jobs_list;
    jid_t jobs_num;

  JobsList();
  ~JobsList() = default;
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
  void setJobsNum (jid_t newNum);
};

class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    explicit ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
  explicit ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() = default;
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  static JobsList jobs_list;
  static pid_t pid;
  static string prompt;
  pid_t current_process;
  jid_t current_job;
  jid_t fg_jid;
  string current_cmd;
  static bool is_cmd_fg;
  SmallShell();
 public:
  char* previous_dir;
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
  // void setPrevDir (char* current_dir){
  //     previous_dir = current_dir;
  // }
  static void setPrompt(string new_prompt) {
      prompt = new_prompt;
  }
  static void setCmdIsFg(bool state) {
      is_cmd_fg = state;
  }
  void setCurrProcess(pid_t pid) {
    current_process = pid;
  }
  void setCurrCmd(string cmd) {
    current_cmd = cmd;
  }
  void setFgJid(jid_t job_id) {
    fg_jid = job_id;
  }
  static string getPrompt (){
      return prompt;
  }
  static pid_t getPid () {
      return pid;
  }
  JobsList getJobsList() const{
      return jobs_list;
  }

  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
