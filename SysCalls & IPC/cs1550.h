
/* TYPEDEFS FOR SEMAPHORE STRUCT and NODE STRUCT */
typedef struct{
    struct task_struct *task;
    struct cs1550_node *next;
}cs1550_node;

typedef struct{ 
    int value; 
    cs1550_node *list;
}cs1550_sem;

/* Definition for spin lock */
DEFINE_SPINLOCK(sem_lock);

