// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;
timed_mutex mtx;
struct Datafile {
    char filename[20];
    char size[10];
    char priority[20];
    int filesize;
};
void Get_file_size(Datafile &require_file) {
    string file(require_file.filename);
    ifstream fin("Data/" + file, ios::in | ios::binary);
    if (!fin) { cout << "Cannot open file!!\n"; return; }
    //Get file size
    fin.seekg(0, ios::end);
    require_file.filesize = fin.tellg();
    fin.seekg(0, ios::beg);
}
void GetFileData(Datafile files[], int& n) {
    ifstream fin("filedata.txt", ios::in);
    if (!fin) cout << "file had't been existed!!   \n";
    else {
        int i = 0;
        string Filename, Size;
        while (!fin.eof()) {
            getline(fin, Filename, ' ');
            strcpy_s(files[i].filename, Filename.c_str());
            getline(fin, Size, '\n');
            strcpy_s(files[i].size, Size.c_str());
            Get_file_size(files[i]);
            i++;
        }
        n = i;
    }
}
void ShowCur(bool CursorVisibility)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ConCurInf;

    ConCurInf.dwSize = 10;
    ConCurInf.bVisible = CursorVisibility;

    SetConsoleCursorInfo(handle, &ConCurInf);
}
void Goto(SHORT posX, SHORT posY)
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD Position;
    Position.X = posX;
    Position.Y = posY;

    SetConsoleCursorPosition(hStdout, Position);
}
bool CheckExist(Datafile files[], Datafile &require_file, int n) {
    for (int i = 0; i < n; i++)
        if (strcmp(require_file.filename, files[i].filename) == 0) {
            require_file.filesize = files[i].filesize;
            return true;
        }
    return false;
}
int option_priority(char priority[20]) {
    if (strcmp(priority, "NORMAL") == 0) return 1;
    else if (strcmp(priority, "HIGH") == 0) return 4;
    else if (strcmp(priority, "CRITICAL") == 0) return 10;
    else return 1;
}
//Send buffer to clients
bool Send_buffer(CSocket& Server, char* buffer, int buffer_size) {
    int total_size_send = 0;
    while (total_size_send < buffer_size) {
        int bytes_send = 0;
        bytes_send = Server.Send(&buffer[total_size_send], buffer_size - total_size_send);
        if (bytes_send <= 0) return false;
        total_size_send += bytes_send;
    }
    return true;
}
//Receive buffer from clients
bool Receive_buffer(CSocket& Server, char* buffer, int buffer_size) {
    int total_size_recv = 0;
    while (total_size_recv < buffer_size) {
        int bytes_recv = 0;
        bytes_recv = Server.Receive(&buffer[total_size_recv], buffer_size - total_size_recv);
        if (bytes_recv <= 0) return false;
        total_size_recv += bytes_recv;
    }
    return true;
}
bool Send_file(int& total_bytes_send, CSocket& Server, Datafile require_file) {
    string filename(require_file.filename);
    fstream fin("Data/" + filename, ios::in | ios::binary);
    if (!fin) {
        cout << "Cannot open file!!\n"; return false;
    }
    fin.seekg(total_bytes_send, ios::beg);
    int file_size, bytes_send;
    file_size = require_file.filesize;
    int n_package = option_priority(require_file.priority);
    //Create size buffer to store file data
    const int BUFFER_SIZE = 10000;
    char buffer[BUFFER_SIZE];
    int i = 0;
    while (i < n_package && total_bytes_send < file_size) {
        bytes_send = min(file_size - total_bytes_send, BUFFER_SIZE);
        fin.read((char*)&buffer, bytes_send);
        if (!Send_buffer(Server, buffer, bytes_send)) return false;
        total_bytes_send += bytes_send;
        i++;
    }
    fin.close();
    return true;
}
//void SendFile(int& total_data, CSocket &Client, Datafile require_file) {
//    string filename(require_file.filename);
//    fstream fin("Data/" + filename, ios::in | ios::binary);
//    if (!fin) cout << "Cannot open file!!\n";
//    else {
//        fin.seekg(total_data, ios::beg);
//        int file_size, bytes_send;
//        file_size = require_file.filesize;
//        //Create size buffer to store file data
//        int n_package = option_priority(require_file.priority);
//        char buffer1[10000];
//        char* buffer2;
//        //Using char[] when it has still has more than 1028 bytes to upload fastly
//        int i = 0;
//        while (i < n_package && total_data < file_size) {
//            if (total_data <= file_size - 10000) {
//                //Get file data
//                fin.read((char*)&buffer1, sizeof(buffer1));
//                //Send file data to client
//                bytes_send = Client.Send((char*)&buffer1, sizeof(buffer1), 0);
//                if (bytes_send < 1) { cout << "\nCannot send " << filename << "!!\n"; break; }
//                total_data += bytes_send;
//                //cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
//            }
//            //Using char* when it just has a little bit of bytes to upload exactly
//            else {
//                buffer2 = new char[file_size - total_data];
//                //Get file data
//                fin.read(buffer2, file_size - total_data);
//                //Send file data to client
//                bytes_send = Client.Send(buffer2, file_size - total_data, 0);
//                if (bytes_send < 1) { cout << "\nCannot send file!!\n"; break; }
//                total_data += bytes_send;
//                //cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
//                break;
//            }
//            i++;
//        }
//    }
//    fin.close();
//}
bool isFinish(vector<Datafile> list_download_files, vector<int> total_size) {
    int n = list_download_files.size();
    for (int i = 0; i < n; i++)
        if (list_download_files[i].filesize != total_size[i])
            return false;
    return true;
}
bool vector_check_exist(vector<int> vec, int val) {
    int n = vec.size();
    for (int i = 0; i < n; i++)
        if (vec[i] == val) return true;
    return false;
}
//bool Send_File_data(CSocket& SocketClients, Datafile files[20], int &n_list) {
//    int bytes_send;
//    GetFileData(files, n_list);
//    SocketClients.Send((char*)&n_list, sizeof(n_list), 0);
//    bytes_send = SocketClients.Send(files, sizeof(files), 0);
//    return true;
//}
int threadID = 0;
DWORD WINAPI function_cal(LPVOID arg) {
    SOCKET* hConnected = (SOCKET*)arg;
    CSocket Server;
    //Transfer back to CSocket
    Server.Attach(*hConnected);
    threadID++;
    cout << "\r" << "Connect to client " << threadID << " successfully!!\n";
    Server.Send((char*)&threadID, sizeof(threadID), 0);
    Datafile files[20]; int n_list = 0;
    int bytes_send;
    GetFileData(files, n_list);
    Server.Send((char*)&n_list, sizeof(n_list), 0);
    bytes_send = Server.Send(files, sizeof(files), 0);
    int idx = 0; bool flag;
    Datafile require_file[20]; int list_refile;
    vector<Datafile> list_download_files;
    vector<int> total_size;
    while (Server.Receive((char*)&flag, sizeof(flag), 0) > 0) {
        bool redflag = false;
        do {
            //Receive require file from client to make sending
            Server.Receive((char*)&list_refile, sizeof(list_refile), 0);
            Server.Receive((char*)&require_file, sizeof(require_file), 0);
            Server.Receive((char*)&idx, sizeof(idx), 0);
            //Check if it existed in Server data
            for (; idx < list_refile; idx++) {
                if (CheckExist(files, require_file[idx], n_list)) {
                    list_download_files.push_back(require_file[idx]);
                    total_size.push_back(0);
                }
                else {
                    cout << "'" << require_file[idx].filename << "'" << " is not found!!\n";
                    require_file[idx].filesize = 0;
                    total_size.push_back(0);
                    list_download_files.push_back(require_file[idx]);
                }
            }
            for (int j = 0; j < idx; j++) {
                if (total_size[j] < list_download_files[j].filesize)
                    if (!Send_file(total_size[j], Server, list_download_files[j])) {
                        redflag = true; break;
                    }
            }
        } while (!isFinish(list_download_files, total_size) && !redflag);
        //cout << "waiting for downloading...";
    }
    Server.Close();
    cout << "Disconnect to Client " << threadID << " !!  \n";

    delete hConnected;
    return 0;
    
}
//void MAIN(CSocket& SocketClients, int i) {
//    cout << "\rConnect to client " << i + 1 << " successfully!!\n";
//    SocketClients.Send((char*)&i, sizeof(i), 0);
//    Datafile files[20]; int n_list = 0;
//    int idx = 0; bool flag; int j;
//    Datafile require_file[20]; int list_refile;
//    vector<Datafile> list_download_files;
//    vector<int> total_size;
//    while (SocketClients.Receive((char*)&flag, sizeof(flag), 0) > 0) {
//        //Receive require file from client to make sending
//        SocketClients.Receive((char*)&list_refile, sizeof(list_refile), 0);
//        SocketClients.Receive((char*)&require_file, sizeof(require_file), 0);
//        SocketClients.Receive((char*)&idx, sizeof(idx), 0);
//        //Check if it existed in Server data
//        for (; idx < list_refile; idx++) {
//            if (CheckExist(files, require_file[idx], n_list)) {
//                list_download_files.push_back(require_file[idx]);
//                total_size.push_back(0);
//            }
//            else {
//                cout << "'" << require_file[idx].filename << "'" << " is not found!!\n";
//            }
//        }
//        while (!isFinish(list_download_files, total_size)) {
//            for (int j = 0; j < idx; j++) {
//                SendFile(total_size[j], SocketClients, list_download_files[j]);
//            }
//        }
//        //cout << "waiting for downloading...";
//    }
//    SocketClients.Close();
//    cout << "Disconnect to Client " << i + 1 << " !!  \n";
//}
//int main()
//{
//    int nRetCode = 0;
//
//    HMODULE hModule = ::GetModuleHandle(nullptr);
//
//    if (hModule != nullptr)
//    {
//        // initialize MFC and print and error on failure
//        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
//        {
//            // TODO: code your application's behavior here.
//            wprintf(L"Fatal Error: MFC initialization failed\n");
//            nRetCode = 1;
//        }
//        else
//        {
//            AfxSocketInit(NULL);
//            CSocket Server, s;
//            DWORD threadID;
//            HANDLE threadStatus;
//            unsigned int port = 1234;
//            AfxSocketInit(NULL);
//            //Create Server
//            if (!Server.Create(port, SOCK_STREAM, NULL)) {
//                cout << "Cannot create Server!!\n";
//                return nRetCode;
//            }
//            int i = -1;
//            do {
//                //Listen to clients
//                if (!Server.Listen(5)) {
//                    cout << " Cannot listen!!\n";
//                    return nRetCode;
//                }
//                if (Server.Accept(s)) {
//                    cout << "Connect is successfull!!\n";
//                    i++;
//                }
//                SOCKET* hConnected = new SOCKET();
//                *hConnected = s.Detach();
//                threadStatus = CreateThread(NULL, 0, function_cal, hConnected, 0, &threadID);
//            } while (true);
//            getchar();
//            Server.Close();
//        }
//    }
//    else
//    {
//        // TODO: change error code to suit your needs
//        wprintf(L"Fatal Error: GetModuleHandle failed\n");
//        nRetCode = 1;
//    }
//    return nRetCode;
//}

