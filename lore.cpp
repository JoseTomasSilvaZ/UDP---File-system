// compile: g++ tarea3v1.cpp -o tarea3 -std=c++17

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

class ArbolNodos
{
private:
    string dato;                          // Nombre del archivo o directorio
    int verificador;                      // 1 para directorio, 0 para archivo
    vector<unique_ptr<ArbolNodos>> hijos; // Lista de subdirectorios o archivos usando unique_ptr
    ArbolNodos *padre;                    // Puntero al nodo padre
    int code;                             // Identificador único
    static int inode;                     // Contador global para asignar identificadores únicos

    size_t size;
    time_t lastModified;
    string permissions;

public:
    // Constructor
    ArbolNodos(string dato, bool esDirectorio) : dato(dato), verificador(esDirectorio ? 1 : 0), padre(nullptr), code(inode++), lastModified(time(nullptr))
    {
        if (esDirectorio)
        {
            permissions = "rwxr-xr-x"; // Permisos por defecto para directorios
        }
        else
        {
            permissions = "rw-r--r--"; // Permisos por defecto para archivos
        }
    }

    void setPermissions(const string &permisos)
    {
        permissions = permisos;
    }

    string get_full_path()
    {
        string path;
        ArbolNodos *temp = this;
        while (temp)
        {
            path = temp->dato + "/" + path;
            temp = temp->padre;
        }
        return path;
    }

    void setPadre(ArbolNodos *p)
    {
        padre = p;
    }

    void addHijo(unique_ptr<ArbolNodos> hijo)
    {
        hijos.push_back(move(hijo));
    }

    // Función para crear un directorio
    void mkdir(string nombreDirectorio)
    {
        // Verificar si el directorio ya existe en la estructura del árbol
        auto it = find_if(hijos.begin(), hijos.end(), [&nombreDirectorio](const unique_ptr<ArbolNodos> &hijo)
                          { return hijo->dato == nombreDirectorio && hijo->verificador == 1; });

        if (it != hijos.end())
        {
            cout << "El directorio ya existe: " << nombreDirectorio << endl;
            return;
        }

        // Crear el directorio en el sistema de archivos real
        fs::path dirPath = fs::current_path() / nombreDirectorio;
        try
        {
            if (!fs::create_directory(dirPath))
            {
                cerr << "Error al crear el directorio en el sistema de archivos: " << dirPath << endl;
                return;
            }
        }
        catch (const fs::filesystem_error &e)
        {
            cerr << "Excepción del sistema de archivos: " << e.what() << endl;
            return;
        }

        // Crear un nuevo directorio en la estructura del árbol
        unique_ptr<ArbolNodos> nuevoDirectorio = make_unique<ArbolNodos>(nombreDirectorio, true);
        nuevoDirectorio->padre = this;
        hijos.push_back(move(nuevoDirectorio));
    }

    void getMetadata(const string &nombre) const
    {
        for (const auto &hijo : hijos)
        {
            if (hijo->dato == nombre)
            {
                cout << "\nInformación del nodo:" << endl;
                cout << "Nombre: " << hijo->dato << endl;
                cout << "Tamaño: " << hijo->size << " bytes" << endl;

                char buffer[32];
                strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", localtime(&hijo->lastModified));
                cout << "Última modificación: " << buffer << endl;

                string permFormat = (hijo->verificador == 1 ? "d" : "-") + hijo->permissions;
                cout << "Permisos: " << permFormat << endl;

                cout << "Identificador único (i-node): " << hijo->code << "\n"
                     << endl;
                return;
            }
        }
        cout << "Archivo o directorio no encontrado: " << nombre << endl;
    }

    // Función para crear un archivo
    void mkfile(string nombreArchivo)
    {
        unique_ptr<ArbolNodos> nuevoArchivo = make_unique<ArbolNodos>(nombreArchivo, false); // false para archivo
        nuevoArchivo->padre = this;
        nuevoArchivo->verificador = 0; // Marcando como archivo
        hijos.push_back(move(nuevoArchivo));

        // Syscall para crear el archivo en el sistema de archivos
        fs::path filePath = fs::current_path() / nombreArchivo;
        ofstream file(filePath);
        if (!file)
        {
            cerr << "Error al crear el archivo: " << nombreArchivo << endl;
        }
    }

    void ls() const
    {
        for (const auto &hijo : hijos)
        {
            string tipo = (hijo->verificador == 1) ? "[Dir]" : "[File]";
            cout << tipo << " " << hijo->dato << endl;
        }
    }

    void chmod(string nombre, int permisosOctal)
    {
        auto it = find_if(hijos.begin(), hijos.end(), [&nombre](const unique_ptr<ArbolNodos> &hijo)
                          { return hijo->dato == nombre; });

        if (it != hijos.end())
        {
            fs::path filePath = fs::current_path() / nombre;
            try
            {
                // Aplicar permisos usando fs::permissions
                fs::perms perms = octal_to_perms(permisosOctal);
                fs::permissions(filePath, perms, fs::perm_options::replace);

                // Actualizar permisos en la estructura del árbol
                (*it)->permissions = convertirAStringDePermisos(permisosOctal, (*it)->verificador == 1);
                cout << "Permisos cambiados a " << (*it)->permissions << " para " << nombre << endl;
            }
            catch (const fs::filesystem_error &e)
            {
                cerr << "Error al cambiar permisos: " << e.what() << endl;
            }
        }
        else
        {
            cout << "Archivo o directorio no encontrado: " << nombre << endl;
        }
    }

