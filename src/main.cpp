#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <thread>

inline int processId = 0;
inline void* handle = nullptr;

template <typename T> constexpr T read(const std::uintptr_t& addr)
{
  T value;
  ReadProcessMemory(handle, reinterpret_cast<const void*>(addr), &value, sizeof(T), 0);
  return value;
}

template <typename T> constexpr void write(const std::uintptr_t& addr, const T& value)
{
  WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &value, sizeof(T), 0);
}

inline auto GetModuleAddress(const std::string name) -> std::uintptr_t
{
  auto me = MODULEENTRY32{ };
  me.dwSize = sizeof(MODULEENTRY32);
  HANDLE hSnapshot;
  if ((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId)) == INVALID_HANDLE_VALUE)
  {
    return 0;
  }
  
  std::uintptr_t addr = -1;
  do {
    if (!name.compare(me.szModule))
    {
      addr = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
      break;
    }
  } while (Module32Next(hSnapshot, &me));

  if (hSnapshot)
  {
    CloseHandle(hSnapshot);
  }
  return addr;
}

inline auto SetProcessIdByName(std::string processName) -> void
{
  auto pe = PROCESSENTRY32{ };
  pe.dwSize = sizeof(PROCESSENTRY32);
  HANDLE hSnapshot;
  if ((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
  {
    return;
  }
  
  do
  {
    if (!processName.compare(pe.szExeFile))
    {
      processId = pe.th32ProcessID;
      break;
    }
  } while (Process32Next(hSnapshot, &pe));

  if (hSnapshot)
  {
    CloseHandle(hSnapshot);
  }
}

auto main(void) -> int
{
  SetProcessIdByName("csgo.exe");

  if (processId == 0)
  {
    MessageBox(NULL, "Run CS:GO first.", "", MB_ICONERROR | MB_OK);
    TerminateProcess(GetCurrentProcess(), 0);
  }

  while (!GetModuleAddress("serverbrowser.dll"))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  std::uintptr_t client = GetModuleAddress("client.dll");
  std::uintptr_t engine = GetModuleAddress("engine.dll");
  handle = OpenProcess(PROCESS_ALL_ACCESS, 0, processId);

  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (GetAsyncKeyState(VK_SPACE))
    {
      const auto flags = read<std::uintptr_t>(read<std::uintptr_t>(client + 0xDC14CC) + 0x104);
      if (flags & (1 << 0))
      {
        write<std::uintptr_t>(client + 0x52878FC, 6);
      }
      else
      {
        write<std::uintptr_t>(client + 0x52878FC, 4);
      }
    }
  }

  if (handle)
  {
    CloseHandle(handle);
  }
  return 0;
}