
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <OMX_Core.h>
#include <st_static_component_loader.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "DSPG_OMX_Core"
#include <utils/Log.h>

#define USE_MUTEX

OMX_ERRORTYPE DSPGOMX_InitComponentLoader();

int count;
static BOSA_COMPONENTLOADER *loader;

#ifdef USE_MUTEX
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/******************************Public*Routine******************************\
* OMX_Init()
*
* Description:This method will initialize the OMX Core.  It is the 
* responsibility of the application to call OMX_Init to ensure the proper
* set up of core resources.
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE DSPGOMX_Init()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	LOGV("[%s]: start\n", __func__);

#ifdef USE_MUTEX
    if(pthread_mutex_lock(&mutex) != 0)
    {
        LOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

    count++;
    LOGV("init count = %d\n", count);

    if (count == 1)
    {
		ret = DSPGOMX_InitComponentLoader();
    }

#ifdef USE_MUTEX
    if(pthread_mutex_unlock(&mutex) != 0)
    {
        LOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

    return ret;
}
/******************************Public*Routine******************************\
* OMX_GetHandle
*
* Description: This method will create the handle of the COMPONENTTYPE
* If the component is currently loaded, this method will reutrn the 
* hadle of existingcomponent or create a new instance of the component.
* It will call the OMX_ComponentInit function and then the setcallback
* method to initialize the callback functions
* Parameters:
* @param[out] pHandle            Handle of the loaded components 
* @param[in] cComponentName     Name of the component to load
* @param[in] pAppData           Used to identify the callbacks of component 
* @param[in] pCallBacks         Application callbacks
*
* @retval OMX_ErrorUndefined         
* @retval OMX_ErrorInvalidComponentName
* @retval OMX_ErrorInvalidComponent
* @retval OMX_ErrorInsufficientResources 
* @retval OMX_NOERROR                      Successful
*
* Note
*
\**************************************************************************/

OMX_ERRORTYPE DSPGOMX_GetHandle( OMX_HANDLETYPE* pHandle, OMX_STRING cComponentName,
    OMX_PTR pAppData, OMX_CALLBACKTYPE* pCallBacks)
{
	OMX_ERRORTYPE ret;
	LOGV("[%s]: start\n", __func__);

#ifdef USE_MUTEX
	if(pthread_mutex_lock(&mutex) != 0)
	{
		LOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
		return OMX_ErrorUndefined;
	}
#endif

	ret = loader->BOSA_CreateComponent(loader, pHandle, cComponentName, pAppData, pCallBacks);

#ifdef USE_MUTEX
	if(pthread_mutex_unlock(&mutex) != 0)
	{
		LOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
		return OMX_ErrorUndefined;
	}
#endif

    return ret;
}


