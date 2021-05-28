# Final Report

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

![img](Final_Report.assets/logo.png)

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

![image-20210427223534886](Final_Report.assets/image-20210427223534886.png)

SylixOS has its own command line interface, which is named as `TTinyShell`. 

The experience of ttinyShell is similar to that of `Bash` or other shell program on Linux. There are some built-in commands such as `help`, `echo`, `ps`, `kill`, `ifconfig`, `shutdown`, `reboot` and so on.

Besides, ttinyShell has already included a basic auto-completion function. When typing in the prefix of the files in the current folder or `Home` folder,  it could automatically complete the rest; and if there are multiple file names share the same prefix, then the alternative will be also shown just below the current line. It's actually similar to `Bash`.

TTinyShell has no pipe, no output filter, and no divi-screen display. It also supports shell scripts, but complicated mechanisms like `if` / `else` / `while` could not be used in the scripts yet.

## Results

### Keyword Auto Completion

When `Tab` is pressed and there is only one word in the input buffer, similar keywords will be shown.

![image-20210528111052338](Final_Report.assets/image-20210528111052338.png)

### History Auto Suggestion

#### 1. Visual Effect

If we input the prefix of a command which was input before, the rest will be shown.

![image-20210528111133474](Final_Report.assets/image-20210528111133474.png)

If we move the cursor to the right and press `ENTER`, the commands could be input correctly.

![image-20210528181742082](Final_Report.assets/image-20210528181742082.png)

#### 2. New commands

There are two new commands related to this function: `clearhistory` , 

![image-20210528111649500](Final_Report.assets/image-20210528111649500.png)

And `loadhistory`.

![image-20210528111735667](Final_Report.assets/image-20210528111735667.png)

### *ls* and *ll* Enhancement

By default, `ll` and `ls ` will always show all the files in the specified directory, whether or not their names start with ".", and there is no argument like `-a` nor `--all` supported. So we implement the function of "hidden files/folders".

![image-20210528112315874](Final_Report.assets/image-20210528112315874.png)

![image-20210528112245294](Final_Report.assets/image-20210528112245294.png)

## Design

### Keyword Auto Completion

Actually, ttinyShell already has the function of auto-completion, however, only names of files/directories could be matched. Here we implement the function of keyword-completion.

In SylixOS, not all shell commands are stored as files in the file system, which means that we cannot just scan all the files in the `PATH` directories and match the most similar one.

Commands in ttinyShell are named as `keyword`, and some APIs related to this function are already included in file `ttinyShellLib.c`. For example, the following function will return all keyword in an array.

```c
ULONG  __tshellKeywordList (__PTSHELL_KEYWORD   pskwNodeStart,
                            __PTSHELL_KEYWORD   ppskwNode[],
                            INT                 iMaxCounter);
```

Besides, as no executable files or shell script could be executed directly in ttinyShell (it means that we must run the program with the commands ` ./xxx.bin`), the first term in a valid command should always be a **keyword**. So our design logic is that, if there is only one term in the commands, only keyword-completion will be used, otherwise only filename-completion will be used.

### History Auto Suggestion

Basically, we implement this function with the data structure *Prefix Tree*, also named as *Trie*. The following image is an example for how to use trie to represent the history commands. (Input commands: `ifconfig`, `loglevel`, `logfiles`) 

![图片2](Final_Report.assets/%E5%9B%BE%E7%89%872.png)

Two new source files are added into the directory `libsylixos/SylixOS/kernel/tree`, which are `trie.c` and `trie.h`. 

Trie node struct is defined as follows:

```c
typedef struct __trie_node {
    unsigned            isValid:1;               /*  child是否被初始化      */
    unsigned            isEnd:1;                 /*  是否是某个指令的结尾    */
    unsigned            frequence:15;            /*  此节点被使用的频率      */
    unsigned            maxFrequence:15;         /*  子节点中最大使用频率    */
    char                mostFrequentlyUsedChar;  /*  最常使用子节点         */
    struct __trie_node *child;                   /*  如被初始化, 则是一个长为256的指针数组     */
} LW_TRIE_NODE;
typedef LW_TRIE_NODE   *PLW_TRIE_NODE;
```

Several important macro function and functions are defined here.

`ENLARGE_LIST()` is used for the self-defined arraylists.

```c
#define ENLARGE_LIST(array_list, original_length_element, element_type, pointer_type)
```

`INITIALIZE_TRIE_NODE` is used for assigning initial values for the attributes in the structure.

```c
#define INITIALIZE_TRIE_NODE(trieNode)
```

`__trieNodeValidate` will allocate memory for the child array.

```c
VOID __trieNodeValidate(PLW_TRIE_NODE trieNode);
```

