#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.get_foreground_job_pid()==-1){
        return;
    }
    else{
        smash.add_job(smash.get_foreground_job_cmd(), smash.get_foreground_job_pid(), true);
        kill(smash.get_foreground_job_pid(),SIGTSTP);
        cout << " smash: process " << std::to_string(smash.get_foreground_job_pid()) << " was stopped" << endl;
        return;
    }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if(smash.get_foreground_job_pid()==-1){
      return;
  }
  else{
      kill(smash.get_foreground_job_pid(),SIGKILL);
      cout << " smash: process " << std::to_string(smash.get_foreground_job_pid()) << " was killed" << endl;
      return;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