/******************************Public*Routine******************************\
* OMX_FreeHandle()
*
* Description:This method will unload the OMX component pointed by 
* OMX_HANDLETYPE. It is the responsibility of the calling method to ensure that
* the Deinit method of the component has been called prior to unloading component
*
* Parameters:
* @param[in] hComponent the component to unload
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE DSPGOMX_FreeHandle (OMX_HANDLETYPE hComponent)
{

    OMX_ERRORTYPE ret;
	LOGV("[%s]: start\n", __func__);

#ifdef USE_MUTEX
    if(pthread_mutex_lock(&mutex) != 0)
    {
        LOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

	ret = loader->BOSA_DestroyComponent(loader, hComponent);

#ifdef USE_MUTEX
    if(pthread_mutex_unlock(&mutex) != 0)
    {
        LOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

    return ret;
}

/******************************Public*Routine******************************\
* OMX_DeInit()
*
* Description:This method will release the resources of the OMX Core.  It is the 
* responsibility of the application to call OMX_DeInit to ensure the clean up of these
* resources.
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE DSPGOMX_Deinit()
{
	OMX_ERRORTYPE ret;
	LOGV("[%s]: start\n", __func__);

#ifdef USE_MUTEX
    if(pthread_mutex_lock(&mutex) != 0) {
        LOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

    if (count) {
        count--;
    }

    LOGV("deinit count = %d\n", count);

	ret = loader->BOSA_DeInitComponentLoader(loader);

#ifdef USE_MUTEX
    if(pthread_mutex_unlock(&mutex) != 0) {
        LOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
#endif

    return ret;
}

/*************************************************************************
* OMX_SetupTunnel()
*
* Description: Setup the specified tunnel the two components
*
* Parameters:
* @param[in] hOutput     Handle of the component to be accessed
* @param[in] nPortOutput Source port used in the tunnel
* @param[in] hInput      Component to setup the tunnel with.
* @param[in] nPortInput  Destination port used in the tunnel
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
/* OMX_SetupTunnel */
OMX_API OMX_ERRORTYPE OMX_APIENTRY DSPGOMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput)
{
    OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;
	LOGV("[%s]: start\n", __func__);
/*
    OMX_COMPONENTTYPE *pCompIn, *pCompOut;
    OMX_TUNNELSETUPTYPE oTunnelSetup;

    if (hOutput == NULL && hInput == NULL)
        return OMX_ErrorBadParameter;

    oTunnelSetup.nTunnelFlags = 0;
    oTunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

    pCompOut = (OMX_COMPONENTTYPE*)hOutput;

    if (hOutput)
    {
        eError = pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, hInput, nPortInput, &oTunnelSetup);
    }


    if (eError == OMX_ErrorNone && hInput) 
    {  
        pCompIn = (OMX_COMPONENTTYPE*)hInput;
        eError = pCompIn->ComponentTunnelRequest(hInput, nPortInput, hOutput, nPortOutput, &oTunnelSetup);
        if (eError != OMX_ErrorNone && hOutput) 
        {
            // cancel tunnel request on output port since input port failed
            pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, NULL, 0, NULL);
        }
    }
  */
    return eError;
}

/*************************************************************************
* OMX_ComponentNameEnum()
*
* Description: This method will provide the name of the component at the given nIndex
*
*Parameters:
* @param[out] cComponentName       The name of the component at nIndex
* @param[in] nNameLength                The length of the component name
* @param[in] nIndex                         The index number of the component 
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE OMX_APIENTRY DSPGOMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	LOGV("[%s]: start\n", __func__);

	ret = loader->BOSA_ComponentNameEnum(loader, cComponentName, nNameLength, nIndex);
    
    return ret;
}


/*************************************************************************
* OMX_GetRolesOfComponent()
*
* Description: This method will query the component for its supported roles
*
*Parameters:
* @param[in] cComponentName     The name of the component to query
* @param[in] pNumRoles     The number of roles supported by the component
* @param[in] roles		The roles of the component
*
* Returns:    OMX_NOERROR          Successful
*                 OMX_ErrorBadParameter		Faliure due to a bad input parameter
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE DSPGOMX_GetRolesOfComponent (
    OMX_IN      OMX_STRING cComponentName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles)
{

    OMX_ERRORTYPE ret;
	LOGV("[%s]: start\n", __func__);

	ret = loader->BOSA_GetRolesOfComponent(loader, cComponentName, pNumRoles, roles);

	return ret;
}

/*************************************************************************
* OMX_GetComponentsOfRole()
*
* Description: This method will query the component for its supported roles
*
*Parameters:
* @param[in] role     The role name to query for
* @param[in] pNumComps     The number of components supporting the given role
* @param[in] compNames      The names of the components supporting the given role
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole ( 
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames)
{
    OMX_ERRORTYPE ret;
	LOGV("[%s]: start\n", __func__);

	ret = loader->BOSA_GetComponentsOfRole(loader, role, pNumComps, compNames);

	return ret;
}


OMX_ERRORTYPE DSPGOMX_InitComponentLoader()
{
	LOGV("[%s]: start\n", __func__);

	loader = calloc(1, sizeof(BOSA_COMPONENTLOADER));
	if (loader == NULL) {
			LOGE("not enough memory for this loader\n");
			return OMX_ErrorInsufficientResources;
	}

	st_static_setup_component_loader(loader);

	LOGV("[%s]: OK\n", __func__);

	return loader->BOSA_InitComponentLoader(loader);
}