`__trieInsert` inserts a string into the trie, and update the frequency of passed nodes. 

```c
VOID __trieInsert(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
```

`__trieSearch`: if the input string cannot be found in the trie, return null; otherwise output the postfix of the most frequently used command with a prefix equal to the current command. 

```c
PCHAR __trieSearch(PLW_TRIE_NODE trieNode, PCHAR sentence, int n);
```

`__trieDelete` will delete all allocated memory of a trie recursively.

```c
VOID __trieDelete(PLW_TRIE_NODE trieNode);
```

`__trieToFile` and `__trieFromFile` are two functions used to transform a trie into sequential bytes. We use BFS algorithm here to traverse every node in the trie, and the trie is represented sequentially in a `Leetcode` way. Read [this website](https://support.leetcode.com/hc/en-us/articles/360011883654-What-does-1-null-2-3-mean-in-binary-tree-representation-) to know more about how does Leetcode represents tree in the OJ. 

```c
VOID            __trieToFile(PLW_TRIE_NODE trieNode, FILE *file);
PLW_TRIE_NODE   __trieFromFile(FILE *file);
```

The history trie will be initialized when the function `__tshellReadlineInit` is called. If a history recording file specified for the current user could be found, the new trie will be restored from the binaries; if not the new trie will be only an empty trie. The logics mentioned above are in the function `__tshellInitHistoryTrie`.

```c
static VOID __tshellInitHistoryTrie(VOID);
```

  Whenever a normal char is input into the shell, the function `__tshellCharInster` will be called. Then in the end of the function processing, `__trieSearch` will be used and the output will be directly print to the terminal.

![image-20210528212636720](Final_Report.assets/image-20210528212636720.png)

When a command is executed successfully after `ENTER` pressed, the function `__tshellAfterExecution` will be called, and it will record the new command and store it into the history trie.

```c
VOID __tshellAfterExecution(PVOID  pcBuffer, size_t  stSize, INT returnValue);
```

All matched string will be stored. If `RIGHT` is pressed and the cursor is at the end of the command, the first character will be included in the current input buffer, and the stored matched string will be updated.

Also, two new commands `clearhistory` and `loadhistory` are created. The implementation logic are similar with what we have mentioned above. The way to register a new command will be shown in the next part.

### *ls* and *ll* Enhancement

Before the introduction in this part,  we should firstly explain how a command is registered into kernel.

There are totally 3 functions related to this topic:

`API_TShellKeywordAdd`  could register a handler function for the name of the new command.

```c
ULONG  API_TShellKeywordAdd(CPCHAR pcKeyword, PCOMMAND_START_ROUTINE pfuncCommand);
```

`API_TShellFormatAdd` and `API_TShellHelpAdd` are used to add "help" infos.

```c
ULONG API_TShellFormatAdd(CPCHAR pcKeyword, CPCHAR pcFormat);
ULONG API_TShellHelpAdd(CPCHAR pcKeyword, CPCHAR pcHelp);
```

Let's take some source code as an example. The following codes shows all of what we need to do of registering a new command.

```c
API_TShellKeywordAdd("echo", __tshellSysCmdEcho);
API_TShellFormatAdd("echo", " [message]");
API_TShellHelpAdd("echo", "echo the input command.\necho [message]\n");
```

The handler function should be defined in this way:

```c
static INT  __tshellSysCmdEcho (INT  iArgC, PCHAR  ppcArgV[]);
```

`ppcArgV` means all input parameter in the command divided by space, and `iArgC` means the number of all the arguments.

The system command `ls` and `ll` are defined in the file `libsylixos\SylixOS\shell\fsLib\ttinyShellFsCmd.c` by the following handler functions:

```c
static INT  __tshellFsCmdLs (INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellFsCmdLl (INT  iArgC, PCHAR  ppcArgV[])
```

In the previous implementation, no additional arguments are supported, including `-a` and `--all`. By default, all file entries will be shown regardless of whether they should be hidden or not.

The rest part of our modification is clear. Firstly, we just read all input arguments and set a flag if `-a` or `--all` are input:

```c
for (index = 1; index < iArgC; index++) {
	...
	if (lib_strcmp(ppcArgV[index], "-a") == 0 || lib_strcmp(ppcArgV[index], "--all") == 0) {
            isAll = LW_TRUE;
	...
}
```

Then, do not print the result if the flag `isAll` is on or if the filename does not start with ".".

```c
if (isAll || *(pdirent->d_name) != '.') {
	<<<PRINT A FILE ENTRY>>>
}
```

So that's all of what we have done for this part. It's simple, but we have learned a lot about the utilization of the command registration APIs.

## Conclusion and Future Work

