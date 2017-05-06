#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <sys/stat.h>

#define MAX_SIZE 160

/**
 * Reads a line from a file into the given buffer.
 * @param fileDesc a file descriptor.
 * @param buffer array.
 */
void readFromFile(int fileDesc, char buffer[]);

/**
 * Searches for a C file in the given directory.
 * @param initPath initial path to search.
 * @param studentDirent student dirent to the path.
 * @param isMultDirectories checks if there are multiple directories.
 * @return path if found, else NULL.
 */
char *
findCFile(char *initPath, struct dirent *studentDirent, int *isMultDirectories,
          int *depth);

/**
 * Checks if the given path belongs to a directory.
 * @param path file path.
 * @return boolean.
 */
int isDirectory(char *path);

int main(int argc, char *argv[]) {

    //Check that the number of command line arguments is correct.
    if (argc != 2) {

        perror("Error: wrong number of parameters.\n");
        exit(1);
    }

    char          *mainPath;
    char          *studentPath = 0;
    char          dirPath[MAX_SIZE];
    char          inputPath[MAX_SIZE];
    char          outputPath[MAX_SIZE];
    int           configFile;
    DIR           *mainDir;
    struct dirent *studentDirent;

    mainPath = argv[1];

    configFile = open(mainPath, O_RDONLY);

    //Check if the file was opened.
    if (configFile < 0) {

        perror("Error occurred while opening file.\n");
        exit(1);
    }

    //Read the file paths.
    readFromFile(configFile, dirPath);
    //readFromFile(configFile, inputPath);
    //readFromFile(configFile, outputPath);

    close(configFile);

    mainDir = opendir(dirPath);

    //Check if the directory eas opened.
    if (mainDir == 0) {

        perror("Error occurred while opening directory.\n");
        exit(1);
    }

    while ((studentDirent = readdir(mainDir)) != 0) {

        if (strcmp(studentDirent->d_name, ".") == 0 ||
            strcmp(studentDirent->d_name, "..") == 0) {
            continue;
        }

        int isMultDirectories = 0;
        int depth             = 0;
        studentPath = findCFile(dirPath, studentDirent, &isMultDirectories,
                                &depth);

        if (studentPath == 0) {

            printf("No c: %s\n", studentDirent->d_name);

            if (isMultDirectories) {

                printf("Multiple directories.\n");
            }
        } else {
            printf("Found: %s depth: %d\n", studentPath, depth);
        }

    }
}

char *findCFile(char *initPath, struct dirent *studentDirent,
                int *isMultDirectories, int *depth) {

    int  stop       = 0;
    int  dirCounter = 0;
    char finalPath[MAX_SIZE];
    char nextFile[MAX_SIZE];
    DIR  *dir;

    strcpy(finalPath, initPath);
    strcpy(nextFile, studentDirent->d_name);

    while (!stop) {

        //TODO now that alloc is not needed, remove tempPath
        //Concatenate next path.
        strcat(finalPath, "/");
        strcat(finalPath, nextFile);

        //Check if found the C file.
        if (!isDirectory(finalPath)) {

            //Return final path.
            char *retPath = (char *) malloc(strlen(finalPath) * sizeof(char));
            strcpy(retPath, finalPath);

            return retPath;
        }

        dir = opendir(finalPath);
        *depth += 1;

        dirCounter = 0;

        //Checks the amount of folders the student has is legal.
        while ((studentDirent = readdir(dir)) != 0) {

            if (strcmp(studentDirent->d_name, ".") == 0 ||
                strcmp(studentDirent->d_name, "..") == 0) {
                continue;
            }

            dirCounter++;

            //Stop searching if more that on inner folder exists.
            if (dirCounter > 1) {

                *isMultDirectories = 1;
                closedir(dir);
                return 0;
            }

            //TODO does using strcpy add \0 at the end? if not then add with strcat
            //Copy next name.
            strcpy(nextFile, studentDirent->d_name);
        }

        //Check if the path contained another file.
        if (dirCounter == 0) {

            closedir(dir);
            return 0;
        }

        //TODO if it doesn't cause any problems
        //closedir(dir);
    }
}

int isDirectory(char *path) {

    struct stat pathStat;
    stat(path, &pathStat);

    return S_ISDIR(pathStat.st_mode);

}

void readFromFile(int fileDesc, char buffer[]) {

    int counter = 0;
    int stop    = 0;
    int readNum = 0;

    //Loop until end of file.
    while (!stop) {

        readNum = read(fileDesc, &buffer[counter], 1);

        //Check if read succeeded.
        if (readNum < 0) {

            perror("Error occurred while reading from file.\n");
            exit(1);
        }

        //Check if there if a need to stop.
        if ((readNum == 0) || (buffer[counter] == '\n')) {

            //TODO maybe there is a need to move the read by one to get the next line
            buffer[counter] = '\0';
            stop = 1;
        }

        counter++;
    }


}