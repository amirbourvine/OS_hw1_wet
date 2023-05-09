#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
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

int is_IO_Pipe(const char *cmd_line){
    //return 0 if not, 1 for >, 2 for >>, 3 for |, 4 for |&
    std::string str = cmd_line;
    if(str.find(">>") != std::string::npos){
        return 2;
    }
    if(str.find(">") != std::string::npos){
        return 1;
    }
    if(str.find("|&") != std::string::npos){
        return 4;
    }
    if(str.find("|") != std::string::npos){
        return 3;
    }
    return 0;
}

bool is_IO(const char *cmd_line){
    return (is_IO_Pipe(cmd_line) == 1) or (is_IO_Pipe(cmd_line) == 2);
}

bool is_PIPE(const char *cmd_line){
    return (is_IO_Pipe(cmd_line) == 3) or (is_IO_Pipe(cmd_line) == 4);
}

bool is_Built_in(const char *cmd_line){
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    string cmd_s = _trim(string(cmd));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    //built-in commands

    if (firstWord.compare("chprompt") == 0) {
        return true;
    }
    if (firstWord.compare("showpid") == 0) {
        return true;
    }
    if (firstWord.compare("pwd") == 0) {
        return true;
    }
    if (firstWord.compare("cd") == 0) {
        return true;
    }
    if (firstWord.compare("jobs") == 0) {
        return true;
    }
    if (firstWord.compare("fg") == 0) {
        return true;
    }
    if (firstWord.compare("bg") == 0) {
        return true;
    }
    if (firstWord.compare("quit") == 0) {
        return true;
    }
    if (firstWord.compare("kill") == 0) {
        return true;
    }
    if (firstWord.compare("setcore") == 0) {
        return true;
    }
    if (firstWord.compare("getfiletype") == 0) {
        return true;
    }
    if (firstWord.compare("chmod") == 0) {
        return true;
    }
    if (firstWord.compare("timeout") == 0) {
        return true;
    }

    return false;
}

// TODO: Add your implementation for classes in Commands.h
chpromptCommand::chpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line){
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
    SmallShell& smash = SmallShell::getInstance();
    smash.setMsg(this->msg);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.get_pid() <<endl;
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
    if(num == 1){
        this->change = false;
    }
    if(num>2){
        cerr << "smash error: cd: too many arguments"<<endl;
    }
}

void ChangeDirCommand::execute() {
    if(this->change) {
        if (chdir((this->curr_dir).c_str()) == -1) {
            perror("smash error: chdir failed");
        }
        else{
            this->smash->setLastDir(this->last_dir);
        }
    }
}

JobsList::JobsList() {
    this->list = std::vector<JobEntry*>();
    this->max_job_id = 0;
}

JobsList::~JobsList(){
    for(auto j : list)
        delete j;
}

void JobsList::addJob(std::string cmd, pid_t pid, int job_id, bool isStopped) {
    JobEntry* je = new JobEntry();

    if(job_id==-1) {
        this->max_job_id = this->max_job_id + 1;
        je->job_id = this->max_job_id;
    }
    else{
        je->job_id = job_id;
    }

    je->cmd_line = cmd;
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
    this->update_max_id();
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
    this->update_max_id();
}

void JobsList::update_max_id() {
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
            perror("smash error: waitpid failed");
        if(pid > 0){//finished
            jobsid_to_delete.push_back(this->list[i]->job_id);
        }
    }

    for(int i = 0; i<int(jobsid_to_delete.size()); i++){
        this->removeJobById(jobsid_to_delete[i]);
    }

    //max job id is updated in removeJobById()
}

bool compareJob_Entry(JobEntry* j1, JobEntry* j2){
    return j1->job_id < j2->job_id;
}

void JobsList::sort() {
    std::sort(this->list.begin(), this->list.end(), compareJob_Entry);
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
    this->jobs->sort();
    this->jobs->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        if(jobs->maxJobId() == 0){
            cerr << "smash error: fg: jobs list is empty" << endl;
            this->exe = false;
        }
        else{
            this->job_id = jobs->maxJobId();
        }
        return;
    }
    if(num >= 2){
        int jobid;
        try {
            jobid = stoi(args[1]);
        }
        catch (invalid_argument& e){
            cerr << "smash error: fg: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        if(jobs->exsits(jobid)){
            this->job_id = jobid;
        }
        else{
            std::string str = "smash error: fg: job-id ";
            str += args[1];
            str += " does not exist";
            cerr << str << endl;
            this->exe = false;
            return;
        }
        if(num != 2){
            cerr << "smash error: fg: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        return;
    }
    cerr << "smash error: fg: invalid arguments" << endl;
    this->exe = false;
}

