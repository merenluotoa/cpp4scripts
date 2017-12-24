/*******************************************************************************
c4s_process.cpp
Implementation for process-class for Cpp4Scripts library.

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
#ifdef C4S_LIB_BUILD
 #include <string.h>
 #include <iostream>
 #if defined(__linux) || defined(__APPLE__)
  #include <sys/wait.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <grp.h>
  #include <fcntl.h>
  #include <stdlib.h>
  #include <signal.h>
 #endif
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_user.hpp"
 #include "c4s_path.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_compiled_file.hpp"
 #include "c4s_variables.hpp"
 #include "c4s_program_arguments.hpp"
 #include "c4s_process.hpp"
 #include "c4s_util.hpp"
 using namespace c4s;
#endif

#ifdef _WIN32
const size_t MAX_ARG_BUFFER = 512;
#endif

// ==================================================================================================
c4s::proc_pipes::proc_pipes()
#ifdef _WIN32
    : out(true), err(true), in(false)
#endif
/*!
  Initializes process pipes by creating three internal pipes.
*/
{
#if defined(__linux) || defined(__APPLE__)
    int fd_tmp[2];
    memset(fd_out,0,sizeof(fd_out));
    memset(fd_err,0,sizeof(fd_err));
    memset(fd_in,0,sizeof(fd_in));

    if(pipe(fd_out))
        throw process_exception("proc_pipes::proc_pipes - Unable to create pipe for the process std output.");
    if(fd_out[0]<3 || fd_out[1]<3) {
//#ifdef _DEBUG
//        cerr << "WARNING - Pipe allocation over standard streams !\n";
//#endif
        if(pipe(fd_tmp))
            throw process_exception("proc_pipes::proc_pipes - Unable to create pipe for the process std output (2).");
        fd_out[0] = fd_tmp[0];
        fd_out[1] = fd_tmp[1];
    }
    if(pipe(fd_err))
        throw process_exception("proc_pipes::proc_pipes - Unable to create pipe for the process std error.");
    if(pipe(fd_in))
        throw process_exception("proc_pipes::proc_pipes - Unable to create pipe for the process std input.");

    // Make out and err read pipes nonblocking
    int fflag = fcntl(fd_out[0], F_GETFL, 0);
    fcntl(fd_out[0], F_SETFL, fflag|O_NONBLOCK);
    fflag = fcntl(fd_err[0], F_GETFL, 0);
    fcntl(fd_err[0], F_SETFL, fflag|O_NONBLOCK);
#else
    // Error handling.
    if(!out.IsOK() || !err.IsOK() || !in.IsOK())
        throw process_exception("proc_pipes::proc_pipes - pipe creation error");
#endif
    br_out = 0;
    br_err = 0;
    br_in  = 0;
    send_ctrlZ = false;
}

// ==================================================================================================
void c4s::proc_pipes::close_child_input()
{
#if defined(__linux) || defined(__APPLE__)
    if(fd_in[0]) {
        close(fd_in[0]);
        fd_in[0] = 0;
    }
    if(fd_in[1]) {
        close(fd_in[1]);
        fd_in[1] = 0;
    }
#else
    if(send_ctrlZ) {
        // cerr << "proc_pipes::close_child_input - Sending ctrl-Z\n";
        char cZ=0x1A;
        DWORD bw;
        WriteFile(in.hpipe,&cZ,1,&bw,NULL);
        send_ctrlZ = false;
    }
    in.close();
#endif
}

// ==================================================================================================
#ifdef _WIN32
int c4s::proc_pipes::winpipe::count = 0;
c4s::proc_pipes::winpipe::winpipe(bool readFlag)
{
    pending = false;
    memset(&overlapped,0,sizeof(overlapped));
    if(readFlag)
        overlapped.hEvent = CreateEvent( NULL, TRUE, TRUE, NULL);

    // Generate pipe
    ostringstream oss;
    oss << "\\\\.\\pipe\\cpp4scripts_"<<GetCurrentProcessId()<<"_"<<count;
    DWORD mode = readFlag ? PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED : PIPE_ACCESS_OUTBOUND;
    hpipe = CreateNamedPipe(oss.str().c_str(),mode,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,1,255,255,500,0);
#ifdef _DEBUG
    if(hpipe == INVALID_HANDLE_VALUE)
        cerr << "DEBUG: proc_pipes::winpipe::winpipe - Create named pipe failed: "<<strerror(GetLastError())<<'\n';
#endif

    // Open client end
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    DWORD cmode = readFlag ? GENERIC_WRITE : GENERIC_READ;
    hchild = CreateFile(oss.str().c_str(),cmode, 0, &sa, OPEN_EXISTING, 0, NULL);
#ifdef _DEBUG
    if(hchild == INVALID_HANDLE_VALUE)
        cerr << "DEBUG: proc_pipes::winpipe::winpipe - Open child pipe failed: "<<strerror(GetLastError())<<'\n';
#endif
    count++;
}

