#include "iostream"
#include "string"
#include "vector"
#include <cstring>

using namespace std;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// extern是一个关键字，它告诉编译器存在着一个变量或者一个函数，
// 如果在当前编译语句的前面中没有找到相应的变量或者函数，也会在当前文件的后面或者其它文件中定义
// 定义print函数
extern "C" {
void asm_print(const char *, const int);
}

void print(const char *ch) {
    asm_print(ch, strlen(ch));
}

vector<string> split(const string& str, const string& delim) {
    vector <string> result;
    if ("" == str) return result;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        result.push_back(s); //存入结果数组
        p = strtok(NULL, d); // 将第一位参数strs设置为NULL
    }

    return result;
}

//定义整个软盘文件区域
int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数

//FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) /*指定按1字节对齐*/

// FAT12数据逻辑组织，通过链表实现
// 可以存储文件，也可以存储目录
class Node {
    string name;                   // 文件名或目录名
    vector<Node*> next;            // 保存当前节点的一级子节点
    string path;                   // 当前文件或目录在文件存储中的路径
    uint32_t fileSize;             // 文件大小
    bool isFile = false;           // 是否是文件
    bool isValue = true;           // 是否是正常节点
    int contentNum = 0;            // 目录数量
    int fileNum = 0;               // 文件数量
    char *content = new char[10000];

public:
    Node() = default;

    // 创建节点
    Node(string name, bool isValue) {
        this->name = name;
        this->isValue = isValue;
    }

    // 创建节点，知道路径
    Node(string name, string path) {
        this->name = name;
        this->path = path;
    }

    // 创建文件节点
    Node(string name, uint32_t fileSize, bool isFile, string path) {
        this->name = name;
        this->fileSize = fileSize;
        this->isFile = isFile;
        this->path = path;
    }

    //得到指向文件内容的指针
    char* getContent() {
        return content;
    }

    // 设置路径
    void setPath (string path) {
        this->path = path;
    }

    // 设置文件名
    void setName (string name) {
        this->name = name;
    }

    // 在root最后添加文件类型的子节点
    void addFileChild (Node *child) {
        this->next.push_back(child); // 在root最后加入一个节点
        this->fileNum++;
    }

    // 在root最后添加目录类型的子节点
    void addContentChild (Node *child) {
        child->addChild(new Node(".", false));
        child->addChild(new Node("..", false));
        this->next.push_back(child);
        this->contentNum++;
    }

    void addChild(Node *child) {
        this->next.push_back(child);
    }

    // 获取路径
    string getPath() {
        return this->path;
    }

    // 获取文件名
    string getName() {
        return this->name;
    }

    // 判断是否是文件
    bool judgeFile() const {
        return isFile;
    }

    // 获取当前节点中的一级子文件+一级子目录的个数
    vector<Node*> getNext() {
        return next;
    }

    // 判断是否是有效节点
    bool judgeValue() {
        return this->isValue;
    }

    // 获取文件大小
    uint32_t getFileSize() {
        return this->fileSize;
    }
};


// 引导扇区
class BPB {
    uint16_t BPB_BytsPerSec;    // 每扇区字节数
    uint8_t BPB_SecPerClus;     // 每簇扇区数
    uint16_t BPB_RsvdSecCnt;    // Boot记录占用的扇区数
    uint8_t BPB_NumFATs;		// FAT表个数
    uint16_t BPB_RootEntCnt;    // 根目录最大文件数
    uint16_t BPB_TotSec16;      // 逻辑扇区总数
    uint8_t BPB_Media;          // 媒体描述符
    uint16_t BPB_FATSz16;       // FAT扇区数
    uint16_t BPB_SecPerTrk;     // 每个磁道扇区数
    uint16_t BPB_NumHeads;      // 磁头数
    uint32_t BPB_HiddSec;       // 隐藏扇区数
    uint32_t BPB_TotSec32;      // 如果BPB_FATSz16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12);
};

