/*******************************************************************************
c4s_process.hpp
Defines process-class for Cpp4Scripts library.

--------------------------------------------------------------------------------
This file is part of Cpp4Scripts library.

  Cpp4Scripts is free software: you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option) any
  later version.

  Cpp4Scripts is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:
  http://www.gnu.org/licenses/lgpl.html

Copyright (c) Menacon Ltd, Finland
*******************************************************************************/
#ifndef C4S_PROCESS_HPP
#define C4S_PROCESS_HPP

namespace c4s {

    class compiled_file;
    class program_arguments;
    class variables;
#if defined(__linux) || defined(__APPLE__)
    class user;
#endif
    // ----------------------------------------------------------------------------------------------------
    //! Process pipes wraps three pipes needed to communicate with child programs / binaries
    class proc_pipes
    {
    public:
        proc_pipes();
        ~proc_pipes();

        void reset();
#if defined(__linux) || defined(__APPLE__)
        void init_child();
        void init_parent();
#else
        void init(STARTUPINFO *, HANDLE *);
#endif
        void read_child_stdout(ostream *);
        void read_child_stderr(ostream *);
        size_t write_child_input(const path &);
        void write_child_input(const string&);
        void close_child_input();
        size_t get_br_out() { return br_out; }
        size_t get_br_err() { return br_err; }
    protected:
        size_t br_out, br_err;
        bool send_ctrlZ;
#if defined(__linux) || defined(__APPLE__)
        int fd_out[2];
        int fd_err[2];
        int fd_in[2];
        size_t br_in;
#else
        struct winpipe {
            winpipe(bool);
            ~winpipe();
            void read(ostream *);
            bool IsOK() { return (hpipe!=INVALID_HANDLE_VALUE && hchild!=INVALID_HANDLE_VALUE) ? true:false; }
            void close();

            OVERLAPPED overlapped;
            HANDLE hpipe, hchild;
            bool pending;
            char buffer[512];
            static int count;
        };
        winpipe out, err, in;
        DWORD br_in;
#endif
    };

    // ----------------------------------------------------------------------------------------------------
    //! Class encapsulates an executable process.
    /*! Class manages single executable process and its parameters. Process can be executed multiple times and
      its arguments can be changed in between. Process output (both stderr and stdout) is by default echoed to parent's
      stdout. See pipe-functions for alternatives. Class destructor terminates the process if it is still running.
    */
    class process
    {
    public:
        //! Default constructor.
        process() { init_member_vars(); }
        //! Creates a new process object from command and its arguments.
        process(const char*, const char *args);
        //! Creates a new proces, sets arguments and pipe target.
        process(const char*, const char *args, ostream *out);
        //! Creates a new process object from command and its arguments.
        process(const string &, const char *args, ostream *out=0);
        //! Creates a new process object from command and its arguments.
        process(const string &, const string &args);
        //! Initializes the command only.
        process(const string &);
        //! Initializes the command only.
        process(const char*);
        //! Deletes the object and possibly kills the process.
        ~process();

        //! Sets the process to be executed.
        void set_command(const char *);

        //!Operator= override for process.
        void operator=(const process &p);
        //!Operator= override for process.
        void operator=(const char* command) { set_command(command); }

        //! Runs the process with given timeout value and optional arguments.
        int operator() (int timeout, const char *args=0) { return exec(timeout,args); }
        //! Runs the process with default timeout
        int operator() () { return exec(C4S_PROC_TIMEOUT); }
        //! Runs the process and captures the output to given stream.
        int operator() (ostream *out, int to=C4S_PROC_TIMEOUT) { pipe_to(out); return exec(to); }

        //! Sets the given string as single argument string for this process.
        void set_args(const char *arg) { arguments.str(""); arguments<<arg; }
        //! Sets the given string as single argument string for this process.
        void set_args(const string &arg) { arguments.str(""); arguments<<arg; }
        //! Adds the given string into argument list.
        void operator+=(const char* arg) { arguments <<' '<<arg; }
        //! Adds the given string into argument list.
        void operator+=(const string &arg) { arguments<<' '<<arg; }

