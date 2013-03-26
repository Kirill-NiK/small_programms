#include <QDir>
#include <QString>
#include <QFile>
#include <stdio.h>
#include <QRegExp>
#include <QtDebug>
#include <QTime>

unsigned int totalTestingFunCount = 0;
unsigned int totalVirtualCount = 0;
unsigned int totalDocumentedCount = 0;

QStringList ignoreFiles;
QString output;
QString outputStats;
QStringList testpathes;

QString commentFreeString(QString str)
{
	int docCount = str.count(QString("/// ")); // this counting is used because we have style guide
	docCount += str.count(QString("/** "));
	docCount += str.count(QString("//! "));
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
			output.append("~" + fileName + "\n");
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

	QRegExp reg("[^A-Za-z0-9~_]~?[_A-Za-z0-9]+(\\s)*\\([^;]*;");
	unsigned int virtCount = virtualMethodsCount(header);
	unsigned int localCount = header.count(reg) - virtCount - macrosesWithParameters(header);

	totalVirtualCount += virtCount;
	return localCount;
}

// without interface methods
void totalFunctionCount(QString dir)
{
	QString filter = "*.h";
	QDir directory(dir);
	QStringList fileList = directory.entryList(QDir::Files);
	QStringList filterFileList = fileList.filter(QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard));
	unsigned int localCount = 0;
	unsigned int oldDocumentedCount = totalDocumentedCount;
	for (int i = 0; i < filterFileList.length(); ++i)
	{
		QString file = filterFileList.at(i);
		file.chop(2);
		localCount += localFunctionCount(dir, file);
	}
	totalTestingFunCount += localCount;
	QStringList dirList = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
	output.append(dir + "\n\ttesting methods: " + QString::number(localCount) + "\tdocumented: " + QString::number(totalDocumentedCount - oldDocumentedCount) + "\n");
	for (int i = 0; i < dirList.length(); ++i)
	{
		bool isMatch = false;
		for (int j = 0; j < ignoreFiles.length(); ++j)
		{
			QRegExp exReg(".*" + ignoreFiles.at(j) + ".*");
			if (exReg.exactMatch(dir + dirList.at(i) + "/")) {
				isMatch = true;
				output.append("~" + dir + dirList.at(i) + "/\n");
				break;
			}
		}
		if (isMatch) {
			continue;
		}
		totalFunctionCount(dir + dirList.at(i) + "/");
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

void fillLog()
{
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
		totalFunctionCount(testpathes.at(j));
		fillLog();
		outputStats.prepend(testpathes.at(j) + "\n");
		QFile outputFile("log" + QString::number(j) + ".txt");
		if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			return 0;
		}
		QTextStream out(&outputFile);
		out << outputStats << output;
		totalTestingFunCount = 0;
		totalVirtualCount = 0;
		totalDocumentedCount = 0;
		outputStats.clear();
		output.clear();
		outputFile.close();
	}

	qDebug() << "time of execution: " + QString::number(time.elapsed()) + " ms";
	return 0;
}
