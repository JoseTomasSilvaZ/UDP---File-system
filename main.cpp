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
void man() {
    map <string, string> commands;
    commands["ls"] = "List files in current directory";
    commands["cd"] = "Change directory";
    commands["mkdir"] = "Create directory";
    commands["touch"] = "Create file";
    commands["rm"] = "Remove file or directory";
    commands["mv"] = "Move file or directory";
    commands["cp"] = "Copy file or directory";
    commands["chmod"] = "Change permissions of file or directory";
    commands["pwd"] = "Show abs path";
    cout << "\n\n --------------" << "List of commands" << "--------------" << endl;
    for (auto &command : commands) {
        cout << command.first << " - " << command.second << endl;
    }
    cout << "\n\n";
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

    void ls() const {
        cout << "Listing files ~" << endl;
        for (const auto& child : childs) {
            string type = child->isDirectory ? "@Directory" : "@File";
           
            cout << type << ": " << child->name << endl;
            
        }
    }

    string getName() {
        return this->name;
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
        createdNode->setPadre(this);
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
            createdNode->setPadre(this);
            childs.push_back(move(createdNode));
            cout << "File created: " << fileName << endl;
        }

    void setPadre(Inodes *p)
    {
        cout << "Setting parent node" << p << endl;
        this->parent = p;
    }

    void addHijo(unique_ptr<Inodes> hijo)
    {
        childs.push_back(move(hijo));
    }


Inodes *cd(string directoryName) {
    if(directoryName == ".."){
        cout << "inside .." <<  this->parent << endl;
        if(this->parent){
            cout<< "efectively has parent : " << this->parent->name<< endl;
            fs::current_path("../");
        }
        // fs::current_path(parent ? parent->name : "./playground");

        return parent ? this->parent : this;
    }
    cout << "not .." << endl;
  for (const auto &child : childs)
{
    cout << "inside children searching" << endl;
    if (child->isDirectory && child->name == directoryName)
    {
        cout << child.get()->name << " children found" << endl;
        fs::current_path(child->name);
        return child.get();
    }
}

// Si el directorio no se encuentra en ninguno de los hijos, entonces regresar this
cout << "children not found" << endl;
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

    if (!fs::exists(playgroundPath)) {
        if (fs::create_directory(playgroundPath))
        {
            cout << "'playground' directory created" << endl;
        }
        else
        {
            cout << "Error creating 'playground' directory" << endl;
        }
    } else {
        cout << "'playground' directory already exists" << endl;
    }

    return playgroundPath;
}

void loadData(Inodes *node, const fs::path &path){
    for (const auto &entry : fs::directory_iterator(path))
    {
        const auto &filePath = entry.path();
        // cout << filePath << endl;
        bool isDirectory = fs::is_directory(filePath);
        cout<< "isDirectory: " << isDirectory << endl;
        // cout << filePath.filename().string() << "fname" << endl;
        auto createdNode = make_unique<Inodes>(filePath.filename().string(), isDirectory);
        cout<< "created node" << endl;
        // cout << "Creating parent node of "<< filePath.filename().string() << "parent: " << node->getName() << endl;
        createdNode->setPadre(node);
        if (isDirectory)
        {
            loadData(createdNode.get(), filePath);
        }
        node->addHijo(move(createdNode));
    }
}

void handleCommandInput(Inodes*& currentNode, Inodes* rootNode, vector<string>input){
    if(input[0] == "ls") {
        currentNode->ls();
    }
    if(input[0] == "man"){
        man();
    }
    if(input[0] == "cd"){
        if(input[1] != ""){
            currentNode = currentNode->cd(input[1]);
        }
        cout<< "path changed: " <<  fs::current_path() << endl;
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

    cout<< "Type 'man' to see the list of commands" << endl;
    cout<< "In order to have a controlled workspace, you cant go further behind than the playground folder. But u can do wathever u want inside ./playground" << endl;
    while(handleInput){
        cout << "🐱/" << current->getName() << "/$ ";
        getline(cin, command);
        cout << "command: " << command << endl;
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
   
    cout << "\n\n😸✨ See u later" << endl;


    return 0;
}