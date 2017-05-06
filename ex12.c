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
 * @return path if found, else NULL.
 */
char *findCFile(char *initPath, struct dirent *studentDirent);

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
    int           depth;
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

        studentPath = findCFile(dirPath, studentDirent);

        if (studentPath == 0) {

            //TODO fail student
        } else {

            //TODO exec
        }

    }
}

char *findCFile(char *initPath, struct dirent *studentDirent) {

    int  stop       = 0;
    int  dirCounter = 0;
    int  pathLength;
    int  studentLength;
    char *finalPath = 0;
    char *tempPath  = 0;
    char *nextFile  = 0;
    DIR  *dir;

    finalPath = (char *) malloc(strlen(initPath) * sizeof(char));
    strcpy(finalPath, initPath);
    nextFile = studentDirent->d_name;

    while (!stop) {

//        if (strcmp(studentDirent->d_name, ".") == 0 ||
        //           strcmp(studentDirent->d_name, "..") == 0) {
        //          continue;
        //    }

        pathLength    = strlen(finalPath);
        studentLength = strlen(nextFile);

        if (tempPath != 0) {

            free(tempPath);
        }

        //Concatenate next path.
        tempPath = (char *) malloc(pathLength * sizeof(char));
        strcpy(tempPath, finalPath);

        if (finalPath != 0) {

            free(finalPath);
        }

        finalPath = (char *) malloc(
                (pathLength + studentLength) * sizeof(char));

        strcpy(finalPath, tempPath);
        strcat(finalPath, "/");
        strcat(finalPath, nextFile);

        //Check if found the C file.
        if (!isDirectory(finalPath)) {

            return finalPath;
        }

        dir = opendir(finalPath);

        dirCounter = 0;

        //Checks the amount of folders the student has is legal.
        while ((studentDirent = readdir(dir)) != 0) {

            if (strcmp(studentDirent->d_name, ".") == 0 ||
                strcmp(studentDirent->d_name, "..") == 0) {
                continue;
            }

            //Copy next name.
            int tempLength = strlen(studentDirent->d_name);
            //TODO free nextFile
            nextFile = (char*)malloc(tempLength * sizeof(char));
            strcpy(nextFile, studentDirent->d_name);

            dirCounter++;

            if (dirCounter > 1) {

                //TODO free all the allocated vars, make a separate function if needed
                closedir(dir);
                return 0;
            }
        }

        nextFile = (char*)malloc(strlen(studentDirent->d_name) * sizeof(char));
        strcpy(nextFile, studentDirent->d_name);

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