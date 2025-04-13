#ifndef __OS_TOPIC_H__
#define __OS_TOPIC_H__

#include "OS_Obj.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/

typedef struct os_topic_{
	os_obj_t  obj;				//Base object (must be first member)
    void*     msgQlist;
} os_topic_t;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Topic create
 *
 * @brief Creates a new topic
 *
 * @param os_handle_t* h : [out] Topic handle
 * @param char* name     : [ in] Topic name or NULL to create an nameless topic
 *
 * @return os_err_e error code (0 = OK)
 **********************************************************************/
os_err_e os_topic_create(os_handle_t* h, char const * name);


/***********************************************************************
 * OS Topic Subscribe
 *
 * @brief Subscribe a task to a topic
 *
 * @param os_handle_t topic : [in] topic to subscribe to
 * 
 * @return os_err_e error code (0 = OK)
 **********************************************************************/
os_err_e os_topic_subscribe(os_handle_t topic);


/***********************************************************************
 * OS Topic Unsubscribe Task
 *
 * @brief Unsubscribe a task from a topic
 *
 * @param os_handle_t topic : [in] topic to unsubscribe from
 * @param os_handle_t task  : [in] task to unsubscribe
 * 
 * @return os_err_e error code (0 = OK)
 **********************************************************************/
os_err_e os_topic_unsubscribeTask(os_handle_t topic, os_handle_t task);


/***********************************************************************
 * OS Topic Receive
 *
 * @brief Receive a message published in the topic
 *
 * @param os_handle_t topic : [ in] Handle to topic
 * @param os_err_e* err     : [out] Error code (or null to ignore)
 * 
 * @return void* : message or NULL if nothing
 **********************************************************************/
void* os_topic_receive(os_handle_t topic, os_err_e* err);


/***********************************************************************
 * OS Topic Publish
 *
 * @brief Publises a message to a topic
 *
 * @param os_handle_t topic : [ in] Handle to topic
 * @param void* msg         : [ in] Message to send
 * 
 * @return os_err_e : Error code
 **********************************************************************/
os_err_e os_topic_publish(os_handle_t topic, void* msg);


/***********************************************************************
 * OS Topic Delete
 *
 * @brief Deles a topic and all contained messages
 *
 * @param os_handle_t topic : [ in] Handle to topic
 * 
 * @return os_err_e : Error code
 **********************************************************************/
os_err_e os_topic_delete(os_handle_t topic);

/**********************************************
 * PUBLIC INLINE FUNCTIONS
 *********************************************/

/***********************************************************************
 * OS Topic 
 *
 * @brief Unsubscribe current task from a topic
 *
 * @param os_handle_t topic : [in] topic to unsubscribe from
 * 
 * @return os_err_e error code (0 = OK)
 **********************************************************************/
static inline os_err_e os_topic_unsubscribe(os_handle_t topic){
    return os_topic_unsubscribeTask(topic, (void*)os_task_getCurrentTask());
}

#endif //__OS_TOPIC_H__