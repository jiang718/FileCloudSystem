#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime> 

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <fstream> 
#include <sstream>
#include <list>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm> 

#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <chrono>
#include <random>

#include <type_traits>
#include <typeinfo>

using namespace std;

#define ll long long

#define MODE_ZERO 0
#define MODE_MIN 1
#define MODE_RANDOM 2
#define MODE_MAX 3

#define TIME_OUT_MAX_DEFAULT 3 

#define TYPE_PRIM 0
#define TYPE_VEC 1
#define TYPE_VEC_PAIR 2

#define TYPE_PRIM_INT 0
#define TYPE_PRIM_BOOL 1
#define TYPE_PRIM_DOUBLE 2
#define TYPE_PRIM_STR 3

#define TYPE_VEC_SINGLE 0
#define TYPE_VEC_MUL 1

#define OK 0 
#define FAIL -1
#define NONE -1

#define SERVER_PORT 33223

#define REQUEST_PIN 0
#define REQUEST_REGISTER 1
#define REQUEST_SHOW_FILE 2
#define REQUEST_FIND 3
#define REQUEST_GET_LOAD 4
#define REQUEST_DOWNLOAD 5
#define REQUEST_UPDATE_LIST 6
#define REQUEST_GET_GLOBAL_INFO 7

#define IP_LEN_MAX 20 
#define FILE_NAME_LEN_MAX 20
#define FILE_CONTENT_LEN_MAX 1000 
#define MSG_LEN_MAX 10000000

/******************Global Useful Functions*************/
class Format {
public:
    static int toInt(const char * s) {return atoi(s);}
    static bool toBool(const char *s) { return s[0] == 't' || s[0] == 'T' || s[0] == 1;}
    static double toDouble(const char * s) { return atof(s); }
    static string toStr(const char * s) { string str(s); return str;}

    static int toInt(string s) { return toInt(s.c_str());}
    static bool toBool(string s) { return toBool(s.c_str());}
    static double toDouble(string s) { return toDouble(s.c_str());}

    static char * toCStr(int x) { return Format::toCStr(toStr(x));}
    static char * toCStr(bool x) { return Format::toCStr(toStr(x));}
    static char * toCStr(double x) { return Format::toCStr(toStr(x));}
    static char * toCStr(string str) {char*s=new char[str.length() + 1];strcpy(s,str.c_str());return s;}
    static string toStr(int x) {return Format::toStr(double(x));}
    static string toStr(bool x) {stringstream ss;ss<<(x ? "true":"false");return ss.str();}
    static string toStr(double x) {stringstream ss;ss << x;return ss.str();}
    static string toStr(string s) {return s;}

    static bool isNum(char x) {
        return x >= '0' && x <= '9';
    }
    static bool isNum(char *s) {
        return isNum(toStr(s));
    }
    static bool isNum(string s) {
        if (s.length() > 1 && s[0] == '0') return false;
        for (int i = 0; i < s.length(); i++) {
            if (!isNum(s[i])) return false;
        }
        return true;
    }
};
class Time {
public:
    static chrono::time_point<std::chrono::high_resolution_clock> now() {
        return chrono::high_resolution_clock::now(); 
    }
    static void sleep(double s) {
        this_thread::sleep_for(std::chrono::milliseconds((int)s * 1000));
    }
    static void show(std::chrono::duration<double> duration) {
        cout << "Duration: " << duration.count() * 1000 << " ms\n" << endl;
    }
    static double getMilliDuration(std::chrono::duration<double> duration) {
        return duration.count() * 1000;
    }
};
class Random {
public:
    static int randInt(int upper) {
        return randInt(0, upper);
    }
    static int randInt(int lower, int upper) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> unif(lower, upper);
        return unif(gen); 
    }
    static bool randBool() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> unif(0, 1);
        return unif(gen);
    }
    static bool randBool(double rate_true) {
        double x = randDouble(1);
        if (x < rate_true) {
            return true;
        }
        return false;
    }
    static double randDouble(double lower, double upper) {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> unif(lower, upper);
        return unif(gen); 
    }
    static double randDouble(double upper) {
        return randDouble(0, upper);
    }
};

static bool endsWith(const string &s, const string &ending) {
    if (s.size() < ending.size()) return false;
    bool res = std::equal(ending.rbegin(), ending.rend(), s.rbegin()); 
    return res;
}
static bool addNoise(char *msg, int msg_len, double error_rate) {
    bool happen = Random::randBool(error_rate);
    if (!happen) {
        //cout << "not adding noise!" << endl;
        return false;
    }
    int pos = Random::randInt(0, msg_len-1); 
    cout << "random choose position : " << pos << " | ";
    msg[pos] = ~msg[pos]; 
    cout << "shift digit: " << pos << endl;
    return true;
}

//generate lantency data
class DataManager {
public:
    static string getTxtName(int n) {
        return Format::toStr(n) + ".txt";
    }
    static int storeFile(int id, string filename, string filebody) {
        stringstream path, command;
        path << "./" << id << "/" << filename;
        //cout << "store file " << filename << "(" << filebody << ")" <<endl;
        ofstream save(path.str());
        if (save.is_open()) {
            save << filebody;
            save.flush();
            save.close();
            return OK;
        }
        return FAIL;
    }
    static void removeFolderContent(int id) {
        struct dirent *ptr;
        struct stat filestat;
        DIR *dir;
        stringstream ss;
        ss << "./" << id;
        string path = ss.str();
        vector<pair<string, string>> filelist;
        vector<string> filename_list;
        if ((dir = opendir(path.c_str())) == NULL) {
            return;
        }
        while ((ptr=readdir(dir)) != NULL) {
            string filepath = path + "/" + ptr->d_name;
            if (stat(filepath.c_str(), &filestat)) continue; //directory
            if (S_ISDIR(filestat.st_mode)) continue;
            if (ptr->d_name == "."|| ptr->d_name == "..") continue;
            string file_path = path + "/" + ptr->d_name;
            ofstream fp(file_path);
            if (fp.is_open()) {
                fp.close();
            }
            if (remove(file_path.c_str()) != 0) {
                string er_msg = "Error deleting file: " + file_path;
                perror(er_msg.c_str());
            }
        }
        closedir(dir);
    }
    static void generateFolder(int n, int count) {
        string command = "mkdir -p " + Format::toStr(n);
        string filename, filebody;
        system(command.c_str());
        if (count == 0) {
        } else if (count == 1) {
            filename = getTxtName(n);
            filebody = Format::toStr(n);
            storeFile(n, filename, filebody);
        } else  {
            for (int i = 1; i <= count; i++) {
                filename = getTxtName(i);
                filebody = Format::toStr(i);
                storeFile(n, filename, filebody);
            }
        }
    }
    static void generateFolderList(int n, int count) {
        for (int i = 1; i<=n; i++) {
            removeFolderContent(i);
            generateFolder(i, count);
        }
    }
    static void removeFolder(int id) {
        removeFolderContent(id);
        stringstream ss;
        ss << "./" << id;
        string path = ss.str();
        remove(path.c_str());
    }
    static void removeFolderList() {
        struct dirent *ptr;
        struct stat filestat;
        DIR *dir;
        stringstream ss;
        string path = "./";
        vector<pair<string, string>> filelist;
        vector<string> filename_list;
        if ((dir = opendir(path.c_str())) == NULL) {
            return;
        }
        while ((ptr=readdir(dir)) != NULL) {
            string filepath = path + "/" + ptr->d_name;
            if (S_ISDIR(filestat.st_mode)) continue;
            if (endsWith(filepath, "c") || endsWith(filepath, "cpp")) continue;
            if (ptr->d_name == "."|| ptr->d_name == ".." || ptr->d_name == "filesys") continue;
            cout << ptr->d_name << endl;
            if (Format::isNum(ptr->d_name)) {
                cout << "remove " << ptr->d_name << endl;
                removeFolder(Format::toInt(ptr->d_name));
            }
        }
        closedir(dir);
    }
    static void generateLatency(int n, int mode) {
        vector<vector<int>> latencies(n, vector<int>(n, 0));
        string s = "";
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                int time = 0;
                switch (mode) {
                    case MODE_ZERO:
                        s = "ZERO";
                        time = 0;
                        break;
                    case MODE_RANDOM:
                        s = "RANDOM";
                        time = Random::randInt(100, 5000);
                        break;
                    case MODE_MIN:
                        s = "MIN";
                        time = 100;
                        break;
                    case MODE_MAX:
                        s = "MAX";
                        time = 5000;
                        break;
                    default:
                        time = -1;
                        break;
                }
                latencies[i][j] = latencies[j][i] = time;
            }
        }
        stringstream ss;
        ss << "latency-table-" << n << "-" << s << ".txt";
        ofstream data(ss.str());
        if (data.is_open()) {
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    data << (i+1) << " " << (j+1) << " " << latencies[i][j] << endl;
                    //cout << (i+1) << " " << (j+1) << " " << latencies[i][j] << endl;
                }
            }
            data.close();
        } else {
            cout << "can't open data" << endl;
        }
    }
};




class SendableData {
private:
    int type = -1, type_prim = -1, type_vec = -1;
    int level = 0, count = 1;
    int length = 0, size = 0;
    int data_int; double data_double; bool data_bool; string data_str;
    vector<SendableData> data_vec;
    vector<pair<SendableData, SendableData>> data_vec_pair;
public:
    //static bits to store information in head
    static const ll kChecksumPrime = 7199369;
    static const int kByteHeader=1; //type+primitive_type+vector_type or boolData
    static const int kByteInt=4, kByteDouble=8, kByteBool=0;
    static const int kByteMeta=kByteInt*6+IP_LEN_MAX;//meta information for sendabledata(sub sendabledata->no meta)
    static const int kSizeMax = 100000000;
    static const int kStepLevel = 5; //spaces for each level

