/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include <iostream> 
#include "BTreeIndex.h"
#include "BTreeNode.h"


using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    nodeCount = -1;
    treeHeight = -1;
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
	indexFilename = indexname + ".idx";
	char buffer[PageFile::PAGE_SIZE];
	if((pf.open(indexFilename, 'r')) == 0) {
		pf.read(0, buffer);
		memcpy(&rootPid, buffer, sizeof(PageId));
		memcpy(&treeHeight, buffer + sizeof(PageId), sizeof(int));
		memcpy(&nodeCount, buffer + sizeof(PageId) + sizeof(int), sizeof(PageId));
		pf.close();
	}
    return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
	char buffer[PageFile::PAGE_SIZE];
	memset(buffer, 0, PageFile::PAGE_SIZE);
	memcpy(buffer, &rootPid, sizeof(PageId));
	memcpy(buffer + sizeof(PageId), &treeHeight, sizeof(int));
	memcpy(buffer + sizeof(PageId) + sizeof(int), &nodeCount, sizeof(PageId));
	pf.open(indexFilename, 'w');
	pf.write(0, buffer);
	pf.close();
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
		rootPid = leafNodePid = 1;
		nodeCount = 1;


	} else {
		IndexCursor cursor;
		locate(key, cursor);
		leafNodePid = cursor.pid;
		readLeafNode(leafNode, leafNodePid);
	}

	if(leafNode.insert(key, rid) == RC_NODE_FULL) {
		
		// Insert and split the full leaf node and set the next node pointers accordingly
		BTLeafNode siblingLeafNode; 
		PageId siblingLeafPid;
		int leafSiblingKey;

		insertSiblingLeafNode(leafNode, key, rid, siblingLeafNode, siblingLeafPid, leafSiblingKey);

		// Create or get parent node page id
		BTNonLeafNode parentNode;
		PageId parentPid;
		if((parentPid = leafNode.getParentPid()) == -1) {
			insertNewRootFromLeaf(leafNode, leafNodePid, siblingLeafNode, siblingLeafPid, leafSiblingKey, parentNode, parentPid);
			return 0;

		} else {
			readNonLeafNode(parentNode, parentPid);
		}

		leafNode.setParentPid(parentPid);
		siblingLeafNode.setParentPid(parentPid);
		writeLeafNode(leafNode, leafNodePid);
		writeLeafNode(siblingLeafNode, siblingLeafPid);
	
		BTNonLeafNode currentNode = parentNode;
		PageId currentPid = parentPid;
		int currentKey = leafSiblingKey;

		PageId currentChildPid = siblingLeafPid;

		int heightUp = 0;
		while(currentNode.insert(currentKey, currentChildPid) == RC_NODE_FULL) {
			BTNonLeafNode parentSiblingNode; 
			PageId parentSiblingPid;
			int midKey;
			PageId midPid;

			insertSiblingNonLeafNode(currentNode, currentKey, currentChildPid, parentSiblingNode, parentSiblingPid, midKey, midPid);

			if(heightUp == 0) {
				siblingLeafNode.setParentPid(parentSiblingPid);
				writeLeafNode(siblingLeafNode, siblingLeafPid);
				BTLeafNode minLeafNode;
				readLeafNode(minLeafNode, midPid);
				minLeafNode.setParentPid(parentSiblingPid);
				writeLeafNode(minLeafNode, midPid);
			} else {
				BTNonLeafNode currentChild;
				readNonLeafNode(currentChild, currentChildPid);
				currentChild.setParentPid(parentSiblingPid);
				writeNonLeafNode(currentChild, currentChildPid);
				BTNonLeafNode minNonLeafNode;
				readNonLeafNode(minNonLeafNode, midPid);
				minNonLeafNode.setParentPid(parentSiblingPid);
				writeNonLeafNode(minNonLeafNode, midPid);
			}
			
			PageId newParentPid = currentNode.getParentPid();
			if(newParentPid == -1) {
				BTNonLeafNode newRoot;
				PageId rootNodePid;
				insertNewRootFromNonLeaf(currentNode, currentPid, parentSiblingNode, parentSiblingPid, midKey, newRoot, rootNodePid);
				break;

			} else {
				heightUp++;
				BTNonLeafNode newParentNode;
				readNonLeafNode(newParentNode, newParentPid);
				writeNonLeafNode(currentNode, currentPid);
				parentSiblingNode.setParentPid(newParentPid);
				writeNonLeafNode(parentSiblingNode, parentSiblingPid);
				currentKey = midKey;
				currentNode = newParentNode;
				currentPid = newParentPid;
				currentChildPid = parentSiblingPid;
				writeNonLeafNode(parentSiblingNode, parentSiblingPid);

			}

		}
		writeNonLeafNode(currentNode, currentPid);

	} else {
		writeLeafNode(leafNode, leafNodePid);
    	return 0;
    }
}

