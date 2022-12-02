#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
//#include <iomanip>
#include "Commands.h"
#include <cassert>
//#include <unistd.h>
#include <csignal>
#include <fcntl.h>


using namespace std;


const string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

// ---- Utilities ---- //

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
void removespace(string str);
string _trim(const std::string& s)
{
    //string string = _rtrim(_ltrim(s));
    //removespace(s);
    return _rtrim(_ltrim(s));
}
int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)));
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = nullptr;
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
    int num =0; //num of '-' (should ignore)
    while(str[num] == '-') {
        num++;
    }
    for (int i=num; (unsigned)i< str.length(); i++){
        if (isdigit(str[i]) == false){
            return false;
        }
    }
    return true;
}

bool is_cmd_builtin_bg(string cmd_s){
    string cmd_to_run = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    return (cmd_to_run == "chprompt&") ||
           (cmd_to_run == "showpid&") ||
           (cmd_to_run == "pwd&") ||
           (cmd_to_run == "cd&") ||
           (cmd_to_run == "jobs&") ||
           (cmd_to_run == "fg&") ||
           (cmd_to_run == "bg&") ||
           (cmd_to_run == "quit&") ||
           (cmd_to_run == "kill&");
}

// ---- Small Shell ---- //

SmallShell::~SmallShell() = default;

SmallShell::SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
//    const char* no_ampersand
    string cmd_s = _trim(string(cmd_line));
    bool is_background = _isBackgroundCommand(cmd_line);
    char c_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    if (is_background) {
        strcpy(c_cmd_line, cmd_s.c_str());
        _removeBackgroundSign(c_cmd_line);
        cmd_s = c_cmd_line;
        cmd_s = _trim(cmd_s);
    }
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (cmd_s.find(">>") != string::npos){
        return new RedirectionCommand(cmd_line,true);
    }
    else if (cmd_s.find('>') != string::npos){
        return new RedirectionCommand(cmd_line,false);
    }
    else if (cmd_s.find("|&") != string::npos){
        return new PipeCommand(cmd_line, false);
    }
    else if (cmd_s.find('|') != string::npos){
        return new PipeCommand(cmd_line,true);
    }
    else if (firstWord =="pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(c_cmd_line);
    }
    else if (firstWord == "chprompt") {
        return new ChpromptCommand(cmd_line);
    }
    else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line);
    }
    else if (firstWord == "fg") {
        is_cmd_fg = true;
        return new ForegroundCommand(cmd_line);
    }
    else if (firstWord == "bg") {
        return new BackgroundCommand(cmd_line);
    }
    else if (firstWord == "kill") {
        return new KillCommand(cmd_line);
    }
    else if (firstWord == "quit") {
        return new QuitCommand(cmd_line);
    }
    else if (firstWord == "timeout") {
        return new TimeoutCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line, false, is_background); //TODO: change false to "is_cmd_alarm"
    }
}

Command::Command(const char *cmd_line) : cmd_line(cmd_line) {
    string cmd_trimmed = _trim(string(cmd_line));
    if (_isBackgroundCommand(cmd_line)){
        cmd_no_ampersand = (cmd_trimmed.substr(0, cmd_trimmed.length() - 1)).c_str();
    }
    else{
        cmd_no_ampersand = cmd_trimmed.c_str();
    }
    num_of_args = _parseCommandLine(cmd_no_ampersand, args);
}

void SmallShell::executeCommand(const char *cmd_line) {
    string s_cmd_line = cmd_line;
    if (s_cmd_line.find_first_not_of(WHITESPACE) == string::npos) { //cmd is whitespace
        return;
    }
    //jobs_list.removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();

    // reset all parameters: preparing for a new cmd
    current_cmd = "";
    curr_fg_pid = EMPTY;
    //current_duration = 0;
    //current_alarm_cmd = "";
}

// ---- Jobs List & entry ---- //

JobsList::JobsList() : jobs_list(), jobs_num(1) {}

void JobsList::removeFinishedJobs() {
    vector<JobEntry>::iterator curr_job;
    //int status;
    for (curr_job = jobs_list.begin(); curr_job != jobs_list.end(); curr_job++){
        pid_t pid = waitpid(curr_job->jobPid, nullptr, WNOHANG);
        if (pid != 0) {
            curr_job--;
            jobs_list.erase(curr_job + 1);
            // int stat = WTERMSIG(status);
        }

    }
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int max_stopped_jid = -1;
    vector<JobsList::JobEntry>::iterator it;
    for (it = jobs_list.begin(); it < jobs_list.end(); it++){
        JobsList::JobEntry current_job = *it;
        if (current_job.is_stopped){
            max_stopped_jid = current_job.jobId;
        }
    }
    *jobId = max_stopped_jid;
    return getJobById(max_stopped_jid);

}

