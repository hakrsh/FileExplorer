#include "header.h"
using namespace std;

enum keys {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  ENTER,
  HOME,
};

struct config {
  struct termios default_termios;
  int cx, cy;
  int screenrows;
  int screencols;
  string root;
  string pwd;
  string status;

} E;

string output;
string root;
vector<string> history;
int historyPtr = 0;
int offset = 0;
int beg, arrlen, end_;

void err(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void outtofile(string name, int data) {
  ofstream ferr(name, ios_base::app);
  ferr << data << endl;
  ferr.close();
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.default_termios) == -1)
    err("tcsetattr");
}
void enableRawMode() {
  struct termios raw;
  if (tcgetattr(STDIN_FILENO, &E.default_termios) == -1)
    err("tcgetattr");
  atexit(disableRawMode);

  raw = E.default_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_iflag &= !(IXON);
  raw.c_oflag &= !(OPOST);
  // raw.c_cc[VMIN] = 0;
  // raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    err("tcsetattr");
}

int readKey() {
  char c;
  int nread;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    if (nread == -1)
      err("read");
  if (c == '\r')
    return ENTER;
  if (c == 'h')
    return HOME;
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      }
    }
    return '\x1b';
  }
  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    return -1;
  else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    if(E.cx > *rows - 2 )
      E.cx = min(int(dirContents.size() - offset), E.screenrows - 2);
    return 0;
  }
}

void init() {
  E.cx = 1;
  E.cy = 42;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    err("getWindowSize");
  char dirName[1000];
  getcwd(dirName, 1000);
  E.root = dirName;
  E.pwd = dirName;
  root = E.root;
  E.status = "normal";
}

void printFiles(string path) {
  string temp = "\x1b[2J\x1b[?25l\x1b[H";
  listDir(path.c_str());
  beg = offset;
  end_ = min(int(dirContents.size()), beg + E.screenrows - 2); // leave 2 rows 

  for (int i = beg; i < end_; i++)
    temp = temp + dirContents[i] + "\r\n\x1b[K";
  write(STDOUT_FILENO, temp.c_str(), temp.length());
  E.cy = cursorpos;
}
void printstatusbar(string msg) {
  string statusbar;
  statusbar = "\x1b[" + to_string(E.screenrows) + ";1H";
  statusbar += msg;
  write(STDOUT_FILENO, statusbar.c_str(), statusbar.length());
}