    static int calSize(int type, int type_prim, int length) {
        if (type == TYPE_PRIM && type_prim != TYPE_PRIM_STR) {
            return length + kByteHeader+ kByteInt;
        }
        return length + kByteHeader+ kByteInt*2;
    }
    SendableData() {
        SendableData(0,0);
    } 
    SendableData(int x, int level_t) {
        type = TYPE_PRIM;
        type_prim = TYPE_PRIM_INT;
        type_vec = NONE;
        level = level_t;
        count = 1;
        length = kByteInt;
        size = calSize(type, type_prim, length);
        data_int = x;
    }
    SendableData(bool x, int level_t) {
        type = TYPE_PRIM; type_prim = TYPE_PRIM_BOOL;
        type_vec = NONE;
        level = level_t;
        count = 1;
        length = kByteBool;
        size = calSize(type, type_prim, length);
        data_bool = x;
    }
    SendableData(double x, int level_t) {
        type = TYPE_PRIM; type_prim= TYPE_PRIM_DOUBLE;
        type_vec = NONE;
        level = level_t;
        count = 1;
        length = kByteDouble;
        size = calSize(type, type_prim, length);
        data_double = x;
    }
    SendableData(string x, int length_t, int level_t) {
        type = TYPE_PRIM; type_prim = TYPE_PRIM_STR;
        type_vec = NONE;
        level = level_t;
        count = 1;
        length = length_t;
        size = calSize(type, type_prim, length);
        data_str = x;
    }
    void init(int type_t, int type_prim_t, int type_vec_t, int level_t, int count_t, vector<SendableData>& data_vec_t) {
        type = type_t;
        type_prim = type_prim_t;
        type_vec = type_vec_t;
        level = level_t;
        count = count_t;
        length = 0;
        for (int i = 0; i < count; i++) {
            data_vec.push_back(data_vec_t[i]);
            length += data_vec_t[i].getSize();
        }
        size = calSize(type, type_prim, length);
    }
    void init(int type_t, int type_prim_t, int type_vec_t,int level_t, int count_t, vector<pair<SendableData, SendableData>> & data_vec_pair_t) {
        type = type_t;
        type_prim = type_prim_t;
        type_vec = type_vec_t;
        level = level_t;
        count = count_t;
        length = 0;
        for (int i = 0; i < count; i++) {
            data_vec_pair.push_back(data_vec_pair_t[i]);
            length += data_vec_pair_t[i].first.getSize() + data_vec_pair_t[i].second.getSize();
        }
        size = calSize(type, type_prim, length);
    }
    SendableData(vector<int> &a, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], level_t));
        }
        init(TYPE_VEC,NONE,0,level_t,data_vec_t.size(),data_vec_t); 
    }
    SendableData(vector<bool> &a, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], level_t));
        }
        init(TYPE_VEC,NONE,0,level_t, data_vec_t.size(), data_vec_t); 
    }
    SendableData(vector<double> &a, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], level_t));
        }
        init(TYPE_VEC,NONE,0,level_t, data_vec_t.size(), data_vec_t); 
    }
    SendableData(vector<string> &a, int length_t, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], length_t, level_t));
        }
        init(TYPE_VEC,NONE,0,level_t, data_vec_t.size(), data_vec_t); 
    }
    //useful for (ip, port) pair -> turns into vector
    SendableData(pair<string, int> &a, int length_t, int level_t) {
        vector<SendableData> data_vec_t;
        data_vec_t.push_back(SendableData(a.first, length_t, level_t));
        data_vec_t.push_back(SendableData(a.second, level_t));
        init(TYPE_VEC,NONE,0,level_t, data_vec_t.size(), data_vec_t); 
    }
    SendableData(vector<pair<string, int>> &a, int length_t, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], length_t, 0));
        }
        init(TYPE_VEC,NONE,1,level_t,data_vec_t.size(), data_vec_t); 
    }
    SendableData(pair<string, string> &a, int length_t_1, int length_t_2, int level_t) {
        vector<SendableData> data_vec_t;
        data_vec_t.push_back(SendableData(a.first, length_t_1, level_t));
        data_vec_t.push_back(SendableData(a.second, length_t_2,level_t));
        init(TYPE_VEC,NONE,0,level_t, data_vec_t.size(), data_vec_t); 
    }
    SendableData(vector<pair<string, string>> &a, int length_t_1, int length_t_2, int level_t) {
        vector<SendableData> data_vec_t;
        for (int i = 0; i < a.size(); i++) {
            data_vec_t.push_back(SendableData(a[i], length_t_1, length_t_2, 0));
        }
        init(TYPE_VEC,NONE,1,level_t,data_vec_t.size(), data_vec_t); 
    }
    SendableData(vector<SendableData> &x, int type_vec_t, int level_t) {
        init(TYPE_VEC, NONE, type_vec_t, level_t, x.size(), x); 
    }
    SendableData(vector<pair<SendableData, SendableData>> data_vec_pair_t, int type_vec_t, int level_t) {
        init(TYPE_VEC_PAIR,NONE,type_vec_t,level_t,data_vec_pair_t.size(),data_vec_pair_t); 
    }
    int getType() { return type; }
    int getTypePrim() { return type_prim; }
    int getTypeVec() { return type_vec; }
    int getLevel() { return level; }
    int getCount() { return count; }
    int getLength() { return length; }
    int getSize() { return size; }
    int getDataInt() {
        if (type!=TYPE_PRIM||type_prim!=TYPE_PRIM_INT) {perror("Wrong get int"); return -1;}
        return data_int;
    }
    bool getDataBool() {
        if (type!=TYPE_PRIM||type_prim!=TYPE_PRIM_BOOL){perror("Wrong get bool");return 0;} 
        return data_bool;
    }
    double getDataDouble() {
        if (type!=TYPE_PRIM||type_prim!=TYPE_PRIM_DOUBLE) {perror("Wrong get db"); return -1;}
        return data_double;
    }
    string getDataStr() {
        if (type!=TYPE_PRIM||type_prim!=TYPE_PRIM_STR) {perror("Wrong get str"); return "";}
        return data_str;
    }
    SendableData & getItem(int x) {
        if (type!=TYPE_VEC||x<0||x>=count) {perror("Wrong getItem()");return data_vec[0];}
        return data_vec[x];
    }
    pair<SendableData,SendableData> & getItemPair(int x) {
    if (type!=TYPE_VEC_PAIR||x<0||x>=count) {perror("Wrong get pair");return data_vec_pair[0];}
        return data_vec_pair[x];
    }
    vector<SendableData> & getDataVec() {
        if (type!=TYPE_VEC) {perror("Wrong get vector");return data_vec;}
        return data_vec;
    }
    vector<pair<SendableData, SendableData>> & getDataVecPair() {
        if (type!=TYPE_VEC_PAIR) {perror("Wrong get vector of pair");return data_vec_pair;}
        return data_vec_pair;
    }
    vector<pair<string,string>> toVecPairStrStr() {
        vector<pair<string,string>> res;
        if (type != TYPE_VEC) {
            perror("wrong converting!");
            return res;
        }
        for (int i = 0; i < count; i++) {
            SendableData now = getItem(i);
            res.push_back(now.toPairStrStr());
        }
        return res;
    }
    pair<string,string> toPairStrStr() {
        pair<string,string> res("","");
        if (type != TYPE_VEC || count != 2) {
            perror("wrong converting!");
            return res;
        }
        SendableData x = getItem(0);
        SendableData y = getItem(1);
        if (x.getType() != TYPE_PRIM && x.getTypePrim() != TYPE_PRIM_STR) {
            return res;
        }
        if (y.getType() != TYPE_PRIM && y.getTypePrim() != TYPE_PRIM_STR) {
            return res;
        }
        return make_pair(x.getDataStr(), y.getDataStr()); 
    }
    vector<pair<string,int>> toVecPairStrInt() {
        vector<pair<string,int>> res;
        if (type != TYPE_VEC) {
            perror("wrong converting!");
            return res;
        }
        for (int i = 0; i < count; i++) {
            SendableData now = getItem(i);
            res.push_back(now.toPairStrInt());
        }
        return res;
    }
    pair<string,int> toPairStrInt() {
        pair<string,int> res("",-1);
        if (type != TYPE_VEC || count != 2) {
            perror("wrong converting!");
            return res;
        }
        SendableData x = getItem(0);
        SendableData y = getItem(1);
        if (x.getType() != TYPE_PRIM && x.getTypePrim() != TYPE_PRIM_STR) {
            return res;
        }
        if (y.getType() != TYPE_PRIM && y.getTypePrim() != TYPE_PRIM_INT) {
            return res;
        }
        return make_pair(x.getDataStr(), y.getDataInt()); 
    }
    vector<string> toVecStr() {
        vector<string> res;
        if (type != TYPE_VEC) {
            perror("wrong converting!");
            return res;
        }
        for (int i = 0; i < count; i++) {
            SendableData now = getItem(i);
            if (now.getType() != TYPE_PRIM || now.getTypePrim() != TYPE_PRIM_STR) {
                perror("wrong converting!");
                return res;
            }
            res.push_back(now.getDataStr());
        }
        return res;
    }
    vector<int> toVecInt() {
        vector<int> res;
        //cout << "into 0" << endl;
        //cout << "type: " << type << endl;
        if (type != TYPE_VEC) {
            perror("wrong converting!");
            return res;
        }
        for (int i = 0; i < count; i++) {
            SendableData now = getItem(i);
            if (now.getType() != TYPE_PRIM || now.getTypePrim() != TYPE_PRIM_INT) {
                perror("wrong converting!");
                return res;
            }
            res.push_back(now.getDataInt());
        }
        return res;
    }
    void printHeader() {
        for (int i = 0; i < kStepLevel* level; i++) { cout << " "; }
    }
    void print(int header_len, bool ifEndline) {
        printHeader();
        switch (type) {
            case NONE:
                //cout << "Content is empty!" << endl;
                break;
            case TYPE_PRIM: //simple
                switch (type_prim) {
                    case TYPE_PRIM_INT: //int
                        cout << data_int; if (ifEndline) cout << endl;
                        break;
                    case TYPE_PRIM_BOOL: //bool
                        cout << (data_bool?"true" : "false"); if (ifEndline) cout<<endl;
                        break;
                    case TYPE_PRIM_DOUBLE: //double
                        cout << data_double; if (ifEndline) cout<<endl;
                        break;
                    case TYPE_PRIM_STR: //string
                        cout << data_str; if(ifEndline) cout<<endl;
                        break;
                    default:break;
                }
                break;
            case TYPE_VEC: //normal vector
                for (int i = 0; i < data_vec.size(); i++) {
                    data_vec[i].print(header_len, type_vec); 
                    if (type_vec==TYPE_VEC_SINGLE && i<data_vec.size()-1) {cout << " ";}
                }
                if (ifEndline) cout << endl;
                break;
            case TYPE_VEC_PAIR:
                for (int i = 0; i < data_vec_pair.size(); i++) {
                    data_vec_pair[i].first.print(header_len, type_vec); cout << "->";
                    data_vec_pair[i].second.print(header_len, true);
                }
                if (ifEndline) cout << endl;
                break;
            default: break;
        }
    }
    void print() {
        print(0, true);
    }
    void printArgument() {
        cout << "Type: " << getType() << endl;
        cout << "Prim Type: " << getTypePrim() << endl;
        cout << "Vec Type: " << getTypeVec() << endl;
        cout << "Level: " << getLevel() << endl;
        cout << "Count: " << getCount() << endl;
        cout << "Length: " << getLength() << endl;
        cout << "Size: " << getSize() << endl;
    }

    static void printMsg(char *a, int len) {
        for (int i = 0; i < len; i++) {
            cout << i << ":" << (int)a[i] << " ";
        }
        cout << endl;
    }

    static void printBinary(int x) {
        for (int i = 31; i >= 0; i--) {
            cout << ((x >> i) & 1) << (i % 8 ? "":" ");
        }
        cout << endl;
    }
    static void printBinary(char x) {
        for (int i = 7; i >= 0; i--) {
            cout << ((x >> i) & 1);
        }
        cout << endl;
    }
    //generation encoding for int/bool/double/string
    static void setInt(char *a, int x) {
        for (int i = 0; i < kByteInt; i++) { 
            int mask = (1<<8)-1;//11111111
            a[i] = (x & (mask<<(i*8))) >> (i*8);
        }
    }
    static int getInt(char *a) {
        int ans = 0;
        try {
            for (int i = 0; i < kByteInt; i++) {
                int mask = (1<<8)-1;
                ans |= ((a[i] & mask) << (i*8));
            }
        } catch (exception e) {
            perror("wrong get int: message size is too long.");
        }
        return ans;
    }
    static void setBool(char *a,bool x) {
        a[0] = a[0] | (((int)x) & 1);
    } //_ _ _ _ _ _ _ *
    int getBool(char *a) {
        return a[0] & 1;
    }
    static void setDouble(char *a, double x) {
        char * b = reinterpret_cast<char*>(&x);
        for (int i = 0; i < kByteDouble; i++) {
            a[i] = b[i];
        }
    }
    static double getDouble(char *a) {
        double ans = *(reinterpret_cast<double*>(a));
        return ans;
    }
    static void setStr(char *a, string s, int length) {
        if (s.length() >= length) {
            perror("the length for string is to large.");
            return;
        }
        for (int i = 0; i < s.length(); i++) {
            a[i] = s[i];
        }
        a[s.length()] = '\0';
    }
    static string getStr(char *a, int length) {
        string s = "";
        for (int i = 0; a[i] != '\0'; i++) {
            if (a[i] != '\0') s.push_back(a[i]);
        }
        return s;
    }

    void setMsgType(char *a) {int mask = (1<<3)-1;a[0] = (type&mask)<<4;} //_ * * * _ _ _ _
    int getMsgType(char *a) { return (a[0] >> 4) & ((1<<3)-1);}
    void setMsgTypePrim(char *a){a[0]=a[0]|((type_prim&3)<<1);} //_ _ _ _ _ * * _
    int getMsgTypePrim(char *a) {return (a[0] >> 1) & ((1<<2)-1);} 
    void setMsgTypeVec(char *a) { a[0] = a[0] | (type_vec& 1);} //_ _ _ _ _ _ _ *
    int getMsgTypeVec(char *a) {return a[0] & 1; }
    void setMsgLevel(char *a) {setInt(a+kByteHeader, level);}
    int getMsgLevel(char *a) {return getInt(a+kByteHeader);}
    void setMsgCount(char *a) {setInt(a+kByteHeader+kByteInt,count);}
    int getMsgCount(char *a) {return getInt(a+kByteHeader+kByteInt);}
    void setMsgLength(char *a) {setInt(a+kByteHeader+kByteInt,length);}
    int getMsgLength(char *a) { return getInt(a+kByteHeader+kByteInt);}
    void setMsgDataInt(char *a) { setInt(a+kByteHeader+kByteInt,data_int);}
    int getMsgDataInt(char *a) { return getInt(a+kByteHeader+kByteInt);}
    void setMsgDataBool(char *a) {setBool(a, data_bool);} //_ _ _ _ _ _ _ *
    bool getMsgDataBool(char *a) {return getBool(a);}
    void setMsgDataDouble(char *a) { setDouble(a+kByteHeader+kByteInt,data_double);}
    double getMsgDataDouble(char *a) { return getDouble(a+kByteHeader+kByteInt);}
    void setMsgDataStr(char *a) { setStr(a+kByteHeader+kByteInt*2,data_str,length);}
    string getMsgDataStr(char *a) {return getStr(a+kByteHeader+kByteInt*2,length);}
    void setMsgDataVec(char * a) {
        int header_len = kByteHeader+kByteInt*2;//9
        int shift = 0;
        for (int i = 0; i < count; i++) {
            data_vec[i].encode(a+shift+header_len);
            shift += data_vec[i].getSize();
        }
    }
    void setMsgDataVecPair(char * a) {
        int header_len = kByteHeader+kByteInt*2;//9
        int shift = 0;
        for (int i = 0; i < count; i++) {
            data_vec_pair[i].first.encode(a+shift+header_len);
            shift += data_vec_pair[i].first.getSize();
            data_vec_pair[i].second.encode(a+shift+header_len);
            shift += data_vec_pair[i].second.getSize();
        }
    }

    //recursive clear()
    void clear() {
        clear(true);
    }
    void clear(bool ifKeepLength) {
        if (type == TYPE_VEC) {
            for (int i = 0; i < count; i++) {
                data_vec[i].clear();
            }
        } else if (type == TYPE_VEC_PAIR) {
            for (int i = 0; i < count; i++) {
                data_vec_pair[i].first.clear();
                data_vec_pair[i].second.clear();
            }
        }
        type = type_prim = type_vec=NONE;
        data_int=data_double= -1; data_bool= false; data_str= "";
        level=0;count=1;size=0;
        length= ifKeepLength ? length: 0;
        data_vec.clear();
        data_vec_pair.clear();
    }

    SendableData copy() {
        switch (type) {
            case TYPE_PRIM:
                switch (type_prim) {
                    case TYPE_PRIM_INT: return SendableData(data_int, level); break;
                    case TYPE_PRIM_BOOL: return SendableData(data_bool, level); break;
                    case TYPE_PRIM_DOUBLE: return SendableData(data_double, level);break;
                    case TYPE_PRIM_STR: return SendableData(data_str, length, level); break;
                    default: break; }
                break;
            case TYPE_VEC:
                return SendableData(data_vec,type_vec,level);break;
            case TYPE_VEC_PAIR:
                return SendableData(data_vec_pair,type_vec,level);break;
            default:break;
        }
        return SendableData(0,0);
    }
    bool equal(SendableData& data) {
        bool res = true;
        res = res && (type == data.getType()); 
        if (!res) cout << "type:" << type << " " << data.getType() << endl;
        res == res && (type_prim == data.getTypePrim()); 
        if (!res) cout << "prim type:" << type_prim << " " << data.getTypePrim() << endl;
        res = res && (type_vec == data.getTypeVec());
        if (!res) cout << "vector type:" << type_vec << " " << data.getTypeVec() << endl;
        res = res && (level == data.getLevel());
        if (!res) cout << "level:" <<level << " " << data.getLevel() << endl;
        res = res && (count == data.getCount());
        if (!res) cout << "count:" <<count<< " " << data.getCount() << endl;
        res = res && (length == data.getLength());
        if (!res) cout << "length:" <<length<< " " << data.getLength() << endl;
        res = res && (size == data.getSize());
        if (!res) cout << "size:" <<size<< " " << data.getSize() << endl;
        if (!res) return false;
        switch (type) {
            case TYPE_PRIM:
                switch (type_prim) {
                    case TYPE_PRIM_INT:res=res && (getDataInt() == data.getDataInt()); break;
                    case TYPE_PRIM_BOOL:res=res && (getDataBool() == data.getDataBool()); break;
                    case TYPE_PRIM_DOUBLE:res=res&&(getDataDouble()==data.getDataDouble());break;
                    case TYPE_PRIM_STR:res = res && (getDataStr() == data.getDataStr());break;
                    default: break;
                }
                break;
            case TYPE_VEC:
                res = res && (count == data_vec.size());
                res = res && (count == data.getDataVec().size());
                if (!res) return false;
                for (int i = 0; i < count; i++) {
                    res = res && (data_vec[i].equal(data.getItem(i)));
                }
                break;
            case 2:
                res = res && (count == data_vec_pair.size());
                res = res && (count == data.getDataVecPair().size());
                if (!res) return false;
                for (int i = 0; i < count; i++) {
                    res = res && (data_vec_pair[i].first.equal(data.getItemPair(i).first));
                    res = res && (data_vec_pair[i].second.equal(data.getItemPair(i).second));
                }
                break;
            default:break;
        }
        cout << "backup: " << endl;
        data.printArgument();
        data.print();
        cout << "new: " << endl;
        printArgument();
        print();
        return res;
    }
    static int getCalChecksum(char *msg, int msg_len) {
        ll product = 1;
        ll sum = 0;
        ll checksum = 0;
        int checksum_pos_start = kByteInt, checksum_pos_end= kByteInt*2;//[4,8)
        try {
            for (int i = 0; i < msg_len; i++) { 
                if (i >= checksum_pos_start && i < checksum_pos_end ) {
                    continue;
                }
                if (msg[i] != 0) {
                    product = product *msg[i] % kChecksumPrime;
                }
                sum = (sum + msg[i]) % kChecksumPrime;
                ll temp = (((i+1)*sum) % kChecksumPrime * product)%kChecksumPrime;
                checksum = (checksum + temp) % kChecksumPrime;
            }
        } catch (exception e) {
            perror("calculate checksum error");
        }
        return (int)checksum;
    }
    //meta (attached at the beginning of the message)
    static void setMsgMetaLength(char *msg, int length) { return setInt(msg, length); }
    static void setMsgMetaChecksum(char*msg,int checksum){return setInt(msg+kByteInt,checksum);}
    static void setMsgMetaRequest(char*msg,int request) {return setInt(msg+kByteInt*2,request);}
    static void setMsgMetaStatus(char*msg,int status) { return setInt(msg+kByteInt*3, status);}
    static void setMsgMetaMarkIp(char*msg,string mark_ip) { return setStr(msg+kByteInt*4,mark_ip,IP_LEN_MAX);}
    static void setMsgMetaMarkPort(char*msg,int mark_port) {return setInt(msg+kByteInt*4+IP_LEN_MAX,mark_port);}
    static void setMsgMetaMarkId(char*msg, int mark_id) {return setInt(msg+kByteInt*5+IP_LEN_MAX,mark_id);}
    static int getMsgMetaLength(char *msg) { return getInt(msg); }
    static int getMsgMetaChecksum(char *msg) { return getInt(msg+kByteInt);}
    static int getMsgMetaRequest(char *msg) { return getInt(msg+kByteInt*2); }
    static int getMsgMetaStatus(char *msg) { return getInt(msg+kByteInt*3); }
    static string getMsgMetaMarkIp(char *msg) { return getStr(msg+kByteInt*4, IP_LEN_MAX); }
    static int getMsgMetaMarkPort(char *msg) { return getInt(msg+kByteInt*4+IP_LEN_MAX); }
    static int getMsgMetaMarkId(char *msg) { return getInt(msg+kByteInt*5+IP_LEN_MAX);}
    static void setMsgMeta(char *msg,int msg_len,int request, int status, string mark_ip, int mark_port, int mark_id = 0) {
        setMsgMetaLength(msg,msg_len);
        setMsgMetaRequest(msg,request);
        setMsgMetaStatus(msg, status);
        setMsgMetaMarkIp(msg, mark_ip);
        setMsgMetaMarkPort(msg, mark_port);
        setMsgMetaMarkId(msg, mark_id);
        int checksum = getCalChecksum(msg, msg_len);
        setMsgMetaChecksum(msg, checksum);
    }
    /*----------------Msg Encoding----------------------*/
    static char *getEncodeEmpty(int request, int status, string mark_ip, int mark_port, int mark_id = 0) {
        char* msg= (char*)malloc(kByteMeta* sizeof(char));
        memset(msg, 0, sizeof(msg));
        setMsgMeta(msg, kByteMeta, request, status, mark_ip, mark_port, mark_id);
        return msg;
    }
    static int getEncodeEmptyLen() {
        return kByteMeta; 
    }
    //contain meta
    char* getEncode(int request, int status, string mark_ip, int mark_port, int mark_id = 0) {
        char* msg= (char*)malloc((size+kByteMeta) * sizeof(char));
        memset(msg, 0, sizeof(msg));
        encode(msg+kByteMeta);//doesn't contain meta
        setMsgMeta(msg,size+kByteMeta,request, status, mark_ip, mark_port, mark_id);
        return msg;
    }
    int getEncodeLen() {
        return size+kByteMeta;
    }
    /*---------------Msg Encoding Helper----------------*/
    void encode(char* a) {
        setMsgType(a);
        setMsgLevel(a);
        switch (type) {
            case TYPE_PRIM:
                setMsgTypePrim(a);
                switch (type_prim) {
                    case TYPE_PRIM_INT:setMsgDataInt(a);break;
                    case TYPE_PRIM_BOOL:setMsgDataBool(a);break;
                    case TYPE_PRIM_DOUBLE:setMsgDataDouble(a);break;
                    case TYPE_PRIM_STR:setMsgLength(a);setMsgDataStr(a);break;
                    default:break;
                }break;
            case TYPE_VEC:
                setMsgTypeVec(a);
                setMsgCount(a);
                setMsgDataVec(a);
                break;
            case TYPE_VEC_PAIR:
                setMsgTypeVec(a);
                setMsgCount(a);
                setMsgDataVecPair(a);
                break;
            default:break;
        }
    }
    //decode with length attached at the front, can be accessed by outside objects;
    static bool ifDecodable(char *msg, int msg_len) {
        int meta_len = getMsgMetaLength(msg);
        if (meta_len!=msg_len) {
            cout << "meta size: " << kByteMeta << " supposed size: " << msg_len << " ";
            cout << "readed size: " << meta_len;
            perror("decode fails, meta length error!");
            return false;
        }
        int meta_status = getMsgMetaStatus(msg);
        if (meta_status != OK && meta_status != FAIL) {
            cout << "meta_status: " << meta_status<< endl;
            perror("decode fails, meta status error!");
            return false;
        }
        int meta_checksum_old = getMsgMetaChecksum(msg);
        int meta_checksum_new = getCalChecksum(msg, meta_len);
        if (meta_checksum_new!= meta_checksum_old) {
            cout << "old checksum:  " << meta_checksum_old << endl;
            cout << "new checksum: " << meta_checksum_new << endl;
            perror("decode fails, meta checksum error!");
            return false;
        }
        return true;
    }
    bool decode(char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        }
        decode(msg+kByteMeta);
        return true;
    }
    //decode the content(exclude meta)
    SendableData(char *a) {
        decode(a);
    }
    void decode(char *a) {
        clear();//clear content before decoding
        type = getMsgType(a);
        type_prim = (type == TYPE_PRIM? getMsgTypePrim(a) : NONE);
        type_vec= (type != TYPE_PRIM? getMsgTypeVec(a) : NONE);
        level = getMsgLevel(a);
        count = (type == TYPE_PRIM ? 1 : getMsgCount(a));
        int header_len=0;
        switch(type) {
            case TYPE_PRIM:
                switch(type_prim) {
                    case TYPE_PRIM_INT:
                        length = kByteInt;
                        size = length+kByteHeader+kByteInt;
                        data_int = getMsgDataInt(a);
                        break;
                    case TYPE_PRIM_BOOL:
                        length = kByteBool;
                        size = length+kByteHeader+kByteInt;
                        data_bool= getMsgDataBool(a);
                        break;
                    case TYPE_PRIM_DOUBLE:
                        length = kByteDouble;
                        size = length+kByteHeader+kByteInt;
                        data_double= getMsgDataDouble(a);
                        break;
                    case TYPE_PRIM_STR:
                        length = getMsgLength(a);
                        size=length+kByteHeader+kByteInt*2;
                        data_str = getMsgDataStr(a);
                        break;
                    default:break;
                }
                break;
            case TYPE_VEC:
                header_len = kByteHeader+kByteInt*2;
                length=0;
                for (int i = 0; i < count; i++) {
                    SendableData now(a+header_len+length);
                    length += now.getSize();
                    data_vec.push_back(now);
                }
                size = header_len+length;
                break;
            case TYPE_VEC_PAIR:
                header_len = kByteHeader+kByteInt*2;
                if (count == 0) {
                    cout << "Content is empty!" << endl;
                }
                length=0;
                for (int i = 0; i < count; i++) {
                    SendableData pair_first(a+header_len+length);
                    length += pair_first.getSize();
                    SendableData pair_second(a+header_len+length);
                    length += pair_second.getSize();
                    data_vec_pair.push_back(make_pair(pair_first, pair_second));
                }
                size = header_len+length;
                break;
            default:
                break;
        }
    }

    bool test() {
        test(0);
    }
    bool test(double error_rate) {
        bool equality = true;
        SendableData data_backup= copy();
        char* msg = getEncode(1, OK, "", 0, 3);
        int msg_len = getEncodeLen();
        addNoise(msg, msg_len, error_rate);
        if (!decode(msg, msg_len)) {
            perror("can't decode");
            return false;
        }
        cout << "After : " << endl;
        equality = equal(data_backup);
        if (equality) {
            cout << "Equal!" << endl;
        } else {
            cout << "NOT Equal!" << endl;
        }
        cout << endl;
        return equality;
    }
    static void markIdTest() {
        char *msg = SendableData::getEncodeEmpty(REQUEST_REGISTER, OK, "Sender-mark", 1234, 9); 
        cout << "mark id: " << getMsgMetaMarkId(msg) << endl;
    }
};

