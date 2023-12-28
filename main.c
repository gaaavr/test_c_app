#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define elementSize 100 // гарантированный предел элемента данных в файле

typedef struct {
    char name[(elementSize - 12) / 2];
    char surname[(elementSize - 12) / 2];
    int day;
    int month;
    int year;
} student;

struct flags {
    char *testData;
    char *rfile;
    char *wfile;
    char *sortBy;
    char *sortDir;
    int offset;
    int limit;
};

struct studentList {
    student student;
    struct studentList *next;
};

struct flags parseFlags(int argc, char *argv[]);

int writeTestData(char *filename);

student *readFromFile(char *filename, int offset, int limit, int *studentsCount);

int writeData(char *filename, struct studentList *list);

void convertToLinkedList(struct studentList **list, student *students, int size);

void writeToFile(FILE *f, struct studentList *list);

void sortData(student *students, char *sortBy, char *sortdir, int size);

int main(int argc, char *argv[]) {
    struct flags flagsValues = parseFlags(argc, argv);

    if (flagsValues.testData) {
        if (writeTestData(flagsValues.testData) != 0) {
            return 1;
        }
    }

    student *students = NULL;
    int studentsCount = 0;

    students = readFromFile(flagsValues.rfile, flagsValues.offset, flagsValues.limit, &studentsCount);
    if (!students) {
        return 1;
    }

    sortData(students, flagsValues.sortBy, flagsValues.sortDir, studentsCount);

    struct studentList *list = NULL;

    convertToLinkedList(&list, students, studentsCount);

    writeData(flagsValues.wfile, list);

    free(students);
    free(list);

    return 0;
}

// Функция для парсинга и валидации аргументов командной строки
struct flags parseFlags(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
        printf("Аргументы командной строки для запуска вводятся в формате \"-аргумент значение\". Сами аргументы перечислены нижею \n"
               "-testData - заполнение файла тестовыми данными (передаётся имя файла для заполнения тестовыми данными)\n"
               "-rfile - имя файла, из которого считываются данные\n"
               "-wfile - имя файла, в который записываются данные (по умолчанию данные запишутся в stdout\n"
               "-sortBy - сортировка данных, (-names - по фамилии с именем, -dates - по датам рождения)\n"
               "-sortDir - направление сортировки (-asc - по возрастанию, -desc - по убыванию)\n"
               "-offset - количество байтов для смещения при чтении данных из файла\n"
               "-limit - верхний предел количества считанных строк из файла (если 0, то limit не применяется)\n");

        exit(0);
    }

    if (argc < 2 || argc > 15) {
        printf("нужно передать не меньше 1 аргумента и не больше 14 для запуска программы,\n"
               "аргументы можно увидеть запустив программу с флагом -help\n");
        exit(1);;
    }

    struct flags flagsValues;
    flagsValues.testData = NULL;
    flagsValues.rfile = NULL;
    flagsValues.wfile = NULL;
    flagsValues.sortBy = NULL;
    flagsValues.sortDir = NULL;
    flagsValues.offset = 0;
    flagsValues.limit = 0;

    for (int i = 1; i < argc; i += 2) {
        // Проверка на флаг -testData
        if (strcmp(argv[i], "-testData") == 0) {
            flagsValues.testData = argv[i + 1];
            continue;
        }

        // Проверка на флаг -rfile
        if (strcmp(argv[i], "-rfile") == 0) {
            flagsValues.rfile = argv[i + 1];
            continue;
        }

        // Проверка на флаг -wfile
        if (strcmp(argv[i], "-wfile") == 0) {
            flagsValues.wfile = argv[i + 1];
            continue;
        }

        // Проверка на флаг -sortBy
        if (strcmp(argv[i], "-sortBy") == 0) {
            flagsValues.sortBy = argv[i + 1];
            continue;
        }

        // Проверка на флаг -sortDir
        if (strcmp(argv[i], "-sortDir") == 0) {
            flagsValues.sortDir = argv[i + 1];
            continue;
        }

        // Проверка на флаг -offset
        if (strcmp(argv[i], "-offset") == 0) {
            flagsValues.offset = atoi(argv[i + 1]);
            continue;
        }

        // Проверка на флаг -limit
        if (strcmp(argv[i], "-limit") == 0) {
            flagsValues.limit = atoi(argv[i + 1]);
            continue;
        }

        printf("неверный аргумент командной строки: %s\n", argv[i]);
        exit(1);
    }

    if (flagsValues.rfile == NULL) {
        printf("не передано название файла для чтения данных\n");
        exit(1);
    }

    if (flagsValues.offset < 0) {
        printf("смещение по файлу не может иметь отрицательное значение\n");
        exit(1);
    }

    if (flagsValues.limit < 0) {
        printf("предел строк для записи не может иметь отрицательное значение\n");
        exit(1);
    }

    return flagsValues;
}