JobsList::JobEntry *JobsList::getJobById(jid_t jobId) {
    vector<JobsList::JobEntry>::iterator it;
    for (it = jobs_list.begin(); it < jobs_list.end(); it++){
        if (it->jobId == jobId){
            return &(*it);
        }
    }
    return nullptr;
}
JobsList::JobEntry* JobsList::getJobByPId(pid_t jobPId){
    vector<JobEntry>::iterator it;
    for(it =jobs_list.begin(); it!= jobs_list.end(); it++){
        if(it->jobPid==jobPId){
            return &(*it);
        }
    }
    return nullptr;
}

JobsList::JobEntry::JobEntry(jid_t jobId, pid_t jobPid, time_t creation_time, string &command, bool is_stopped, int duration) :
        jobId (jobId),
        jobPid(jobPid),
        creation_time(creation_time),
        command (command),
        is_stopped(false),
        duration(duration) {}

time_t JobsList::JobEntry::getElapsedTime() const {
    time_t curr_time;
    time(&curr_time);
    int elapsed_time = (int) difftime(curr_time, creation_time);
    return elapsed_time;
}

void JobsList::killAllJobs() {
    vector<JobEntry>::iterator curr_job;
    for(curr_job = jobs_list.begin(); curr_job != jobs_list.end();  curr_job++) {
        cout << curr_job->jobPid << ": " << curr_job->command << endl;
        if(kill(curr_job->jobPid, SIGKILL) == FAIL) {
            perror("smash error: kill failed");
        }
    }
}

void JobsList::removeJobById(jid_t id){
    vector<JobsList::JobEntry>::iterator curr_job;
    for(curr_job = jobs_list.begin(); curr_job != jobs_list.end(); curr_job++)
    {
        if(curr_job->jobId == id) {
            jobs_list.erase(curr_job);
            return;
        }
    }
}

void JobsList::addJob(string cmd, pid_t pid, int duration, bool is_stopped) {
    int id=1;
    removeFinishedJobs();
    time_t timestamp;
    time(&timestamp);
    SmallShell& smash = SmallShell::getInstance();

    if(!jobs_list.empty())
    {
        id=jobs_list.back().jobId + 1;
    }

    if(smash.fg_jid!=EMPTY) {
        int curr_job_id = smash.fg_jid;
        JobEntry new_job(curr_job_id, pid, timestamp, cmd, is_stopped, duration);
        vector<JobEntry>::iterator it;
        int i=0;
        for (it = jobs_list.begin(); it != jobs_list.end(); it++) {
            if (it->jobId > curr_job_id){
                break;
            }
            i++;
        }
        jobs_list.insert(jobs_list.begin() + i, new_job);
    }
    else{
        JobEntry new_job(id,pid,timestamp,cmd,is_stopped,duration);
        jobs_list.push_back(new_job);
    }
}

time_t SmallShell::getMostRecentAlarmTime() {
    alarms_list.removeFinishedJobs();
    if (alarms_list.jobs_list.size() == 0) {
        return EMPTY;
    }
    vector<JobsList::JobEntry>::iterator curr_job = alarms_list.jobs_list.begin();
    time_t time = curr_job->duration - curr_job->getElapsedTime();
    curr_job++;
    for (; curr_job != alarms_list.jobs_list.end(); curr_job++){
        if (curr_job->duration - curr_job->getElapsedTime() < time){
            time = curr_job->duration - curr_job->getElapsedTime();
        }
    }
    return time;
}