/********************various message -> SendableData test***************/
//class Node {
//public:
//    string value;
//    Node *next;
//    Node(string s):value(s), next(NULL) {
//    }
//    SendableData toSendable(int level) {
//        if (next != NULL) {
//            vector<SendableData> list;
//            list.push_back(SendableData(value, 16, level));
//            list.push_back(next->toSendable(level+1));
//            return SendableData(list, 1, level); 
//        }
//        return SendableData(value, 16, level);
//    }
//    void print(int level) {
//        for (int i = 0; i < 4 * level; i++) {
//            cout << " ";
//        }
//        cout << value << endl;
//        if (next != NULL) {
//            next->print(level+1);
//        }
//    }
//    int current_level = 0;
//    void init(SendableData &data) {
//        clear();
//        if (data.getType() == 0) {
//            value = data.getString();
//            next = NULL;
//        } else {
//            vector<SendableData> &v = data.getVector();
//            value = v[0].getString();
//            next = new Node("");
//            next->init(v[1]);
//        }
//    }
//    void clear() {
//        value = "";
//        if (next != NULL) {
//            next->clear(); 
//            delete next;
//        }
//    }
//    void test() {
//        toSendable(0).test();
//    }
//};
//class Node2 {
//public:
//    string value;
//    vector<Node2*> nextList;
//    Node2(string s):value(s) {}
//    void addNext(Node2* p) {
//        nextList.push_back(p);
//    }
//    SendableData toSendable(int level) {
//        //vector<Node*> -> vector<SendableData>
//        vector<SendableData> nextSendableList;
//        for (int i = 0; i < nextList.size(); i++) {
//            nextSendableList.push_back(nextList[i]->toSendable(level+1));
//        }
//        //pack it into single sendableData
//        SendableData nextSendable(nextSendableList, 1, level);
//        vector<SendableData> resList;
//        resList.push_back(SendableData(value, 16, level));
//        resList.push_back(nextSendable);
//        return SendableData(resList, 1, level);
//    }
//    void print(int level) {
//        for (int i = 0; i < 4 * level; i++) {
//            cout << " ";
//        }
//        cout << value << endl;
//        for (int i = 0; i < nextList.size(); i++) {
//            nextList[i]->print(level+1);
//        }
//    }
//    void init(SendableData & data) {
//        clear();
//        vector<SendableData> & list = data.getVector();
//        value = list[0].getString();
//        int count = list[1].getCount();
//        vector<SendableData> & list2 = list[1].getVector();
//        for (int i = 0; i < count; i++) {
//            nextList.push_back(new Node2(""));
//            nextList[i]->init(list2[i]);
//        }
//    }
//    void clear() {
//        value = "";
//        while (!nextList.empty()) {
//            nextList[nextList.size()-1]->clear();
//            nextList.pop_back();
//        }
//    }
//    void test() {
//        toSendable(0).test();
//    }
//};
//class Tree {
//public:
//    int count;
//    Node * head;
//    Node * tail;
//    SendableData toSendable() {
//        vector<SendableData> argumentList; 
//        //argumentList.push_back(SendableData(count, 0));
//        if (head != NULL) {
//            argumentList.push_back(head->toSendable(0));
//        }
//        return SendableData(argumentList, 1, 0);
//    }
//    Tree(): count(0), head(NULL), tail(NULL) {
//    }
//    void insert(string s) {
//        count = count+1;
//        if (head == NULL) {
//            head = tail = new Node(s);
//        } else {
//            Node *now = new Node(s);
//            tail->next = now;
//            tail = now;
//        }
//    }
//    void print() {
//        if (head != NULL) {
//            head->print(0);
//        }
//    }
//    void init(SendableData &send) {
//    }
//    void test() {
//        toSendable().test();
//    }
//};
//class Simple {
//    int x;
//    bool y;
//    double z;
//    string s;
//    vector<int> a;
//    vector<string> b;
//    vector<vector<string>> c;
//    map<pair<int, string>, int> m;
//public:
//    Simple(int x_t, bool y_t, double z_t, string s_t, vector<int> a_t, vector<string> b_t, vector<vector<string>> c_t, map<pair<int, string>, int> m_t):x(x_t), y(y_t), z(z_t), s(s_t), a(a_t),b(b_t), c(c_t), m(m_t) { toSendable(); }
//    SendableData toSendable() {
//        SendableData sendable_x(x, 0);
//        SendableData sendable_y(y, 0);
//        SendableData sendable_z(z, 0);
//        SendableData sendable_s(s, 16, 5);
//        SendableData sendable_a(a, 0);
//        //sendable_a.test();
//        SendableData sendable_b(b, 16, 0);
//        //sendable_b.test();
//        vector<SendableData> c_list;
//        for (int i = 0; i < c.size(); i++) {
//            c_list.push_back(SendableData(c[i], 16, 0));
//        }
//        SendableData sendable_c(c_list, 1, 0);
//        vector<pair<SendableData, SendableData>> m_list;
//        for (auto it = m.begin(); it != m.end(); it++) {
//            //key and value
//            pair<int, string> key_pair = it->first;
//            int value = it->second;
//            vector<SendableData> key_list;
//            //int
//            key_list.push_back(SendableData(key_pair.first, 0));
//            key_list.push_back(SendableData(key_pair.second, 16, 0));
//            m_list.push_back(make_pair(SendableData(key_list, 0, 0), SendableData(value, 0)));
//        }
//        SendableData sendable_m(m_list, 0, 0);
//        vector<SendableData> list = { sendable_x, sendable_y, sendable_z, sendable_s, sendable_a , sendable_b, sendable_c, sendable_m };
//        list.clear();
//        list = { sendable_x };
//        list.clear();
//        list = { sendable_x, sendable_y };
//        list.clear();
//        list = { sendable_x, sendable_y, sendable_z, sendable_s, sendable_a, sendable_b, sendable_c };
//        SendableData res(list, 1, 0);
//        return res;
//    }
//    void test() {
//        toSendable().test();
//    }
//};
//
//void test2() {
//    //vector<int> a = {3,4,2};
//    //vector<string> b = {"Mijia", "Jiang"};
//    //vector<vector<string>> c = {{"1", "2", "3"}, {"4", "5", "6"}, {"7", "8", "9"}};
//    //map<pair<int, string>, int> m = {{make_pair(3, "a"), 1}, {make_pair(4, "b"), 2}};
//    //Simple s(3, true, 2.1, "abc", a, b, c, m);
//    //s.test();
//    
//    //Node *p;
//    //p = new Node("123");
//    //p->next = new Node("456");
//    ////(p->toSendable(0)).print();
//    //SendableData data = p->toSendable(0);
//    //Node *pp = new Node("1");
//    //pp->init(data);
//    //p->test();
//    //
//    //cout << " --------------" << endl;
//    //Node2 *p2 = new Node2("789");
//    //Node2 *p4 = new Node2("111");
//    //p4->addNext(new Node2("333"));
//    //p2->addNext(p4);
//    //p2->addNext(new Node2("555"));
//    //SendableData d = p2->toSendable(0);
//    //Node2 *p3 = new Node2("");
//    //p3->init(d);
//    //p3->print(0);
//    //p2->toSendable(0)).print();
//    //p2->print(0);
//    //Tree *t = new Tree();
//    //cout << "------------" << endl;
//    //t->insert("149");
//    //t->insert("135");
//    //t->toSendable()).print();
//    //t->print();
//    //t->test();
//    //
//    //Test Group 1: primitive types
//    //int test1_1 = 0;
//    //int test1_2 = -3;
//    //double test1_3 = -2.5;
//    //double test1_4 = 3.3;
//    //bool test1_5 = true;
//    //bool test1_6 = false;
//    //string test1_7 = "abc";
//    //string test1_8 = "";
//    //
//
//    //bool res = true;
//    //int fail_count = 0;
//    //int times = 100;
//    //double error_rate = 0.5;
//    //bool total_res = true;
//    //for (int i = 0; i < times; i++) {
//    //    SendableData test1_1_send(test1_1, 0);
//    //    res = test1_1_send.test(error_rate);
//    //    if (!res) { cout << "Fail1" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_2_send(test1_2, 1);
//    //    res = test1_2_send.test(error_rate);
//    //    if (!res) { cout << "Fail2" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_3_send(test1_3, 2);
//    //    res = test1_3_send.test(error_rate);
//    //    if (!res) { cout << "Fail3" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_4_send(test1_4, 3);
//    //    res = test1_4_send.test(error_rate);
//    //    if (!res) { cout << "Fail4" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_5_send(test1_5, 4);
//    //    res = test1_5_send.test(error_rate);
//    //    if (!res) { cout << "Fail5" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_6_send(test1_6, 5);
//    //    res = test1_6_send.test(error_rate);
//    //    if (!res) { cout << "Fail6" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_7_send(test1_7, 16, 0);
//    //    res = test1_7_send.test(error_rate);
//    //    if (!res) { cout << "Fail7" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    SendableData test1_8_send(test1_8, 16, 0);
//    //    res = test1_8_send.test(error_rate);
//    //    if (!res) { cout << "Fail8" << endl;fail_count++;}
//    //    total_res = res && total_res;
//    //    if (total_res) {
//    //        cout << "Pass All Tests!" << endl;
//    //    } else {
//    //        cout << "Doesn't Pass All Tests!" << endl;
//    //    }
//    //}
//    //cout << "total fails: " << fail_count << endl;
//    //cout << "supposed fails: " << 8 * times * error_rate << endl;
//}
/******************************various message to SendableData test****************************/


/*****************************Transimisson Data***********************************************/

/*****************************Global Info Data (start)****************************************/
class GlobalInfo {
private:
    vector<vector<int>> latency_table;
    int time_out_max;
public:
    GlobalInfo(): time_out_max(TIME_OUT_MAX_DEFAULT) {}
    GlobalInfo(vector<vector<int>> latency_table_t, int time_out_max_t) {
        for (int i = 0; i < latency_table.size(); i++) {
            if (latency_table[i].size() != latency_table.size() || latency_table[i][i] != 0) {
                perror("latency table error!");
                return;
            }
            for (int j = i+1; j < latency_table[i].size(); j++) {
                if (latency_table[i][j] != latency_table[j][i]) {
                    perror("latency table error!");
                    return;
                }
            }
        }
        latency_table = latency_table_t;
        time_out_max = time_out_max_t;
    }
    vector<vector<int>> & getLatencyTable() {
        return latency_table;
    }
    int getTimeOutMax() {
        return time_out_max;
    }
    int getLatency(int i, int j) {
        if (i < 0 || i >= latency_table.size() || j < 0 || j >=latency_table.size()) {
            perror("latency index error!");
            return NONE;
        }
        return latency_table[i][j];
    }
    int getCount() {
        return latency_table.size();
    }
    pair<char *,int> getMsg(int request, string mark_ip, int mark_port, int mark_id) {
        SendableData data = toSendable();
        char * msg = data.getEncode(request, OK, mark_ip, mark_port, mark_id); 
        int msg_len = data.getEncodeLen(); 
        return make_pair(msg, msg_len); 
    }
    void clear() {
        for (int i = 0; i < latency_table.size(); i++) {
            latency_table[i].clear();
        }
        latency_table.clear();
        time_out_max = NONE;
    }
    void reset(SendableData &data) {
        clear();
        vector<SendableData> global_info_data;
        SendableData latency_table_data = data.getItem(0);
        SendableData time_out_max_data= data.getItem(1);
        for (int i = 0; i < latency_table_data.getCount(); i++) {
            vector<int> now = latency_table_data.getItem(i).toVecInt();
            latency_table.push_back(now);
        }
        time_out_max = time_out_max_data.getDataInt();
    }
    SendableData toSendable() {
        vector<SendableData> global_info_data;
        vector<SendableData> latency_table_data;
        for (int i = 0; i < latency_table.size(); i++) {
            SendableData now(latency_table[i], 0);
            latency_table_data.push_back(now);
        }
        global_info_data.push_back(SendableData(latency_table_data, TYPE_VEC_MUL, 0));
        global_info_data.push_back(SendableData(time_out_max,0));
        return SendableData(global_info_data, TYPE_VEC_MUL, 0);
    }
    void decode(char *msg, int msg_len) {
        SendableData data;
        data.decode(msg, msg_len);
        reset(data);
    }
    void print() {
        cout << "print global info: " << endl;
        for (int i = 0; i < latency_table.size(); i++) {
            for (int j = 0; j < latency_table[i].size(); j++) {
                cout << i << "->" << j << ":" << latency_table[i][j] << endl;
            }
        }
        cout << "time out max: " << time_out_max << endl;
    }
};
/*****************************Global Info Data (end)******************************************/