// BPB的初始化
void BPB::init(FILE *fat12) {
    fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
    fread(this, 1, 25, fat12); //BPB长度为25字节

    BytsPerSec = this->BPB_BytsPerSec; //初始化各个全局变量
    SecPerClus = this->BPB_SecPerClus;
    RsvdSecCnt = this->BPB_RsvdSecCnt;
    NumFATs = this->BPB_NumFATs;
    RootEntCnt = this->BPB_RootEntCnt;

    if (this->BPB_FATSz16 != 0){
        FATSz = this->BPB_FATSz16;
    }
    else{
        FATSz = this->BPB_TotSec32;
    }
    fatBase = RsvdSecCnt * BytsPerSec; // FAT1的偏移字节=Boot记录占用的扇区数*每扇区字节数
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
    dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
}

//根目录条目
class RootEntry {
    char DIR_Name[11];      //文件名8字节，扩展名3字节
    uint8_t DIR_Attr;       //文件属性
    char reserved[10];      //保留位
    uint16_t DIR_WrtTime;   // 最后一次写入时间
    uint16_t DIR_WrtDate;   //最后一次写入日期
    uint16_t DIR_FstClus;   //文件开始的簇号
    uint32_t DIR_FileSize;  //文件大小
public:
    RootEntry() {};

// 初始化根目录，同时将读取的整个文件写入到Node中
    void initRootEntry(FILE *fat12, Node *root) {
        int base = fileRootBase;
        char realName[12];

        for (int i = 0; i < RootEntCnt; ++i) {
            fseek(fat12, base, SEEK_SET);
            fread(this, 1, 32, fat12);

            base += 32;

            if (isEmptyName() || isInvalidName()) continue;

            if (this->isFile()) { // 读到文件时
                generateFileName(realName); // 将DIR_NAME放进realName中
                Node *child = new Node(realName, this->DIR_FileSize, true, root->getPath()); // 一开始getPath为空
                root->addFileChild(child);
                RetrieveContent(fat12, this->DIR_FstClus, child);
            } else { // 读到目录时
                generateDirName(realName);
                Node *child = new Node();
                child->setName(realName); // 设置目录名
                child->setPath(root->getPath() + realName + "/");
                root->addContentChild(child);
                readChildren(fat12, this->getFstClus(), child); // 此时根节点变成child节点
            }
        }
    }

    // 递归将读取到的整个文件写入到Node节点当中
    // 广度优先添加
    void readChildren(FILE *fat12, int startClus, Node *root) {

        int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

        int currentClus = startClus;
        int value = 0; //得到当前文件的地址,以int形式存储地址
        while (value < 0xFF8) {
            value = getFATValue(fat12, currentClus);
            if (value == 0xFF7) {
                print("ERROR");
                //cout << "ERROR";
                break;
            }

            int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;

            int size = SecPerClus * BytsPerSec; //一个簇的大小 = 一个簇的扇区数*扇区占字节数
            int loop = 0;
            while (loop < size) {
                RootEntry *rootEntry = new RootEntry();
                fseek(fat12, startByte + loop, SEEK_SET);
                fread(rootEntry, 1, 32, fat12);

                loop += 32; // 每一个根目录区大小为32字节

                if (rootEntry->isEmptyName() || rootEntry->isInvalidName()) { // 根目录为空名字或者名字不合法
                    continue;
                }

                char tmpName[12];
                if ((rootEntry->isFile())) { //是文件
                    rootEntry->generateFileName(tmpName); // 将得到的文件名存储到tmpName中
                    Node *child = new Node(tmpName, rootEntry->getFileSize(), true, root->getPath()); // 此时getPath为子节点中的路径，如NJU/
                    root->addFileChild(child);
                    RetrieveContent(fat12, rootEntry->getFstClus(), child);
                } else { //是目录
                    rootEntry->generateDirName(tmpName);
                    Node *child = new Node(); // 一开始就设定为目录
                    child->setName(tmpName);
                    child->setPath(root->getPath() + tmpName + "/");
                    root->addContentChild(child);
                    readChildren(fat12, rootEntry->getFstClus(), child);
                }
            }
        }
    }

