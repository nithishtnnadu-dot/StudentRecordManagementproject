
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define FILE_NAME "students.txt"
#define NAME_LEN 100
#define DEPT_LEN 50
#define INPUT_LEN 256
#define RESET "\033[0m"
#define CYAN "\033[1;36m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define MAGENTA "\033[1;35m"
#define BLUE "\033[1;34m"
#define WHITE "\033[1;37m"

typedef struct Student {
    int roll;
    char name[NAME_LEN];
    char department[DEPT_LEN];
    float marks;
    struct Student *next;
} Student;

static Student *head = NULL;

/* ---------- CONSOLE ---------- */

void initializeConsole(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (h != INVALID_HANDLE_VALUE &&
        GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | 0x0004);
    }
#endif
}

void clearScreen(void) {
    printf("\033[2J\033[H");
}

void pauseScreen(void) {
    char line[INPUT_LEN];
    printf("\n" CYAN "Press ENTER to continue..." RESET);
    fgets(line, sizeof(line), stdin);
}

void line(void) {
    printf(BLUE
           "============================================================\n"
           RESET);
}

/* ---------- SAFE INPUT ---------- */

int readLine(const char *prompt, char *buffer, size_t size) {
    size_t len;
    int ch;

    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL)
        return 0;

    len = strlen(buffer);

    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[--len] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard excess input. */
        }
    }

    if (len > 0 && buffer[len - 1] == '\r')
        buffer[len - 1] = '\0';

    return 1;
}

int readInt(const char *prompt, int *value) {
    char input[INPUT_LEN];
    char *end;
    long number;

    while (1) {
        if (!readLine(prompt, input, sizeof(input)))
            return 0;

        errno = 0;
        number = strtol(input, &end, 10);

        while (isspace((unsigned char)*end))
            end++;

        if (input[0] != '\0' && *end == '\0' &&
            errno != ERANGE && number > 0 &&
            number <= INT_MAX) {
            *value = (int)number;
            return 1;
        }

        printf(RED "Enter a valid positive whole number.\n" RESET);
    }
}

int readMarks(const char *prompt, float *marks) {
    char input[INPUT_LEN];
    char *end;
    float value;

    while (1) {
        if (!readLine(prompt, input, sizeof(input)))
            return 0;

        errno = 0;
        value = strtof(input, &end);

        while (isspace((unsigned char)*end))
            end++;

        if (input[0] != '\0' && *end == '\0' &&
            errno != ERANGE && isfinite(value) &&
            value >= 0.0f && value <= 100.0f) {
            *marks = value;
            return 1;
        }

        printf(RED "Marks must be between 0 and 100.\n" RESET);
    }
}

int readText(const char *prompt, char *text, size_t size) {
    while (1) {
        if (!readLine(prompt, text, size))
            return 0;

        int nonSpace = 0;
        size_t i;

        for (i = 0; text[i] != '\0'; i++) {
            if (!isspace((unsigned char)text[i]))
                nonSpace = 1;
        }

        if (nonSpace && strchr(text, '|') == NULL)
            return 1;

        printf(RED
               "Field cannot be empty or contain '|'. Try again.\n"
               RESET);
    }
}

/* ---------- SEARCH ---------- */

Student *findByRoll(int roll) {
    Student *current = head;

    while (current != NULL) {
        if (current->roll == roll)
            return current;

        current = current->next;
    }

    return NULL;
}

int containsIgnoreCase(const char *text, const char *query) {
    size_t i, j;

    if (*query == '\0')
        return 1;

    for (i = 0; text[i] != '\0'; i++) {
        for (j = 0; query[j] != '\0'; j++) {
            if (text[i + j] == '\0' ||
                tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)query[j])) {
                break;
            }
        }

        if (query[j] == '\0')
            return 1;
    }

    return 0;
}

void printStudent(const Student *s) {
    printf("\nRoll number : %d\n", s->roll);
    printf("Name        : %s\n", s->name);
    printf("Department  : %s\n", s->department);
    printf("Marks       : %.2f / 100\n", s->marks);
    printf("Result      : %s\n",
           s->marks >= 50.0f ? "PASS" : "FAIL");
}

/* ---------- FILE HANDLING ---------- */

int saveStudents(void) {
    FILE *file = fopen(FILE_NAME, "w");
    Student *current;

    if (file == NULL) {
        printf(RED "Cannot open %s for writing.\n" RESET,
               FILE_NAME);
        return 0;
    }

    for (current = head; current != NULL;
         current = current->next) {
        if (fprintf(file, "%d|%s|%s|%.2f\n",
                    current->roll, current->name,
                    current->department, current->marks) < 0) {
            fclose(file);
            printf(RED "Error writing records.\n" RESET);
            return 0;
        }
    }

    if (fclose(file) != 0) {
        printf(RED "Error closing the data file.\n" RESET);
        return 0;
    }

    printf(GREEN "Records saved to %s.\n" RESET, FILE_NAME);
    return 1;
}

