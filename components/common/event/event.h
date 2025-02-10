#ifndef EVENT_H
#define EVENT_H
typedef struct event
{
    int type;
    unsigned char* data;
}Event;

// 被观察者
typedef struct listener
{
    int type;
    void (*update)(int);       // 观察者回调
}Listener;

// 观察者链表
typedef struct listener_list
{
    struct listener_list *prior;
    struct listener *listener;
    struct listener_list *next;
}ListenerList;

typedef struct disptcher
{
    struct listener_list *list; // 观察者链表（队列）
}Dispatcher;

Listener *add_event_listener(Dispatcher *dp,int type,void(* fun)(int i));
void remove_event_listener(Dispatcher *dp,Listener *lt);
Dispatcher *create_dipatcher();
void dispatch(Dispatcher *dp,Event *event);
int get_dispatcher_listener_size(Dispatcher *dp);
#endif