    // 判断文件名的封装函数
    bool isValidNameAt(int i) {
        return ((this->DIR_Name[i] >= 'a') && (this->DIR_Name[i] <= 'z'))
               ||((this->DIR_Name[i] >= 'A') && (this->DIR_Name[i] <= 'Z'))
               ||((this->DIR_Name[i] >= '0') && (this->DIR_Name[i] <= '9'))
               ||((this->DIR_Name[i] == ' '));
    }

    bool isEmptyName() {
        return this->DIR_Name[0] == '\0'; // \0作为名字的结束符
    }

    // 判断是否是合法的文件名
    bool isInvalidName() {
        int invalid = false;
        for (int k = 0; k < 11; ++k) {
            if (!this->isValidNameAt(k)) {
                invalid = true;
                break;
            }
        }
        return invalid;
    }

    // 判断是否是文件
    bool isFile() {
        return (this->DIR_Attr & 0x10) == 0;
    }

    // 根据根目录中的文件名，将文件名存到参数中，从而得到文件名
    void generateFileName(char name[12]) {
        int tmp = -1;
        for (int i = 0; i < 11; ++i) {
            if (this->DIR_Name[i] != ' ') {
                name[++tmp] = this->DIR_Name[i];
            } else {
                name[++tmp] = '.';
                while (this->DIR_Name[i] == ' ') i++;
                i--;
            }
        }
        ++tmp;
        name[tmp] = '\0'; // 作为名字的结束符
    }

    // 创建目录名
    void generateDirName(char name[12]) {
        int tmp = -1;
        for (int i = 0; i < 11; ++i) {
            if (this->DIR_Name[i] != ' ') {
                name[++tmp] = this->DIR_Name[i];
            } else {
                tmp++;
                name[tmp] = '\0'; // 作为名字的结束符
                break;
            }
        }
    }

    // 得到文件名
    uint32_t getFileSize() {
        return DIR_FileSize;
    }

    //  得到文件开始的簇号
    uint16_t getFstClus() { return DIR_FstClus; }

    // 得到文件对象存储的地址
    int getFATValue(FILE *fat12, int num) { //num:当前簇
        int base = RsvdSecCnt * BytsPerSec;
        int pos = base + num * 3 / 2; //偏移量???????????
        int type = num % 2;

        uint16_t bytes; //存储对象的地址
        uint16_t *bytesPtr = &bytes; //存储对象的指针
        fseek(fat12, pos, SEEK_SET); // seek(fp,100L,0);把文件内部 指针 移动到离文件开头100字节处；
        fread(bytesPtr, 1, 2, fat12); //先移动文件的内部指针，再从内部指针开始读取文件

        if (type == 0) {
            bytes = bytes << 4; // ???????????????????????????????
        }
        return bytes >> 4;
    }

    // 加载文件内容到node节点中，并且只加载一次
    void RetrieveContent(FILE *fat12, int startClus, Node *child) {
        // 得到文件数据的起始地址
        int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
        int currentClus = startClus;
        int value = 0;
        char *pointer = child->getContent(); // 指向文件内容的指针

        if (startClus == 0) return;

        while (value < 0xFF8) {
            value = getFATValue(fat12, currentClus);
            if (value == 0xFF7) {
                print("ERROR");
                //cout << "ERROR";
                break;
            }

            int size = SecPerClus * BytsPerSec;
            char *str = (char*)malloc(size);
            char *content = str;
            int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;

            fseek(fat12, startByte, SEEK_SET);
            fread(content, 1, size, fat12);

            for (int i = 0; i < size; ++i) {
                *pointer = content[i];
                pointer++;
            }
            free(str);
            currentClus = value;
        }
    }
};

