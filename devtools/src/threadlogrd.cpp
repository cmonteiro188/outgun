#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <cassert>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using std::make_pair;
using std::map;
using std::max;
using std::pair;
using std::sort;
using std::string;
using std::vector;

#define nAssert assert

struct BadLogFileData {
    string condition;
    int line;

    BadLogFileData(string cond, int line_) : condition(cond), line(line_) { }
};

#define dataAssert(condition) ((void)((condition) ? 0 : throw BadLogFileData(#condition, __LINE__)))

typedef uint32_t ThreadId;
typedef uint64_t ObjectId;

template<class Type> Type get(FILE* file) { Type t; fread(&t, sizeof(Type), 1, file); return t; }

string getString(FILE* file) {
    string s;
    char ch;
    for (;;) {
        ch = getc(file);
        dataAssert(ch != EOF);
        if (ch == '\0')
            return s;
        s += ch;
    }
}

enum IdFormat { IDF_Number, IDF_CWNumber, IDF_Verbose };

string utoa(uint32_t number) {
    if (number == 0)
        return "0";
    char buf[14];
    int bi = 13;
    buf[bi] = '\0';
    for (; number; number /= 10)
        buf[--bi] = '0' + number % 10;
    nAssert(bi >= 0);
    return &buf[bi];
}

struct NamedObject {
    int number;
    string strIdentifier;

    NamedObject(int num) : number(num) { }
    virtual ~NamedObject() { }

    string identify(IdFormat f) const {
        const string num = utoa(number);
        switch (f) {
         /*break;*/case IDF_Number:   return type() + num;
            break; case IDF_CWNumber: return type() + string(3 - num.length(), ' ') + num;
            break; case IDF_Verbose:  return type() + num + '(' + (strIdentifier.empty() ? "?" : strIdentifier) + ')';
            break; default: nAssert(0);
        }
        return strIdentifier.empty() ? "???" : strIdentifier;
    }
    virtual char type() const = 0;
};

class Mutex;
class CondVar;

enum GetNewIndicator { GetNew }; // used in constructors so that default construction doesn't allocate numbers (or isn't allowed at all)

struct Thread : public NamedObject {
    Mutex* waitingFor;
    CondVar* waitCondition;
    int nLockedMutexes;
    Thread* waitJoinee;
    enum { Unknown, RunningDetached, RunningUndetached, RunningUnknown, WaitingToBeJoined, Cleaned } status;
    enum { InSync, RunningSeen, CreateSeen } startSyncStatus;
    int waitStartTime; // only if waitingFor something, status == WaitingToBeJoined, or waitJoinee

    static int newNumber() { static int n = 0; return n++; }
    Thread(GetNewIndicator) : NamedObject(newNumber()), waitingFor(0), waitCondition(0), nLockedMutexes(0), waitJoinee(0), status(Unknown), startSyncStatus(InSync) { }

    char type() const { return 'T'; }
};

struct SyncObject : public NamedObject {
    bool created, deleted;

    SyncObject(int num) : NamedObject(num), created(false), deleted(false) { }
    virtual ~SyncObject() { }

    virtual bool inUse() const = 0;
};

struct Mutex : public SyncObject {
    bool locked;
    Thread* owner; // only if locked
    int lockStartTime; // only if locked
    int nWaiters;

    static int newNumber() { static int n = 0; return n++; }
    Mutex(GetNewIndicator) : SyncObject(newNumber()), locked(false), nWaiters(0) { }

    bool inUse() const { return locked || nWaiters != 0; }
    char type() const { return 'M'; }
};

struct CondVar : public SyncObject {
    int nWaiting;

    static int newNumber() { static int n = 0; return n++; }
    CondVar(GetNewIndicator) : SyncObject(newNumber()), nWaiting(0) { }

    bool inUse() const { return nWaiting != 0; }
    char type() const { return 'C'; }
};

class SyncStatus {
    map<ThreadId, Thread*> threadsById;
    map<ObjectId, Mutex*> mutexesById;
    map<ObjectId, CondVar*> condVarsById;

    vector<Thread*> threads;
    vector<Mutex*> mutexes;
    vector<CondVar*> condVars;

    int prevTn;
    int nOperations, mutexLocks, condWaits, threadOps;

