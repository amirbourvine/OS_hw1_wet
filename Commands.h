#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
    std::string cmd_line;
public:
    Command(const char* cmd_line){
        this->cmd_line = cmd_line;
    }
    virtual ~Command(){
    }
    std::string getCmdLine(){
        return this->cmd_line;
    }
    void setCmdLine(std::string str){
        this->cmd_line = str;
    }
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class PipeCommand : public Command {
    // TODO: Add your data members
    const char* initial_cmd_line;
    std::string first_cmd;
    std::string second_cmd;
    bool exe;
    int std_out;
    int std_err;
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
    bool is3();
    bool is4();

};

class RedirectionCommand : public Command {
    // TODO: Add your data members
    int std_out;
    const char* initial_cmd_line;
    bool exe;
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    int handle1_2(const char* cmd_line, int cmd_num);
    int handle_IO_Command_Before(const char* cmd_line);
    char* handle_IO_Built_in_Simple(char* final_cmd, int cmd_num);
    int handle_IO_Command_After(const char* final_cmd);
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
    JobsList* list;
    bool iskill;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobEntry {
    // TODO: Add your data members
public:
    int job_id;
    std::string cmd_line;
    pid_t pid;
    time_t in_time;
    bool stopped;

    std::string toString(bool wo_time = false){
        std::string str = "";
        if(wo_time) {
            str += this->cmd_line;
            str += " : ";
            str += std::to_string(this->pid);
            return str;
        }
        str += "[";
        str += std::to_string(this->job_id);
        str += "] ";
        str += this->cmd_line;
        str += " : ";
        str += std::to_string(this->pid);
        str += " ";
        time_t temp = time(nullptr);
        if(temp == ((time_t) -1)){
            perror("smash error: time failed");
            return "";
        }
        str += std::to_string(int(difftime(temp ,this->in_time)));
        str += " secs";
        if(this->stopped){
            str += " (stopped)";
        }
        return str;
    }

    std::string toString_Kill(){
        std::string str = "";
        str += std::to_string(this->pid);
        str += ": ";
        str += this->cmd_line;
        return str;
    }
};

class JobsList {
public:
    // TODO: Add your data members
    std::vector<JobEntry*> list;
    int max_job_id;
public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry *getLastStoppedJob();
    bool exsits(int jobid);
    int maxJobId();
    // TODO: Add extra methods or modify exisitng ones as needed
};

struct timeoutEntry{
    pid_t pid;
    time_t in_time;

    timeoutEntry(int pid, int duration) : pid(pid){
        time_t temp = time(nullptr);
        if(temp == ((time_t) -1)){
            perror("smash error: time failed");
        }

        in_time = temp + duration;
    }
} typedef timeoutEntry;

class timeoutEntriesList {
public:
    // TODO: Add your data members
    std::vector<timeoutEntry*> list;
public:
    timeoutEntriesList();
    ~timeoutEntriesList();
    void addTimeoutEntry(pid_t pid, int duration);
    void handleAlarm();
};

class ChmodCommand : public BuiltInCommand {
    char* path_to_file;
    mode_t mode;
    bool exe;
public:
    ChmodCommand(const char* cmd_line);
    virtual ~ChmodCommand() {}
    void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
    char* path_to_file;
    bool exe;
public:
    GetFileTypeCommand(const char* cmd_line);
    virtual ~GetFileTypeCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    JobsList* list;
    int job_id;
    int core_num;
    bool exe;
public:
    SetcoreCommand(const char* cmd_line, JobsList *jobs);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* list;
    int job_id;
    int sig_num;
    bool exe;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class SmallShell {
private:
    // TODO: Add your data members
    std::string msg;
    std::string last_dir;
    JobsList* jobs_list;
    pid_t foreground_job_pid;
    Command* foreground_job_cmd;
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line, bool isTimeoutCommand);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line, bool is_pipe_second_cmd = false, bool timeout = false);
    // TODO: add extra methods as needed
    void setMsg(std::string msg);
    const std::string getMsg();
    void setLastDir(std::string last_dir);
    const std::string getLastDir();
    void add_job(Command* cmd, pid_t pid, bool isStopped = false);
    void killFinishedJobs();
    void set_foreground_job_pid(pid_t pid);
    pid_t get_foreground_job_pid();
    void set_foreground_job_cmd(Command* cmd);
    Command* get_foreground_job_cmd();
    int handle3_4(const char* cmd_line, int* std_in, int* std_out, int cmd_num);
};

class chpromptCommand : public BuiltInCommand {
    std::string msg;
public:
    chpromptCommand(const char* cmd_line);
    ~chpromptCommand() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    SmallShell* smash;
    std::string last_dir;
    std::string curr_dir;
    bool change;
public:
    ChangeDirCommand(const char* cmd_line, SmallShell* smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
    const char* command;
    char** args;
    bool isback;
    bool iscomplex;
    SmallShell* smash;
public:
    ExternalCommand(const char* cmd_line, SmallShell* smash, bool timeout);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* list;
    int job_id;
    bool exe;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};


class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* list;
    int job_id;
    bool exe;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};


class TimeoutCommand : public BuiltInCommand {
/* Bonus */
    int duration;
    const char* command;
    SmallShell* smash;

public:
    explicit TimeoutCommand(const char* cmd_line,  SmallShell* smash);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

#endif //SMASH_COMMAND_H_