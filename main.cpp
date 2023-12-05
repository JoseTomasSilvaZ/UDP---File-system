#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <iostream>
#include <map>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;
vector<string> split(const string& str, char delimiter) {
    vector<string> result;
    string token;
    istringstream tokenStream(str);

    while (getline(tokenStream, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}
void pwd() {
    cout << "Current path: " << fs::current_path() << endl;
}


fs::perms translatePerms(int octalPerms) {
    fs::perms resultPerms = fs::perms::none;

    // Descompone el número octal en dígitos para propietario, grupo y otros
    int ownerPerms = (octalPerms / 100) % 10;
    int groupPerms = (octalPerms / 10) % 10;
    int othersPerms = octalPerms % 10;

    // Función auxiliar para obtener los permisos específicos
    auto getPerms = [](int perms, fs::perms read, fs::perms write, fs::perms exec) {
        fs::perms result = fs::perms::none;
        if (perms & 4) result |= read;
        if (perms & 2) result |= write;
        if (perms & 1) result |= exec;
        return result;
    };

    // Aplica los permisos para propietario, grupo y otros
    resultPerms |= getPerms(ownerPerms, fs::perms::owner_read, fs::perms::owner_write, fs::perms::owner_exec);
    resultPerms |= getPerms(groupPerms, fs::perms::group_read, fs::perms::group_write, fs::perms::group_exec);
    resultPerms |= getPerms(othersPerms, fs::perms::others_read, fs::perms::others_write, fs::perms::others_exec);

    return resultPerms;
}

void man() {
    map <string, string> commands;
    commands["ls"] = "List files in current directory"; //ok
    commands["ls -l"] = "List files including metadata";
    commands["cd"] = "Change directory";    // ok
    commands["mkdir"] = "Create directory"; //ok
    commands["touch"] = "Create file"; //ok
    commands["rm"] = "Remove file or directory"; //ok
    commands["mv"] = "Move file or directory";
    commands["chmod"] = "Change permissions of file or directory"; //ok
    commands["pwd"] = "Show abs path"; //ok
    commands["rn"] = "Rename file or directory"; //ok
    commands["meta"] = "Show metadata of file or directory"; //ok
    cout << "\n --------------" << "List of commands" << "--------------" << endl;
    for (auto &command : commands) {
        cout << command.first << " - " << command.second << endl;
    }
}

class Inodes {
private:
    string name;
    vector<unique_ptr<Inodes>> childs;
    Inodes* parent;
    bool isDirectory;
    string permissions;


public:
    Inodes(string name, bool isDirectory) : name(name), isDirectory(isDirectory), parent(nullptr) {
        if (isDirectory) {
            permissions = "rwxr-xr-x"; // Permisos para directorios
        } else {
            permissions = "rw-r--r--"; // Permisos para archivos
        }
    }

    void ls(string showMeta="") const {
        cout << "\n";
        if(showMeta == "-l"){
            for (const auto &child : childs)
            {
                
                child->meta(child->name);
            }
            cout << "\n";
            return;
        }
        for (const auto &child : childs)
        {
            string type = child->isDirectory ? "@Directory" : "@File";
           
            cout << type << ": " << child->name << endl;
        }

    }

    string getName() {
        return this->name;
    }

   void meta(string fileName) {
    fs::path filePath = fs::current_path() / fileName;

    try {
        if (!fs::exists(filePath)) {
            cout << "File does not exist." << endl;
            return;
        }

        auto permissions = fs::status(filePath).permissions();
        cout << ((permissions & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
             << ((permissions & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
             << ((permissions & fs::perms::owner_exec) != fs::perms::none ? "x" : "-");
        cout << ((permissions & fs::perms::group_read) != fs::perms::none ? "r" : "-")
             << ((permissions & fs::perms::group_write) != fs::perms::none ? "w" : "-")
             << ((permissions & fs::perms::group_exec) != fs::perms::none ? "x" : "-");
        cout << ((permissions & fs::perms::others_read) != fs::perms::none ? "r" : "-")
             << ((permissions & fs::perms::others_write) != fs::perms::none ? "w" : "-")
             << ((permissions & fs::perms::others_exec) != fs::perms::none ? "x" : "-");

        if (fs::is_directory(filePath)) {
            cout << " Directory";
        } else {
            auto fileSize = fs::file_size(filePath);
            cout << " " << fileSize << " bytes";
        }

        // Mostrar fecha de última modificación
        auto ftime = fs::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        cout << " " << std::put_time(std::localtime(&cftime), "%F %T") << " last mod.";
        cout<<" " << fileName << endl;
    } catch (const fs::filesystem_error &e) {
        cout << "Error retrieving file metadata: " << e.what() << endl;
    }
}



    void rm(string fileName) {
        for (const auto& child : childs) {
            if (child->name == fileName) {
                if (child->isDirectory) {
                    string confirmation;
                    cout << "Are you sure you want to delete the directory? (y/n): ";
                    cin >> confirmation;
                    if (confirmation != "y") {
                        return;
                    }
                    fs::path dirPath = fs::current_path() / fileName;
                    try {
                        fs::remove_all(dirPath);

                        
                    } catch (fs::filesystem_error& e) {
                        cout << e.what() << endl;
                    }
                } else {
                    fs::path filePath = fs::current_path() / fileName;
                    try {
                        fs::remove(filePath);
                    } catch (fs::filesystem_error& e) {
                        cout << e.what() << endl;
                    }
                }
                for (auto &child : childs) {
                    if (child->name == fileName) {
                        childs.erase(remove(childs.begin(), childs.end(), child), childs.end());
                        return;
                    }
                }
            }
        }
        cout << "File not found." << endl;
    }

    void chmod(string fileName, int permissions){
        for (const auto& child : childs) {
            if (child->name == fileName) {
                fs::path filePath = fs::current_path() / fileName;
                try {
                    fs::perms newPermissions = translatePerms(permissions);
                    fs::permissions(filePath, newPermissions);
                } catch (fs::filesystem_error& e) {
                    cout << e.what() << endl;
                }
            }
        }
    }

    void mkdir(string dirName) {
        for (const auto& child : childs) {
            if (child->name == dirName) {
                cout << "The directory already exists." << endl;
                return;
            }
        }
        fs::path newDirPath = fs::current_path() / dirName;
        try {
            fs::create_directory(newDirPath);
        } catch (fs::filesystem_error& e) {
            cout << e.what() << endl;
        }
        auto createdNode = make_unique<Inodes>(dirName, true);
        createdNode->setParentNode(this);
        childs.push_back(move(createdNode));
    }

   void touch(string fileName) {
            // Revisar si el archivo ya existe en el árbol
            for (const auto& child : childs) {
                if (child->name == fileName) {
                    cout << "The file already exists here." << endl;
                    return;
                }
            }

            // Crear el archivo en el sistema de archivos
            fs::path filePath = fs::current_path() / fileName;
            ofstream file(filePath);
            if (!file) {
                cout << "Error creating file in the filesystem." << endl;
                return;
            }
            file.close(); // Cerrar el archivo después de crearlo

            // Agregar el nodo al árbol
            auto createdNode = make_unique<Inodes>(fileName, false);
            createdNode->setParentNode(this);
            childs.push_back(move(createdNode));
            cout << "File created: " << fileName << endl;
        }

    void setParentNode(Inodes *p)
    {
        this->parent = p;
    }

    void addHijo(unique_ptr<Inodes> hijo)
    {
        childs.push_back(move(hijo));
    }


Inodes *cd(string directoryName) {
    if(directoryName == ".."){
        if(this->parent){
            fs::current_path("../");
        }
        return parent ? this->parent : this;
    }
  for (const auto &child : childs)
{
    if (child->isDirectory && child->name == directoryName)
    {
        fs::current_path(child->name);
        return child.get();
    }
}

cout << "Folder not found" << endl;
return this;
}

void rn (string prevName, string newName) {
    for (const auto& child : childs) {
        if (child->name == prevName) {
            child->name = newName;
        }
    }
    fs::path prevPath = fs::current_path() / prevName;
    fs::path newPath = fs::current_path() / newName;
    try {
        fs::rename(prevPath, newPath);
    } catch (fs::filesystem_error& e) {
        cout << e.what() << endl;
    }
}

};

string createPlaygroundFolder() {
    string playgroundPath = "./playground";
    cout << "Checking if ./playground folder exists... ---> ";
    if (!fs::exists(playgroundPath))
    {
        if (fs::create_directory(playgroundPath))
        {
            cout << "'playground' directory created" << endl;
        }
        else
        {
            cout << "Error creating 'playground' directory" << endl;
        }
    }
    else
    {
        cout << "'playground' directory already exists" << endl;
    }

    return playgroundPath;
}

void loadData(Inodes *node, const fs::path &path){
    for (const auto &entry : fs::directory_iterator(path))
    {
        const auto &filePath = entry.path();
        bool isDirectory = fs::is_directory(filePath);
        auto createdNode = make_unique<Inodes>(filePath.filename().string(), isDirectory);
        createdNode->setParentNode(node);
        if (isDirectory)
        {
            loadData(createdNode.get(), filePath);
        }
        node->addHijo(move(createdNode));
    }
}

void handleCommandInput(Inodes*& currentNode, Inodes* rootNode, vector<string>input){
    if(input[0] == "ls") {
        if(input.size() > 1) {

        currentNode->ls(input[1]);
        } else {
            currentNode->ls();
        }
    }
    if(input[0] == "man"){
        man();
    }
    if(input[0] == "cd"){
        if(input[1] != ""){
            currentNode = currentNode->cd(input[1]);
        }
    }
    if(input[0] == "mkdir"){
        currentNode->mkdir(input[1]);
    }
    if(input[0] == "touch"){
        currentNode->touch(input[1]);
    }
    if(input[0] == "rm"){
        currentNode->rm(input[1]);
    }
    if(input[0] == "pwd"){
        pwd();
    }
    if(input[0] == "rn"){
        currentNode->rn(input[1], input[2]);
    
    }
    if(input[0] == "chmod"){
        currentNode->chmod(input[2], stoi(input[1]));
    }

    if(input[0] == "meta"){
        currentNode->meta(input[1]);
    }
}

void printCurrentPath(){
    vector<string> path = split(fs::current_path(), '/');
    bool printWord = false;
    cout << "\n🐱/";
    for (auto &word : path)
    {
    if(word == "playground"){
        printWord = true;
    }
    if(printWord){
        cout << word << "/";
    }
}
cout << "$ ";
}

int main () {

    unique_ptr<Inodes> root = make_unique<Inodes>("playground", true);
    Inodes* rootNode = root.get(); 
    Inodes* current = rootNode;
    createPlaygroundFolder();
    loadData(rootNode, "./playground");
    fs::current_path("./playground");
    string command;
    bool handleInput = true;

    cout << "To maintain a controlled workspace, you are not allowed to go beyond the 'playground' folder. However, you can do whatever you want inside './playground'. \n" << endl;
    cout<< "Type 'man' to see the list of commands \n" << endl;
  

    while (handleInput)
    {
        printCurrentPath();
        getline(cin, command);
        vector<string> input = split(command, ' ');
        
        if(input.size() > 3) {
            cout << "Too many arguments" << endl;
            continue;
        }
        if(input.size() == 0 ) {
            continue;
        }
        if(input[0] == "exit") {
            handleInput = false;
            break;
        }
        
        handleCommandInput(current, rootNode, input);
    }

    cout << "\n😸✨ See u later" << endl;


    return 0;
}