/*****************************Client List Data (start)****************************************/
struct ClientSingle {
public:
    ClientSingle() {}
    ClientSingle(string mark_ip_t, int mark_port_t, int mark_id_t, int load_t = -1, int latency_t = -1):mark_ip(mark_ip_t), mark_port(mark_port_t), mark_id(mark_id_t), load(load_t), latency(latency_t) {}
    string mark_ip;
    int mark_port;
    int mark_id;
    int load;
    int latency;
};
//algorithm for soring client list
struct client_comparison {
    bool operator() (ClientSingle const &a, ClientSingle const &b) {
        if (a.load == NONE) return 1; //NONE means doesn't get load
        if (b.load == NONE) return -1;
        return (a.load+1) * a.latency < (b.load+1) * b.latency;
    }
};
class ClientList{
private:
    vector<ClientSingle> client_list;
public:
    string getMarkIp(int i) {
        return client_list[i].mark_ip;
    }
    int getMarkPort(int i) {
        return client_list[i].mark_port;
    }
    pair<string,int> getMarkAddr(int i) {
        return make_pair(client_list[i].mark_ip, client_list[i].mark_port);
    }
    int getMarkId(int i) {
        return client_list[i].mark_id;
    }
    int getCount() {
        return client_list.size();
    }
    void sort(vector<int> & load_list_t, int mark_id, GlobalInfo & global_info) {
        cout << "Check latecy:" << global_info.getLatency(4, 8) << endl;
        for (int i = 0; i < load_list_t.size(); i++) {
            client_list[i].load = load_list_t[i];
            cout << "latency:index" << mark_id << " " << client_list[i].mark_id << endl;
            client_list[i].latency = global_info.getLatency(mark_id, client_list[i].mark_id);
        }
        std::sort(client_list.begin(), client_list.end(), client_comparison());
    }
    void init(vector<pair<string,int>> & client_addr_list_t, vector<int> & client_id_list_t) {
        if (client_addr_list_t.size() != client_id_list_t.size()) {
            perror("size error for client list.");
        }
        client_list.clear();
        for (int i = 0; i < client_id_list_t.size(); i++) {
            ClientSingle now(client_addr_list_t[i].first, client_addr_list_t[i].second, client_id_list_t[i]);
            client_list.push_back(now);
        }
    }
    ClientList() {}
    ClientList(vector<pair<string,int>> & client_addr_list_t, vector<int> & client_id_list_t) {
        init(client_addr_list_t, client_id_list_t);
    }
    pair<char *,int> getMsg(int request, string mark_ip, int mark_port, int mark_id) {
        SendableData data = toSendable();
        char * msg = data.getEncode(request, OK, mark_ip, mark_port, mark_id); 
        int msg_len = data.getEncodeLen(); 
        return make_pair(msg, msg_len); 
    }
    void clear() {
        client_list.clear();
    }
    void reset(SendableData &data) {
        clear();
        vector<pair<string, int>> addr_list = data.getItem(0).toVecPairStrInt();
        vector<int> id_list = data.getItem(1).toVecInt();
        init(addr_list, id_list);
    }
    int getLoad(int i) {
        return client_list[i].load;
    }
    vector<pair<string, int>> toAddrList() {
        vector<pair<string, int>> a;
        for (int i = 0; i < client_list.size(); i++) {
            a.push_back(make_pair(client_list[i].mark_ip, client_list[i].mark_port));
        }
        return a;
    }
    vector<int> toIdList() {
        vector<int> a;
        for (int i = 0; i < client_list.size(); i++) {
            a.push_back(client_list[i].mark_id);
        }
        return a;
    }
    SendableData toSendable() {
        vector<pair<string,int>> addr_list = toAddrList();
        vector<int> id_list = toIdList();
        SendableData client_addr_list_data(addr_list, IP_LEN_MAX, 0);
        SendableData client_id_list_data(id_list, 0);
        vector<SendableData> client_list_data;
        client_list_data.push_back(client_addr_list_data);
        client_list_data.push_back(client_id_list_data);
        return SendableData(client_list_data, TYPE_VEC_MUL, 0);
    }
    void decode(char *msg, int msg_len) {
        SendableData data;
        data.decode(msg, msg_len);
        reset(data);
    }
    void print() {
        cout << "print client list" << endl;
        for (int i = 0; i < client_list.size(); i++) {
            cout << client_list[i].mark_id << ":("<<client_list[i].mark_ip<< "," << client_list[i].mark_port<<") load:" << client_list[i].load <<  " latency: " << client_list[i].latency << endl;
        }
    }
};
/*****************************Client List Data (end)******************************************/
/*****************************Client Data (start)*********************************************/
class ClientData {
public:
    map<int, vector<string>> idtof;//mark id -> file name list
    map<string, set<int>> ftoid; // file name -> set of id
    map<int, pair<string,int>> idtoaddr; //id -> addr
    ClientList getClientList(string filename) {
        vector<pair<string, int>> client_addr_list;
        vector<int> client_id_list;
        if (ftoid.find(filename) != ftoid.end()) {
            for (auto id : ftoid[filename]) {
                client_id_list.push_back(id);
                client_addr_list.push_back(idtoaddr[id]);
            }
        }
        return ClientList(client_addr_list, client_id_list);
    }
    void print() {
        //cout << "client match id -> file name list" << endl;
        //for (auto pair: idtof) {
        //    cout << pair.first << ": " ;
        //    auto list = pair.second;
        //    for (int i = 0; i < list.size(); i++) {
        //        cout << list[i] << " ";
        //    }
        //    cout << endl;
        //}
        cout << "filename  -> client match id" << endl;
        for (auto pair: ftoid) {
            cout << pair.first << ": " ;
            auto id_set = pair.second;
            for (auto x: id_set) {
                cout << x << " ";
            }
            cout << endl;
        }
        //cout << "client match id -> client match ip and port" << endl;
        //for (auto pair: idtoaddr) {
        //    cout << pair.first << " -> " << pair.second.first << "," << pair.second.second << endl;
        //}
    }
    void clear() {
        idtof.clear();
        ftoid.clear();
        idtoaddr.clear();
    }
    void setFileNameList(string mark_ip, int mark_port, int mark_id, vector<string> & list) {
        if (idtof.find(mark_id) != idtof.end()) {
            auto now = idtof[mark_id];
            for (int i = 0; i < now.size(); i++) {
                auto id_set = ftoid[now[i]];
                id_set.erase(id_set.find(mark_id));
                if (id_set.empty()) {
                    ftoid.erase(ftoid.find(now[i]));
                }
            }
        }    
        idtof[mark_id] = list;
        for (int i = 0; i < list.size(); i++) {
            if (ftoid.find(list[i]) !=ftoid.end()) {
                ftoid[list[i]].insert(mark_id);
            } else {
                set<int> id_set;
                id_set.insert(mark_id);
                ftoid[list[i]] = id_set; 
            }
        }
        idtoaddr[mark_id] = make_pair(mark_ip, mark_port);
        //
    }
    vector<string> getFileNameList() {
        vector<string> res;
        for (auto pair : ftoid) {
            res.push_back(pair.first);
        }
        return res;
    }
    void test() {
        vector<string> test_1 = {"a.txt", "b.txt"};
        vector<string> test_2 = {"a.txt", "c.txt"};
        vector<string> test_3 = {"a.txt", "b.txt"};
        ClientData data;
        data.setFileNameList("Client", 1234, 1, test_1); 
        data.setFileNameList("Client", 1266, 2, test_3); 
        data.setFileNameList("Client", 1259, 1, test_2); 
        data.print();
        ClientList c = data.getClientList(Format::toStr("a.txt"));
        c.print();
        ClientList c2 = data.getClientList(Format::toStr(".txt"));
        c2.print();
        vector<string> fl = data.getFileNameList();
        for (int i = 0; i < fl.size(); i++) {
            cout << fl[i] << " ";
        }
        cout << endl;
    }
};
/*****************************Client Data (end)***********************************************/

/*******************MsgSt and Message: Wrapper Class for SendableData (start)*****************/
struct MsgSt {
public:
    MsgSt():if_decodable(false), status(FAIL), request(NONE),o_ip("NOT SET"), o_port(NONE), o_mark_ip("NOT SET"), o_mark_port(NONE), o_mark_id(NONE), t_ip("NOT SET"), t_port(NONE), t_mark_id(NONE), msg_len(NONE) {
        msg = new char[MSG_LEN_MAX];
    }
    bool if_decodable = false;
    int status;
    int request;
    string o_mark_ip;
    int o_mark_port;
    int o_mark_id;
    string o_ip;
    int o_port;
    string t_ip;
    int t_port;
    int t_mark_id;
    char *msg;
    int msg_len;
};
class Message {
public:
    //get a msgst contains message (for send)
    //t: target address (either server's address, or previous received msg's origin addr)
    static MsgSt deepCopy(MsgSt &msgst) {
        MsgSt st;
        Message::setMsgStMsg(st, msgst.msg, msgst.msg_len);
        st.o_ip = msgst.o_ip;
        st.o_port = msgst.o_port;
        st.t_ip = msgst.t_ip;
        st.t_port = msgst.t_port;
        st.t_mark_id = msgst.t_mark_id;
        return st;
    }
    static MsgSt getMsgStSend(pair<char *, int> &msg, string t_ip, int t_port, int t_mark_id) {
        MsgSt st;
        setMsgStMsg(st, msg.first, msg.second);
        st.t_ip = t_ip;
        st.t_port = t_port;
        st.t_mark_id = t_mark_id;
        return st;
    }
    //get a msgst to receive coming message( for listen)
    //t: receiver's listening address
    static MsgSt getMsgStRecv(string t_ip, int t_port, int t_mark_id) {
        MsgSt st;
        st.t_ip = t_ip;
        st.t_port = t_port;
        st.t_mark_id = t_mark_id;
        return st;
    }

