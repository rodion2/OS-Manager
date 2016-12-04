// OS-Manager.cpp: определяет точку входа для консольного приложения.
//
#include "stdafx.h"
#include <vector>
#include <iostream>
#include "windows.h"
#include "ctime"
#include "iostream"
#include "math.h"
#include "string"
#include "atlstr.h"
#include "mutex"
using namespace std;
int currentId = 0;
int getId() {
	return currentId++;
}

HANDLE handle = NULL;
HANDLE event;

struct Process {
	PROCESS_INFORMATION info;
	bool isKilled;
	time_t killingTime;
	char* name;
	char* shortname;
	int id;
	Process(time_t livingTime, char* const name, char* const shortname){
		this->info = info;
		isKilled = false;
		this->killingTime = time(NULL) + livingTime;
		this->name = name;
		this->shortname = shortname;
		this->id = getId();
	}
};

int timeLive;

vector<Process> processes;
HANDLE hMutex;



TCHAR *convertToTCHAR(char *data) {
	size_t newsize = strlen(data) + 1;
	wchar_t * convertetData = new wchar_t[newsize];
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, convertetData, newsize, data, _TRUNCATE);

	return convertetData;
}

bool isProcessValid(Process &process) {
	DWORD dwFlags;
	BOOL result;
	result = GetHandleInformation(&process.info, &dwFlags);

	return !result;
}




int kill_process(Process &process) {
	WaitForSingleObject(hMutex, INFINITE);
	bool killed = false;
	if (TerminateProcess(process.info.hProcess, 0)) {
		process.isKilled = true;
		killed = true;
	}
	else {
		ReleaseMutex(hMutex);
		return GetLastError();
	}
	ReleaseMutex(hMutex);
	return 0;
}


int getProcessNumber(Process &process) {
	WaitForSingleObject(hMutex, INFINITE);
	for (int i = 0; i < processes.size(); i++) {
		if (processes[i].id == process.id) {
			ReleaseMutex(hMutex);
			return i;
		}
	}
	ReleaseMutex(hMutex);
	return -1;
}



void addProcess(Process &process) {
	WaitForSingleObject(hMutex, INFINITE);
	processes.push_back(process);
	ReleaseMutex(hMutex);
}

void deleteProcessByNumber(int number) {
	WaitForSingleObject(hMutex, INFINITE);
	processes.erase(processes.begin() + number);
	ReleaseMutex(hMutex);
}

void deleteProcess(Process &process) {
	WaitForSingleObject(hMutex, INFINITE);
	int number = getProcessNumber(process);

	if (number != -1) {
		deleteProcessByNumber(number);
	}
	ReleaseMutex(hMutex);
}



void tryToCloseProcess(Process &process) {
	kill_process(process);
	//deleteProcess(process);
}

Process& getProcessByNumber(int number) {
	WaitForSingleObject(hMutex, INFINITE);
	if (number >= 0 && number < currentId) {
		ReleaseMutex(hMutex);
		return processes[number];
	}
	ReleaseMutex(hMutex);
}

Process& getProcessByID(int number) {
	WaitForSingleObject(hMutex, INFINITE);
	int check = 0;
	for (int i = 0; i < processes.size(); i++)
	if (number == processes[i].id) {
		ReleaseMutex(hMutex);
		return processes[i];
	}
	ReleaseMutex(hMutex);
}

void validateProcesses() {
	int processesSize = processes.size();

	vector<Process> deletedProcesses;

	for (int i = 0; i < processesSize; i++) {
		Process process = getProcessByNumber(i);
		if (!isProcessValid(process)) {
			deletedProcesses.push_back(process);
		}
	}

	for (int i = 0; i < deletedProcesses.size(); i++) {
		tryToCloseProcess(deletedProcesses[i]);
	}
}

void showAllProcesses()
{
	HANDLE CONST hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	validateProcesses();
	int processesSize = processes.size();

	if (processesSize == 0) {
		cout << " -- No processes --" << endl;
	}
	else{
		printf("Name                                 ID           Time\n");
		printf("==================================== ===== =================\n");
		for (int i = 0; i < processesSize; i++) {
			{
				if (!processes[i].isKilled){
					Process tempProcess = getProcessByNumber(i);
					printf("%36s%6d%18d\n", tempProcess.shortname, tempProcess.id, timeLive);
				}
			}
		}
	}
}

char* getShortName(char* fullName)
{
char *shortName = new char[256];
if (strchr(fullName, '\\') != NULL)//для короткого имени
	{
		int len = strlen(fullName);
		
		int i = len - 1;
		int a = 0;
		while (fullName[i] != '\\')
		{
			shortName[a] = fullName[i];
			i--;
			a++;
		}
		shortName[a] = '\0';
		int length = strlen(shortName) - 1;
		for (int i = 0; i <= length / 2; i++)
		{
			char temp;
			temp = shortName[i];
			shortName[i] = shortName[length - i];
			shortName[length - i] = temp;
		}
	}
	else{ //для полного имени
		strcpy(shortName,fullName);
	}
	return shortName;
}

