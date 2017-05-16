/******************************************
* Student name: Danny Perov
* Student ID: 318810637
* Course Exercise Group: 05
* Exercise name: Exercise 1
******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_SIZE 160

//Holds the the student's status
typedef struct {

    //Compilation status.
    int compileStatus;

    //Execution status.
    int executeStatus;

    //Comparison status.
    int compareStatus;
} Status;

//Holds the student result.
typedef struct {

    //Student's feedback.
    char feedback[MAX_SIZE];

    //Student's grade.
    int grade;
} Result;

//Holds the students info.
typedef struct {

    //Student's name.
    char *name;

    //Student's directory path.
    char *dirPath;

    //Student's C file path.
    char *cFilePath;

    //Student's executable file path.
    char *execFilePath;

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
 * function name: WriteToFile.
 * The input: file descriptor, message to write.
 * The output: void.
 * The function operation: Writes a message to the given file.
*/
void WriteToFile(int file, char *message);

/**
 * function name: ReadFromFile.
 * The input: file descriptor, array.
 * The output: void.
 * The function operation: Reads a line from a file into the given buffer.
*/
void ReadFromFile(int fileDesc, char *buffer);

/**
 * function name: FindCFile.
 * The input: initial path to search, student struct.
 * The output: path if found, else NULL.
 * The function operation: Searches for a C file in the given directory.
*/
char *
FindCFile(char *initPath, Student *student);

/**
 * function name: IsDirectory.
 * The input: file path.
 * The output: boolean
 * The function operation: Checks if the given path belongs to a directory.
*/
int IsDirectory(char *path);

/**
 * function name: IsCFile.
 * The input: path
 * The output:  1 true, 0 false.
 * The function operation: Checks that the given path leads to a C file.
*/
int IsCFile(char *path);

/**
 * function name: InitStudent.
 * The input: dirent, path.
 * The output: Student.
 * The function operation: Initializes a student.
*/
Student *InitStudent(struct dirent *studentDirent, char *dirPath);

/**
 * function name: WaitForChildExec.
 * The input: status.
 * The output: -1 exec failed, 0 program failed, 1 succeeded.
 * The function operation: Waits for the child to finish execution.
*/
int WaitForChildExec(int *status);

/**
 * function name: CompileStudentFile.
 * The input: *Student
 * The output: 0 if failed, 1 if succeeded.
 * The function operation: Compiles the student's C file.
*/
int CompileStudentFile(Student *student);

/**
 * function name: ExecuteStudentFile.
 * The input: student, input file path.
 * The output:  0 if failed, 1 if succeeded.
 * The function operation: Executes the student's C file.
*/
int ExecuteStudentFile(Student *student, char *inputFilePath);

/**
 * function name: CompareStudentFile.
 * The input: student, correct output path,  student's output path.
 * The output: -1 different, 0 similar, 1 same.
 * The function operation: Compares between the correct and student's outputs.
*/
int CompareStudentFile(Student *student, char *correctOutput,
                       char *studentOutput);

/**
 * function name: HandleNoCFile.
 * The input: student.
 * The output: void.
 * The function operation: Handles the case in which a C file was not found.
*/
void HandleNoCFile(Student *student);

/**
 * function name: WriteStudentResult.
 * The input: student.
 * The output: void.
 * The function operation: Writes the student's result into the results file.
*/
void WriteStudentResult(Student *student);

/**
 * function name: TimeoutHandler.
 * The input: process id, status.
 * The output: 0 timeout, 1 no timeout.
 * The function operation:  Handles process timeout.
*/
int TimeoutHandler(pid_t pid, int *status);

/**
 * function name: FreeStudent.
 * The input: student.
 * The output: void.
 * The function operation: Frees the student and his content.
*/
void FreeStudent(Student *student);

/**
 * function name: HandleCompilationError.
 * The input: student.
 * The output: void.
 * The function operation:  Handles compilation error of student's file.
*/
void HandleCompilationError(Student *student);

/**
 * function name: HandleComparisonResult.
 * The input: student.
 * The output: comparison result.
 * The function operation: Handle student's comparison result.
*/
void HandleComparisonResult(Student *student, int compareResult);

/**
 * function name: HandleTimeout.
 * The input: student.
 * The output: void.
 * The function operation: Handles timeout of student's file.
*/
void HandleTimeout(Student *student);

