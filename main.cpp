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
QString output;
QString outputStats;
QStringList testpathes;
DirTree *dirTree = NULL;

QString commentFreeString(QString str)
{
	// this counting is used and accurate because we have style guide
	int docCount = str.count(QRegExp("\\n[^/\\n]*\\n[^\\n]/// [^\\n]*\\n[^/\\(]*\\("));
	docCount += str.count(QString("/** "));
	docCount += str.count(QRegExp("\\n[^/\\n]*\\n[^\\n]//! [^\\n]*\\n[^/\\(]*\\("));
	totalDocumentedCount += docCount;
	QRegExp reg = QRegExp("/\\*.*\\*/");
	reg.setMinimal(true);
	str.remove(reg);
	str.remove(QRegExp("//[^\\n$]*(\\n|$)"));
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
			// output.append("~" + fileName + "\n");  это в моем новом логе пока не отображается. так как файл. а не папка
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
		// qDebug("We have problems files - 0");
		return 0;
	}

	QString header = file.readAll();
	header = commentFreeString(header);

	QRegExp reg("[^A-Za-z0-9~_]~?[_A-Za-z0-9]+(\\s)*\\([^;]*;");
	unsigned int virtCount = virtualMethodsCount(header);
	unsigned int localCount = header.count(reg) - virtCount - macrosesWithParameters(header);

	totalVirtualCount += virtCount;
	return localCount;
}


void dirTreeInitialization(QString name, int localTesting, int localDocumented, int localTests = 0, bool isIgnored = false)
{
	dirTree = new DirTree(name, localTesting, localDocumented, localTests, isIgnored);
}

// without interface methods
// считаем, что первый путь не игнорится. никогда. добавить в рид.ми.
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
			dirTreeInitialization(dir, localCount, totalDocumentedCount - oldDocumentedCount);
			node = dirTree->getRoot();
		} else {
			node = DirTree::createNode(dir, localCount, totalDocumentedCount - oldDocumentedCount);
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
		node = DirTree::createNode(dir, 0, 0, 0, true);
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
		qDebug("We have problems files - 1");
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
		qDebug("We have problems files - 2");
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

void fillLog()
{
	// пройтись по всему дереву типа name: директория + 4 числа вывести:
/*	output.append(dir + "\n\tin this folder:\ttesting methods: " + QString::number() +
			"\tdocumented: " + QString::number() +
			// тестов
			"\n\tin subfolders:\ttesting methods: " + QString::number() +
			"\tdocumented: " + QString::number() + "\n");
			// + тестов*/
	//output.append("~" + dir + dirList.at(i) + "/\n"); // надо тут тоже изменить. (в лог занести)

	output.prepend("\nignored pathes:\n" + ignoreFiles.join("\n") + "\n----------\n\n");
	outputStats.append("\n\t\ttesting: " + QString::number(totalTestingFunCount) + "\n");
	outputStats.append("\t\tvirtual: " + QString::number(totalVirtualCount) + "\n");
	outputStats.append("\t\ttotal: " + QString::number(totalVirtualCount + totalTestingFunCount) + "\n");
	outputStats.append("\t\tdocumented: " + QString::number(totalDocumentedCount) + "\n\n");
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
		fillLog();
		outputStats.prepend(testpathes.at(j) + "\n");
		QFile outputFile("log" + QString::number(j) + ".txt");
		if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			qDebug("We have problems files - 3");
			return 0;
		}
		QTextStream out(&outputFile);
		out << outputStats << output;
		totalTestingFunCount = 0;
		totalVirtualCount = 0;
		totalDocumentedCount = 0;
		dirTree->~DirTree();
		dirTree = NULL;
		outputStats.clear();
		output.clear();
		outputFile.close();
	}

	qDebug() << "time of execution: " + QString::number(time.elapsed()) + " ms";
	return 0;
}
