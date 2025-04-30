typedef struct rwlock RWLock;

RWLock *makeRWLock(void);
void destroyRWLock(RWLock *rwl);
void enterRead(RWLock *rwl);
void exitRead(RWLock *rwl);
void enterWrite(RWLock *rwl);
void exitWrite(RWLock *rwl);