// ==================================================================================================
c4s::proc_pipes::winpipe::~winpipe()
{
    if(hpipe)
        close();
}

// ==================================================================================================
void c4s::proc_pipes::winpipe::close()
{
    if(pending)
#if(_WIN32_WINNT >= 0x0600)
        CancelIoEx(hpipe,&overlapped);
#else
        CancelIo(hpipe);
#endif
    if(overlapped.hEvent)
        CloseHandle(overlapped.hEvent);
    if(hpipe)
        CloseHandle(hpipe);
    if(hchild)
        CloseHandle(hchild);
    hchild = 0;
    hpipe = 0;
}

// ==================================================================================================
void c4s::proc_pipes::winpipe::read(ostream *pout)
{
    ostringstream oss;
    DWORD br,gle;
    if(!hpipe)
        return;
    if(!pending) {
        if( ReadFile( hpipe, buffer, sizeof(buffer), &br, &overlapped) ) {
            if(pout)
                pout->write(buffer,br);
            return;
        }
        gle = GetLastError();
        if(gle == ERROR_IO_PENDING) {
            pending = true;
            return;
        }
        oss << "proc_pipes::winpipe::read - Read file ("<<hpipe<<") failure:"<<strerror(gle);
        throw process_exception(oss.str());
    }
    if(GetOverlappedResult(hpipe,&overlapped,&br,FALSE)) {
        if(pout)
            pout->write(buffer,br);
        pending = false;
        return;
    }
    gle = GetLastError();
    if(gle != ERROR_IO_INCOMPLETE){
        oss << "proc_pipes::winpipe::read - ("<<hpipe<<") GetOverlapped failure:"<<strerror(gle);
        throw process_exception(oss.str());
    }
}
#endif // _WIN32

// ==================================================================================================
c4s::proc_pipes::~proc_pipes()
/*!
  Closes the pipes created by the constructor.
*/
{
#if defined(__linux) || defined(__APPLE__)
    if(fd_out[0]) close(fd_out[0]);
    if(fd_out[1]) close(fd_out[1]);
    if(fd_err[0]) close(fd_err[0]);
    if(fd_err[1]) close(fd_err[1]);
    if(fd_in[0]) close(fd_in[0]);
    if(fd_in[1]) close(fd_in[1]);
#endif
}

// ==================================================================================================
#if defined(__linux) || defined(__APPLE__)
void c4s::proc_pipes::init_child()
/*!
  Initilizes child side pipes. Call this from the child right after the fork and before exec.
  After this function you should destroy this object.
  NOTE! This does nothing in Windows since the functionality is not needed
  \retval bool True on succes, false on error.
*/
{
    close(fd_in[1]);
    close(fd_out[0]);
    close(fd_err[0]);
    dup2(fd_in[0],STDIN_FILENO); // = 0
    dup2(fd_out[1],STDOUT_FILENO); // = 1
    dup2(fd_err[1],STDERR_FILENO); // = 2
    close(fd_in[0]);
    close(fd_out[1]);
    close(fd_err[1]);
    memset(fd_out,0,sizeof(fd_out));
    memset(fd_err,0,sizeof(fd_err));
    memset(fd_in,0,sizeof(fd_in));
}
#endif

// ==================================================================================================
#if defined(__linux) || defined(__APPLE__)
void c4s::proc_pipes::init_parent()
/*!
  Initializes the parent side pipes. Call this from parent right after the fork.
  NOTE! This does nothing in Windows since the functionality is not needed
*/
{
    // Close the input read side
    close(fd_in[0]);
    fd_in[0] = 0;
    // Close the output write sides
    close(fd_out[1]);
    close(fd_err[1]);
    fd_out[1] = 0;
    fd_err[1] = 0;
}
#endif

// ==================================================================================================
#ifdef _WIN32
void c4s::proc_pipes::init(STARTUPINFO *info, HANDLE *waits)
/*! Initialize startup function. (Windows only)
   \param info Pointer to startup infro structure to be filled.
   \param waits Pointer to array of three wait handles.
*/
{
    memset(info,0,sizeof(STARTUPINFO));
    info->cb = sizeof(STARTUPINFO);
    info->hStdOutput = out.hchild;
    info->hStdError = err.hchild;
    info->hStdInput = in.hchild;
    info->dwFlags = STARTF_USESTDHANDLES;

    waits[1] = out.overlapped.hEvent;
    waits[2] = err.overlapped.hEvent;
}
#endif

// ==================================================================================================
void c4s::proc_pipes::read_child_stdout(ostream *pout)
/*!
  Read both stdout from the child process and print it out on the given stream.
  \param pout Stream for the output
*/
{
#if defined(__linux) || defined(__APPLE__)
    char out_buffer[512];
    ssize_t rsize = read(fd_out[0],out_buffer,sizeof(out_buffer));
    if(rsize>0 && pout) {
        pout->write(out_buffer,rsize);
    }
    br_out+=rsize;
#else
    out.read(pout);
#endif
}

