#include <QDir>
#include <QString>
#include <QFile>
#include <stdio.h>
#include <QRegExp>
#include <QtDebug>
#include <QTime>

#include "dirtree.h"

unsigned int totalTestingFunCount = 0;
unsigned int totalVirtualCount = 0;
unsigned int totalDocumentedCount = 0;

QStringList ignoreFiles;
QString outputStats = "";
QStringList testpathes;
DirTree *dirTree = NULL;

QString colorToString(bgcolors color)
{
	QString arr[]={"yellow", "white", "pink", "orange", "00CC66", "CC99CC", "00CCCC"};
	return arr[color];
}

bgcolors nextColor(bgcolors color)
{
	return (bgcolors)(((int)color + 1) % 7);
}

QString commentFreeString(QString str)
{
	// this counting is used and accurate because we have style guide
	int docCount = str.count(QRegExp("///[^\\n]*\\n[^\\n/\\(]*\\("));
	docCount += str.count(QString("/**"));
	docCount += str.count(QRegExp("//![^\\n]*\\n[^/\\(]*\\("));
	totalDocumentedCount += docCount;
	QRegExp reg = QRegExp("/\\*.*\\*/");
	reg.setMinimal(true);
	str.remove(reg);
	str.remove(QRegExp("//[^\\n$]*(\\n|$)"));
	return str;
}

QString removeImplementations(QString str)
{
	int publics = str.count(QString("public:"));
	int index = str.indexOf(QString("public:"));
	str.remove(0, index + 2);
	if (publics > 1) {
		QRegExp reg("class.*public:");
		reg.setMinimal(true);
		str.remove(reg);
	}
	int count = str.count("{");
	for (int i = 1; i <= count; ++i) {
		str.remove(QRegExp("\\{[^\\{\\}]*\\}"));
	}
	return str;
}

unsigned int virtualMethodsCount(QString str)
{
	return str.count(QRegExp("\\bvirtual\\b"));
}

unsigned int macrosesWithParameters(QString str)
{
	return str.count(QRegExp("Q_[A-Z]+(\\s)*\\([^\\)]*\\)"));
}

bool isIgnored(QString fileName)
{
	bool isMatch = false;
	for (int j = 0; j < ignoreFiles.length(); ++j)
	{
		QRegExp exReg(".*" + ignoreFiles.at(j) + ".*");
		if (exReg.exactMatch(fileName)) {
			isMatch = true;
			break;
		}
	}
	return isMatch;
}

unsigned int localFunctionCount(QString path, QString fileName)
{
	QFile file(path + fileName + ".h");
	// the second condition need for events when .h is interface
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || !QFile::exists(path + fileName + ".cpp")
		|| isIgnored(path + fileName + ".h")) {
		return 0;
	}

	QString header = file.readAll();
	header = commentFreeString(header);
	header = removeImplementations(header);

	QRegExp functions("[^A-Za-z0-9~_]~?[_A-Za-z0-9]+(\\s)*\\([^\\)]*\\)");
	QRegExp operators("[^A-Za-z0-9~_]operator[>!=<\\+\\*-\\|\\^&/]+(\\s)*\\([^\\)]*\\)");
	unsigned int virtCount = virtualMethodsCount(header);
	unsigned int localCount = header.count(functions) + header.count(operators) - virtCount - macrosesWithParameters(header);

	totalVirtualCount += virtCount;
	return localCount;
}


void dirTreeInitialization(QString name, int localTesting, int localDocumented, int localTests = 0, bool isIgnored = false, bgcolors color = white)
{
	dirTree = new DirTree(name, localTesting, localDocumented, localTests, isIgnored, color);
}

// without interface methods
void totalFunctionCount(QString dir, bool dirIsIgnored, DirNode *parent)
{
	DirNode *node;
	QString filter = "*.h";
	QDir directory(dir);
	QStringList fileList = directory.entryList(QDir::Files);
	QStringList filterFileList = fileList.filter(QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard));
	QStringList dirList = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
	unsigned int localCount = 0;
	unsigned int oldDocumentedCount = totalDocumentedCount;

	if (!parent || !dirIsIgnored) {
		for (int i = 0; i < filterFileList.length(); ++i)
		{
			QString file = filterFileList.at(i);
			file.chop(2);
			localCount += localFunctionCount(dir, file);
		}
		totalTestingFunCount += localCount;

		if (!dirTree) {
			dirTreeInitialization(dir, localCount, totalDocumentedCount - oldDocumentedCount, 0, false, orange);
			node = dirTree->getRoot();
		} else {
			node = DirTree::createNode(dir, localCount, totalDocumentedCount - oldDocumentedCount, 0, false, nextColor(parent->color));
			dirTree->addChild(node, parent);
		}
		for (int i = 0; i < dirList.length(); ++i)
		{
			bool followDirIsIgnored = false;
			for (int j = 0; j < ignoreFiles.length(); ++j)
			{
				QRegExp exReg(".*" + ignoreFiles.at(j) + ".*");
				if (exReg.exactMatch(dir + dirList.at(i) + "/")) {
					followDirIsIgnored = true;
					break;
				}
			}
			totalFunctionCount(dir + dirList.at(i) + "/", followDirIsIgnored, node);
		}
	} else {
		node = DirTree::createNode(dir, 0, 0, 0, true, nextColor(parent->color));
		dirTree->addChild(node, parent);
		for (int i = 0; i < dirList.length(); ++i)
		{
			totalFunctionCount(dir + dirList.at(i) + "/", true, node);
		}
	}
}

