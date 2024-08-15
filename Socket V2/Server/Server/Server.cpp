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
    const int BUFFER_SIZE = 10240;
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
int threadID = 0;
DWORD WINAPI function_cal(LPVOID arg) {
    SOCKET* hConnected = (SOCKET*)arg;
    CSocket Server;
    //Transfer back to CSocket
    Server.Attach(*hConnected);
    threadID++;
    int ID = threadID;
    cout << "\r" << "Connect to client " << ID << " successfully!!\n";
    Server.Send((char*)&ID, sizeof(ID), 0);
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
    cout << "Disconnect to Client " << ID << " !!  \n";

    delete hConnected;
    return 0;
    
}
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