void appendStudent(Student *student) {
    Student *current;

    student->next = NULL;

    if (head == NULL) {
        head = student;
        return;
    }

    current = head;

    while (current->next != NULL)
        current = current->next;

    current->next = student;
}

void loadStudents(void) {
    FILE *file = fopen(FILE_NAME, "r");
    char buffer[INPUT_LEN];
    int loaded = 0;

    if (file == NULL)
        return;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        int roll;
        char name[NAME_LEN];
        char department[DEPT_LEN];
        float marks;
        char extra;

        if (strchr(buffer, '\n') == NULL && !feof(file)) {
            int ch;
            while ((ch = fgetc(file)) != '\n' && ch != EOF) {
            }
            printf(YELLOW "Skipped an overlong record.\n" RESET);
            continue;
        }

        if (sscanf(buffer, "%d|%99[^|]|%49[^|]|%f %c",
                   &roll, name, department, &marks, &extra) != 4 ||
            roll <= 0 || !isfinite(marks) ||
            marks < 0 || marks > 100 ||
            name[0] == '\0' || department[0] == '\0' ||
            findByRoll(roll) != NULL) {
            printf(YELLOW "Skipped an invalid record.\n" RESET);
            continue;
        }

        Student *student = malloc(sizeof(Student));

        if (student == NULL) {
            printf(RED "Insufficient memory to load records.\n" RESET);
            break;
        }

        student->roll = roll;
        strcpy(student->name, name);
        strcpy(student->department, department);
        student->marks = marks;

        appendStudent(student);
        loaded++;
    }

    if (ferror(file))
        printf(RED "An error occurred while reading records.\n" RESET);

    fclose(file);
    printf(GREEN "%d record(s) loaded.\n" RESET, loaded);
}

/* ---------- ADD STUDENT ---------- */

void addStudent(void) {
    int roll;
    Student *student;

    line();
    printf(CYAN "                 ADD NEW STUDENT\n" RESET);
    line();

    if (!readInt("Roll number: ", &roll))
        return;

    if (findByRoll(roll) != NULL) {
        printf(RED "This roll number already exists.\n" RESET);
        return;
    }

    student = malloc(sizeof(Student));

    if (student == NULL) {
        printf(RED "Memory allocation failed.\n" RESET);
        return;
    }

    student->roll = roll;

    if (!readText("Student name: ", student->name,
                  sizeof(student->name)) ||
        !readText("Department: ", student->department,
                  sizeof(student->department)) ||
        !readMarks("Marks (0-100): ", &student->marks)) {
        free(student);
        return;
    }

    appendStudent(student);

    if (saveStudents())
        printf(GREEN "Student added successfully!\n" RESET);
    else
        printf(YELLOW
               "Student is in memory, but saving failed.\n" RESET);
}

/* ---------- DISPLAY ---------- */

void displayStudents(void) {
    Student *current = head;

    line();
    printf(CYAN "                    STUDENT RECORDS\n" RESET);
    line();

    if (current == NULL) {
        printf(YELLOW "No student records found.\n" RESET);
        return;
    }

    printf(WHITE "%-8s %-25s %-22s %8s\n" RESET,
           "ROLL NO", "NAME", "DEPARTMENT", "MARKS");

    printf("-------------------------------------------------------------------\n");

    while (current != NULL) {
        printf("%-8d %-25.25s %-22.22s %8.2f\n",
               current->roll, current->name,
               current->department, current->marks);

        current = current->next;
    }

    printf("-------------------------------------------------------------------\n");
}

/* ---------- SEARCH BY ROLL OR NAME ---------- */

void searchStudent(void) {
    int choice;

    printf("\n1. Search by roll number\n");
    printf("2. Search by student name\n");

    if (!readInt("Choose search method: ", &choice))
        return;

    if (choice == 1) {
        int roll;

        if (!readInt("Enter roll number: ", &roll))
            return;

        Student *s = findByRoll(roll);

        if (s != NULL)
            printStudent(s);
        else
            printf(RED "Student not found.\n" RESET);
    } else if (choice == 2) {
        char query[NAME_LEN];
        int found = 0;
        Student *current = head;

        if (!readText("Enter name or part of name: ",
                      query, sizeof(query)))
            return;

        while (current != NULL) {
            if (containsIgnoreCase(current->name, query)) {
                printStudent(current);
                found = 1;
            }

            current = current->next;
        }

        if (!found)
            printf(RED "No matching student found.\n" RESET);
    } else {
        printf(RED "Invalid search option.\n" RESET);
    }
}

