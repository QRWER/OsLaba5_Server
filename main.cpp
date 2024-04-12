#define NOMINMAX
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#pragma comment (lib,"Ws2_32.lib")
#pragma warning(disable: 4996)

std::vector<std::string> potFood = {"stew"};

DWORD WINAPI TakeFood(LPVOID t){
    int i = rand() % potFood.size();
    t = &potFood[i];
    potFood.erase(potFood.cbegin() + i);
    return 0;
}

DWORD WINAPI Cook(LPVOID t) {
    potFood = {"stew"};
    return 0;
}

bool printCausedBy(int Result, const char* nameOfOper) {
    if (!Result) {
        std::cout << "Connection closed.\n";
        return false;
    }
    else if (Result < 0) {
        std::cout << nameOfOper;
        std::cout << "failed:\n", WSAGetLastError();
        return false;
    }
    return true;
}

int main() {
    srand(time(0));
    //Загрузка библиотеки
    WSAData wsaData{}; //создаем структуру для загрузки
    //запрашиваемая версия библиотеки winsock
    WORD DLLVersion = MAKEWORD(2, 1);
    //проверка подключения библиотеки
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cerr << "Error: failed to link library.\n";
        return 1;
    }
    //Заполняем информацию об адресе сокета
    SOCKADDR_IN addr;
    static int sizeOfAddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;
    //сокет для прослушвания порта
    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    //привязка адреса сокету
    if (bind(sListen, (SOCKADDR *) &addr, sizeOfAddr) == SOCKET_ERROR) {
        std::cout <<"Error bind " << WSAGetLastError() << '\n';
        closesocket(sListen);
        WSACleanup();
        return 1;
    }
    //подкючение прослушивания с максимальной очередью
    if (listen(sListen, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(sListen);
        WSACleanup();
        return 1;
    }
    std::string food;
    HANDLE cheff = CreateThread(NULL,
                                0,
                                Cook,
                                NULL,
                                CREATE_SUSPENDED,
                                NULL);
    HANDLE pot = CreateThread(NULL,
                              0,
                              TakeFood,
                              food.data(),
                              CREATE_SUSPENDED,
                              NULL);
    HANDLE hMutex;
    hMutex = CreateMutexA(NULL, false, "mutex");
    if (hMutex == NULL) {
        std::cout << "CreateMutex error: " << GetLastError() << '\n';
        return 1;
    }
    int n;
    std::cout << "Enter count of starving human: ";
    std::cin >> n;
    while(std::cin.fail() || n<1 || std::cin.peek() != '\n'){
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Incorrect input. Please try again\n";
        std::cout << "Enter count of starving human: ";
        std::cin >> n;
    }
    std::vector<SOCKET> Sockets(n); //вектор для сокетов
    STARTUPINFO si;//структура
    PROCESS_INFORMATION pi;// структура с информацией о процессе
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);// указываем размер
    ZeroMemory(&pi, sizeof(pi));
    for (uint8_t i = 0; i < n; i++)
    {
        if (!CreateProcess("C:\\Users\\mum50\\CLionProjects\\Laba5_Client\\cmake-build-debug\\Laba5_Client.exe", // module name
                           NULL, // Command line
                           NULL, // Process handle not inheritable
                           NULL, // Thread handle not inheritable
                           FALSE, // Set handle inheritance to FALSE
                           CREATE_NEW_CONSOLE, //creation flags
                           NULL, // Use parent's environment block
                           NULL, // Use parent's starting directory
                           &si, // Pointer to STARTUPINFO structure
                           &pi) // Pointer to PROCESS_INFORMATION structure
                )
        {
            std::cout << "CreateProcess failed " << GetLastError() << '\n';
            return 1;
        }
        Sleep(10);
    }
    for (uint8_t i = 0; i < n; i++) { //сокеты для соединения с клиентом
        Sockets[i] = accept(sListen, (SOCKADDR*)&addr, &sizeOfAddr);
    }
    HANDLE cheffMutex;
    cheffMutex = CreateMutexA(NULL, false, "cmutex");
    for(uint8_t i = 0; i < n; i++) {
        WaitForSingleObject(cheffMutex, INFINITE);
        std::string buffer;
        printCausedBy(recv(Sockets[i], buffer.data(), sizeof(buffer), NULL), "Recv");
        if(potFood.empty()){
            ResumeThread(cheff);
        }
        ResumeThread(pot);
        printCausedBy(send(Sockets[i], food.data(), sizeof(food), NULL), "Send");
        ReleaseMutex(hMutex);
    }
    CloseHandle(hMutex);
    if (closesocket(sListen) == SOCKET_ERROR)
        std::cerr << "Failed to terminate connection. Error code: " << WSAGetLastError();
    WSACleanup();
    return 0;
}