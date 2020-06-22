/****************************************************************************
Copyright (c) 2004 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    bluestack_handler.h
    
DESCRIPTION

*/

#ifndef    CONNECTION_BLUESTACK_HANDLER_H_
#define    CONNECTION_BLUESTACK_HANDLER_H_


/****************************************************************************
NAME    
    connectionBluestackHandler    

DESCRIPTION
    Connection task handler for incoming Bluestack primitives

RETURNS
    void    
*/
void connectionBluestackHandler(Task task, MessageId id, Message message);

#endif    /* CONNECTION_BLUESTACK_HANDLER_H_ */