    static bool setMsgStMsg(MsgSt &st, char *msg, int msg_len) {
        st.msg_len = msg_len;
        memcpy(st.msg, msg, msg_len);
        return setMsgStMeta(st);
    }
    static bool setMsgStMeta(MsgSt &st) {
        st.if_decodable = ifDecodable(st.msg, st.msg_len);
        if (st.if_decodable == false) {
            st.status = FAIL;
            st.request = FAIL;
            return false;
        }
        st.status = getStatus(st.msg, st.msg_len);
        st.request = getRequest(st.msg, st.msg_len);
        if (st.status != OK) {
            return false;
        }
        st.o_mark_ip = getMarkIp(st.msg, st.msg_len);
        st.o_mark_port = getMarkPort(st.msg, st.msg_len);
        st.o_mark_id = getMarkId(st.msg, st.msg_len);
        return true;
    }
    static bool setMsgStO(MsgSt &st, string o_ip, int o_port) {
        st.o_ip = o_ip;
        st.o_port = o_port;
        return true;
    }
    static MsgSt getOkMsgSt(int request, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getOkMsg(request, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id); 
    }
    static MsgSt getFailMsgSt(int request, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getFailMsg(request, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id); 
    }
    static pair<char *,int> getOkMsg(int request, string mark_ip, int mark_port, int mark_id) {
        return make_pair(SendableData::getEncodeEmpty(request, OK, mark_ip, mark_port, mark_id), SendableData::getEncodeEmptyLen()); 
    }
    static pair<char *,int> getFailMsg(int request, string mark_ip, int mark_port, int mark_id) {
        return make_pair(SendableData::getEncodeEmpty(request,FAIL, mark_ip, mark_port, mark_id), SendableData::getEncodeEmptyLen());
    }
    static bool ifDecodable(char *msg, int msg_len) {
        //cout << "Decodable: " << SendableData::ifDecodable(msg, msg_len) << endl;
        return SendableData::ifDecodable(msg, msg_len);
    }
    static int getRequest(char *msg, int msg_len) {
        int res = SendableData::getMsgMetaRequest(msg);
        return res;
    }
    static int tryGetStatus(int &status, char *msg, int msg_len){
        if (!ifDecodable(msg, msg_len)) {
            return false;
        }
        status = getStatus(msg, msg_len);
        return true;
    }
    static int getStatus(char *msg, int msg_len){
        return SendableData::getMsgMetaStatus(msg);
    }
    static string getMarkIp(char *msg, int msg_len) {
        return SendableData::getMsgMetaMarkIp(msg);
    }
    static int getMarkPort(char *msg, int msg_len) {
        return SendableData::getMsgMetaMarkPort(msg);
    }
    static int getMarkId(char *msg, int msg_len) {
        return SendableData::getMsgMetaMarkId(msg);
    }
    static MsgSt getGlobalInfoMsgSt(int request, GlobalInfo &global_info, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getGlobalInfoMsg(request, global_info, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id);
    }
    static pair<char *,int> getGlobalInfoMsg(int request, GlobalInfo &global_info, string mark_ip, int mark_port, int mark_id) {
        return global_info.getMsg(request, mark_ip, mark_port, mark_id);
    }
    static bool tryGetGlobalInfo(GlobalInfo &global_info, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            global_info = getGlobalInfo(msg, msg_len);
        }
    }
    static GlobalInfo getGlobalInfo(char *msg, int msg_len) {
        GlobalInfo res;
        res.decode(msg, msg_len);
        return res;
    }
    static MsgSt getLoadMsgSt(int request, const int & load, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getLoadMsg(request, load, mark_ip, mark_port, mark_id);
        auto msgst = getMsgStSend(msg, t_ip, t_port, t_mark_id);
        return msgst;
    }
    static pair<char *,int> getLoadMsg(int request, const int & load, string mark_ip, int mark_port, int mark_id) {
        SendableData load_data(load, 0);
        char * msg = load_data.getEncode(request, OK, mark_ip, mark_port, mark_id);
        int msg_len = load_data.getEncodeLen();
        return make_pair(msg, msg_len);
    }
    static bool tryGetLoad(int &load, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            load = getLoad(msg, msg_len);
        }
        return true;
    }
    static int getLoad(char *msg, int msg_len) {
        SendableData data; 
        if (data.decode(msg, msg_len)) {
            return data.getDataInt();
        }
        perror("can't decode (in Message)");
        return NONE;
    }
    static MsgSt getFileNameMsgSt(int request, string & file_name, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getFileNameMsg(request, file_name, mark_ip, mark_port, mark_id);
        auto msgst = getMsgStSend(msg, t_ip, t_port, t_mark_id);
        return msgst;
    }
    static pair<char *,int> getFileNameMsg(int request, string & file_name, string mark_ip, int mark_port, int mark_id) {
        SendableData file_name_data(file_name, FILE_NAME_LEN_MAX, 0);
        char * msg = file_name_data.getEncode(request, OK, mark_ip, mark_port, mark_id);
        int msg_len = file_name_data.getEncodeLen();
        return make_pair(msg, msg_len);
    }
    static bool tryGetFileName(string &file_name, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            file_name = getFileName(msg, msg_len);
        }
        return true;
    }
    static string getFileName(char *msg, int msg_len) {
        SendableData data; 
        if (data.decode(msg, msg_len)) {
            return data.getDataStr();
        }
        perror("can't decode (in Message)");
        return "";
    }
    static MsgSt getFileNameListMsgSt(int request, vector<string> & file_name_list, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getFileNameListMsg(request, file_name_list, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id);
    }
    static pair<char *,int> getFileNameListMsg(int request, vector<string> & file_name_list, string mark_ip, int mark_port, int mark_id) {
        SendableData file_name_list_data(file_name_list, FILE_NAME_LEN_MAX, 0);
        char * msg = file_name_list_data.getEncode(request, OK, mark_ip, mark_port, mark_id);
        int msg_len = file_name_list_data.getEncodeLen();
        return make_pair(msg, msg_len);
    }
    static bool tryGetFileNameList(vector<string> &file_name_list, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            file_name_list = getFileNameList(msg, msg_len);
        }
        return true;
    }
    static vector<string> getFileNameList(char *msg, int msg_len) {
        SendableData data; 
        if (data.decode(msg, msg_len)) {
            return data.toVecStr();
        }
        return vector<string>();
    }
    static MsgSt getClientListMsgSt(int request, ClientList & client_list, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getClientListMsg(request, client_list, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id);
    }
    static pair<char *,int> getClientListMsg(int request, ClientList & client_list, string mark_ip, int mark_port, int mark_id) {
        SendableData client_list_data = client_list.toSendable();
        //SendableData client_list_data(client_list, IP_LEN_MAX, 0);
        char * msg = client_list_data.getEncode(request, OK, mark_ip, mark_port, mark_id);
        int msg_len = client_list_data.getEncodeLen();
        return make_pair(msg, msg_len);
    }
    static bool tryGetClientList(ClientList &client_list, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            client_list = getClientList(msg, msg_len);
        }
        return true;
    }
    static ClientList getClientList(char *msg, int msg_len) {
        ClientList client_list;
        client_list.decode(msg, msg_len);
        return client_list; 
    }
    static MsgSt getFileMsgSt(int request, pair<string, string> &file, string mark_ip, int mark_port, int mark_id, string t_ip, int t_port, int t_mark_id) {
        auto msg = getFileMsg(request, file, mark_ip, mark_port, mark_id);
        return getMsgStSend(msg, t_ip, t_port, t_mark_id);
    }
    static pair<char *,int> getFileMsg(int request, pair<string,string> & file, string mark_ip, int mark_port, int mark_id) {
        SendableData file_data(file, FILE_NAME_LEN_MAX, FILE_CONTENT_LEN_MAX, 0);
        char * msg = file_data.getEncode(request, OK, mark_ip, mark_port, mark_id);
        int msg_len = file_data.getEncodeLen();
        return make_pair(msg, msg_len);
    }
    static bool tryGetFile(pair<string, string> &file, char *msg, int msg_len) {
        if (!ifDecodable(msg, msg_len)) {
            return false;
        } else if (getStatus(msg, msg_len) == FAIL) {
            return false;
        } else {
            file = getFile(msg, msg_len);
        }
        return true;
    }
    static pair<string, string> getFile(char *msg, int msg_len) {
        SendableData data; 
        if (data.decode(msg, msg_len)) {
            return data.toPairStrStr();
        }
        return pair<string,string>("","");
    }
    static void print(const vector<int> &a) {
        for (int i = 0; i < a.size(); i++) {
            cout << a[i] << " ";
        }
        cout << endl;
    }
    static void print(const vector<string> &a) {
        for (int i = 0; i < a.size(); i++) {
            cout << a[i] << endl;
        }
    }
    static void print(const vector<pair<string,int>> &a) {
        for (int i = 0; i < a.size(); i++) {
            cout << a[i].first << " " << a[i].second<<endl;
        }
    }
    static void print(const vector<pair<string,string>> &a) {
        for (int i = 0; i < a.size(); i++) {
            cout << a[i].first << " " << a[i].second<<endl;
        }
    }
    static void print(const pair<string,int> &x) {
        cout << x.first << " " << x.second<<endl;
    }
    static void print(const pair<string,string> &x) {
        cout << x.first << " " << x.second<<endl;
    }
    static void printMsgStMeta(const MsgSt &st) {
        cout << "if_decodable:" << " " << st.if_decodable << " ";
        cout << "status: " << st.status << " ";
        cout << "request: " << st.request << endl;
        cout << "o_ip: " << st.o_ip << endl;
        cout << "o_port: " << st.o_port << endl;
        cout << "o_mark_ip: " << st.o_mark_ip << endl;
        cout << "o_mark_port: " << st.o_mark_port << endl;
        cout << "o_mark_id: " << st.o_mark_id << endl;
        cout << "t_ip: " << st.t_ip << endl;
        cout << "t_port: " << st.t_port << endl;
        cout << "t_mard id: " << st.t_mark_id << endl;
        if (st.if_decodable == false ) {
            cout << "msgst can't be decoded." << endl;
            return;
        }
        if (st.status == FAIL) {
            cout << "msgst is a fail msg" << endl;
            return;
        }
        if (st.request == NONE) {
            cout << "msgst is a NONE request msg" << endl;
            return;
        }
        if (st.status == OK) {
            cout << "msgst is a OK msg" << endl;
        }
    }
    static void test2() {
        int count_pass = 0, count_fail = 0;
        string test_1 = "a.txt";
        vector<string> test_2 = {}; 
        vector<string> test_3 = {"a.txt"}; 
        vector<string> test_4 = {"a.txt", "b.txt"}; 
        vector<pair<string,int>> test_5_addr = {make_pair("127.125.0.0",3269)};
        vector<int> test_5_id = {1};
        vector<pair<string,int>> test_6_addr = {make_pair("127.125.0.0",3269), make_pair("138.164.4.9", 2571)};
        vector<int> test_6_id = {1, 2};
        ClientList test_5(test_5_addr, test_5_id);
        ClientList test_6(test_6_addr, test_6_id);
        vector<pair<string,int>> test_7 = {};
        pair<string,string> test_8 = make_pair("a.txt", "yesyesno");
        pair<string,string> test_9 = make_pair("b.txt", "");
        GlobalInfo test_10;
        for (int error_rate = 0; error_rate < 2; error_rate++) {
            cout << "test_1" << endl;
            auto msgst = getFileNameMsgSt(1, test_1, "Sender", 3000, 1, "Target", 2000, 9);
            setMsgStO(msgst,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst);
            auto msgst_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst.msg, msgst.msg_len, error_rate);
            msgst_r.msg_len = msgst.msg_len;
            memcpy(msgst_r.msg, msgst.msg, msgst.msg_len);
            setMsgStMeta(msgst_r);
            setMsgStO(msgst_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_2" << endl;
            auto msgst2 = getFileNameListMsgSt(1, test_2, "Sender", 3000, 1, "Target", 2000, 9);
            setMsgStO(msgst2,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst2);
            auto msgst2_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst2.msg, msgst2.msg_len, error_rate);
            msgst2_r.msg_len = msgst2.msg_len;
            memcpy(msgst2_r.msg, msgst2.msg, msgst2.msg_len);
            setMsgStMeta(msgst2_r);
            setMsgStO(msgst2_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst2_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_3" << endl;
            auto msgst3 = getGlobalInfoMsgSt(1, test_10, "Sender", 3000, 1, "Target", 2000, 9);
            setMsgStO(msgst3,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst3);
            auto msgst3_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst3.msg, msgst3.msg_len, error_rate);
            msgst3_r.msg_len = msgst3.msg_len;
            memcpy(msgst3_r.msg, msgst3.msg, msgst3.msg_len);
            setMsgStMeta(msgst3_r);
            setMsgStO(msgst3_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst3_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_4" << endl;
            auto msgst4 = getFileMsgSt(1, test_8, "Sender", 3000, 1, "Target", 2000, 9);
            setMsgStO(msgst4,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst4);
            auto msgst4_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst4.msg, msgst4.msg_len, error_rate);
            msgst4_r.msg_len = msgst4.msg_len;
            memcpy(msgst4_r.msg, msgst4.msg, msgst4.msg_len);
            setMsgStMeta(msgst4_r);
            setMsgStO(msgst4_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst4_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_5" << endl;
            auto msgst5 = getClientListMsgSt(1, test_6, "Sender", 3000, 1,"Target", 2000, 9);
            setMsgStO(msgst5,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst5);
            auto msgst5_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst5.msg, msgst5.msg_len, error_rate);
            msgst5_r.msg_len = msgst5.msg_len;
            memcpy(msgst5_r.msg, msgst5.msg, msgst5.msg_len);
            setMsgStMeta(msgst5_r);
            setMsgStO(msgst5_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst5_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_6" << endl;
            auto msgst6 = getOkMsgSt(1, "Sender", 3000,1, "Target", 2000, 9);
            setMsgStO(msgst6,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst6);
            auto msgst6_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst6.msg, msgst6.msg_len, error_rate);
            msgst6_r.msg_len = msgst6.msg_len;
            memcpy(msgst6_r.msg, msgst6.msg, msgst6.msg_len);
            setMsgStMeta(msgst6_r);
            setMsgStO(msgst6_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst6_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);

            cout << "test_7" << endl;
            auto msgst7 = getFailMsgSt(1, "Sender", 3000,1, "Target", 2000, 9);
            setMsgStO(msgst7,"Origin", 1);
            cout << "to be sended: " << endl;
            printMsgStMeta(msgst7);
            auto msgst7_r = getMsgStRecv("Listener", 2, 9);
            addNoise(msgst7.msg, msgst7.msg_len, error_rate);
            msgst7_r.msg_len = msgst7.msg_len;
            memcpy(msgst7_r.msg, msgst7.msg, msgst7.msg_len);
            setMsgStMeta(msgst7_r);
            setMsgStO(msgst7_r, "Origin", 4);
            cout << "received: " << endl;
            if (msgst7_r.if_decodable) {count_pass++;} else {count_fail++;}
            printMsgStMeta(msgst_r);
        }
        cout << "count decodable: " << count_pass << endl;
        cout << "count undecodable: " << count_fail << endl;
    }

    static void test() {
        int count_pass = 0, count_fail = 0;
        string test_1 = "a.txt";
        vector<string> test_2 = {}; 
        vector<string> test_3 = {"a.txt"}; 
        vector<string> test_4 = {"a.txt", "b.txt"}; 
        vector<pair<string,int>> test_5_addr = {make_pair("127.125.0.0",3269)};
        vector<int> test_5_id = {1};
        vector<pair<string,int>> test_6_addr = {make_pair("127.125.0.0",3269), make_pair("138.164.4.9", 2571)};
        vector<int> test_6_id = {1, 2};
        ClientList test_5(test_5_addr, test_5_id);
        ClientList test_6(test_6_addr, test_6_id);
        vector<pair<string,int>> test_7 = {};
        pair<string,string> test_8 = make_pair("a.txt", "yesyesno");
        pair<string,string> test_9 = make_pair("b.txt", "");
        //cout << "test_1" << endl;
        //auto msg_1 = getFileNameMsg(1, test_1, "",0);
        //auto msg_1_noise = getFileNameMsg(1, test_1, "",0);
        //addNoise(msg_1_noise.first, msg_1_noise.second, 1);
        //string test_res_1; 
        //string test_res_1_noise; 
        //if (tryGetFileName(test_res_1, msg_1.first, msg_1.second)) {
        //    cout << "Success:" <<endl;
        //    cout << test_1 << "\n" << test_res_1<<endl;
        //    count_pass++;
        //}
        //if (!tryGetFileName(test_res_1_noise, msg_1_noise.first, msg_1_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    cout << test_1 << "\n" <<test_res_1_noise<<endl;
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_2" << endl;
        //auto msg_2 = getFileNameListMsg(1, test_2,"",0);
        //auto msg_2_noise = getFileNameListMsg(1, test_2,"",0);
        //addNoise(msg_2_noise.first, msg_2_noise.second, 1);
        //vector<string> test_res_2; 
        //vector<string> test_res_2_noise; 
        //if (tryGetFileNameList(test_res_2, msg_2.first, msg_2.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_2);
        //    print(test_res_2);
        //    count_pass++;
        //}
        //if (!tryGetFileNameList(test_res_2_noise, msg_2_noise.first, msg_2_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_2);
        //    print(test_res_2_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_3" << endl;
        //auto msg_3 = getFileNameListMsg(1, test_3,"",0);
        //auto msg_3_noise = getFileNameListMsg(1, test_3,"",0);
        //addNoise(msg_3_noise.first, msg_3_noise.second, 1);
        //vector<string> test_res_3; 
        //vector<string> test_res_3_noise; 
        //if (tryGetFileNameList(test_res_3, msg_3.first, msg_3.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_3);
        //    print(test_res_3);
        //    count_pass++;
        //}
        //if (!tryGetFileNameList(test_res_3_noise, msg_3_noise.first, msg_3_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_3);
        //    print(test_res_3_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_4" << endl;
        //auto msg_4 = getFileNameListMsg(1, test_4,"",0);
        //auto msg_4_noise = getFileNameListMsg(1, test_4, "",0);
        //addNoise(msg_4_noise.first, msg_4_noise.second, 1);
        //vector<string> test_res_4; 
        //vector<string> test_res_4_noise; 
        //if (tryGetFileNameList(test_res_4, msg_4.first, msg_4.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_4);
        //    print(test_res_4);
        //    count_pass++;
        //}
        //if (!tryGetFileNameList(test_res_4_noise, msg_4_noise.first, msg_4_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_4);
        //    print(test_res_4_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_5" << endl;
        //auto msg_5 = getClientListMsg(1, test_5,"Sender", 2000, 1);
        //auto msg_5_noise = getClientListMsg(1, test_5,"Sender",2000, 1);
        //addNoise(msg_5_noise.first, msg_5_noise.second, 1);
        //ClientList test_res_5; 
        //ClientList test_res_5_noise; 
        //if (tryGetClientList(test_res_5, msg_5.first, msg_5.second)) {
        //    cout << "Success:" <<endl;
        //    test_5.print();
        //    test_res_5.print();
        //    count_pass++;
        //}
        //if (!tryGetClientList(test_res_5_noise, msg_5_noise.first, msg_5_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    test_5.print();
        //    test_res_5_noise.print();
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_6" << endl;
        //auto msg_6 = getClientListMsg(1, test_6,"Sender",2000,0);
        //auto msg_6_noise = getClientListMsg(1, test_6,"Sender",2000,0);
        //addNoise(msg_6_noise.first, msg_6_noise.second, 1);
        //vector<pair<string,int>> test_res_6_addr = {make_pair("1", 2)};
        //vector<int> test_res_6_id = {9}; 
        //ClientList test_res_6(test_res_6_addr, test_res_6_id);
        //ClientList test_res_6_noise(test_res_6_addr, test_res_6_id);
        //if (tryGetClientList(test_res_6, msg_6.first, msg_6.second)) {
        //    cout << "Success:" <<endl;
        //    test_6.print();
        //    test_res_6.print();
        //    count_pass++;
        //}
        //if (!tryGetClientList(test_res_6_noise, msg_6_noise.first, msg_6_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    test_6.print();
        //    test_res_6_noise.print();
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_7" << endl;
        //auto msg_7 = getClientListMsg(1, test_7,"",0);
        //auto msg_7_noise = getClientListMsg(1, test_7,"",0);
        //addNoise(msg_7_noise.first, msg_7_noise.second, 1);
        //vector<pair<string,int>> test_res_7; 
        //vector<pair<string,int>> test_res_7_noise; 
        //if (tryGetClientList(test_res_7, msg_7_noise.first, msg_7_noise.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_7);
        //    print(test_res_7);
        //    count_pass++;
        //}
        //if (!tryGetClientList(test_res_7_noise, msg_7.first, msg_7.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_7);
        //    print(test_res_7_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_8" << endl;
        //auto msg_8 = getFileMsg(1, test_8,"",0);
        //auto msg_8_noise = getFileMsg(1, test_8,"",0);
        //addNoise(msg_8_noise.first, msg_8_noise.second, 1);
        //pair<string,string> test_res_8; 
        //pair<string,string> test_res_8_noise; 
        //if (tryGetFile(test_res_8, msg_8.first, msg_8.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_8);
        //    print(test_res_8);
        //    count_pass++;
        //}
        //if (!tryGetFile(test_res_8_noise, msg_8_noise.first, msg_8_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_8);
        //    print(test_res_8_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_9" << endl;
        //auto msg_9 = getFileMsg(1, test_9, "",0);
        //auto msg_9_noise = getFileMsg(1, test_9, "",0);
        //addNoise(msg_9_noise.first, msg_9_noise.second, 1);
        //pair<string,string> test_res_9; 
        //pair<string,string> test_res_9_noise; 
        //if (tryGetFile(test_res_9, msg_9.first, msg_9.second)) {
        //    cout << "Success:" <<endl;
        //    print(test_9);
        //    print(test_res_9);
        //    count_pass++;
        //}
        //if (!tryGetFile(test_res_9_noise, msg_9_noise.first, msg_9_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    print(test_9);
        //    print(test_res_9_noise);
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_ok" << endl;
        //auto msg_ok = getOkMsg(1, "",0);
        //auto msg_ok_noise = getOkMsg(1,"",0);
        //addNoise(msg_ok_noise.first, msg_ok_noise.second, 1);
        //int test_res_ok = 100;
        //int test_res_ok_noise = 100;
        //if (tryGetStatus(test_res_ok, msg_ok.first, msg_ok.second)) {
        //    cout << "Success:" <<endl;
        //    cout << test_res_ok << endl;
        //    count_pass++;
        //}
        //if (!tryGetStatus(test_res_ok_noise, msg_ok_noise.first, msg_ok_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    cout << test_res_ok_noise << endl;
        //    count_fail++;
        //}
        //cout << endl;
        //cout << "test_fail" << endl;
        //auto msg_fail = getFailMsg(1,"",0);
        //auto msg_fail_noise = getFailMsg(1,"",0);
        //addNoise(msg_fail_noise.first, msg_fail_noise.second, 1);
        //int test_res_fail = 100;
        //int test_res_fail_noise = 100;
        //if (tryGetStatus(test_res_fail, msg_fail.first, msg_fail.second)) {
        //    cout << "Success:" <<endl;
        //    cout << test_res_fail << endl;
        //    count_pass++;
        //}
        //if (!tryGetStatus(test_res_fail_noise, msg_fail_noise.first, msg_fail_noise.second)) {
        //    cout << "Noise:" <<endl;
        //    cout << test_res_fail_noise << endl;
        //    count_fail++;
        //}
        //cout << endl;
        cout << "Passed " << count_pass << " tests." << endl;
        cout << "Failed " << count_fail<< " tests." << endl;
    }
};
/*******************MsgSt and Message: Wrapper Class for SendableData(end)********************/
//
//
/*******************Mutex For Protecting Shared Data (start)***********************************/
mutex output_mutex;
mutex my_mark_ip_mutex;
mutex ip_list_mutex;
mutex client_data_mutex;
mutex global_info_mutex;
mutex msgst_mutex;
mutex download_o_list_mutex;
/*******************Mutex For Protecting Shared Data (end)************************************/



struct LocalInfo {
public:
    bool if_delay;
    bool if_timeout;
    double error_rate;
    LocalInfo(): if_delay(false), if_timeout(false), error_rate(0) {}
    LocalInfo(bool if_delay_t, bool if_timeout_t, double error_rate_t = 0): if_delay(if_delay_t), if_timeout(if_timeout_t), error_rate(error_rate_t) {}

};
class Helper {
public:
    static vector<string> getInetIp(string & mark_ip, int mark_port, int mark_id) {
        vector<string> ip_list;
        char search0[]{"inet addr:"};
        stringstream ss;
        ss << "ip_" << mark_port << "_" << mark_id << ".txt";
        string filename = ss.str();
        stringstream ss2;
        ss2 << "ifconfig > " << filename;
        system(ss2.str().c_str());
        string filepath = "./" + filename;
        struct stat filestat;
        if (stat(filepath.c_str(), &filestat) || S_ISDIR(filestat.st_mode)) { 
            perror("can't read ip file.");
            return ip_list;
        }
        ifstream ip_file;
        ip_file.open(filepath);
        if (ip_file.is_open()) {
            while (!ip_file.eof()) {
                int offset;
                string line;
                getline(ip_file, line);
                if ((offset = line.find(search0, 0)) != string::npos) {
                    int start = offset+strlen(search0);
                    int offset2 = line.find(" ", start);
                    ip_list.push_back(line.substr(start,offset2-start));
                    if (ip_list[ip_list.size()-1] != "127.0.0.1") {
                        mark_ip = ip_list[ip_list.size()-1];
                    }
                }
            }
            ip_file.close();
        }
        remove(filename.c_str());
        return ip_list;
    }
    static bool udpListen(int &fd, MsgSt &st, GlobalInfo &global, LocalInfo &local) {
        //consider time out
        struct sockaddr_in my_addr, rem_addr;
        memset((void*)&my_addr, 0, sizeof(my_addr));
        string rem_ip; int rem_port;
        socklen_t rem_len = sizeof(rem_addr);
        st.msg_len = recvfrom(fd, st.msg, MSG_LEN_MAX, 0, (struct sockaddr *)&rem_addr, &rem_len);

        //obtain binded ip and port
        socklen_t my_addr_len = sizeof(my_addr);
        if (getsockname(fd, (struct sockaddr*)&my_addr, &my_addr_len) <0) {
            perror("get sock name and port fails.");
            return false;
        }

        if (local.if_timeout) {
            struct timeval tv;
            //global_info_mutex.lock();
            tv.tv_sec = global.getTimeOutMax();
            //global_info_mutex.unlock();
            tv.tv_usec = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                perror("set timeout fails.");
                return false;
            } else {
                //if(st.request != REQUEST_PIN)
                //cout << "set time out for " << tv.tv_sec << " s" << endl;
            }
        }

