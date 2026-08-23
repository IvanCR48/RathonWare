#include "port_manager.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>

PortManager::PortManager(QObject *parent)
    : QObject(parent)
{
}

PortManager::~PortManager()
{
}

QString PortManager::getProcessName(unsigned long pid)
{
    if (pid == 0) return "System Idle Process";
    if (pid == 4) return "System";

    QString name = "Unknown";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ProcessID == pid) {
                    name = QString::fromWCharArray(pe32.szExeFile);
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return name;
}

double PortManager::getProcessMemoryMB(unsigned long pid)
{
    if (pid == 0 || pid == 4) return 0.0;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    }
    if (hProcess) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            CloseHandle(hProcess);
            return pmc.WorkingSetSize / (1024.0 * 1024.0);
        }
        CloseHandle(hProcess);
    }
    return 0.0;
}

QString PortManager::getProcessCommandLine(unsigned long pid)
{
    if (pid == 0 || pid == 4) return "";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
            CloseHandle(hProcess);
            return QString::fromWCharArray(path);
        }
        CloseHandle(hProcess);
    }
    return "";
}

QVector<PortProcessEntry> PortManager::queryAllPorts()
{
    QVector<PortProcessEntry> entries;

    // 1. IPv4 TCP Table
    DWORD dwSize = 0;
    GetExtendedTcpTable(NULL, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwSize > 0) {
        std::vector<BYTE> buffer(dwSize);
        if (GetExtendedTcpTable(buffer.data(), &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            PMIB_TCPTABLE_OWNER_PID pTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
            for (DWORD i = 0; i < pTable->dwNumEntries; i++) {
                const MIB_TCPROW_OWNER_PID& row = pTable->table[i];
                int port = ntohs(static_cast<u_short>(row.dwLocalPort));
                unsigned long pid = row.dwOwningPid;

                in_addr ipAddr;
                ipAddr.S_un.S_addr = row.dwLocalAddr;
                char ipStr[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN);

                QString stateStr;
                switch (row.dwState) {
                case MIB_TCP_STATE_CLOSED: stateStr = "CLOSED"; break;
                case MIB_TCP_STATE_LISTEN: stateStr = "LISTENING"; break;
                case MIB_TCP_STATE_SYN_SENT: stateStr = "SYN_SENT"; break;
                case MIB_TCP_STATE_SYN_RCVD: stateStr = "SYN_RCVD"; break;
                case MIB_TCP_STATE_ESTAB: stateStr = "ESTABLISHED"; break;
                case MIB_TCP_STATE_FIN_WAIT1: stateStr = "FIN_WAIT1"; break;
                case MIB_TCP_STATE_FIN_WAIT2: stateStr = "FIN_WAIT2"; break;
                case MIB_TCP_STATE_CLOSE_WAIT: stateStr = "CLOSE_WAIT"; break;
                case MIB_TCP_STATE_CLOSING: stateStr = "CLOSING"; break;
                case MIB_TCP_STATE_LAST_ACK: stateStr = "LAST_ACK"; break;
                case MIB_TCP_STATE_TIME_WAIT: stateStr = "TIME_WAIT"; break;
                case MIB_TCP_STATE_DELETE_TCB: stateStr = "DELETE_TCB"; break;
                default: stateStr = "UNKNOWN"; break;
                }

                PortProcessEntry entry;
                entry.port = port;
                entry.pid = pid;
                entry.protocol = "TCP";
                entry.state = stateStr;
                entry.localAddress = QString("%1:%2").arg(ipStr).arg(port);
                entry.processName = getProcessName(pid);
                entry.ramUsageMB = getProcessMemoryMB(pid);
                entry.cmdLine = getProcessCommandLine(pid);
                entries.append(entry);
            }
        }
    }

    // 2. IPv6 TCP Table
    DWORD dwSize6 = 0;
    GetExtendedTcpTable(NULL, &dwSize6, TRUE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwSize6 > 0) {
        std::vector<BYTE> buffer6(dwSize6);
        if (GetExtendedTcpTable(buffer6.data(), &dwSize6, TRUE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            PMIB_TCP6TABLE_OWNER_PID pTable6 = reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(buffer6.data());
            for (DWORD i = 0; i < pTable6->dwNumEntries; i++) {
                const MIB_TCP6ROW_OWNER_PID& row = pTable6->table[i];
                int port = ntohs(static_cast<u_short>(row.dwLocalPort));
                unsigned long pid = row.dwOwningPid;

                char ipStr[INET6_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET6, (PVOID)&row.ucLocalAddr, ipStr, INET6_ADDRSTRLEN);

                QString stateStr = (row.dwState == MIB_TCP_STATE_LISTEN) ? "LISTENING" : 
                                   (row.dwState == MIB_TCP_STATE_ESTAB) ? "ESTABLISHED" : "ACTIVE";

                PortProcessEntry entry;
                entry.port = port;
                entry.pid = pid;
                entry.protocol = "TCP6";
                entry.state = stateStr;
                entry.localAddress = QString("[%1]:%2").arg(ipStr).arg(port);
                entry.processName = getProcessName(pid);
                entry.ramUsageMB = getProcessMemoryMB(pid);
                entry.cmdLine = getProcessCommandLine(pid);
                entries.append(entry);
            }
        }
    }

    // 3. IPv4 UDP Table
    DWORD dwUdpSize = 0;
    GetExtendedUdpTable(NULL, &dwUdpSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (dwUdpSize > 0) {
        std::vector<BYTE> bufferUdp(dwUdpSize);
        if (GetExtendedUdpTable(bufferUdp.data(), &dwUdpSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
            PMIB_UDPTABLE_OWNER_PID pUdpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(bufferUdp.data());
            for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++) {
                const MIB_UDPROW_OWNER_PID& row = pUdpTable->table[i];
                int port = ntohs(static_cast<u_short>(row.dwLocalPort));
                unsigned long pid = row.dwOwningPid;

                in_addr ipAddr;
                ipAddr.S_un.S_addr = row.dwLocalAddr;
                char ipStr[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &ipAddr, ipStr, INET_ADDRSTRLEN);

                PortProcessEntry entry;
                entry.port = port;
                entry.pid = pid;
                entry.protocol = "UDP";
                entry.state = "BOUND";
                entry.localAddress = QString("%1:%2").arg(ipStr).arg(port);
                entry.processName = getProcessName(pid);
                entry.ramUsageMB = getProcessMemoryMB(pid);
                entry.cmdLine = getProcessCommandLine(pid);
                entries.append(entry);
            }
        }
    }

    return entries;
}

QVariantList PortManager::getProcessesByPort(int port)
{
    QVariantList list;
    QVector<PortProcessEntry> all = queryAllPorts();
    for (const auto& entry : all) {
        if (entry.port == port) {
            QVariantMap map;
            map["port"] = entry.port;
            map["pid"] = static_cast<qlonglong>(entry.pid);
            map["name"] = entry.processName;
            map["protocol"] = entry.protocol;
            map["state"] = entry.state;
            map["localAddress"] = entry.localAddress;
            map["ramMB"] = entry.ramUsageMB;
            map["cmdLine"] = entry.cmdLine;
            list.append(map);
        }
    }
    return list;
}

QVariantList PortManager::getAllListeningPorts()
{
    QVariantList list;
    QVector<PortProcessEntry> all = queryAllPorts();
    for (const auto& entry : all) {
        if (entry.state == "LISTENING" || entry.state == "ESTABLISHED" || entry.state == "BOUND") {
            QVariantMap map;
            map["port"] = entry.port;
            map["pid"] = static_cast<qlonglong>(entry.pid);
            map["name"] = entry.processName;
            map["protocol"] = entry.protocol;
            map["state"] = entry.state;
            map["localAddress"] = entry.localAddress;
            map["ramMB"] = entry.ramUsageMB;
            map["cmdLine"] = entry.cmdLine;
            list.append(map);
        }
    }
    return list;
}

bool PortManager::killProcessOnPort(int port)
{
    bool anyKilled = false;
    QVector<PortProcessEntry> all = queryAllPorts();
    for (const auto& entry : all) {
        if (entry.port == port && entry.pid > 4) {
            if (killProcessByPid(entry.pid)) {
                anyKilled = true;
            }
        }
    }
    return anyKilled;
}

bool PortManager::killProcessByPid(int pid)
{
    if (pid <= 4) return false;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProcess) return false;
    bool success = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return success;
}
