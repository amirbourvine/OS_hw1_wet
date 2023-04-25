#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.get_foreground_job_pid()==-1){
        return;
    }
    else{
        /* test
        cout << "pid: " << smash.get_foreground_job_pid()<<endl;
        cout << (smash.get_foreground_job_cmd()== nullptr) << endl;
        */

        smash.add_job(smash.get_foreground_job_cmd(), smash.get_foreground_job_pid(), true);
        int err = kill(smash.get_foreground_job_pid(),SIGSTOP);
        if(err == -1){
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << std::to_string(smash.get_foreground_job_pid()) << " was stopped" << endl;
        smash.set_foreground_job_cmd("");
        smash.set_foreground_job_pid(-1);
        return;
    }
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if(smash.get_foreground_job_pid()==-1){
      return;
  }
  else{
      int err = kill(smash.get_foreground_job_pid(),SIGKILL);
      if(err == -1){
          perror("smash error: kill failed");
          return;
      }
      cout << "smash: process " << std::to_string(smash.get_foreground_job_pid()) << " was killed" << endl;
      smash.set_foreground_job_cmd("");
      smash.set_foreground_job_pid(-1);
      return;
  }
}

void alarmHandler(int sig_num) {
  SmallShell& smash = SmallShell::getInstance();
  smash.handleAlarm();
}

