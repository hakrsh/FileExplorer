#include "header.h"

using namespace std;

int userlen = INT_MIN;
int grplen = INT_MIN;
int sizelen = INT_MIN;

int cursorpos;

string toString(float f) {
  ostringstream os;
  os << fixed << setprecision(1) << f;
  return os.str();
}
string getUser(uid_t uid) {
  struct passwd *userDetails;
  userDetails = getpwuid(uid);
  if (!userDetails)
    return "error";

  string usr = userDetails->pw_name;
  return usr;
}
string getGroup(gid_t gid) {
  struct group *groupDetails;
  groupDetails = getgrgid(gid);
  if (!groupDetails)
    return "error";
  string grp = groupDetails->gr_name;
  return grp;
}

bool isDir(string path) {
  struct stat info;
  stat(path.c_str(), &info);
  return S_ISDIR(info.st_mode);
}

bool isFile(string path) {
  struct stat info;
  stat(path.c_str(), &info);
  return S_ISREG(info.st_mode);
}
string getPerm(mode_t mode) {
  string perm;
  S_ISDIR(mode) ? perm += "d" : perm += "-";
  mode &S_IRUSR ? perm += "r" : perm += "-";
  mode &S_IWUSR ? perm += "w" : perm += "-";
  mode &S_IXUSR ? perm += "x" : perm += "-";
  mode &S_IRGRP ? perm += "r" : perm += "-";
  mode &S_IWGRP ? perm += "w" : perm += "-";
  mode &S_IXGRP ? perm += "x" : perm += "-";
  mode &S_IROTH ? perm += "r" : perm += "-";
  mode &S_IWOTH ? perm += "w" : perm += "-";
  mode &S_IXOTH ? perm += "x" : perm += "-";
  return perm;
}
string getSize(off_t size_) {
  unsigned long long size = size_;
  long kb, mb, gb;
  kb = 1024;
  mb = 1024 * kb;
  gb = 1024 * mb;
  string s;
  if (size / gb) {
    s += toString(size / float(gb));
    s += "G";
  } else if (size / mb) {
    s += toString(size / float(mb));
    s += "M";
  } else if (size / kb) {
    s += toString(size / float(kb));
    s += "K";
  } else {
    s += to_string(size);
    s += "B";
  }
  return s;
}

string getTime(struct stat info) {
  struct tm *dt;
  time_t mtime = info.st_mtim.tv_sec;
  dt = localtime(&mtime);
  string ltime;
  if(dt){
    char *buf = new char[20];
    vector<const char*> mon{"Jan","Feb","Mar","Apr","May","Jun","Jul","Sep","Oct","Nov","Dec"};
    sprintf(buf, "%s %02d %02d:%02d",mon[dt->tm_mon],dt->tm_mday, dt->tm_hour, dt->tm_min);
    return string(buf);
  }
  return "error";
}


vector<string> dirContents;
void listDir(string dirName) {
  dirContents.clear();
  DIR *folder;
  folder = opendir(dirName.c_str());
  if(folder){
  int dirfd = open(dirName.c_str(), O_DIRECTORY);
  struct dirent *dirEntry;
  vector<vector<string>>temp;
  
  while ((dirEntry = readdir(folder)) != nullptr) {
    struct stat info;

    fstatat(dirfd, dirEntry->d_name, &info, 0);
    if (dirEntry->d_name[0] == '.' && dirEntry->d_name[1] && dirEntry->d_name[1] != '.')
      continue;
    
    string usr = getUser(info.st_uid);
    string grp = getGroup(info.st_gid);
    string sizefield = getSize(info.st_size);
    temp.push_back({getPerm(info.st_mode),usr,grp,sizefield,getTime(info),dirEntry->d_name});
    userlen = max(userlen,int(usr.length()));
    grplen = max(grplen,int(grp.length()));
    sizelen = max(sizelen,int(sizefield.length()));

  }
  closedir(folder);
  for(auto i : temp){
    ostringstream os;
    os << i[0] << " ";
    os << setw(userlen) << i[1] << " ";
    os << setw(grplen) << i[2] << " ";
    os << setw(sizelen) << i[3] << " ";
    os << setw(TIMELEN) << i[4] << " ";
    os << i[5];
    dirContents.push_back(os.str());
  }
  cursorpos = PERMLEN + userlen + grplen + sizelen + TIMELEN + GAP;
}
}

//split the string last field is the path
string getPath(int i,int off) {
  stringstream os(dirContents[off+i-1]);
  string temp;
  string path;
  int k = 1;
  while (os >> temp) {
    if (k == 8)
      path = temp;
    k++;
  }
  return path;
}

string getHistory() {
  string temp = "/";
  for (int i = 0; i < historyPtr; i++) {
    temp = temp + history[i];
    if (i < historyPtr - 1)
      temp += "/";
  }
  return temp;
}
// int main(){
//     char dirName[100];
//     // cin >> dirName;
//     // if(!getcwd(dirName,sizeof(dirName))){
//     //     perror("getcwd");
//     //     return 1;
//     // }
//     listDir(dirName);
//     cout << getPath(1);
//     DIR *folder;
//     folder = opendir(getPath(1).c_str());
//     cout << folder->d_name;

// //     for(auto i : list(dirName))
// //         cout << i << endl;

// }