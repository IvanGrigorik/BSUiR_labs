//
// Created by sifi on 28.03.2022.
//


#include "functions.h"

int layer = 0;
int num = 0;
unsigned long all_files_size = 0;

std::string md5_to_string(unsigned char *md) {

    std::stringstream s_stream{};

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        s_stream << std::hex << static_cast<int>(md[i]);
    }

    return s_stream.str();
}


/* Function to collect files hash to files_vector
 * if the flag is 'n' (names) - we collect files with the same hash, but different names.
 * if the flag is 'd' (default) - we collect files with unique hash (don't look at names).
*/
void collect_files(const std::string &current_dir,
                   std::vector<file_data_t> &unique_files,
                   std::vector<file_to_delete_t> &duplicated_files, char flag) {
    unsigned char result[MD5_DIGEST_LENGTH];

    DIR *dir = opendir((current_dir + "/").c_str());

    if (!dir) {
        if (errno == EACCES) {
            std::cout << "Cant open file: access error" << std::endl;
            closedir(dir);
            return;
        } else {
            std::cerr << strerror(errno) << std::endl;
            closedir(dir);
            exit(errno);
        }
    }

    dirent *file;
    while ((file = readdir(dir))) {
        std::string filename = file->d_name;
        if ((filename == "." || filename == "..")) {
            continue;
        }

        if (file->d_type == DT_DIR) {
            collect_files(current_dir + '/' + filename, unique_files, duplicated_files, flag);
            continue;
        }
        // If it not directory:

        int file_descript;
        unsigned long file_size;
        void *file_buffer;

        file_descript = open((current_dir + '/' + filename).c_str(), O_RDONLY);

        if (file_descript < 0) {
            continue;
        }

        file_size = get_size_by_fd(file_descript);

        // Функция возвращает адрес начала участка отображаемой памяти или MAP_FAILED в случае неудачи.
        file_buffer = mmap(nullptr, file_size, PROT_READ, MAP_SHARED, file_descript, 0);
        if (file_buffer == MAP_FAILED) {
            close(file_descript);
            munmap(file_buffer, file_size);
            perror("Error");
            continue;
        }
        MD5(reinterpret_cast <unsigned char *>(file_buffer), file_size, result);
        munmap(file_buffer, file_size);

        std::string hash = md5_to_string(result);

        all_files_size += file_size;
        std::cout << "All files size: " << all_files_size << std::endl;
        std::cout << ++num << std::endl;
        std::cout << "File descript: " << file_descript << std::endl;


        if (unique_files.empty()) {
            unique_files.emplace_back(filename, hash);
            continue;
        }

        bool is_in_files{};
        for (int i = 0; i < unique_files.size(); i++) {
            if (unique_files[i].file_hash == hash) {

                // If we collect files with same hash, but different names
                if (flag == 'n') {
                    // If names equal - skip
                    if (unique_files[i].file_name == filename) {
                        duplicated_files.emplace_back(file_to_delete_t(filename, (current_dir + '/' + filename)));
                        break;
                    } else {
                        unique_files.emplace_back(filename, hash);
                        break;
                    }
                }
                // And if we should collect only unique hash
                duplicated_files.emplace_back(file_to_delete_t(filename, (current_dir + '/' + filename)));
                is_in_files = true;
            }
        }
        if (!is_in_files && flag != 'n') {
            unique_files.emplace_back(filename, hash);
        }

        close(file_descript);
    }
    closedir(dir);
}


// Get the size of the file by its file descriptor
unsigned long get_size_by_fd(int fd) {
    struct stat statbuf{};
    if (fstat(fd, &statbuf) < 0) {
        exit(-1);
    }
    return statbuf.st_size;
}