int main(int argc, char *argv[]) {

    //Check that the number of command line arguments is correct.
    if (argc != 2) {

        perror("Error: wrong number of parameters.\n");
        exit(1);
    }

    //Variable declarations.
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

    //Holds the main path to student's directory.
    mainPath = argv[1];

    //Open configuration file.
    configFile = open(mainPath, O_RDONLY);

    //Check if the file was opened.
    if (configFile < 0) {

        perror("Error: failed to open file.\n");
        exit(1);
    }

    //Read the file paths.
    ReadFromFile(configFile, dirPath);
    ReadFromFile(configFile, inputPath);
    ReadFromFile(configFile, outputPath);

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

        Student *student;

        //Initialize student.
        student = InitStudent(studentDirent, dirPath);

        //Search for the student's C file.
        studentPath = FindCFile(dirPath, student);

        student->cFilePath = studentPath;

        //Check if C file was found.
        if (student->cFilePath == 0) {

            HandleNoCFile(student);

        } else {

            //Variable declarations.
            int compileResult;
            int executeResult;
            int compareResult;
            int chDirResult;
            int unlinkResult;

            //Set student's grade tp 100 - 10 * depth.
            student->result.grade = 100 - (10 * student->depth);

            //Compiles the C file.
            compileResult = CompileStudentFile(student);

            //Check if compilation failed.
            if (compileResult == 0) {

                HandleCompilationError(student);
                continue;
            }

            //Executes the C file.
            executeResult = ExecuteStudentFile(student, inputPath);

            //Unlinks exe file.
            unlinkResult = unlink("student.out");

            //Check if unlinked file.
            if (unlinkResult < 0) {

                perror("Error: failed to unlink file.\n");
                exit(1);
            }

            //Check if there was a timeout.
            if (student->isTimeOut) {

                HandleTimeout(student);
                continue;
            }

            //Compare the student's result to the correct answer.
            compareResult = CompareStudentFile(student, outputPath,
                                               "studentOutput.txt");

            //Handle comparison result.
            HandleComparisonResult(student, compareResult);

            //Unlink student's output file.
            unlinkResult = unlink("studentOutput.txt");

            //Check if unlinked file.
            if (unlinkResult < 0) {

                perror("Error: failed to unlink file.\n");
                exit(1);
            }
        }

        //Write student's result.
        WriteStudentResult(student);

        FreeStudent(student);
    }

    //Close main directory.
    closeValue = closedir(mainDir);

    //Check if directory was closed.
    if (closeValue < 0) {

        perror("Error: failed to close directory.\n");
        exit(1);
    }
}

char *FindCFile(char *initPath, Student *student) {

    //Variable declarations.
    int  stop        = 0;
    int  dirCounter  = 0;
    int  isCFound    = 0;
    int  closeResult = 0;
    char finalPath[MAX_SIZE];
    char nextFile[MAX_SIZE];
    DIR  *dir;

    //Set path name.
    strcpy(finalPath, initPath);
    strcat(finalPath, "/");
    strcat(finalPath, student->dirent->d_name);

    while (!stop) {

        //Check if found the C file.
        if (isCFound) {

            //Return final path.
            char *retPath = (char *) malloc(
                    strlen(finalPath) * sizeof(char) + 1);

            strcpy(retPath, finalPath);
            strcat(retPath, "\0");

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

            if (IsDirectory(temp)) {

                strcpy(nextFile, temp);
                dirCounter++;
            } else if (IsCFile(temp)) {

                strcpy(nextFile, temp);
                isCFound = 1;
                break;
            }
        }

        closeResult = closedir(dir);

        //Check if the directory was closed.
        if (closeResult < 0) {

            perror("Error: failed to close directory");
            exit(1);
        }

        //Stop searching if more that on inner folder exists.
        if ((dirCounter > 1) && !isCFound) {

            student->isMultipleDirectories = 1;

            return 0;
        }

        //Check if there were sub-directories.
        if (dirCounter == 0 && !isCFound) {

            return 0;
        }

        //Copy next name.
        strcpy(finalPath, nextFile);
    }
}

void HandleNoCFile(Student *student) {

    //Set student's grade tp 0.
    student->result.grade = 0;

    //Set the reason for the grade.
    if (student->isMultipleDirectories) {

        strcat(student->result.feedback, ",MULTIPLE_DIRECTORIES");

    } else {

        strcat(student->result.feedback, ",NO_C_FILE");
    }
}

int IsDirectory(char *path) {

    struct stat pathStat;
    stat(path, &pathStat);

    return S_ISDIR(pathStat.st_mode);
}

int IsCFile(char *path) {

    //Variable declarations.
    int length;

    length = strlen(path);

    //Check if is a C file.
    if (length > 2 && path[length - 1] == 'c' && path[length - 2] == '.') {

        return 1;
    }

    return 0;
}

void WriteToFile(int file, char *message) {

    //Variable declarations.
    int bytesWrote;

    bytesWrote = write(file, message, strlen(message));

    //Check that the message was written.
    if (bytesWrote < 0) {

        perror("Error: failed to write to file.\n");
        exit(1);
    }
}

void ReadFromFile(int fileDesc, char *buffer) {

    //Variable declarations.
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

            buffer[counter] = '\0';
            stop = 1;
        }

        counter++;
    }
}

int CompileStudentFile(Student *student) {

    //Variable declarations.
    pid_t compilePId;

    compilePId = fork();

    if (compilePId < 0) {

        perror("Error: fork failed.\n");
        exit(1);
    }

    if (compilePId == 0) {

        char *args[] = {"gcc", student->cFilePath, "-o", "student.out", 0};
        int  retExec;

        retExec = execvp("gcc", args);

        if (retExec == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }
    } else {

        return WaitForChildExec(&student->status.compileStatus);
    }
}

