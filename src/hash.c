/*
** hash.c
** hash manager for lua
** Luiz Henrique de Figueiredo - 17 Aug 90
*/

char *rcs_hash="$Id: hash.c,v 2.1 1994/04/20 22:07:57 celes Exp $";

#include <string.h>
#include <stdlib.h>

#include "mm.h"

#include "opcode.h"
#include "hash.h"
#include "inout.h"
#include "table.h"
#include "lua.h"

#define streq(s1,s2)	(strcmp(s1,s2)==0)//字符串比较宏，比较两个字符串s1和s2是否相等
#define strneq(s1,s2)	(strcmp(s1,s2)!=0)//字符串比较宏，比较两个字符串s1和s2是否不相等

#define new(s)		((s *)malloc(sizeof(s)))//内存分配宏，分配一个s类型的内存块，并返回指向该内存块的指针
#define newvector(n,s)	((s *)calloc(n,sizeof(s)))//内存分配宏，分配一个包含n个s类型元素的数组，并返回指向该数组的指针

#define nhash(t)	((t)->nhash)//获取hash表的大小，即hash表里有多少个桶
#define nodelist(t)	((t)->list)//获取hash表的桶列表，即指向Node指针的指针，list是一个数组，每个元素都是一个Node指针，指向该桶的链表头节点
#define list(t,i)	((t)->list[i])//获取hash表t中第i个桶的链表头节点，即list是一个数组，每个元素都是一个Node指针，指向该桶的链表头节点
#define markarray(t)    ((t)->mark)//获取hash表的标记，表示这个hash表是否在使用
#define ref_tag(n)	(tag(&(n)->ref))//获取hash节点n的key键值的类型标签
#define ref_nvalue(n)	(nvalue(&(n)->ref))//获取hash节点n的key键值的数值
#define ref_svalue(n)	(svalue(&(n)->ref))//获取hash节点n的key键值的字符串值

#ifndef ARRAYBLOCK
#define ARRAYBLOCK 50//当Lua在执行过程中需要创建新的hash表时，每次创建多少个hash表，这个值是为了优化性能
#endif

typedef struct ArrayList
{
 Hash *array;
 struct ArrayList *next;
} ArrayList;//ArrayList结构体，用于管理hash表的链表，每个ArrayList节点包含一个Hash指针和一个指向下一个ArrayList节点的指针，形成一个链表结构，用于跟踪所有创建的hash表，以便进行垃圾回收等操作

static ArrayList *listhead = NULL;//hash表链表的头指针，指向第一个ArrayList节点，初始值为NULL，表示当前没有任何hash表被创建

static int head (Hash *t, Object *ref)		/* hash function，计算hash表t中键ref应该存储在哪个桶中，返回桶的索引，如果ref的类型不受支持，则返回-1 */
{
 if (tag(ref) == T_NUMBER) return (((int)nvalue(ref))%nhash(t));//如果ref是数字类型，则将ref的数值转换为整数，并对hash表的大小取模，得到桶的索引
 else if (tag(ref) == T_STRING)//如果ref是字符串类型，则将ref的字符串值解释为一个二进制数，计算出一个整数值，并对hash表的大小取模，得到桶的索引
 {
  int h;
  char *name = svalue(ref);
  for (h=0; *name!=0; name++)		/* interpret name as binary number，将名字视为二进制处理 */
  {
   h <<= 8;
   h  += (unsigned char) *name;		/* avoid sign extension */
   h  %= nhash(t);			/* make it a valid index ，防止内存泄漏*/
  }
  return h;
 }
 else
 {
  lua_reportbug ("unexpected type to index table");
  return -1;//如果ref的类型既不是数字也不是字符串，则报告一个错误，说明这是一个不受支持的类型，并返回-1表示计算失败
 }
}

static Node *present(Hash *t, Object *ref, int h)//在hash表t中查找键ref是否已经存在于桶h的链表中，如果存在则返回指向该节点的指针，否则返回NULL
{
 Node *n=NULL, *p;//定义两个Node指针n和p，n用于遍历桶h的链表，p用于记录n的前一个节点，以便在需要时进行链表操作
 if (tag(ref) == T_NUMBER)//数字
 {
  for (p=NULL,n=list(t,h); n!=NULL; p=n, n=n->next)//遍历桶h的链表，查找键ref是否存在，如果存在则返回指向该节点的指针，否则继续查找直到链表末尾
   if (ref_tag(n) == T_NUMBER && nvalue(ref) == ref_nvalue(n)) break;
 }  
 else if (tag(ref) == T_STRING)//字符串
 {
  for (p=NULL,n=list(t,h); n!=NULL; p=n, n=n->next)
   if (ref_tag(n) == T_STRING && streq(svalue(ref),ref_svalue(n))) break;
 }  
 if (n==NULL)				/* name not present */
  return NULL;
#if 0//Move-to-front 策略，将搜索到的节点移动至链表头
 if (p!=NULL)				/* name present but not first */
 {
  p->next=n->next;			/* move-to-front self-organization */
  n->next=list(t,h);
  list(t,h)=n;
 }
#endif//一种注释方式，0改为1则下面的代码块就会被编译，否则就会被忽略
 return n;
}

