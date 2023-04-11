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
    //puts the command then the parameters in args, and returns the number of things it put in args
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

bool _isBackgroundComamnd(const char* cmd_line) {
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
chpromptCommand::chpromptCommand(const char *cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line){
    this->smash = smash;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        this->msg = "smash";
    }
    else{
        this->msg = args[1];
    }
}

void chpromptCommand::execute() {
    this->smash->setMsg(this->msg);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid()<<endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[INT8_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        cout << cwd << endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line){
    this->smash = smash;
    this->change = false;
    this->last_dir = "";
    this->curr_dir = "";
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 2){
        std::string temp = args[1];
        char cwd[INT8_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            std::string cwdir = cwd;
            if(temp.compare("-") == 0){
                if(smash->getLastDir().compare("")==0){
                    cerr << "smash error: cd: OLDPWD not set" << endl;
                    return;
                }
                else {
                    this->last_dir = cwdir;
                    this->curr_dir = smash->getLastDir();
                    this->change = true;
                }
            }
            else {
                this->last_dir = cwdir;
                this->curr_dir = args[1];
                this->change = true;
            }
        } else {
            perror("smash error: getcwd failed");
            return;
        }
    }
    if(num>2){
        cerr << "smash error: cd: too many arguments"<<endl;
    }
}

void ChangeDirCommand::execute() {
    if(this->change) {
        this->smash->setLastDir(this->last_dir);
        if (chdir((this->curr_dir).c_str()) == -1) {
            perror("smash error: chdir failed");
        }
    }
}

JobsList::JobsList() {
    this->list = std::vector<JobEntry*>();
    this->max_job_id = 0;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    JobEntry* je = new JobEntry();
    this->max_job_id = this->max_job_id+1;
    je->job_id = this->max_job_id;
    je->cmd_line = cmd->getCmdLine();
    time_t temp = time(nullptr);
    if(temp == ((time_t) -1)){
        perror("smash error: time failed");
        return;
    }
    else{
        je->in_time = temp;
    }
    je->pid = pid;
    je->stopped = isStopped;

    this->list.push_back(je);
}

void JobsList::printJobsList() {
    for(int j = 0; j<int(this->list.size()); j++){
        cout << this->list[j]->toString() << endl;
    }
}

void JobsList::removeJobById(int jobId) {
    for(int j = 0; j<int(this->list.size()); j++){
        if(jobId == this->list[j]->job_id){
            this->list.erase(this->list.cbegin()+j);
        }
    }

    //update max job id
    this->max_job_id = 0;
    for (int i = 0; i < int(this->list.size()); i++) {
        if(this->list[i]->job_id>this->max_job_id){
            this->max_job_id = this->list[i]->job_id;
        }
    }
}

void JobsList::removeFinishedJobs() {
    std::vector<int> jobsid_to_delete;
    for(int i = 0; i<int(this->list.size()); i++){
        pid_t pid = waitpid(this->list[i]->pid, NULL, WNOHANG);
        if(pid == -1)//error
            perror("smash error: execvp failed");
        if(pid > 0){//finished
            jobsid_to_delete.push_back(this->list[i]->job_id);
        }
    }

    for(int i = 0; i<int(jobsid_to_delete.size()); i++){
        this->removeJobById(jobsid_to_delete[i]);
    }

    //max job id is updated in removeJobById()
}

bool JobsList::exsits(int jobid) {
    for(int i = 0; i<int(this->list.size()); i++){
        if(jobid == this->list[i]->job_id)
            return true;
    }
    return false;
}

int JobsList::maxJobId() {
    return this->max_job_id;
}

JobEntry *JobsList::getJobById(int jobId) {
    for(int i = 0; i<int(this->list.size()); i++){
        if(jobId == this->list[i]->job_id)
            return this->list[i];
    }
    return nullptr;
}

JobEntry *JobsList::getLastStoppedJob() {
    int max_id = 0;
    for(int i = 0; i<int(this->list.size()); i++){
        if((max_id < this->list[i]->job_id) and (this->list[i]->stopped))
            max_id = this->list[i]->job_id;
    }
    if(max_id == 0)
        return nullptr;
    else
        return this->getJobById(max_id);
}

void JobsList::killAllJobs() {
    std::string str = "smash: sending SIGKILL signal to ";
    str += std::to_string(int(this->list.size()));
    str +=  " jobs:";
    cout << str << endl;

    for(int i = 0; i<int(this->list.size()); i++){
        cout << this->list[i]->toString_Kill() << endl;
        int err = kill(this->list[i]->pid, SIGKILL);
        if(err == -1){
            perror("smash error: kill failed");
            return;
        }
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){
    this->jobs = jobs;
}

void JobsCommand::execute() {
    this->jobs->removeFinishedJobs();
    this->jobs->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        if(jobs->maxJobId() == 0){
            cerr << "smash error: fg: jobs list is empty" << endl;
        }
        else{
            this->job_id = jobs->maxJobId();
        }
        return;
    }
    if(num == 2){
        int jobid = stoi(args[1]);
        if(jobs->exsits(jobid)){
            this->job_id = jobid;
        }
        else{
            std::string str = "smash error: fg: job-id ";
            str += args[1];
            str += " does not exist";
            cerr << str << endl;
        }
        return;
    }
    cerr << "smash error: fg: invalid arguments" << endl;
}

