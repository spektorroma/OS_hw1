#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include "signals.h"


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class Command {
public:
// TODO: Add your data members
	vector<string> args;
	string noBackCommand;
	string rawCommand;
	bool isBackground;
	int numParams;
	bool setgrp;
	int  closeChannel;
 public:
  Command(const char* cmd_line);
  virtual ~Command() = default;
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  vector<string> getArgs();
  string getRawCommand();
  void echo(string command, string msg) const;
  void echoSysError(string msg) const;
  void echoError(string command, string msg) const;
  void setCloseChannel(int channel) {
	  closeChannel = channel;
  }
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
  virtual void execute() = 0;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
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

class TimeoutCommand : public Command
{
public:
	explicit TimeoutCommand(const char* cmd_line);
	virtual ~TimeoutCommand() {}
	void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  ChangePromptCommand(const char* cmd_line);
  virtual ~ChangePromptCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
// TODO: Add your data members public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
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
public:
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};




class JobsList {
 public:
  class JobEntry {
  public:
	  pid_t process_id;
	  string command;
	  time_t time;
	  bool stoped;
	  int job_id;
	  JobEntry(pid_t PID, string command, time_t time, bool
		  stoped, int job_id) :
		  process_id(PID), command(command), time(time),
		  stoped(stoped), job_id(job_id) {};
	  // TODO: Add your data members
   // TODO: Add your data members
  };
  vector<JobEntry> job_list;
 // TODO: Add your data members
 public:
  JobsList();
  virtual ~JobsList() {};
  void addJob(Command* cmd, int job_id , bool isStopped = false);
  void addJob(Command* cmd, bool isStopped, pid_t pid, int job_id);
  void addJob(JobsList::JobEntry* job);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
  string prompt;
  int last_dir;
  vector<string> list_last_dir;
  JobsList job;
  JobsList::JobEntry* currentJob;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  void setCurrentJob(JobsList::JobEntry* newJob) {
	  delete currentJob;
	  currentJob = nullptr;
	  if (newJob) {
		  currentJob = new JobsList::JobEntry(*newJob);
	  }
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
