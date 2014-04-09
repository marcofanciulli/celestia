// directory.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
#ifdef WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <wordexp.h>
#endif

#include <iostream>
#include "directory.h"


using namespace std;


bool Directory::enumFiles(EnumFilesHandler& handler, bool deep)
{
    string filename;

    while (nextFile(filename))
    {
        // Skip all files beginning with a period, most importantly, . and ..
        if (filename[0] == '.')
            continue;

        // TODO: optimize this to avoid allocating so many strings
        string pathname = handler.getPath() + string("/") + filename;
        if (IsDirectory(pathname))
        {
            if (deep)
            {
                Directory* dir = OpenDirectory(pathname);
                bool cont = true;

                if (dir != NULL)
                {
                    handler.pushDir(filename);
                    cont = dir->enumFiles(handler, deep);
                    handler.popDir();
                    delete dir;
                }

                if (!cont)
                    return false;
            }
        }
        else
        {
            if (!handler.process(filename))
                return false;
        }
    }

    return true;
}


EnumFilesHandler::EnumFilesHandler()
{
}


void EnumFilesHandler::pushDir(const std::string& dirName)
{
    if (dirStack.size() > 0)
        dirStack.push_back(dirStack.back() + string("/") + dirName);
    else
        dirStack.push_back(dirName);
}


void EnumFilesHandler::popDir()
{
    dirStack.pop_back();
}


const string& EnumFilesHandler::getPath() const
{
    // need this so we can return a non-temporary value:
    static const string emptyString("");

    if (dirStack.size() > 0)
        return dirStack.back();
    else
        return emptyString;
}

#ifdef WIN32

class WindowsDirectory : public Directory
{
public:
    WindowsDirectory(const std::string&);
    virtual ~WindowsDirectory();

    virtual bool nextFile(std::string&);

    enum {
        DirGood = 0,
        DirBad = 1
    };

private:
    string dirname;
    string searchName;
    int status;
    HANDLE searchHandle;
};


WindowsDirectory::WindowsDirectory(const std::string& _dirname) :
    dirname(_dirname),
    status(DirGood),
    searchHandle(INVALID_HANDLE_VALUE)
{
    searchName = dirname + string("\\*");
    // Check to make sure that this file is a directory
}


WindowsDirectory::~WindowsDirectory()
{
    if (searchHandle != INVALID_HANDLE_VALUE)
        FindClose(searchHandle);
    searchHandle = NULL;
}


bool WindowsDirectory::nextFile(std::string& filename)
{
    WIN32_FIND_DATAA findData;

    if (status != DirGood)
        return false;

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        searchHandle = FindFirstFileA(const_cast<char*>(searchName.c_str()),
                                      &findData);
        if (searchHandle == INVALID_HANDLE_VALUE)
        {
            status = DirBad;
            return false;
        }
        else
        {
            filename = findData.cFileName;
            return true;
        }
    }
    else
    {
        if (FindNextFileA(searchHandle, &findData))
        {
            filename = findData.cFileName;
            return true;
        }
        else
        {
            status = DirBad;
            return false;
        }
    }
}


Directory* OpenDirectory(const std::string& dirname)
{
    return new WindowsDirectory(dirname);
}


bool IsDirectory(const std::string& filename)
{
    DWORD attr = GetFileAttributesA(const_cast<char*>(filename.c_str()));
    if (attr == 0xffffffff)
        return false;
    else
        return ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

std::string WordExp(const std::string& filename) {
    return filename;
}

int Directory::chdir(const std::string & dir) {
  return _chdir(dir.c_str());
}
#else

class UnixDirectory : public Directory
{
public:
    UnixDirectory(const std::string&);
    virtual ~UnixDirectory();

    virtual bool nextFile(std::string&);

    enum {
        DirGood = 0,
        DirBad = 1
    };

private:
    string dirname;
    int status;
    DIR* dir;
};


UnixDirectory::UnixDirectory(const std::string& _dirname) :
    dirname(_dirname),
    status(DirGood),
    dir(NULL)
{
}


UnixDirectory::~UnixDirectory()
{
    if (dir != NULL)
    {
        closedir(dir);
        dir = NULL;
    }
}


bool UnixDirectory::nextFile(std::string& filename)
{
    if (status != DirGood)
        return false;

    if (dir == NULL)
    {
        dir = opendir(dirname.c_str());
        if (dir == NULL)
        {
            status = DirBad;
            return false;
        }
    }

    struct dirent* ent = readdir(dir);
    if (ent == NULL)
    {
        status = DirBad;
        return false;
    }
    else
    {
        filename = ent->d_name;
        return true;
    }
}


Directory* OpenDirectory(const std::string& dirname)
{
    return new UnixDirectory(dirname);
}


bool IsDirectory(const std::string& filename)
{
    struct stat buf;
    stat(filename.c_str(), &buf);
    return S_ISDIR(buf.st_mode);
}

std::string WordExp(const std::string& filename)
{
#ifndef WORDEXP_PROBLEM
    wordexp_t result;
    std::string expanded;

    switch(wordexp(filename.c_str(), &result, WRDE_NOCMD)) {
    case 0: // successful
        break;
    case WRDE_NOSPACE:
            // If the error was `WRDE_NOSPACE',
            // then perhaps part of the result was allocated.
        wordfree(&result);
    default: // some other error
        return filename;
    }

    if (result.we_wordc != 1) {
        wordfree(&result);
        return filename;
    }

    expanded = result.we_wordv[0];
    wordfree(&result);
#else
    std::string expanded = filename;
#endif
    return expanded;
}
#endif
