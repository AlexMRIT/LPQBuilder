#pragma once

#include <iostream>
#include <Windows.h>
#include <fstream>
#include <zlib.h>
#include <dirent.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <iterator>

typedef int (*_compress2Func)(Bytef*, uLongf*, const Bytef*, uLong, int);
typedef int (*_uncompress2Func)(Bytef*, uLongf*, const Bytef*, uLong);
typedef uLongf (*_compressBoundFunc)(uLongf);

typedef struct LPQFILEHANDLER
{
	std::size_t original_size;
	uLongf compressed_size;
	std::streamsize lpq_memory_position;
	char* pFileName;
	char* lpq_file_name;
} lpqedict_t;

typedef struct BHLFILEHANDLER
{
	char* lpq_file_name;
	std::vector<lpqedict_t*> lpq_t;
} bhledict_t;

typedef struct FILEBUFFER
{
	Bytef* pBuffer;
	char* pFileName;
} file_buffer;

class LpqEngine
{
public:
	LpqEngine()
	{
		zlib = LoadLibrary(L"zlib.dll");

		if (zlib == nullptr)
		{
			std::cout << "Failed to load zlib.dll" << std::endl;
			return;
		}

		compress2 = (_compress2Func)GetProcAddress(zlib, "compress2");
		uncompress = (_uncompress2Func)GetProcAddress(zlib, "uncompress");
		compressBound = (_compressBoundFunc)GetProcAddress(zlib, "compressBound");

		if (compress2 == nullptr || uncompress == nullptr || compressBound == nullptr)
		{
			std::cout << "Failed to get function addresses from zlib.dll" << std::endl;
			return;
		}
	}

	~LpqEngine() 
	{
		if (zlib != nullptr)
			FreeLibrary(zlib);
	}

	void compress(const char* __restrict patchname, const char* __restrict filename);
	std::vector<bhledict_t*> load();
	std::vector<lpqedict_t*> find_lpq_files(std::vector<bhledict_t*>& bhl_buffer, const char* filename);
	void save_file(const file_buffer& file);
	file_buffer* decompress(const std::vector<lpqedict_t*>& lpqs, const char* filename);

private:
	int get_count_files(const char* patchname);
	void log(const char* message, const char* patchfile);
	Bytef* compress_string(std::vector<Bytef> buffer, uLongf& compressed_size, int level) const;
	Bytef* decompress_string(const Bytef* compressed_data, uLongf compressed_size, std::size_t original_size) const;

	HMODULE zlib = nullptr;
	_compress2Func compress2 = nullptr;
	_uncompress2Func uncompress = nullptr;
	_compressBoundFunc compressBound = nullptr;
};