void ForegroundCommand::execute() {
    if(this->exe == false){
        return;
    }
    JobEntry* job = this->list->getJobById(this->job_id);
    cout << job->toString(true) << endl;
    int err = kill(job->pid, SIGCONT);
    if(err == -1){
        perror("smash error: kill failed");
        return;
    }
    SmallShell& smash = SmallShell::getInstance();
    smash.set_foreground_job_pid(job->pid);
    smash.set_foreground_job_id(this->job_id);
    smash.set_foreground_job_cmd(job->cmd_line);
    this->list->removeJobById(this->job_id);
    waitpid(job->pid, NULL, WUNTRACED);
    smash.set_foreground_job_cmd("");
    smash.set_foreground_job_pid(-1);
    smash.set_foreground_job_id(-1);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 1){
        JobEntry* job = jobs->getLastStoppedJob();
        if(job == nullptr){
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            this->exe = false;
        }
        else{
            this->job_id = job->job_id;
        }
        return;
    }
    if(num == 2){
        int jobid;
        try {
            jobid = stoi(args[1]);
        }
        catch (invalid_argument& e){
            cerr << "smash error: bg: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        if(!(jobs->exsits(jobid))){
            std::string str = "smash error: bg: job-id ";
            str += args[1];
            str += " does not exist";
            cerr << str << endl;
            this->exe = false;
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
                this->exe = false;
            }
        }
        return;
    }
    cerr << "smash error: bg: invalid arguments" << endl;
    this->exe = false;
}

void BackgroundCommand::execute() {
    if(this->exe == false){
        return;
    }
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
    exit(0);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num >= 3){
        try {
            this->job_id = stoi(args[2]);
        }
        catch (invalid_argument& e){
            cerr << "smash error: kill: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        if(!jobs->exsits(this->job_id)){
            cerr << "smash error: kill: job-id " << std::to_string(this->job_id) <<" does not exist" << endl;
            this->exe = false;
            return;
        }
        std::string str = args[1];
        if(str[0] != '-'){
            cerr << "smash error: kill: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        else{
            std::string temp = str.substr(1, int(str.size())-1);
            try {
                this->sig_num = stoi(temp);
            }
            catch (invalid_argument& e){
                cerr << "smash error: kill: invalid arguments" << endl;
                this->exe = false;
                return;
            }
        }
    }
    if(num!=3){
        cerr << "smash error: kill: invalid arguments" << endl;
        this->exe = false;
    }
}