/* ---------- UPDATE ---------- */

void updateStudent(void) {
    int roll;
    char name[NAME_LEN];
    char department[DEPT_LEN];
    float marks;

    if (!readInt("Enter roll number to update: ", &roll))
        return;

    Student *student = findByRoll(roll);

    if (student == NULL) {
        printf(RED "Student not found.\n" RESET);
        return;
    }

    printf("Current details:\n");
    printStudent(student);

    printf("\nEnter replacement details:\n");

    if (!readText("New name: ", name, sizeof(name)) ||
        !readText("New department: ", department,
                  sizeof(department)) ||
        !readMarks("New marks: ", &marks))
        return;

    strcpy(student->name, name);
    strcpy(student->department, department);
    student->marks = marks;

    if (saveStudents())
        printf(GREEN "Student updated successfully.\n" RESET);
    else
        printf(YELLOW "Update made in memory; saving failed.\n" RESET);
}

/* ---------- DELETE ---------- */

void deleteStudent(void) {
    int roll;
    char answer[INPUT_LEN];
    Student *current = head;
    Student *previous = NULL;

    if (!readInt("Enter roll number to delete: ", &roll))
        return;

    while (current != NULL && current->roll != roll) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        printf(RED "Student not found.\n" RESET);
        return;
    }

    printStudent(current);

    if (!readLine("Delete this record? (Y/N): ",
                  answer, sizeof(answer)))
        return;

    if (strcmp(answer, "Y") != 0 &&
        strcmp(answer, "y") != 0) {
        printf(YELLOW "Deletion cancelled.\n" RESET);
        return;
    }

    if (previous == NULL)
        head = current->next;
    else
        previous->next = current->next;

    free(current);

    if (saveStudents())
        printf(GREEN "Student deleted successfully.\n" RESET);
    else
        printf(YELLOW "Deleted from memory; saving failed.\n" RESET);
}

/* ---------- MERGE SORT ---------- */

int compareStudents(const Student *a, const Student *b,
                    int sortByMarks) {
    if (sortByMarks) {
        if (a->marks < b->marks)
            return -1;
        if (a->marks > b->marks)
            return 1;
    }

    if (a->roll < b->roll)
        return -1;
    if (a->roll > b->roll)
        return 1;

    return 0;
}

Student *mergeLists(Student *a, Student *b, int sortByMarks) {
    Student dummy;
    Student *tail = &dummy;

    dummy.next = NULL;

    while (a != NULL && b != NULL) {
        if (compareStudents(a, b, sortByMarks) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }

        tail = tail->next;
    }

    tail->next = (a != NULL) ? a : b;

    return dummy.next;
}

