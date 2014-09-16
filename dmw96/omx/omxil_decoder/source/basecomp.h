/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  Basecomponent contains code for component thread handling
--                 and message sending.
--
------------------------------------------------------------------------------*/



#ifndef HANTRO_BASECOMP_H
#define HANTRO_BASECOMP_H

#include "msgque.h"
#include "OSAL.h"
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_Image.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct BASECOMP
{
    OMX_HANDLETYPE    thread;
    msgque            queue;

} BASECOMP;

typedef enum CMD_TYPE
{
    CMD_SEND_COMMAND,
    CMD_EXIT_LOOP

} CMD_TYPE;

typedef struct SEND_COMMAND_ARGS
{
    OMX_COMMANDTYPE cmd;
    OMX_U32         param1;
    OMX_PTR         data;
    OMX_ERRORTYPE (*fun)(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);
} SEND_COMMAND_ARGS ;


typedef struct CMD
{
    CMD_TYPE          type;
    SEND_COMMAND_ARGS arg;
}CMD;


typedef OMX_U32 (*thread_main_fun)(BASECOMP*, OMX_PTR);

OMX_ERRORTYPE HantroOmx_basecomp_init(BASECOMP* b, thread_main_fun f, OMX_PTR arg);
OMX_ERRORTYPE HantroOmx_basecomp_destroy(BASECOMP* b);
OMX_ERRORTYPE HantroOmx_basecomp_send_command(BASECOMP* b, CMD* c);
OMX_ERRORTYPE HantroOmx_basecomp_recv_command(BASECOMP* b, CMD* c);
OMX_ERRORTYPE HantroOmx_basecomp_try_recv_command(BASECOMP* b, CMD* c, OMX_BOOL* ok);

void HantroOmx_generate_uuid(OMX_HANDLETYPE comp, OMX_UUIDTYPE* uuid);


OMX_ERRORTYPE HantroOmx_cmd_dispatch(CMD* cmd, OMX_PTR arg);


#define CHECK_PARAM_NON_NULL(param)  \
  if ((param) == 0)                  \
      return OMX_ErrorBadParameter


#define CHECK_STATE_INVALID(state) \
  if ((state) == OMX_StateInvalid) \
      return OMX_ErrorInvalidState

#define CHECK_STATE_IDLE(state) \
  if ((state) == OMX_StateIdle) \
      return OMX_ErrorIncorrectStateOperation

#define CHECK_STATE_LOADED(state) \
  if ((state) != OMX_StateLoaded) \
      return OMX_ErrorIncorrectStateOperation

#define CHECK_PARAM_VERSION(param) \
  if ((param).nVersion.s.nVersionMajor != 1 || \
      (param).nVersion.s.nVersionMinor != 1) \
      return OMX_ErrorVersionMismatch;

#define INIT_OMX_VERSION_PARAM(param)   \
  (param).nSize = sizeof(param);        \
  (param).nVersion.s.nVersionMajor = 1; \
  (param).nVersion.s.nVersionMinor = 1; \
  (param).nVersion.s.nRevision     = 2; \
  (param).nVersion.s.nStep         = 0

#define INIT_EXIT_CMD(cmd)        \
  memset((&cmd), 0, sizeof(CMD)); \
  cmd.type = CMD_EXIT_LOOP


#define INIT_SEND_CMD(cmd_, omxc, param, data_, fun_)   \
  memset(&cmd_, 0, sizeof(CMD));                        \
  cmd_.type         = CMD_SEND_COMMAND;                 \
  cmd_.arg.cmd    = omxc;                               \
  cmd_.arg.param1 = param;                              \
  cmd_.arg.data   = data_;                              \
  cmd_.arg.fun    = fun_


#define DECLARE_STUB(x) static OMX_ERRORTYPE x(OMX_HANDLETYPE,   ...)
#define DEFINE_STUB(x)  static OMX_ERRORTYPE x(OMX_HANDLETYPE h, ...) { return OMX_ErrorNotImplemented; }

#ifdef __cplusplus
}
#endif
#endif // HANTRO_BASECOMP_H