void KillCommand::execute() {
    if(this->exe == false){
        return;
    }
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

bool isComplex(const char* cmd_line){
    std::string temp = cmd_line;
    for(int i = 0; i<(int)temp.size(); i++){
        if(temp[i] == '?' or temp[i] == '*'){
            return true;
        }
    }
    return false;
}

ExternalCommand::ExternalCommand(const char *cmd_line, SmallShell* smash, bool timeout) : Command(cmd_line){
    this->isback = _isBackgroundComamnd(cmd_line);
    this->smash = smash;

    char* cmd = strdup(cmd_line);

    _removeBackgroundSign(cmd);//lose &
    string cmd_s = _trim(string(cmd));

    if(timeout){
        for(int i = 0; i < 2; ++i)
            cmd_s = cmd_s.substr(cmd_s.find_first_of(" \t")+1);
    }

    this->args = new char*[20];
    _parseCommandLine(cmd_s.c_str(), this->args);//without the &

    this->iscomplex = isComplex(cmd_s.c_str());

    if(this->iscomplex){
        this->command = strdup(cmd_s.c_str());
    }
    else{
        this->command = strdup(cmd_s.substr(0, cmd_s.find_first_of(" \t")).c_str());
    }
}

void ExternalCommand::execute() {
    int num;

    if(this->iscomplex){//complex
        if(this->isback){//back
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                num = execlp("/bin/bash", "/bin/bash", "-c",
                             this->command, nullptr);
                if(num==-1)
                    perror("smash error: execlp failed");
                exit(0);
            }
            else{//father
                this->smash->add_job(this->getCmdLine(), pid, -1);
                this->smash->set_top_timeout_pid(pid);
                return;
            }
        }
        else{//front
            pid_t pid = fork();
            if(pid==0){//son
                setpgrp();
                cout << strdup(this->command) << endl;

                num = execlp("/bin/bash", "/bin/bash", "-c",
                             strdup(this->command), nullptr);
                if(num==-1)
                    perror("smash error: execlp failed");
                exit(0);
            }
            else{//father
                this->smash->set_top_timeout_pid(pid);

                this->smash->set_foreground_job_pid(pid);
                this->smash->set_foreground_job_cmd(this->getCmdLine());
                waitpid(pid, NULL, WUNTRACED);
                this->smash->set_foreground_job_pid(-1);
                this->smash->set_foreground_job_cmd("");
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
                exit(0);
            }
            else{//father
                this->smash->add_job(this->getCmdLine(), pid, -1);

                this->smash->set_top_timeout_pid(pid);
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
                exit(0);
            }
            else{//father
                this->smash->set_top_timeout_pid(pid);

                this->smash->set_foreground_job_pid(pid);
                this->smash->set_foreground_job_cmd(this->getCmdLine());
                waitpid(pid, NULL, WUNTRACED);
                this->smash->set_foreground_job_pid(-1);
                this->smash->set_foreground_job_cmd("");
                return;
            }
        }
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line){
    this->std_out = -1;
    this->exe = 1;
    this->initial_cmd_line = cmd_line;

    this->handle_IO_Command_Before(cmd_line);
}


int RedirectionCommand::handle1_2(const char *cmd_line, int cmd_num) {
    this->std_out =  dup(1);
    if(this->std_out == -1) {
        perror("smash error: dup failed");
        this->exe = 0;
        return -1;
    }

    //add space before > or >> anyway
    std::string str = cmd_line;
    int index = -1;
    std::string cmd_line_changed;
    if(cmd_num==1) {
        index = str.find(">");
        cmd_line_changed = str.substr(0, index) + " > " + str.substr(index+1, str.size()-index-1);
    }
    if(cmd_num==2) {
        index = str.find(">>");
        cmd_line_changed = str.substr(0, index) + " >> " + str.substr(index+2, str.size()-index-2);
    }


    char* args[20];
    char* cmd = strdup(cmd_line_changed.c_str());
    _removeBackgroundSign(cmd);
    int num = _parseCommandLine(cmd, args);
    index = -1;
    for(int i = 0; i<num; i++){
        str = args[i];

        if(cmd_num == 1 && str.compare(">")==0){
            index = i + 1;
        }

        if(cmd_num == 2 && str.compare(">>")==0){
            index = i + 1;
        }
    }
    int err = close(1);
    if(err == -1) {
        perror("smash error: close failed");
        this->exe = -1;
        return -1;
    }
    const char* temp = args[index];
    if(cmd_num == 1) {
        err = open(temp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if(err == -1) {
            perror("smash error: open failed");
            this->exe = -2;
            return -1;
        }
    }
    if(cmd_num == 2) {
        err = open(temp, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if(err == -1) {
            perror("smash error: open failed");
            this->exe = -2;
            return -1;
        }
    }
    return 1;
}

char *RedirectionCommand::handle_IO_Built_in_Simple(char *final_cmd, int cmd_num) {
    if(!isComplex(final_cmd)){
        std::string temp = final_cmd;
        if(cmd_num == 1)
            temp = temp.substr(0, temp.find_first_of('>'));
        if(cmd_num == 2)
            temp = temp.substr(0, temp.find_first_of(">>"));
        final_cmd = strdup(temp.c_str());
    }
    return final_cmd;
}

void RedirectionCommand::handle_IO_Command_Before(const char *cmd_line) {
    char* final_cmd = strdup(cmd_line);

    if(is_IO_Pipe(final_cmd) > 0){
        _removeBackgroundSign(final_cmd);
    }

    if(is_IO_Pipe(final_cmd)==1){
        int err = this->handle1_2(final_cmd, 1);
        if(err == -1){
            return;
        }
        //change cmd_line for external simple
        //has to come after handle cause handle needs the file to change stdout into
        final_cmd = this->handle_IO_Built_in_Simple(final_cmd, 1);
    }

    if(is_IO_Pipe(final_cmd)==2){
        int err = this->handle1_2(final_cmd, 2);
        if(err == -1){
            return;
        }
        //change cmd_line for external simple.
        //has to come after handle cause handle needs the file to change stdout into
        final_cmd = this->handle_IO_Built_in_Simple(final_cmd, 2);
    }

    this->setCmdLine(final_cmd);
}

int RedirectionCommand::handle_IO_Command_After(const char *cmd_line) {
    if(is_IO_Pipe(cmd_line)==1 or is_IO_Pipe(cmd_line)==2){
        int err;
        if(this->exe == 0){
            return 1;
        }
        if(this->exe == 1){
            err = close(1);
            if(err == -1) {
                perror("smash error: close failed");
                return -1;
            }
            err = dup2(this->std_out, 1);
            if(err == -1) {
                perror("smash error: dup2 failed");
                return -1;
            }
            err = close(this->std_out);
            if(err == -1) {
                perror("smash error: close failed");
                return -1;
            }
            return 1;
        }
        if(this->exe == -1){
            err = close(this->std_out);
            if(err == -1) {
                perror("smash error: close failed");
                return -1;
            }
            return 1;
        }
        if(this->exe == -2){
            err = dup2(this->std_out, 1);
            if(err == -1) {
                perror("smash error: dup2 failed");
                return -1;
            }
            err = close(this->std_out);
            if(err == -1) {
                perror("smash error: close failed");
                return -1;
            }
            return 1;
        }
    }
    return 1;
}

void RedirectionCommand::execute() {
    if(this->exe == 1) {
        SmallShell &smash = SmallShell::getInstance();
        smash.executeCommand(this->getCmdLine().c_str(), false);
    }
    int err = handle_IO_Command_After(this->initial_cmd_line);
    if(err == -1){
        return;
    }
}

bool PipeCommand::is3() {
    return is_IO_Pipe(this->initial_cmd_line) == 3;
}

bool PipeCommand::is4() {
    return is_IO_Pipe(this->initial_cmd_line) == 4;
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line){
    this->exe = true;
    this->initial_cmd_line = cmd_line;
    this->std_out = -1;
    this->std_err = -1;

    char* final_cmd = strdup(cmd_line);
    _removeBackgroundSign(final_cmd);
    std::string temp = final_cmd;
    if(this->is3()) {
        this->first_cmd = temp.substr(0, temp.find_first_of('|'));
        this->second_cmd = temp.substr(temp.find_first_of('|')+1, temp.size()-temp.find_first_of('|')-1);
    }
    if(this->is4()){
        this->first_cmd = temp.substr(0, temp.find_first_of("|&"));
        this->second_cmd = temp.substr(temp.find_first_of("|&")+2, temp.size()-temp.find_first_of("|&")-2);
    }
}

void PipeCommand::execute(){
    if(!this->exe) {
        return;
    }
    SmallShell &smash = SmallShell::getInstance();
    int fd[2];
    pipe(fd);

    pid_t pid;

    if(this->is3()) {
        if ((pid = fork()) == 0) {
            //child
            setpgrp();
            dup2(fd[0], 0);
            close(fd[0]);
            close(fd[1]);
            smash.executeCommand(this->second_cmd.c_str(), true);
            exit(0);
        }
        else{
            //father
            this->std_out = dup(1);
            dup2(fd[1], 1);
            close(fd[0]);
            close(fd[1]);
            smash.executeCommand(this->first_cmd.c_str(), false);
        }
    }

    if(this->is4()) {
        if ((pid = fork()) == 0) {
            //child
            setpgrp();
            dup2(fd[0], 0);
            close(fd[0]);
            close(fd[1]);
            smash.executeCommand(this->second_cmd.c_str(), true);
            exit(0);
        }
        else{
            //father
            this->std_err = dup(2);
            dup2(fd[1], 2);
            close(fd[0]);
            close(fd[1]);
            smash.executeCommand(this->first_cmd.c_str(), false);
        }
    }

    if(this->std_out != -1){
        dup2(this->std_out, 1);
        close(this->std_out);
    }
    if(this->std_err != -1) {
        dup2(this->std_err, 2);
        close(this->std_err);
    }

    waitpid(pid, NULL, WUNTRACED);
}

SetcoreCommand::SetcoreCommand(const char* cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->list = jobs;
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num >= 3){
        try {
            this->job_id = stoi(args[1]);
        }
        catch (invalid_argument& e){
            cerr << "smash error: setcore: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        if(!jobs->exsits(this->job_id)){
            cerr << "smash error: setcore: job-id " << std::to_string(this->job_id) <<" does not exist" << endl;
            this->exe = false;
            return;
        }
        try {
            this->core_num = stoi(args[2]);
        }
        catch (invalid_argument& e){
            cerr << "smash error: setcore: invalid core number" << endl;
            this->exe = false;
            return;
        }
        if(this->core_num < 0 || this->core_num >= get_nprocs_conf()){
            cerr << "smash error: setcore: invalid core number" << endl;
            this->exe = false;
            return;
        }
    }
    if(num!=3){
        cerr << "smash error: setcore: invalid arguments" << endl;
        this->exe = false;
    }
}

void SetcoreCommand::execute() {
    if(this->exe == false){
        return;
    }
    JobEntry* job = this->list->getJobById(this->job_id);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_num, &cpuset);
    if(sched_setaffinity(job->pid, sizeof(cpu_set_t), &cpuset) == -1){
        perror("smash error: sched_setaffinity failed");
        return;
    }
}

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line) : BuiltInCommand(cmd_line){
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 2){
        path_to_file = args[1];
    }
    else{
        cerr << "smash error: getfiletype: invalid arguments" << endl;
        this->exe = false;
    }
}