static void freelist (Node *n)//释放链表n占用的内存，遍历链表中的每个节点，释放它们占用的内存，最后释放整个链表的内存
{
 while (n)
 {
  Node *next = n->next;
  free (n);
  n = next;
 }
}

/*
** Create a new hash. Return the hash pointer or NULL on error.
*/
static Hash *hashcreate (unsigned int nhash)//创建一个新的hash表，hash表里有nhash个桶，返回指向该hash表的指针，如果内存分配失败则返回NULL
{
 Hash *t = new (Hash);
 if (t == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }
 nhash(t) = nhash;//设置hash表的大小，即hash表里有多少个桶
 markarray(t) = 0;//设置hash表的标记，表示这个hash表是否在使用，初始值为0，表示这个hash表不再使用，可以被垃圾回收
 nodelist(t) = newvector (nhash, Node*);//分配一个包含nhash个Node*类型元素的数组，作为hash表的桶列表，每个元素都是一个Node指针，指向该桶的链表头节点
 if (nodelist(t) == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }
 return t;
}

/*
** Delete a hash
*/
static void hashdelete (Hash *h)//删除一个hash表，释放hash表占用的内存，首先释放hash表中每个桶的链表占用的内存，然后释放整个hash表的内存
{
 int i;
 for (i=0; i<nhash(h); i++)
  freelist (list(h,i));
 free (nodelist(h));//释放hash表的桶列表占用的内存
 free(h);
}


/*
** Mark a hash and check its elements 
*/
void lua_hashmark (Hash *h)//mark hash表还在使用，遍历hash表中的每个桶的链表，标记每个节点的key键值和value值还在使用
{
 if (markarray(h) == 0)
 {
  int i;
  markarray(h) = 1;
  for (i=0; i<nhash(h); i++)//遍历hash表中的每个桶的链表，标记每个节点的key键值和value值还在使用
   if (list(h,i) != NULL)
  {
   Node *n;
   for (n = list(h,i); n != NULL; n = n->next)//遍历桶h的链表，标记每个节点的key键值和value值还在使用
   {
    lua_markobject(&n->ref);
    lua_markobject(&n->val);
   }
  }
 } 
}
 
/*
** Garbage collection to arrays
** Delete all unmarked arrays.
*/
void lua_hashcollector (void)//回收hash表里没有被mark即不再使用的hash表，遍历hash表链表中的每个hash表，如果某个hash表的标记为0，表示这个hash表不再使用，可以被垃圾回收，则删除该hash表并从链表中移除，否则将该hash表的标记重置为0，为下一次垃圾回收做准备
{
 ArrayList *curr = listhead, *prev = NULL;
 while (curr != NULL)
 {
  ArrayList *next = curr->next;
  if (markarray(curr->array) != 1)
  {
   if (prev == NULL) listhead = next;
   else              prev->next = next;//链接prev和next，跳过curr节点，为后续的垃圾回收做准备
   hashdelete(curr->array);//删除curr节点指向的hash表，释放其占用的内存
   free(curr);
  }
  else
  {
   markarray(curr->array) = 0;//重置curr节点指向的hash表的标记，为下一次垃圾回收做准备
   prev = curr;
  }
  curr = next;
 }
}


/*
** Create a new array
** This function insert the new array at array list. It also
** execute garbage collection if the number of array created
** exceed a pre-defined range.
*/
Hash *lua_createarray (int nhash)//创建一个新的hash表，hash表里有nhash个桶，返回指向该hash表的指针，如果内存分配失败则返回NULL，这个函数首先调用hashcreate函数创建一个新的hash表，如果创建成功，则将该hash表插入到hash表链表中，并检查当前创建的hash表数量是否超过预定义的范围，如果超过则执行垃圾回收，以释放不再使用的hash表占用的内存
{
 ArrayList *new = new(ArrayList);//分配一个新的ArrayList节点，用于管理新创建的hash表，如果内存分配失败则返回NULL
 if (new == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }
 new->array = hashcreate(nhash);
 if (new->array == NULL)
 {
  lua_error ("not enough memory");
  return NULL;
 }

 if (lua_nentity == lua_block)//检查当前创建的hash表数量是否超过预定义的范围，如果超过则执行垃圾回收，以释放不再使用的hash表占用的内存
  lua_pack(); 

 lua_nentity++;
 new->next = listhead;
 listhead = new;//插入到hash链表头
 return new->array;
}