    template<class IdType, class Type> Type& getObject(IdType id, map<IdType, Type*>& idMap, vector<Type*>& objects) {
        typename map<IdType, Type*>::const_iterator oi = idMap.find(id);
        if (oi != idMap.end())
            return *oi->second;
        objects.push_back(new Type(GetNew));
        idMap[id] = objects.back();
        return *objects.back();
    }
    Thread&  getThread (ThreadId id) { return getObject(id,  threadsById, threads ); }
    Mutex&   getMutex  (ObjectId id) { return getObject(id,  mutexesById, mutexes ); }
    CondVar& getCondVar(ObjectId id) { return getObject(id, condVarsById, condVars); }

    void checkThreadStart(Thread& t);
    bool threadOperation(Thread& t, Thread& tgt, char operation, FILE* logFile);
    bool mutexOperation(Thread& t, Mutex& m, char operation);
    bool condVarOperation(Thread& t, CondVar& c, char operation);

public:
    SyncStatus() : prevTn(0), nOperations(0), mutexLocks(0), condWaits(0), threadOps(0) { }
    ~SyncStatus() { } // for more complex programs, we should free threads, mutexes and condVars
    bool readLog(FILE* logFile, bool indent, int verbosity, int roundLimit = 0);
    void printStatus(bool showAll, bool verbose);
};

void SyncStatus::checkThreadStart(Thread& t) {
    if (t.status == Thread::Unknown)
        return;
    dataAssert(t.status == Thread::Cleaned);
    // archive a copy of the old instance and make the active one fresh with new id
    threads.push_back(new Thread(t));
    t = Thread(GetNew);
    printf(" - reused; new id: %d", t.number);
}

bool SyncStatus::threadOperation(Thread& t, Thread& tgt, char operation, FILE* logFile) {
    ++threadOps;
    switch (operation) {
    /*break;*/case 'C': {
        const char detachState = get<char>(logFile);
        dataAssert(detachState == 'D' || detachState == 'J');
        if (tgt.startSyncStatus == Thread::InSync) {
            checkThreadStart(tgt);
            tgt.status = detachState == 'D' ? Thread::RunningDetached : Thread::RunningUndetached;
            tgt.startSyncStatus = Thread::CreateSeen;
        }
        else {
            dataAssert(tgt.startSyncStatus == Thread::RunningSeen);
            /* Possibilities in the thread execution path (with corresponding status) are as follows:
             *   R    RunningUnknown
             *  (RD   RunningDetached)
             *  (RDE  Cleaned)
             *   RE   WaitingToBeJoined
             * ((RE<joined> Cleaned))
             * ((RED  Cleaned))
             *
             * Actually the ones in (parentheses) shouldn't happen with the current Thread class because access to the Thread object is required,
             * and the program is failing on synchronization if its accessing it while one thread is in its doStart (producing the C operation).
             * The ones in ((double parentheses)) shouldn't happen even if the thread was able to detach itself, since they require outside cooperation.
             */
            switch (tgt.status) {
            /*break;*/ case Thread::RunningUnknown:    tgt.status = detachState == 'D' ? Thread::RunningDetached : Thread::RunningUndetached;
                break; case Thread::RunningDetached:   dataAssert(detachState == 'J');
                break; case Thread::WaitingToBeJoined: if (detachState == 'D') tgt.status = Thread::Cleaned;
                break; case Thread::Cleaned:
                break; default: dataAssert(0);
            }
            tgt.startSyncStatus = Thread::InSync;
        }
        printf(" - P");
        // continue to 'P' because priority is in 'C' too
     }
    /*no break*/case 'P': {
        const string priority = getString(logFile);
        printf(" = %s", priority.c_str());
     }
     break; case 'D': case 'd':
        if (tgt.status == Thread::WaitingToBeJoined)
            tgt.status = Thread::Cleaned;
        else if (tgt.status == Thread::RunningUndetached || tgt.status == Thread::RunningUnknown)
            tgt.status = Thread::RunningDetached;
        else
            dataAssert(0);
     break; case 'J':
        dataAssert(tgt.status == Thread::RunningUndetached || tgt.status == Thread::RunningUnknown || tgt.status == Thread::WaitingToBeJoined);
        t.waitJoinee = &tgt;
        t.waitStartTime = nOperations;
     break; case 'j':
        dataAssert(t.waitJoinee == &tgt && tgt.status == Thread::WaitingToBeJoined);
        t.waitJoinee = 0;
        tgt.status = Thread::Cleaned;
     break; case 'R':
        dataAssert(&t == &tgt);
        dataAssert(tgt.status != Thread::WaitingToBeJoined);
        if (tgt.startSyncStatus == Thread::RunningSeen) {
            dataAssert(tgt.status == Thread::Cleaned);
            printf("\nCan't handle: thread started, exited, joined (or was detached), and new one created with same ID before the first pthread_create returning\n");
            return false;
        }
        if (tgt.startSyncStatus == Thread::InSync) {
            checkThreadStart(tgt);
            tgt.status = Thread::RunningUnknown;
            tgt.startSyncStatus = Thread::RunningSeen;
        }
        else {
            dataAssert(tgt.startSyncStatus == Thread::CreateSeen);
            tgt.startSyncStatus = Thread::InSync;
        }
     break; case 'E':
        dataAssert(&t == &tgt);
        dataAssert(tgt.status == Thread::RunningUndetached || tgt.status == Thread::RunningDetached || tgt.status == Thread::RunningUnknown);
        dataAssert(!tgt.waitingFor && !tgt.waitCondition && !tgt.waitJoinee);
        if (tgt.nLockedMutexes != 0) {
            printf("\nError: thread exiting with locks\n");
            return false;
        }
        tgt.status = tgt.status == Thread::RunningDetached ? Thread::Cleaned : Thread::WaitingToBeJoined;
        tgt.waitStartTime = nOperations;
     break; default:
        dataAssert(0);
    }
    return true;
}