void GetFileTypeCommand::execute() {
    if(this->exe == false){
        return;
    }

    struct stat buf;
    std::string file_type;

    if (lstat(path_to_file, &buf) == -1) {
        perror("smash error: stat failed");;
        return;
    }

    switch (buf.st_mode & S_IFMT) {
        case S_IFDIR:
            file_type = "directory";
            break;
        case S_IFCHR:
            file_type = "character device";
            break;
        case S_IFBLK:
            file_type = "block device";
            break;
        case S_IFIFO:
            file_type = "FIFO";;
            break;
        case S_IFLNK:
            file_type = "symbolic link";
            break;
        case S_IFSOCK:
            file_type = "socket";
            break;
        case S_IFREG:
            file_type = "regular file";
            break;
        default:
            printf("unknown\n");
            break;
    }

    int file_size = buf.st_size;

    cout << path_to_file << "'s type is \"" << file_type << "\" and take up "
        << file_size << " bytes" << endl;
}

ChmodCommand::ChmodCommand(const char *cmd_line) : BuiltInCommand(cmd_line){
    this->exe = true;
    char* args[20];
    char* cmd = strdup(cmd_line);
    _removeBackgroundSign(cmd);//lose &
    int num = _parseCommandLine(cmd, args);
    if(num == 3){
        try {
            mode = stoi(args[1],0 ,8);
        }
        catch (invalid_argument& e){
            cerr << "smash error: chmod: invalid arguments" << endl;
            this->exe = false;
            return;
        }
        path_to_file = args[2];
    }
    else{
        cerr << "smash error: chmod: invalid arguments" << endl;
        this->exe = false;
    }
}