JobsList::JobEntry* SmallShell::getTimedOutJob(){

    this->alarms_list.removeFinishedJobs();
    vector<JobsList::JobEntry>::iterator curr_job;
    for (curr_job = alarms_list.jobs_list.begin(); curr_job != alarms_list.jobs_list.end(); curr_job++) {
        if (curr_job->getElapsedTime() >= curr_job->duration)
            return &(*curr_job);
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){
    *lastJobId = jobs_list.back().jobId;
    return &jobs_list.back();
}

// ---- Built-in ---- //

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    int num_of_args = 0;

    char **args = makeArgs(cmd_line, &num_of_args);

    SmallShell &smash = SmallShell::getInstance();
    if (num_of_args == 1){
        smash.prompt = "smash";
    }
    else {
        smash.prompt = args[1];
    }
    freeArgs(args,num_of_args);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute() {
    long max_path_length = pathconf(".", _PC_PATH_MAX); //pathconf returns the max length of "." path (currDir)
    char buffer[max_path_length];
    getcwd(buffer,max_path_length);
    cout << buffer << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    if (num_of_args > 2){
        cerr << "smash error: cd: too many arguments" << endl;
        freeArgs(args, num_of_args);
        return;
    }
    SmallShell &smash = SmallShell::getInstance();
    long max_path_length = pathconf(".", _PC_PATH_MAX);
    char buffer[max_path_length];
    string current_wd = getcwd(buffer, max_path_length);
    string new_wd = args[1];
    if (new_wd == "-"){
        string previous_wd = smash.previous_dir;
        if (previous_wd.empty()) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            freeArgs(args, num_of_args);
            return;
        }
        else if (chdir(previous_wd.c_str()) == FAIL){
            cerr << "smash error: cd: OLDPWD not set" << endl;
            freeArgs(args, num_of_args);
            return;
        }
    }
    else if(chdir(new_wd.c_str()) == FAIL) {
        perror("smash error: chdir failed");
        return;
    }
    smash.previous_dir = current_wd;
}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    (smash.jobs_list).removeFinishedJobs();
    vector<JobsList::JobEntry>::iterator it;
    for (it = ((smash.jobs_list).jobs_list).begin(); it < ((smash.jobs_list).jobs_list).end(); it++){
        JobsList::JobEntry current_job = *it;
        if (current_job.is_stopped){
            //difftime() function returns the number of seconds elapsed between time time1 and time time0, represented as a double
            //time(nullptr) returns the current calendar time as an object of type time_t
            cout << "[" << current_job.jobId << "] " << current_job.command << " : " << current_job.jobPid
                 << " " << current_job.getElapsedTime() << " secs (stopped)" << endl;
        }
        else {
            cout << "[" << current_job.jobId << "] " << current_job.command << " : " << current_job.jobPid
                 << " " << current_job.getElapsedTime() << " secs" << endl;
        }
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

ForegroundCommand::~ForegroundCommand(){
    SmallShell& smash = SmallShell::getInstance();
    smash.fg_jid = EMPTY;
    smash.curr_fg_pid = EMPTY;
}
void ForegroundCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs_list.removeFinishedJobs();
    if (num_of_args == 1) { //Bring max job in list to foreground
        int max_jid;
        JobsList::JobEntry *job = smash.jobs_list.getLastJob(&max_jid);

        if (!job) {
            cerr << "smash error: fg: jobs list is empty" << endl;
        }
        if (job->is_stopped) {
            if(kill(job->jobPid, SIGCONT) == FAIL) { // need to bring job to background
                perror("smash error: kill failed");
                freeArgs(args, num_of_args);
                return;
            }
        } // now job is in backround
        pid_t job_pid = job->jobPid;
        jid_t job_id = job->jobId;
        cout << job->command << " : " << job->jobPid << endl;
        //   smash.current_process_pid = job_pid;
        smash.curr_fg_pid = job_pid;
        smash.current_cmd = job->command;
        smash.fg_jid = job_id;
        smash.jobs_list.removeJobById(job_id);
        int stat_loc;
        if (waitpid(job_pid, &stat_loc, WUNTRACED) == FAIL) {
            perror("smash error: waitpid failed");
            freeArgs(args, num_of_args);
            return;
        }
    }

    else if (num_of_args == 2) { //Bring wanted job to foreground
        if(!isNumber(args[1])) {
            cerr << "smash error: fg: invalid arguments" << endl;
            freeArgs(args, num_of_args);
            return;
        }
        jid_t job_id = stoi(args[1]);
        JobsList::JobEntry *job = (smash.jobs_list).getJobById(job_id);
        if (job){
            int job_pid = job->jobPid;
            if (job->is_stopped){ // need to bring job to background
                if(kill(job_pid, SIGCONT) == FAIL) {
                    perror("smash error: kill failed");
                    freeArgs(args, num_of_args);
                    return;
                }
            }
            // now job is in backround
            cout << job->command << " : " << job->jobPid << endl;

            //       smash.current_process_pid = job_pid;
            smash.curr_fg_pid = job_pid;
            smash.current_cmd = job->command;
            smash.fg_jid = job_id;
            smash.jobs_list.removeJobById(job_id);

            int stat_loc;
            if (waitpid(job_pid, &stat_loc, WUNTRACED) == FAIL) {
                perror("smash error: waitpid failed");
                freeArgs(args, num_of_args);
                return;
            }
        }
        else {
            cerr << "smash error: fg: job-id" << job_id << "does not exist" << endl;
        }
        freeArgs(args, num_of_args);
        return;
    }

    else {
        cerr << "smash error: fg: invalid arguments" << endl;
        freeArgs(args, num_of_args);
        return;
    }
    //smash.setCmdIsFg(false);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void BackgroundCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid = -1;
    //if no arguments - the last stopped job (in the jobs list, the one with mac JID) should be selected to continue running it in bg
    if (num_of_args == 1){
        jid_t last_job_jid;
        JobsList::JobEntry *last_job = (smash.jobs_list).getLastStoppedJob(&last_job_jid);
        if (last_job_jid == -1){
            //can also use cout but it's good practice to use cerr
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        }
        else {
            //kill returns 0 for success and -1 for failure
            pid = last_job->jobPid;
            if (kill(pid, SIGCONT) == -1){
                perror("smash error: kill failed");
                freeArgs(args,num_of_args);
                return;
            }
            last_job->is_stopped = false;
            cout << last_job->command << " : " << pid << endl;
        }
    }
    else if (num_of_args == 2){
        if (!isNumber(args[1])){
            cerr << "smash error: bg: invalid arguments" << endl;
        }
        jid_t job_id = stoi(args[1]); //stoi converts a string to an integer
        JobsList::JobEntry *job = (smash.jobs_list).getJobById(job_id);
        if (job != nullptr){
            if (!(job->is_stopped)){
                cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            }
            else {
                pid = job->jobPid;
                if (kill(pid, SIGCONT) == -1){
                    perror("smash error: kill failed");
                    freeArgs(args,num_of_args);
                    return;
                }
                job->is_stopped = false;
                cout << job->command << " : " << job->jobPid << endl;
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

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void KillCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    if (num_of_args != 3 || !((args[1])[0]=='-' && (isNumber(args[1]))) || !(isNumber(args[2]))){
        cerr << "smash error: kill: invalid arguments" << endl;
        freeArgs(args,num_of_args);
        return;
    }

    SmallShell &smash = SmallShell::getInstance();
    string signal_str = args[1];
    signal_str = signal_str.erase(0,1);
    int signum = stoi(signal_str); //the minus has been erased
    int job_id = stoi(args[2]);
    smash.jobs_list.removeFinishedJobs();
    JobsList::JobEntry *job = (smash.jobs_list).getJobById(job_id);
    if (job == nullptr){ //no such job
        cerr << "smash error: kill: job-id " << job_id << " does not exist" <<endl;
        freeArgs(args,num_of_args);
        return;
    }
    pid_t job_pid = job->jobPid;
    if (kill(job_pid, signum) == -1){ //kill failed
        perror("smash error: kill failed");
        freeArgs(args,num_of_args);
        return;
    }
    cout << "signal number " << signum << " was sent to pid " << job_pid << endl;
    if (signum == SIGSTOP){
        job->is_stopped = true;
    }
    else if (signum == SIGCONT){
        job->is_stopped = false;
    }
    freeArgs(args,num_of_args);
}

QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
    (smash.jobs_list).removeFinishedJobs();
    if (num_of_args > 1 && string(args[1]) == "kill") {
        cout << "smash: sending SIGKILL signal to " << smash.jobs_list.jobs_list.size() << " jobs:" << endl;
        smash.jobs_list.killAllJobs();
    }
    freeArgs(args,num_of_args);
    //delete this;
    exit(0);
}


// ---- External ---- //

ExternalCommand::ExternalCommand(const char* cmd_line, bool is_alarm, bool is_background) :
        Command(cmd_line),
        is_alarm(is_alarm),
        is_background(is_background){}

bool ExternalCommand::isCmdComplex(string cmd) {
    return ((cmd.find_first_of('*') != string::npos) || (cmd.find_first_of('?') != string::npos));
}

void ExternalCommand::execute() {
//    int num_of_args = 0;
//    char **args = makeArgs(cmd_no_ampersand, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();
//    char *temp = args[1];
//    cout << args[1] << endl;
    pid_t pid = fork();
    if (pid == FAIL) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) { //process is son
        if (setpgrp() == FAIL) {
            perror("smash error: setpgrp failed");
            return;
        }
        if (isCmdComplex(cmd_line)) {
            if (execl("/bin/bash", "bin/bash", "-c", cmd_no_ampersand, nullptr) == FAIL) {
                perror("smash error: execl failed");
                return;
            }
        } else { //simple command
            if (execvp(args[0], args) == FAIL) {
                perror("smash error: execvp failed");
                return;
            }
        }
    } else { //process is original (father)
        if (is_background) { //background
            smash.curr_fg_pid = EMPTY;
            smash.jobs_list.addJob(cmd_line, pid);
        } else { //foreground
            smash.curr_fg_pid = pid;
            smash.current_cmd = cmd_line;
            int stat_loc;
            if (waitpid(pid, &stat_loc, WUNTRACED) == FAIL) {
                perror("smash error: waitpid failed");
                return;
            }
            smash.curr_fg_pid = EMPTY;
            smash.current_cmd = "";
            smash.fg_jid = EMPTY;
        }
    }
}

void ExternalCommand::timeoutExecute(TimeoutCommand* cmd) {
    int num_of_args = 0;
    //char **args = makeArgs(cmd_line, &num_of_args);
    SmallShell &smash = SmallShell::getInstance();

    pid_t pid = fork();
    if (pid == FAIL) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) { //process is son
        if (setpgrp() == FAIL) {
            perror("smash error: setpgrp failed");
            return;
        }
        if (execl("/bin/bash", "bin/bash", "-c", cmd_no_ampersand, nullptr) == FAIL) { //TODO: need to remove "&"?
            perror("smash error: execl failed");
            return;
        }
    } else { //process is original (father)
        cmd->addAlarm(pid);
        if (_isBackgroundCommand(cmd->getCmdLine())) {
            smash.curr_fg_pid = EMPTY;
            smash.jobs_list.addJob(cmd->getCmdLine(), pid);
        } else {
            smash.curr_fg_pid = pid;
            int stat_loc;
            if (waitpid(pid, &stat_loc, WUNTRACED) == FAIL) {
                perror("smash error: waitpid failed");
                return;
            }
            smash.curr_fg_pid = EMPTY;
            smash.fg_jid = EMPTY;
        }
    }
}


