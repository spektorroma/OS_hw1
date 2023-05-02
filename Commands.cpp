#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h> 
#include <fstream>
#include <time.h>
#include <utime.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

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

vector<string> _parseCommandLine(const char* cmd_line) {
  FUNC_ENTRY()
  vector<string> args;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
	  args.push_back(s);
  }

  return args;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

string Command::getRawCommand() {
	return rawCommand;
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


bool isInteger(const string& s)
{
	if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))){
		return false;
	}

	char* p;
	strtol(s.c_str(), &p, 10);

	return (*p == 0);
}

// TODO: Add your implementation for classes in Commands.h 



// Command
Command::Command(const char* cmd_line) : args(_parseCommandLine(cmd_line)), isBackground(_isBackgroundComamnd(cmd_line)), setgrp(true) {


	rawCommand = string(cmd_line);
	auto argss = getArgs();
	char* copy_cmd = new char[80];

	std::string str(cmd_line);
	std::size_t found = str.find("timeout");
	if (found != std::string::npos)
	{
		std::size_t found = str.find(argss[1]);
		str.replace(0, found + 2, "");
	}
	strcpy(copy_cmd, str.c_str());

	_removeBackgroundSign(copy_cmd);
	_trim(copy_cmd);

	noBackCommand = string(copy_cmd);

	delete[] copy_cmd;
	closeChannel = -1;
};
vector<string> Command::getArgs() { return args; }

// BuiltInCommand
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {};


/*
Builtin commands
*/

// ChangePromptCommand
ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void ChangePromptCommand::execute()
{
	SmallShell& smash = SmallShell::getInstance();

	auto args = getArgs();

	if (args.size() == 1)
	{
		smash.prompt = "smash";
	}
	else
	{
		smash.prompt = args[1];
	}
}

// GetCurrDirCommand
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void GetCurrDirCommand::execute()
{
	char* dirNameCStr = get_current_dir_name();
	string dirName(dirNameCStr);
	free(dirNameCStr);

	cout << dirName << endl;
}

// ShowPidCommand
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void ShowPidCommand::execute()
{
	pid_t x = getpid();
	cout << "smash pid is " << x << "\n";
}

// ChangeDirCommand
ChangeDirCommand::ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void ChangeDirCommand::execute()
{
	SmallShell& smash = SmallShell::getInstance();

	auto args = getArgs();

	if (args.size() > 2) {
		echoError("cd", "too many arguments");
		return;
	}

	string change_dir = args[1];
	char* cwd = get_current_dir_name();
	string current_dir(cwd);
	free(cwd);

	if (args[1].compare("-") == 0) {
		if (smash.last_dir == 0) {
			echoError("cd", "OLDPWD not set");
			return;
		}
		change_dir = smash.list_last_dir[0];
		smash.list_last_dir.insert(smash.list_last_dir.begin(), current_dir);
	}
	else {
		smash.list_last_dir.insert(smash.list_last_dir.begin(), current_dir);
		smash.last_dir++;
	}

	if (chdir(change_dir.c_str()) != 0)
	{
		echoSysError("chdir failed");
		if (smash.last_dir != 0) {
			smash.list_last_dir.erase(smash.list_last_dir.begin());
		}
		return;
	}
}

JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void JobsCommand::execute()
{
	SmallShell& smash = SmallShell::getInstance();
	smash.job.removeFinishedJobs();
	smash.job.printJobsList();
}




KillCommand::KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void KillCommand::execute()
{
	SmallShell& smash = SmallShell::getInstance();

	auto args = getArgs();

	if (args.size() != 3) {
		echoError("kill", "invalid arguments");
		return;
	}

	if (!isInteger(args[1]) || !isInteger(args[2])) {
		echoError("kill", "invalid arguments");
		return;
	}

	std::string command = args[1];

	JobsList::JobEntry* job;

	if (args.size() < 2) {
		echoError("kill", "invalid arguments");
		return;
	}

	smash.job.removeFinishedJobs();
	job = smash.job.getJobById(stoi(args[2]));

	if (command[0] != '-' || args.size() > 3) {
		echoError("kill", "invalid arguments");
		return;
	}

	if (job == nullptr) {
		echoError("kill", "job-id " + args[2] + " does not exist");
		return;
	}

	int sigNum = atoi(command.substr(1).c_str());
	job = smash.job.getJobById(std::stoi(args[2]));
	pid_t sigPid = job->process_id;

	if (kill(sigPid, sigNum) != 0) {
		echoError("kill", "invalid arguments");
		return;
	}
	cout << "signal number " << sigNum << " was sent to pid " << sigPid << endl;
}


