#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <windows.h>

#define PRINT_LOG false

unsigned long long typedef uint64;

#pragma pack(push,1)
struct entry {
	int Year;
	int Month;
	int Day;
	int Hour;
	int Minute;
	int Second;

	int LinesCount;
};
#pragma pack(pop)

uint64 CountLinesFile(char* FileDir)
{
	uint64 LinesCount = 0;

	HANDLE FileHandle;
	FileHandle = CreateFile(FileDir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesCount = GetFileSize(FileHandle, NULL);

		void *FileBuffer = malloc(BytesCount);
		DWORD BytesReadCount;
		ReadFile(FileHandle, FileBuffer, BytesCount, &BytesReadCount, NULL);

		char* CharBuffer = (char *)FileBuffer;
		for (int FileIndex = 0; FileIndex < BytesCount; FileIndex++) {
			if (CharBuffer[FileIndex] == '\n') {
				LinesCount++;
			}
		}
		
	} else {
		if (PRINT_LOG) {
			fprintf(stdout, "Problem opening file \n");
		}
	}

	CloseHandle(FileHandle);

	// Add one for the last line
	LinesCount += 1;
	
	return (LinesCount);
}

uint64 CountLines(char* Dir)
{
	uint64 LinesCount = 0;

	// This handles the case of Dir pointing directly to a file
	LinesCount += CountLinesFile(Dir);

	if (PRINT_LOG) {
		fprintf(stdout, "Checking directory %s \n", Dir);
	}

	char StarPath[100];
	strcpy(StarPath, Dir);
	strcat(StarPath, "*");

	WIN32_FIND_DATA FileData = {};
	HANDLE FileHandle = FindFirstFile(StarPath, &FileData);

	if (FileHandle != INVALID_HANDLE_VALUE) {
		while (FindNextFile(FileHandle, &FileData)) {
			if (FileData.cFileName[0] != '.') {
				if (FileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
					char ChildDir[100];
					strcpy(ChildDir, Dir);
					strcat(ChildDir, FileData.cFileName);
					strcat(ChildDir, "\\");

					LinesCount += CountLines(ChildDir);
				} else {
					char FileDir[100];
					strcpy(FileDir, Dir);
					strcat(FileDir, FileData.cFileName);

					if (PRINT_LOG) {
						fprintf(stdout, "Opening File %s \n", FileDir);
					}

					LinesCount += CountLinesFile(FileDir);
				}
			}
		}
	}

	return (LinesCount);
}

char* IntToChar(int Input)
{
	char* Buffer = (char *)malloc(sizeof(char) * 100);
	sprintf(Buffer, "%d", Input);
	return (Buffer);
}

int
main(int ArgCount, char **Args)
{
	if (ArgCount == 3) {
		if (strcmp(Args[1], "-csv") == 0) {
			char* History = Args[2];
			HANDLE HistoryFileHandle = CreateFile(History, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			int BytesCount = GetFileSize(HistoryFileHandle, NULL);
			int HistoryCount = BytesCount / sizeof(entry);

			void *FileData = malloc(sizeof(entry) * HistoryCount);
			DWORD BytesReadCount;
			ReadFile(HistoryFileHandle, FileData, BytesCount, &BytesReadCount, NULL);

			char FinalFile[100000] = {};

			strcat(FinalFile, "Year,Month,Day,Hour,Minute,Second,LinesCount\n");

			entry* HistoryData = (entry*)FileData;
			for (int EntryIndex = 0; EntryIndex < HistoryCount; EntryIndex++) {
				entry* NextEntry = &HistoryData[EntryIndex];

				strcat(FinalFile, IntToChar(NextEntry->Year));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Month));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Day));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Hour));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Minute));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Second));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->LinesCount));

				strcat(FinalFile, "\n");
			}

			HANDLE CSVHandle = CreateFile("CSV.csv", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			DWORD BytesWritten;
			WriteFile(CSVHandle, FinalFile, sizeof(FinalFile), &BytesWritten, NULL);
		} else {
			char* PathFilesCounting = Args[1];
			char* OutputFile = Args[2];

			bool addEntry = false;
			int HistoryFileHandle = _open(OutputFile, _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
			if (HistoryFileHandle == -1) {
				int HistoryFileHandle = _open(OutputFile, _O_RDWR | _O_BINARY | _O_CREAT, _S_IREAD | _S_IWRITE);
				addEntry = true;
			}

			if (!addEntry) {
				// Only update if we haven't saved an entry for today

				SYSTEMTIME CurrentTime = {};
				GetLocalTime(&CurrentTime);

				// Get the previous entry time
				void *FileData = malloc(sizeof(SYSTEMTIME));
				_read(HistoryFileHandle, FileData, sizeof(SYSTEMTIME));
				SYSTEMTIME* PrevEntryTime = (SYSTEMTIME*)FileData;

				// Not adding new entry, because there already is one for today
				addEntry = true;
			}

			if (addEntry) {

				uint64 LinesCount = CountLines(PathFilesCounting);

				SYSTEMTIME CurrentTime = {};
				GetLocalTime(&CurrentTime);

				entry NewEntry = {};
				NewEntry.LinesCount = LinesCount;
				NewEntry.Year = CurrentTime.wYear;
				NewEntry.Month = CurrentTime.wMonth;
				NewEntry.Day = CurrentTime.wDay;
				NewEntry.Hour = CurrentTime.wHour;
				NewEntry.Minute = CurrentTime.wMinute;
				NewEntry.Second = CurrentTime.wSecond;

				if ((_lseek(HistoryFileHandle, 0, SEEK_END) >= 0) &&
				        (_write(HistoryFileHandle, &NewEntry, sizeof(NewEntry)) == sizeof(NewEntry))) {
					// Writing was successful
				} else {
					fprintf(stderr, "ERROR: Unable to append new entry to file \n");
				}

				fprintf(stdout, "LIZARD LINES: %I64i  \n", LinesCount);
			}

			_close(HistoryFileHandle);
		}
	} else {
		fprintf(stdout, "Usage: \n	LizardLines <code dir> <output file dir> \n 	-csv : output csv file of current stats \n");
	}
}