//int main()
//{
//    int nRetCode = 0;
//
//    HMODULE hModule = ::GetModuleHandle(nullptr);
//
//    if (hModule != nullptr)
//    {
//        // initialize MFC and print and error on failure
//        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
//        {
//            // TODO: code your application's behavior here.
//            wprintf(L"Fatal Error: MFC initialization failed\n");
//            nRetCode = 1;
//        }
//        else
//        {
//            CSocket Server;
//            unsigned int port = 1234;
//            AfxSocketInit(NULL);
//            //Create Server
//            if (!Server.Create(port, SOCK_STREAM, NULL)) {
//                cout << "Cannot create Server!!\n";
//                return nRetCode;
//            }
//            if (!Server.Listen(5)) {
//                cout << " Cannot listen!!\n";
//                return nRetCode;
//            }
//            int num_clients;
//            cout << "Input number of clients: "; cin >> num_clients;
//            CSocket* SocketClients = new CSocket[num_clients];
//            for (int i = 0; i < num_clients; i++) {
//                ShowCur(0);
//                cout << "Waiting for conection....";
//                Server.Accept(SocketClients[i]);
//                cout << "\r" << "Connect to client " << i + 1 << " successfully!!\n";
//                SocketClients[i].Send((char*)&i, sizeof(i), 0);
//                Datafile files[20]; int n_list = 0;
//                GetFileData(files, n_list);
//                SocketClients[i].Send((char*)&n_list, sizeof(n_list), 0);
//                SocketClients[i].Send(files, sizeof(files), 0);
//                int idx = 0; bool flag; int j;
//                Datafile require_file[20]; int list_refile;
//                vector<Datafile> list_download_files;
//                vector<int> total_size;
//                while (SocketClients[i].Receive((char*)&flag, sizeof(flag), 0) > 0) {
//                    //Receive require file from client to make sending
//                    SocketClients[i].Receive((char*)&list_refile, sizeof(list_refile), 0);
//                    SocketClients[i].Receive((char*)&require_file, sizeof(require_file), 0);
//                    SocketClients[i].Receive((char*)&idx, sizeof(idx), 0);
//                    //Check if it existed in Server data
//                    for (; idx < list_refile; idx++) {
//                        if (CheckExist(files, require_file[idx], n_list)) {
//                            list_download_files.push_back(require_file[idx]);
//                            total_size.push_back(0);
//                        }
//                        else {
//                            cout << "'" << require_file[idx].filename << "'" << " is not found!!\n";
//                        }
//                    }
//                    while (!isFinish(list_download_files, total_size)) {
//                        for (int j = 0; j < idx; j++) {
//                            SendFile(total_size[j], SocketClients[i], list_download_files[j]);
//                        }
//                    }
//                    //cout << "waiting for downloading...";
//                }
//                SocketClients[i].Close();
//                cout << "Disconnect to Client " << i + 1 << " !!  \n";
//            }
//            getchar();
//            delete[] SocketClients;
//            Server.Close();
//            getchar();
//            Server.Close();
//        }
//    }
//    else
//    {
//        // TODO: change error code to suit your needs
//        wprintf(L"Fatal Error: GetModuleHandle failed\n");
//        nRetCode = 1;
//    }
//    return nRetCode;
//}
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(NULL);

    if (hModule != NULL)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
        {
            // TODO: change error code to suit your needs
            _tprintf(_T("Fatal Error: MFC initialization failed\n"));
            nRetCode = 1;
        }
        else
        {
            // TODO: code your application's behavior here.
            AfxSocketInit(NULL);
            CSocket server, s;
            DWORD threadID;
            HANDLE threadStatus;

            server.Create(1234);
            do {
                ShowCur(0);
                server.Listen();
                cout << "\rWaiting for connection...";
                server.Accept(s);
                //Khoi tao con tro Socket
                SOCKET* hConnected = new SOCKET();
                //Chuyển đỏi CSocket thanh Socket
                *hConnected = s.Detach();
                //Khoi tao thread tuong ung voi moi client Connect vao server.
                //Nhu vay moi client se doc lap nhau, khong phai cho doi tung client xu ly rieng
                threadStatus = CreateThread(NULL, 0, function_cal, hConnected, 0, &threadID);
            } while (1);
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        _tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
        nRetCode = 1;
    }

    return nRetCode;
}
