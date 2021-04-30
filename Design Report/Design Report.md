# Design Report of Project 2

## Group Info

### Project Title

proj12-shell-enhancement-on-SylixOS

### Project Objective

Design and improve the command line mechanism of `SylixOS`.

### Deadline for this Project

2021/5/30 23 : 00

### Group ID

We are Group-11.

### Group Member

| Name   | SID      |
| ------ | -------- |
| 徐向宇 | 11810113 |
| 罗叶安 | 11810616 |
| 李昊锦 | 11810911 |

## Project Background and Description

### Introduction to SylixOS

![img](Design%20Report.assets/logo.png)

`SylixOS` is a large-scale embedded real-time operating system (OS), with a completely China-made kernel.

Real-time operating system (RTOS) refers to an OS intended to run real-time applications that process the data when it is is inputted into the system, typically without buffer delays. 

Embedded system refers to operating system that runs on embedded devices. Embedded devices typically use special OS, which is RTOS in most cases.

As a preemptive multitasking hard real-time operating system, SylixOS has the many useful functions and characteristics. Some of which might be related to our project are listed as following:

1. Compatible with POSIX 1003.1b (ISO/IEC 9945-1) real-time programming standard
2. Support unlimited multitasking
3. Support a variety of emerging asynchronous event synchronization interfaces, such as: `signalfd`, `timerfd`, `eventfd`, etc.
4. Support extended system symbol interface
5. Support standard `TCP` / `IPv4` / `IPv6` dual network protocol stack and provide standard socket operation interface
6. Internal integration of many network tools, such as: `FTP`, `TFTP`, `NAT`, `PING`, `TELNET`, `NFS`, etc.
7. Internal integrated shell interface, support environment variables (basically compatible with Linux operating habits)
8. Support many standard equipment abstractions, such as: `TTY`, `BLOCK`, `DMA`, `ATA`, `GRAPH`, `RTC`, `PIPE`, etc.
9. The kernel, drivers, and applications support GDB debugging
10. Provide a kernel behavior tracker to facilitate application performance and failure analysis

### Introduction to TTinyShell

![image-20210427223534886](Design%20Report.assets/image-20210427223534886.png)

SylixOS has its own command line interface, which is named as `TTinyShell`. 

The experience of ttinyShell is similar to that of `Bash` or other shell program on Linux. There are some built-in commands such as `help`, `echo`, `ps`, `kill`, `ifconfig`, `shutdown`, `reboot` and so on.

Besides, ttinyShell has already included a basic auto-completion function. When typing in the prefix of the files in the current folder or `Home` folder,  it could automatically complete the rest; and if there are multiple file names share the same prefix, then the alternative will be also shown just below the current line. It's actually similar to `Bash`.

TTinyShell has no pipe, no output filter, and no divi-screen display. It also supports shell scripts, but complicated mechanisms like `if` / `else` / `while` could not be used in the scripts yet.

### Tools We will Use

#### 1. RealEvo-IDE

![image-20210427220346152](Design%20Report.assets/image-20210427220346152.png)

`RealEvo-IDE` is a SylixOS dedicated operating system integrated development environment. It's a Eclipse-like software, and it includes:

1. SylixOS Base project, which includes `libsylixos` (the kernel of SylixOS) , `libcextern` (C extension library), `libreadline`, `liblua`, `libluaplugin`, `libzmodem`, `libsqlite3`, etc.
2. Various Project Template: `BSP` project, application project, kernel module project, etc.
3. Some built-in tools, such as auto-commit tool, kernel behavior  monitor, etc.
4. IDE: helps us to manage, construct and compile projects, and provides supports for different tool-chain
5. Tool-chains: compilers under different platforms, includes ARM, PowerPC, x86, x86_64, DSP, SPARC, Lite, RISC-V, etc.

#### 2. RealEvo-Simulator

![image-20210427220313915](Design%20Report.assets/image-20210427220313915.png)

`RealEvo-Simulator` is a hypervisor for SylixOS virtual machines. Since `RealEvo-Simulator` runs VMs, it would not require the CPU architecture type of the host machine. It provides putty-based telnet terminal for users to interact with the VMs, and help users to configure the network adapters of the host machine.

## Expected goals

### Auto Completion

Basically, our goals for this part is as following:

1. Auto filename filling for all files in the current / HOME directory and all paths included in the `PATH` variable. After this function is finished, the shell could auto-fill all command names.

    ![image-20210429192102031](Design%20Report.assets/image-20210429192102031.png)

2. History command completion. Example: `zsh-autosuggestions` plugin of `oh-my-zsh`

    ![image-20210429192442646](Design%20Report.assets/image-20210429192442646.png)

3. **If possible:** an API for adding help info for various commands. Example `complete`

### Pipeline

Pipeline is a very important function in the implementation of the shell, as the wiki says, "Streams may be used to chain applications, meaning that the output stream of one program can be redirected to be the input stream to another application. In many operating systems this is expressed by listing the application names, separated by the vertical bar character, for this reason often called the [pipeline](https://en.wikipedia.org/wiki/Pipeline_(Unix)) character. " 

And in bash's design, A pipeline is a sequence of one or more commands separated by one of the control operators ‘|’ or ‘|&’, whose format is:

```
[time [-p]] [!] command1 [ | or |& command2 ] ...
```

In general, Pipeline allows users to connect output of a command to the input of the next command. That is, each command reads the previous command’s output. Pipes and redirects are very similar and connection of pipes is performed before any redirections specified by the command. Same as other commands, 'time' can be used at the beginning of command to count the elapsed, user and system time.

For example, the following command means that redirecting standard output to null file if error happens such as not finding the file and pass the output to "grep" command otherwise:

```
cat test.sh test1.sh 2>null | grep -n 'echo'
```

In particular,  '|&' means that in addition to command1's standard output, its standard error is connected to command2's standard input through pipe. It is short for 2>&1|.

In SylixOS, redirection has been implemented, but the pipeline part has not been yet. So what we want to realize is a pipeline mechanism like bash which connect output of the command to the next command, including '|' and '|&'. But for the 'time' segment mentioned above, which is not realized in SylixOS now, we do not guarantee to implement it, because it is not part of pipeline. 

### Shell Execution Enhancement

Linux shell provide convenient functions like **grep**, **find**. In addition, mordern shell script interpreters provide schema supporting complex branching control behaviors, for example, parse and interpret **if** ,**while** and **for** statements.</br>
SylixOS also has its shell script interpreter. Unfortunately, it does not support **grep**, **find** and some other basic commands or complex branching control behaviors. Its recognition capability of shell syntax patterns is also deficient. The following command runs correctly but produces an error on SylixOS due to its extra blank space:</br>

```
ls > 1.txt
```

Our goal is to improve the shell programming capacity. First to implement simple functions like **grep** and **find**, and combine them with pipe schema.</br>
**If possible:** Support regular expression inputs in **grep**, **find** and other commands like Linux do. </br>
**If possible:** We will implement a more powerful shell interpreter supports complex branching control behaviors.

## Implementation

### Auto Completion

In `Bash`, the auto-completion function is based on `Readline` library. The GNU Readline library provides a set of functions for use by applications that allow users to edit command lines as they are typed in. In SylixOS, there is already basic auto-completion function implemented, which could fill the filename of those in the current or `HOME` directory when typing in the prefixes of them, and the way of its implementation is quite similar to the Readline library.

#### Where to change

The first question we shall answer about the function we want to accomplish is that, which codes we should modify?

 Let's begin with a brief introduction to the source codes of ttinyShell. Most of the core running logics are in the function `__tshellThread` in the file `ttinyShellLib.c`, this function will repeatedly read in characters in a loop.

```c
PVOID   __tshellThread (PVOID  pcArg)
{
	...
	for (;;) {
		...
		if (__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_NOECHO) {
            iReadNum = (INT)read(0, &cRecvBuffer[iTotalNum], 
                                 LW_CFG_SHELL_MAX_COMMANDLEN - 
                                 iTotalNum);                            /*  接收字符串  */
        } else {
            iReadNum = (INT)__tshellReadline(0, &cRecvBuffer[iTotalNum], 
                                             LW_CFG_SHELL_MAX_COMMANDLEN - 
                                             iTotalNum);                /*  接收字符串  */
        }
		...		
	}
	...
}
```

 Then, the function `__tshellReadline` in file `ttinyShellReadline.c` will check every input character, and dispatch the events to different event handler depends on the current input character and the characters in the input context.

