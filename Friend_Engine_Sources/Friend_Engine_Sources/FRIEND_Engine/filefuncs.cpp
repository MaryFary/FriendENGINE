#ifndef WINDOWS
#include <dirent.h>
#include <unistd.h>
#ifdef UNIX
#ifndef LINUX
#include "copyfile.h"
#endif
#endif
#include <wordexp.h>
#else
#include "dirent.h"
#include "direct.h"
#include "io.h"
#endif

#include "defs.h"
#include <sys/stat.h>
#include "filefuncs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "utils.h"

char exePath[500];

// performs a shell like expansion of fileName
void expandFilename(char *fileName)
{
#ifndef WINDOWS
	wordexp_t exp_result;

	wordexp(fileName, &exp_result, 0);
	fileName[0] = 0;
	strcpy(fileName, exp_result.we_wordv[0]);
	wordfree(&exp_result);

#endif

	char actualPath[500], actualSuperPath[500];
	// getting the actual directory
	if (strlen(exePath)== 0) getcwd(exePath, 500);
	strcpy(actualPath, exePath);
	strcpy(actualSuperPath, actualPath);

	includeTrailingPathDelimiter(actualPath);

	// resolving . and ..
	string auxName = fileName;
	string path = actualPath;
	string superPath = actualSuperPath;
	string toChange;

	toChange = "..";
	toChange += PATHSEPCHAR;
	int hasTwoDots = 0;
	size_t pos;
	while ((pos = auxName.find(toChange, 0)) != string::npos) {
		hasTwoDots = 1;
		extractFilePath(actualSuperPath, actualSuperPath);
		auxName.erase(pos, toChange.length());
	}
	if (hasTwoDots)
	{
		includeTrailingPathDelimiter(actualSuperPath);
		superPath = actualSuperPath;
		auxName = superPath + auxName;
	}
	

	toChange = ".";
	toChange += PATHSEPCHAR;
	replaceAll(auxName, toChange, path);
	strcpy(fileName, auxName.c_str());
}

void mergeFiles(const char *fromfileA, const char *fromfileB, const char *tofile)
{
	ofstream dest(tofile, ios::binary);

	ifstream sourceA(fromfileA, ios::binary);
	dest << sourceA.rdbuf();
	sourceA.close();

	ifstream sourceB(fromfileB, ios::binary);
	dest << sourceB.rdbuf();
	sourceB.close();

	dest.close();
}

void copyFile(const char *fromfile, const char *tofile)
{
	ifstream source(fromfile, ios::binary);
	ofstream dest(tofile, ios::binary);

	dest << source.rdbuf();

	source.close();
	dest.close();
}

// just copying the files
void copyfile(char *fromfile, char *tofile)
{
#ifdef UNIX
#ifndef LINUX
   copyfile_state_t state;
   state = copyfile_state_alloc();
   copyfile(fromfile, tofile, state, COPYFILE_DATA | COPYFILE_XATTR);
   copyfile_state_free(state);
#endif
#endif
}

// copy directory
int copyDirectory(const char *dirSource, const char *dirDest)
{
	DIR *dirIn;
	struct dirent *entry;
	char pathIn[PATH_MAX], pathOut[PATH_MAX];

#ifdef WIN32
	_mkdir(dirDest);
#else
	mkdir(dirDest, 0777); // notice that 777 is different than 0777
#endif

	dirIn = opendir(dirSource);
	if (dirIn == NULL) {
		return 0;
	}

	while ((entry = readdir(dirIn)) != NULL) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			sprintf(pathIn, "%s%c%s", dirSource, PATHSEPCHAR,  entry->d_name);
			sprintf(pathOut, "%s%c%s", dirDest, PATHSEPCHAR, entry->d_name);
			if (entry->d_type == DT_DIR) {
				copyDirectory(pathIn, pathOut);
			}
			else if (entry->d_type == DT_REG)
			    copyFile(pathIn, pathOut);
		}

	}
	closedir(dirIn);

	return 1;
}