    fs::perms octal_to_perms(int permisosOctal)
    {
        fs::perms result = fs::perms::none;

        // Permisos para el propietario (owner)
        int ownerPerms = (permisosOctal / 100) % 10;
        if (ownerPerms & 0b100)
            result |= fs::perms::owner_read;
        if (ownerPerms & 0b010)
            result |= fs::perms::owner_write;
        if (ownerPerms & 0b001)
            result |= fs::perms::owner_exec;

        // Permisos para el grupo (group)
        int groupPerms = (permisosOctal / 10) % 10;
        if (groupPerms & 0b100)
            result |= fs::perms::group_read;
        if (groupPerms & 0b010)
            result |= fs::perms::group_write;
        if (groupPerms & 0b001)
            result |= fs::perms::group_exec;

        // Permisos para otros (others)
        int othersPerms = permisosOctal % 10;
        if (othersPerms & 0b100)
            result |= fs::perms::others_read;
        if (othersPerms & 0b010)
            result |= fs::perms::others_write;
        if (othersPerms & 0b001)
            result |= fs::perms::others_exec;

        return result;
    }

    // Método auxiliar para convertir permisos octales a string
    string convertirAStringDePermisos(int permisosOctal, bool esDirectorio)
    {
        if (permisosOctal < 0 || permisosOctal > 777)
        {
            return "";
        }
        string permisos = esDirectorio ? "" : ""; // Prefijo para directorio o archivo
        for (int i = 0; i < 3; ++i)
        {
            int permiso = permisosOctal % 10; // Extraer el dígito octal
            switch (permiso)
            {
            case 0:
                permisos += "---";
                break;
            case 1:
                permisos += "--x";
                break;
            case 2:
                permisos += "-w-";
                break;
            case 3:
                permisos += "-wx";
                break;
            case 4:
                permisos += "r--";
                break;
            case 5:
                permisos += "r-x";
                break;
            case 6:
                permisos += "rw-";
                break;
            case 7:
                permisos += "rwx";
                break;
            }
            permisosOctal /= 10;
        }
        return permisos;
    }

    // Método para cambiar de directorio
    ArbolNodos *cd(string nombreDirectorio)
    {
        if (nombreDirectorio == "..")
        {
            // Si es el comando para ir al directorio padre y existe un padre
            if (padre != nullptr)
            {
                return padre; // Retorna el directorio padre
            }
            else
            {
                cout << "Ya estás en el directorio raíz, no hay directorio padre." << endl;
                return nullptr; // No hay directorio padre
            }
        }
        else
        {
            // Manejo normal para cambiar a otro directorio
            for (auto &hijo : hijos)
            {
                if (hijo->dato == nombreDirectorio && hijo->verificador == 1)
                {
                    return hijo.get(); // Retorna el directorio hijo
                }
            }
            return nullptr; // Directorio no encontrado
        }
    }

    // Método para eliminar un archivo o directorio
    bool rm(string nombre)
    {
        auto it = find_if(hijos.begin(), hijos.end(), [&nombre](const unique_ptr<ArbolNodos> &hijo)
                          { return hijo->dato == nombre; });

        if (it != hijos.end())
        {
            // Eliminar del sistema de archivos
            if ((*it)->verificador == 1)
            { // Si es un directorio
                fs::remove_all(fs::current_path() / (*it)->dato);
            }
            else
            { // Si es un archivo
                fs::remove(fs::current_path() / (*it)->dato);
            }

            // Eliminar del árbol
            hijos.erase(it);
            return true;
        }
        return false;
    }

    bool rename(string oldName, string newName)
    {
        auto it = find_if(hijos.begin(), hijos.end(), [&oldName](const unique_ptr<ArbolNodos> &hijo)
                          { return hijo->dato == oldName; });

        if (it != hijos.end())
        {
            // Renombrar en el árbol
            (*it)->dato = newName;

            // Renombrar en el sistema de archivos
            fs::path oldPath = fs::current_path() / oldName;
            fs::path newPath = fs::current_path() / newName;
            try
            {
                fs::rename(oldPath, newPath);
            }
            catch (const fs::filesystem_error &e)
            {
                cerr << "Error al renombrar: " << e.what() << endl;
                return false;
            }

            return true;
        }
        else
        {
            cout << "Archivo o directorio no encontrado: " << oldName << endl;
            return false;
        }
    };
};

