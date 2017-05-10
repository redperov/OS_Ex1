#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_SIZE 160

typedef struct {

    //Compilation status.
    int compileStatus;

    //Execution status.
    int executeStatus;

    //Comparison status.
    int compareStatus;
} Status;

typedef struct {

    //Student's directory path.
    char *dirPath;

    //Student's C file path.
    char *cFilePath;

    //Student's executable file path.
    char *execFilePath;

    //TODO delete if not needed
    //Student's path to the main folder.
    char *mainDirPath;

    //The path to the main directory of students.
    char *homePath;

    //Student's dirent.
    struct dirent *dirent;

    //Depth to c file.
    int depth;

    //Boolean does the student have multiple directories.
    int isMultipleDirectories;

    Status status;

} Student;

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
 * @param student student struct.
 * @return path if found, else NULL.
 */
char *
findCFile(char *initPath, Student *student);

/**
 * Checks if the given path belongs to a directory.
 * @param path file path.
 * @return boolean.
 */
int isDirectory(char *path);

/**
 * Adds error message to a student in the results file.
 * @param fileDesc file descriptor.
 * @param errorMessage error message.
 */
void addErrorToStudent(int fileDesc, char *errorMessage);

/**
 * Initializes all the student's members.
 * @return Student.
 */
Student initStudent();

/**
 * Waits for the child to finish execution.
 * @param status.
 * @return -1 exec failed, 0 program failed, 1 succeeded.
 */
int waitForChildExec(int *status);

/**
 * Compiles the student's C file.
 * @param student *Student
 * @return 0 if failed, 1 if succeeded.
 */
int compileStudentFile(Student *student);

/**
 * Executes the student's C file.
 * @param student student.
 * @param inputFilePath input file path.
 * @return 0 if failed, 1 if succeeded.
 */
int executeStudentFile(Student *student, char *inputFilePath);

/**
 * Compares between the correct and student's outputs.
 * @param student student
 * @param correctOutput correct output path.
 * @param studentOutput student's output path.
 * @return -1 different, 0 similar, 1 same.
 */
int compareStudentFile(Student *student, char *correctOutput,
                       char *studentOutput);

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

    //Creates the results file.
    results = open("results.csv", O_CREAT | O_WRONLY,
                   0777);

    //Run over all the students.
    while ((studentDirent = readdir(mainDir)) != 0) {

        if (strcmp(studentDirent->d_name, ".") == 0 ||
            strcmp(studentDirent->d_name, "..") == 0) {
            continue;
        }

        Student student;

        student.dirent   = studentDirent;
        student.homePath = dirPath;

        writeToFile(results, student.dirent->d_name);

        //Search for the student's C file.
        studentPath = findCFile(dirPath, &student);

        student.cFilePath = studentPath;

        //Check if C file was found.
        if (student.cFilePath == 0) {

            //Check if the reason is multiple directories.
            if (student.isMultipleDirectories) {

                //TODO grade missing
                //TODO is \n necessary when writing to .csv
                writeToFile(results, ",MULTIPLE_DIRECTORIES");

                printf("Multiple directories.\n");

            } else {

                writeToFile(results, ",NO_C_FILE");
            }

            printf("No c: %s\n", student.dirent->d_name);


        } else {

            printf("Found: %s depth: %d\n", student.cFilePath, student.depth);

            int compileResult;
            int executeResult;
            int compareResult;
            int chDirResult;

            //compiles the C file.
            compileResult = compileStudentFile(&student);

            if (compileResult == 0) {

                writeToFile(results, ",COMPILATION_ERROR");
                //TODO make sure the main moves to the next student.
                continue;
            }

            executeResult = executeStudentFile(&student, inputPath);

            if (executeResult == 0) {

                //TODO check what to do in that case, maybe just perror and exit
            }

            compareResult = compareStudentFile(&student, outputPath,
                                               "studentOutput.txt");

            switch (compareResult) {
                case -1:
                    writeToFile(results, "BAD_OUTPUT");
                    break;
                case 0:
                    writeToFile(results, "SIMILAR_OUTPUT");
                    break;
                case 1:
                    writeToFile(results, "WRONG_DIRECTORY");
                    break;

                default:
                    break;
            }

            //Return back to main directory.
            chDirResult = chdir(mainPath);

            if (chDirResult == -1) {

                perror("Error: chdir failed");
                exit(1);
            }
        }

        writeToFile(results, "\n");
    }

//TODO check if all opened files were closed.
    close(results);
}

