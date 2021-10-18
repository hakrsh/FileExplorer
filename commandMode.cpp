#include "header.h"

vector<string> commands{"copy",       "move", "rename", "create_file",
                        "create_dir", "goto", "delete_dir","delete_file","search"};
string cmd, destination;
vector<string> srcs;
vector<string> tokens;


// string root = "/home/hari/testing/aos/A1";

string manualerrmsg; //for custom err msg
string commandstatus(){
  string msg = "\r\n";
  if(errno){
    msg += strerror(errno);
    msg += "\r\n";
  }
  else if(!manualerrmsg.empty()){
    msg += manualerrmsg;
    msg += "\r\n";
  }
return msg;
}


bool iscmd(string cmd) {
  return find(commands.begin(), commands.end(), cmd) != commands.end();
}
bool ismove = false;

//spliting the string into cmd,src,dest 
bool parsecmd(string s) {
  errno = 0;
  manualerrmsg = "";
  tokens.clear();
  s.erase(s.begin()); //remove \r
  cmd = destination = "";
  destination.clear();
  srcs.clear();
  stringstream ss(s);
  string temp;
  while (ss >> temp)
    tokens.push_back(temp);
  if (tokens.size() == 1)
    // return false;
    cmd = tokens[0];
  if (tokens.size() == 2) {
    cmd = tokens[0];
    destination = tokens[1];
  } else if (tokens.size() == 3) {
    cmd = tokens[0];
    srcs.push_back(tokens[1]);
    destination = tokens[2];
  } else if (tokens.size() > 3) {
    cmd = tokens[0];
    int i = 1;
    for (i = 1; i < tokens.size() - 1; i++)
      srcs.push_back(tokens[i]);
    destination = tokens[i];
  }
  return true;
}

string getBaseName(string s) {
  char *temp = new char[s.length() + 1];
  strcpy(temp, s.c_str());
  string name = basename(temp);
  // cout << name << endl;
  return name;
}
string getAbsPath(string path, string cwd) {
  // if (path.length() == 1 && path[0] == '.')
  //   return cwd;
  if (path[0] == '/')
    return path;
  if (path[0] == '~') {
    path.erase(path.begin());
    path = root + path;
  }
  else if (path[0] == '.' && (path.length() == 1 || path[1] == '/')) {
    if (cwd[cwd.length() - 1] == '/')
      cwd.pop_back();
    path.erase(path.begin());
    if(path.empty()) path = "/";
    path = cwd + path;
  }
   else if (path[0] == '.' && path[0] == '.') {
    chdir(cwd.c_str());
    char buf[PATH_MAX];
    realpath(path.c_str(), buf);
    // if (buf)
    path = buf;

    //   path.erase(0,2); //delete 2 char from index 0
    //   string fname = getBaseName(path);
    //   path = root + path;
  } 
  else
    path = cwd + "/" + path;
  return path;
}
bool isfile(string path) {
  struct stat info;
  stat(path.c_str(), &info);
  return S_ISREG(info.st_mode);
}
bool isdir(string path) {
  struct stat info;
  stat(path.c_str(), &info);
  return S_ISDIR(info.st_mode);
}
void copy_(string s, string d) {
  if (isfile(s)) {
    ifstream src;
    ofstream dst;
    string dest = d;
    if (d[d.length() - 1] != '/')
      dest += '/';
    string filename = getBaseName(s);
    dest += filename;
    struct stat st = {0};
    // if (stat(d.c_str(), &st) == -1){
    //   mkdir(d.c_str(), 0777);
    //   chown(d.c_str(), st.st_uid, info.st_gid); // copy user,grp
    //   chmod(d.c_str(), st.st_mode);             // copy perm
    // }

    src.open(s, ios::in | ios::binary);
    dst.open(dest, ios::out | ios::binary);
    dst << src.rdbuf();

    struct stat info = {0};
    stat(s.c_str(), &info);
    

    if(ismove) remove(s.c_str());
    src.close();
    dst.close();

    //file perm
    chown(dest.c_str(), info.st_uid, info.st_gid); // copy user,grp
    chmod(dest.c_str(), info.st_mode);             // copy perm

    //folder perm
    // chown(d.c_str(), info.st_uid, info.st_gid); // copy user,grp
    // chmod(d.c_str(), info.st_mode);             // copy perm

    return;
  } else {
    DIR *folder;
    folder = opendir(s.c_str());
    if (folder) {
      if (isdir(s)) {
        string foldername = getBaseName(s);
        // string d = d;
        if (d[d.length() - 1] != '/')
          d += '/';
        d += foldername;
        
        struct stat st = {0};
        struct stat st_src = {0};
        stat(s.c_str(),&st_src);

        if (stat(d.c_str(), &st) == -1){
          mkdir(d.c_str(), 0777);
          chown(d.c_str(), st_src.st_uid, st_src.st_gid); // copy user,grp
          chmod(d.c_str(), st_src.st_mode);             // copy perm
        }
      }
      int dirfd = open(s.c_str(), O_DIRECTORY);

      struct dirent *dirEntry;

      while ((dirEntry = readdir(folder))) {
        struct stat info;

        fstatat(dirfd, dirEntry->d_name, &info, 0);
        ostringstream os;
        if (dirEntry->d_name[0] == '.')
          continue;

        string nsrc = s;
        if (nsrc[nsrc.length() - 1] != '/')
          nsrc += '/';
        nsrc += dirEntry->d_name;
        copy_(nsrc, d);       
        if(ismove) remove(nsrc.c_str());
      }
    }
  }
}
void copy(string cwd) {
  ismove = false;
  destination = getAbsPath(destination, cwd);
  for (auto &i : srcs) {
    i = getAbsPath(i, cwd);
    struct stat st = {0};
    if (stat(i.c_str(), &st) == -1){
      manualerrmsg += i;
      manualerrmsg += ": not found!\r\n";
    }
    else
      copy_(i, destination);
    errno=0;
  }
}