RC BTreeIndex::insertSiblingLeafNode(BTLeafNode& leafNode, int key, RecordId rid, BTLeafNode& siblingLeafNode, PageId& siblingLeafPid, int& siblingLeafKey)
{
	leafNode.insertAndSplit(key, rid, siblingLeafNode, siblingLeafKey);
	siblingLeafPid = increaseNodeCount();
	siblingLeafNode.setNextNodePtr(leafNode.getNextNodePtr());
	leafNode.setNextNodePtr(siblingLeafPid);
	return 0;
}

RC BTreeIndex::insertSiblingNonLeafNode(BTNonLeafNode& nonLeafNode, int key, PageId pid, BTNonLeafNode& siblingNonLeafNode, PageId& siblingLeafPid, int& midKey, PageId& midPid)
{
	nonLeafNode.insertAndSplit(key, pid, siblingNonLeafNode, midKey, midPid);
	siblingLeafPid = increaseNodeCount();
	siblingNonLeafNode.setMinPageId(midPid);
}

RC BTreeIndex::insertNewRootFromLeaf(BTLeafNode& leafNode, PageId leafNodePid, BTLeafNode& siblingLeafNode, PageId siblingLeafPid, int siblingLeafKey, BTNonLeafNode& rootNode, PageId& rootNodePid) 
{
	rootNode.initializeRoot(leafNodePid, siblingLeafKey, siblingLeafPid);
	rootNodePid = increaseNodeCount();
	treeHeight++;
	leafNode.setParentPid(rootNodePid);
	siblingLeafNode.setParentPid(rootNodePid);
	rootPid = rootNodePid;
	writeLeafNode(leafNode, leafNodePid);
	writeLeafNode(siblingLeafNode, siblingLeafPid);
	writeNonLeafNode(rootNode, rootPid);
	
	return 0;
}

RC BTreeIndex::insertNewRootFromNonLeaf(BTNonLeafNode& nonLeafNode, PageId nonLeafPid, BTNonLeafNode& siblingNonLeafNode, PageId siblingNonLeafPid, int midKey, BTNonLeafNode& rootNode, PageId& rootNodePid)
{
	rootNode.initializeRoot(nonLeafPid, midKey, siblingNonLeafPid);
	rootNodePid = increaseNodeCount();
	treeHeight++;
	nonLeafNode.setParentPid(rootNodePid);
	siblingNonLeafNode.setParentPid(rootNodePid);
	rootPid = rootNodePid;
	writeNonLeafNode(rootNode, rootPid);
	writeNonLeafNode(nonLeafNode, nonLeafPid);
	writeNonLeafNode(siblingNonLeafNode, siblingNonLeafPid);
}

RC BTreeIndex::readLeafNode(BTLeafNode& leafNode, PageId leafPid)
{
	pf.open(indexFilename, 'r');
	leafNode.read(leafPid, pf);
	pf.close();
	return 0;
}

RC BTreeIndex::writeLeafNode(BTLeafNode leafNode, PageId leafNodePid)
{
	pf.open(indexFilename, 'w');
	leafNode.write(leafNodePid, pf);
	pf.close();
	return 0;
}

RC BTreeIndex::readNonLeafNode(BTNonLeafNode& nonLeafNode, PageId nonLeafPid)
{
	pf.open(indexFilename, 'r');
	nonLeafNode.read(nonLeafPid, pf);
	pf.close();
	return 0;
}