void ChmodCommand::execute() {
    if(this->exe == false){
        return;
    }

    if(chmod(path_to_file, mode) == -1){
        perror("smash error: chmod failed");
        return;
    }
}

timeoutEntriesList::timeoutEntriesList() {
    this->list = std::vector<timeoutEntry>();
}

const bool timeoutEntriesList::isEmpty() const{
    return this->list.size() == 0;
}


void timeoutEntriesList::addTimeoutEntry(pid_t pid, int duration, const char* command){
    timeoutEntry temp(pid, duration, command);

    this->list.push_back(temp);
    std::sort(this->list.begin(), this->list.end());
}

void timeoutEntriesList::setTopTimeoutPid(pid_t pid){
    for(unsigned int i = 0; i < this->list.size(); ++i) {
        if (this->list[i].pid == 0) {
            this->list[i].pid = pid;
            return;
        }
    }
}

const char* timeoutEntriesList::getTopTimeoutCommand() const{
    return this->list[0].command;
}

const time_t timeoutEntriesList::getTopTimeoutInTime() const{
    return this->list[0].in_time;
}

const pid_t timeoutEntriesList::getTopTimeoutPID() const{
    return this->list[0].pid;
}

void timeoutEntriesList::removeTopTimeout(){
    this->list.erase(this->list.begin());
}

TimeoutCommand::TimeoutCommand(const char *cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line) {
    this->smash = smash;

    char* args[20];
    char* cmd = strdup(cmd_line);
    _parseCommandLine(cmd, args);
    duration = stoi(args[1]);

    this->command = strdup(cmd_line);

}

void TimeoutCommand::execute() {
    //pid will be defined later on in the external command fork
    smash->add_timeout(this->duration, this->command);

    smash->setAlarm();

    smash->executeCommand(this->command, false, true);
}

void SmallShell::add_job(std::string cmd, pid_t pid, int job_id, bool isStopped) {
    this->killFinishedJobs();
    this->jobs_list->addJob(cmd, pid, job_id,isStopped);
}
void SmallShell::setMsg(const std::string msgg) {
    this->msg = msgg;
}