bool SyncStatus::mutexOperation(Thread& t, Mutex& m, char operation) {
    switch (operation) {
    /*break;*/case 'L':
        dataAssert(!t.waitingFor && !t.waitCondition);
        if (m.locked && m.owner == &t) {
            printf("\nError: Locking something already locked by the same thread\n");
            return false;
        }
        ++m.nWaiters;
        t.waitingFor = &m;
        t.waitCondition = 0;
        t.waitStartTime = nOperations;
     break; case 'U': case 'W':
        dataAssert(!t.waitingFor && !t.waitCondition);
        if (!m.locked) {
            printf("\nError: Unlocking something not locked\n");
            return false;
        }
        if (m.owner != &t) {
            printf("\nError: Unlocking something owned by another thread\n");
            return false;
        }
        if (operation == 'W' && t.nLockedMutexes > 1) {
            printf("\nUnexpected: Waiting for signal while holding a different mutex\n");
            return false;
        }
        m.locked = false;
        nAssert(t.nLockedMutexes > 0);
        --t.nLockedMutexes;
        if (operation == 'W') {
            t.waitingFor = &m;
            t.waitCondition = 0; // will be updated on next operation if the log file isn't corrupt
            t.waitStartTime = nOperations;
        }
     break; case 'G': case 'w':
        dataAssert(!m.locked);
        dataAssert(t.waitingFor == &m && !t.waitCondition); // waitCondition because cvar 'w' comes first
        if (operation == 'G') {
            nAssert(m.nWaiters > 0);
            --m.nWaiters;
        }
        m.locked = true;
        m.owner = &t;
        m.lockStartTime = nOperations;
        t.waitingFor = 0;
        ++t.nLockedMutexes;
        ++mutexLocks;
     break; default:
        dataAssert(0);
    }
    return true;
}

bool SyncStatus::condVarOperation(Thread& t, CondVar& c, char operation) {
    switch (operation) {
    /*break;*/case 'W':
        dataAssert(!t.waitCondition && !t.waitJoinee);
        if (t.nLockedMutexes != 0) {
            printf("Unexpected: Waiting for condition while holding a different mutex\n");
            return false;
        }
        ++c.nWaiting;
        t.waitCondition = &c;
     break; case 'w':
        dataAssert(t.waitCondition == &c && !t.waitJoinee);
        t.waitCondition = 0;
        --c.nWaiting;
        ++condWaits;
     break; case 'S':
     break; case 'B':
     break; default:
        dataAssert(0);
    }
    return true;
}

