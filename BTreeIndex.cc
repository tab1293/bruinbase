/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;
const string BTreeIndex::LEAF_NODE_PAGE_NAME = "leafPage";
const string BTreeIndex::NON_LEAF_NODE_PAGE_NAME ="nonLeafPage";
/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
	//const string BTreeIndex::LEAF_NODE_PAGE_NAME = "leafPage";
	//const string BTreeIndex::NON_LEAF_NODE_PAGE_NAME ="nonLeafPage";
    rootPid = -1;
    nodeCount = -1;
    treeHeight = 0;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
    return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    return 0;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
	BTLeafNode leafNode;
	PageId leafNodePid;

	// If the tree is empty: make a new root which is also a leaf node
	// Else: Find where the node where the new key should be inserted
	if(treeHeight == -1) {
		treeHeight = 0;
		rootPid = leafNodePid = 0;
		nodeCount = 1;

	} else {
		IndexCursor cursor;
		locate(key, cursor);
		leafNodePid = cursor.pid;
		leafNodePf.open(LEAF_NODE_PAGE_NAME, 'r');
		leafNode.read(leafNodePid, leafNodePf);
		leafNodePf.close();
	}

	if(leafNode.insert(key, rid) == RC_NODE_FULL) {
		// Insert and split the full leaf node and set the next node pointers accordingly
		BTLeafNode siblingLeafNode; 
		int leafSiblingKey;
		
		leafNode.insertAndSplit(key, rid, siblingLeafNode, leafSiblingKey);
		PageId leafSiblingPid = increaseNodeCount();
		siblingLeafNode.setNextNodePtr(leafNode.getNextNodePtr());
		leafNode.setNextNodePtr(leafSiblingPid);
		
		// Create or get parent node page id
		BTNonLeafNode parentNode;
		PageId parentPid;
		if((parentPid = leafNode.getParentPid()) == -1) {
			parentNode.initializeRoot(leafNodePid, key, leafSiblingPid);
			treeHeight++;
			parentPid = increaseNodeCount();
			leafNode.setParentPid(parentPid);
			siblingLeafNode.setParentPid(parentPid);

		} else {
			nonLeafNodePf.open(NON_LEAF_NODE_PAGE_NAME, 'r');
			parentNode.read(parentPid, nonLeafNodePf);
			nonLeafNodePf.close();
		}

		leafNodePf.open(LEAF_NODE_PAGE_NAME, 'w');
		leafNode.write(leafNodePid, leafNodePf);
		siblingLeafNode.write(leafSiblingPid, leafNodePf);
		leafNodePf.close();

		BTNonLeafNode currentNode = parentNode;
		int currentKey = leafSiblingKey;
		int currentPid = leafSiblingPid;

		while(currentNode.insert(currentKey, currentPid) == RC_NODE_FULL) {
			BTNonLeafNode parentSiblingNode; 
			int midKey;
			currentNode.insertAndSplit(currentKey, currentPid, parentSiblingNode, midKey);
			PageId parentSiblingPid = increaseNodeCount();

			PageId parentPid = currentNode.getParentPid();
			if(parentPid == -1) {
				BTNonLeafNode newRoot;
				newRoot.initializeRoot(currentPid, currentKey, parentSiblingPid);
				PageId newRootPid = increaseNodeCount();
				currentNode.setParentPid(newRootPid);
				parentSiblingNode.setParentPid(newRootPid);

				nonLeafNodePf.open(NON_LEAF_NODE_PAGE_NAME, 'w');
				newRoot.write(newRootPid, nonLeafNodePf);
				parentSiblingNode.write(parentSiblingPid, nonLeafNodePf);
				nonLeafNodePf.close();
				treeHeight++;
				break;

			} else {
				parentSiblingNode.setMinPageId(currentPid);
				nonLeafNodePf.open(NON_LEAF_NODE_PAGE_NAME, 'w');
				currentNode.write(currentPid, nonLeafNodePf);
				parentSiblingNode.write(parentSiblingPid, nonLeafNodePf);
				nonLeafNodePf.close();
				currentNode.clear();
				nonLeafNodePf.open(NON_LEAF_NODE_PAGE_NAME, 'r');
				currentNode.read(parentPid, nonLeafNodePf);
				nonLeafNodePf.close();
				currentKey = midKey;
				currentPid = parentSiblingPid;
			}

		}

		nonLeafNodePf.open(NON_LEAF_NODE_PAGE_NAME, 'w');
		currentNode.write(currentPid, nonLeafNodePf);
		nonLeafNodePf.close();

	}

	leafNodePf.open(LEAF_NODE_PAGE_NAME, 'r');
	leafNode.write(leafNodePid, leafNodePf);
	leafNodePf.close();
    return 0;
}

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
	int currentLevel = 0;
	PageId nodePid = rootPid;
		
	while(currentLevel != treeHeight) {
		BTNonLeafNode internalNode;
		internalNode.read(nodePid, pf);
		internalNode.locateChildPtr(searchKey, nodePid);
		currentLevel++;
	}

	BTLeafNode leafNode;
	int eid;

	leafNode.read(nodePid, pf);
	leafNode.locate(searchKey, eid);
	
	cursor.pid = nodePid;
	cursor.eid = eid;
    return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    return 0;
}

int BTreeIndex::increaseNodeCount()
{
	nodeCount++;
	return nodeCount;
}
