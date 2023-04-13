#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
 std::string cmd_line;
 public:
  Command(const string cmd_line){
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
  BuiltInCommand(const string cmd_line);
  virtual ~BuiltInCommand() {}
};

class PipeCommand : public Command {
  // TODO: Add your data members
  int std_in;
  int std_out;
  int std_err;
  const string initial_cmd_line;
  std::string first_cmd;
    std::string second_cmd;
  bool exe;
 public:
  PipeCommand(const string cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
  bool is3();
  bool is4();
  void cleanup();
  void handle_bi_bi(int fd[2]);
  void handle_ext_bi(int fd[2]);
  void handle_ext_ext(int fd[2]);
  void handle_bi_ext(int fd[2]);
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 int std_out;
 const string initial_cmd_line;
 bool exe;
 public:
  explicit RedirectionCommand(const string cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  int handle1_2(const string cmd_line, int cmd_num);
  int handle_IO_Command_Before(const string cmd_line);
  string handle_IO_Built_in_Simple(string final_cmd, int cmd_num);
  int handle_IO_Command_After(const string final_cmd);
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const string cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const string cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
JobsList* list;
bool iskill;
public:
  QuitCommand(const string cmd_line, JobsList* jobs);
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
  ~JobsList() = default;
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

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const string cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  ChmodCommand(const string cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const string cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand(const string cmd_line);
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
  KillCommand(const string cmd_line, JobsList* jobs);
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
  Command *CreateCommand(const string cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const string cmd_line);
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
  int handle3_4(const string cmd_line, int* std_in, int* std_out, int cmd_num);
};

class chpromptCommand : public BuiltInCommand {
    std::string msg;
    SmallShell* smash;
public:
    chpromptCommand(const string cmd_line, SmallShell* smash);
    ~chpromptCommand() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    SmallShell* smash;
    std::string last_dir;
    std::string curr_dir;
    bool change;
public:
    ChangeDirCommand(const string cmd_line, SmallShell* smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    JobsCommand(const string cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
    const string command;
    string* args;
    bool isback;
    bool iscomplex;
    SmallShell* smash;
public:
    ExternalCommand(const string cmd_line, SmallShell* smash);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* list;
    int job_id;
    bool exe;
public:
    ForegroundCommand(const string cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};


class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* list;
    int job_id;
    bool exe;
public:
    BackgroundCommand(const string cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

#endif //SMASH_COMMAND_H_