void ForegroundCommand::execute() {
    JobEntry* job = this->list->getJobById(this->job_id);
    cout << job->toString(true) << endl;
    int err = kill(job->pid, SIGCONT);
    if(err == -1){
        perror("smash error: kill failed");
        return;
    }
    this->list->removeJobById(this->job_id);
    waitpid(job->pid, NULL, 0);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        JobEntry* job = jobs->getLastStoppedJob();
        if(job == nullptr){
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        }
        else{
            this->job_id = job->job_id;
        }
        return;
    }
    if(num == 2){
        int jobid = stoi(args[1]);
        if(!(jobs->exsits(jobid))){
            std::string str = "smash error: bg: job-id ";
            str += args[1];
            str += " does not exist";
            cerr << str << endl;
        }
        else {
            JobEntry *job = jobs->getJobById(jobid);
            if (job->stopped) {
                this->job_id = jobid;
            } else {
                std::string str = "smash error: bg: job-id ";
                str += args[1];
                str += " is already running in the background";
                cerr << str << endl;
            }
        }
        return;
    }
    cerr << "smash error: bg: invalid arguments" << endl;
}

void BackgroundCommand::execute() {
    JobEntry* job = this->list->getJobById(this->job_id);
    cout << job->toString(true) << endl;
    int err = kill(job->pid, SIGCONT);
    if(err == -1){
        perror("smash error: kill failed");
        return;
    }
    job->stopped = false;
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    this->list = jobs;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        this->iskill = false;
        return;
    }
    std::string str = args[1];
    if (str.compare("kill") == 0) {
        this->iskill = true;
    }
    else{
        this->iskill = false;
    }
}

void QuitCommand::execute() {
    if(this->iskill){
        this->list->killAllJobs();
    }
    exit(1);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 3){
        this->job_id = stoi(args[2]);
        if(!jobs->exsits(this->job_id)){
            cerr << "smash error: kill: job-id" << std::to_string(this->job_id) <<" does not exist" << endl;
            return;
        }
        std::string str = args[1];
        if(str[0] != '-'){
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        else{
            std::string temp = str.substr(1, int(str.size())-1);
            this->sig_num = stoi(temp);
        }
    }
    else{
        cerr << "smash error: kill: invalid arguments" << endl;
    }
}

void KillCommand::execute() {
    JobEntry* job = this->list->getJobById(this->job_id);
    int err = kill(job->pid, this->sig_num);
    if(err == -1){
        perror("smash error: kill failed");
        return;
    }

    std::string str = "";
    str += "signal number ";
    str += std::to_string(this->sig_num);
    str += " was sent to pid ";
    str += std::to_string(job->pid);
    cout << str << endl;
}

ExternalCommand::ExternalCommand(const char *cmd_line, SmallShell* smash) : Command(cmd_line){
    this->isback = _isBackgroundComamnd(cmd_line);
    this->smash = smash;

    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    string cmd_s = _trim(string(cmd));
    this->command = cmd_s.substr(0, cmd_s.find_first_of(" \n")).c_str();

    this->args = new char*[20];
    _parseCommandLine(cmd, this->args);//without the &

    this->iscomplex =false;
    std::string temp = cmd_line;
    for(int i = 0; i<(int)temp.size(); i++){
        if(temp[i] == '?' or temp[i] == '*'){
            this->iscomplex = true;
            return;
        }
    }
}

void ExternalCommand::execute() {
    int num;
    if(this->iscomplex){//complex
        if(this->isback){//back
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                num = execlp("/bin/bash", "/bin/bash", "-c", this->getCmdLine().c_str(), nullptr);
                if(num==-1)
                    perror("smash error: execvp failed");
                return;
            }
            else{//father
                this->smash->add_job(this, pid);
                return;
            }
        }
        else{//front
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                num = execlp("/bin/bash", "/bin/bash", "-c", this->getCmdLine().c_str(), nullptr);
                if(num==-1)
                    perror("smash error: execvp failed");
                return;
            }
            else{//father
                waitpid(pid, NULL, 0);
                return;
            }
        }
    }
    else{//simple
        if(this->isback){//back
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                num = execvp(this->command, this->args);
                if(num==-1)
                    perror("smash error: execvp failed");
                return;
            }
            else{//father
                this->smash->add_job(this, pid);
                return;
            }
        }
        else{//front
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                num = execvp(this->command, this->args);
                if(num==-1)
                    perror("smash error: execvp failed");
                return;
            }
            else{//father
                waitpid(pid, NULL, 0);
                return;
            }
        }
    }
}

void SmallShell::add_job(Command *cmd, pid_t pid, bool isStopped) {
    this->killFinishedJobs();
    this->jobs_list->addJob(cmd, pid, isStopped);
}
void SmallShell::setMsg(const std::string msg) {
    this->msg = msg;
}

const std::string SmallShell::getMsg() {
    return this->msg;
}

const std::string SmallShell::getLastDir() {
    return this->last_dir;
}

void SmallShell::killFinishedJobs() {
    this->jobs_list->removeFinishedJobs();
}

void SmallShell::setLastDir(const std::string last_dir) {
    this->last_dir = last_dir;
}

SmallShell::SmallShell() {
// TODO: add your implementation
    this->msg = "smash";
    this->last_dir = "";
    this->jobs_list = new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    // For example:

    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
  string cmd_s = _trim(string(cmd));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    //built-in commands

  if (firstWord.compare("chprompt") == 0) {
    return new chpromptCommand(cmd_line, this);
  }
  if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
  }
  if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, this);
  }
  if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line, this->jobs_list);
  }
  if (firstWord.compare("fg") == 0) {
      return new ForegroundCommand(cmd_line, this->jobs_list);
  }
  if (firstWord.compare("bg") == 0) {
      return new BackgroundCommand(cmd_line, this->jobs_list);
  }
    if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, this->jobs_list);
    }
    if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, this->jobs_list);
    }

    //external commands
    return new ExternalCommand(cmd_line, this);
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
}