// ---- Time Out ---- //

TimeoutCommand::TimeoutCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void TimeoutCommand::execute() {
    int num_of_args = 0;
    char **args = makeArgs(cmd_line, &num_of_args);
    int timeout_duration = stoi(args[1]);
    this->time_out = timeout_duration;
    SmallShell &smash = SmallShell::getInstance();
    Command *command = smash.CreateCommand(cmd_line); //TODO: check
    command->setCmd(cmd_line);
    auto *ex_cmd = dynamic_cast<ExternalCommand *>(command);
    if (!ex_cmd) {
        ex_cmd->timeoutExecute(this);
    } else {
        command->execute();
    }
}


void SmallShell::addTimeoutToAlarm(const char* cmd, pid_t pid, int duration)
{
    string cmd_line = string(cmd);
    alarms_list.addJob(cmd_line, false, duration);
}

void TimeoutCommand::addAlarm(pid_t pid) const{
    SmallShell& smash = SmallShell::getInstance();
    time_t min_alarm = smash.getMostRecentAlarmTime();
    if (min_alarm == EMPTY || time_out < min_alarm) {
        alarm(time_out);
        SmallShell::getInstance().addTimeoutToAlarm(cmd_line, pid, time_out);
    }
    else
        SmallShell::getInstance().addTimeoutToAlarm(cmd_line, pid, time_out);
}



