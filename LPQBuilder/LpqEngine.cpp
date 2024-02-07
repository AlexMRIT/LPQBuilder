#include "LpqEngine.h"

void LpqEngine::compress(const char* __restrict patchname, const char* __restrict filename)
{
    int countFiles = get_count_files(patchname);

    DIR* dirp = opendir(patchname);
    struct dirent* dp;

    char file_name_bhl[64];
    snprintf(file_name_bhl, sizeof(file_name_bhl), "%s.bhl", filename);
    std::ofstream file_bhl(file_name_bhl, std::ios::binary);
    if (!file_bhl)
    {
        log("Error file: ", file_name_bhl);
        return;
    }

    file_bhl.write(reinterpret_cast<const char*>(&countFiles), sizeof(int));

    char file_name_lpq[64];
    snprintf(file_name_lpq, sizeof(file_name_lpq), "%s.lpq", filename);
    std::ofstream file_lpq(file_name_lpq, std::ios::binary);
    if (!file_lpq)
    {
        log("Error file: ", file_name_lpq);
        return;
    }

    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_type == DT_REG)
        {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", patchname, dp->d_name);
            std::ifstream file(file_path, std::ifstream::binary);
            std::cout << "Attempting to read and compress a file: " << file_path << std::endl;

            if (file.is_open())
            {
                file.seekg(0, std::ifstream::end);
                std::streamsize size = file.tellg();
                file.seekg(0, std::ifstream::beg);

                std::vector<Bytef> buffer(size);
                file.read(reinterpret_cast<char*>(buffer.data()), size);
                std::streamsize lpq_position = file_lpq.tellp();

                uLongf compressed_size = 0;
                std::size_t original_size = buffer.size();
                Bytef* compressed_buffer = compress_string(buffer, compressed_size, Z_BEST_SPEED);
                file_bhl.write(reinterpret_cast<const char*>(&original_size), sizeof(std::size_t));
                file_bhl.write(reinterpret_cast<const char*>(&compressed_size), sizeof(uLongf));
                file_bhl.write(reinterpret_cast<const char*>(&lpq_position), sizeof(std::streamsize));

                file_lpq.write(reinterpret_cast<const char*>(compressed_buffer), compressed_size);

                std::vector<Bytef> file_name(std::begin(dp->d_name), std::end(dp->d_name));
                original_size = file_name.size();
                Bytef* compress_buffer_file_name = compress_string(file_name, compressed_size, Z_BEST_SPEED);
                file_bhl.write(reinterpret_cast<const char*>(&original_size), sizeof(std::size_t));
                file_bhl.write(reinterpret_cast<const char*>(&compressed_size), sizeof(uLongf));
                file_bhl.write(reinterpret_cast<const char*>(compress_buffer_file_name), compressed_size);
                free(compressed_buffer);
                free(compress_buffer_file_name);
            }
            else
            {
                log("Error reading file: ", file_path);
                return;
            }

            file.close();
        }
    }

    file_lpq.close();
    file_bhl.close();
    closedir(dirp);
}