// just deleting a directory
int removeDirectory(const char *dirName)
{
   DIR *dir;
   struct dirent *entry;
   char path[PATH_MAX];
   
   dir = opendir(dirName);
   if (dir == NULL) {
      return 0;
   }
   
   while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
         sprintf(path, "%s/%s", dirName, entry->d_name);
         if (entry->d_type == DT_DIR) {
            removeDirectory(path);
         }        
         remove(path);
      }
      
   }
   closedir(dir);
   remove(dirName);
   
   return 1;
}

// verifying if a file is ready to be read
bool isReadable(char *fileName)
{
   fstream filen;
   int r;
   if (!fileExists(fileName)) return false; 
   filen.open(fileName, ios::in | ios::out);
   if (filen.good()) r =true;
   else r = false;
   filen.close();
   return r;
}

// checks if fileName or fileName.gz exists and return the one that exists
int returnFileNameExists(char *fileName, char *fileNameExists)
{
   int resp;
#ifdef WIN32
   if (_access(fileName, 0) != -1)
#else
      if (access(fileName, 0) != -1)
#endif
         resp=1;
      else resp=0;
   
   if (resp==0)
   {
      char auxfilename[2048];
      sprintf(auxfilename, "%s%s", fileName, ".gz");
#ifdef WIN32
      if (_access(auxfilename, 0) != -1)
#else
         if (access(auxfilename, 0) != -1)
#endif
            resp=1;
         else resp=0;
      if (resp)
         sprintf(fileNameExists, "%s", auxfilename);
   }
   else sprintf(fileNameExists, "%s", fileName);
   return resp;
}

// checks if a file exists
int _fileExists(char *fileName)
{
	int resp;
#ifdef WIN32
	if (_access(fileName, 0) != -1)
#else
	if (access(fileName, 0) != -1)
#endif
		resp = 1;
	else resp = 0;
	return resp;
}

// checks if a file exists
int fileExists(char *fileName)
{
   int resp;
   if (fileName==NULL) return 0;
   resp = _fileExists(fileName);
   char auxFileName[2048];

   if (resp == 0)
   {
      sprintf(auxFileName, "%s%s", fileName, ".gz");
      resp = _fileExists(auxFileName);
      if (resp == 0)
      {
         sprintf(auxFileName, "%s%s", fileName, ".nii");
         resp = _fileExists(auxFileName);
         if (resp == 0)
         {
            sprintf(auxFileName, "%s%s", fileName, ".nii.gz");
            resp = _fileExists(auxFileName);
         }
      }
      if (resp)
         strcpy(fileName, auxFileName);
   }
   return resp;
}

// returns the path of `fileName` without the last PATHSEPARATOR. Note `output` must have at least the same size than `fileName`
void extractFilePath(char *fileName, char *output)
{
	strcpy(output, fileName);
	int i;
	for(i=strlen(output)-1;i >= 0; i--)
		if (output[i] == PATHSEPCHAR)
		{
			output[i]=0;
			break;
		}
	if (i <= 0) output[0] = 0;
}

// returns the filename part of `fileName`. Note `output` must have at least the same size than `fileName`
void extractFileName(char *fileName, char *output)
{
	for(int i=strlen(fileName)-1; i >= 0; i--)
		if (fileName[i] == PATHSEPCHAR)
		{
			strcpy(output, (fileName + i+1));
			break;
		}
}

// returns the path of `fileName`. Please free the memory. I cant garantee if I do
char *extractFileName(char *file)
{
   char *fileName = (char *) malloc((strlen(file)+1) * sizeof(char));
   for (int i=strlen(file)-1;i >= 0; i--)
       if (file[i] == PATHSEPCHAR) 
       {
          strcpy(fileName, (file + i+1));
          break;
       }
   return fileName;
}

// just changing the extension of the file. Note output must have the necessary size
void changeFileExt(char *filename, const char* newExt, char *output)
{
    strcpy(output, filename);
    char *end=strrchr(output, '.');
    if (end)
    {
        end[0]=0;
        strcat(output, newExt);
    }
	else strcat(output, newExt);
}

// including a last path delimiter
void includeTrailingPathDelimiter(char *path)
{
   if (path[strlen(path)-1] != PATHSEPCHAR)
   {
      int pathSize=strlen(path);
      path[pathSize] = PATHSEPCHAR;
      path[pathSize+1] = 0;
   }
}
