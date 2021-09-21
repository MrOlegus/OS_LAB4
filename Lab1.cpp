#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <list>

using namespace std;

#define THREAD_COUNT 5

class Task {
public:
    char* text;

    Task(char* text, int n, int size) {
        (*this).text = (char*)malloc(size * sizeof(char));
        for (int i = 0; i < size; i++)
            (*this).text[i] = text[i + n];
        (*this).text[size] = '\0';
    }
};

class Queue {
public:
    list <Task> taskList;

    void PushTask(Task task) {
        taskList.push_back(task);
    }

    Task PopTusk() {
        if (taskList.empty()) return *new Task((char*)"", 0, 0);
        Task result = taskList.front();
        taskList.pop_front();
        return result;
    }
};

Queue queue = (*new Queue()); // очередь задач
Queue queueCompleted = (*new Queue()); // очередь законченных задач
HANDLE hMutexQueue; // для синхронизации доступа к queue
HANDLE hMutexCompleted; // для синхронизации доступа к queueCompleted

void MergeSort(char a[], size_t l)
{
    size_t BlockSizeIterator;
    size_t BlockIterator;
    size_t LeftBlockIterator;
    size_t RightBlockIterator;
    size_t MergeIterator;

    size_t LeftBorder;
    size_t MidBorder;
    size_t RightBorder;
    for (BlockSizeIterator = 1; BlockSizeIterator < l; BlockSizeIterator *= 2)
    {
        for (BlockIterator = 0; BlockIterator < l - BlockSizeIterator; BlockIterator += 2 * BlockSizeIterator)
        {
            //Производим слияние с сортировкой пары блоков начинающуюся с элемента BlockIterator
            //левый размером BlockSizeIterator, правый размером BlockSizeIterator или меньше
            LeftBlockIterator = 0;
            RightBlockIterator = 0;
            LeftBorder = BlockIterator;
            MidBorder = BlockIterator + BlockSizeIterator;
            RightBorder = BlockIterator + 2 * BlockSizeIterator;
            RightBorder = (RightBorder < l) ? RightBorder : l;
            int* SortedBlock = new int[RightBorder - LeftBorder];

            //Пока в обоих массивах есть элементы выбираем меньший из них и заносим в отсортированный блок
            while (LeftBorder + LeftBlockIterator < MidBorder && MidBorder + RightBlockIterator < RightBorder)
            {
                if (a[LeftBorder + LeftBlockIterator] < a[MidBorder + RightBlockIterator])
                {
                    SortedBlock[LeftBlockIterator + RightBlockIterator] = a[LeftBorder + LeftBlockIterator];
                    LeftBlockIterator += 1;
                }
                else
                {
                    SortedBlock[LeftBlockIterator + RightBlockIterator] = a[MidBorder + RightBlockIterator];
                    RightBlockIterator += 1;
                }
            }
            //После этого заносим оставшиеся элементы из левого или правого блока
            while (LeftBorder + LeftBlockIterator < MidBorder)
            {
                SortedBlock[LeftBlockIterator + RightBlockIterator] = a[LeftBorder + LeftBlockIterator];
                LeftBlockIterator += 1;
            }
            while (MidBorder + RightBlockIterator < RightBorder)
            {
                SortedBlock[LeftBlockIterator + RightBlockIterator] = a[MidBorder + RightBlockIterator];
                RightBlockIterator += 1;
            }

            for (MergeIterator = 0; MergeIterator < LeftBlockIterator + RightBlockIterator; MergeIterator++)
            {
                a[LeftBorder + MergeIterator] = SortedBlock[MergeIterator];
            }
            delete SortedBlock;
        }
    }
}

char* ReadFile(const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc((file_size + 1) * sizeof(char));
    int count = fread(buffer, sizeof(char), file_size, file);
    buffer[count] = '\0';
    fclose(file);

    return buffer;
}

void WriteFile(const char* fileName, char* text)
{
    FILE* file = fopen(fileName, "w");
    if (file == NULL) return;

    fwrite(text, sizeof(char), strlen(text), file);
    fclose(file);
}

void SortText(char* text)
{
    for (int i = strlen(text) - 2; i >= 0; i--)
        for (int j = 0; j <= i; j++)
            if (text[j] > text[j + 1])
            {
                char temp = text[j];
                text[j] = text[j + 1];
                text[j + 1] = temp;
            }
}

DWORD WINAPI ThreadProc() {
    WaitForSingleObject(hMutexQueue, INFINITE);
    Task task = queue.PopTusk();
    ReleaseMutex(hMutexQueue);

    SortText(task.text);

    WaitForSingleObject(hMutexCompleted, INFINITE);
    queueCompleted.PushTask(task);
    ReleaseMutex(hMutexCompleted);

    ExitThread(0);
}

int WINAPI WinMain(HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    char* text = ReadFile("./data.txt"); // прочитали текст из файла

    int size;
    if (strlen(text) % THREAD_COUNT == 0) size = strlen(text) / THREAD_COUNT; 
    else size = strlen(text) / THREAD_COUNT + 1; // нашли размер блока для каждого потока
    for (int i = 0; i < THREAD_COUNT; i++) 
    {
        Task task = (*new Task(text, i * size, size)); // создаем задачу
        queue.PushTask(task); // добавляем задачу в очередь задач
    }

    hMutexQueue = CreateMutex(NULL, FALSE, NULL);
    hMutexCompleted = CreateMutex(NULL, FALSE, NULL);

    HANDLE threads[THREAD_COUNT] = {};
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, 0, NULL);
    }
    
    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE); // ожидаем завершения всех потоков

    char* res = (char*)malloc(strlen(text) * sizeof(char));
    res[0] = '\0';
    for (int i = 0; i < THREAD_COUNT; i++)
        strcat(res, queueCompleted.PopTusk().text);

    MergeSort(res, strlen(res));
    WriteFile("./out.txt", res);
    return 0;
}