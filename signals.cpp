#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	cout << "smash: got ctrl-Z" << endl;
	SmallShell& smash = SmallShell::getInstance();
	if (!(smash.currentJob == nullptr)) {
		if (killpg(smash.currentJob->process_id, SIGSTOP) == -1) {
			perror("smash error: kill error");
		}
		cout << "smash: process " << smash.currentJob->process_id << " was stopped" << endl;
		smash.currentJob->stoped = true;
		smash.job.addJob(smash.currentJob);
		smash.currentJob = nullptr;
	}
}

void ctrlCHandler(int sig_num) {
	cout << "smash: got ctrl-C" << endl;
	SmallShell& smash = SmallShell::getInstance();
	if (!(smash.currentJob == nullptr)) {
		if (killpg(smash.currentJob->process_id, SIGKILL) == -1) {
			perror("smash error: kill error");
		}
		cout << "smash: process " << smash.currentJob->process_id << " was killed" << endl;
		smash.currentJob = nullptr;
	}
}

void alarmHandler(int sig_num) {
	cout << "smash: got an alarm" << endl;
	SmallShell& smash = SmallShell::getInstance();
	if (!(smash.currentJob == nullptr)) {
		if (killpg(smash.currentJob->process_id, SIGKILL) == -1) {
			perror("smash error: kill error");
		}
		cout << "smash: " << smash.currentJob->command << " timed out!" << endl;
	}
	else
	{
		cout << "smash: " << smash.job.getLastJob()->command << " timed out!" << endl;
	}
}
