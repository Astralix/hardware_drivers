#include <dirent.h>

#define MAX_ROLES 20
#define MAX_TABLE_SIZE 30
#define MAX_CONCURRENT_INSTANCES 4

typedef struct _ComponentTable {
    OMX_STRING name;
    OMX_U16 nRoles;
    OMX_STRING pRoleArray[MAX_ROLES];
    OMX_HANDLETYPE* pHandle[MAX_CONCURRENT_INSTANCES];
    int refCount;
}ComponentTable;

OMX_ERRORTYPE hantro_OMX_BuildComponentTable();