        //output_mutex.lock();
        //cout << "temporarilly listen on (" << inet_ntoa(my_addr.sin_addr) << ", " << ntohs(my_addr.sin_port) << ")" << endl; 
        //output_mutex.unlock();

        Message::setMsgStMeta(st);//update o_mark_ip, o_mark_port, o_mark_id, status, request,if_decodable
        if (st.msg_len > 0) {
            Message::setMsgStMeta(st);
            Message::setMsgStO(st, inet_ntoa(rem_addr.sin_addr), ntohs(rem_addr.sin_port)); 
            return true;
        }
        return false;
    }
    static void udpSocketPrint(int &fd) {
        struct sockaddr_in my_addr;
        memset((void*)&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family=AF_INET;
        my_addr.sin_port=htons(0);
        my_addr.sin_addr.s_addr=htonl(INADDR_ANY);

        //obtain binded ip and port
        socklen_t my_addr_len = sizeof(my_addr);
        if (getsockname(fd, (struct sockaddr*)&my_addr, &my_addr_len) <0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("get sock name and port fails.");
            return ;
        }
        close(fd);
    }

    //create the socket and bind it to current machine ip list and a random port
    //use getsockname to find out the random port
    static bool udpSocketCreate(int &fd) {
        return true;
    }

    //open a new socket for send
    //directly send via this socket
    //delay and set time out accordingly
    static bool udpSend(int &fd, MsgSt &st, GlobalInfo &global, LocalInfo & local) {
        if ((fd=socket(AF_INET,SOCK_DGRAM,0))<0) {
            perror("failed create fd (Server)");
            return false;
        }
        struct sockaddr_in my_addr, rem_addr;
        memset((void*)&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family=AF_INET;
        my_addr.sin_port=htons(0);
        my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        memset((void*)&rem_addr, 0, sizeof(rem_addr));
        rem_addr.sin_family=AF_INET;
        rem_addr.sin_port=htons(st.t_port);
        inet_pton(rem_addr.sin_family, st.t_ip.c_str(), &rem_addr.sin_addr.s_addr);

        //bind to current ip list and a random port
        if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed bind fd (Client).");
            return false;
        }
        int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("set socket reusable failed");
            return false;
        }

        //obtain binded ip and port
        socklen_t my_addr_len = sizeof(my_addr);
        if (getsockname(fd, (struct sockaddr*)&my_addr, &my_addr_len) <0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("get sock name and port fails.");
            return false;
        }


        //output_mutex.lock();
        //cout << "inside udp send: " << endl;
        //udpSocketPrint(fd);
        //output_mutex.unlock();
        Message::setMsgStO(st, inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port)); 
        addNoise(st.msg, st.msg_len, local.error_rate);
        //delay
        if (local.if_delay) {
            int x = st.o_mark_id;
            int y = st.t_mark_id;
            if (x <= 0 || y <= 0 ){
                //lock_guard<mutex> lock(output_mutex);
                if (st.request != REQUEST_PIN) 
                perror("error reading latency file.");
                return false;
            }
            //o_ip
            //global_info_mutex.lock();
            int l = global.getLatency(x, y);
            //output_mutex.lock();
            if (st.request != REQUEST_PIN) 
            cout << "sleeped for " << l << " ms before sending msg" << endl;
            //output_mutex.unlock();
            Time::sleep((double)l / 1000);

            //global_info_mutex.unlock();
        }
        if (local.if_timeout) {
            struct timeval tv;
            //global_info_mutex.lock();
            tv.tv_sec = global.getTimeOutMax();
            //global_info_mutex.unlock();
            tv.tv_usec = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                if (st.request != REQUEST_PIN) 
                perror("set timeout fails.");
                return false;
            } else {
                if (st.request != REQUEST_PIN) 
                cout << "set time out for " << tv.tv_sec << " s" << endl;
            }
        }

        //cout << "inside udp send 3" << endl; 
        //send_time_out
        if (sendto(fd,st.msg,st.msg_len,0,(struct sockaddr*)&rem_addr,sizeof(rem_addr)) < 0) {
            if (st.request != REQUEST_PIN) 
            perror("send message fails (client)");
            return false;
        }
        if (st.request != REQUEST_PIN)  {
            cout << "Successfully send the message from (" << st.o_ip << "," << st.o_port << ") to (" << st.t_ip << ":" << st.t_port << ")" << endl;
        }
        return true;
    }
};


class Server {
private:
    ClientData client_data;
    GlobalInfo global_info;
    string my_mark_ip;
    vector<string> ip_list;
    atomic<int> my_mark_port;
    atomic<int> my_mark_id;
public:
    vector<vector<int>> getLatencyTableFromFile(string filename) {
        string filepath = "./" + filename; 
        vector<vector<int>> latency_table;
        latency_table.push_back(vector<int>()); //0->0...n
        struct stat filestat;
        if (stat(filepath.c_str(), &filestat) || S_ISDIR(filestat.st_mode)) { 
            perror("can't read latency file.");
            return latency_table;
        }
        ifstream fin;
        fin.open(filepath.c_str());
        if (fin.is_open()) {
            int prevx = 0;
            int x, y, value;
            while (fin >> x >> y >> value) { 
                if (x > prevx) {
                    prevx = x; 
                    latency_table.push_back(vector<int>());
                    latency_table[latency_table.size()-1].push_back(0); //1->0
                //1->0
                }
                latency_table[latency_table.size()-1].push_back(value);
            }
            fin.close();
        }
        //0,1 .... 0..n
        for (int i = 0; i < latency_table.size(); i++) {
            latency_table[0].push_back(0);
        }
        return latency_table;
    }
    //undecodable message for public listening port
    //can't determine which request
    int undecodableRecv(const MsgSt &msgst) {
        int fd;
        my_mark_ip_mutex.lock();
        MsgSt msgst_s = Message::getFailMsgSt(NONE, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
        my_mark_ip_mutex.unlock();
        LocalInfo local_s(false, true);
        Helper::udpSend(fd, msgst_s, global_info, local_s);
        close(fd);
        return FAIL;
    }

    /********************pin receiver operation (start)*********************/ 
    int pinReceiptCheck(MsgSt &msgst) {
        return OK;
    }
    void pinRecvLogic(const MsgSt & msgst) {
        //lock_guard<mutex> lock(output_mutex);
        //cout << "receive pin." << endl;
    }
    int pinRecv(const MsgSt &msgst) {
        //if (msgst.if_decodable == true && msgst.status == FAIL) {
        //    lock_guard<mutex> lock(output_mutex);
        //    cout << "Received a failed request in pinRecv, don't do anything." << endl;
        //    return FAIL;
        //}
        //MsgSt msgst_s;
        int fd;
        //if (msgst.if_decodable == true && msgst.status == OK) {
            pinRecvLogic(msgst);
            my_mark_ip_mutex.lock();
            MsgSt msgst_s = Message::getOkMsgSt(msgst.request,my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            local_s = LocalInfo(false, true);
            Helper::udpSend(fd, msgst_s, global_info, local_s);
            //lock_guard<mutex> lock(output_mutex);
            //cout << "Send back ok msg after receiving pin." << endl;
            close(fd);
            return OK;
        //}
        //return FAIL;
    }
    /********************pin receiver operation (end)***********************/ 

    /********************register receiver operation (start)****************/ 
    int registerReceiptCheck(MsgSt &msgst) {
        //lock_guard<mutex> lock(output_mutex);
        cout << "into register receipt check." << endl;
        if (msgst.if_decodable == true && msgst.status == OK) {
            cout << "client successfully registerd to tracking server." << endl;
            return OK;
        } else {
            cout << "client doesn't register." << endl;
        }
        return FAIL;
    }
    void registerRecvLogic(const MsgSt & msgst) {
        auto file_name_list = Message::getFileNameList(msgst.msg, msgst.msg_len);
        client_data_mutex.lock();
        client_data.setFileNameList(msgst.o_mark_ip, msgst.o_mark_port, msgst.o_mark_id, file_name_list);
        //cout << "Received client data with lantency table (size " << client_data.getCount() << ") time_out_max : " << client_data.getTimeOutMax();
        client_data.print();
        client_data_mutex.unlock();
    }
    //handle msg decoding error and fail msg
    int registerRecv(const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, don't do anything." << endl;
            return FAIL;
        }
        MsgSt msgst_s;
        int fd;
        if (msgst.if_decodable == true && msgst.status == OK) {
            registerRecvLogic(msgst);
            my_mark_ip_mutex.lock();
            global_info_mutex.lock();
            MsgSt msgst_s = Message::getGlobalInfoMsgSt(msgst.request, global_info, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            global_info_mutex.unlock();
            my_mark_ip_mutex.unlock();
            //msgst_s.t_port = 33000;//deliberately let the send time out 
            //msgst_s = Message::getFailMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            LocalInfo local_s(false, true);
            local_s = LocalInfo(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
                MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
                LocalInfo local_r(false, true);
                if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                    return registerReceiptCheck(msgst_r);
                } else {
                    //time out
                    //lock_guard<mutex> lock(output_mutex);
                    perror("listen final response time out");
                    return FAIL;
                }
            } else {
                //time out
                //lock_guard<mutex> lock(output_mutex);
                perror("send global info time out");
                close(fd);
                return FAIL;
            }
        }
        return FAIL;
    }
    /********************register receiver operation (end)******************/ 

    /********************find receiver operation (start)********************/ 
    int findReceiptCheck(MsgSt &msgst) {
        //lock_guard<mutex> lock(output_mutex);
        if (msgst.if_decodable == true && msgst.status == OK) {
            cout << "client successfully get a client list from tracking server." << endl;
            return OK;
        } else {
            cout << "client doesn't get a client list from tracking server." << endl;
        }
        return FAIL;
    }
    void findRecvLogic(ClientList & client_list, const MsgSt & msgst) {
        string filename = Message::getFileName(msgst.msg, msgst.msg_len);
        client_data_mutex.lock();
        client_data.print();
        client_list = client_data.getClientList(filename);
        client_list.print();
        client_data_mutex.unlock();
    }
    int findRecv(const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, don't do anything." << endl;
            return FAIL;
        }
        MsgSt msgst_s;
        int fd;
        if (msgst.if_decodable == true && msgst.status == OK) {
            ClientList client_list;
            findRecvLogic(client_list, msgst);
            MsgSt msgst_s = Message::getClientListMsgSt(msgst.request, client_list, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            //msgst_s.t_port = 33000;//deliberately let the send time out 
            //msgst_s = Message::getFailMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            LocalInfo local_s(false, true);
            local_s = LocalInfo(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
                MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
                LocalInfo local_r(false, true);
                if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                    return findReceiptCheck(msgst_r);
                } else {
                    //time out
                    //lock_guard<mutex> lock(output_mutex);
                    perror("listen \"find request\"final response time out");
                    return FAIL;
                }
            } else {
                //time out
                //lock_guard<mutex> lock(output_mutex);
                perror("time out when sending client list");
                close(fd);
                return FAIL;
            }
        } 
        return FAIL;
    }
    /********************find receiver operation (end)**********************/ 

    /********************update list receiver operation (start)*************/ 
    int updateListReceiptCheck(MsgSt &msgst) {
        return OK;
    }
    void updateListRecvLogic(const MsgSt & msgst) {
        vector<string> file_name_list = Message::getFileNameList(msgst.msg, msgst.msg_len);
        client_data_mutex.lock();
        client_data.setFileNameList(msgst.o_mark_ip,msgst.o_mark_port,msgst.o_mark_id,file_name_list);
        output_mutex.lock();
        cout << "client data is updated!!!" << endl;
        client_data.print();
        cout << "client data finish update!!!" << endl;
        output_mutex.unlock();
        client_data_mutex.unlock();
    }
    int updateListRecv(const MsgSt &msgst) {
        cout << "into update list recv." << endl;
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed request in updateListRecv, don't do anything." << endl;
            return FAIL;
        }
        MsgSt msgst_s;
        int fd;
        if (msgst.if_decodable == true && msgst.status == OK) {
            updateListRecvLogic(msgst);
            my_mark_ip_mutex.lock();
            MsgSt msgst_s = Message::getOkMsgSt(msgst.request,my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            local_s = LocalInfo(false, true);
            Helper::udpSend(fd, msgst_s, global_info, local_s);
            close(fd);
            return OK;
        }
        return FAIL;
    }
    /********************update list receiver operation (end)***************/ 

    /********************server request listener (start)********************/ 
    void requestListen() {
        int fd;
        struct sockaddr_in my_addr, rem_addr;
        char *msg = new char[MSG_LEN_MAX];
        socklen_t rem_len = sizeof(rem_addr);
        if ((fd=socket(AF_INET,SOCK_DGRAM,0))<0) {
            perror("failed create fd (Server)");
        }
        memset((void*)&my_addr,0,sizeof(my_addr));
        my_addr.sin_family=AF_INET;
        my_addr.sin_port=htons(my_mark_port);
        my_addr.sin_addr.s_addr=htonl(INADDR_ANY);

        if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed bind fd (Server).");
        }

        int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("set socket reusable failed");
            return;
        }

        struct timeval time_val;
        socklen_t time_len = sizeof(time_val);
        if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &time_val, &time_len) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed to preserve time struct for receive.");
            return;
        }

        socklen_t my_addr_len = sizeof(my_addr);
        if (getsockname(fd, (struct sockaddr*)&my_addr, &my_addr_len) <0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("get sock name and port fails.");
            return;
        }
        //update ip_list, my_mark_ip, my_mark_port, my_mark_id
        my_mark_port = ntohs(my_addr.sin_port);
        my_mark_ip_mutex.lock();
        ip_list_mutex.lock();
        ip_list = Helper::getInetIp(my_mark_ip, my_mark_port, my_mark_id); 
        ip_list_mutex.unlock();
        my_mark_ip_mutex.unlock();

        bool keep_quiet = false;
        while (1) {
            output_mutex.lock();
            ip_list_mutex.lock();
            if (!keep_quiet) {
                cout << "Server is listening on ";;
                for (int i = 0; i < ip_list.size(); i++) { 
                    cout << "("<<ip_list[i] << "," << ntohs(my_addr.sin_port) << ") ";
                }
                cout << "..............."<<endl;
            }
            ip_list_mutex.unlock();
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            MsgSt msgst = Message::getMsgStRecv(my_mark_ip, my_mark_port, my_mark_id);
            my_mark_ip_mutex.unlock();
            msgst.msg_len = recvfrom(fd, msgst.msg,MSG_LEN_MAX,0,(struct sockaddr*)&rem_addr, &rem_len);
            if (msgst.msg_len > 0) {
                Message::setMsgStMeta(msgst);
                Message::setMsgStO(msgst, inet_ntoa(rem_addr.sin_addr), ntohs(rem_addr.sin_port));
                output_mutex.lock();
                if (msgst.request != REQUEST_PIN) {
                    keep_quiet = false;
                    cout << "Received a message from ("<<msgst.o_ip<<","<<msgst.o_port<< ")"<< endl;
                } else {
                    keep_quiet = true;
                }
                output_mutex.unlock();
                MsgSt st = Message::deepCopy(msgst);
                if (!st.if_decodable) {
                    //lock_guard<mutex> lock(output_mutex);
                    cout<<"Error msg from ("<<msgst.o_ip<<","<<msgst.o_port<< ")" << endl;
                    thread([=] {undecodableRecv(st);}).detach();
                    //notify sender
                } else {
                    switch (st.request) {
                        case REQUEST_REGISTER: 
                            thread([=]{registerRecv(st);}).detach();
                            break;
                        case REQUEST_SHOW_FILE:
                            //thread([=]{showFileRecv(msg, msg_len);}).detach();
                            break;
                        case REQUEST_FIND:
                            thread([=]{findRecv(st);}).detach();
                            break;
                        case REQUEST_UPDATE_LIST:
                            thread([=]{updateListRecv(st);}).detach();
                            break;
                        case REQUEST_PIN:
                            thread([=]{pinRecv(st);}).detach();
                            break;
                        default:
                            output_mutex.lock();
                            cout << "request is: " << msgst.request << endl;
                            perror("request error (server).");
                            output_mutex.unlock();
                            break;
                    }
                }
            }
        }
        close(fd);
    }
    /********************server request listnener (end)*********************/ 


    /********************server constructor (start)************************/ 
    Server(string latency_file, int time_out_max, int my_mark_port_t) {
        my_mark_id = 0;
        my_mark_port = my_mark_port_t; 
        global_info = GlobalInfo(getLatencyTableFromFile(latency_file), time_out_max);
        vector<thread> thread_list;
        thread_list.push_back(thread([=]{requestListen();}));
        for (int i = 0; i < thread_list.size(); i++) {
            thread_list[i].join();
        }
    }
    /********************server constructor (end)**************************/ 
};

