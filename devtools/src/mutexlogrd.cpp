#include <cstdio>
#include <vector>

int main(int argc, const char*[]) {
    static const char* fileName = "mutexlog.bin";
    static const bool orderByThread = (argc == 1);  // default
    FILE* logFile = fopen(fileName, "rb");
    if (!logFile) {
        printf("Can't open '%s' for reading\n", fileName);
        return 1;
    }
    std::vector<int> threads;
    std::vector<int> mutexes;
    std::vector<bool> locks;
    int operations = 0;
    for (;;) {
        char operation;
        int threadId, mutexId;
        fread(&operation, sizeof(char), 1, logFile);
        fread(&threadId, sizeof(int), 1, logFile);
        fread(&mutexId, sizeof(int), 1, logFile);
        if (feof(logFile))
            break;
        int ti, mi; // indices to vectors, instead of real ids
        for (ti = 0; ti < (int)threads.size(); ++ti)
            if (threadId == threads[ti])
                break;
        if (ti == (int)threads.size())
            threads.push_back(threadId);
        for (mi = 0; mi < (int)mutexes.size(); ++mi)
            if (mutexId == mutexes[mi])
                break;
        if (mi == (int)mutexes.size()) {
            mutexes.push_back(mutexId);
            locks.push_back(false);
        }
        int column, value;
        if (orderByThread) {
            column = ti;
            value = mi;
        }
        else {
            column = mi;
            value = ti;
        }
        for (int i = 0; i < column; ++i)
            printf("  ");
        printf("%2d%c\n", value, operation);
        ++operations;
        if (operation == 'U') {
            if (!locks[mi]) {
                printf("Error: Unlocking something not locked\n");
                break;
            }
            locks[mi] = false;
        }
        if (operation == 'G') {
            if (locks[mi]) {
                printf("Error: Locking something already locked\n");
                break;
            }
            locks[mi] = true;
        }
    }
    for (int i = 0; i < (int)locks.size(); ++i)
        if (locks[i])
            printf("Error: mutex %d left locked\n", i);
    fclose(logFile);
    printf("\nFull thread ids as follows:\n");
    for (int i = 0; i < (int)threads.size(); ++i)
        printf("%2d: %d\n", i, threads[i]);
    printf("\n%d operations (%d lock/unlocks) on %d mutexes\n", operations, operations / 3, (int)mutexes.size());
    return 0;
}
