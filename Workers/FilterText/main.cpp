#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include "../Utils/Utils.h"

class CWorker {
public:
    CWorker(const PROCESS_INFORMATION processInfo, const HANDLE terminationEvent) :
        processInfo(processInfo),
        terminationEvent(terminationEvent) {
        dataIsReadyEvent = GetDataIsReadyEvent(processInfo.dwProcessId);
    }

    HANDLE GetTerminationEvent() const {
        return terminationEvent;
    }

    void Notify() {
        SetEvent(dataIsReadyEvent);
    }

    void Join() {
        if (processInfo.hProcess != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(processInfo.hProcess, INFINITE);
            CloseHandle(processInfo.hProcess);
            processInfo.hProcess = INVALID_HANDLE_VALUE;
        } else {
            throw std::runtime_error("Trying to join already joined or not created worker");
        }
    }

private:
    PROCESS_INFORMATION processInfo;
    HANDLE terminationEvent, dataIsReadyEvent;
};

HANDLE CreateTerminationEvent()
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = NULL;
    attributes.bInheritHandle = TRUE;
    HANDLE terminationEvent = CreateEvent(&attributes, TRUE, FALSE, 0);
    if (terminationEvent == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to create event");
    }
    return terminationEvent;
}

std::vector<CWorker> InitWorkers(size_t numberOfWorkers, const std::wstring &dictionaryPath) 
{
    std::vector<CWorker> workers;
    if (numberOfWorkers <= 1) {
        return workers;
    }

    std::wstring workerPath(L"Worker.exe");
    HANDLE terminationEvent = CreateTerminationEvent();
    
    for (size_t i = 0; i < numberOfWorkers; ++i) {
        std::wstring arguments = workerPath.append(L" ")
            .append(dictionaryPath).append(L" ")
            .append(std::to_wstring(reinterpret_cast<int> (terminationEvent)));

        auto cArguments = std::make_unique<wchar_t[]>(arguments.length() + 1);
        wcscpy(cArguments.get(), arguments.c_str());

        PROCESS_INFORMATION processInfo;
        STARTUPINFO startUpInfo = { sizeof(startUpInfo) };
        if (CreateProcess(0, cArguments.get(), 0, 0, TRUE, 0, 0, 0, &startUpInfo, &processInfo) != 0) {
            std::cerr << "Process created " << processInfo.dwProcessId << std::endl;
            CloseHandle(processInfo.hThread);
            CWorker worker(processInfo, terminationEvent);
            workers.push_back(worker);
        } else {
            DWORD lerr = GetLastError();
            throw std::runtime_error("Failed to create a process");
        }
    }
    return workers;
}

void JoinWorkers(std::vector<CWorker> &workers) 
{
    for (CWorker& worker : workers) {
        worker.Join();
    }
}

int main(int argc, char* argv[]) {
    try {
        auto workers = InitWorkers(4, GetDictionaryPathFromArgs());
        std::cin.ignore();
        workers[0].Notify();
        std::cin.ignore();
        SetEvent(workers[0].GetTerminationEvent());
        JoinWorkers(workers);
        std::cin.ignore();
        return 0;
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}