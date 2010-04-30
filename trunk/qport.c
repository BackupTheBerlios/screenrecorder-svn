/*
 *  $Id: png.c,v 1.4 2008/07/10 15:41:57 itix Exp $
 */

#include <proto/exec.h>

void CreateQPort(struct MsgPort *port)
{
	port->mp_Node.ln_Type = NT_MSGPORT;
	port->mp_Flags        = PA_SIGNAL;
	if ((BYTE) (port->mp_SigBit = AllocSignal(-1)) == -1)
	{
		port->mp_SigBit = SIGB_SINGLE;
		SetSignal(0, SIGF_SINGLE);
	}
	port->mp_SigTask      = FindTask(NULL);
	NEWLIST(&port->mp_MsgList);
}

void DeleteQPort(struct MsgPort *port)
{
	if (port->mp_SigTask)
	{
		if (port->mp_SigBit == SIGB_SINGLE)
			SetSignal(0, SIGF_SINGLE);
		else
			FreeSignal(port->mp_SigBit);
	}
}