void splitList(Student *source, Student **front,
               Student **back) {
    Student *slow = source;
    Student *fast = source->next;

    while (fast != NULL) {
        fast = fast->next;

        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = source;
    *back = slow->next;
    slow->next = NULL;
}

Student *mergeSort(Student *list, int sortByMarks) {
    Student *left;
    Student *right;

    if (list == NULL || list->next == NULL)
        return list;

    splitList(list, &left, &right);

    left = mergeSort(left, sortByMarks);
    right = mergeSort(right, sortByMarks);

    return mergeLists(left, right, sortByMarks);
}

void sortStudents(void) {
    int choice;

    if (head == NULL) {
        printf(YELLOW "No records to sort.\n" RESET);
        return;
    }

    printf("\n1. Sort by roll number\n");
    printf("2. Sort by marks (highest first)\n");

    if (!readInt("Choose sorting method: ", &choice))
        return;

    if (choice != 1 && choice != 2) {
        printf(RED "Invalid sorting option.\n" RESET);
        return;
    }

    head = mergeSort(head, choice == 2);

    if (choice == 2) {
        /* Reverse ascending marks order for highest first. */
        Student *previous = NULL;
        Student *current = head;

        while (current != NULL) {
            Student *next = current->next;
            current->next = previous;
            previous = current;
            current = next;
        }

        head = previous;
    }

    if (saveStudents())
        printf(GREEN "Sorting completed and records saved.\n" RESET);
    else
        printf(YELLOW "Sorted in memory, but saving failed.\n" RESET);

    displayStudents();
}

/* ---------- DASHBOARD AND REPORTS ---------- */

void showDashboard(void) {
    Student *current = head;
    Student *topper = NULL;
    int total = 0, passed = 0, failed = 0;
    float totalMarks = 0.0f;

    while (current != NULL) {
        total++;
        totalMarks += current->marks;

        if (current->marks >= 50)
            passed++;
        else
            failed++;

        if (topper == NULL || current->marks > topper->marks)
            topper = current;

        current = current->next;
    }

    line();
    printf(MAGENTA "                    EDUCORE DASHBOARD\n" RESET);
    printf(WHITE "               STUDENT MANAGEMENT SYSTEM\n" RESET);
    line();

    printf(CYAN " Total students : " RESET "%d\n", total);
    printf(CYAN " Average marks  : " RESET "%.2f\n",
           total > 0 ? totalMarks / total : 0.0f);

    printf(GREEN " Passed         : %d\n" RESET, passed);
    printf(RED " Failed         : %d\n" RESET, failed);

    if (topper != NULL) {
        printf(YELLOW " Topper         : %s\n" RESET, topper->name);
        printf(YELLOW " Highest marks  : %.2f\n" RESET, topper->marks);
    } else {
        printf(YELLOW " Topper         : No records yet\n" RESET);
    }

    line();
}

void departmentReport(void) {
    Student *current = head;

    if (head == NULL) {
        printf(YELLOW "No records available.\n" RESET);
        return;
    }

    line();
    printf(CYAN "                  DEPARTMENT REPORT\n" RESET);
    line();

    while (current != NULL) {
        int alreadyShown = 0;
        Student *check = head;

        while (check != current) {
            if (strcmp(check->department,
                       current->department) == 0) {
                alreadyShown = 1;
                break;
            }

            check = check->next;
        }

        if (!alreadyShown) {
            int count = 0, passed = 0;
            float sum = 0.0f;
            Student *item = head;

            while (item != NULL) {
                if (strcmp(item->department,
                           current->department) == 0) {
                    count++;
                    sum += item->marks;

                    if (item->marks >= 50)
                        passed++;
                }

                item = item->next;
            }

            printf("\nDepartment: %s\n", current->department);
            printf("Students  : %d\n", count);
            printf("Average   : %.2f\n", sum / count);
            printf("Pass      : %d\n", passed);
            printf("Fail      : %d\n", count - passed);
        }

        current = current->next;
    }
}

/* ---------- MENU ---------- */

void showMenu(void) {
    printf("\n");
    line();
    printf(WHITE "                       MAIN MENU\n" RESET);
    line();

    printf(" " GREEN "1." RESET " Add Student\n");
    printf(" " GREEN "2." RESET " Display All Students\n");
    printf(" " GREEN "3." RESET " Search Student\n");
    printf(" " GREEN "4." RESET " Update Student\n");
    printf(" " GREEN "5." RESET " Delete Student\n");
    printf(" " GREEN "6." RESET " Sort Students\n");
    printf(" " GREEN "7." RESET " Dashboard\n");
    printf(" " GREEN "8." RESET " Department Report\n");
    printf(" " GREEN "9." RESET " Save Records\n");
    printf(" " RED "10." RESET " Exit\n");

    line();
}

/* ---------- MEMORY CLEANUP ---------- */

void freeAllStudents(void) {
    Student *current = head;

    while (current != NULL) {
        Student *next = current->next;
        free(current);
        current = next;
    }

    head = NULL;
}

/* ---------- MAIN ---------- */

int main(void) {
    int choice;

    initializeConsole();

    clearScreen();

    printf(MAGENTA
           "============================================================\n"
           "                         EDUCORE\n"
           "               STUDENT MANAGEMENT SYSTEM\n"
           "============================================================\n"
           RESET);

    printf(WHITE
           "          C PROGRAMMING | DATA STRUCTURES & ALGORITHMS\n"
           "             LINKED LIST | MERGE SORT | FILE STORAGE\n"
           RESET);

    line();

    loadStudents();

    while (1) {
        showDashboard();
        showMenu();

        if (!readInt("Enter choice (1-10): ", &choice)) {
            printf("\nSaving before exit...\n");
            saveStudents();
            break;
        }

        switch (choice) {
            case 1:
                addStudent();
                break;
            case 2:
                displayStudents();
                break;
            case 3:
                searchStudent();
                break;
            case 4:
                updateStudent();
                break;
            case 5:
                deleteStudent();
                break;
            case 6:
                sortStudents();
                break;
            case 7:
                showDashboard();
                break;
            case 8:
                departmentReport();
                break;
            case 9:
                saveStudents();
                break;
            case 10:
                if (saveStudents()) {
                    freeAllStudents();
                    printf(GREEN
                           "Thank you for using EDUCORE!\n"
                           RESET);
                    return 0;
                }
                printf(RED
                       "Save failed. Please retry before exiting.\n"
                       RESET);
                break;
            default:
                printf(RED "Choose a number from 1 to 10.\n" RESET);
        }

        pauseScreen();
        clearScreen();
    }

    freeAllStudents();
    return 0;
}