class FileData {
    map<string, string> filemap;
    atomic<int> my_mark_id; 
public:
    void print() {
        for (auto pair: filemap) {
            cout << pair.first << ":\n" << pair.second << "\n" << endl;
        }
    }
    bool has(string filename) {
        return filemap.find(filename) != filemap.end();
    }
    int storeFile( const pair<string, string> &file) {
        return DataManager::storeFile(my_mark_id, file.first, file.second);
    }
    vector<pair<string, string>> getDirFileList(int id) {
        struct dirent *ptr;
        struct stat filestat;
        DIR *dir;
        //cout << "id: " << id << endl;
        stringstream ss;
        ss << "./" << id;
        string path = ss.str();
        //cout << "path: " << path << endl;
        vector<pair<string, string>> filelist;
        vector<string> filename_list;
        if ((dir = opendir(path.c_str())) == NULL) {
            perror("can't open directory:");
        }
        while ((ptr=readdir(dir)) != NULL) {
            string filepath = path + "/" + ptr->d_name;
            if (stat(filepath.c_str(), &filestat)) continue; //directory
            if (S_ISDIR(filestat.st_mode)) continue;
            if (ptr->d_name == "."|| ptr->d_name == "..") continue;
            if (endsWith(Format::toStr(ptr->d_name), ".txt")) {
                ifstream fin;
                stringstream ss;
                fin.open(filepath.c_str());
                if (fin.is_open()) {
                    string content;
                    while (getline(fin, content)) {
                        ss << content;
                    }
                    filelist.push_back(make_pair(ptr->d_name, ss.str()));
                    output_mutex.lock();
                    //cout << filelist[filelist.size()-1].first << "->\n" << filelist[filelist.size()-1].second << "\n" << endl;
                    output_mutex.unlock();
                    fin.close();
                }
            }
        }
        closedir(dir);
        return filelist;
    }

    FileData() {}
    void init(int mark_id) {
        filemap.clear();
        vector<pair<string, string>> file_list = getDirFileList(mark_id);
        for (int i = 0; i < file_list.size(); i++) {
            addFile(file_list[i], false);
        }
        my_mark_id = mark_id;
    }

    FileData(int mark_id) {
        init(mark_id);
    }
    void addFile(pair<string, string> & file, bool if_store = true) {
        filemap[file.first] = file.second;
        if (if_store) storeFile(file);
    }
    string getFileContent(string name) { 
        return filemap[name];
    }
    pair<string, string> getFile(string name) { 
        return *filemap.find(name);
    }
    void removeFile(pair<string, string> & file) {
        auto p = filemap.find(file.first);
        if (p != filemap.end()) {
            filemap.erase(p);
        }
    }
    vector<string> toFileNameList() {
        vector<string> res;
        for (auto pair: filemap) {
            res.push_back(pair.first);
        }
        return res;
    }
    vector<pair<string, string>> toFileList() {
        vector<pair<string,string>> res;
        for (auto pair: filemap) {
            res.push_back(pair);
        }
        return res;
    }
    static void test() {
        FileData d(1);
        pair<string, string> m = make_pair("z.txt", "zzz");
        pair<string, string> m2 = make_pair("z2.txt", d.getFileContent("a.txt"));
        d.addFile(m);
        d.addFile(m2);
        m = make_pair("z.txt", "z3z");
        d.addFile(m);
        d.print();
    }
};

mutex file_data_mutex;
mutex server_ip_mutex;

//
class Client {
private:
    vector<int> download_o_list;
    atomic<double> total_time;
    atomic<int> total_download;
    atomic<int> error_rate;
    atomic<bool> if_need_register;
    string server_ip; 
    atomic<int> server_port;
    string my_mark_ip="";
    atomic<int> my_mark_port;
    atomic<int> my_mark_id;
    atomic<int> load;
    vector<string> ip_list;
    FileData file_data;
    GlobalInfo global_info;
public:
    //check whether center is alive period
    void checkServerAlivePeriod() {
        while (1) {
            Time::sleep(3);
            //cout << "Current load: " << load << endl;
            if (pinSend() == OK) {
                if (if_need_register) registerSend(); //register again if needed
            } else {
                if_need_register = true;
            }
        }
    }

