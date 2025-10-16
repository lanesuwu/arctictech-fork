#include <Windows.h>
#include <locale.h>
#include <DbgHelp.h>
#include <codecvt>
#include <format>
#include <locale>
#include <winternl.h>
#pragma comment(lib, "DbgHelp.lib")

#include "SDK/Interfaces.h"
#include "SDK/Hooks.h"
#include "Features/Visuals/SkinChanger.h"
#include "Features/Lua/Bridge/Bridge.h"


static void* cheat_module_base = 0;

static std::pair<uint64_t, std::string> GetContainingModule(uint64_t address) {
    struct _LDR_DATA_TABLE_ENTRY_ {
        PVOID Reserved1[2];
        LIST_ENTRY InMemoryOrderLinks;
        PVOID Reserved2[2];
        PVOID DllBase;
        PVOID Reserved3[2];
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
    };

    const auto peb = reinterpret_cast<PEB*>(reinterpret_cast<TEB*>(__readfsdword(0x18))->ProcessEnvironmentBlock);
    const auto module_list = &peb->Ldr->InMemoryOrderModuleList;

    for (auto i = module_list->Flink; i != module_list; i = i->Flink) {
        auto entry = CONTAINING_RECORD(i, _LDR_DATA_TABLE_ENTRY_, InMemoryOrderLinks);

        if (!entry)
            continue;

        const auto module_start = (uint64_t)entry->DllBase;
        const auto dos_headers = (IMAGE_DOS_HEADER*)module_start;
        const auto nt_headers = (IMAGE_NT_HEADERS*)(module_start + dos_headers->e_lfanew);

        if (address >= module_start && address <= module_start + nt_headers->OptionalHeader.SizeOfImage) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

            return std::make_pair(module_start,
                converter.to_bytes(entry->BaseDllName.Buffer, entry->BaseDllName.Buffer + entry->BaseDllName.Length / 2));
        }
    }

    MEMORY_BASIC_INFORMATION mbi;

    VirtualQuery(cheat_module_base, &mbi, sizeof(mbi));

    if ((DWORD64)address >= (DWORD64)mbi.BaseAddress && (DWORD64)address <= (DWORD64)mbi.BaseAddress + mbi.RegionSize) {
        return std::make_pair((uint64_t)mbi.BaseAddress, ("<cheat>"));
    }
    else {
        return std::make_pair((uint64_t)mbi.BaseAddress, std::format(("<unknown module {:#x}>"), (DWORD64)mbi.BaseAddress));
    }

    return std::make_pair(0, ("<unknown module>"));
}

long __stdcall ExceptionHandler(EXCEPTION_POINTERS* info) {
    if (Interfaces::Panic)
        return EXCEPTION_CONTINUE_SEARCH;

    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    DWORD64 displacement;

    const auto symbol = (SYMBOL_INFO*)std::malloc(sizeof(SYMBOL_INFO) + 256);

    if (symbol != nullptr) {
        const auto [module_base, module_name] = GetContainingModule((uint64_t)info->ExceptionRecord->ExceptionAddress);

        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;

        std::string symbol_info;

        if (SymFromAddr(GetCurrentProcess(), (DWORD64)info->ExceptionRecord->ExceptionAddress, &displacement, symbol)) {
            symbol_info = std::format(("{}!{} + {:#x}"), module_name, symbol->Name, displacement);
        }
        else {
            symbol_info = std::format(("{} + {:#x}"), module_name, (DWORD64)info->ExceptionRecord->ExceptionAddress - module_base);
        }

        auto message = std::format(("Exception code: {:#x}\nException information: {}\n"), (uintptr_t)info->ExceptionRecord->ExceptionCode, symbol_info);

        if (info->ContextRecord->Ebp != 0) {
            message += ("\n");

            auto ebp = info->ContextRecord->Ebp;

            while (true) {
                const auto eip_ebp = ebp + 4;
                const auto eip = *(uint32_t*)eip_ebp;

                if (eip == 0)
                    break;

                ebp = *(uint32_t*)ebp;

                const auto [module_base, module_name] = GetContainingModule((uint64_t)eip);

                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = 255;

                if (SymFromAddr(GetCurrentProcess(), (DWORD64)eip, &displacement, symbol)) {
                    message += std::format(("> {}!{} + {:#x}\n"), module_name, symbol->Name, displacement);
                }
                else {
                    message += std::format(("> {} + {:#x}\n"), module_name, (DWORD64)eip - module_base);
                }
            }
        }

        MessageBoxA(nullptr, message.data(), nullptr, MB_ICONERROR | MB_OK);
        ExitProcess(0);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


void Initialize(HMODULE hModule) {
    while (!GetModuleHandle("serverbrowser.dll"))
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));


    setlocale(LC_ALL, "ru_RI.UTF-8");

#ifndef _DEBUG
    AddVectoredExceptionHandler(true, ExceptionHandler);
#endif

    Interfaces::Initialize();
    Hooks::Initialize();
    Lua->Setup();
    SkinChanger->FixViewModelSequence();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Initialize, hModule, 0, 0);
    return TRUE;
}