Process &create_process(char *appName, DWORD priority, int timeLive1) {
	STARTUPINFO startUpInfo = { sizeof(startUpInfo) };
	ZeroMemory(&startUpInfo, sizeof(STARTUPINFO));
	char *shortName = getShortName(appName);
	TCHAR *convertedName = convertToTCHAR(appName);
	int appNameLength = strlen(appName);

	char *name = new char[appNameLength];
	for (int i = 0; i < appNameLength; i++) {
		name[i] = appName[i];
	}
	name[appNameLength] = '\0';

	Process process(timeLive1, name, shortName);

	TCHAR *convertedName2 = new TCHAR[256];
	int i = 0;
	while (convertedName[i] != (L'\0'))
	{
		convertedName2[i] = convertedName[i + 1];
		i++;
	}
	delete[] convertedName;
	if (CreateProcess(NULL, convertedName2, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startUpInfo, &process.info))
	{
		SetPriorityClass(&process.info, priority);
	}
	else {
		throw GetLastError();
	}
	return process;
}

void killAllProcesses() {
	bool notAllProcessesKilled = true;
	int currentProcessNumber = processes.size() - 1;
	while (notAllProcessesKilled) {
		if (currentProcessNumber == -1) {
			notAllProcessesKilled = false;
			break;
		}
		Process process = getProcessByNumber(currentProcessNumber--);
		tryToCloseProcess(process);
	}
}

unsigned int __stdcall live(LPVOID lpParam) {
	//HANDLE hStdout;

	//hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	//if (hStdout == INVALID_HANDLE_VALUE)
		//return 1;
	while (true){
		WaitForSingleObject(event, INFINITE);
		//предполагаем что потоков будет больше 0
		int minI = -1;
		WaitForSingleObject(hMutex,INFINITE);
		for (int i = 0; i < processes.size(); i++){
			if (!processes[i].isKilled)
				if (minI == -1 || processes[i].killingTime < processes[minI].killingTime) minI = i;
		}
		
		time_t timeToWakeUp = minI != -1 ? processes[minI].killingTime - time(NULL): 100000000;
		ReleaseMutex(hMutex);
		
		Sleep(timeToWakeUp);
		if (!processes.empty()) {
			for (int i = 0; i < processes.size(); i++)
			{
				time_t temp = time(NULL);
				if (processes[i].killingTime >= time(NULL))
				{
					Process &process = getProcessByNumber(i);
					tryToCloseProcess(process);
				}
			}
			//	killAllProcesses();
			//cout << "\nAll processes was closed by process manager\n";
			showAllProcesses();
		}
	}
	

	return 0;
}

bool checkID(int id)
{
	int check = 0;
	for (int i = 0; i < processes.size(); i++)
	{
		if (id == processes[i].id) check++;
	}
	if (check != 0)return (true);
	else return(false);
}

void closeProcess() {
	int number = -1;
	cin >> number;
	number;

	if (number != -2 && number < currentId && processes.size() != 0 && checkID(number) == true) {
		Process &process = getProcessByID(number);
		tryToCloseProcess(process);
	}
	showAllProcesses();
}

void createProcess() {
	char executableFileName[256];
	int timeLive = 5;
	int times = 2;
	cin >> timeLive;
	cin >> times;
	cin.getline(executableFileName, 256);
	//char *executableFileName = " C:\\Program Files (x86)\\UltraISO\\UltraISO.exe";
	//cout << "Enter time and number for applications: ";
	
	for (int i = 0; i < times; i++) {
		try{
			if (event==NULL)
				event = CreateEvent(NULL, false, false, NULL);
			Process process = create_process(executableFileName, NORMAL_PRIORITY_CLASS, timeLive*1000);
			addProcess(process);
		}
		catch(...){
			cout << "Error: New process error creation: " << GetLastError() << endl;
		}
	}
	showAllProcesses();
	
	if (handle == NULL){
		handle = (HANDLE)_beginthreadex(NULL, 0, &live, &processes, 0, NULL);
	}
	SetEvent(event);
	if (handle == NULL) {
		cout << "Error: New thread error creation: " << GetLastError() << endl;
	}

}



void setMainProcessHighPriority() {
	HANDLE currentProcess = GetCurrentProcess();
	SetPriorityClass(&currentProcess, HIGH_PRIORITY_CLASS);
}

void initialize() {
	setlocale(LC_ALL, "RUS");
	SetConsoleCP(1251);
	hMutex = CreateMutex(NULL, FALSE, NULL);
	setMainProcessHighPriority();
}

void showAllCommands()
{
	cout << "Input 1 [path to app]\n";
	cout << "Input 2 [id] to close app by id\n";
	cout << "Input 3 to exit the manager\n";
	cout << "Input 4 to show processes\n";
	cout << "Input 5 to show commands\n";
}


void handleCommands() {
	bool exit = false;

	cout << "Type 5 to show all commands" << endl;

	while (!exit) {
		char *a = new char;
		cin >> a;
		switch (*a) {
		case '1':
			createProcess();
			break;
		case '2':
			closeProcess();
			break;
		case '3':
			killAllProcesses();
			exit = true;
			break;
		case '4':
			showAllProcesses();
			break;
		case '5':
			showAllCommands();
			break;
		default:
			cout << "Invalid command, try again" << endl;
			cin.clear();
			break;
		}
	}

}

int main()
{
	initialize();

	handleCommands();

	CloseHandle(hMutex);

	return 0;
}

