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
    //puts the command then the parameters in args, and returns the number of thins it put in args
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
    int num = _parseCommandLine(cmd_line, args);
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
    char* args[20];
    int num = _parseCommandLine(cmd_line, args);
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
                cout << "HERE1" << endl;
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
    cout << "HERE2" << endl;
    if(this->change) {
        this->smash->setLastDir(this->last_dir);
        cout << "HERE3" << endl;
        if (chdir((this->curr_dir).c_str()) == -1) {
            perror("smash error: chdir failed");
        }
    }
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

void SmallShell::setLastDir(const std::string last_dir) {
    cout<<"HERE7" << endl;
    cout << "LAST DIR: "<< this->last_dir <<endl;
    cout<<"HERE8" << endl;
    this->last_dir = last_dir;
    cout<<"HERE9" << endl;
}

SmallShell::SmallShell() {
// TODO: add your implementation
    this->msg = "smash";
    this->last_dir = "";
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

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
  return nullptr;
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