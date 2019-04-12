#pragma once
/************************************************************************************
* Copyright (C) 2017                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#ifndef _NETSIM_OSPF_LIST_H_
#define _NETSIM_OSPF_LIST_H_
#ifdef  __cplusplus
extern "C" {
#endif

//#define _DEBUG_LIST_
	ptrOSPFLIST ospf_list_init(void(*freeMem)(void*),
							   void*(*copyMem)(void*));
	UINT ospf_list_get_size(ptrOSPFLIST list);
	
#ifdef _DEBUG_LIST_
#define ospf_list_add_mem(list,mem) ospf_list_add_mem_dbg(list,mem,__FUNCTION__,__LINE__)
#else
#define ospf_list_add_mem(list,mem) ospf_list_add_mem_dbg(list,mem,NULL,0)
#endif
	void ospf_list_add_mem_dbg(ptrOSPFLIST list,
							   void* mem,
							   char* fun,
							   int line);
	bool ospf_list_is_empty(ptrOSPFLIST list);
	void* ospf_list_get_mem(ptrOSPFLIST list);
	void ospf_list_delete_all(ptrOSPFLIST list);
	void* ospf_list_iterate_mem(ptrOSPFLIST list, void* iterator);
	void* ospf_list_get_headptr(ptrOSPFLIST list);
	void ospf_list_delete_mem(ptrOSPFLIST list, void* mem, void* ite);
	void ospf_list_destroy(ptrOSPFLIST list);
	void ospf_list_replace_mem(ptrOSPFLIST list, void* oldMem, void* newMem);
	void ospf_list_remove_mem(ptrOSPFLIST list, void* mem, void* ite);
	ptrOSPFLIST ospf_list_copyAll(ptrOSPFLIST list);
	void* ospf_list_newIterator();
	void ospf_list_deleteIterator(void* ite);

	struct stuc_arraylist
	{
		UINT count;
		void** mem;
		void(*freemem)(void*);
	};
	ptrARRAYLIST ospf_arrayList_init(void(*freeMem)(void*));
	void ospf_add_ptr_to_arrayList(ptrARRAYLIST list, void* mem);
	void ospf_arraylist_free(ptrARRAYLIST list);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_LIST_H_