void fillIgnoredFiles()
{
	QFile file("~testignore");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}

	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = in.readLine();
		ignoreFiles.append(line);
	}
	file.close();
}

void fillPathesForTest()
{
	QFile file("testpathes");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}

	QTextStream in(&file);
	while (!in.atEnd())
	{
		QString line = in.readLine();
		testpathes.append(line);
	}
	file.close();
}

void fillOutputStats(DirNode *node)
{
	QString name1 = "<tr bgcolor =" + colorToString(node->color) + "><td>";
	if (node->childNode) {
		name1.append("<a href=\"#" + node->childNode->name + "\">");
	}
	name1.append((node->isIgnored ? "<i>~" : "") + node->name + (node->isIgnored ? "</i>" : "") + "</a></td>");
	QString name2 = "<td> <a name=\"" + node->name + "\"></a>" + QString::number(node->localTesting) + "</td>";
	QString name3 = "<td>" + QString::number(node->localDocumented)+ "</td>";
	QString name4 = "<td>" + QString::number(node->localTests) + "</td>";
	QString name5 = "<td>" + QString::number(node->totalTesting) + "</td>";
	QString name6 = "<td>" + QString::number(node->totalDocumented) + "</td>";
	QString name7 = "<td>" + QString::number(node->totalTests) + "</td></tr>";

	outputStats.append(name1 + name2 + name3 + name4 + name5 + name6 + name7);

}

void fillNodeLogHTML(DirNode *node)
{
	if (node == dirTree->getRoot()) {
		fillOutputStats(node);
	}
	DirNode *tmpNode = node->childNode;
	if (tmpNode) {
		if (tmpNode->rightNode) {
			fillOutputStats(tmpNode);
			while (tmpNode->rightNode != NULL) {
				tmpNode = tmpNode->rightNode;
				fillOutputStats(tmpNode);
			}
			tmpNode = node->childNode;
			fillNodeLogHTML(tmpNode);
			while (tmpNode->rightNode != NULL) {
				tmpNode = tmpNode->rightNode;
				fillNodeLogHTML(tmpNode);
			}
		} else {
			fillOutputStats(node->childNode);
			fillNodeLogHTML(node->childNode);
		}
	}
}

void fillLog(QString fileName)
{
	QString frame1 = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
			+ QString("<html><head><meta http-equiv=Content-Type content=text/html; charset=utf-8><title>method's ")
			+ fileName + "</title><caption>" + "Project method's information" + "</caption></head><body><table border=1 "
			+ "width=100% cellpadding=5 cols=7 bgcolor=white><tr bgcolor = pink><th width=40%>path</th><th width=10%>testing</th><th width=10%>doc</th><th width=10%>tests"
			+ "</th><th width=10%>totaltesting</th><th width=10%>totaldoc</th><th width=10%>totaltests</th width=10%></tr>";
	QString frame2 = "</table></body></html>";
	outputStats.append(frame1);
	fillNodeLogHTML(dirTree->getRoot());
	outputStats.append(frame2);
}

int main(int argc, char *argv[])
{
	QTime time;
	time.start();

	fillIgnoredFiles();
	fillPathesForTest();
	for (int j = 0; j < testpathes.length(); ++j)
	{
		totalFunctionCount(testpathes.at(j), false, NULL);
		dirTree->calculateTotalData();
		QString fileName = "log" + QString::number(j) + ".html";
		QFile outputFile(fileName);
		if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			return 0;
		}
		fillLog(fileName);
		QTextStream out(&outputFile);
		out << outputStats;
		totalTestingFunCount = 0;
		totalVirtualCount = 0;
		totalDocumentedCount = 0;
		dirTree->~DirTree();
		dirTree = NULL;
		outputStats.clear();
		outputFile.close();
	}

	qDebug() << "time of execution: " + QString::number(time.elapsed()) + " ms";
	return 0;
}
