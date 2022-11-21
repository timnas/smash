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

JobsList::JobsList() : jobs_list(), jobs_num(0){}
JobsList::JobEntry::JobEntry(jid_t jobId, time_t creation_time, string &command, bool isStopped) : jobId (jobId),
creation_time(creation_time), command (command), isStopped(false){

}


Command::Command(const char *cmd_line) : cmd_line(cmd_line){}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {

}//timnatest

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