// Функция для записи тестовых данных в файл
int writeTestData(char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        printf("ошибка при попытке открыть файл для записи тестовых данных%s\n", filename);
        return 1;
    }

    // Инициализируем массив с тестовыми данными из 5 строк, размер каждой строки ограничен 80 символами
    char testData[6][elementSize] = {
            "Гаврилин Сергей 10.03.1997\n",
            "Тереньтев Михаил 10.04.1987\n",
            "Абрамов Антон 02.11.1994\n",
            "Мастихина Анна 01.03.1997\n",
            "Бобров Иван 12.04.1961\n",
            "Бобров Виктор 14.06.1988",
    };


    for (int i = 0; i < (sizeof(testData) / sizeof(testData[0])); i++) {
        fputs(testData[i], f);
    }

    fclose(f);

    return 0;
};

// Функция для чтения данных из файла
student *readFromFile(char *filename, int offset, int limit, int *studentsCount) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("ошибка при попытке открыть файл %s\n", filename);
        return NULL;
    }

    if (fseek(f, offset, SEEK_SET) != 0) {
        printf("ошибка при попытке смещения по файлу");
        return NULL;
    };

    int i = 0;

    student *students = malloc(sizeof(student));

    // считываем данные пока не наступит конец файла или пока не дойдём до заданного лимита
    while ((!feof(f) && limit == 0) || (!feof(f) && i < limit)) {
        students[i] = *(student *) malloc(sizeof(student));

        fscanf(f, "%s %s %d.%d.%d", students[i].surname, students[i].name,
               &students[i].day, &students[i].month, &students[i].year);

        i++;
    }

    fclose(f);

    *studentsCount = i;

    return students;
}

// Функция для конвертации данных из массива структур в односвязный список
void convertToLinkedList(struct studentList **list, student *students, int size) {
    int i = 0;
    while (i < size) {
        struct studentList *nextStudent = (struct studentList *) malloc(sizeof(struct studentList));
        nextStudent->student = students[i];
        nextStudent->next = *list;
        *list = nextStudent;

        i++;
    }
}

// Функция для записи данных в файл
int writeData(char *filename, struct studentList *list) {
    FILE *f = stdout;
    if (filename) {
        f = fopen(filename, "w");
        if (!f) {
            printf("ошибка при попытке открыть файл для записи данных %s\n", filename);
            return 1;
        }
    }

    writeToFile(f, list);

    fclose(f);

    return 0;
};

// Функция для записи данных в файл построчно, вызывается рекурсивно до тех пор, пока односвязный список не достигнет конца
void writeToFile(FILE *f, struct studentList *list) {
    if (list) {
        writeToFile(f, list->next);

        char student[elementSize];

        sprintf(student, "%s %s %d.%d.%d\n",
                list->student.surname, list->student.name, list->student.day, list->student.month, list->student.year);
        fprintf(f, "%s", student);
    }
}

// Функция для сравнения даты при сортировке по возрастанию
int cmpByDatesAsc(const void *_a, const void *_b) {
    student *a = (student *) _a;
    student *b = (student *) _b;

    int firstEl = b->year;
    int secondEl = a->year;

    if (firstEl == secondEl) {
        firstEl = b->month;
        secondEl = a->month;
    }

    if (firstEl == secondEl) {
        firstEl = b->day;
        secondEl = a->day;
    }

    return secondEl - firstEl;
}

// Функция для сравнения даты при сортировке по убыванию
int cmpByDatesDesc(const void *_a, const void *_b) {
    student *a = (student *) _a;
    student *b = (student *) _b;

    int firstEl = b->year;
    int secondEl = a->year;

    if (firstEl == secondEl) {
        firstEl = b->month;
        secondEl = a->month;
    }

    if (firstEl == secondEl) {
        firstEl = b->day;
        secondEl = a->day;
    }

    return firstEl - secondEl;
}

// Функция для сравнения фамилий и имён при сортировке по убыванию
int cmpByNamesDesc(const void *_a, const void *_b) {
    student *a = (student *) _a;
    student *b = (student *) _b;

    char *firstEl = b->surname;
    char *secondEl = a->surname;

    if (strcmp(secondEl, firstEl) == 0) {
        firstEl = b->name;
        secondEl = a->name;
    }

    return strcmp(firstEl, secondEl);
}

// Функция для сравнения фамилий и имён при сортировке по возрастанию
int cmpByNamesAsc(const void *_a, const void *_b) {
    student *a = (student *) _a;
    student *b = (student *) _b;

    char *firstEl = b->surname;
    char *secondEl = a->surname;

    if (strcmp(secondEl, firstEl) == 0) {
        firstEl = b->name;
        secondEl = a->name;
    }

    return strcmp(secondEl, firstEl);
}

// Функция для сортировки данных в соответствии с переданными аргументами командной строки
void sortData(student *students, char *sortBy, char *sortdir, int size) {
    if (!sortBy) {
        return;
    }

    if (!sortdir) {
        sortdir = "asc";
    }

    if (strcmp(sortBy, "names") == 0) {
        if (strcmp(sortdir, "desc") == 0) {
            qsort(students, size, elementSize, cmpByNamesDesc);
            return;
        }

        qsort(students, size, elementSize, cmpByNamesAsc);
        return;
    }

    if (strcmp(sortBy, "dates") == 0) {
        if (strcmp(sortdir, "desc") == 0) {
            qsort(students, size, elementSize, cmpByDatesDesc);
            return;
        }

        qsort(students, size, elementSize, cmpByDatesAsc);
        return;
    }
}