// ---- Pipe ---- //
PipeCommand::PipeCommand(const char* cmd_line, bool is_stdout) : Command(cmd_line), is_stdout(is_stdout){}
void PipeCommand::execute(){
    int fds[2];
    if (pipe(fds) == FAIL){
        perror("smash error: pipe failed");
        return;
    }
    string command1,command2;
    if (is_stdout) {
        command1 = string(cmd_no_ampersand).substr(0, string(cmd_no_ampersand).find_first_of('|'));
        command1 =  _trim(command1);
        command2 = string(cmd_no_ampersand).substr(string(cmd_no_ampersand).find_first_of('|') + 1, string(cmd_no_ampersand).size());
        command2 = _trim(command2);
    } else {
        command1 = string(cmd_no_ampersand).substr(0, string(cmd_no_ampersand).find_first_of("|&"));
        command1 =  _trim(command1);
        command2 = string(cmd_no_ampersand).substr(string(cmd_no_ampersand).find_first_of("|&") + 2, string(cmd_no_ampersand).size());
        command2 = _trim(command2);
    }
    int pid1 = fork();
    if (pid1 == FAIL){
        perror("smash error: fork failed");
        if (close(fds[0]) == FAIL){
            perror("smash error: close failed");
        }
        if (close(fds[1]) == FAIL){
            perror("smash error: close failed");
        }
        return;
    }
    if (pid1 == 0){
        SmallShell &smash = SmallShell::getInstance();
        if (dup2(fds[1], (is_stdout ? 1 : 2)) == FAIL){
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fds[0]) == FAIL){
            perror("smash error: close failed");
            return;
        }
        if (close(fds[1]) == FAIL){
            perror("smash error: close failed");
            return;
        }
        // Command *cmd = smash.CreateCommand(command1.c_str());
        smash.executeCommand(command1.c_str());
        if (kill(getpid(), SIGKILL) == FAIL){
            perror("smash error: kill failed");
            return;
        }
        return;
    }
    int pid2 = fork();
    if (pid2 == FAIL){
        perror("smash error: fork failed");
        if (close(fds[0]) == FAIL){
            perror("smash error: close failed");
        }
        if (close(fds[1]) == FAIL){
            perror("smash error: close failed");
        }
        return;
    }
    if (pid2 == 0){
        SmallShell &smash = SmallShell::getInstance();
        if (dup2(fds[0], 0) == FAIL){
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fds[0]) == FAIL){
            perror("smash error: close failed");
            return;
        }
        if (close(fds[1]) == FAIL){
            perror("smash error: close failed");
            return;
        }
        //  Command *cmd = smash.CreateCommand(command2.c_str());
        smash.executeCommand(command2.c_str());
        if (kill(getpid(), SIGKILL) == FAIL){
            perror("smash error: kill failed");
            return;
        }
        return;
    }
    if (close(fds[0]) == FAIL){
        perror("smash error: close failed");
        return;
    }
    if (close(fds[1]) == FAIL){
        perror("smash error: close failed");
        return;
    }
    if (waitpid(pid1, nullptr,0) == FAIL){
        perror("smash error: waitpid failed");
        return;
    }
    if (waitpid(pid2, nullptr,0) == FAIL){
        perror("smash error: waitpid failed");
        return;
    }
}

