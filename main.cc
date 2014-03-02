/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "Bruinbase.h"
#include "SqlEngine.h"

#include <iostream>
#include "BTreeNode.h"

using namespace std;
int main()
{
  // run the SQL engine taking user commands from standard input (console).
  //SqlEngine::run(stdin);
  
  		
 //  	BTLeafNode btLeafNode;
 //  	RecordId rid;
 //  	//cout << btLeafNode.ENTRY_LIMIT << endl;
 //  	for(int i=0; i<84; i++) {
 //  		rid.pid = 0;
 //  		rid.sid = i*3;
 //  		btLeafNode.insert(i*7, rid);
 //  	}
  
 //  	BTLeafNode btSiblingLeafNode;
 //  	int siblingKey;
 //  	rid.pid = 0;
 //  	rid.sid = 84;
 //  	btLeafNode.insertAndSplit(85, rid, btSiblingLeafNode, siblingKey);
 //  	cout << "First node: " << endl;
 //  	btLeafNode.printNode();
 //  	cout << "First node key count: " << btLeafNode.getKeyCount() << endl;
 //  	cout << "End first node" << endl << endl;
 //  	cout << "Second node: " << endl;
 //  	btSiblingLeafNode.printNode();
 //  	cout << "Second node key count: " << btSiblingLeafNode.getKeyCount() << endl;
 //  	cout << "End second node" << endl << endl;

 //  	int eid;
 //  	btLeafNode.locate(42, eid);

 //  	cout << "EID: " << eid << endl;

 //  	int key; 
 //  	btLeafNode.readEntry(0, key, rid);
 //  	cout << "Key: " << key << "    PageID: " << rid.pid << "    Slot ID: " << rid.sid << endl;

	// PageFile pf = PageFile("testPage", 'w');
	// btLeafNode.write(0, pf);

	// BTLeafNode btLeafNode;
	// cout << btLeafNode.ENTRY_SIZE << endl;
	// PageFile pf = PageFile("testPage", 'r');
	// btLeafNode.read(0, pf);
	// btLeafNode.printNode();
	// cout << btLeafNode.getNextNodePtr() << endl;
	// cout << btLeafNode.getKeyCount() << endl;
	// cout << btLeafNode.getParentPid() << endl;
	
 // 	BTNonLeafNode btNonLeafNode;
 //  	PageId pid;
 //  	for(int i=0; i<126; i++) {
 //  		btNonLeafNode.insert(i, 0);
 //  	}
 //  	BTNonLeafNode btNonLeafSibling;
 //  	int midKey;
 //  	btNonLeafNode.insertAndSplit(126, 0, btNonLeafSibling, midKey);
 //  	cout << "Entry limit: " << btNonLeafNode.ENTRY_LIMIT << endl;
 //  	btNonLeafNode.printNode();
 //  	cout << "Sibling node: " << endl;
 //  	btNonLeafSibling.printNode();
 //  	cout << btNonLeafNode.getKeyCount() << endl;
 //  	cout << midKey << endl;
 //  	cout << btNonLeafNode.getParentPid() << endl;

 //  	PageFile pf = PageFile("nonleafpage", 'w');
	// btNonLeafNode.write(0, pf);
	
	// BTNonLeafNode btNonLeafNode;
	// PageFile pf = PageFile("nonleafpage", 'r');
	// btNonLeafNode.read(0, pf);
	// btNonLeafNode.printNode();
	// cout << btNonLeafNode.getMinPageId() << endl;
	// cout << btNonLeafNode.getKeyCount() << endl;
	// cout << btNonLeafNode.getParentPid() << endl;

  	return 0;
}