ForegroundCommand::ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void ForegroundCommand::execute() {

	SmallShell& smash = SmallShell::getInstance();
	smash.job.removeFinishedJobs();

	auto args = getArgs();

	if (smash.job.job_list.empty() && args.size() == 1) {
		echoError("fg", "jobs list is empty");
		return;
	}
	JobsList::JobEntry* job;
	if (args.size() == 1) {
		job = smash.job.getLastJob();
	}

	if (args.size() > 1) {
		if (args.size() > 2 || (!isInteger(args[1]) && args.size() > 1)) {
			echoError("fg", "invalid arguments");
			return;
		}
		std::string command = args[1];

		int jobId = atoi(args[1].c_str());
		job = smash.job.getJobById(jobId);


		if (job == nullptr) {
			echoError("fg", "job-id " + args[1] + " does not exist");
			return;
		}
	}
	if (job == nullptr) {
		echoError("fg", "job-id " + args[1] + " does not exist");
		return;
	}


	cout << job->command + " : " << job->process_id << endl;
	int pid = job->process_id;
	if (killpg(pid, SIGCONT) == -1) {

	}

	job->stoped = false;
	smash.setCurrentJob(job);
	smash.job.removeJobById(pid);
	int s = 0;
	waitpid(pid, &s, WUNTRACED);

}

BackgroundCommand::BackgroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void BackgroundCommand::execute() {
	SmallShell& smash = SmallShell::getInstance();
	smash.job.removeFinishedJobs();
	auto args = getArgs();

	if (smash.job.job_list.empty() && args.size() == 1) {
		echoError("bg", "there is no stopped jobs to resume");
		return;
	}

	JobsList::JobEntry* job;
	bool no_arg = (args.size() == 1);

	if (no_arg == true) {
		job = smash.job.getLastStoppedJob();
	}
	if (args.size() > 1) {
		if (args.size() > 2 || (!isInteger(args[1]) && args.size() > 1)) {
			echoError("bg", "invalid arguments");
			return;
		}
		std::string command = args[1];

		int jobId = atoi(args[1].c_str());
		job = smash.job.getJobById(jobId);

		if (job == nullptr) {
			echoError("bg", "job-id " + args[1] + " does not exist");
			return;
		}
	}

	if (!job->stoped && !no_arg) {
		echoError("bg", "job-id " + args[1] + " is already running in the background");
		return;
	}

	if (!job->stoped && no_arg) {
		echoError("bg", "there is no stopped jobs to resume");
		return;
	}
	cout << job->command + " : " << job->process_id << endl;
	pid_t pid = job->process_id;
	if (killpg(pid, SIGCONT) == -1) {
		echoSysError("kill failed");
		return;
	}
	job->stoped = false;

}

QuitCommand::QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
void QuitCommand::execute()
{
	SmallShell& smash = SmallShell::getInstance();

	auto args = getArgs();

	smash.setCurrentJob(nullptr);
	smash.job.removeFinishedJobs();
	if (args.size() > 1 && args[1] == "kill") {
		int numOfJobs = smash.job.job_list.size();
		cout << "smash: sending SIGKILL signal to " + to_string(numOfJobs) + " jobs:" << endl;
		smash.job.killAllJobs();
	}
	delete this;
	exit(0);
}




void Command::echo(string command, string msg) const {
	cerr << "smash error: " << command << ": " << msg << endl;
}

void Command::echoError(string command, string msg) const {
	cerr << "smash error: " + command + ": " + msg << endl;
}

void Command::echoSysError(string msg) const {
	perror(("smash error: " + msg).c_str());
}


/*
Class Jobs
*/

void JobsList::addJob(Command* cmd, bool isStopped, pid_t pid, int job_id) {
	string command = cmd->getRawCommand();
	time_t currTime = time(nullptr);
	JobsList::JobEntry newEntry = JobsList::JobEntry(pid, command, currTime,
		isStopped, job_id);
	this->job_list.emplace_back(newEntry);
}

JobsList::JobsList() {};
void JobsList::printJobsList()
{
	for (size_t i = 0; i < job_list.size(); i++) {
		JobEntry* job = &job_list[(int)i];
		if (job->stoped)
			cout << "[" << (job->job_id) << "] " << job->command << " : " << job->process_id << " " << difftime(time(NULL), job->time) << " secs" << " (stopped)" << endl;
		else
			cout << "[" << (job->job_id) << "] " << job->command << " : " << job->process_id << " " << difftime(time(NULL), job->time) << " secs" << endl;
	}
}

void JobsList::addJob(JobsList::JobEntry* job) {
	job->time = time(nullptr);
	if (job_list.size() == 0) {
		job_list.push_back(*job);
		return;
	}
	for (size_t i = 0; i < job_list.size(); i++) {
		if (job_list[(int)i].process_id > job->process_id) {
			job_list.insert(job_list.begin() + i, *job);
			return;
		}
		if ((i + 1) == job_list.size()) {
			job_list.insert(job_list.begin() + i + 1, *job);
			return;
		}
	}
}

void JobsList::removeFinishedJobs() {
	SmallShell& smash = SmallShell::getInstance();
	size_t end_size = job_list.size();
	for (size_t i = 0; i < end_size; i++) {
		JobEntry* job = &job_list[(int)i];
		int status;
		if (waitpid(job->process_id, &status, WNOHANG) == job->process_id) {
			smash.job.removeJobById(job->process_id);
			i = i - 1;
			end_size = end_size - 1;
		}

	}
}