// ---- Redirection ---- //
RedirectionCommand::RedirectionCommand(const char* cmd_line, bool append) : Command(cmd_line), is_append(append){
    //remove ampersand - not sure if needed
    if (append){
        command = string(cmd_no_ampersand).substr(0,string(cmd_no_ampersand).find_first_of(">>"));
        command = _trim(command);
        file_name = string(cmd_no_ampersand).substr(string(cmd_no_ampersand).find_first_of(">>")+2);
        file_name = _trim(file_name);
    }
    else {
        command = string(cmd_no_ampersand).substr(0,string(cmd_no_ampersand).find_first_of('>'));
        command = _trim(command);
        file_name = string(cmd_no_ampersand).substr(string(cmd_no_ampersand).find_first_of('>')+1);
        file_name = _trim(file_name);
    }
}

void RedirectionCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    int fd;
    if (is_append){
        fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_APPEND, 0655);
    }
    else {
        fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0655);
    }
    if (fd == FAIL){
        perror("smash error: open failed");
        return;
    }
    int stdout_temp_fd = dup(STDOUT_FILENO);
    if (stdout_temp_fd == FAIL){
        perror("smash error: dup failed");
        return;
    }
    if (dup2(fd, STDOUT_FILENO) == FAIL){
        perror("smash error: dup2 failed");
        return;
    }
    if (close(fd) == FAIL){
        perror("smash error: close failed");
        return;
    }
    smash.executeCommand(cmd_line); //maybe command line??
    if (dup2(stdout_temp_fd,STDOUT_FILENO) == FAIL){
        perror("smash error: dup2 failed");
        return;
    }
    if (close(stdout_temp_fd) == FAIL){
        perror("smash error: close failed");
        return;
    }

}
