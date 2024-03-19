#include "event.h"
#include "stdio.h"
#include "stdlib.h"
unsigned int dipatcher_id;
Dispatcher *create_dipatcher()
{
    Dispatcher* dp = malloc(sizeof(Dispatcher));
    dp->list = NULL;
    return dp;
}
Listener *add_event_listener(Dispatcher *dp,int type,void(* fun)(int i))
{
    ListenerList *list=malloc(sizeof(ListenerList));
    list->listener=malloc(sizeof(Listener));
    list->listener->type=type;
    list->listener->update=fun;
    list->prior=NULL;
    list->next=NULL;
    if (dp->list==NULL)
    {
        dp->list = list;
        return list->listener;
    }
    ListenerList* tmp = dp->list;
    tmp = tmp->next;
    printf("%s\n", tmp ? "not" : "null");
    while (tmp)
    {
        printf("tmp");
        if (tmp->next == NULL)
        {
            tmp->next=list;
            list->prior = tmp;
            break;;
        }
        tmp=tmp->next;
    }
    return list->listener;
}
void remove_event_listener(Dispatcher *dp, Listener *lt)
{
    ListenerList *tmp=dp->list;
    while (tmp)
    {
        if (tmp->listener == lt)
        {
            if (tmp->prior==NULL)
            {
                dp->list=tmp->next;
                break;;
            }
            if (tmp->next==NULL)
            {
                tmp->prior->next=NULL;
                break;;
            }
            else{
            tmp->prior->next = tmp->next;
            tmp->next->prior = tmp->prior;
            }
        }
        tmp=tmp->next;
    }
    free(lt);
}

void dispatch(Dispatcher *dp,Event *event)
{
    ListenerList *tmp=dp->list;
    while (tmp)
    {
        if (tmp->listener->type==event->type)
        {
            tmp->listener->update(event->data);
        }
        tmp=tmp->next;
    }
}

int get_dispatcher_listener_size(Dispatcher *dp)
{
    int size=0;
    ListenerList *tmp=dp->list;
    while (tmp)
    {
        size++;
        tmp=tmp->next;
    }
    return size;
}