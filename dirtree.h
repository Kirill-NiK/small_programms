#pragma once

#include <QString>

// подумать о том, надо ли поставить флаг типа путь загноренный или нет. где это лучше делать.
struct DirNode {
	DirNode *parentNode;
	DirNode *rightNode;
	DirNode *childNode;
	QString name;
	int localTesting;
	int localDocumented;
	int totalTesting;
	int totalDocumented;
	int localTests;
	int totalTests;
	bool isIgnored;
};

class DirTree
{
	public:
		explicit DirTree(QString name, int localTesting, int localDocumented, int localTests = 0, bool isIgnored = false);
		virtual ~DirTree();
		void addChild(DirNode *child, DirNode *parent);
		DirNode *getRoot();
		static DirNode *createNode(QString name, int localTesting, int localDocumented, int localTests = 0,  bool isIgnored = false);
		void calculateTotalData();

	private:
		void calculateTotalDataForNode(DirNode *node);
		void delNode(DirNode *node);
		DirNode *root;
};