std::vector<bhledict_t*> LpqEngine::load()
{
    DIR* dir;
    struct dirent* ent;
    std::vector<bhledict_t*> vecBhlEdict;

    char self_path[MAX_PATH];
    GetModuleFileNameA(NULL, self_path, MAX_PATH);

    for (int iterator = MAX_PATH; iterator > 0; iterator--)
    {
        if (self_path[iterator - 1] == '\\')
        {
            self_path[iterator - 1] = '\0';
            break;
        }
    }

    if ((dir = opendir(self_path)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (strstr(ent->d_name, ".bhl") != NULL)
            {
                std::cout << "Reading file: " << ent->d_name << std::endl;

                std::ifstream file(ent->d_name, std::ifstream::binary);
                if (file.is_open())
                {
                    int countFiles = 0;
                    file.read(reinterpret_cast<char*>(&countFiles), sizeof(int));

                    bhledict_t* pBhlEdict = reinterpret_cast<bhledict_t*>(calloc(1, sizeof(bhledict_t)));
                    if (!pBhlEdict)
                    {
                        std::cout << "Error decompressing file: " << ent->d_name << "! The file is damaged." << std::endl;
                        continue;
                    }

                    for (int i = 0; i < countFiles; i++)
                    {
                        lpqedict_t* pLpqEdict = reinterpret_cast<lpqedict_t*>(calloc(1, sizeof(lpqedict_t)));
                        if (!pLpqEdict)
                        {
                            std::cout << "Error decompressing file: " << ent->d_name << "! The file is damaged." << std::endl;
                            continue;
                        }

                        file.read(reinterpret_cast<char*>(&((*pLpqEdict).original_size)), sizeof(std::size_t));
                        file.read(reinterpret_cast<char*>(&((*pLpqEdict).compressed_size)), sizeof(uLongf));
                        file.read(reinterpret_cast<char*>(&((*pLpqEdict).lpq_memory_position)), sizeof(std::streamsize));

                        uLongf compressed_size_filename = 0;
                        std::size_t original_size = 0;
                        file.read(reinterpret_cast<char*>(&original_size), sizeof(std::size_t));
                        file.read(reinterpret_cast<char*>(&compressed_size_filename), sizeof(uLongf));
                        Bytef* buffer_filename = reinterpret_cast<Bytef*>(calloc(compressed_size_filename, sizeof(Bytef)));
                        if (!buffer_filename) continue;

                        file.read(reinterpret_cast<char*>(buffer_filename), compressed_size_filename);
                        (*(pLpqEdict)).pFileName = reinterpret_cast<char*>(decompress_string(buffer_filename, compressed_size_filename, original_size));
                        free(buffer_filename);

                        (*(pBhlEdict)).lpq_t.push_back(pLpqEdict);
                    }

                    (*(pBhlEdict)).lpq_file_name = _strdup(ent->d_name);
                    int nameLenght = 0;
                    int allSize = strlen(ent->d_name);
                    for (int i = 0; i < allSize; i++)
                    {
                        if ((*(pBhlEdict)).lpq_file_name[i] == '.')
                        {
                            nameLenght = i;
                            break;
                        }
                    }

                    strcpy((*(pBhlEdict)).lpq_file_name + nameLenght, ".lpq");

                    std::cout << "File: " << ent->d_name << " loaded!" << std::endl;
                    vecBhlEdict.push_back(pBhlEdict);
                }

                file.close();
            }
        }

        closedir(dir);
    }

    return vecBhlEdict;
}

std::vector<lpqedict_t*> LpqEngine::find_lpq_files(std::vector<bhledict_t*>& bhl_buffer, const char* filename)
{
    std::size_t size = bhl_buffer.size();
    for (int i = 0; i < size; i++)
    {
        if (strcmp((*(bhl_buffer[i])).lpq_file_name, filename) == 0)
        {
            for (int a = 0; a < (*(bhl_buffer[i])).lpq_t.size(); a++)
                (*(bhl_buffer[i])).lpq_t[a]->lpq_file_name = (*(bhl_buffer[i])).lpq_file_name;

            return (*(bhl_buffer[i])).lpq_t;
        }
    }

    return std::vector<lpqedict_t*>();
}

void LpqEngine::save_file(const file_buffer& file_pBuffer)
{
    if (file_pBuffer.pBuffer)
    {
        std::ofstream file(file_pBuffer.pFileName, std::ifstream::binary);
        if (file.is_open())
        {
            std::size_t size = strlen(reinterpret_cast<const char*>(file_pBuffer.pBuffer));
            file.write(reinterpret_cast<const char*>(file_pBuffer.pBuffer), size);
        }
    }
}