        //! Given file is fed to child's stdin as the child is started.
        void pipe_from(const path &from) { in_path=from; }
        //! Pipes child stderr & stdout to given stream (file, stringstream or cout)
        void pipe_to(ostream *out) { pipe_target=out; }
        //! Disables piping from child's stderr and stdout all together.
        void pipe_null() { pipe_target=0; }
        //! Enables default piping i.e. stdout
        void pipe_default() { pipe_target = &cout; }
        //! Sends the given string into childs stdin. Process must be running.
        void pipe_send(const string &out) { if(pid && pipes) pipes->write_child_input(out); }
        //! Closes the send pipe to client.
        void pipe_send_close() { if(pid && pipes) pipes->close_child_input(); }
        //! Sets a global file to catch output from all processes.
        static void pipe_global_start(ofstream *po) { if(po) pipe_global=po; }
        //! Stops the global output catching
        static void pipe_global_stop() { pipe_global = 0; }

#if defined(__linux) || defined(__APPLE__)
        //! Sets the effective owner for the process. (Linux only)
        void set_user(user *);
        //! Sets the daemon flag. Use only for attached processes.
        void set_daemon(bool enable) { daemon = enable; }
        //! Returns the pid for this process.
        int get_pid() { return pid; }
        //! Attaches this object to running process.
        void attach(int pid);
#else
        //! Returns the pid for this process.
        HANDLE get_pid() { return pid; }
#endif
        //! Starts the executable process.
        /*! Process runs assynchronously. Command and possible arguments should have been given before this command.
          If process needs to be killed before it runs it's course, you may simply delete the process object. Normally
          one would wait untill it completes. If process is already running, calling this function will call stop
          first and then start new process.
        */
        void start(const char *args=0);
        //! Stops the process i.e. terminates it if it is sill running and closes files.
        void stop();
        //! Waits for this process to end or untill the given timeout is expired.
        int  wait_for_exit(int timeOut);
        //! Executes command after appending given argument to the current arguments.
        int execa(const char *arg, int timeout=C4S_PROC_TIMEOUT);
        //! Executes the command with optional arguments = calls start and waits for the exit.
        int  exec(int timeout, const char *args=0);
        //! Executes the command with optional arguments = calls start and waits for the exit.
        int  exec(int timeout, const string &);

        //! Checks if the process is still running.
        bool is_running();
        //! Returns return value from last execution.
        int  last_return_value() { return last_ret_val; }
        //! Enables or disables command echoing before execution.
        void set_echo(bool e) { echo = e; }

        //! Static function that returns an output from given command.
        static void catch_output(const char *cmd, const char *args, string &output);
        //! Static function that appends parameters from one process to another.
        static void append_from(const char *cmd, const char *args, process &target);
        //! Static function to get current PID
#if defined(__linux) || defined(__APPLE__)
        static pid_t get_running_pid() { return getpid(); }
#endif
        //! Dumps the process name and arguments into given stream. Use for debugging.
        void dump(ostream &);

        static bool no_run;          //1< If true then the command is simply echoed to stdout but not actually run. i.e. dry run.
        static bool nzrv_exception;  //!< If true 'Non-Zero Return Value' causes exception.

    protected:
        //! Initializes process member variables. Called by constructors.
        void init_member_vars();
        void stop_daemon();

        path command;               //!< Full path to a command that should be executed.
        stringstream arguments;     //!< Stream of process arguments. Must not contain variables.

#if defined(__linux) || defined(__APPLE__)
        user  *owner;               //!< If defined, process will be executed with user's credentials.
        pid_t pid;
        int last_ret_val;
        bool daemon;                //!< If true then the process is to be run as daemon and should not be terminated at class dest
#else
        HANDLE pid;
        HANDLE output;
        HANDLE wait_handles[3];
        DWORD  last_ret_val;
#endif
        ostream *pipe_target;       //!< Common pipe target.
        path in_path;               //!< If defined and exists, files content will be used as input to process.
        proc_pipes *pipes;          //!< Pipe to child for input and output. Valid when child is running.
        static ofstream *pipe_global; //!< Global pipe target
        bool echo;                  //!< If true then the commands are echoed to stdout before starting them. Use for debugging.
    };

}
#endif
