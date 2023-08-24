#define _GNU_SOURCE
#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void loader_cleanup();
void load_elf_header(int fd);
void load_program_headers(int fd);
void map_and_run_executable(int fd);
void run_entry_point(void *entry_point);
void close_file_descriptor(int fd);

void free_elf_header() {
    if (ehdr) {
        free(ehdr);
        ehdr = NULL;
    }
}

void free_program_headers() {
    if (phdr) {
        free(phdr);
        phdr = NULL;
    }
}


void loader_cleanup() {
    free_elf_header();
    free_program_headers();
}

void load_elf_header(int fd) {
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        printf("Memory allocation error");
        close(fd);
        exit(1);
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        printf("Error reading ELF header");
        close(fd);
        exit(1);
    }
}

void load_program_headers(int fd) {
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);

    lseek(fd, ehdr->e_phoff, SEEK_SET);
    if (read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum) != ehdr->e_phentsize * ehdr->e_phnum) {
        printf("Error reading program headers");
        close(fd);
        exit(1);
    }
}

void map_and_run_executable(int fd) {
    void *entry_point;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (ehdr->e_entry >= (int)(void *)phdr[i].p_vaddr) {
                if(ehdr->e_entry < (int)(void *)(phdr[i].p_vaddr + phdr[i].p_memsz)){
                void *mem = mmap((void *)phdr[i].p_vaddr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, phdr[i].p_offset);
                entry_point = mem + ehdr->e_entry - phdr[i].p_vaddr;
                if (mem == MAP_FAILED) {
                    printf("Memory mapping error");
                    close(fd);
                    exit(1);
                }
                lseek(fd, phdr[i].p_offset, SEEK_SET);
                if (read(fd, mem, phdr[i].p_filesz) != phdr[i].p_filesz) {
                    printf("segment data not read successfully");
                    close(fd);
                    exit(1);
                }
                run_entry_point(entry_point);
            }
            }
        }
    }
}

void run_entry_point(void *entry_point) {
    int (*_start)() = (int (*)())entry_point;
    int result = _start();
    printf("output: %d\n", result);
}

void close_file_descriptor(int fd) {
    close(fd);
}

void load_and_run_elf(char **exe) {
    int fd = open(exe[1], O_RDONLY);

    if (fd == -1) {
        printf("Error opening file");
        exit(1);
    }

    load_elf_header(fd);
    load_program_headers(fd);
    map_and_run_executable(fd);
    close_file_descriptor(fd);
}

int main(int argc, char **argv) {
    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}
