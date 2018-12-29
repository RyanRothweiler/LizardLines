#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <windows.h>

#define PRINT_LOG false

unsigned long typedef uint32;

#pragma pack(push,1)
struct entry {
	int Year;
	int Month;
	int Day;

	uint32 LinesCount;
};
#pragma pack(pop)

char** FileTypes;
int FileTypesCount = 0;

char* DefaultFileTypes[] = {
	"java", "cpp", "h", "py"
};


uint32 CountLinesFile(char* FileDir)
{
	uint32 LinesCount = 0;

	// Make sure the file is of the correct type. Start at the end of the path making sure it matches one of our file types
	int DirLength = strlen(FileDir);

	bool* Matches = (bool*)malloc(sizeof(bool) * FileTypesCount);
	for (int Index = 0; Index < FileTypesCount; Index++) {
		Matches[Index] = true;
	}

	int PotentialMatchCount = FileTypesCount;
	for (int DirIndex = DirLength - 1; DirIndex >= 0; DirIndex--) {

		for (int TypeIndex = 0; TypeIndex < FileTypesCount; TypeIndex++) {

			if (Matches[TypeIndex]) {

				int TypeLength = strlen(FileTypes[TypeIndex]);

				// - 1 because of zero indexing
				int DirCountMoved = DirLength - DirIndex - 1;
				int TypeEndIndex = TypeLength - DirCountMoved - 1;
				if (FileTypes[TypeIndex][TypeEndIndex] == FileDir[DirIndex]) {

					// If we're at the end of the type. then we know the types match
					if (strlen(FileTypes[TypeIndex]) <= DirCountMoved + 1) {
						goto VALIDFILE;
					}
				} else {
					Matches[TypeIndex] = false;

					// If there are no more potential matches left, then this isn't the correct type.
					PotentialMatchCount--;
					if (PotentialMatchCount <= 0) {
						return 0;
					}
				}
			}
		}
	}

VALIDFILE:

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

uint32 CountLines(char* Dir)
{
	uint32 LinesCount = 0;

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

					char ChildDir[MAX_PATH];

					strcpy(ChildDir, Dir);
					strcat(ChildDir, FileData.cFileName);
					strcat(ChildDir, "\\");

					LinesCount += CountLines(ChildDir);
				} else {

					char FileDir[MAX_PATH];

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
	if (strcmp(Args[1], "-csv") == 0) {

		char* HistoryFilePath = Args[2];
		char* OutputFilePath = Args[3];

		int HistoryFileHandle = _open(HistoryFilePath, _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
		if (HistoryFileHandle != -1) {

			// Get the size of the file
			_lseek(HistoryFileHandle, 0, SEEK_END);
			int BytesCount = tell(HistoryFileHandle);
			_lseek(HistoryFileHandle, 0, SEEK_SET);

			int HistoryCount = BytesCount / sizeof(entry);

			void *FileData = malloc(BytesCount);
			_read(HistoryFileHandle, FileData, BytesCount);


			char* Header = "Year,Month,Day,LinesCount\n";

			// This is a guestimate, but always larger than the actual
			int MaxPerHistory = 20;
			int OutputFileSize = (HistoryCount * MaxPerHistory) + strlen(Header);
			char* FinalFile = (char*)malloc(OutputFileSize);
			ZeroMemory(FinalFile, OutputFileSize);

			strcat(FinalFile, Header);

			entry* HistoryData = (entry*)FileData;
			for (int EntryIndex = 0; EntryIndex < HistoryCount; EntryIndex++) {
				entry* NextEntry = &HistoryData[EntryIndex];

				strcat(FinalFile, IntToChar(NextEntry->Year));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Month));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->Day));
				strcat(FinalFile, ",");
				strcat(FinalFile, IntToChar(NextEntry->LinesCount));

				strcat(FinalFile, "\n");
			}

			int OutputFile = _open(OutputFilePath, _O_RDWR | _O_BINARY | _O_CREAT, _S_IREAD | _S_IWRITE);
			if (OutputFile != -1 &&
			        _write(OutputFile, FinalFile, OutputFileSize) == OutputFileSize) {
				fprintf(stdout, "Successfully wrote file. \n");
			} else {
				fprintf(stderr, "ERROR: Unable to write final file. \n");
			}
		}
	} else {
		char* PathFilesCounting = Args[1];
		char* OutputFile = Args[2];

		int HistoryFileHandle = _open(OutputFile, _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);

		// Check if we've alrady entered an entry for today
		if (HistoryFileHandle != -1) {

			// Get the size of the file
			_lseek(HistoryFileHandle, 0, SEEK_END);
			int BytesCount = tell(HistoryFileHandle);
			_lseek(HistoryFileHandle, 0, SEEK_SET);

			int HistoryCount = BytesCount / sizeof(entry);

			void *FileData = malloc(BytesCount);
			_read(HistoryFileHandle, FileData, BytesCount);

			entry* HistoryData = (entry*)FileData;
			entry* LastEntry = &HistoryData[HistoryCount - 1];

			SYSTEMTIME CurrentTime = {};
			GetLocalTime(&CurrentTime);
			if (LastEntry->Day == CurrentTime.wDay) {

				// We already have an entry for today. So return out.
				_close(HistoryFileHandle);
				return 0;
			}
		}

		HistoryFileHandle = _open(OutputFile, _O_RDWR | _O_BINARY | _O_CREAT, _S_IREAD | _S_IWRITE);

		FileTypesCount = ArgCount - 3;
		if (FileTypesCount > 0) {
			// Use the arguments provided

			FileTypes = (char**)malloc(FileTypesCount * sizeof(char*));
			for (int index = 0; index < FileTypesCount; index++) {
				FileTypes[index] = Args[index + 3];
			}
		} else {
			// Fill with default info

			FileTypesCount = sizeof(DefaultFileTypes) / sizeof(DefaultFileTypes[0]);
			FileTypes = (char**)malloc(FileTypesCount * sizeof(char*));

			for (int index = 0; index < FileTypesCount; index++) {
				FileTypes[index] = DefaultFileTypes[index];
			}
		}

		uint32 LinesCount = CountLines(PathFilesCounting);

		SYSTEMTIME CurrentTime = {};
		GetLocalTime(&CurrentTime);

		entry NewEntry = {};
		NewEntry.LinesCount = LinesCount;
		NewEntry.Year = CurrentTime.wYear;
		NewEntry.Month = CurrentTime.wMonth;
		NewEntry.Day = CurrentTime.wDay;

		if (
		    (_lseek(HistoryFileHandle, 0, SEEK_END) >= 0) &&
		    (_write(HistoryFileHandle, &NewEntry, sizeof(NewEntry)) == sizeof(NewEntry))
		) {
			fprintf(stdout, "LIZARD LINES: %I32i  \n", LinesCount);
		} else {
			fprintf(stderr, "ERROR: Unable to append new entry to file \n");
		}


		_close(HistoryFileHandle);
	}
}