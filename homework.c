#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 256

typedef long int FileSize;

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    char permissions[11];
    FileSize size;
} FileInfo;

void GetFileInfo(const char *filename, FileInfo *file) {
    struct stat fileStat;

    if (stat(filename, &fileStat) == -1) {
        perror("Dosya bulunamadı");
        exit(1);
    }

    strcpy(file->filename, filename);
    snprintf(file->permissions, sizeof(file->permissions), "%o", fileStat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

    file->size = fileStat.st_size;
}

void MergeFiles(int fileCount, char *fileList[], char *archiveName){
    FileSize totalFileSize = 0;

    FILE* archive = fopen(archiveName, "w");
    for (int i = 2; i < fileCount - 2; i++) {
        FileInfo file;
        GetFileInfo(fileList[i], &file);
        totalFileSize += file.size;
    }

    if (!archive) {
        perror("Dosya bulunamadı");
    }
    fprintf(archive, "%010ld", totalFileSize);

    for (int i = 2; i < fileCount - 2; i++) {
        FileInfo dataFile;
        GetFileInfo(fileList[i], &dataFile);
        printf("|%s, %s, %ld", dataFile.filename, dataFile.permissions, dataFile.size);
        fprintf(archive, "|%s, %s, %ld", dataFile.filename, dataFile.permissions, dataFile.size);
    }
    printf("|");
    fprintf(archive, "|");

    for (int i = 2; i < fileCount - 2; i++) {
        FileInfo dataFile;
        FILE* openFile = fopen(fileList[i], "r");
        GetFileInfo(fileList[i], &dataFile);
        int fileSize = dataFile.size;
        char* buffer = (char*)malloc(fileSize + 1);
        fread(buffer, 1, fileSize, openFile);
        strcat(buffer, "\n");
        fwrite(buffer, 1, fileSize, archive);
        fclose(openFile);
        free(buffer);
    }
    fclose(archive);
}

void ExtractArchive(char *archiveName, char *outputDirectory) {
    CreateDirectory(outputDirectory);

    FILE *archive = fopen(archiveName, "r");
    if (!archive) {
        perror("Çıkış dosyası çalışmıyor");
        return;
    }

    fseek(archive, 0, SEEK_END);
    long fileSize = ftell(archive);
    fseek(archive, 0, SEEK_SET);

    char *buffer = (char *)malloc(fileSize + 1);
    if (!buffer) {
        perror("Bellek tahsisi hatası");
        fclose(archive);
        return;
    }

    fread(buffer, 1, fileSize, archive);
    buffer[fileSize] = '\0';
    const char delimiters[] = "|,";

    char *token = strtok(buffer, delimiters);
    FileInfo infoFile;
    FileInfo *arrayOfInfoFiles = NULL;
    int infoFileCount = 0;
    int totalFileSize = 0;
    while (token != NULL) {
        if (strstr(token, ".txt") != NULL) {
            strcpy(infoFile.filename, token);
            token = strtok(NULL, delimiters);
            strcpy(infoFile.permissions, token);
            token = strtok(NULL, delimiters);

            if (token != NULL && atoi(token) != 0) {
                infoFile.size = atoi(token);
                totalFileSize += infoFile.size;
                char *fileContent = (char *)malloc(infoFile.size);
                if (!fileContent) {
                    perror("Bellek tahsisi hatası");
                    fclose(archive);
                    free(buffer);
                    return;
                }
                fread(fileContent, 1, infoFile.size, archive);
                free(fileContent);
            }
            arrayOfInfoFiles = realloc(arrayOfInfoFiles, (infoFileCount + 1) * sizeof(FileInfo));
            arrayOfInfoFiles[infoFileCount++] = infoFile;
        }
        token = strtok(NULL, delimiters);
    }

    int fileOffset = fileSize;
    for(int i = infoFileCount - 1; i >=0; --i){
        fileOffset += arrayOfInfoFiles[i].size;
    }

    int currentOffset = fileSize;
    for (int i = infoFileCount - 1; i >= 0; i--) {
        char* outputPath = (char*)malloc(strlen(arrayOfInfoFiles[i].filename) + strlen(outputDirectory) + strlen(archiveName) + 1);
        sprintf(outputPath, "%s/%s", outputDirectory, arrayOfInfoFiles[i].filename);

        FILE* destFile = fopen(outputPath, "wb");
        int increment = currentOffset - arrayOfInfoFiles[i].size;

        for (int j = increment; j < currentOffset; j++) {
            fseek(archive, j, SEEK_SET);
            fputc(fgetc(archive), destFile);
        }
        currentOffset = increment;
        fclose(destFile);
        free(outputPath);
    }

    for(int i = infoFileCount - 1; i >=0; --i){
        printf("%s, ", arrayOfInfoFiles[i].filename);
    }

    free(arrayOfInfoFiles);
    free(buffer);
    fclose(archive);
}

void CreateDirectory(const char *directoryName){
    int result = mkdir(directoryName, S_IRWXU | S_IRWXG | S_IRWXO);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanım: %s dosya1 [dosya2 ...]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        char* outputName = argv[argc - 1];
        MergeFiles(argc, argv, outputName);
    }
    else if (strcmp(argv[1], "-a") == 0) {
        char* outputName = argv[argc - 2];
        char* outputDirectory = argv[argc-1];
        ExtractArchive(outputName, outputDirectory);
    }

    return 0;
}

