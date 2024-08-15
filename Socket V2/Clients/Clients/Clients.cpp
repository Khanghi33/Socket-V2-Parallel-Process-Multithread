// Clients.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "Clients.h"
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;
mutex mtx;
struct Datafile {
    char filename[20];
    char size[10];
    char priority[20];
    int filesize;
};
//Function to turn ON/OFF cursor in console
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

//Function to call CTR + C to exit
void signal_callback_handler(int signum) {
    cout << "\nDisconnect to Server!!\n";
    // Terminate program
    exit(signum);
}
int option_priority(char priority[20]) {
    if (strcmp(priority, "NORMAL") == 0) return 1;
    else if (strcmp(priority, "HIGH") == 0) return 4;
    else if (strcmp(priority, "CRITICAL") == 0) return 10;
    else return 1;
}
bool CheckExist_Getsize(Datafile files[], Datafile &require_file, int n) {
    for (int i = 0; i < n; i++)
        if (strcmp(require_file.filename, files[i].filename) == 0) {
            require_file.filesize = files[i].filesize; 
            return true;
        }
    return false;
}
//Get info from input file
void GetinfoInputFile(Datafile require_file[], int& n) {
    ifstream fin("input.txt", ios::in);
    if (!fin) cout << "Cannot open file!!       ";
    else {
        int i = 0;
        string line, name, priority;
        while (getline(fin, line)) {
            stringstream ss(line);
            ss >> name >> priority;
            strcpy_s(require_file[i].filename, name.c_str());
            strcpy_s(require_file[i].priority, priority.c_str());
            i++;
        }
        n = i;
    }
    fin.close();
}
void update_list_files(Datafile require_file[20], int &list_refile) {
    while (true) {
        //Get info all file need to be download from input file
        {
            lock_guard<mutex> lock(mtx);  GetinfoInputFile(require_file, list_refile);
        }
        Sleep(2000);
    }
}
//Download file
//Send buffer to clients
bool Send_buffer(CSocket& Server, char* buffer, int buffer_size) {
    int total_size_send = 0;
    while (total_size_send < buffer_size) {
        int bytes_send = 0;
        bytes_send = Server.Send(&buffer[total_size_send], buffer_size - total_size_send);
        if (bytes_send == 0) return false;
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
        if (bytes_recv == 0) return false;
        total_size_recv += bytes_recv;
    }
    return true;
}
bool Receive_file(int idx, int n_file, int& total_bytes_recv, CSocket& Client, Datafile require_file) {
    string filename(require_file.filename);
    int file_size, bytes_recv = 0;
    file_size = require_file.filesize;
    int n_package = option_priority(require_file.priority);
    //Create buffer
    const int BUFFER_SIZE = 10240;
    char buffer[BUFFER_SIZE];
    //Open file to store data
    fstream fout("Data/" + filename, ios::app | ios::binary);
    if (!fout) { cout << "Cannot open file!!\n"; return false; }
    fout.seekp(total_bytes_recv, ios::beg);
    int i = 0;
    while (i < n_package && total_bytes_recv < file_size) {
        bytes_recv = min(file_size - total_bytes_recv, BUFFER_SIZE);
        if (!Receive_buffer(Client, buffer, bytes_recv)) return false;
        fout.write((char*)&buffer, bytes_recv);
        total_bytes_recv += bytes_recv;
        Goto(0, idx + n_file + 4);  cout << "Downloading " << filename << "..." << int((total_bytes_recv / float(file_size)) * 100) << "%";
        i++;
    }
    fout.close();
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
void printdata(Datafile require_file[20], int& list_refile) {
    for (int i = 0; i < list_refile; i++)
        cout << require_file[i].filename << " " << require_file[i].priority << endl;
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
            CSocket Client;

            Client.Create();
            char ipAdd[100];
            cout << "Input Server ip: "; cin.getline(ipAdd, 100);
            //Set connection to ip Address
            if (Client.Connect(CA2W(ipAdd), 1234)) {
                ShowCur(0);
                cout << "Client has been connected!!\n";
                int id;
                //Receive the number of being client
                Client.Receive((char*)&id, sizeof(id), 0);
                cout << "Client's id: " << id << endl;
                //Receive info of file can be downloaded
                Datafile files[20]; int n_list;
                Client.Receive((char*)&n_list, sizeof(n_list), 0);
                Client.Receive((char*)&files, sizeof(files), 0);
                cout << "List of file can be downloaded:\n";
                for (int i = 0; i < n_list; i++)
                    cout << files[i].filename << " " << files[i].size << " " << files[i].filesize << endl;
                signal(SIGINT, signal_callback_handler);
                //Using idx add the number of file which had been downloaded from server
                //So, Client had't to download them again
                int idx = 0;
                //Real time downloading
                Datafile require_file[20]; int list_refile;
                vector<Datafile> list_download_files;
                vector<int> total_size;
                GetinfoInputFile(require_file, list_refile);
                thread update_file(update_list_files, require_file, ref(list_refile));
                update_file.detach();
                while (true) {
                    //Using flag to transfer signal when Clients is exit
                    bool flag = 0; Client.Send((char*)&flag, sizeof(flag), 0);
                    //Get info all file need to be download from input file, update 2s each time
                    do {
                        //Send require file to server to ask for downloading
                        Client.Send((char*)&list_refile, sizeof(list_refile), 0);
                        Client.Send((char*)&require_file, sizeof(require_file), 0);
                        Client.Send((char*)&idx, sizeof(idx), 0);
                        //Check if it existed in Server data
                        for (; idx < list_refile; idx++) {
                            if (CheckExist_Getsize(files, require_file[idx], n_list)) {
                                list_download_files.push_back(require_file[idx]);
                                total_size.push_back(0);
                            }
                            else {
                                Goto(0, idx + 4 + n_list);  cout << "'" << require_file[idx].filename << "'" << " is not found!!\n";
                                require_file[idx].filesize = 0;
                                list_download_files.push_back(require_file[idx]);
                                total_size.push_back(0);
                            }
                        }
                        for (int i = 0; i < idx; i++) {
                            if (total_size[i] < list_download_files[i].filesize)
                                Receive_file(i, n_list, total_size[i], Client, list_download_files[i]);
                                
                        }
                    } while (!isFinish(list_download_files, total_size));
                    Goto(0, idx + 4 + n_list); cout << "waiting for downloading...";
                    Sleep(1000);
                }
                Client.ShutDown(2);
                Client.Close();
            }
            else cout << "Cannot connect to server :(( \n";
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