char *findCFile(char *initPath, Student *student) {

    int  stop       = 0;
    int  dirCounter = 0;
    char finalPath[MAX_SIZE];
    char nextFile[MAX_SIZE];
    DIR  *dir;

    strcpy(finalPath, initPath);
    strcpy(nextFile, student->dirent->d_name);

    while (!stop) {

        //TODO now that alloc is not needed, remove tempPath
        //Concatenate next path.
        strcat(finalPath, "/");
        strcat(finalPath, nextFile);

        //Check if found the C file.
        if (!isDirectory(finalPath)) {

            //Return final path.
            //TODO free
            char *retPath = (char *) malloc(
                    strlen(finalPath) * sizeof(char));

            strcpy(retPath, finalPath);

            return retPath;
        }

        dir = opendir(finalPath);
        student->depth += 1;

        dirCounter = 0;

        //Checks the amount of folders the student has is legal.
        while ((student->dirent = readdir(dir)) != 0) {

            if (strcmp(student->dirent->d_name, ".") == 0 ||
                strcmp(student->dirent->d_name, "..") == 0) {
                continue;
            }

            dirCounter++;

            //Stop searching if more that on inner folder exists.
            if (dirCounter > 1) {

                student->isMultipleDirectories = 1;
                closedir(dir);
                return 0;
            }

            //TODO does using strcpy add \0 at the end? if not then add with strcat
            //Copy next name.
            strcpy(nextFile, student->dirent->d_name);
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

int compileStudentFile(Student *student) {

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
//        strcpy(student->mainDirPath,);
//        strcat(studentDir, "/");
//        strcat(studentDir, studentDirent->d_name);

        char studentMainDirPath[MAX_SIZE];
        snprintf(studentMainDirPath, MAX_SIZE, "%s/%s",
                 student->homePath, student->dirent->d_name);

        //TODO go one dir up at the end.
        isDirChanged = chdir(studentMainDirPath);

        //Check if changing directory succeeded.
        if (isDirChanged < 0) {

            perror("Error: failed to change directory.\n");
            exit(1);
        }
        //TODO change student.out to a.out
        char *args[] = {"gcc", student->cFilePath, "-o", "student.out", 0};
        int  retExec;

        printf("Enter child process.\n");
        retExec = execvp("gcc", args);

        if (retExec == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }
    } else {

        //TODO should compile all files and then execute and use fork to save time?

        return waitForChildExec(&student->status.compileStatus);
    }
}

int executeStudentFile(Student *student, char *inputFilePath) {
    pid_t execPId;
    execPId = fork();

    //Check if fork succeeded.
    if (execPId < 0) {

        perror("Error: fork failed.\n");
        exit(1);
    }

    if (execPId == 0) {

        //TODO change student.out to a.out
        char *argsStudent[] = {"./student.out", inputFilePath,
                               0};
        int  studentOutputFile;
        int  inputFile;
        int  execVal;

        //TODO this file already gets opened in ex11, make sure it doesn't cause problems.
        //TODO delete all student files after they are no longer needed
        studentOutputFile = open("studentOutput.txt",
                                 O_CREAT | O_WRONLY, 777);

        //Check if studentOutputFile was opened.
        if (studentOutputFile < 0) {

            perror("Error: failed to open file.\n");
            exit(1);
        }

        inputFile = open(inputFilePath, O_RDONLY);

        //Check if inputFile was opened.
        if (inputFile < 0) {

            perror("Error: failed to open file.\n");
            exit(1);
        }

        //Redirect input and output to files.
        dup2(inputFile, 0);
        dup2(studentOutputFile, 1);

        close(studentOutputFile);
        close(inputFile);

        execVal = execvp("./student.out", argsStudent);

        //Check if execvp failed.
        if (execVal == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }
    } else {

        //Wait for execution to end.
        return waitForChildExec(&student->status.executeStatus);
    }
}

int compareStudentFile(Student *student, char *correctOutput,
                       char *studentOutput) {

    pid_t compPId;

    compPId = fork();

    if (compPId == 0) {

        char *argsComp[] = {"./comp.out", correctOutput, studentOutput,
                            0};
        int  compExec;

        compExec = execvp("./comp.out",
                          argsComp);

        if (compExec == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }

    } else {

        return waitForChildExec(&student->status.compareStatus);
    }

}

Student initStudent() {

    Student student;

    memset(student.dirPath, 0, MAX_SIZE);
    memset(student.cFilePath, 0, MAX_SIZE);
    memset(student.execFilePath, 0, MAX_SIZE);
    memset(student.mainDirPath, 0, MAX_SIZE);

    student.dirent                = 0;
    student.depth                 = -1;
    student.isMultipleDirectories = 0;

    student.status.compileStatus = 0;
    student.status.compareStatus = 0;
    student.status.executeStatus = 0;

    return student;
}

int waitForChildExec(int *status) {

    wait(status);

    if (WIFEXITED(*status)) {

        printf("finished exec:%d.\n",
               WEXITSTATUS(*status));

        //Check if execution succeeded.
        if (WEXITSTATUS(*status) == 1) {

            return 0;
        }

        return 1;
    }

    //TODO delete if not necessary
//        return -1;
}
