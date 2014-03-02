#include "BTreeNode.h"
#include <iterator>
#include <algorithm>
#include <iostream>

using namespace std;

BTLeafNode::BTLeafNode()
{
	keyCount = 0;
	nextNode = -1;
	parentPid = -1;
}
/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ 
	RC code = pf.read(pid, buffer);
	memcpy(&keyCount, buffer, sizeof(int));
	memcpy(&nextNode, buffer + sizeof(int), sizeof(PageId));
	memcpy(&parentPid, buffer + sizeof(int) + sizeof(PageId), sizeof(PageId));
	for(int i=0; i < keyCount; i++) {
		int key; RecordId rid;
		char* bufferOffset = (buffer + sizeof(int) + sizeof(PageId)*2) + i * ENTRY_SIZE;
		memcpy(&key, bufferOffset, sizeof(int));
		memcpy(&rid.pid, bufferOffset + sizeof(int), sizeof(PageId));
		memcpy(&rid.sid, bufferOffset + sizeof(int) + sizeof(PageId), sizeof(int));
		nodeBuckets[key] = rid;
	}
	return code;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
	memset(buffer, 0, PageFile::PAGE_SIZE);
	memcpy(buffer, &keyCount, sizeof(int));
	memcpy(buffer + sizeof(int), &nextNode, sizeof(PageId));
	memcpy(buffer + sizeof(int) + sizeof(PageId), &parentPid, sizeof(PageId));

	map<int, RecordId>::iterator it; int i;
	for(it = nodeBuckets.begin(), i=0; it != nodeBuckets.end(); ++it, i++) {
		char* bufferOffset = (buffer + sizeof(int) + sizeof(PageId)*2) + i * ENTRY_SIZE;
		memcpy(bufferOffset, &it->first, sizeof(int));
		memcpy(bufferOffset + sizeof(int), &it->second.pid, sizeof(PageId));
		memcpy(bufferOffset + sizeof(int) + sizeof(PageId), &it->second.sid, sizeof(int));

	}
	RC code = pf.write(pid, buffer);
	return code; 
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{ 
	return keyCount; 
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{ 
	if(nodeBuckets.size() >= ENTRY_LIMIT) {
		return RC_NODE_FULL;
	} else {
		nodeBuckets[key] = rid;
		keyCount++;
	}
	return 0; 
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ 
	// Insert key and record id
	nodeBuckets[key] = rid;

	// Set up the iterators 
	int halfLocation = nodeBuckets.size()/2;
	map<int, RecordId>::iterator itHalf = nodeBuckets.begin();
	advance(itHalf, halfLocation);

	// Copy the second half of the node to the new sibling node and then erase this data from the current node
	for(map<int, RecordId>::iterator it = itHalf; it != nodeBuckets.end(); ++it) {
		sibling.nodeBuckets[it->first] = it->second;
	}
	// Erase the split data and set the key count right
	nodeBuckets.erase(itHalf, nodeBuckets.end());
	keyCount = nodeBuckets.size();

	// Set the sibling key to the first key in the sibling node and set the siblings key count
	siblingKey = sibling.nodeBuckets.begin()->first;
	sibling.keyCount = sibling.nodeBuckets.size();
	return 0; 
}

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
	int i = 0;
	for(map<int, RecordId>::iterator it = nodeBuckets.begin(); it != nodeBuckets.end(); ++it) {
		if(it->first >= searchKey) {
			eid = i;
			return 0;
		}
		i++;
	}
	return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ 
	if(eid > keyCount) {
		return RC_NO_SUCH_RECORD;
	}
	map<int, RecordId>::iterator it = nodeBuckets.begin();
	advance(it, eid);
	key = it->first;
	rid = it->second;
	return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 
	return nextNode; 
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ 
	nextNode = pid;
	return 0; 
}

PageId BTLeafNode::getParentPid()
{
	return parentPid;
}

RC BTLeafNode::setParentPid(PageId pid)
{
	parentPid = pid;
	return 0;
}


void BTLeafNode::printNode()
{
	for(map<int, RecordId>::iterator it = nodeBuckets.begin(); it != nodeBuckets.end(); ++it) {
		cout << "Node Key: " << it->first << endl;
		cout << "Page ID: " << it->second.pid << "    Slot ID: " << it->second.sid << endl << endl;
	}
}

