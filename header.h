#ifndef HEADER_H
#define HEADER_H

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <string>
#include <pwd.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <grp.h>
#include <unistd.h>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fstream>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <queue>


using namespace std;

#define PERMLEN 10
#define TIMELEN 12
#define GAP 6

extern int userlen;
extern int grplen;
extern int sizelen;
extern int cursorpos;


extern 	vector<string> dirContents;
extern 	vector<string> history;
extern int historyPtr;
extern string cmd,destination;
extern vector<string> srcs;
extern vector<string> tokens;
extern string root;
extern int offset;


string getHistory();
void listDir(string);
bool isDir(string path);
bool isFile(string path);
string getPath(int ,int );

string commandstatus();
bool iscmd(string);
bool parsecmd(string);
void copy(string);
void move(string);
void create_dir(string);
void create_file(string);
void rename(string);
void delete_dir(string);
void delete_file(string);
bool search(string);
string Goto(string,string);



#endif