    /********************client request listener (start)********************/ 
    void requestListen() {
        int fd;
        struct sockaddr_in my_addr, rem_addr;
        char *msg = new char[MSG_LEN_MAX];
        socklen_t rem_len = sizeof(rem_addr);
        if ((fd=socket(AF_INET,SOCK_DGRAM,0))<0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed create fd (Server)");
            return;
        }
        memset((void*)&my_addr,0,sizeof(my_addr));
        my_addr.sin_family=AF_INET;
        my_addr.sin_port=htons(0);
        my_addr.sin_addr.s_addr=htonl(INADDR_ANY);

        if (bind(fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed bind fd (Server).");
            return;
        }

        int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            perror("set socket reusable failed");
            return;
        }

        struct timeval time_val;
        socklen_t time_len = sizeof(time_val);
        if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &time_val, &time_len) < 0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("failed to preserve time struct for receive.");
            return;
        }

        socklen_t my_addr_len = sizeof(my_addr);
        if (getsockname(fd, (struct sockaddr*)&my_addr, &my_addr_len) <0) {
            //lock_guard<mutex> lock(output_mutex);
            perror("get sock name and port fails.");
            return;
        }
        my_mark_port = ntohs(my_addr.sin_port);
        my_mark_ip_mutex.lock();
        ip_list_mutex.lock();
        ip_list = Helper::getInetIp(my_mark_ip, my_mark_port, my_mark_id); 
        ip_list_mutex.unlock();
        my_mark_ip_mutex.unlock();

        while (1) {
            output_mutex.lock();
            cout << "Client is listening on ";;
            ip_list_mutex.lock();
            for (int i = 0; i < ip_list.size(); i++) { 
                cout << "("<<ip_list[i] << "," << my_mark_port << ") ";
            }
            ip_list_mutex.unlock();
            cout << "..............."<<endl;
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            MsgSt msgst = Message::getMsgStRecv(my_mark_ip, my_mark_port, my_mark_id);
            my_mark_ip_mutex.unlock();
            msgst.msg_len = recvfrom(fd, msgst.msg, MSG_LEN_MAX,0,(struct sockaddr*)&rem_addr, &rem_len);
            if (msgst.msg_len > 0) {
                Message::setMsgStMeta(msgst);
                Message::setMsgStO(msgst, inet_ntoa(rem_addr.sin_addr), ntohs(rem_addr.sin_port));
                output_mutex.lock();
                cout << "Received a message from (" << msgst.o_ip << "," << msgst.o_port<< ")" << endl;
                output_mutex.unlock();
                MsgSt st = Message::deepCopy(msgst);
                if (!st.if_decodable) {
                    //lock_guard<mutex> lock(output_mutex);
                    cout << "Message error from (" << msgst.o_ip<< "," << msgst.o_port<< ")" << endl;
                    thread([=] {undecodableRecv(st);}).detach();
                } else {
                    switch (st.request) {
                        case REQUEST_GET_LOAD: 
                            thread([=]{getLoadRecv(st);}).detach();
                            break;
                        case REQUEST_DOWNLOAD:
                            thread([=]{downloadRecv(st);}).detach();
                            break;
                        case REQUEST_PIN:
                            //thread([=]{pinRecv(msgst);}).detach();
                        case REQUEST_GET_GLOBAL_INFO:
                            //thread([=]{getGlobalInfoRecv(msgst);}).detach();
                        default:
                            perror("request error (client).");
                            break;
                    }
                }
            }
        }
        close(fd);
    }
    /********************client request listener (end)**********************/ 


    /********************unknow request receiver operation (start)***********/ 
    int undecodableRecv(const MsgSt &msgst) {
        int fd;
        my_mark_ip_mutex.lock();
        MsgSt msgst_s = Message::getFailMsgSt(NONE, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
        my_mark_ip_mutex.unlock();
        LocalInfo local_s(false, true);
        Helper::udpSend(fd, msgst_s, global_info, local_s);
        close(fd);
        return FAIL;
    }
    /********************unknow request receiver operation (end)************/ 

    /********************pin sender operation (start)***********************/ 
    int pinReceiptLogic(const MsgSt &msgst) {
        return OK;
    }
    int pinReceipt(const MsgSt &msgst) {
        //pin is ok no matther msgst if FAIL or undecodable since the target responsed
        return OK;
    }
    int pinSend() {
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        my_mark_ip_mutex.lock();
        server_ip_mutex.lock();
        auto msgst_s = Message::getOkMsgSt(REQUEST_PIN, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        server_ip_mutex.unlock();
        my_mark_ip_mutex.unlock();
        LocalInfo local(false, true); //1->send error
        if (Helper::udpSend(fd, msgst_s, global_info, local)) {
        } else {
            //lock_guard<mutex> lock(output_mutex);
            //perror("send pin request fail");
            close(fd);
            return FAIL;
        }
        MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
        LocalInfo local_r(false, true);
        if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
            return pinReceipt(msgst_r);
        } else {
            //lock_guard<mutex> lock(output_mutex);
            //perror("listen to server pin response time out");
            return FAIL;
        }
        return FAIL;
    }
    /********************pin sender operation (end)*************************/ 

    /********************register sender operation (start)******************/ 
    int registerReceiptLogic(const MsgSt &msgst) {
        global_info = Message::getGlobalInfo(msgst.msg, msgst.msg_len);
        cout << "Received global info with size : " << global_info.getCount() << " time_out_max: " << global_info.getTimeOutMax() << endl;
        global_info.print();
        cout << "Check latecy:" << global_info.getLatency(4, 8) << endl;
        if_need_register = false;
        download_o_list_mutex.lock();
        download_o_list.clear();
        for (int i = 0; i < global_info.getCount(); i++) { 
            download_o_list.push_back(0);
        }
        download_o_list_mutex.unlock();
        return OK;
    }
    int registerReceipt(const MsgSt &msgst) {
        //stops
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, there might be transmission error." << endl;
            return FAIL;
        }
        int fd;
        MsgSt msgst_s;
        if (msgst.if_decodable == true && msgst.status == OK) {
            int res = registerReceiptLogic(msgst);
            my_mark_ip_mutex.lock();
            msgst_s = Message::getOkMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final ok response time out");
            }
            return res;
        } else if (msgst.if_decodable == false) {
            output_mutex.lock();
            cout << "msgst is not decodable in register receipt." << endl;
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            msgst_s = Message::getFailMsgSt(REQUEST_REGISTER, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final fail response time out");
                return FAIL;
            }
        }
        //no listen, mannually close
        close(fd);
        return FAIL; 
    }
    int registerSend() {
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        file_data_mutex.lock();
        vector<string> filename_list = file_data.toFileNameList();
        file_data_mutex.unlock();
        my_mark_ip_mutex.lock();
        server_ip_mutex.lock();
        auto msgst_s = Message::getFileNameListMsgSt(REQUEST_REGISTER, filename_list, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        server_ip_mutex.unlock();
        my_mark_ip_mutex.unlock();
        auto file_name_list = Message::getFileNameList(msgst_s.msg, msgst_s.msg_len);
        output_mutex.lock();
        cout << "print file name list :" << endl;
        Message::print(file_name_list);
        output_mutex.unlock();
        //msgst_s = Message::getFailMsgSt(1, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        //cout << "request: " << msgst_s.request << endl;
        //
        //block waiting for server
        LocalInfo local(false, true); //1->send error

        //block
        while (1) {
            if (Helper::udpSend(fd, msgst_s, global_info, local)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("register: send file name list fail");
                close(fd);
                return FAIL;
            }
            MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
            LocalInfo local_r(false, true);
            if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                return registerReceipt(msgst_r);
            } else {
                if_need_register = true;
                //lock_guard<mutex> lock(output_mutex);
                perror("listen global info fail");
            }
        }
        return FAIL;
    }
    /********************register sender operation (end)********************/ 

    /********************find sender operation (start)**********************/ 
    int findReceiptLogic(ClientList & client_list, const MsgSt &msgst) {
        client_list = Message::getClientList(msgst.msg, msgst.msg_len);
        //client_list.print();
        if (client_list.getCount() == 0) return FAIL;
        return OK;
    }

    int findReceipt(ClientList & client_list, const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, there might be transmission error." << endl;
            return FAIL;
        }
        int fd;
        MsgSt msgst_s;
        if (msgst.if_decodable == true && msgst.status == OK) {
            int res = findReceiptLogic(client_list, msgst);
            my_mark_ip_mutex.lock();
            msgst_s = Message::getOkMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final ok response time out");
            }
            return res;
        } else if (msgst.if_decodable == false) {
            output_mutex.lock();
            cout << "msgst is not decodable in get load receipt." << endl;
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            msgst_s = Message::getFailMsgSt(REQUEST_GET_LOAD, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final fail response time out");
            }
            return FAIL;
        }
        close(fd);
        return FAIL;
    }

    int findSend(ClientList &client_list, string filename) {
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        my_mark_ip_mutex.lock();
        server_ip_mutex.lock();
        auto msgst_s = Message::getFileNameMsgSt(REQUEST_FIND, filename, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        server_ip_mutex.unlock();
        my_mark_ip_mutex.unlock();
        auto file_name = Message::getFileName(msgst_s.msg, msgst_s.msg_len);
        output_mutex.lock();
        cout << "print file name:" << file_name << endl;
        output_mutex.unlock();
        //msgst_s = Message::getFailMsgSt(1, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        //cout << "request: " << msgst_s.request << endl;
        //
        //block for server
        LocalInfo local(false, true); //1->send error
        ////block
        while (1) {
            //send and receive
            if (Helper::udpSend(fd, msgst_s, global_info, local)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send file name time out");
                close(fd);
                return FAIL;
            }
            MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
            LocalInfo local_r(false, true);
            if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                return findReceipt(client_list, msgst_r);
            } else {
                if_need_register = true;
                //lock_guard<mutex> lock(output_mutex);
                perror("listen client list time out");
                //return FAIL;
            }
            Time::sleep(2);
        }
        return FAIL;
    }
    /********************find sender operation (end)************************/ 

    /********************get load sender operation (start)******************/ 
    int getLoadReceiptLogic(int &request_load, const MsgSt &msgst) {
        request_load = Message::getLoad(msgst.msg, msgst.msg_len);
        cout <<"load in get load receipt logic: " << request_load << endl;
        return OK;
    }
    int getLoadReceipt(int & request_load, const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, there might be transmission error." << endl;
            return NONE;
        }
        int fd;
        MsgSt msgst_s;
        if (msgst.if_decodable == true && msgst.status == OK) {
            int res = getLoadReceiptLogic(request_load, msgst);
            my_mark_ip_mutex.lock();
            msgst_s = Message::getOkMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(true, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final ok response time out");
            }
            return res;
        } else if (msgst.if_decodable == false) {
            output_mutex.lock();
            cout << "msgst is not decodable in get load receipt." << endl;
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            msgst_s = Message::getFailMsgSt(REQUEST_GET_LOAD, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(true, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final fail response time out");
            }
            return FAIL;
        }
        close(fd);
        return FAIL;
    }
    int getLoadSend(int & request_load, string t_ip, int t_port, int t_mark_id) {
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        my_mark_ip_mutex.lock();
        auto msgst_s = Message::getOkMsgSt(REQUEST_GET_LOAD, my_mark_ip, my_mark_port, my_mark_id, t_ip, t_port, t_mark_id);
        my_mark_ip_mutex.unlock();
        LocalInfo local(true, true); //1->send error
        //udpSend will be delayed
        if (Helper::udpSend(fd, msgst_s, global_info, local)) {
        } else {
            //lock_guard<mutex> lock(output_mutex);
            perror("send get load request time out");
            close(fd);
            return FAIL;
        }
        MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
        LocalInfo local_r(false, true);
        if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
            return getLoadReceipt(request_load, msgst_r);
        } else {
            //lock_guard<mutex> lock(output_mutex);
            perror("listen load time out");
            return FAIL;
        }
        return FAIL;
    }
    /********************get load sender operation (end)********************/ 


    /********************get load receiver operation (start)****************/ 
    int getLoadReceiptCheck(MsgSt &msgst) {
        //lock_guard<mutex> lock(output_mutex);
        if (msgst.if_decodable == true && msgst.status == OK) {
            cout << "requester successfully get the load." << endl;
            return OK;
        } else {
            cout << "requester doesn't get load." << endl;
        }
        return FAIL;
    }
    void getLoadRecvLogic(const MsgSt & msgst) {
        //no logic needed for get load
    }
    int getLoadRecv(const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, don't do anything." << endl;
            return FAIL;
        }
        MsgSt msgst_s;
        int fd;
        if (msgst.if_decodable == true && msgst.status == OK) {
            getLoadRecvLogic(msgst);
            my_mark_ip_mutex.lock();
            MsgSt msgst_s = Message::getLoadMsgSt(msgst.request,load,my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(false, true);
            local_s = LocalInfo(true, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
                MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
                LocalInfo local_r(false, true);
                if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                    return getLoadReceiptCheck(msgst_r);
                } else {
                    //time out
                    //lock_guard<mutex> lock(output_mutex);
                    perror("listen \"get load request\"final response time out");
                    return FAIL;
                }
            } else {
                //time out
                //lock_guard<mutex> lock(output_mutex);
                perror("time out when sending client list");
                close(fd);
                return FAIL;
            }
        }
        return FAIL;
    }
    /********************get load receiver operation (end)******************/ 


    /********************download sender operation (start)******************/ 
    int downloadReceiptLogic(const MsgSt &msgst) {
        auto file = Message::getFile(msgst.msg, msgst.msg_len);
        file_data_mutex.lock();
        file_data.addFile(file);
        file_data_mutex.unlock();
        download_o_list_mutex.lock();
        download_o_list[msgst.o_mark_id] = download_o_list[msgst.o_mark_id] + 1;
        download_o_list_mutex.unlock();
        return OK;
    }
    int downloadReceipt(const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, there might be transmission error." << endl;
            return FAIL;
        }
        int fd;
        MsgSt msgst_s;
        if (msgst.if_decodable == true && msgst.status == OK) {
            int res = downloadReceiptLogic(msgst);
            my_mark_ip_mutex.lock();
            msgst_s = Message::getOkMsgSt(msgst.request, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(true, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final ok response time out");
            }
            return res;
        } else if (msgst.if_decodable == false) {
            output_mutex.lock();
            cout << "msgst is not decodable in download receipt." << endl;
            output_mutex.unlock();
            my_mark_ip_mutex.lock();
            msgst_s = Message::getFailMsgSt(REQUEST_DOWNLOAD, my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            LocalInfo local_s(true, true);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
            } else {
                //lock_guard<mutex> lock(output_mutex);
                perror("send final fail response time out");
            }
            return FAIL;
        }
        close(fd);
        return FAIL;
    }
    int downloadSend(string file_name, string t_ip, int t_port, int t_mark_id) {
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        my_mark_ip_mutex.lock();
        auto msgst_s = Message::getFileNameMsgSt(REQUEST_DOWNLOAD, file_name, my_mark_ip, my_mark_port, my_mark_id, t_ip, t_port, t_mark_id);
        my_mark_ip_mutex.unlock();
        LocalInfo local(true, true); //1->send error
        if (Helper::udpSend(fd, msgst_s, global_info, local)) {
        } else {
            //lock_guard<mutex> lock(output_mutex);
            perror("send download request time out");
            close(fd);
            return FAIL;
        }
        MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
        LocalInfo local_r(false, true);
        if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
            return downloadReceipt(msgst_r);
        } else {
            //lock_guard<mutex> lock(output_mutex);
            perror("listen downloadRecv time out");
            load--;
            return FAIL;
        }
        return FAIL;
    }
    /********************download sender operation (end)********************/ 


    /********************download receiver operation (start)****************/ 
    int downloadReceiptCheck(MsgSt &msgst) {
        //lock_guard<mutex> lock(output_mutex);
        if (msgst.if_decodable == true && msgst.status == OK) {
            cout << "requester successfully get the file." << endl;
            load--;
            return OK;
        } else {
            cout << "requester doesn't get the file." << endl;
        }
        load--;
        return FAIL;
    }

    void downloadRecvLogic(pair<string, string> & file, const MsgSt & msgst) {
        string file_name = Message::getFileName(msgst.msg, msgst.msg_len);
        file_data_mutex.lock();
        file = file_data.getFile(file_name);
        file_data_mutex.unlock();
    }
    int downloadRecv(const MsgSt &msgst) {
        load++;
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, don't do anything." << endl;
            load--;
            return FAIL;
        }
        MsgSt msgst_s;
        int fd;
        if (msgst.if_decodable == true && msgst.status == OK) {
            pair<string, string> file;
            downloadRecvLogic(file, msgst);
            my_mark_ip_mutex.lock();
            MsgSt msgst_s = Message::getFileMsgSt(msgst.request,file,my_mark_ip, my_mark_port, my_mark_id, msgst.o_ip, msgst.o_port, msgst.o_mark_id);
            my_mark_ip_mutex.unlock();
            //contains error in file
            LocalInfo local_s(false, true, error_rate);
            if (Helper::udpSend(fd, msgst_s, global_info, local_s)) {
                MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
                LocalInfo local_r(false, true);
                if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
                    return getLoadReceiptCheck(msgst_r);
                } else {
                    //time out
                    //lock_guard<mutex> nock(output_mutex);
                    perror("listen \"get load request\"final response time out");
                    load--;
                    return FAIL;
                }
            } else {
                //time out
                //lock_guard<mutex> lock(output_mutex);
                perror("time out when sending client list");
                close(fd);
                load--;
                return FAIL;
            }
        }
        load--;
        return FAIL;
    }
    /********************download receiver operation (end)******************/ 


    /********************update list sender operation (start)***************/ 
    int updateListReceiptLogic(const MsgSt &msgst) {
        return OK;
    }
    int updateListReceipt(const MsgSt &msgst) {
        if (msgst.if_decodable == true && msgst.status == FAIL) {
            //lock_guard<mutex> lock(output_mutex);
            cout << "Received a failed msg, there might be transmission error." << endl;
            return FAIL;
        }
        if (msgst.if_decodable == true && msgst.status == OK) {
            return OK;
        //if undecodable -> notify client 
        } else if (msgst.if_decodable == false) {
            //return OK;
            return FAIL;
        }
        return FAIL;
    }
    int updateListSend(vector<string> file_name_list) {
        //cout << "into update list send" << endl;
        load++;
        while (my_mark_ip == "" || my_mark_port == -1) {}
        int fd;
        my_mark_ip_mutex.lock();
        server_ip_mutex.lock();
        auto msgst_s = Message::getFileNameListMsgSt(REQUEST_UPDATE_LIST, file_name_list, my_mark_ip, my_mark_port, my_mark_id, server_ip, server_port, 0);
        server_ip_mutex.unlock();
        my_mark_ip_mutex.unlock();
        //block for server
        LocalInfo local(false, true); //1->send error
        cout << "into update list send 2" << endl;
        if (Helper::udpSend(fd, msgst_s, global_info, local)) {
            cout << "into update list send 2.5" << endl;
        } else {
            ///lock_guard<mutex> lock(output_mutex);
            perror("send udpate list request time out");
            close(fd);
            load--;
            return FAIL;
        }
        //cout << "into update list send 3" << endl;
        MsgSt msgst_r = Message::getMsgStRecv(msgst_s.o_ip, msgst_s.o_port, msgst_s.o_mark_id);
        LocalInfo local_r(false, true);
        //cout << "into update list send 3.5" << endl;
        //block
        if (Helper::udpListen(fd, msgst_r, global_info, local_r)) {
            load--;
            return updateListReceipt(msgst_r);
        } else {
            if_need_register = true;
            perror("listen to updateListRecv time out");
            load--;
            return FAIL;
        }
        //cout << "into update list send 4" << endl;
        load--;
        return FAIL;
    }
    /********************update list sender operation (end)*****************/ 

    /********************user input operation (start)***********************/

    void downloadUIAction(string filename) {
        load++;
        auto t1 = Time::now();
        file_data_mutex.lock();
        bool res = file_data.has(filename);
        file_data_mutex.unlock();
        if (res) {
            cout << "file is already under this client: " << filename << endl;
            Time::show(Time::now()-t1);
        } else {
            file_data_mutex.unlock();
            vector<int> load_list;
            ClientList client_list;
            if (findSend(client_list, filename) == OK) {
                output_mutex.lock();
                cout << "Succefully finding file: " << filename << endl;
                auto t2 = Time::now();
                Time::show(t2-t1);
                output_mutex.unlock();
                for (int i = 0; i < client_list.getCount(); i++) {
                    int loadi = -1;
                    if (getLoadSend(loadi, client_list.getMarkIp(i), client_list.getMarkPort(i), client_list.getMarkId(i)) == OK) {;
                        load_list.push_back(loadi);
                    } else {
                        load_list.push_back(-1);
                    }
                }
                client_list.sort(load_list, my_mark_id, global_info);
                output_mutex.lock();
                cout << "Client after sorting: : " << endl;
                client_list.print();
                output_mutex.unlock();
                bool res = false;
                int node_count = 0;
                for (int i = 0; i < client_list.getCount(); i++) {
                    if (client_list.getLoad(i) == NONE) break;
                    //don't download file from a peer that has same match id 
                    if (client_list.getMarkId(i) == my_mark_id) {
                        continue;
                    }
                    node_count++;
                    string t_ip = client_list.getMarkIp(i);
                    int t_port = client_list.getMarkPort(i);
                    int t_id = client_list.getMarkId(i);
                    if (downloadSend(filename, t_ip, t_port, t_id) == OK) {
                        res = true;
                        output_mutex.lock(); 
                        output_mutex.unlock(); 
                        break;
                    }
                }
                if (res) {
                    //lock_guard<mutex> lock(output_mutex);
                    cout << "Successfully download the file." << endl;
                    total_time = total_time + Time::getMilliDuration(Time::now()-t1);
                    total_download = total_download + 1;
                    output_mutex.lock();
                    cout << "Current average downloading time: " << total_time / total_download << " ms" << endl;
                    cout << "Current download origin list:" << endl;
                    for (int i = 1; i < download_o_list.size(); i++) {
                        if (download_o_list[i] > 0) {
                            cout << "Downloaded " << download_o_list[i] << " files from peer " << i << endl;
                        }
                    }
                    output_mutex.unlock();
                    file_data_mutex.lock();
                    vector<string> file_name_list = file_data.toFileNameList();
                    file_data_mutex.unlock();
                    if (updateListSend(file_name_list) == OK) {
                        //lock_guard<mutex> lock(output_mutex);
                        cout << "Successfully update the new file list to server." << endl;
                    } else {
                        perror("Doesn't update successfully, or already updated but the server response msg contains error");
                    }
                } else {
                    //lock_guard<mutex> lock(output_mutex);
                    cout << "Download file fails." << endl;
                    if (client_list.getCount() == 0) {
                        perror("No peer avaliable for download.");
                    } if (node_count == 0) {
                        perror("Can't get load from avaliable nodes.");
                    } else {
                        perror("Failed after trying all avaliable nodes.");
                    }
                }
            } else {
                //lock_guard<mutex> lock(output_mutex);
                cout << "Failed finding file: " << filename << endl;
                auto t2 = Time::now();
                Time::show(t2-t1);
            }
        }
        load--;
    }


    void showUserScreen() {
        cout << "Please input the command you want to run. " << endl;
        cout << "[1]  CONCURRENT DOWNLOAD FROM x.txt - y.txt (x, y are number)" << endl;
        cout << "[2]  DOWNLOAD" << endl;
        cout << "[3]  SHOW INFO" << endl;
    }

    void showOriginUIAction() {
        cout << "Current average downloading time: " << total_time / total_download << " ms" << endl;
        cout << "Current download origin list:" << endl;
        for (int i = 1; i < download_o_list.size(); i++) {
            if (download_o_list[i] > 0) {
                cout << "Downloaded " << download_o_list[i] << " files from peer " << i << endl;
            }
        }
        cout << endl;
    }


    void userRequestListen() {
        string input, filename, test_str, index1, index2;
        int test;
        while (1) {
            showUserScreen();    
            cin >> input;
            int now;
            try {
                now = Format::toInt(input);
            } catch (std::exception e) {
                perror("Please input a numeric command.");
            }
            switch (now) {
                case 1:
                    cout << "concurret-downloading from x.txt - y.txt (note: x and y should be number)" << endl;
                    cout << "Please input x and y (x < y)" << endl;
                    cin >> index1 >> index2;
                    for (int i = Format::toInt(index1); i <= Format::toInt(index2); i++) {
                        thread([=]{downloadUIAction(DataManager::getTxtName(i));}).detach();
                    }
                    break;
                case 2:
                    cout << "Please input the filename: " << endl;
                    cin >> filename;
                    thread([=]{downloadUIAction(filename);}).detach();
                    break;
                case 3:
                    showOriginUIAction();
                    break;
                default:
                    perror("Please input a correct command.");
                    break;
            }
        }
    }
    /********************user input operation (end)*************************/



    /****************client constructor*************************8***********/
    Client(int my_mark_id_t, string server_ip_t, int server_port_t, double error_rate_t) {
        total_time = 0;
        total_download = 0;
        error_rate = error_rate_t;
        if_need_register = true;
        my_mark_port=0;
        my_mark_id = -1;
        my_mark_id = my_mark_id_t;
        server_ip = server_ip_t;
        server_port = server_port_t;
        file_data.init(my_mark_id);
        load = 0;
        vector<thread> thread_list;
        thread_list.push_back(thread([=]{registerSend();}));
        thread_list.push_back(thread([=]{requestListen();}));
        thread_list.push_back(thread([=]{checkServerAlivePeriod();}));
        thread_list.push_back(thread([=]{userRequestListen();}));
        for (int i = 0; i < thread_list.size(); i++) {
            thread_list[i].join();
        }
    }
};





int main(int argc, char ** argv) {
    int count = 50;
    int file_count = 10;
    //DataManager::generateLatency(count, MODE_ZERO);
    //DataManager::generateLatency(count, MODE_MIN);
    //DataManager::generateLatency(count, MODE_RANDOM);
    //DataManager::generateLatency(count, MODE_MAX);
    //DataManager::generateFolderList(count, MODE_ZERO);
    //DataManager::generateFolderList(count, MODE_SINGLE);
    //DataManager::generateFolderList(count, file_count);
    //DataManager::removeFolderList(count);
    //FileData::test();
    //ClientData::test();
    //Message::test();
    //SendableData::markIdTest();
    //Message::test2();
    //Server *server;
    //Client *client;
    //
    /**************************main program*******************/
    if (argc < 2) {
        perror("Start Server: ./filesys S latency_table_file time_out_max server_port");
        perror("Start Client: ./filesys C matchID server_ip server_port (please makesure that matchId is a sub folder of the running environment.");
        perror("Generate Testing Folder: ./filesys G folder_count file_count");
        perror("Erase Testing Folder: ./filesys R");
    }
    if (argv[1][0] == 'S'|| argv[1][0] == 's') {
        if (argc != 5) {
            perror("Usage: ./filesys S latency_table_file time_out_max server_port");
        }
        thread serverOtherTd = thread([]{cout << "start the server" << endl;});
        cout << "lagency_table_file: " << Format::toStr(argv[2]) << endl;
        thread serverTd = thread([=]{new Server(Format::toStr(argv[2]), Format::toInt(argv[3]), Format::toInt(argv[4]));});
        serverTd.join();
        serverOtherTd.join();
    }
    else if (argv[1][0] == 'C' || argv[1][0] == 'c') {
        if (argc < 5 || argc > 7) {
            perror("Usage: ./filesys C matchID server_ip server_port");
        }
        DataManager::generateFolder(Format::toInt(argv[2]), 0);
        thread clientOtherTd = thread([]{cout << "start the client" << endl;});
        int error_rate = (argc >= 6 ? Format::toInt(argv[5]) : 0);
        thread clientTd = thread([=]{new Client(Format::toInt(argv[2]), Format::toStr(argv[3]), Format::toInt(argv[4]), error_rate);});
        clientTd.join();
        clientOtherTd.join();
    } else if (argv[1][0] == 'G' ) {
        if (argc != 4) {
            perror("Usage: ./filesys G folder_total file_total");
            perror("If file_total == 0 -> Empty Folder");
            perror("If file_total == 1 -> Folder With 1 file");
            perror("If file_total == n -> n Folder With n file for each");
        }
        DataManager::generateFolderList(Format::toInt(argv[2]), Format::toInt(argv[3]));
    } else if (argv[1][0] == 'R') {
        if (argc != 2) {
            perror("Usage: ./filesys R");
        }
        DataManager::removeFolderList();
    } else if (argv[1][0] == 'L') {
        if (argc != 4) {
            perror("Usage: ./filesys L peer_count latency_mode");
            perror("latency mode: 0- latency is all 0");
            perror("latency mode: 1- latency all sets to minimum(same)");
            perror("latency mode: 2- latency are random(not same)");
            perror("latency mode: 3- latency all sets to maximum(same)");
        }
        DataManager::generateLatency(Format::toInt(argv[2]), Format::toInt(argv[3]));
    } else {
        perror("Usage: ./filesys S latency_table_file time_out_max server_port");
        perror("Usage: ./filesys C matchID server_ip server_port (please makesure that matchId is a sub folder of the running environment.");
        perror("Usage: ./filesys G folder_count file_count");
        perror("Usage: ./filesys R folder_count");
        perror("Usage: ./filesys L peer_count latency_mode");
    }

    //vector<vector<int>> latency_table_t = getLatencyTableFromFile("latency_table_1.txt");
    //GlobalInfo info(latency_table_t, 50);
    //info.print();
    return 0;
}