// ==================================================================================================
void c4s::proc_pipes::read_child_stderr(ostream *pout)
/*!
  Read both stdout and stderr from the child process and print it out on the given stream.
  \param pout Stream for the output
*/
{
#if defined(__linux) || defined(__APPLE__)
    char out_buffer[512];
    ssize_t rsize = read(fd_err[0],out_buffer,sizeof(out_buffer));
    if(rsize>0 && pout) {
        pout->write(out_buffer,rsize);
    }
    br_err+=rsize;
#else
    err.read(pout);
#endif
}

// ==================================================================================================
// Experimental!
#if 0
size_t c4s::proc_pipes::read_child_stdin(ostream *pout)
{
#if defined(__linux) || defined(__APPLE__)
    char out_buffer[512];
    size_t total=0;
    ssize_t rsize=0;
    if(!pout)
        return 0;
    do {
        pout->write(out_buffer,rsize);
        total += rsize;
        rsize = read(fd_in[0],out_buffer,sizeof(out_buffer));
        cerr << "rsize="<<rsize<<'\n';
    }while(rsize>0);
    return total;
#endif
}
#endif

// ==================================================================================================
size_t c4s::proc_pipes::write_child_input(const path &pin)
/*!
  Writes the given string into the child input.
  \param input String to write.
*/
{
    char buffer[1024];
    size_t cnt=0;
#ifdef C4S_DEBUGTRACE
    cerr << "proc_pipes::write_child_input - Feeding '"<<pin.get_path()<<"' to child input\n";
#endif
#if defined(__linux) || defined(__APPLE__)
    streamsize ss;
    ifstream input(pin.get_path().c_str());
    if(!input) {
        ostringstream os;
        os << "proc_pipes::write_child_input - Unable to open input file:"<<pin.get_path()<<" for the child stdin.";
        throw process_exception(os.str());
    }
    do {
        input.read(buffer,sizeof(buffer));
        ss = input.gcount();
        if(ss>0) {
            write(fd_in[1],buffer,ss);
            cnt+=ss;
        }
    }while( ss>0 && input.good() );
    input.close();
#else
    HANDLE hin = CreateFile(pin.get_path().c_str(),GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(hin == INVALID_HANDLE_VALUE) {
        ostringstream os;
        os << "proc_pipes::write_child_input - Unable to open input file:"<<strerror(GetLastError());
        throw process_exception(os.str());
    }
    DWORD bw,ss;
    do {
        ReadFile(hin,buffer,sizeof(buffer),&ss,0);
        if(ss>0) {
            WriteFile(in.hpipe, buffer, ss, &bw, NULL);
            cnt+=ss;
        }
    }while( ss>0 );
    CloseHandle(hin);
    // Send Ctrl-Z = EOF.
    buffer[0]=0x1A;
    WriteFile(in.hpipe,buffer,1,&bw,NULL);
#endif
#ifdef C4S_DEBUGTRACE
    cerr << "proc_pipes::write_child_input - Wrote "<<cnt<<" bytes to child stdin\n";
#endif
    close_child_input();
    return cnt;
}

// ==================================================================================================
void c4s::proc_pipes::write_child_input(const string &input)
/*!
  Writes the given string into the child input.
  \param input String to write.
*/
{
#if defined(__linux) || defined(__APPLE__)
    ssize_t bw;
    bw = write(fd_in[1],input.c_str(),input.size());
#else
    DWORD bw;
    WriteFile(in.hpipe, input.c_str(), (DWORD)input.size(), &bw, NULL);
#endif
#ifdef C4S_DEBUGTRACE
    cerr << "proc_pipes::write_child_input - len:"<<input.size()<<'\n';
    if(bw<input.size()) {
        if(errno)
            cerr << "proc_pipes::write_child_input - "<<strerror(errno);
        cerr << "proc_pipes::write_child_input - failed. BW="<<bw<<'\n';
    }
#endif
    br_in += bw;
    send_ctrlZ = true;
}

// ==================================================================================================
// ###############################  PROCESS  ########################################################
// ==================================================================================================
bool c4s::process::no_run = false;
bool c4s::process::nzrv_exception = false;  //!< If true 'Non-Zero Return Value' causes exception.
ofstream* c4s::process::pipe_global = 0;

// ==================================================================================================
void c4s::process::init_member_vars()
/*! Common initialize code for all constructors. Constructors call this function first.
*/
{
    pid = 0;
    last_ret_val = 0;
    pipe_target = 0;
    pipes = 0;
    echo = false;
#if defined(__linux) || defined(__APPLE__)
    owner = 0;
    daemon = false;
#else
    output = 0;
#endif
}

// ==================================================================================================
c4s::process::process(const char* cmd, const char *args)
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
{
    init_member_vars();
    if(cmd) set_command(cmd);
    if(args) set_args(args);
}

// ==================================================================================================
c4s::process::process(const char *cmd, const char *args, ostream *out)
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
  \param out Pipe target. Process output is sent to this stream.
 */
{
    init_member_vars();
    if(cmd) set_command(cmd);
    if(args) arguments << args;
    pipe_target = out;
}

// ==================================================================================================
c4s::process::process(const string &cmd, const char *args, ostream *out)
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
{
    init_member_vars();
    if(!cmd.empty()) set_command(cmd.c_str());
    if(args) arguments << args;
    if(out) pipe_target=out;
}

// ==================================================================================================
c4s::process::process(const string &cmd, const string &args)
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
{
    init_member_vars();
    if(!cmd.empty()) set_command(cmd.c_str());
    if(!args.empty()) arguments << args;
}

// ==================================================================================================
c4s::process::process(const string &cmd)
/*! \param cmd Name of the command to execute. No arguments should be specified with command.
*/
{
    init_member_vars();
    if(!cmd.empty()) set_command(cmd.c_str());
}
// ==================================================================================================
c4s::process::process(const char *cmd)
/*! \param cmd Name of the command to execute. No arguments should be specified with command.
*/
{
    init_member_vars();
    if(cmd) set_command(cmd);
}

// ==================================================================================================
c4s::process::~process()
/*! If the the daemon mode has NOT been set then the destructor kills the process if it is still running.
 */
{
#ifdef C4S_DEBUGTRACE
    cerr << "process::~process - name="<< command.get_base()<<'\n';
#endif
#if defined(__linux) || defined(__APPLE__)
    if(pid && !daemon)
#else
    if(pid)
#endif
        stop();

    if(pipes) {
        delete pipes;
        pipes = 0;
    }
}

// ==================================================================================================
void c4s::process::set_command(const char *cmd)
/*!  Finds the command from current directory or path. If not found throws process
  exception. In windows .exe-is appended automatically if not specified in command.
  \param cmd Name of the command.
*/
{
    if(!cmd)
        throw process_exception("process::set_command - empty command");
    command = cmd;
#if defined(__linux) || defined(__APPLE__)
    struct stat sbuf;
    memset(&sbuf,0,sizeof(sbuf));
    // Check the user provided path first. This includes current directory.
    if( stat(command.get_path().c_str(),&sbuf) == -1 || ( sbuf.st_mode|(S_IXUSR|S_IXGRP|S_IXOTH) ) == 0 )
    {
        // cerr << "DEBUG - set_command:"<<command.get_path()<<"; st_mode="<<hex<<sbuf.st_mode<<dec<<'\n';
        if(!command.exists_in_env_path("PATH",true))
        {
            ostringstream ss;
            command.clear();
            ss << "process::set_command - Command not found: " << cmd;
            throw process_exception(ss.str());
        }
    }
#else
    char cmdpath[MAX_PATH];
    char *fnamePtr;
    if(!command.is_absolute())
    {
        if( command.get_ext().find(".exe") == string::npos )
            command.set_ext(".exe");
        // Search for the exe from pdsath.
        if(!SearchPath(0,command.get_base().c_str(),0,sizeof(cmdpath),cmdpath,&fnamePtr))
        {
            ostringstream ss;
            //cerr << "DEBUG -- process::set_command - command:"<<command.get_base()<<" - not found!\n";
            ss << " Command not found: " << cmd<<"\n System error: "<<strerror(GetLastError());
            throw process_exception(ss.str());
        }
        else
            command = cmdpath;
    }
#endif
}

// ==================================================================================================
void c4s::process::operator=(const process &source)
/*!  Copies the command name, arguments and pipe_target. Other
  attributes are simply cleared. Function should not be called if this process is running.
  \param source Source process to copy.
*/
{
    command = source.command;
    arguments.str("");
    arguments<<source.arguments.str();
    pid = 0;
    pipe_target = source.pipe_target;
#if defined(__linux) || defined(__APPLE__)
    daemon = source.daemon;
#else
    output = 0;
#endif
}

// ### \TODO continue to improve documentation from here on down.
// ==================================================================================================
void c4s::process::dump(ostream &os)
{
    const char *pe = echo ? "true":"false";
    const char *pt = pipe_target ? "OK":"None";
    os << "Process - "<<command.get_path()<<"(";
    os << arguments.str();
    os << ");\n   PID="<<pid<<"; echo="<<pe<<"; LRV="<<last_ret_val<<"; PT="<<pt<<'\n';
}

// ==================================================================================================
#ifdef _WIN32
void c4s::process::start(const char *args)
{
    STARTUPINFO info;
    PROCESS_INFORMATION pi;
    char *arg_ptr, arg_buffer[MAX_ARG_BUFFER];
    bool args_reserved = false;

    if(command.empty())
        throw process_exception("process::start - Unable to start process. No command specified.");
    if(pid)
        stop();
    streamsize max=command.get_path().size();
    if(!args)
        max += arguments.tellg();
    else
        max += strlen(args);
    max+=3;
    if(max>=MAX_ARG_BUFFER) {
        arg_ptr = new char[(SIZE_T)max];
        args_reserved = true;
    }
    else
        arg_ptr = arg_buffer;
    strcpy(arg_ptr,command.get_path_quot().c_str());
    strcat(arg_ptr," ");
    if(args)
        strcat(arg_ptr,args);
    else
        strcat(arg_ptr,arguments.str().c_str());

#ifdef C4S_DEBUGTRACE
    const char *pstr = pipe_target ? "enabled":"disabled";
    cerr << "process::start - "<<command.get_base()<<"; pipe="<<pstr<<"\n   final args:["<<arg_ptr<<"]\n";
#else
    if(echo)
        cerr << command.get_base() << '(' <<arguments.str()<<")\n";
#endif
    if(no_run)
        return;
    if(pipes)
        delete pipes;
    pipes = new proc_pipes();
    pipes->init(&info,wait_handles);

    if(!CreateProcess(command.get_path().c_str(),arg_ptr,0,0,TRUE,CREATE_NO_WINDOW,0,0,&info,&pi))
    {
        ostringstream os;
        os << " Unable to create process: "<<command.get_base()<<" with command line: "<<arguments.str();
        if(args_reserved) delete[] arg_ptr;
        throw process_exception(os.str());
    }
    pid = pi.hProcess;
    wait_handles[0] = pi.hProcess;
    if(args_reserved) delete[] arg_ptr;

    // If child input file has been defined, feed it to child.
    if(!in_path.empty()) {
        size_t wb = pipes->write_child_input(in_path);
        if(echo)
            cerr << "process::start - "<<command.get_base()<<". "<<wb<<" bytes committed to child.\n";
    }
}
#endif

// ==================================================================================================
#if defined(__linux) || defined(__APPLE__)
void c4s::process::start(const char *args)
{
    streamsize max;
    char tmp_argbuf[512];
    char tmp_cmdbuf[128];
    char *arg_buffer, *dynamic_buffer=0;
    const char *arg_ptr[MAX_PROCESS_ARGS];
    int ptr_count;

    if(command.empty())
        throw process_exception("process::start - Unable to start process. No command specified.");

    if(pid)
        stop();
    if(args) {
        arguments.str("");
        arguments << args;
    }
#ifndef C4S_DEBUGTRACE
    if(echo)
        cerr << command.get_base()<<'('<<arguments.str()<<")\n";
#endif
    last_ret_val = 0;
    if(no_run)
        return;

    memset(arg_ptr,0,sizeof(arg_ptr));
    // convention requires the first argument to be the path to command itself
    strcpy(tmp_cmdbuf,command.get_path().c_str());
    arg_ptr[0] = tmp_cmdbuf;
    ptr_count = 1;
    // These need to be in *argv[]. Copy from argumetn streamd to static buffer and
    // change spaces to zeros and at the same time make pointers to args.
    max = arguments.tellp();
    if(max>0) {
        size_t arg_buffer_length = max;
        if(arg_buffer_length > sizeof(tmp_argbuf)) {
            dynamic_buffer = new char[arg_buffer_length+1];
            arg_buffer = dynamic_buffer;
            arg_buffer_length++;
        }else{
            arg_buffer = tmp_argbuf;
            arg_buffer_length = sizeof(tmp_argbuf);
        }
        memset(arg_buffer,0,arg_buffer_length);
        arg_ptr[ptr_count++] = arg_buffer;

        int ch, prev=' ', quote=0;
        stringbuf *argsb = arguments.rdbuf();
        argsb->pubseekpos(0,ios_base::in);
        for(ch=argsb->sgetc(); ch!=EOF && max>0; ch=argsb->snextc()){
            if(quote) {
                if(quote==ch) {
                    if(prev!='\\')
                        quote = 0;
                    else
                        arg_buffer--;
                }
                else
                    *arg_buffer++=(char)(ch&0xff);
            }
            else if(ch == '\'' || ch == '\"') {
                if (prev=='\\')
                    *(arg_buffer-1)=(char)(ch&0xff);
                else
                    quote = ch;
            }
            else if(ch == ' ') {
                if(prev!=' ') {
                    *arg_buffer++=0;
                    arg_ptr[ptr_count++] = arg_buffer;
                    if(ptr_count >= MAX_PROCESS_ARGS-1)
                        throw process_exception("process::start - Too many arguments. Use response file.");
                }
            }
            else
                *arg_buffer++=(char)(ch&0xff);
            prev=ch;
            max--;
        }
        *arg_buffer = 0;
        if(quote)
            throw process_exception("process::start - Unmatched quote marks in arguments.");
    } // if arguments.tellp()
    else {
        arg_buffer = 0;
    }

    // Finalize argument pointer array
    arg_ptr[ptr_count] = 0;
    if(arg_ptr[ptr_count-1][0]==0)
        arg_ptr[ptr_count-1]=0;

#ifdef C4S_DEBUGTRACE
    cerr << "process::start - "<<command.get_path()<<'('<<tmp_cmdbuf<<"):\n";
    for(int i=0; arg_ptr[i]; i++)
        cerr << " ["<<i<<"] "<<arg_ptr[i]<<'\n';
    cerr << "process::start - About to fork, pipe=";
    if(pipe_target) cerr << "enabled";
    else cerr << "disabled";
    cerr << ", pipe_global=";
    if(pipe_global) cerr << "enabled\n";
    else cerr << "disabled\n";
#endif

    if(pipes)
        delete pipes;
    pipes = new proc_pipes();

    // Create the child process i.e. fork
    pid = fork();
    if(!pid) {
#ifdef C4S_DEBUGTRACE
        cerr << "process::start - created child: "<<getpid()<<endl;
#endif
        pipes->init_child();
        delete pipes;
        if(owner) {
            if(initgroups(owner->get_name().c_str(),owner->get_gid())!=0 ||
               setuid(owner->get_uid())!=0 ) {
                int er = errno;
                cerr << "process::start - child-process: Unable to change process persona. User:"<<owner->get_name()<<".\nError ("<<er<<") ";
                cerr << strerror(er)<<'\n';
                _exit(EXIT_FAILURE);
            }
        }
        if(execv(tmp_cmdbuf,(char**)arg_ptr) == -1) {
            cerr << "process::start - child-process: Unable to start process:"<<tmp_cmdbuf<<"\nError ("<<errno<<") "<<strerror(errno)<<'\n';
        }
        _exit(EXIT_FAILURE);
    }
    pipes->init_parent();
    if(dynamic_buffer)
        delete[] dynamic_buffer;
    // If child input file has been defined, feed it to child.
    if(!in_path.empty())
        pipes->write_child_input(in_path);
}
#endif // linux

// ==================================================================================================
#if defined(__linux) || defined(__APPLE__)
void c4s::process::set_user(user *_owner)
{
    if(_owner && _owner->status()==0)
        owner = _owner;
    else {
#ifdef C4S_DEBUGTRACE
        cerr << "process::set_user() - unable to set user.\n";
#endif
        owner=0;
    }
}
// ------------------------------------------------------------------------------------------
void c4s::process::attach(int _pid)
/*! Attaching allows developer to stop running processes by first attaching object to a process
  and then calling stop-function. Exception is thrown if pid is not found. If process alredy is running
  this function does nothing. Daemon mode is set on.
  \param _pid Process id to attach to.
 */
{
    if(pid)
        return;
    pid = _pid;
    last_ret_val = 0;
    daemon = true;
    if(!is_running()) {
        ostringstream os;
        os << "process::attach - Cannot attach. Process with PID ("<<pid<<") not found.";
        throw process_exception(os.str());
    }
}
// ------------------------------------------------------------------------------------------
void c4s::process::attach(const path &pid_file)
{
    long attach_pid=0;
    ostringstream os;
    if(!pid_file.exists()) {
        os<<"process::attach - pid file "<<pid_file.get_path()<<" not found";
        throw process_exception(os.str());
    }
    ifstream pf(pid_file.get_path().c_str());
    if(!pf) {
        os<<"process::attach - Unable to open pid file "<<pid_file.get_path();
        throw process_exception(os.str());
    }
    pf>>attach_pid;
    if(attach_pid)
        attach((int)attach_pid);
    else {
        os<<"process::attach - Unable to read pid from file "<<pid_file.get_path();
        throw process_exception(os.str());
    }
}

#endif // linux || Apple

// ==================================================================================================
int c4s::process::wait_for_exit(int timeout)
/*! If the timeout expires the process_exception is trown.
  \param timeout Number of seconds to wait.
  \retval int Return value from the process.
*/
{
    if(!pid)
        return last_ret_val;
    if(no_run) {
        last_ret_val=0;
        return 0;
    }
    ostream *pipe = pipe_global ? pipe_global : pipe_target;
#ifdef C4S_DEBUGTRACE
    cerr << "process::wait_for_exit - name="<<command.get_base()<<", pid="<<pid<<", timeout="<<timeout;
    if(!pipe) cerr << ", quiet mode\n";
    else cerr << '\n';
    time_t beg = time(0);
#endif
#if defined(__linux) || defined(__APPLE__)
    int counter = timeout*10;
    struct timespec ts_delay, ts_remain;
    ts_delay.tv_sec = 0;
    ts_delay.tv_nsec = 100000000L;
    pid_t wait_val;
    do {
        nanosleep(&ts_delay,&ts_remain);
        pipes->read_child_stderr(pipe);
        pipes->read_child_stdout(pipe);
        wait_val = waitpid(pid, &last_ret_val, WNOHANG);
        if(wait_val && wait_val!= pid) {
            ostringstream os;
            os<<"process::wait_for_exit - name="<<command.get_base()<<", wait error: "<<strerror(errno);
            throw process_exception(os.str());
        }
        counter--;
    }while(!wait_val && counter>0);
    pipes->read_child_stderr(pipe);
    pipes->read_child_stdout(pipe);
#ifdef C4S_DEBUGTRACE
    cerr <<"process::wait_for_exit - name="<<command.get_base()<<", retval:"<<last_ret_val<<", counter:"<<counter<<", seconds:"<<time(0)-beg;
    cerr <<", br_out:"<<pipes->get_br_out()<<", br_err:"<<pipes->get_br_err()<<'\n';
#endif
    if(counter<=0) {
        ostringstream os;
        os << "process::wait_for_exit - name="<<command.get_base()<<", pid="<<pid<<"; Process timeout!";
        throw process_exception(os.str());
    }

#else // Win32 ------------------------------
    DWORD lapse, now, start = GetTickCount();
    DWORD wfmo;
    do {
        wfmo = WaitForMultipleObjects(3,wait_handles,FALSE,100);
        now = GetTickCount();
        lapse = now>=start ? now-start : (~0)-start+now;
        if(wfmo>=WAIT_OBJECT_0 && wfmo<=WAIT_OBJECT_0+3) {
            int ndx = wfmo-WAIT_OBJECT_0;
            if(ndx==0)
                break; // = Process has stopped execution.
            if(ndx==1 && pipes)
                pipes->read_child_stdout(pipe);
            if(ndx==2 && pipes)
                pipes->read_child_stderr(pipe);
        }
    }while( wfmo <= WAIT_TIMEOUT && lapse<(DWORD)timeout*1000);

    if(wfmo>WAIT_TIMEOUT) {
        ostringstream os;
        os << "process::wait_for_exit - WaitForSingleObject error:"<<strerror(GetLastError());
        throw process_exception(os.str());
    }
    // Check for the timeout
    if(lapse>=(DWORD)timeout*1000) {
        ostringstream os;
        cerr <<"process::wait_for_exit - name="<<command.get_base()<<", pid="<<(int)pid<<"; Process timeout!";
        throw process_exception(os.str());
    }
    // Get the exit code
    if(!GetExitCodeProcess(pid,(LPDWORD)&last_ret_val)) {
        ostringstream os;
        os << "process::wait_for_exit - Unable to get process "<<hex<<pid<<dec<<" exit code.";
        throw process_exception(os.str());
    }
#endif
    pid = 0;
    pipes->read_child_stdout(pipe);
    pipes->read_child_stderr(pipe);
    stop();
    if(nzrv_exception && last_ret_val!=0) {
        ostringstream os;
        os << "Process: '"<<command.get_base()<<' '<<arguments.str()<<"' retured:"<<last_ret_val;
        throw process_exception(os.str());
    }
    return last_ret_val;
}
// ==================================================================================================
int c4s::process::execa(const char *plus, int timeout)
/*!
  Executes process with additional argument. Given argument is not stored permanently.
  Returns when the process is completed or timeout exeeded.
  \param args Additional argument to append to current set.
  \param timeout Number of seconds to wait. Defaults to C4S_PROC_TIMEOUT.
  \retval int Return value from the command.
*/
{
    streampos end = arguments.tellp();
    arguments<<' '<<plus;
    start();
    int rv = wait_for_exit(timeout);
    arguments.seekp(end);
    return rv;
}

// ==================================================================================================
int c4s::process::exec(int timeout, const char *args)
/*!
  Returns when the process is completed or
  timeout exeeded. This is a shorthand for start-wait_for_exit combination.
  \param timeout Number of seconds to wait for the process completion.
  \param args Optional arguments for the command. Overrides previously entered arguments if specified.
  \retval int Return value from the command.
*/
{
    start(args);
    return wait_for_exit(timeout);
}

// ==================================================================================================
int c4s::process::exec(int timeout, const string &args)
/*!
  Executes the command with optional arguments. Returns when the process is completed or
  timeout exeeded. This is a shorthand for start-wait_for_exit combination.
  \param timeout Number of seconds to wait for the process completion.
  \param args Arguments for the command. Overrides existing arguments.
  \retval int Return value from the command.
*/
{
    start(args.c_str());
    return wait_for_exit(timeout);
}

// ==================================================================================================
bool c4s::process::is_running()
/*!
   \retval bool True if it is, false if not.
*/
{
    if(!pid)
        return false;
#if defined(__linux) || defined(__APPLE__)
    if(kill(pid, 0) == 0)
        return true;
#else
    DWORD rv;
    GetExitCodeProcess(pid,&rv);
    if(rv == STILL_ACTIVE)
        return true;
    last_ret_val = rv;
#endif
    pid = 0;
    stop();
    return false;
}

// ==================================================================================================
void c4s::process::stop_daemon()
{
#if defined(__linux) || defined(__APPLE__)
    ostringstream os;
#ifdef C4S_DEBUGTRACE
    cerr << "process::stop_daemon - name="<<command.get_base()<<'\n';
#endif
    if(!pid)
        return;
    // Send termination signal.
    if(kill(pid,SIGTERM)) {
#ifdef C4S_DEBUGTRACE
      cerr << "process::stop: kill(pid,SIGTERM) error:"<<strerror(errno) <<'\n';
#else
      os << "process::stop: kill(pid,SIGTERM) error:"<<strerror(errno) <<'\n';
#endif
        throw process_exception(os.str());
    }
    // Lets wait for a while:
    struct timespec ts_delay, ts_remain;
    ts_delay.tv_sec = 0;
    ts_delay.tv_nsec = 400000000L;
    int rv, count=20;
    do {
        nanosleep(&ts_delay,&ts_remain);
        rv = kill(pid,0);
        count--;
    }while(count>0 && rv==0);
#ifdef C4S_DEBUGTRACE
    cerr <<"process::stop_daemon - term result:"<<rv<<"; count="<<count<<"; errno="<<errno<<'\n';
#endif
    if(count==0) {
        count = 10;
        kill(pid,SIGKILL);
        do {
            nanosleep(&ts_delay,&ts_remain);
            rv = kill(pid,0);
            count--;
        }while(count>0 && rv==0);
#ifdef C4S_DEBUGTRACE
        cerr <<"process::stop_daemon - kill result:"<<rv<<"; count="<<count<<"; errno="<<errno<<'\n';
#endif
        if(count==0)
            throw process_exception("process::stop_daemon - Failed, daemon sill running.");
    }
    if(errno!=ESRCH) {
#ifdef C4S_DEBUGTRACE
      cerr << "process::stop_daemon - error stopping daemon\n";
#endif
        throw process_exception("process::stop_daemon - error stopping daemon");
    }
    pid = 0;
#endif
}

// ==================================================================================================
void c4s::process::stop()
/*! Arguments and
  last_ret_val is left un-touched so that the process can be restarted. It is not recommended that
  this function is used to stop the process since the processes usually stop on their own. Use the
  exec- or wait_for_exit-functions instead. If the wait_for_exit gives timeout exception then this
  function can be called to stop the run-loose process.
*/
{
#ifdef C4S_DEBUGTRACE
    cerr << "process::stop - name="<<command.get_base()<<'\n';
#endif
    if(pid) {
      if(daemon) {
	stop_daemon();
	return;
      }
#if defined(__linux) || defined(__APPLE__)
        ostringstream os;
    AGAIN:
        pid_t cid = waitpid(pid,&last_ret_val,WNOHANG|WUNTRACED);
        if(cid == 0) {
            if(kill(pid,SIGTERM)) {
                os << "Unable to send termination signal to running process:"<<pid<<". (errno="<<errno<<")";
                throw process_exception(os.str());
            }
            cid = waitpid(pid,&last_ret_val,WNOHANG|WUNTRACED);
            if(cid == 0) {
                if(kill(pid,SIGKILL)) {
                    os << "Unable to kill process "<<pid<<". (errno="<<errno<<")";
                    throw process_exception(os.str());
                }
            }
#ifdef C4S_DEBUGTRACE
            cerr <<"Process::stop - used TERM/KILL to stop "<<pid<<".\n";
#endif
        }
        if(cid == -1) {
#ifdef C4S_DEBUGTRACE
            cerr << "process::stop - name="<<command.get_base()<<", waitpid failed. Errno="<<errno<<'\n';
#endif
            if(errno == EINTR)
                goto AGAIN;
            else if(errno == ECHILD) {
                // Process was probably attached and hence this is not a parent for the process.
	      os << "process::stop: waitpid error:"<<strerror(errno) <<'\n';
	      throw process_exception(os.str());
            }
            else{
                throw process_exception("Process::stop: Invalid argument to wait-function.");
            }
        }
#else // __linux
        if(!TerminateProcess(pid,999)){
            if(pipes) {
                delete pipes;
                pipes = 0;
            }
            ostringstream oss;
            oss << "Failed to terminate process:"<<pid<<". Please terminate it manually. ";
            oss << "Terminate error: "<<strerror(GetLastError());
            throw process_exception(oss.str());
        }
        CloseHandle(pid);
#endif
        pid=0;
    } // if(pid)

    if(pipes) {
        delete pipes;
        pipes = 0;
    }
}

// ==================================================================================================
void c4s::process::catch_output(const char *cmd, const char *args, string &output)
/*!  Possible errors with command will throw an exception.

  \param cmd Command to run.
  \param args Command arguments.
  \param output Buffer where the output will be stored into.
*/
{
    ostringstream os;
    process source(cmd,args);
    source.pipe_to(&os);
    int rv = source();
    if(rv) {
        ostringstream err;
        err << "process::catch-output - command returned error "<<rv;
        err << ". Output: "<<os.str();
        throw process_exception(err.str());
    }
    output = os.str();
}

// ==================================================================================================
void c4s::process::append_from(const char *cmd, const char *args, process &target)
/*! Possible errors with command will throw an exception. \see catch_output

  \param cmd Command to run.
  \param args Command arguments.
  \param target Process where the output will be appended to.
*/
{
    ostringstream os;
    process source(cmd,args);
    source.pipe_to(&os);
    int rv = source();
    if(rv) {
        ostringstream err;
        err << "process::append_from - command returned error "<<rv;
        err << ". Output: "<<os.str();
        throw process_exception(err.str());
    }
    target += os.str();
}