/*
** If the hash node is present, return its pointer, otherwise create a new
** node for the given reference and also return its pointer.
** On error, return NULL.
*/
Object *lua_hashdefine (Hash *t, Object *ref)//在hash表t中为键ref寻找或创建一个位置，返回该位置的指针，这个函数首先调用present函数检查键ref是否已经存在于hash表t中，如果存在则返回指向该节点的指针，否则创建一个新的节点，将键ref和一个初始值nil存储在该节点中，并将该节点插入到桶h的链表中，最后返回指向新节点的value值的指针
{
 int   h;
 Node *n;
 h = head (t, ref);
 if (h < 0) return NULL; 
 
 n = present(t, ref, h);
 if (n == NULL)
 {
  n = new(Node);
  if (n == NULL)
  {
   lua_error ("not enough memory");
   return NULL;
  }
  n->ref = *ref;
  tag(&n->val) = T_NIL;
  n->next = list(t,h);			/* link node to head of list */
  list(t,h) = n;
 }
 return (&n->val);//返回指向新节点的value值的指针
}


/*
** Internal function to manipulate arrays. 
** Given an array object and a reference value, return the next element
** in the hash.
** This function pushs the element value and its reference to the stack.
*/
static void firstnode (Hash *a, int h)//内部函数，用于操作数组。给定一个数组对象和一个引用值，返回hash中的下一个元素，这个函数将元素值和它的引用推入栈中
{
 if (h < nhash(a))
 {  
  int i;
  for (i=h; i<nhash(a); i++)
  {
   if (list(a,i) != NULL)
   {
    if (tag(&list(a,i)->val) != T_NIL)
    {
     lua_pushobject (&list(a,i)->ref);//将桶i的链表头节点的key键值推入栈中
     lua_pushobject (&list(a,i)->val);
     return;
    }
    else
    {
     Node *next = list(a,i)->next;
     while (next != NULL && tag(&next->val) == T_NIL) next = next->next;//
     if (next != NULL)
     {
      lua_pushobject (&next->ref);
      lua_pushobject (&next->val);
      return;
     }
    }
   }
  }
 }
 lua_pushnil();
 lua_pushnil();//如果没有找到下一个元素，则将nil推入栈中，表示迭代结束
}

void lua_next (void)//迭代器，在hash表里返回下一个键值对，这个函数首先从栈中获取一个数组对象和一个引用值，如果数组对象的类型不正确或者参数数量不正确，则报告一个错误并返回，否则调用firstnode函数在hash表中查找下一个元素，并将其键值对推入栈中，如果没有找到下一个元素，则将nil推入栈中，表示迭代结束
{
 Hash   *a;
 Object *o = lua_getparam (1);// 获取第一个参数：Table
 Object *r = lua_getparam (2);// 获取第二个参数：当前的 Key
 if (o == NULL || r == NULL)
 { lua_error ("too few arguments to function `next'"); return; }
 if (lua_getparam (3) != NULL)
 { lua_error ("too many arguments to function `next'"); return; }
 if (tag(o) != T_ARRAY)
 { lua_error ("first argument of function `next' is not a table"); return; }
 a = avalue(o);
 if (tag(r) == T_NIL)
 {
  firstnode (a, 0);//如果当前的Key是nil，表示这是第一次调用next函数，应该从hash表的第一个桶开始查找下一个元素
  return;
 }
 else
 {
  int h = head (a, r);
  if (h >= 0)
  {
   Node *n = list(a,h);
   while (n)
   {
    if (memcmp(&n->ref,r,sizeof(Object)) == 0)//如果找到了当前的Key，则继续在桶h的链表中查找下一个元素
    {
     if (n->next == NULL)
     {
      firstnode (a, h+1);//如果当前节点是链表的最后一个节点，表示在桶h中没有下一个元素了，应该从桶h+1开始查找下一个元素
      return;
     }
     else if (tag(&n->next->val) != T_NIL)
     {
      lua_pushobject (&n->next->ref);
      lua_pushobject (&n->next->val);
      return;
     }
     else//如果当前节点的下一个节点的value值是nil，表示下一个元素已经被删除了，应该继续在链表中查找下一个元素，直到找到一个value值不为nil的节点或者链表末尾
     {
      Node *next = n->next->next;
      while (next != NULL && tag(&next->val) == T_NIL) next = next->next;
      if (next == NULL)
      {
       firstnode (a, h+1);
       return;
      }
      else
      {
       lua_pushobject (&next->ref);
       lua_pushobject (&next->val);
      }
      return;
     }
    }
    n = n->next;
   }
   if (n == NULL)
    lua_error ("error in function 'next': reference not found");
  }
 }
}