JobsList::JobEntry* JobsList::getLastJob() {
	if (job_list.size() == 0) {
		return nullptr;
	}
	return &job_list[job_list.size() - 1];
}

JobsList::JobEntry* JobsList::getLastStoppedJob() {
	int i = job_list.size() - 1;
	JobEntry* job = &job_list[i];
	bool loop = true;
	while (loop) {
		job = &job_list[i];
		if (job->stoped == true) {
			loop = false;
		}
		i--;
		if (i == (int)(job_list.size() - 1)) {
			loop = false;
		}
	}
	return job;
}

JobsList::JobEntry* JobsList::getJobById(int id) {
	JobsList::JobEntry* return_job = nullptr;
	for (size_t i = 0; i < job_list.size(); i++)
	{
		JobsList::JobEntry* job;
		job = &job_list[i];
		if (job->job_id == id) {
			return_job = &job_list[i];
		}
	}
	return return_job;
}

void JobsList::removeJobById(int id) {
	for (size_t i = 0; i < job_list.size(); i++)
	{
		JobsList::JobEntry* job = &job_list[(int)i];
		if (job->process_id == id) {
			job_list.erase(job_list.begin() + (int)i);
		}
	}
}

void JobsList::killAllJobs() {
	for (size_t i = 0; i < job_list.size(); i++) {
		JobEntry* job = &job_list[(int)i];
		cout << job->process_id << ": " << job->command << endl;
		if (killpg(job->process_id, SIGKILL) == -1) {

		}
	}
}

/*
External Commands
*/

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {};
void ExternalCommand::execute() {
	char* exevArgs[4];
	exevArgs[0] = (char* const)"/bin/bash";
	exevArgs[1] = (char* const)"-c";
	exevArgs[2] = (char* const)noBackCommand.c_str();
	exevArgs[3] = nullptr;
	

	SmallShell& smash = SmallShell::getInstance();
	smash.job.removeFinishedJobs();


	auto args = getArgs();
	pid_t pid = fork();
	if (pid == -1) {
		echoSysError("fork failed");
		return;
	}

	if (pid == 0) {
		if (setgrp) {
			setpgrp();
		}
		execv(exevArgs[0], exevArgs);
	}
	else {
		if (isBackground) {
			JobsList::JobEntry* job;
			job = smash.job.getLastJob();
			if (job != nullptr) {
				smash.job.addJob(this, false, pid, job->job_id + 1);
			}
			else {
				smash.job.addJob(this, false, pid, 1);
			}
		}
		else {
			if (setgrp) {
				JobsList::JobEntry* job;
				JobsList::JobEntry* currJob;
				job = smash.job.getLastJob();
				if (job != nullptr) {
					JobsList::JobEntry copy = JobsList::JobEntry(pid,
						rawCommand, 0, false, job->job_id + 1);
					currJob = &copy;
				}
				else {
					JobsList::JobEntry copy = JobsList::JobEntry(pid,
						rawCommand, 0, false, 1);
					currJob = &copy;
				}
				smash.setCurrentJob(currJob);
			}
			int s = 0;
			if (closeChannel != -1) {
				close(closeChannel);
			}
			waitpid(pid, &s, WUNTRACED);
			smash.setCurrentJob(nullptr);
		}
	}
}






SmallShell::SmallShell() : prompt("smash"), last_dir(0), list_last_dir(vector<string>())
{
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	
	string cmd_s = _trim(string(cmd_line));
	if (cmd_s.size() == 0) {
		return nullptr;
	}
	string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));


	if (firstWord.compare("chprompt") == 0)
	{
		return new ChangePromptCommand(cmd_line);
	}
	else if (firstWord.compare("pwd") == 0)
	{
		return new GetCurrDirCommand(cmd_line);
	}
	else if (firstWord.compare("showpid") == 0)
	{
		return new ShowPidCommand(cmd_line);
	}
	else if (firstWord.compare("cd") == 0)
	{
		return new ChangeDirCommand(cmd_line);
	}
	else if (firstWord.compare("jobs") == 0)
	{
		return new JobsCommand(cmd_line);
	}
	else if (firstWord.compare("kill") == 0)
	{
		return new KillCommand(cmd_line);
	}
	else if (firstWord.compare("fg") == 0)
	{
		return new ForegroundCommand(cmd_line);

	}
	else if (firstWord.compare("bg") == 0)
	{
		return new BackgroundCommand(cmd_line);
	}
	else if (firstWord.compare("quit") == 0)
	{
		return new QuitCommand(cmd_line);
	}
	else
	{
		return new ExternalCommand(cmd_line);
	}

	return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {

	Command* cmd = CreateCommand(cmd_line);
	if (cmd == nullptr) {
		return;
	}
	cmd->execute();
}