BTNonLeafNode::BTNonLeafNode()
{
	minPageId = -1;
	keyCount = 0;
	parentPid = -1;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 
	RC code = pf.read(pid, buffer);
	memcpy(&keyCount, buffer, sizeof(int));
	memcpy(&minPageId, buffer + sizeof(int), sizeof(PageId));
	memcpy(&parentPid, buffer + sizeof(int) + sizeof(PageId), sizeof(PageId));
	for(int i=0; i < keyCount; i++) {
		int key; PageId pid;
		char* bufferOffset = (buffer + sizeof(int) + sizeof(PageId)*2) + i * ENTRY_SIZE;
		memcpy(&key, bufferOffset, sizeof(int));
		memcpy(&pid, bufferOffset + sizeof(int), sizeof(PageId));
		nodeBuckets[key] = pid;
	}
	return code; 
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ 
	memset(buffer, 0, PageFile::PAGE_SIZE);
	memcpy(buffer, &keyCount, sizeof(int));
	memcpy(buffer + sizeof(int), &minPageId, sizeof(PageId));
	memcpy(buffer + sizeof(int) + sizeof(PageId), &parentPid, sizeof(PageId));

	map<int, PageId>::iterator it; int i;
	for(it = nodeBuckets.begin(), i=0; it != nodeBuckets.end(); ++it, i++) {
		char* bufferOffset = (buffer + sizeof(int) + sizeof(PageId)*2) + i * ENTRY_SIZE;
		memcpy(bufferOffset, &it->first, sizeof(int));
		memcpy(bufferOffset + sizeof(int), &it->second, sizeof(PageId));
	}
	RC code = pf.write(pid, buffer);
	return code;  
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ 
	return keyCount; 
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ 
	if(nodeBuckets.size() >= ENTRY_LIMIT) {
		return RC_NODE_FULL;
	} else {
		nodeBuckets[key] = pid;
		keyCount++;
	}
	return 0;  
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ 
	// Insert key and record id
	nodeBuckets[key] = pid;

	// Set up the iterators 
	int halfLocation = nodeBuckets.size()/2;
	map<int, PageId>::iterator itMidEntry = nodeBuckets.begin();
	advance(itMidEntry, halfLocation);
	map<int, PageId>::iterator itSiblingBegin = itMidEntry;
	advance(itSiblingBegin, 1);
	midKey = itMidEntry->first;

	// Copy the second half of the node to the new sibling node and then erase this data from the current node
	for(map<int, PageId>::iterator it = itSiblingBegin; it != nodeBuckets.end(); ++it) {
		sibling.nodeBuckets[it->first] = it->second;
	}
	// Erase the split data and set the key count right
	nodeBuckets.erase(itMidEntry, nodeBuckets.end());
	keyCount = nodeBuckets.size();

	// Set the sibling's key count
	sibling.keyCount = sibling.nodeBuckets.size();
	return 0; 
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ 
	for(map<int, PageId>::iterator it = nodeBuckets.begin(); it != nodeBuckets.end(); ++it) {
		if(searchKey >= it->first) {
			pid = it->second;
			return 0;
		}
	}
	// If the search key is greater than all the keys in the node, return the 
	pid = minPageId;
	return 0; 
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ 
	minPageId = pid1;
	nodeBuckets[key] = pid2;
	return 0; 
}

PageId BTNonLeafNode::getMinPageId()
{
	return minPageId;
}

RC BTNonLeafNode::setMinPageId(PageId pid)
{
	minPageId = pid;
	return 0;
}

PageId BTNonLeafNode::getParentPid()
{
	return parentPid;
}

RC BTNonLeafNode::setParentPid(PageId pid)
{
	parentPid = pid;
	return 0;
}

void BTNonLeafNode::clear()
{
	nodeBuckets.clear();
	minPageId = -1;
	keyCount = 0;
	parentPid = -1;
}

void BTNonLeafNode::printNode()
{
	for(map<int, PageId>::iterator it = nodeBuckets.begin(); it != nodeBuckets.end(); ++it) {
		cout << "Node Key: " << it->first << endl;
		cout << "Page ID: " << it->second << endl << endl;
	}
}