void moveCursor(int key) {
  switch (key) {
  case ARROW_UP:
    if (E.cx > 1)
      E.cx--;
    break;
  case ARROW_DOWN:
    if (E.cx < min(int(dirContents.size() - offset), E.screenrows - 2))
      E.cx++;
    break;
  }
}
void refreshScreen() {
  if (E.status == "normal") {
    printFiles(E.pwd);
    printstatusbar(":Normal Mode");
    output = "\x1b[" + to_string(E.cx) + ";" + to_string(E.cy) + "H";
    output += "\x1b[?25h";

    write(STDOUT_FILENO, output.c_str(), output.length());
  }
}
void processKeyPress() {
  int c = readKey();
  switch (c) {
  case 'q':
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    disableRawMode();
    exit(0);
    break;

  case ARROW_UP:
  case ARROW_DOWN:
    moveCursor(c);
    break;

  case HOME: {
    if (E.pwd == E.root)
      return;
    E.pwd = E.root;
    history.clear();
    historyPtr = 0;
    E.cx = 1;
    offset = 0;
    break;
  }
  case ARROW_LEFT: {
    offset = 0;
    if (historyPtr == 0)
      return;
    if (historyPtr > 1) {
      historyPtr--;
      E.pwd = history[historyPtr - 1];
    } else {
      historyPtr = 0;
      E.pwd = E.root;
    }
    E.cx = 1;
    break;
  }
  case ARROW_RIGHT: {
    offset = 0;
    if (history.size() != historyPtr) {
      historyPtr++;
      E.pwd = history[historyPtr - 1];
      E.cx = 1;
    }
    break;
  }
  case BACKSPACE: {
    offset = 0;
    if (historyPtr == 0)
      return;
    if (historyPtr == 1) {
      history.erase(history.begin() + historyPtr, history.end());
      E.pwd = E.root;
    } else {
      string parent = history[historyPtr - 1];
      if (parent == E.root)
        return;
      parent.erase(parent.find_last_of('/'));
      if (historyPtr != history.size()) {
        history.erase(history.begin() + historyPtr, history.end());
      }

      history.push_back(parent);
      historyPtr++;

      E.pwd = parent;
    }
    E.cx = 1;
    break;
  }
  case ':': {
    E.status = "command";
    char c[4];
    string temp = "\r";
    printstatusbar(":Command Mode\r\n");
    while (read(STDIN_FILENO, &c, 4)) {
      if (c[0] == '\x1b') {
        if (c[1] == '[') {
          c[1] = 0;
          continue;
        }
        // char d;
        // if(read(STDIN_FILENO,&d,1) != -1 ){
        //   while (getchar() != EOF);
        //   continue;
        // }
        E.status = "normal";
        break;
      } else if (c[0] == BACKSPACE) {
        write(STDOUT_FILENO, "\x1b[2K", 4);
        if (temp.length() > 1)
          temp.pop_back();
        write(STDOUT_FILENO, temp.c_str(), temp.length());

      } else if (c[0] == 13) {
        parsecmd(temp);
        if (!iscmd(cmd)) {
          temp += "\r\nCommand Not found\r\n";
          write(STDOUT_FILENO, temp.c_str(), temp.length());
        } else {
          if (cmd == "copy") {
            if (tokens.size() < 3)
              temp += "\r\nmissing arguments! \r\n";
            else {
              copy(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "move") {
            if (tokens.size() < 3)
              temp += "\r\nmissing arguments! \r\n";
            else {
              move(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "create_dir") {
            if (tokens.size() < 3)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 3)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              create_dir(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "create_file") {
            if (tokens.size() < 3)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 3)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              create_file(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "rename") {
            if (tokens.size() < 3)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 3)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              rename(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "delete_dir") {
            if (tokens.size() < 2)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 2)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              delete_dir(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else if (cmd == "delete_file") {
            if (tokens.size() < 2)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 2)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              delete_file(E.pwd);
              // temp = "\r\n";
              temp = commandstatus();
            }

          } else if (cmd == "search") {
            if (tokens.size() < 2)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 2)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              string cwd = E.pwd;
              temp = search(E.pwd) ? "\r\nTrue\r\n" : "\r\nFalse\r\n";
              E.pwd = cwd;
              
            }
          } else if (cmd == "goto") {
            if (tokens.size() < 2)
              temp += "\r\nmissing arguments! \r\n";
            else if (tokens.size() > 2)
              temp += "\r\ntoo many arguments!! \r\n";
            else {
              E.pwd = Goto(E.pwd,E.root);
              E.cx = 1;
              // E.status = "normal";
              // refreshScreen();
              // E.status = "command";
              // temp = "\r\n";
              temp = commandstatus();
            }
          } else
            temp += "\r\nComming Soon!\r\n";
          write(STDOUT_FILENO, temp.c_str(), temp.length());
        }
        temp = "\r";
      } else {
        temp += c[0];
        write(STDOUT_FILENO, temp.c_str(), temp.length());
      }
    }
    break;
  }
  case 'k': {
    if (offset > 0) {
      offset--;
    }
    break;
  }
  case 'l': {
    if ((offset + E.screenrows-2) < dirContents.size()) {
      offset++;
    }
    break;
  }
  case ENTER: {
    string path = getPath(E.cx, offset);
    string absPath = E.pwd + "/" + path;

    if (isFile(absPath)) {

      if (fork() == 0) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        disableRawMode();
        execlp("vi", "vi", absPath.c_str(), NULL);
      } else {
        wait(NULL);
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        enableRawMode();
        refreshScreen();
      }
      return;
    }
    if (absPath == (E.root + "/.."))
      return;

    if (path == ".")
      return;
    if (path == ".." && historyPtr > 0)
      historyPtr--;

    else if (isDir(absPath)) {
      if (history.size() == historyPtr) {
        history.push_back(absPath);
        historyPtr++;
      } else if (absPath == history[historyPtr])
        historyPtr++;
      else if (absPath != history[historyPtr]) {
        history.erase(history.begin() + historyPtr, history.end());
        history.push_back(absPath);

        historyPtr++;
      }
    }
    if (historyPtr)
      E.pwd = history[historyPtr - 1];
    else
      E.pwd = E.root;
    E.cx = 1;
    offset = 0;
    break;
  }
  }
}

int main() {
  enableRawMode();
  init();

  while (1) {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
      err("getWindowSize");
    refreshScreen();
    processKeyPress();
  }
}