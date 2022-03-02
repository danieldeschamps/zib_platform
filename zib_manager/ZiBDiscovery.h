#ifndef _ZIB_DISCOVERY_H_
#define _ZIB_DISCOVERY_H_

#include <vector>
#include <map>
/*
WORD nwk_addr;
BYTE ep;
BYTE cluster;
WORD attribute_id;
*/

/*class Attribute
{
public:
	Attribute();
	~Attribute();

protected:
	BYTE attribute_id;
	string attribute_value;
};*/

#define ATTRIBUTE map<WORD, string>


class Cluster
{
public:
	ZiBCluster();
	~ZiBCluster();

protected:
	vector<ATTRIBUTE>
};


class ZiBNode
{
public:
	ZiBNode();
	~ZiBNode();

protected:
	vector<>
};

#endif // _ZIB_DISCOVERY_H_