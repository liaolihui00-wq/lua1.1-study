/*
** hash.h
** hash manager for lua
** Luiz Henrique de Figueiredo - 17 Aug 90
** $Id: hash.h,v 2.1 1994/04/20 22:07:57 celes Exp $
*/

#ifndef hash_h
#define hash_h

typedef struct node//Node结构体，hash表的每个节点
{
 Object ref;//key键值
 Object val;//value值
 struct node *next;//指向下一个节点的指针，形成链表
} Node;

typedef struct Hash//Hash结构体
{
 char           mark;//标记，表示这个hash表是否在使用
 unsigned int   nhash;//hash表的大小，即hash表里有多少个桶
 Node         **list;//指向Node指针的指针，list是一个数组，每个元素都是一个Node指针，指向该桶的链表头节点
} Hash;


Hash    *lua_createarray (int nhash);//创建一个新的hash表，hash表里有nhash个桶
void     lua_hashmark (Hash *h);//mark hash表还在使用
void     lua_hashcollector (void);//hashcollector回收hash表里没有被mark即不再使用的hash表
Object 	*lua_hashdefine (Hash *t, Object *ref);//在表 t 中为键 ref 寻找或创建一个位置，返回该位置的指针
void     lua_next (void);//迭代器，在hash表里返回下一个键值对

#endif