void formatPath (string &filePath) {
    if (filePath[0] != '/') filePath = "/" + filePath;
}

/**
 * @error 错误码
 * error = 0 正确输出
 * error = 1 fileName不是文件名
 * error = 2 找不到文件名
 * */
void printCat (Node *root, string fileName, int &error) {
    formatPath(fileName);
    if (fileName == (root->getPath() + root->getName())) { // 最为递归的结束条件之一
        if (root->judgeFile()) {
            error = 0;
            print(root->getContent());
            print("\n");
            //cout << root->getContent() << endl;
            return;
        }
        error = 1;
        return;
    }

    if (fileName.length() <= root->getPath().length()) { // 作为递归的返回条件之一
        // 一开始文件名长度大于路径，当文件名长度小于路径时，说明前面的路径上都没有文件
        // 当文件名长度小于节点路径时，说明文件肯定不在这个节点上，文件无法找到
        error = 2;
        return;
    }

    string tmpPath = fileName.substr(0, root->getPath().length());
    if (tmpPath == root->getPath()) {
        vector<Node*> nodes = root->getNext();
        for (Node *next : nodes) { // 递归查找文件所在路径
            printCat(next, fileName, error);
        }
    }
}

void printLS(Node *root) {
    string path;
    Node *p = root;

    if (p->judgeFile()) { // 若此时P为文件节点，不打印,递归返回即可
        return;
    }
    else {
        path = p->getPath() + ":\n"; //先输出当前路径
        const char *pathPoint = path.c_str();
        print(pathPoint);
        //cout << path;
        path.clear();

        Node *node;
        int length = p->getNext().size(); // 得到当前路径下文件和目录总数
        for (int i = 0; i < length; i++) { // 先输出当前层，再进行循环递归
            node = p->getNext()[i];
            if (node->judgeFile()) { // 输出文件名
                cout << node->getName() << " ";
            }
            else { // 输出目录名
                cout << "\033[31m" << node->getName() << "\033[0m"<<"  ";
            }
        }
        cout << endl;
        for (int i = 0; i < length; i++) { // 对于每个目录下进行递归
            node = p->getNext()[i];
            if (node->judgeValue()) {
                printLS(node);
            }
        }
    }
}

// 判断是否是字母或数字
bool judgePhaOrDigit(string str) {
    for (int i = 0; i < str.length(); i++) {
        if(!isalpha(str[i]) && !isdigit(str[i])) return false;
    }
    return true;
}

// 判断是否只有ls
bool judgeOnlyRoot(vector<string> orders) { // 单个 ls
    if (orders.size() == 1) return true;

    if (orders.size() == 2) {
        if (orders[1] == "/" || orders[1] == "." || orders[1] == ".."
            || orders[1] == "/." || orders[1] == "/..") { // ls特殊情况
            return true;
        }
        else if (orders[1][0] == '/' || isalpha(orders[1][0])) { // ls /NJU/.. or ls NJU/..
            bool judge = false;
            int cnt = 0;
            int length = orders[1].length();
            vector<string> str = split(orders[1], "/");
            for (int i = 0; i < str.size(); i++) {
                if (str[i] == "..") {
                    judge = true;
                    cnt = 1;
                }
                else if (judgePhaOrDigit(str[i])) judge = true; // 排除 ls /NJU的情况，这种不算输出根目录
                else{
                    judge = false;
                }
            }
            if (cnt == 1 && judge) return true;

            return false;
        }
    }

    return false;
}