file_buffer* LpqEngine::decompress(const std::vector<lpqedict_t*>& lpqs, const char* filename)
{
    std::size_t size = lpqs.size();
    for (int i = 0; i < size; i++)
    {
        if (strcmp((*(lpqs[i])).pFileName, filename) == 0)
        {
            std::ifstream file((*(lpqs[i])).lpq_file_name, std::ifstream::binary);
            if (file.is_open())
            {
                file.seekg((*(lpqs[i])).lpq_memory_position, std::ifstream::beg);

                file_buffer* pfb = reinterpret_cast<file_buffer*>(calloc(1, sizeof(file_buffer)));
                if (!pfb) return nullptr;

                std::vector<Bytef> buffer((*(lpqs[i])).compressed_size);
                file.read(reinterpret_cast<char*>(buffer.data()), (*(lpqs[i])).compressed_size);

                if (file.gcount() != (*(lpqs[i])).compressed_size) 
                {
                    std::cout << "Error: file is too short" << std::endl;
                    return nullptr;
                }

                (*(pfb)).pBuffer = decompress_string(reinterpret_cast<const Bytef*>(buffer.data()), (*(lpqs[i])).compressed_size, (*(lpqs[i])).original_size);
                (*(pfb)).pBuffer[(*(lpqs[i])).original_size] = '\0';
                (*(pfb)).pFileName = _strdup((*(lpqs[i])).pFileName);

                file.close();
                return pfb;
            }
            else
            {
                std::cout << "File not found: " << (*(lpqs[i])).pFileName << std::endl;
                return nullptr;
            }
        }
    }

    return nullptr;
}

int LpqEngine::get_count_files(const char* patchname)
{
    DIR* dirp = opendir(patchname);
    struct dirent* dp;
    int file_count = 0;

    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_type == DT_REG)
            ++file_count;
    }

    closedir(dirp);
    return file_count;
}

void LpqEngine::log(const char* message, const char* patchfile)
{
    char message_error[128];
    snprintf(message_error, sizeof(message_error), "%s%s!", message, patchfile);
    MessageBox(NULL, (LPCWSTR)message_error, (LPCWSTR)L"Error Details", MB_ICONERROR | MB_OK);
}

Bytef* LpqEngine::compress_string(std::vector<Bytef> buffer, uLongf& compressed_size, int level) const
{
    uLongf compressed_size2 = compressBound(buffer.size() + 1);
    Bytef* compressed_data = reinterpret_cast<Bytef*>(calloc(compressed_size2, sizeof(Bytef)));

    if (compressed_data == nullptr)
        return nullptr;

    if (compress2((Bytef*)compressed_data, &compressed_size2, (const Bytef*)buffer.data(), buffer.size() + 1, level) != Z_OK)
    {
        std::cerr << "Error compressing string." << std::endl;
        compressed_size = 0;
        free(compressed_data);
        return nullptr;
    }

    compressed_size = compressed_size2;
    return compressed_data;
}

Bytef* LpqEngine::decompress_string(const Bytef* compressed_data, uLongf compressed_size, std::size_t original_size) const
{
    uLongf decompressed_size = original_size + 1;
    Bytef* decompressed_data = reinterpret_cast<Bytef*>(calloc(decompressed_size, sizeof(Bytef)));

    if (decompressed_data == nullptr)
        return nullptr;

    while (true)
    {
        int result = uncompress(decompressed_data, &decompressed_size, compressed_data, compressed_size);

        if (result == Z_OK)
            return decompressed_data;
        else if (result == Z_BUF_ERROR)
        {
            decompressed_size *= 2;
            Bytef* temp = reinterpret_cast<Bytef*>(realloc(decompressed_data, decompressed_size));

            if (temp == nullptr)
            {
                free(decompressed_data);
                return nullptr;
            }

            decompressed_data = temp;
        }
        else
        {
            std::cerr << "Error decompressing." << std::endl;
            free(decompressed_data);
            return nullptr;
        }
    }
}