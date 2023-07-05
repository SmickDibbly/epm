#ifndef ENTITY_H
#define ENTITY_H

typedef void (*fn_onTic)(void *entity);
typedef struct epm_EntityNode epm_EntityNode;
struct epm_EntityNode {
    epm_EntityNode *next;
    epm_EntityNode *prev;
    
    void *entity;
    fn_onTic onTic;
};

#endif /* ENTITY_H */