void move(string cwd) {
  ismove = true;
  destination = getAbsPath(destination, cwd);
  for (auto &i : srcs) {
    i = getAbsPath(i, cwd);
    struct stat st = {0};
    if (stat(i.c_str(), &st) == -1){
      manualerrmsg += i;
      manualerrmsg += ": not found!\r\n";
    }
    else{
      copy_(i, destination);
      remove(i.c_str());
    }
    errno=0;
  }
}

void create_file(string cwd) {
  destination = getAbsPath(destination, cwd);
   if (destination[destination.length() - 1] != '/')
          destination += '/';
  destination += srcs[0];
    open(destination.c_str(), O_CREAT | O_WRONLY , 0777);
}
void create_dir(string cwd) {
  destination = getAbsPath(destination, cwd);
  if (destination[destination.length() - 1] != '/')
          destination += '/';
  destination += srcs[0];
  struct stat st = {0};
  // if (stat(destination.c_str(), &st) == -1)
  mkdir(destination.c_str(), 0777);
  // if(errno)
  //   perror("create_dir");
}
void rename(string cwd) {
  string src = getAbsPath(srcs[0], cwd);
  destination = getAbsPath(destination, cwd);
  rename(src.c_str(), destination.c_str());
}

void remove_(string d) {
  if (isfile(d)) {
    remove(d.c_str());
    return;
  } else {
    DIR *folder;
    folder = opendir(d.c_str());
    if (folder) {
      int dirfd = open(d.c_str(), O_DIRECTORY);
      struct dirent *dirEntry;
      while ((dirEntry = readdir(folder))) {
        // fstatat(dirfd, dirEntry->d_name, &info, 0);
        if (dirEntry->d_name[0] == '.')
          continue;
        string ndest = d;
        if (ndest[ndest.length() - 1] != '/')
          ndest += '/';
        ndest += dirEntry->d_name;
        remove_(ndest);
      }
      // rmdir(d.c_str());
      remove(d.c_str());
    }
  }
}

void delete_dir(string cwd) {
  string d = getAbsPath(destination, cwd);
  opendir(d.c_str());
  if(isdir(d)){
    remove_(d);
    errno = 0;
  }
}

void delete_file(string cwd) {
  string d = getAbsPath(destination, cwd);
  manualerrmsg = "";
  if(!isfile(d)){
    manualerrmsg = "Is a directory";
  }
  else remove(d.c_str());
}

//search using BFS
queue<string> q;
bool search(string cwd) {
  DIR *folder;
  folder = opendir(cwd.c_str());
  if (folder) {
    struct dirent *dirEntry;
    while ((dirEntry = readdir(folder))) {
      if (dirEntry->d_name[0] == '.')
        continue;
      if (dirEntry->d_name == destination)
        return true;
      string ncwd = cwd;
        if (ncwd[ncwd.length() - 1] != '/')
          ncwd += '/';
        ncwd += dirEntry->d_name;
      // cout << ncwd << endl;
      
      if(isdir(ncwd))
        q.push(ncwd);
    }
    closedir(folder);
    while(!q.empty()){
      string temp = q.front();
      q.pop();
      return search(temp);
    }
  }
  // else return false;
}

string Goto(string cwd,string root){
  string path = getAbsPath(destination,cwd);
  struct stat st = {0};
  int fd = stat(path.c_str(), &st);
  if(fd != -1) {
    if(path.length() < root.length()){
      errno=0;
      manualerrmsg = "Restricted area!";
      path = cwd;
    }
  }
  else path = cwd;
  return path;
}
// int main() {
//   string cwd = "/home/hari/testing/aos/A1/testing/1";

//   string s = "create_dir test12 ";
//   // string s;
//   // cin >> s;
//   // cout << getAbsPath(s,cwd);
//   // cout << s << endl;
//   parsecmd(s);
//   create_dir(cwd);
// }
