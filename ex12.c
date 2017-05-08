#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_SIZE 160

/**
 * Writes a message to the given file.
 * @param file file descriptor.
 * @param message message to write.
 */
void writeToFile(int file, char *message);

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
    char          studentDir[MAX_SIZE];
    int           configFile;
    int           results;
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
    readFromFile(configFile, inputPath);
    readFromFile(configFile, outputPath);

    close(configFile);

    mainDir = opendir(dirPath);

    //Check if the directory eas opened.
    if (mainDir == 0) {

        perror("Error occurred while opening directory.\n");
        exit(1);
    }

    results = open("results.csv", O_CREAT | O_WRONLY,
                   0777);

    //Run over all the students.
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

            if (isMultDirectories) {

                //TODO grade missing
                //TODO is \n necessary when writing to .csv
                char message[MAX_SIZE];
                snprintf(message, MAX_SIZE, "%s,MULTIPLE_DIRECTORIES\n",
                         studentDirent->d_name);

                writeToFile(results, message);

                printf("Multiple directories.\n");
            } else {

                char message[MAX_SIZE];
                snprintf(message, MAX_SIZE, "%s,NO_C_FILE\n",
                         studentDirent->d_name);

                writeToFile(results, message);
            }

            printf("No c: %s\n", studentDirent->d_name);


        } else {
            printf("Found: %s depth: %d\n", studentPath, depth);

            pid_t compilePId;
            pid_t execPId;
            int   compileStatus;
            int   execStatus;
            int   isDirChanged;

            compilePId = fork();

            if (compilePId < 0) {

                perror("Error: fork failed.\n");
                exit(1);
            }

            if (compilePId == 0) {

                //Move to student directory.
                strcpy(studentDir, dirPath);
                strcat(studentDir, "/");
                strcat(studentDir, studentDirent->d_name);

                //TODO go one dir up at the end.
                isDirChanged = chdir(studentDir);

                //Check if changing directory succeeded.
                if (isDirChanged < 0) {

                    perror("Error: failed to change directory.\n");
                    exit(1);
                }
                //TODO change student.out to a.out
                char *args[] = {"gcc", studentPath, "-o", "student.out", 0};
                int  retExec;

                printf("Enter child process.\n");
                retExec = execvp("gcc", args);

                if(retExec == -1){

                    perror("Error: execution failed.\n");
                    exit(1);
                }

            } else {

                //TODO should compile all files and then execute and use fork to save time?

                wait(&compileStatus);

                if (WIFEXITED(compileStatus)) {

                    printf("finished compileStatus:%d.\n",
                           WEXITSTATUS(compileStatus));

                    //Check if compilation succeeded.
                    if (WEXITSTATUS(compileStatus) == 1) {

                        char message[MAX_SIZE];
                        snprintf(message, MAX_SIZE, "%s,COMPILATION_ERROR\n",
                                 studentDirent->d_name);

                        writeToFile(results, message);

                    } else {

                        execPId = fork();

                        if (execPId < 0) {

                            perror("Error: fork failed.\n");
                            exit(1);
                        }

                        if (execPId == 0) {

                            //TODO change student.out to a.out
                            char *argsStudent[] = {"./student.out", inputPath, 0};
                            int studentOutputFile;
                            int inputFile;
                            int execVal;

                            studentOutputFile = open("studentOutput.txt", O_CREAT | O_WRONLY, 777);

                            if(studentOutputFile < 0){

                                perror("Error: failed to open file.\n");
                                exit(1);
                            }

                            inputFile = open(inputPath, O_RDONLY);

                            if(inputFile < 0){

                                perror("Error: failed to open file.\n");
                                exit(1);
                            }

                            //Redirect input and output to files.
                            dup2(inputFile, 0);
                            dup2(studentOutputFile, 1);

                            close(studentOutputFile);
                            close(inputFile);

                            execVal = execvp("./student.out", argsStudent);

                            if(execVal == -1){

                                perror("Error: execution failed.\n");
                                exit(1);
                            }
                        }
                        else{

                            wait(&execStatus);

                            if(WIFEXITED(execStatus)){

                                printf("finished execStatus:%d.\n",
                                       WEXITSTATUS(execStatus));

                                if(WEXITSTATUS(execStatus) == 1){

                                    //TODO should there be a handle to a runtime error?
                                }
                                else{

                                    pid_t compPId;
                                    int compStatus;

                                    compPId = fork();

                                    if(compPId == 0){

                                    }
                                    else{

                                        wait(&compStatus);

                                        if(WIFEXITED(compStatus)){

                                            printf("finished execStatus:%d.\n",
                                                   WEXITSTATUS(execStatus));

                                            if(WEXITSTATUS(compStatus) == 1){

                                                perror("Error: comparison failed.\n");
                                                exit(1);
                                            }
                                    }

                                    char *argsComp[] = {"./comp.out", "studentOutput.txt", outputPath, 0};
                                    int compExec;

                                    compExec = execvp("./comp.out", argsComp);

                                    if(compExec == -1){

                                        perror("Error: execution failed.\n");
                                        exit(1);
                                    }
                                }
                            }
                            else{

                                //TODO should you write error and even exit in that case
                                perror("Execution failed.\n");
                                exit(1);
                            }
                        }


                    }

                } else {
                    //TODO should you write error and even exit in that case
                    perror("Execution failed.\n");
                    exit(1);
                }
            }
        }

    }

    //TODO check if all opened files were closed.
    close(results);
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

void writeToFile(int file, char *message) {

    int bytesWrote;

    bytesWrote = write(file, message, strlen(message));

    //Check that the message was written.
    if (bytesWrote < 0) {

        perror("Error: failed to write to file.\n");
        exit(1);
    }
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