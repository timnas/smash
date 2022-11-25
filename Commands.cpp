#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

#include <csignal>
using namespace std;
#define FAIL -1

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

//make sure the last char "/n" is also checked
bool isNumber (string str){
    for (int i=0; i< str.length(); i++){
        if (isdigit(str[i]) == false){
            return false;
        }
    }
    return true;
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
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, &previous_dir);
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

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int max_stopped_jid = -1;
    vector<JobsList::JobEntry>::iterator it;
    for (it = jobs_list.begin(); it < jobs_list.end(); it++){
        JobsList::JobEntry current_job = *it;
        if (current_job.isJobStopped() == true){
            max_stopped_jid = current_job.getJobId();
        }
    }
    *jobId = max_stopped_jid;
    return getJobById(max_stopped_jid);

}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    vector<JobsList::JobEntry>::iterator it;
    for (it = jobs_list.begin(); it < jobs_list.end(); it++){
        JobsList::JobEntry current_job = *it;
        if (current_job.getJobId() == jobId){
            return &current_job;
        }
    }
    return nullptr;
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

void JobsList::JobEntry::setJobStatus(bool stopped_status) {
    isStopped = stopped_status;
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

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getPid() << endl;
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


BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){}

void BackgroundCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
    //if no arguments - the last stopped job (in the jobs list, the one with mac JID) should be selected to continue running it in bg
    if (num_of_args == 1){
        jid_t last_job_jid;
        JobsList::JobEntry *last_job = (smash.getJobsList()).getLastStoppedJob(&last_job_jid);
        if (last_job_jid == -1){
            //can also use cout but it's good practice to use cerr
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        }
        else {
            //kill returns 0 for success and -1 for failure
            if (kill(last_job_jid, SIGCONT) == -1){
                perror("smash error: kill failed");
                freeArgs(args,num_of_args);
                return;
            }
            last_job->setJobStatus(false);
            cout << last_job->getCommand() << " : " << last_job->getJobPid() << endl;
        }
    }
    else if (num_of_args == 2){
        if (!isNumber(args[1])){
            cerr << "smash error: bg: invalid arguments" << endl;
        }
        jid_t job_id = stoi(args[1]); //stoi converts a string to an integer
        JobsList::JobEntry *job = (smash.getJobsList()).getJobById(job_id);
        if (job != nullptr){
            int job_pid = job->getJobPid();
            if (job->isJobStopped() == false){
                cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            }
            else {
                if (kill(job_id, SIGCONT) == -1){
                    perror("smash error: kill failed");
                    freeArgs(args,num_of_args);
                    return;
                }
                job->setJobStatus(false);
                cout << job->getCommand() << " : " << job->getJobPid() << endl;
            }
        }
        else { //job doesn't exist
            cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        }
    }
    else { //number of arguments isn't valid
        cerr << "smash error: bg: invalid arguments" << endl;
    }
    freeArgs(args,num_of_args);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){}

void KillCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    if (num_of_args != 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        freeArgs(args,num_of_args);
        return;
    }
    char first_sig_char = string(args[1]).at(0);
    if ((!isNumber(args[2])) || (first_sig_char != '-') || (!isNumber(string(args[1]).erase(0)))){
        cerr << "smash error: kill: invalid arguments" << endl;
        freeArgs(args,num_of_args);
        return;
    }
    SmallShell &smash = SmallShell::getInstance();
    int signum = stoi(args[1]); //the minus has been erased
    jid_t job_id = stoi(args[2]);
    JobsList::JobEntry *job = (smash.getJobsList()).getJobById(job_id);
    if (job == nullptr){ //no such job
        cerr << "smash error: kill: job-id " << job_id << " does not exist" <<endl;
        freeArgs(args,num_of_args);
        return;
    }
    pid_t job_pid = job->getJobPid();
    if (kill(job_pid, signum) == -1){ //kill failed
        perror("smash error: kill failed");
        freeArgs(args,num_of_args);
        return;
    }
    cout << "signal number " << signum << " was sent to pid " << job_pid << endl;
    if (signum == SIGSTOP){
        job->setJobStatus(true);
    }
    else if (signum == SIGCONT){
        job->setJobStatus(false);
    }
    freeArgs(args,num_of_args);
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char** p_previous_dir) : BuiltInCommand(cmd_line), p_previous_dir(p_previous_dir) {}

//NOTE TO SELF: Talk to Timna about "cd .."
void ChangeDirCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    if (num_of_args > 2){
        cerr << "smash error: cd: too many arguments" << endl;
        freeArgs(args, num_of_args);
        return;
    }
    long size = pathconf(".", _PC_PATH_MAX);
    char *path = (char *) malloc((size_t) size);
    // if (!path) {
    //     perror("smash error: malloc failed");
    //     freeArgs(args, num_of_args);
    //     return;
    // } DO WE WANT TO CHECK MALLOC SUCCESS?

    string dir_to_set = args[1];
    if (dir_to_set == "-") { //go to previos working directory
        if (!(*p_previous_dir)) { //previous working directory is empty
            cerr << "smash error: cd: OLDPWD not set" << endl;
            free(path);
            freeArgs(args, num_of_args);
            return;
        }
        else { //previous working directory exists     
            if (chdir(*p_previous_dir) == FAIL) {
                perror("smash error: chdir failed");
            }
            else {
                free(*p_previous_dir); //Should we check if p_previous_dir!=null first?
                *p_previous_dir = path; //free prev wd and load new one
            }
            free(path);
            freeArgs(args, num_of_args);
            return;
        }     
    }
    else { //new dir is a path
        if (chdir(args[1]) == FAIL) {
            perror("smash error: chdir failed");
        }
        else {
            free(*p_previous_dir); //Should we check if p_previous_dir!=null first?
            *p_previous_dir = path; //free prev wd and load new one
        }
        freeArgs(args, num_of_args);
        return;
    }
}