const std::string SmallShell::getMsg() {
    return this->msg;
}

const std::string SmallShell::getLastDir() {
    return this->last_dir;
}

int SmallShell::get_pid() {
    return this->pid;
}

void SmallShell::set_foreground_job_pid(pid_t pid) {
    this->foreground_job_pid = pid;
}

pid_t SmallShell::get_foreground_job_pid() {
    return this->foreground_job_pid;
}

void SmallShell::set_foreground_job_cmd(std::string cmd) {
    this->foreground_job_cmd = cmd;
}

std::string SmallShell::get_foreground_job_cmd() {
    return this->foreground_job_cmd;
}

int SmallShell::get_foreground_job_id() {
    return this->foregroud_job_id;
}

void SmallShell::set_foreground_job_id(int job_id) {
    this->foregroud_job_id = job_id;
}

void SmallShell::killFinishedJobs() {
    this->jobs_list->removeFinishedJobs();
}

void SmallShell::setLastDir(const std::string last_dirr) {
    this->last_dir = last_dirr;
}

void SmallShell::add_timeout(int duration, const char* command){
    //pid is 0 but will be updated later
    this->timeout_list.addTimeoutEntry(0, duration, command);
}

void SmallShell::set_top_timeout_pid(pid_t pid){
    if(!this->timeout_list.isEmpty())
        this->timeout_list.setTopTimeoutPid(pid);
}

const char* SmallShell::get_top_timeout_command() const{
    return this->timeout_list.getTopTimeoutCommand();
}

void SmallShell::remove_top_timeout(){
    this->timeout_list.removeTopTimeout();
}

void SmallShell::setAlarm(){
    if(this->timeout_list.isEmpty())
        return;

    time_t temp = time(nullptr);
    if(temp == ((time_t) -1)){
        perror("smash error: time failed");
    }

    int duration = this->timeout_list.getTopTimeoutInTime() - temp;
    alarm(duration);
}

void SmallShell::handleAlarm(){
    cout << "smash: got an alarm" << endl;

    this->killFinishedJobs();

    if(kill(this->timeout_list.getTopTimeoutPID(), 0) == 0) { //if pid still exists
        //Print the kill message
        cout << "smash: " << this->get_top_timeout_command() << " timed out!" << endl;

        //Kill the relevant process
        int err = kill(this->timeout_list.getTopTimeoutPID(), SIGKILL);
        if (err == -1) {
            perror("smash error: kill failed");
            return;
        }
    }

    //Remove the top timeout command
    this->remove_top_timeout();

    //Signal a new alarm
    this->setAlarm();
}


SmallShell::SmallShell() {
// TODO: add your implementation
    this->msg = "smash";
    this->last_dir = "";
    this->jobs_list = new JobsList();
    this->foreground_job_pid = -1;
    this->foregroud_job_id = -1;
    this->foreground_job_cmd = "";
    this->pid = getpid();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line, bool timeout) {
    // For example:
    char* cmd = strdup(cmd_line);

    //special case
    std::string temp = cmd;
    if(temp.compare("")==0){
        return nullptr;
    }

    _removeBackgroundSign(cmd);//lose &
    string cmd_s = _trim(string(cmd));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));

    //special commands
    if(is_IO(cmd_line)){
        return new RedirectionCommand(cmd_line);
    }

    if(is_PIPE(cmd_line)){
        return new PipeCommand(cmd_line);
    }


    //built-in commands

    if (firstWord.compare("chprompt") == 0) {
        return new chpromptCommand(cmd_line);
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
    if (firstWord.compare("setcore") == 0) {
        return new SetcoreCommand(cmd_line, this->jobs_list);
    }
    if (firstWord.compare("getfiletype") == 0) {
        return new GetFileTypeCommand(cmd_line);
    }
    if (firstWord.compare("chmod") == 0) {
        return new ChmodCommand(cmd_line);
    }
    if (firstWord.compare("timeout") == 0 && !timeout) {
        return new TimeoutCommand(cmd_line, this);
    }

    //external commands
    return new ExternalCommand(cmd_line, this, timeout);
}



void SmallShell::executeCommand(const char *cmd_line, bool is_pipe_second_cmd, bool timeout) {
    Command* cmd = CreateCommand(cmd_line, timeout);

    if(cmd!= nullptr) {
        cmd->execute();
        delete cmd;
    }
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
}