bool SyncStatus::readLog(FILE* logFile, bool indent, int verbosity, int roundLimit) {
    for (int round = 0;; ++round, ++nOperations) {
        if (round == roundLimit && roundLimit)
            return true;

        const ThreadId threadId = get<ThreadId>(logFile);
        if (feof(logFile))
            return true;

        Thread& t = getThread(threadId);
        if (indent) {
            for (int i = 0; i < t.number; ++i)
                printf("  ");
        }
        else if (t.number != prevTn) {
            printf("---\n");
            prevTn = t.number;
        }
        printf("%s:", t.identify(verbosity >= 2 ? IDF_Verbose : IDF_Number).c_str());

        const char objectType = get<char>(logFile);

        NamedObject* base = 0;
        SyncObject* objBase = 0;
        Thread* ot = 0;
        Mutex* om = 0;
        CondVar* oc = 0;
        if      (objectType == 'T')
            base =           ot = &getThread (get<ThreadId>(logFile));
        else if (objectType == 'M')
            base = objBase = om = &getMutex  (get<ObjectId>(logFile));
        else if (objectType == 'C')
            base = objBase = oc = &getCondVar(get<ObjectId>(logFile));
        else
            dataAssert(0);

        const char operation = get<char>(logFile);
        dataAssert(!feof(logFile));

        NamedObject& b = *base;

        printf("%s%c", b.identify(verbosity >= 1 ? IDF_Verbose : IDF_CWNumber).c_str(), operation);
        if (operation == 'I') {
            b.strIdentifier = getString(logFile);
            printf(" = %s\n", b.strIdentifier.c_str());
            continue;
        }

        if (objectType == 'T') {
            if (!threadOperation(t, *ot, operation, logFile))
                return false;
        }
        else {
            nAssert(objBase);
            SyncObject& o = *objBase;
            if (operation == 'C') {
                if (o.created) {
                    dataAssert(o.deleted);
                    // archive a copy of the old instance and make the active one fresh with new id
                    if (objectType == 'M') {
                        mutexes.push_back(new Mutex(*om));
                        *om = Mutex(GetNew);
                    }
                    else {
                        condVars.push_back(new CondVar(*oc));
                        *oc = CondVar(GetNew);
                    }
                    printf(" - reused; new id: %d", o.number);
                }
                o.created = true;
                o.deleted = false;
            }
            else if (!o.created || o.deleted)
                dataAssert(0);
            else if (operation == 'D') {
                if (o.inUse()) {
                    printf("\nError: Deleting an object in use\n");
                    return false;
                }
                o.deleted = true;
            }
            else if (objectType == 'M') {
                if (!mutexOperation(t, *om, operation))
                    return false;
            }
            else {
                if (!condVarOperation(t, *oc, operation))
                    return false;
            }
        }
        printf("\n");
    }
}

bool sortByNumber(const NamedObject* o1, const NamedObject* o2) { return o1->number < o2->number; }