string obtenerPermisosComoString(const fs::path &path)
{
    auto perms = fs::status(path).permissions();
    string permisosStr;

    // Permisos para el propietario (Owner)
    permisosStr += (perms & fs::perms::owner_read) != fs::perms::none ? "r" : "-";
    permisosStr += (perms & fs::perms::owner_write) != fs::perms::none ? "w" : "-";
    permisosStr += (perms & fs::perms::owner_exec) != fs::perms::none ? "x" : "-";

    // Permisos para el grupo (Group)
    permisosStr += (perms & fs::perms::group_read) != fs::perms::none ? "r" : "-";
    permisosStr += (perms & fs::perms::group_write) != fs::perms::none ? "w" : "-";
    permisosStr += (perms & fs::perms::group_exec) != fs::perms::none ? "x" : "-";

    // Permisos para otros (Others)
    permisosStr += (perms & fs::perms::others_read) != fs::perms::none ? "r" : "-";
    permisosStr += (perms & fs::perms::others_write) != fs::perms::none ? "w" : "-";
    permisosStr += (perms & fs::perms::others_exec) != fs::perms::none ? "x" : "-";

    return permisosStr;
}

void loadFileSystem(ArbolNodos *nodo, const fs::path &path)
{
    for (const auto &entry : fs::directory_iterator(path))
    {
        const auto &p = entry.path();
        bool esDirectorio = fs::is_directory(p);

        auto nuevoNodo = make_unique<ArbolNodos>(p.filename().string(), esDirectorio);

        // Establecer los permisos utilizando el nuevo método
        string permisos = obtenerPermisosComoString(p);
        nuevoNodo->setPermissions(permisos);

        nuevoNodo->setPadre(nodo);

        if (esDirectorio)
        {
            loadFileSystem(nuevoNodo.get(), p);
        }

        nodo->addHijo(move(nuevoNodo));
    }
}

void help()
{
    cout << "\nComandos disponibles:" << endl;
    cout << "mkdir <nombre> - Crear un directorio" << endl;
    cout << "mkfile <nombre> - Crear un archivo" << endl;
    cout << "ls - Listar archivos y directorios" << endl;
    cout << "cd <nombre> - Cambiar de directorio" << endl;
    cout << "rm <nombre> - Eliminar un archivo o directorio" << endl;
    cout << "rename <nombre> <nuevoNombre> - Renombrar un archivo o directorio" << endl;
    cout << "metadata <nombre> - Obtener metadatos de un archivo o directorio" << endl;
    cout << "chmod <nombre> <permisosOctal> - Cambiar permisos de un archivo o directorio" << endl;
    cout << "exit - Salir del programa\n" << endl;
}

void build()
{
    fs::path rootPath = fs::current_path() / "root";
    // Crear el directorio raíz si no existe
    if (!fs::exists(rootPath))
    {
        if (!fs::create_directory(rootPath))
        {
            cerr << "Error al crear el directorio raíz en el sistema de archivos." << endl;
            return;
        }
    }
    fs::current_path(rootPath);

    // Crear el nodo raíz y cargar la estructura existente del sistema de archivos
    unique_ptr<ArbolNodos> raiz = make_unique<ArbolNodos>("root", true);
    ArbolNodos *current = raiz.get();
    loadFileSystem(current, rootPath);

    string comando;
    string argumento;
    while (true)
    {
        cout << current->get_full_path() << " % ";
        cin >> comando;

        if (comando == "mkdir")
        {
            cin >> argumento;
            current->mkdir(argumento);
        }
        else if (comando == "mkfile")
        {
            cin >> argumento;
            current->mkfile(argumento);
        }
        else if (comando == "ls")
        {
            current->ls();
        }
        else if (comando == "cd")
        {
            cin >> argumento;
            ArbolNodos *nuevoCurrent = current->cd(argumento);
            if (nuevoCurrent != nullptr)
            {
                current = nuevoCurrent;
                fs::current_path(argumento); // Cambiar el directorio de trabajo del sistema de archivos real
            }
            else
            {
                cout << "Directorio no encontrado." << endl;
            }
        }
        else if (comando == "chmod")
        {
            string nombre;
            int permisosOctal;
            cin >> nombre >> permisosOctal;
            current->chmod(nombre, permisosOctal);
        }
        else if (comando == "rm")
        {
            cin >> argumento;
            if (!current->rm(argumento))
            {
                cout << "No se pudo eliminar el archivo o directorio." << endl;
            }
        }
        else if (comando == "rename")
        {
            string oldName, newName;
            cin >> oldName >> newName;
            if (!current->rename(oldName, newName))
            {
                cout << "No se pudo renombrar " << oldName << " a " << newName << endl;
            }
        }
        else if (comando == "metadata")
        {
            cin >> argumento;
            current->getMetadata(argumento);
        }
        else if (comando == "help")
        {
            help();
        }
        else if (comando == "exit")
        {
            break;
        }
        else
        {
            cout << "Comando no reconocido." << endl;
            cout << "Escribe 'help' para ver los comandos disponibles." << endl;
        }
    }
}

int ArbolNodos::inode = 0; // Inicialización del contador estático

int main()
{
    build();
    return 0;
}