RC BTreeIndex::writeNonLeafNode(BTNonLeafNode nonLeafNode, PageId nonLeafPid)
{
	pf.open(indexFilename, 'w');
	nonLeafNode.write(nonLeafPid, pf);
	pf.close();
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
		readNonLeafNode(internalNode, nodePid);
		internalNode.locateChildPtr(searchKey, nodePid);
		currentLevel++;
	}

	BTLeafNode leafNode;
	int eid;
	readLeafNode(leafNode, nodePid);
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
	BTLeafNode leafNode;
	PageId leafNodePid = cursor.pid;
	readLeafNode(leafNode, leafNodePid);
	leafNode.readEntry(cursor.eid, key, rid);
	cursor.eid++;
	
	int testKey;
	RecordId testRid;
	if(leafNode.readEntry(cursor.eid, testKey, testRid) == RC_NO_SUCH_RECORD) {
		cursor.eid = 0;
		cursor.pid = leafNode.getNextNodePtr();
	}
    return 0;
}

int BTreeIndex::increaseNodeCount()
{
	nodeCount++;
	return nodeCount;
}


void BTreeIndex::printTree()
{
	map<int, map<PageId, BTNonLeafNode> > nonLeafLevels;
	int currentLevel = 0;
	BTNonLeafNode root;
	readNonLeafNode(root, rootPid);
	map<PageId, BTNonLeafNode>  rootLevel;
	rootLevel[rootPid] = root;
	nonLeafLevels[currentLevel] = rootLevel;

	map<PageId, BTNonLeafNode> currentLevelNodes;
	currentLevelNodes[rootPid] = root;
	while(currentLevel + 1 < treeHeight)
	{
		vector<PageId> nextLevelPids;
		for (map<PageId, BTNonLeafNode>::iterator it = currentLevelNodes.begin() ; it != currentLevelNodes.end(); ++it) {
			BTNonLeafNode currentNode = it->second;
			vector<PageId> currentNodePids = currentNode.getAllPids();
			for(vector<PageId>::iterator it = currentNodePids.begin() ; it != currentNodePids.end(); ++it) {
				nextLevelPids.push_back(*it);
			}
		}
		
		map<PageId, BTNonLeafNode> nextLevelNodes;
		for (vector<PageId>::iterator it = nextLevelPids.begin() ; it != nextLevelPids.end(); ++it) {
			BTNonLeafNode nextLevelNode;
			PageId nextLevelPid = *it;
			readNonLeafNode(nextLevelNode, nextLevelPid);
			nextLevelNodes[nextLevelPid] = nextLevelNode;
		}
		currentLevel++;
		nonLeafLevels[currentLevel] = nextLevelNodes;
		currentLevelNodes = nextLevelNodes;
	}

	vector<PageId> leafLevelPids;
	for (map<PageId, BTNonLeafNode>::iterator it = currentLevelNodes.begin() ; it != currentLevelNodes.end(); ++it) {
		BTNonLeafNode currentNode = it->second;
		vector<PageId> currentNodePids = currentNode.getAllPids();
		for(vector<PageId>::iterator it = currentNodePids.begin() ; it != currentNodePids.end(); ++it) {
			leafLevelPids.push_back(*it);
		}
	}

	map<PageId, BTLeafNode> leafLevelNodes;
	for (vector<PageId>::iterator it = leafLevelPids.begin() ; it != leafLevelPids.end(); ++it) {
		BTLeafNode leafLevelNode;
		PageId leafLevelPid = *it;
		readLeafNode(leafLevelNode, leafLevelPid);
		leafLevelNodes[leafLevelPid] = leafLevelNode;
	}

	// Print non leaf level nodes
	for (map<int, map<PageId, BTNonLeafNode> >::iterator it = nonLeafLevels.begin() ; it != nonLeafLevels.end(); ++it) {
		cout << "Tree level: " << it->first << endl;
		map<PageId, BTNonLeafNode> nodes = it->second;
		for(map<PageId, BTNonLeafNode>::iterator it = nodes.begin() ; it != nodes.end(); ++it) {
			cout << "Node PID: " <<  it->first << endl;
			BTNonLeafNode node = it->second;
			node.printNode();
		}
	}

	// Print leaf level nodes
	cout << "Leaf level: " << endl;
	for(map<PageId, BTLeafNode>::iterator it = leafLevelNodes.begin() ; it != leafLevelNodes.end(); ++it) {
			cout << "Node PID: " << it->first << endl;
			BTLeafNode node = it->second;
			node.printNode();
	}

}