bool judgeOnlyPath(vector<string> orders) {
    if(orders.size() == 2) {
        if (orders[1] == "-l") return false;

        if(judgePhaOrDigit(orders[1])) { // 全是字母或数字
            return true;
        }
        else if(orders[1][0] == '/' || isalpha(orders[1][0])) { // ls /NJU or ls /NJU/ or NJU/
            for (int i = 0; i < orders[1].length(); i++) {
                if(orders[1][i] != '/' && !isalpha(orders[1][i]) && !isdigit(orders[1][i])) {
                    return false;
                }
            }
            return true;
        }
        else {
            bool judge = false;
            vector<string> str = split(orders[1], "/");
            if(str[0] == "." || str[0] == "..") { // ls ./NJU or ls ../NJU
                for (int i = 1; i < str.size(); i++) {
                    if(!judgePhaOrDigit(str[i]) && str[i] != ".." && str[i] != ".") {
                        judge = false;
                        break;
                    }
                    judge = true;
                }
            }
            return judge;
        }
    }
    return false;
}

// 判断是否只有ls -l，输出根目录
bool judgeOnlyL(vector<string> orders) { // 只有-l，没有设定文件路径
    if (orders.size() == 2) {
        string str = orders[1];
        for (int i = 0; i < str.length(); i++) {
            if(str[i] != '-' && str[i] != 'l') return false;
        }
        return true;
    }
    return false;
}

// 判断是否只有ls /NJU -l，输出子目录
bool judgeContentL(vector<string> orders) { // 设定文件路径的-l
    // 检查路径设置是否合法
    bool judge = true;
    if (orders.size() <= 2) return false;
    for (int i = 0; i < orders.size(); i++) {
        if (orders[i][0] == '-') {
            for (int j = 1; j < orders[i].length(); j++) {
                if (orders[i][j] != 'l') judge = false;
            }
        }
    }
    return judge;
}

void printL(Node  *root) {
    string path;
    Node *p = root;
    if (p->judgeFile()) { // 若此时P为文件节点，不打印,递归返回即可
        return;
    }
    else {
        // 计算输出当前节点的子文件和目录个数
        int fileNum = 0;
        int contentNum = 0;
        int length = p->getNext().size(); // 当前节点的子节点数
        for (int i = 0; i < length; i++) {
            if (p->getNext()[i]->getName() == "." || p->getNext()[i]->getName() == "..") {
                continue;
            }
            if(p->getNext()[i]->judgeFile()) {
                fileNum++;
            }
            else {
                contentNum++;
            }
        }
        cout << p->getPath() << " " << contentNum << " " << fileNum << endl;

        // 输出当前节点的每一个子节点的内容
        Node *node; // 一级子节点
        for (int i = 0; i < length; i++) {
            node = p->getNext()[i];
            if (node->judgeFile()) { // 子节点是文件节点,输出文件名 文件大小
                cout << node->getName() << " " << node->getFileSize() << endl;
            }
            else {// 子节点是目录节点，输出目录名 子目录个数 文件个数
                if (node->getName() == "." || node->getName() == "..") {
                    cout << "\033[31m" << node->getName() << "\033[0m"<<" " << endl;
                }
                else {
                    // 输出子节点包含的二级子文件和子目录数
                    int childFileNum = 0;
                    int childContentNum = 0;
                    uint32_t childLength = node->getNext().size();
                    for (int j = 0; j < childLength; j++) {
                        if(node->getNext()[j]->getName() == "." || node->getNext()[j]->getName() == "..") continue;
                        if (node->getNext()[j]->judgeFile()) {
                            childFileNum++;
                        }
                        else {
                            childContentNum++;
                        }
                    }

                    cout << "\033[31m" << node->getName() << "\033[0m" << " " << childContentNum << " " << childFileNum << endl;
                }

            }
        }
        cout << endl;

        for (int i = 0; i < length; i++) {
            if (p->getNext()[i]->judgeValue()) {
                printL(p->getNext()[i]);
            }
        }
    }

}

Node* findByName(Node *root, vector<string> content) {
    if (content.empty()) { // 递归结束条件
        return root;
    }

    string name = content[content.size()-1];
    for (int i = 0; i < root->getNext().size(); i++) {
        if (name == root->getNext()[i]->getName()) {
            content.pop_back();
            return findByName(root->getNext()[i], content);
        }
    }

    return nullptr; // 递归结束条件，什么都没找到
}