void SyncStatus::printStatus(bool showAll, bool verbose) {
    sort( threads.begin(),  threads.end(), sortByNumber);
    sort( mutexes.begin(),  mutexes.end(), sortByNumber);
    sort(condVars.begin(), condVars.end(), sortByNumber);

    int maxIdentityLength = 0;
    for (int i = 0; i < (int)threads.size(); ++i)
        maxIdentityLength = max(maxIdentityLength, (int)threads[i]->identify(IDF_Verbose).length());
    for (int i = 0; i < (int)mutexes.size(); ++i)
        maxIdentityLength = max(maxIdentityLength, (int)mutexes[i]->identify(IDF_Verbose).length());
    for (int i = 0; i < (int)condVars.size(); ++i)
        maxIdentityLength = max(maxIdentityLength, (int)condVars[i]->identify(IDF_Verbose).length());

    const IdFormat refFormat = verbose ? IDF_Verbose : IDF_Number;

    printf("\nMutexes:\n");
    for (vector<Mutex*>::const_iterator mi = mutexes.begin(); mi != mutexes.end(); ++mi) {
        const Mutex& m = **mi;
        if (m.nWaiters == 0 && !m.locked && !showAll)
            continue;
        printf("%-*s", maxIdentityLength, m.identify(IDF_Verbose).c_str());
        if (m.deleted)
            printf(" (deleted)");
        else {
            if (m.nWaiters != 0)
                printf(" - %d threads waiting", m.nWaiters);
            if (m.locked)
                printf(" - locked by %s since %d ops ago", m.owner->identify(refFormat).c_str(), nOperations - m.lockStartTime);
        }
        printf("\n");
    }

    printf("\nCondition variables:\n");
    for (vector<CondVar*>::const_iterator ci = condVars.begin(); ci != condVars.end(); ++ci) {
        const CondVar& c = **ci;
        if (c.nWaiting == 0 && !showAll)
            continue;
        printf("%-*s", maxIdentityLength, c.identify(IDF_Verbose).c_str());
        if (c.deleted)
            printf(" (deleted)");
        else if (c.nWaiting)
            printf(" - %d threads waiting", c.nWaiting);
        printf("\n");
    }

    printf("\nThreads:\n");
    for (vector<Thread*>::const_iterator ti = threads.begin(); ti != threads.end(); ++ti) {
        const Thread& t = **ti;
        const bool hasInfo = t.waitingFor || t.waitCondition || t.nLockedMutexes != 0 || t.waitJoinee || t.status == Thread::WaitingToBeJoined;
        if (!hasInfo && !showAll)
            continue;
        printf("%-*s", maxIdentityLength, t.identify(IDF_Verbose).c_str());
        switch (t.status) {
         /*break;*/case Thread::Unknown:           printf(" (unknown)");
            break; case Thread::RunningDetached:   printf(" (detached)");
            break; case Thread::RunningUndetached:
            break; case Thread::RunningUnknown:    printf(" (C not seen)");
            break; case Thread::WaitingToBeJoined: // printed last
            break; case Thread::Cleaned:           printf(" (cleaned)");
            break; default: nAssert(0);
        }
        if (hasInfo) {
            if (t.nLockedMutexes) {
                printf(" - %d locked mutexes (", t.nLockedMutexes);
                bool first = true;
                for (int mi = 0; mi < (int)mutexes.size(); ++mi) {
                    const Mutex& m = *mutexes[mi];
                    if (m.locked && m.owner == &t) {
                        if (first)
                            first = false;
                        else
                            printf(", ");
                        printf("%s:%d", m.identify(refFormat).c_str(), nOperations - m.lockStartTime);
                    }
                }
                printf(")");
            }
            if (t.waitingFor || t.waitCondition) {
                printf(" - waiting for %s since %d ops ago",
                       (t.waitCondition ? t.waitCondition->identify(refFormat) : t.waitingFor->identify(refFormat)).c_str(),
                       nOperations - t.waitStartTime);
            }
            if (t.waitJoinee)
                printf(" - waiting to join %s since %d ops ago", t.waitJoinee->identify(refFormat).c_str(), nOperations - t.waitStartTime);
            if (t.status == Thread::WaitingToBeJoined)
                printf(" - waiting to be joined since %d ops ago", nOperations - t.waitStartTime);
        }
        printf("\n");
    }
    printf("\n%d operations (%d thread operations, %d mutex locks, %d condition wakeups) on %d threads, %d mutexes, %d condition variables\n",
           nOperations, threadOps, mutexLocks, condWaits,
           (int)threads.size(), (int)mutexes.size(), (int)condVars.size());
}

int main(int argc, const char* argv[]) {
    string fileName;
    bool showAll = false, indent = false;
    int verbosity = 0, roundLimit = 0;
    for (int argi = 1; argi < argc; ++argi) {
        if (argv[argi][0] == '-') {
            if (!strcmp(argv[argi], "-a"))
                showAll = true;
            else if (!strcmp(argv[argi], "-i"))
                indent = true;
            else if (!strcmp(argv[argi], "-v"))
                ++verbosity;
            else if (!strcmp(argv[argi], "-s") && argi + 1 < argc) {
                ++argi;
                roundLimit = atoi(argv[argi]);
            }
        }
        else if (fileName.empty())
            fileName = argv[argi];
        else {
            printf("syntax: %s [-a] [-i] [-v ...] [-s <operation-count>] threadlog.bin\n"
                   "-a     print information on all known objects, not just \"interesting\" ones\n"
                   "-i     indent lines based on the thread id\n"
                   "-v     be verbose (use -v -v etc. for more verbosity)\n"
                   "-s N   pretend the file only contains the first N operations\n",
                   argv[0]);
            return 1;
        }
    }
    if (fileName.empty())
        fileName = "threadlog.bin";
    FILE* logFile = fopen(fileName.c_str(), "rb");
    if (!logFile) {
        printf("Can't open '%s' for reading\n", fileName.c_str());
        return 1;
    }
    SyncStatus s;
    try {
        s.readLog(logFile, indent, verbosity - 1, roundLimit);
    } catch (BadLogFileData& e) {
        printf("\nInvalid data in '%s' (read position at %lX): '%s' failed on line %d\n", fileName.c_str(), ftell(logFile), e.condition.c_str(), e.line);
        return 1;
    }
    fclose(logFile);
    s.printStatus(showAll, verbosity >= 1);
    return 0;
}
