#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#define BUFFER_SIZE 1

/**
* Checks if the files are identical.
* @param fileName1 file path.
* @param fileName2 file path.
* @return 1 if the files are identical, else 0.
*/
int isFilesIdentical(char *fileName1, char *fileName2);

/**
 * Checks if the files are similar.
 * @param fileName1 file path.
 * @param fileName2 file path.
 * @return 1 if the files are similar, else 0.
 */
int isFilesSimilar(char *fileName1, char *fileName2);

/**
 * Opens a given file path for reading.
 * @param fileName file path.
 * @return file descriptor.
 */
int openFileToRead(char *fileName);

int main(int argc, char *argv[]) {

    //Check that the number of parameters is correct.
    if (argc != 3) {

        perror("Error: wrong number of parameters.\n");

        return 3;
    }

    //Sets the files names.
    char *fileName1 = argv[1];
    char *fileName2 = argv[2];

    int retVal = 0;

    //Compare files.
    if (isFilesIdentical(fileName1, fileName2)) {

        retVal = 1;
    } else if (isFilesSimilar(fileName1, fileName2)) {

        retVal = 2;
    } else {

        retVal = 3;
    }

    return retVal;
}

int isFilesIdentical(char *fileName1, char *fileName2) {

    //Variable declaration.
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    int  file1       = 0;
    int  file2       = 0;
    int  stop        = 0;
    int  readFile1   = 0;
    int  readFile2   = 0;
    int  retVal      = 1;
    int  closeResult = 0;

    //Open files for reading.
    file1 = openFileToRead(fileName1);
    file2 = openFileToRead(fileName2);

    while (!stop) {

        readFile1 = read(file1, buffer1, 1);

        if (readFile1 < 0) {

            perror("Error while reading from file.\n");
        }

        readFile2 = read(file2, buffer2, 1);

        //Check if read data.
        if (readFile2 < 0) {

            perror("Error while reading from file.\n");
        }

        //Check if reached end of files.
        if (readFile1 == 0 && readFile2 == 0) {

            stop = 1;
        }

        //Check if the read chars are equal.
        if (*buffer1 != *buffer2) {

            stop   = 1;
            retVal = 0;
        }
    }

    closeResult = close(file1);

    //Check if file was closed.
    if (closeResult < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    closeResult = close(file2);

    //Check if file was closed.
    if (closeResult < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    return retVal;
}

int isFilesSimilar(char *fileName1, char *fileName2) {

    //Variable declaration.
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    int  file1       = 0;
    int  file2       = 0;
    int  stop        = 0;
    int  isLetter1   = 0;
    int  isLetter2   = 0;
    int  readFile1   = 0;
    int  readFile2   = 0;
    int  retVal      = 1;
    int  closeResult = 0;

    //Open files for reading.
    file1 = openFileToRead(fileName1);
    file2 = openFileToRead(fileName2);

    while (!stop) {

        isLetter1 = 0;
        isLetter2 = 0;

        //Search for a legal char in file 1.
        while (!isLetter1) {

            readFile1 = read(file1, buffer1, 1);

            //Check if read data.
            if (readFile1 < 0) {

                perror("Error while reading from file.\n");
                return 3;
            }

            //Check if encountered a legal letter.
            if (readFile1 == 0) {

                isLetter1 = 1;
            }

            //Check if encountered an illegal letter.
            if ((*buffer1 != ' ') && (*buffer1 != '\n')) {

                isLetter1 = 1;
            }
        }

        //Search for a legal char in file 2.
        while (!isLetter2) {

            readFile2 = read(file2, buffer2, 1);

            //Check if read data.
            if (readFile2 < 0) {

                perror("Error while reading from file.\n");
                return 3;
            }

            //Check if encountered a legal letter.
            if (readFile2 == 0) {

                isLetter2 = 1;
            }

            //Check if encountered an illegal letter.
            if ((*buffer2 != ' ') && (*buffer2 != '\n')) {

                isLetter2 = 1;
            }
        }

        //Check if reached end of files.
        if (readFile1 == 0 && readFile2 == 0) {

            stop = 1;
        }

        //Convert to lower case chars.
        *buffer1 = tolower(*buffer1);
        *buffer2 = tolower(*buffer2);

        //Check if chars are equal.
        if (*buffer1 != *buffer2) {

            stop   = 1;
            retVal = 0;
        }
    }

    closeResult = close(file1);

    //Check if file was closed.
    if (closeResult < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    closeResult = close(file2);

    //Check if file was closed.
    if (closeResult < 0) {

        perror("Error: failed to close file.\n");
        exit(1);
    }

    return retVal;
}

int openFileToRead(char *fileName) {

    int file = 0;

    file = open(fileName, O_RDONLY);

    //Check that file 1 was opened correctly.
    if (file == -1) {

        perror(fileName);

        exit(1);
    }

    return file;
}

