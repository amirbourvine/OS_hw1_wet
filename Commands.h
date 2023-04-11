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
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  public:
   int job_id;
   std::string cmd_line;
   pid_t pid;
   time_t in_time;
   bool stopped;

   std::string toString(){
       std::string str = "";
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
  };
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
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
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
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
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
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  void setMsg(std::string msg);
  const std::string getMsg();
  void setLastDir(std::string last_dir);
  const std::string getLastDir();
  void add_job(Command* cmd, pid_t pid, bool isStopped = false);
  void killFinishedJobs();
};

class chpromptCommand : public BuiltInCommand {
    std::string msg;
    SmallShell* smash;
public:
    chpromptCommand(const char* cmd_line, SmallShell* smash);
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
    ExternalCommand(const char* cmd_line, SmallShell* smash);
    virtual ~ExternalCommand() {}
    void execute() override;
};

#endif //SMASH_COMMAND_H_
