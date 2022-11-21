#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

int _parseCommandLine(const char* cmd_line, char** args);

char** makeArgs(const char* cmd_line, int *num_of_args){
    char** args = (char**)malloc(COMMAND_MAX_ARGS * sizeof(char**));
    if (!args){
        return nullptr;
    }
    for (int i=0; i<COMMAND_MAX_ARGS; i++){
        args[i] = nullptr;
    }
    int num = _parseCommandLine(cmd_line, args);
    *num_of_args = num;
    return args;
}

void freeArgs (char **args, int size){
    if (!args){
        return;
    }
    for (int i=0; i<size; i++){
        if (args[i]){
            free(args[i]);
        }
    }
    free(args);
}

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

JobsList SmallShell::jobs_list;
pid_t SmallShell::pid = getpid();
string SmallShell::prompt = "smash";
SmallShell::SmallShell() : current_process(-1),
                            current_job (-1),
                            previous_dir("")
                        {}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
      return new ChpromptCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

JobsList::JobsList() : jobs_list(), jobs_num(1){}

void JobsList::removeFinishedJobs() {
    SmallShell &smash = SmallShell::getInstance();
    vector<JobEntry>::iterator it;
    int newMax=0;
    //first clean the list - delete all finished jobs
    for (it=jobs_list.begin(); it!=jobs_list.end(); it++){
        JobEntry currentJob = *it;
        int status;
        int returned_val = waitpid(currentJob.getJobId(), &status, WNOHANG); //WNOHANG: return immediately if no child has exited
        //waitpid returns the process ID of the child whose state has changed, and returns -1 when there's an error
        //if returned_val = currentJID or -1, it means the job either finished or there's an error. so delete job
        if (returned_val == -1 ||returned_val == currentJob.getJobId()){
            jobs_list.erase(it);
            it--;
        }
    }
    //if the list is empty, the new jobs_num is 1. return
    if (jobs_list.empty() == true){
        this->setJobsNum((jid_t)1);
        return;
    }
    //else = find new jobs_num
    for (it=jobs_list.begin(); it!=jobs_list.end(); it++){
        JobEntry currentJob = *it;
        if (currentJob.getJobId() > newMax){
            newMax = currentJob.getJobId();
        }
    }
    newMax++;
    this->jobs_num = newMax;
}

void JobsList::setJobsNum(jid_t newNum) {
    this->jobs_num = newNum;
}

JobsList::JobEntry::JobEntry(jid_t jobId, pid_t jobPid, time_t creation_time, string &command, bool isStopped) :
jobId (jobId), jobPid(jobPid), creation_time(creation_time), command (command), isStopped(false){

}

jid_t JobsList::JobEntry::getJobId() const {
    return jobId;
}

bool JobsList::JobEntry::isJobStopped() const {
    return isStopped;
}

string JobsList::JobEntry::getCommand() const {
    return command;
}

pid_t JobsList::JobEntry::getJobPid() const {
    return jobPid;
}

time_t JobsList::JobEntry::getCreationTime() const {
    return creation_time;
}


Command::Command(const char *cmd_line) : cmd_line(cmd_line){}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
    if (num_of_args == 1){
        smash.setPrompt("smash");
    }
    else {
        smash.setPrompt(args[1]);
    }
    freeArgs(args,num_of_args);
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute() {
    long max_path_length = pathconf(".", _PC_PATH_MAX); //pathconf returns the max length of "." path (currDir)
    char* buffer = (char*)malloc(max_path_length+1);
    if (buffer){
        //getcwd gets the full path name of the current working directory, up to max_path_length bytes long and stores it in buffer.
        getcwd(buffer, (size_t)max_path_length);
        //If the full path name length (including the null terminator) is longer than max_path_length bytes, an error occurs.
        assert(buffer!=nullptr);
        cout<<buffer<<endl;
        free(buffer);
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){}

void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    (smash.getJobsList()).removeFinishedJobs();
    vector<JobsList::JobEntry>::iterator it;
    for (it = ((smash.getJobsList()).jobs_list).begin(); it < ((smash.getJobsList()).jobs_list).end(); it++){
        JobsList::JobEntry current_job = *it;
        if (current_job.isJobStopped() == true){
            //difftime() function returns the number of seconds elapsed between time time1 and time time0, represented as a double
            //time(nullptr) returns the current calendar time as an object of type time_t
            cout << "[" << current_job.getJobId() << "]" << current_job.getCommand() << " : " << current_job.getJobPid()
            << difftime(time(nullptr), current_job.getCreationTime()) << "secs(stopped)" << endl;
        }
        else {
            cout << "[" << current_job.getJobId() << "]" << current_job.getCommand() << " : " << current_job.getJobPid()
                 << difftime(time(nullptr), current_job.getCreationTime()) << "secs" << endl;
        }
    }
}