int ExecuteStudentFile(Student *student, char *inputFilePath) {

    //Variable declarations.
    pid_t execPId;

    execPId = fork();

    //Check if fork succeeded.
    if (execPId < 0) {

        perror("Error: fork failed.\n");
        exit(1);
    }

    if (execPId == 0) {

        char *argsStudent[] = {"./student.out", inputFilePath,
                               0};

        //Variable declarations.
        int  studentOutputFile;
        int  inputFile;
        int  execValue;
        int  closeValue;
        int  dupResult;

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
        dupResult = dup2(inputFile, 0);

        //Check if dup succeeded.
        if (dupResult < 0) {

            perror("Error: dup2 failed.\n");
            exit(1);
        }

        dupResult = dup2(studentOutputFile, 1);

        //Check if dup succeeded.
        if (dupResult < 0) {

            perror("Error: dup2 failed.\n");
            exit(1);
        }

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

        //Variable declarations.
        int exitStatus;
        int timerStatus;

        //Check for timeout.
        student->isTimeOut = TimeoutHandler(execPId, &timerStatus);

        if (student->isTimeOut == 1) {

            //Wait for child process to finish.
            return WaitForChildExec(&exitStatus);

        } else {

            return 1;
        }
    }
}

int CompareStudentFile(Student *student, char *correctOutput,
                       char *studentOutput) {

    //Variable declarations.
    pid_t compPId;

    compPId = fork();

    //Check if fork succeeded.
    if(compPId == -1){

        perror("Error: fork failed.\n");
        exit(1);
    }

    if (compPId == 0) {

        char *argsComp[] = {"./comp.out", correctOutput, studentOutput, 0};
        int  compExec;

        //Execute comparison.
        compExec = execvp("./comp.out", argsComp);

        //Check if execution worked.
        if (compExec == -1) {

            perror("Error: execution failed.\n");
            exit(1);
        }

    } else {

        //Wait for child process to finish.
        WaitForChildExec(&student->status.compareStatus);

        return WEXITSTATUS(student->status.compareStatus);
    }
}

int WaitForChildExec(int *status) {

    //Variable declarations.
    int waitVal;

    waitVal = wait(status);

    if (waitVal == -1) {

        perror("Error: wait failed.\n");
        exit(1);
    }

    //Check status.
    if (WIFEXITED(*status)) {

        //Check if execution succeeded.
        if (WEXITSTATUS(*status) == 1) {

            return 0;
        }

        return 1;
    }
}

void WriteStudentResult(Student *student) {

    //Variable declarations.
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
    WriteToFile(results, resultToWrite);

    closeValue = close(results);

    //Check if file was closed.
    if (closeValue < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }
}

int TimeoutHandler(pid_t pid, int *status) {

    //Variable declarations.
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

Student *InitStudent(struct dirent *studentDirent, char *dirPath) {

    //Variable declarations.
    Student *student;

    student = (Student *) malloc(sizeof(Student));

    //Check if allocation worked.
    if (student == 0) {

        perror("Error: malloc failed.\n");
        exit(1);
    }

    //Initialize student members.
    student->dirent   = studentDirent;
    student->name     = studentDirent->d_name;
    student->homePath = dirPath;
    student->depth    = -1;
    strcpy(student->result.feedback, "\0");
    student->isMultipleDirectories = 0;
    student->isTimeOut             = 0;

    return student;
}

void FreeStudent(Student *student) {

    free(student->cFilePath);
    free(student);
}

void HandleCompilationError(Student *student) {

    //Set student's grade tp 0.
    student->result.grade = 0;
    strcat(student->result.feedback, ",COMPILATION_ERROR");
    WriteStudentResult(student);
    FreeStudent(student);
}

void HandleTimeout(Student *student) {

    //Variable declarations.
    int unlinkResult;

    //Set student's grade tp 0.
    student->result.grade = 0;
    strcat(student->result.feedback, ",TIMEOUT");
    WriteStudentResult(student);
    FreeStudent(student);

    //Unlink student's output file.
    unlinkResult = unlink("studentOutput.txt");

    //Check if unlinked file.
    if (unlinkResult < 0) {

        perror("Error: failed to unlink file.\n");
        exit(1);
    }
}

void HandleComparisonResult(Student *student, int compareResult) {

    switch (compareResult) {

        case 1:
            strcat(student->result.feedback, ",GREAT_JOB");
            break;

        case 2:
            //Set student's grade grade - 30.
            student->result.grade -= 30;
            strcat(student->result.feedback, ",SIMILLAR_OUTPUT");
            break;

        case 3:
            //Set student's grade tp 0.
            student->result.grade = 0;
            strcat(student->result.feedback, ",BAD_OUTPUT");
            break;

        default:
            break;
    }

    //Check C file's depth.
    if (student->depth > 0) {
        strcat(student->result.feedback, ",WRONG_DIRECTORY");
    }
}
