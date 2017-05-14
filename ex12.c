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

    //Student's feedback.
    char feedback[MAX_SIZE];

    //Student's grade.
    int grade;
} Result;

typedef struct {

    //Student's name.
    char *name;

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

    //Boolean did the student receive a timeout.
    int isTimeOut;

    //Student's status.
    Status status;

    //Student's result.
    Result result;

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
 * Checks that the given path leads to a C file.
 * @param path path.
 * @return 1 true, 0 false.
 */
int isCFile(char *path);

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

/**
 * Handles the case in which a C file was not found.
 * @param student student.
 */
void handleNoCFile(Student *student);

/**
 * Writes the student's result into the results file.
 * @param student student.
 */
void writeStudentResult(Student *student);

/**
 * Handles process timeout.
 * @param pid process id.
 * @param status status.
 * @return 0 timeout, 1 no timeout.
 */
int timeoutHandler(pid_t pid, int *status);

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
    int           closeValue;
    DIR           *mainDir;
    struct dirent *studentDirent;

    mainPath = argv[1];

    configFile = open(mainPath, O_RDONLY);

    //Check if the file was opened.
    if (configFile < 0) {

        perror("Error: failed to open file.\n");
        exit(1);
    }

    //Read the file paths.
    readFromFile(configFile, dirPath);
    readFromFile(configFile, inputPath);
    readFromFile(configFile, outputPath);

    closeValue = close(configFile);

    //Check if the file was closed.
    if (closeValue < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    mainDir = opendir(dirPath);

    //Check if the directory eas opened.
    if (mainDir == 0) {

        perror("Error: failed to open directory.\n");
        exit(1);
    }

    //Creates the results file.
    results = open("results.csv", O_CREAT, 0777);

    //Check if results file was opened.
    if (results < 0) {

        perror("Error: failed to open file.\n");
        exit(1);
    }

    //Close the results file.
    closeValue = close(results);

    //Check if closed file.
    if (closeValue < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    //Run over all the students.
    while ((studentDirent = readdir(mainDir)) != 0) {

        //Check if read from directory.
        if (studentDirent == 0) {

            perror("Error: failed to read from directory.\n");
            exit(1);
        }

        //Ignore inner directories.
        if (strcmp(studentDirent->d_name, ".") == 0 ||
            strcmp(studentDirent->d_name, "..") == 0) {
            continue;
        }

        //Create student handler.
        //TODO free student and all its content
        Student *student;

        //TODO move to initStudent
        student = (Student *) malloc(sizeof(Student));
        student->dirent   = studentDirent;
        student->name     = studentDirent->d_name;
        student->homePath = dirPath;
        student->depth    = -1;
        strcpy(student->result.feedback, "\0");
        student->isMultipleDirectories = 0;
        student->isTimeOut             = 0;

        //Set student's name in results file.
        //writeToFile(results, student.dirent->d_name);

        //Search for the student's C file.
        //TODO maybe change this and next line to if succeeded and that's it
        studentPath = findCFile(dirPath, student);

        student->cFilePath = studentPath;

        //Check if C file was found.
        if (student->cFilePath == 0) {

            handleNoCFile(student);

        } else {

            printf("Found: %s depth: %d\n", student->cFilePath, student->depth);

            int compileResult;
            int executeResult;
            int compareResult;
            int chDirResult;
            int unlinkResult;

            //Set student's grade tp 100 - 10 * depth.
            //TODO make sure that the initial grade can only be 100 and not lower because of a previous mistake.
            //TODO make sure acceptable depth is 0 and not 1 in findCFile
            student->result.grade = 100 - (10 * student->depth);

            if (student->depth > 0) {
                strcat(student->result.feedback, ",WRONG_DIRECTORY");
            }

            //Compiles the C file.
            compileResult = compileStudentFile(student);

            //Check if compilation failed.
            if (compileResult == 0) {

                //Set student's grade tp 0.
                student->result.grade = 0;

                //writeToFile(results, ",COMPILATION_ERROR");
                strcat(student->result.feedback, ",COMPILATION_ERROR");
                writeStudentResult(student);
                //TODO make sure the there are no additional files created that weren't deleted
                continue;
            }

            //Executes the C file.
            executeResult = executeStudentFile(student, inputPath);

            //Unlinks exe file.
            //TODO what happens if I unlink it and the other process which caused timeout still runs
            unlinkResult = unlink("student.out");

            //Check if unlinked file.
            if (unlinkResult < 0) {

                perror("Error: failed to unlink file.\n");
                exit(1);
            }

            //Check if execution worked.
            if (executeResult == 0) {

                //TODO check what to do in that case, maybe just perror and exit
            }

            //Check if there was a timeout.
            if (student->isTimeOut) {

                //Set student's grade tp 0.
                student->result.grade = 0;
                strcat(student->result.feedback, ",TIMEOUT");
                writeStudentResult(student);

                //Unlink student's output file.
                unlinkResult = unlink("studentOutput.txt");

                //Check if unlinked file.
                if (unlinkResult < 0) {

                    perror("Error: failed to unlink file.\n");
                    exit(1);
                }
                //TODO make sure the there are no additional files created that weren't deleted
                continue;
            }

            //Compare the student's result to the correct answer.
            compareResult = compareStudentFile(student, outputPath,
                                               "studentOutput.txt");

            //Handle comparison result.
            switch (compareResult) {

                case 1:
                    strcat(student->result.feedback, ",GREAT_JOB");
                    break;

                case 2:
                    //Set student's grade grade - 30.
                    student->result.grade -= 30;
                    strcat(student->result.feedback, ",SIMILAR_OUTPUT");
                    break;

                case 3:
                    //Set student's grade tp 0.
                    student->result.grade = 0;
                    strcat(student->result.feedback, ",BAD_OUTPUT");
                    break;

                default:
                    break;
            }

            //Unlink student's output file.
            unlinkResult = unlink("studentOutput.txt");

            //Check if unlinked file.
            if (unlinkResult < 0) {

                perror("Error: failed to unlink file.\n");
                exit(1);
            }

//            //Return back to main directory.
//            chDirResult = chdir(mainPath);
//
//            if (chDirResult == -1) {
//
//                perror("Error: change directory failed.\n");
//                exit(1);
//            }
        }

        //Finish writing the student's result.
        //writeToFile(results, "\n");

        //Write student's result.
        writeStudentResult(student);

        //TODO free what's inside student, like cFilePath
       // free(student);
    }

//TODO check if all opened files were closed.
    /*closeValue = close(results);

    if (closeValue < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }*/
}

char *findCFile(char *initPath, Student *student) {

    int  stop       = 0;
    int  dirCounter = 0;
    int  isCFound   = 0;
    char finalPath[MAX_SIZE];
    char nextFile[MAX_SIZE];
    DIR  *dir;

    strcpy(finalPath, initPath);
    strcat(finalPath, "/");
    strcat(finalPath, student->dirent->d_name);

    while (!stop) {

        //TODO now that alloc is not needed, remove tempPath
        //Concatenate next path.
        //strcat(finalPath, "/");
        //strcat(finalPath, nextFile);

        //Check if found the C file.
        if (isCFound) {

            //Return final path.
            //TODO free
            char *retPath = (char *) malloc(
                    strlen(finalPath) * sizeof(char));

            strcpy(retPath, finalPath);

            return retPath;
        }

        dir = opendir(finalPath);

        //Check if directory was opened.
        if (dir == 0) {

            perror("Error: failed to open directory.\n");
            exit(1);
        }

        student->depth += 1;

        dirCounter = 0;

        //Checks the amount of folders the student has is legal.
        //TODO make sure folder is not empty also
        while ((student->dirent = readdir(dir)) != 0) {

            //Check if read from directory.
            if (student->dirent == 0) {

                perror("Error: failed to read directory.\n");
                exit(1);
            }

            if (strcmp(student->dirent->d_name, ".") == 0 ||
                strcmp(student->dirent->d_name, "..") == 0) {
                continue;
            }

            char temp[MAX_SIZE];
            strcpy(temp, finalPath);
            strcat(temp, "/");
            strcat(temp, student->dirent->d_name);

            if (isDirectory(temp)) {

                strcpy(nextFile, temp);
                dirCounter++;
            }

            else if (isCFile(temp)) {

                strcpy(nextFile, temp);
                isCFound = 1;
                break;
            }
        }

        //Stop searching if more that on inner folder exists.
        //TODO check in Piazza, if c file was found but isMultipleDirectories than write that in results, then move isCFound inside if()
        //TODO also make sure to change that in main so that it would continue to execute the file
        if ((dirCounter > 1) && !isCFound) {

            int closeValue;

            student->isMultipleDirectories = 1;
            closeValue = closedir(dir);

            //Check if the directory was closed.
            if (closeValue < 0) {

                perror("Error: failed to close directory");
                exit(1);
            }

            return 0;
        }

        //Check if there were sub-directories.
        if (dirCounter == 0 && !isCFound) {

            closedir(dir);
            return 0;
        }

        //TODO does using strcpy add \0 at the end? if not then add with strcat
        //Copy next name.
         strcpy(finalPath, nextFile);
    }

    //Check if the path contained another file.


    //TODO if it doesn't cause any problems
    //closedir(dir);
}

void handleNoCFile(Student *student) {

    //Set student's grade tp 0.
    student->result.grade = 0;

    //Set the reason for the grade.
    if (student->isMultipleDirectories) {

        //TODO grade missing
        //TODO is \n necessary when writing to .csv
        strcat(student->result.feedback, ",MULTIPLE_DIRECTORIES");
        // writeToFile(results, ",MULTIPLE_DIRECTORIES");

        printf("Multiple directories.\n");

    } else {

        strcat(student->result.feedback, ",NO_C_FILE");
        //writeToFile(results, ",NO_C_FILE");
    }

    printf("No c: %s\n", student->name);

}

int isDirectory(char *path) {

    struct stat pathStat;
    stat(path, &pathStat);

    return S_ISDIR(pathStat.st_mode);

}

int isCFile(char *path) {

    int length;

    length = strlen(path);

    //TODO make an actual check that a path is a file, and then check for .c, also make sure length > 3 is correct
    if (length > 3 && path[length - 1] == 'c' && path[length - 2] == '.') {

        return 1;
    }

    return 0;
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

        /*isDirChanged = chdir(studentMainDirPath);
        //Check if changing directory succeeded.
        if (isDirChanged < 0) {

            perror("Error: failed to change directory.\n");
            exit(1);
        }*/

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
        int  execValue;
        int  closeValue;

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
        //TODO make if check it worked
        dup2(inputFile, 0);
        dup2(studentOutputFile, 1);

        closeValue = close(studentOutputFile);

        //Check if output file was closed.
        if (closeValue < 0) {

            perror("Error: failed to close file.\n");
        }

        closeValue = close(inputFile);

        //Check if input file was closed.
        if (closeValue < 0) {

            perror("Error: failed to close file.\n");
        }

        //Execute file.
        execValue = execvp("./student.out", argsStudent);

        //Check if execvp failed.
        if (execValue == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }
    } else {

        int exitStatus;
        int timerStatus;
        //Check for timeout.
        student->isTimeOut = timeoutHandler(execPId, &timerStatus);

        if (student->isTimeOut == 1) {

            return waitForChildExec(&exitStatus);

        } else {

            return 1;
        }
    }
}

int compareStudentFile(Student *student, char *correctOutput,
                       char *studentOutput) {

    pid_t compPId;
    int   waitResult;

    compPId = fork();

    if (compPId == 0) {

        char *argsComp[] = {"./comp.out", correctOutput, studentOutput, 0};
        int  compExec;

        compExec = execvp("./comp.out", argsComp);

        if (compExec == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }

    } else {

        //TODO handle the case in which comp.out failes at runtime.
        waitResult = waitForChildExec(&student->status.compareStatus);

        return WEXITSTATUS(student->status.compareStatus);
    }

}

int waitForChildExec(int *status) {

    int waitVal;

    waitVal = wait(status);

    if (waitVal == -1) {

        perror("Error: wait failed.\n");
        exit(1);
    }

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

void writeStudentResult(Student *student) {

    int  results;
    int  closeValue;
    char resultToWrite[MAX_SIZE];

    //Open results file.
    results = open("results.csv", O_APPEND | O_WRONLY, 777);

    //Check if results file was opened.
    if (results < 0) {

        perror("Error: failed to open file.\n");
        exit(1);
    }

    //Check that the grade is not less a negative number.
    if (student->result.grade < 0) {

        student->result.grade = 0;
    }

    //Create student result.
    sprintf(resultToWrite, "%s,%d%s\n", student->name, student->result.grade,
            student->result.feedback);

    //Write result to file.
    writeToFile(results, resultToWrite);

    closeValue = close(results);

    //Check if file was closed.
    if (closeValue < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }
}

int timeoutHandler(pid_t pid, int *status) {

    int counter = 5;
    int waitResult;

    //Run until time runs out.
    while (counter != 0) {

        waitResult = waitpid(pid, status, WNOHANG);

        //Check if waitpid worked.
        if (waitResult < 0) {

            perror("Error: waitpid failed.\n");
            exit(1);
        }

        //Check if process status was changed.
        if (waitResult == 0) {

            //Wait for a second.
            sleep(1);
            counter--;

        } else {

            return 0;
        }
    }

    int killResult;

    //Stop the process due to timeout.
    killResult = kill(pid, SIGKILL);

    //Check if kill worked.
    if (killResult < 0) {

        perror("Error: kill failed.\n");
        exit(1);
    }

    return 1;
}

//TODO delete if not needed
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