```c
ssize_t  __tshellReadline (INT  iFd, PVOID  pcBuffer, size_t  stSize)
{
	...
	} else if (__KEY_IS_TAB(cRead)) {                               /*  tab  */
            __tshellCharTab(iFd, &sicContext);
	...
}
```

The function `__tshellCharTab`  is what we call "handler". If the users type in `TAB`, this function will be executed; and that means, most of our future work about auto-completion will base on this function.

#### How to change

The following are a quite short and brief route to implement auto-completion:

1. Modify the function `__tshellFileMatch`, to make it possible to input multiple paths

    ```c
    static VOID  __tshellFileMatch (INT  iFd, PCHAR  pcDir, PCHAR  pcFileName, __PSHELL_INPUT_CTX  psicContext);
    ```

2. Modify the function `__tshellCharTab`. When a `TAB` is read in and there is only one item (the command) in the command line buffer, run `__tshellFileMatch` for all paths in `PATH`. We could get the environment variable by the function `__tshellVarGet` in the file `ttinyVarLib.h`

    ```c
    ULONG  __tshellVarGet(CPCHAR  pcVarName, PCHAR  *ppcVarValue);
    ```

3. To implement history suggestions: currently the history commands are stored in a double linked list in struct `__SHELL_HISTORY_CTX`, and the head node is named to `_K_plineShellHisc`. If we traverse all the nodes to find the command best fit the current input prefix, the time complexity would be $O(n)$. If possible, we could implement a trie (prefix tree) to store them. This will reduce the time complexity to $O(\log n)$, and the worst space complexity is $O(n)$.

4. TTinyShell supports color configuration in the file `ttinyShellColor.h`, so we could change characters' color if they are "suggestions"

### Pipeline

As the part of Project Background and Description and Expected Goals says, we want to realize a pipeline mechanism including '|' and '|&'.

At present, our general realization idea is to connect the standard input and output of two different program processes.

**Standard input & Standard output:** 

Every process has 3 default file handles and descriptor:

| file descriptor number | name   | meaning                                                      |
| ---------------------- | ------ | ------------------------------------------------------------ |
| 0                      | Stdin  | stream from which a program reads its input data             |
| 1                      | Stdout | stream to which a program writes its output data             |
| 2                      | Stderr | another output stream typically used by programs to output error messages |

So if we can connect the command's Stdout to the next command's Stdin, the pipeline will be implemented. 

In terms of specific implementation, our current consideration is achieved through the pipe method in the C header file unistd.h. Since SylixOS conforms to the POSIX standard, unistd.h should be a usable header file, and the pipe() system call can create a pipe. We then redirect the standard output in the process where the previous command is located to the pipe entry, Redirect the standard input of the latter command to the exit.

But the current difficulty is that it does not directly use fork, dup2 and other system calls, and it also encapsulates exec calls, so it cannot be easily achieved through processes such as pipe, fork, exec, dup2. In our current opinion, it may be necessary to further understand the source code, debug the virtual machine, explore and try.

### Shell Execution Enhancement

**grep** and **find** functions receive strings from command line argument and then process them. To implement these function, we shall use the command register function provided by SylixOS:</br>

```c
VOID tmInstallCmd(VOID);
```

It enables us to add functions without performing invasive changes on SylixOS's kernel, which is suitable for simple functions like **grep**.</br>
Unfortunately, we did not find the source code of its original shell interpreter in the whole source code, and none of us has expierence on implementing interpreter for script languages. Therefore, enhancing branching control behaviors would be extremely difficult.

## Division of Labors
| Name   | Labor           |
| ------ | --------------- |
| 徐向宇 | auto-completion |
| 罗叶安 | pipe            |
| 李昊锦 | split-screen    |

## Time Schedule

| Week | Task                  |
| ---- | --------------------- |
| 12   | auto-completion       |
| 13   | pipe                  |
| 14   | pipe, shell execution |
| 15   | shell execution       |

## Reference

1. `SylixOS_application_usermanual.pdf`
2. `SylixOS shell 增强开发指导文档.docx`
3. https://en.wikipedia.org/wiki/Standard_streams
4. https://en.wikipedia.org/wiki/Operating_system
5. http://www.gnu.org/software/bash/manual/