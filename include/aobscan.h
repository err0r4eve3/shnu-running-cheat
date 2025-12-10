// aobscan.h
#pragma once

const BYTE* aobscan(const BYTE* pattern, size_t patternLen, const BYTE* buffer, size_t bufferLen);
uintptr_t ScanProcessMemory(HANDLE hProcess, const BYTE* pattern, size_t patternLen, uintptr_t startAddr = 0);