Node* findContentRoot (Node *root, vector<string> orders) {
    // 规范或orders中的目录名
    if(orders[1][0] != '/' && orders[1][0] != '.') orders[1] = "/" + orders[1];
    if(orders[1][orders[1].length()-1] == '/') {
        string str;
        for(int i = 0; i < orders[1].length()-1; i++) {
            str = str + orders[1][i];
        }
        orders[1] = str;
    }
    if(orders[1][0] == '.') {
        string str;
        for(int i = 0; i < orders[1].length(); i++) {
            if (orders[1][i] != '.') str = str + orders[1][i];
        }
        orders[1] = str;
    }

    Node *path = nullptr;
    for (int i = 0; i < orders.size(); i++) {
        if (orders[i][0] == '/') {
            vector<string> tmp = split(orders[i], "/");
            vector<string> content;
            while (!tmp.empty()) {
                content.push_back(tmp[tmp.size()-1]);
                tmp.pop_back();
            }
            path = findByName(root, content);
            break;
        }
    }
    return path;
}

int main() {
    FILE *fat12;

    // 打开FAT12映像文件
    fat12 = fopen("a2.img", "rb");

    //创建根节点
    Node *root = new Node();
    root->setName("");
    root->setPath("/");

    // 创建引导扇区
    BPB *bpb = new BPB();
    bpb->init(fat12); // 初始化

    // 创建根目录
    RootEntry *rootEntry = new RootEntry();
    rootEntry->initRootEntry(fat12, root);

    while(true) {
        print(">");
        //cout << ">";
        string input;
        getline(cin, input);
        vector<string> orders = split(input, " ");
        if (orders[0] == "exit") { // 退出程序
            break;
        }
        else if (orders[0] == "cat") { // 输入:cat 文件名，输出路径对应文件的内容， 若路径不存在或不是一个普通文件则给出提示
            if (orders.size() != 2) { // cat后面没有单个文件名
                print("Parameter is wrong\n");
                //cout << "Your input is missing the file name";
            }
            else {
                int error = 2;
                string fileName = orders[1]; // 包括path+文件名
                printCat(root, fileName, error);

                if (error == 0) {
                    print("success");
                    //cout << "success" << endl;
                }
                else if (error == 1) {
                    print("input is not a file name");
                    //cout << "input is not a file name" << endl;
                }
                else if (error == 2) {
                    print("cannot find file name");
                    //cout << "cannot find file name" << endl;
                }
            }

        }
        else if (orders[0] == "ls") {
            if (judgeOnlyRoot(orders)) { // 用户不添加任何参数,ls指令,根目录输出
                printLS(root);
            }
            else if(judgeOnlyPath(orders)) { // 输出指定目录内容，ls /NJU，没有l
                Node *path = findContentRoot(root, orders);
                if(path != nullptr) {
                    printL(path);
                }
                else { // 输入目录不存在
                    print("your content has an error");
                    //cout << "your content has an error" << endl;
                }
            }
            else { // 含有l的ls指令
                if (judgeOnlyL(orders)) { // 只有ls -l
                    printL(root);
                }
                else if (judgeContentL(orders)) { // 设定文件路径的ls /NJU -l
                    // 找到设定的文件路径
                    Node *path;
                    path = findContentRoot(root, orders);
                    if (path != nullptr) { // 检验是输入目录是否存在
                        printL(path);
                    }
                    else { // 输入目录不存在
                        print("your content has an error");
                        //cout << "your content has an error" << endl;
                    }
                }
                else {
                    print("your input has an error");
                    //cout << "your input has an error" << endl;
                }
            }
        }
        else {
            print("your order has an error");
            //cout << "your order has an error" << endl;
        }
    }
}
